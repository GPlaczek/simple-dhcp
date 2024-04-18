#include "rfc2131.h"
#include "leases.h"
#include "dhcp.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define DEFAULT_LEASE_TIME  htonl(86400)

struct dhcp_option {
	uint8_t len;
	uint8_t *ptr;
};
struct dhcp_option client_id, parameter_request_list;


size_t write_option8(uint8_t *options, uint8_t option_type, uint8_t option) {
	options[0] = option_type;
	options[1] = 0x01;
	options[2] = option;
	return 3;
}

size_t write_option32(uint8_t *options, uint8_t option_type, uint32_t option) {
	options[0] = option_type;
	options[1] = 0x04;
	uint32_t *view32 = (uint32_t *)(options + 2);
	*view32 = option;
	return 6;
}

size_t write_option(uint8_t *restrict options, uint8_t option_type, void *restrict option, size_t len) {
	options[0] = option_type;
	options[1] = len;
	memcpy(options + 2, option, len);
	return len + 2;
}

struct lease *get_new_lease(
	struct rfc2131_dhcp_msg *dhcp_msg,
	struct leaselist *llist
) {
	struct lease *lease = NULL;

	if (dhcp_msg->op != RFC2131_OP_BOOTREQUEST)
		goto exit;

	if (*(uint32_t *)dhcp_msg->options != RFC2131_MAGIC_COOKIE)
		goto exit;

	uint8_t *option_ptr = dhcp_msg->options + 4;
	uint8_t msgtype = 0;

	while (option_ptr < dhcp_msg->options + 312) {
		uint8_t option_id = *(option_ptr++);
		uint8_t option_len = *(option_ptr++);

		switch (option_id) {
		case RFC2131_OPTION_MSGTYPE:
			msgtype = *option_ptr;
			break;
		case RFC2131_OPTION_CLIENT_ID:
			client_id.len = option_len;
			client_id.ptr = option_ptr;
			break;
		case RFC2131_OPTION_PARAMETER_REQUEST_LIST:
			parameter_request_list.len = option_len;
			parameter_request_list.ptr = option_ptr;
			break;
		}

		option_ptr += option_len;
	}

	switch (msgtype) {
	case RFC2131_OPTION_MSGTYPE_DHCPREQUEST:
		for (int i = 0; i < llist->len; i++) {
			if (llist->lease_vec[i].xid == dhcp_msg->xid) {
				lease = &llist->lease_vec[i];
				lease->efd = 1;
				break;
			}
		}

		if (lease != NULL)
			break;
		// fall through
	case RFC2131_OPTION_MSGTYPE_DHCPDISCOVER:
		lease = leaselist_get_lease(llist);
		lease->xid = dhcp_msg->xid;
		memcpy(lease->chaddr, dhcp_msg->chaddr, 16);
		goto exit;
	case RFC2131_OPTION_MSGTYPE_DHCPDECLINE:
	case RFC2131_OPTION_MSGTYPE_DHCPRELEASE:
	case RFC2131_OPTION_MSGTYPE_DHCPINFORM:
	default:
		// Invalid dhcp option
		goto exit;
	}
exit:
	return lease;
}

void prepare_response(
	struct rfc2131_dhcp_msg *dhcp_msg,
	struct rfc2131_dhcp_msg *dhcp_resp,
	struct leaselist *llist,
	struct lease *lease
) {
	if (lease == NULL)
		return;

	dhcp_resp->htype = dhcp_msg->htype;
	dhcp_resp->hlen = dhcp_msg->hlen;
	dhcp_resp->xid = dhcp_msg->xid;
	dhcp_resp->yiaddr = htonl(lease->ipaddr);
	dhcp_resp->giaddr = dhcp_msg->giaddr;
	memcpy(dhcp_resp->chaddr, dhcp_msg->chaddr, 16);
	int i = 4;

	if (lease->efd == -1) {
		dhcp_resp->ciaddr = 0;
		i += write_option8(dhcp_resp->options + i, RFC2131_OPTION_MSGTYPE, RFC2131_OPTION_MSGTYPE_DHCPOFFER);
	} else {
		dhcp_resp->ciaddr = dhcp_msg->ciaddr;
		i += write_option8(dhcp_resp->options + i, RFC2131_OPTION_MSGTYPE, RFC2131_OPTION_MSGTYPE_DHCPACK);
	}

	i+= write_option32(dhcp_resp->options + i, RFC2131_OPTION_IP_ADDRESS_LEASE_TIME, DEFAULT_LEASE_TIME);

	if (client_id.len > 0) {
		i+= write_option(dhcp_resp->options + i, RFC2131_OPTION_CLIENT_ID, client_id.ptr, client_id.len);
		client_id.len = 0;
	}

	for (int j = 0; j < parameter_request_list.len; j++) {
		switch (parameter_request_list.ptr[j]) {
		case RFC2131_OPTION_SUBNET_MASK:
			i += write_option32(dhcp_resp->options + i, RFC2131_OPTION_SUBNET_MASK, htonl(llist->netmask)); break;
		}
	}
	dhcp_resp->options[i++] = 0xff;
}

void process_dhcp_msg(
	struct rfc2131_dhcp_msg *dhcp_msg,
	struct rfc2131_dhcp_msg *dhcp_resp,
	struct leaselist *llist
) {
	struct lease *lease = get_new_lease(dhcp_msg, llist);
	prepare_response(dhcp_msg, dhcp_resp, llist, lease);
}
