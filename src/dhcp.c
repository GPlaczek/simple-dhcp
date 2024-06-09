#include "rfc2131.h"
#include "leases.h"
#include "dhcp.h"
#include "config.h"
#include "log.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>


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

void write_response(
	struct rfc2131_dhcp_msg *dhcp_msg,
	struct rfc2131_dhcp_msg *dhcp_resp,
	struct dhcp_server *srv,
	struct lease *lease,
	uint8_t msgtype
) {
	int i = 4;

	dhcp_resp->ciaddr = 0;
	i += write_option8(dhcp_resp->options + i, RFC2131_OPTION_MSGTYPE, msgtype);

	dhcp_resp->htype = dhcp_msg->htype;
	dhcp_resp->hlen = dhcp_msg->hlen;
	dhcp_resp->xid = dhcp_msg->xid;
	if (lease != NULL) dhcp_resp->yiaddr = htonl(lease->ipaddr);
	dhcp_resp->giaddr = dhcp_msg->giaddr;
	memcpy(dhcp_resp->chaddr, dhcp_msg->chaddr, 16);

	if (lease != NULL)
		i+= write_option32(dhcp_resp->options + i, RFC2131_OPTION_IP_ADDRESS_LEASE_TIME, htonl((uint32_t)lease->efd));

	if (client_id.len > 0) {
		i+= write_option(dhcp_resp->options + i, RFC2131_OPTION_CLIENT_ID, client_id.ptr, client_id.len);
		client_id.len = 0;
	}

	for (int j = 0; j < parameter_request_list.len; j++) {
		switch (parameter_request_list.ptr[j]) {
		case RFC2131_OPTION_SUBNET_MASK:
			i += write_option32(dhcp_resp->options + i, RFC2131_OPTION_SUBNET_MASK, htonl(srv->llist.netmask)); break;
		case RFC2131_OPTION_DOMAIN_NAME_SERVER:
			i += write_option32(dhcp_resp->options + i, RFC2131_OPTION_DOMAIN_NAME_SERVER, srv->dns); break;
		}
	}

	dhcp_resp->options[i++] = 0xff;
}

void write_nak(struct rfc2131_dhcp_msg *dhcp_resp) {
	int i = 4;

	dhcp_resp->ciaddr = 0;
	i += write_option8(dhcp_resp->options + i, RFC2131_OPTION_MSGTYPE, RFC2131_OPTION_MSGTYPE_DHCPNAK);
}

int process_dhcp_msg(
	struct rfc2131_dhcp_msg *dhcp_msg,
	struct rfc2131_dhcp_msg *dhcp_resp,
	struct dhcp_server *srv
) {
	in_addr_t __addr;
	uint8_t *__u8v, msgtype = 0, *option_ptr = dhcp_msg->options + 4;
	char human_haddr[64];
	struct lease *lease = NULL;

	human_readable_haddr(human_haddr, dhcp_msg->chaddr, dhcp_msg->hlen);
	hslog(LOGLEVEL_DEBUG, human_haddr, "Got message\n");

	if (dhcp_msg->op != RFC2131_OP_BOOTREQUEST) {
		hslog(LOGLEVEL_WARN, human_haddr, "Invalid dhcp operation type\n");
		write_nak(dhcp_resp);
		return 1;
	}

	if (*(uint32_t *)dhcp_msg->options != RFC2131_MAGIC_COOKIE) {
		hslog(LOGLEVEL_WARN, human_haddr, "Invalid magic cookie\n");
		write_nak(dhcp_resp);
		return 1;
	}

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
		hslog(LOGLEVEL_INFO, human_haddr, "DHCPREQUEST\n");
		lease = leaselist_find_lease(&srv->llist, dhcp_msg->chaddr, dhcp_msg->hlen, dhcp_msg->xid);
		if (lease == NULL) {
			lease = leaselist_get_lease(&srv->llist, dhcp_msg->chaddr, dhcp_msg->hlen);
			lease->xid = dhcp_msg->xid;
			memcpy(lease->chaddr, dhcp_msg->chaddr, 16);
		}

		lease->efd = srv->llist.max_lease_time;
		timer_arm(&srv->timer, lease);

		__addr = htonl(lease->ipaddr);
		__u8v = (uint8_t *)&__addr;
		hslog(LOGLEVEL_INFO, human_haddr, "Leasing %d.%d.%d.%d (xid: %d)\n",
			__u8v[0], __u8v[1], __u8v[2], __u8v[3], lease->xid);

		write_response(dhcp_msg, dhcp_resp, srv, lease, RFC2131_OPTION_MSGTYPE_DHCPACK);
		dhcp_resp->ciaddr = dhcp_msg->ciaddr;

		return 1;
	case RFC2131_OPTION_MSGTYPE_DHCPDISCOVER:
		hslog(LOGLEVEL_INFO, human_haddr, "DHCPDISCOVER\n");
		lease = leaselist_get_lease(&srv->llist, dhcp_msg->chaddr, dhcp_msg->hlen);
		if (lease == NULL) {
			write_response(dhcp_msg, dhcp_resp, srv, lease, RFC2131_OPTION_MSGTYPE_DHCPNAK);
			return 1;
		}

		lease->xid = dhcp_msg->xid;
		__addr = htonl(lease->ipaddr);
		__u8v = (uint8_t *)&__addr;
		hslog(LOGLEVEL_INFO, human_haddr, "Offering %d.%d.%d.%d (xid: %d)\n",
			__u8v[0], __u8v[1], __u8v[2], __u8v[3], lease->xid);
		memcpy(lease->chaddr, dhcp_msg->chaddr, 16);
		write_response(dhcp_msg, dhcp_resp, srv, lease, RFC2131_OPTION_MSGTYPE_DHCPOFFER);
		return 1;
	case RFC2131_OPTION_MSGTYPE_DHCPDECLINE:
		hslog(LOGLEVEL_INFO, human_haddr, "DHCPDECLINE\n");
		lease = leaselist_find_lease(&srv->llist, dhcp_msg->chaddr, dhcp_msg->hlen, dhcp_msg->xid);

		if (lease != NULL)
			lease->efd = LEASE_DECLINED;
		else
			hslog(LOGLEVEL_WARN, human_haddr, "No lease for the DHCPDECLINE\n");

		return 0;
	case RFC2131_OPTION_MSGTYPE_DHCPRELEASE:
		hslog(LOGLEVEL_INFO, human_haddr, "DHCPDECLINE\n");
		lease = leaselist_find_lease(&srv->llist, dhcp_msg->chaddr, dhcp_msg->hlen, dhcp_msg->xid);

		if (lease != NULL)
			lease->efd = LEASE_RELEASED;
		else
			hslog(LOGLEVEL_WARN, human_haddr, "No lease for the DHCPRELEASE\n");

		return 0;
	case RFC2131_OPTION_MSGTYPE_DHCPINFORM:
		hslog(LOGLEVEL_INFO, human_haddr, "DHCPINFORM\n");
		write_response(dhcp_msg, dhcp_resp, srv, NULL, RFC2131_OPTION_MSGTYPE_DHCPACK);
		return 1;
	default:
		// Invalid dhcp option
		return 0;
	}
}

void configure(struct dhcp_server *srv, struct dhcp_config *conf) {
	srv->gateway = conf->gateway;
	srv->dns = conf->dns;

	srv->llist.netaddr = conf->address;
	srv->llist.netmask = conf->netmask;
	srv->llist.max_lease_time = conf->lease_time;

	leaselist_init(&srv->llist);
	timer_init(&srv->timer, &srv->llist);
}
