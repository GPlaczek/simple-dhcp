#ifndef timer_h_INCLUDED
#define timer_h_INCLUDED

#include "leases.h"

struct timer {
	int tfd, current_lease;
	long last_armed;
	struct leaselist *ll;
};

int timer_init(struct timer *timer, struct leaselist *ll);
int timer_arm(struct timer *timer, struct lease *lease);

#endif // timer_h_INCLUDED
