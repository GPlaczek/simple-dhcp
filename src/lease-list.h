#ifndef leaselist_h_INCLUDED
#define leaselist_h_INCLUDED

#include "leases.h"
#include <stddef.h>

#define LEASELIST_INITIALIZER(addr, netmask) { \
	.netaddr: addr, \
	.netmask: netmask, \
	.head: NULL, \
	.tail: NULL, \
}

struct leaselist_node {
	struct leaselist_node *next;
	struct lease *lease;
};

struct leaselist {
	struct in_addr netaddr;
	struct in_addr netmask;
	struct leaselist_node *head;
	struct leaselist_node *tail;
};

struct leaselist_node *add_lease(struct leaselist *ll);
int *remove_lease(struct leaselist *ll, struct lease *l);

#endif // leaselist_h_INCLUDED
