#include "rfc2131.h"
#include "leases.h"

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define DEFAULT_LEASE_TIME  htonl(86400)

struct rfc2131_dhcp_msg dhcp_msg, dhcp_resp;
struct leaselist llist;

struct dhcp_option {
	uint8_t len;
	uint8_t *ptr;
};
struct dhcp_option client_id, parameter_request_list;

struct lease *process_dhcp_msg() {
	struct lease *lease = NULL;

	if (dhcp_msg.op != RFC2131_OP_BOOTREQUEST)
		goto exit;

	if (*(uint32_t *)dhcp_msg.options != RFC2131_MAGIC_COOKIE)
		goto exit;

	uint8_t *option_ptr = dhcp_msg.options + 4;
	uint8_t msgtype = 0;

	while (option_ptr < dhcp_msg.options + 312) {
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
		for (int i = 0; i < llist.len; i++) {
			if (llist.lease_vec[i].xid == dhcp_msg.xid) {
				lease = &llist.lease_vec[i];
				lease->efd = 1;
				break;
			}
		}

		if (lease != NULL)
			break;
	case RFC2131_OPTION_MSGTYPE_DHCPDISCOVER:
		lease = leaselist_get_lease(&llist);
		lease->xid = dhcp_msg.xid;
		memcpy(lease->chaddr, dhcp_msg.chaddr, 16);
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

void init_dhcp_resp() {
	memset(&dhcp_resp, 0, sizeof(dhcp_resp));
	dhcp_resp.op = RFC2131_OP_BOOTREPLY;
	dhcp_resp.siaddr = htonl(inet_addr("192.168.1.1"));
	dhcp_resp.hops = 0;
	dhcp_resp.secs = 0;
	dhcp_resp.flags = 0;
	dhcp_resp.siaddr = 0;
	dhcp_resp.giaddr = 0;
	dhcp_resp.sname[0] = 0;
	dhcp_resp.file[0] = 0;

	uint32_t *options32 = (uint32_t *) dhcp_resp.options;
	options32[0] = RFC2131_MAGIC_COOKIE;
}

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

int main(int argv, char **args) {
	int err, sfd, sockopt;
	ssize_t msg_size;
	struct sockaddr dgram_addr;
	socklen_t addrlen = sizeof(dgram_addr);

	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd < 0) {
		fprintf(stderr, "Could not create socket (error %d)\n", errno);
		return 1;
	}
	sockopt=1;
	err = setsockopt(sfd, SOL_SOCKET, SO_BROADCAST, &sockopt, sizeof(sockopt));
	if (err < 0) {
		fprintf(stderr, "Could not set socket broadcast option (error %d)\n", errno);
		return 1;
	}

	struct sockaddr_in bind_addr = { AF_INET, htons(67), INADDR_ANY };
	err = bind(sfd, (struct sockaddr*) &bind_addr, sizeof(bind_addr));
	if (err < 0) {
		fprintf(stderr, "Could not bind to the given address (error %d)\n", errno);
		return 1;
	}

	leaselist_init(&llist, htonl(inet_addr("192.168.16.0")), htonl(inet_addr("255.255.255.0")));
	init_dhcp_resp();

	struct sockaddr_in s1;
	s1.sin_family = AF_INET;
	s1.sin_port = htons(68);
	s1.sin_addr.s_addr = INADDR_BROADCAST;

    client_id.len = 0;
    parameter_request_list.len = 0;

	for (;;) {
		memset(&dhcp_msg, 0, sizeof(msg_size));
		msg_size = recvfrom(sfd, &dhcp_msg, sizeof(dhcp_msg), 0, &dgram_addr, &addrlen);

		if (msg_size <= 0)
			fprintf(stderr, "Error receiving the message (error %d)\n", errno);

		struct lease *l = process_dhcp_msg();
		if (l == NULL) {
		} else {
			dhcp_resp.htype = dhcp_msg.htype;
			dhcp_resp.hlen = dhcp_msg.hlen;
			dhcp_resp.xid = dhcp_msg.xid;
			dhcp_resp.yiaddr = htonl(l->ipaddr);
			dhcp_resp.giaddr = dhcp_msg.giaddr;
			memcpy(dhcp_resp.chaddr, dhcp_msg.chaddr, 16);
			int i = 4;

			if (l->efd == -1) {
				dhcp_resp.ciaddr = 0;
				i += write_option8(dhcp_resp.options + i, RFC2131_OPTION_MSGTYPE, RFC2131_OPTION_MSGTYPE_DHCPOFFER);
			} else {
				dhcp_resp.ciaddr = dhcp_msg.ciaddr;
				i += write_option8(dhcp_resp.options + i, RFC2131_OPTION_MSGTYPE, RFC2131_OPTION_MSGTYPE_DHCPACK);
			}

			i+= write_option32(dhcp_resp.options + i, RFC2131_OPTION_IP_ADDRESS_LEASE_TIME, DEFAULT_LEASE_TIME);

			if (client_id.len > 0) {
				i+= write_option(dhcp_resp.options + i, RFC2131_OPTION_CLIENT_ID, client_id.ptr, client_id.len);
				client_id.len = 0;
			}

			for (int j = 0; j < parameter_request_list.len; j++) {
				switch (parameter_request_list.ptr[j]) {
				case RFC2131_OPTION_SUBNET_MASK:
					i += write_option32(dhcp_resp.options + i, RFC2131_OPTION_SUBNET_MASK, htonl(llist.netmask)); break;
				}
			}

			dhcp_resp.options[i++] = 0xff;
		}

		msg_size = sendto(sfd, &dhcp_resp, sizeof(dhcp_resp), 0, (const struct sockaddr *)&s1, sizeof(s1));

		if (msg_size < 0)
			fprintf(stderr, "Could not send message (error %d)\n", errno);
	};

	close(sfd);
	return 0;
}
