#ifndef leases_h_INCLUDED
#define leases_h_INCLUDED

#include <stdint.h>
#include <arpa/inet.h>

struct lease {
	struct in_addr ipaddr;
	uint8_t chaddr[16];
	int efd;
};

struct lease get_lease(struct in_addr netaddr);


#endif // leases_h_INCLUDED
