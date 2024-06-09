#include "leases.h"

#include <stdlib.h>
#include <string.h>

#define INITIAL_CAP 256

void leaselist_init(struct leaselist *ll) {

	unsigned int initial_cap, net_size;
	net_size = ~ll->netmask;
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

struct lease *leaselist_get_lease(struct leaselist *ll, uint8_t *hwaddr, uint8_t hwlen) {
	struct lease *lease = NULL, *empty = NULL;

	int i = 0;
	for (
		lease = &ll->lease_vec[i];
		lease != NULL && memcmp(lease->chaddr, hwaddr, hwlen) && i < ll->len;
		lease = &ll->lease_vec[++i]
	) {
		if (empty == NULL && lease != NULL && lease->efd == LEASE_FREE)
			empty = lease;
	}
	if (ll->len < ll->net_size) {
		i = ll->len;
		lease = &ll->lease_vec[ll->len];
		ll->len++;
	} else { lease = empty; }

	if (lease == NULL)
		return NULL;

	lease->ipaddr = __ind_to_addr(ll, ll->len);
	lease->efd = LEASE_OFFERED;

	return lease;
}

struct lease *leaselist_find_lease(struct leaselist *ll, uint8_t *hwaddr, uint8_t hwlen, uint32_t xid) {
	for (int i = 0; i < ll->len; i++) {
		// Use a lease if it has been offered in the same transaction or if
		// it was leased to the same hardware address before
		if (ll->lease_vec[i].xid == xid ||
			!memcmp(ll->lease_vec[i].chaddr, hwaddr, hwlen)
		) {
			return &ll->lease_vec[i];
		}
	}

	return NULL;
}
