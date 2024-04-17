#ifndef leaselist_h_INCLUDED
#define leaselist_h_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <arpa/inet.h>

struct lease {
	in_addr_t ipaddr;
	uint8_t chaddr[16];
	int efd;
};

struct leaselist {
	in_addr_t netaddr;
	in_addr_t netmask;
	struct lease *lease_vec;
	int len, cap, net_size;
};

void leaselist_init(struct leaselist *ll, in_addr_t na, in_addr_t nm);
struct lease *leaselist_get_lease(struct leaselist *ll);

#endif // leaselist_h_INCLUDED
