#include "leases.h"

#include <stdlib.h>

#define INITIAL_CAP 256

void leaselist_init(struct leaselist *ll, in_addr_t na, in_addr_t nm) {
	ll->netaddr = na;
	ll->netmask = nm;

	unsigned int initial_cap, net_size;
	net_size = ~nm;
	if (net_size > INITIAL_CAP)
		initial_cap = INITIAL_CAP;
	else
		initial_cap = net_size;

	ll->lease_vec = malloc(sizeof(struct lease) * initial_cap);
	ll->cap = initial_cap;
	ll->net_size = net_size;
}

unsigned int __addr_to_ind(struct leaselist *ll, in_addr_t addr) {
	return addr - ll->netaddr - 1;
}

in_addr_t __ind_to_addr(struct leaselist *ll, unsigned int ind) {
	return ind + ll->netaddr + 1;
}

struct lease *leaselist_get_lease(struct leaselist *ll) {
	struct lease *lease = NULL;

	int i = 0;
	if (ll->len < ll->net_size) {
		i = ll->len;
		lease = &ll->lease_vec[ll->len];
		ll->len++;
	} else {
		for (lease = &ll->lease_vec[i]; lease->efd == -1; lease=&ll->lease_vec[++i]);
	}

	lease->ipaddr = __ind_to_addr(ll, ll->len);

	return lease;
}
