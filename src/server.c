#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#define PAYLOAD_SIZE 65507

char dgram_buf[PAYLOAD_SIZE];

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

	for (;;) {
		printf("DUPA\n");
		msg_size = recvfrom(sfd, &dgram_buf, PAYLOAD_SIZE, 0, &dgram_addr, &addrlen);
		if (msg_size > 0) {
			printf("Message is %s\n", dgram_buf);
		} else {
			fprintf(stderr, "Error receiving the message (error %d)\n", errno);
		}

		memset(dgram_buf, 0, msg_size);

		msg_size = sendto(sfd, "Hello", sizeof("Hello"), 0, &dgram_addr, addrlen);
		if (msg_size < 0)
			fprintf(stderr, "Could not send message (error %d)\n", errno);
	};

	close(sfd);
	return 0;
}
