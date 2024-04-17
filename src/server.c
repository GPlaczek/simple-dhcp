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

struct rfc2131_dhcp_msg dhcp_msg;
struct leaselist llist;

void print_dhcp_msg() {
	char addr_str[INET_ADDRSTRLEN];

	printf("op: %02x\n", dhcp_msg.op);
	printf("htype: %02x\n", dhcp_msg.htype);
	printf("hlen: %02x\n", dhcp_msg.hlen);
	printf("hops: %02x\n", dhcp_msg.hops);
	printf("xid: %08x\n", dhcp_msg.xid);
	printf("xid: %04x\n", dhcp_msg.secs);
	printf("xid: %04x\n", dhcp_msg.flags);
	inet_ntop(AF_INET, &(dhcp_msg.ciaddr), addr_str, INET_ADDRSTRLEN);
	printf("ciaddr: %s\n", addr_str);
	inet_ntop(AF_INET, &(dhcp_msg.yiaddr), addr_str, INET_ADDRSTRLEN);
	printf("yiaddr: %s\n", addr_str);
	inet_ntop(AF_INET, &(dhcp_msg.siaddr), addr_str, INET_ADDRSTRLEN);
	printf("siaddr: %s\n", addr_str);
	inet_ntop(AF_INET, &(dhcp_msg.giaddr), addr_str, INET_ADDRSTRLEN);
	printf("giaddr: %s\n", addr_str);
	for (int i = 0; i < 312; i++) {
		for (; i % 8 < 7; i++)
			printf("%02x ", dhcp_msg.options[i]);
		printf("\n");
	}
	printf("\n");
}

int check_magic_cookie(uint8_t *options) {
	printf("%08x\n", *(uint32_t *)options);
	printf("%08x\n", RFC2131_MAGIC_COOKIE);
	return *(uint32_t *)options == RFC2131_MAGIC_COOKIE;
}

struct lease *process_dhcp_msg() {
	struct lease *lease = NULL;

	if (dhcp_msg.op != RFC2131_OP_BOOTREQUEST)
		goto exit;

	if (!(check_magic_cookie(dhcp_msg.options)))
		goto exit;

	uint8_t *option_ptr = dhcp_msg.options + 4;
	uint8_t msgtype = 0;

	while (option_ptr < dhcp_msg.options + 312) {
		uint8_t option_id = *(option_ptr++);
		uint8_t option_len = *(option_ptr++);

		switch (option_id) {
		case RFC2131_OPTION_MSGTYPE:
			msgtype = *option_ptr;
		}

		option_ptr += option_len;
	}

	switch (msgtype) {
	case RFC2131_OPTION_MSGTYPE_DHCPDISCOVER:
		lease = leaselist_get_lease(&llist);
		goto exit;
	case RFC2131_OPTION_MSGTYPE_DHCPREQUEST:
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

	for (;;) {
		memset(&dhcp_msg, 0, sizeof(msg_size));
		msg_size = recvfrom(sfd, &dhcp_msg, sizeof(dhcp_msg), 0, &dgram_addr, &addrlen);

		if (msg_size > 0)
			print_dhcp_msg();
		else
			fprintf(stderr, "Error receiving the message (error %d)\n", errno);

		struct lease *l = process_dhcp_msg();
		if (l != NULL) {
			char __addrs[INET_ADDRSTRLEN];
			in_addr_t __addri = ntohl(l->ipaddr);
			inet_ntop(AF_INET, &__addri, __addrs, INET_ADDRSTRLEN);
			printf("About to give %s address\n", __addrs);
		}

		msg_size = sendto(sfd, "Hello", sizeof("Hello"), 0, &dgram_addr, addrlen);
		if (msg_size < 0)
			fprintf(stderr, "Could not send message (error %d)\n", errno);
	};

	close(sfd);
	return 0;
}
