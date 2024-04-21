#include "rfc2131.h"
#include "dhcp.h"

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


struct rfc2131_dhcp_msg dhcp_msg, dhcp_resp;


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

int main(int argc, char **argv) {
	int err, sfd, sockopt;
	ssize_t msg_size;
	struct sockaddr dgram_addr;
	struct pollfd poll_fds[2];
	socklen_t addrlen = sizeof(dgram_addr);

	struct cli_args cli;
	memset(&cli, 0, sizeof(cli));
	parse_command_line(argc, argv, &cli);
	struct dhcp_server dhcpsrv;
	memset(&dhcpsrv, 0, sizeof(dhcpsrv));
	create(&cli, &dhcpsrv);

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

	struct sockaddr_in bind_addr = { AF_INET, htons(67), INADDR_ANY, 0 };
	err = bind(sfd, (struct sockaddr*) &bind_addr, sizeof(bind_addr));
	if (err < 0) {
		fprintf(stderr, "Could not bind to the given address (error %d)\n", errno);
		return 1;
	}

	init_dhcp_resp();

	struct sockaddr_in s1;
	s1.sin_family = AF_INET;
	s1.sin_port = htons(68);
	s1.sin_addr.s_addr = INADDR_BROADCAST;

	poll_fds[0].fd = dhcpsrv.timer.tfd;
	poll_fds[0].events = POLLIN;
	poll_fds[1].fd = sfd;
	poll_fds[1].events = POLLIN;

	for (;;) {
		poll(poll_fds, 2, -1);
		if (poll_fds[1].revents & POLLIN) {
			memset(&dhcp_msg, 0, sizeof(msg_size));
			msg_size = recvfrom(sfd, &dhcp_msg, sizeof(dhcp_msg), 0, &dgram_addr, &addrlen);

			if (msg_size <= 0)
				fprintf(stderr, "Error receiving the message (error %d)\n", errno);

			process_dhcp_msg(&dhcp_msg, &dhcp_resp, &dhcpsrv);

			msg_size = sendto(sfd, &dhcp_resp, sizeof(dhcp_resp), 0, (const struct sockaddr *)&s1, sizeof(s1));

			if (msg_size < 0)
				fprintf(stderr, "Could not send message (error %d)\n", errno);
		} else if (poll_fds[0].revents & POLLIN) {
			static long __void;
			struct lease *lease = &dhcpsrv.llist.lease_vec[dhcpsrv.timer.current_lease];
			memset(lease, 0, sizeof(*lease));
			lease->efd = -1;

			timer_arm(&dhcpsrv.timer);

			read(dhcpsrv.timer.tfd, &__void, 8);
		}
	};

	close(sfd);
	return 0;
}
