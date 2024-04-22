#ifndef leaselist_h_INCLUDED
#define leaselist_h_INCLUDED

#include <stdint.h>
#include <arpa/inet.h>

#define LEASE_FREE    -1
#define LEASE_OFFERED -2


struct lease {
	uint32_t xid;
	in_addr_t ipaddr;
	uint8_t chaddr[16];
	int efd;
};

struct leaselist {
	in_addr_t netaddr, netmask;
	long max_lease_time;
	struct lease *lease_vec;
	int len, cap, net_size;
};

void leaselist_init(struct leaselist *ll, in_addr_t na, in_addr_t nm);
struct lease *leaselist_get_lease(struct leaselist *ll, uint8_t *hwaddr, uint8_t hwlen);

#endif // leaselist_h_INCLUDED
