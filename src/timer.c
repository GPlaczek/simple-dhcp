#include "timer.h"

#include <sys/timerfd.h>
#include <string.h>

int timer_init(struct timer *timer, struct leaselist *ll) {
	int tfd;
	tfd = timerfd_create(CLOCK_BOOTTIME, TFD_CLOEXEC);
	if (tfd < 0)
		return 1;

	timer->tfd = tfd;
	timer->ll = ll;

	return 0;
}

int timer_arm(struct timer *timer) {
	struct itimerspec newtime, oldtime;
	int err, ind;
	long elapsed, mintime = -1;
	memset(&newtime, 0, sizeof(newtime));
	memset(&oldtime, 0, sizeof(oldtime));

	err = timerfd_gettime(timer->tfd, &oldtime); 
	if (err < 0)
		return 1;

	elapsed = timer->last_armed - oldtime.it_value.tv_sec;

	for (int i = 0; i < timer->ll->len; i++) {
		if (timer->ll->lease_vec[i].efd < 0)
			continue;

		timer->ll->lease_vec[i].efd -= elapsed;

		if (timer->ll->lease_vec[i].efd < mintime || mintime < 0) {
			mintime = timer->ll->lease_vec[i].efd;
			ind = i;
		}
	}
	if (mintime < 0)  // no leases
		return 0;

	newtime.it_value.tv_sec = mintime;
	timer->last_armed = mintime;
	timer->current_lease = ind;
	err = timerfd_settime(timer->tfd, 0, &newtime, NULL);
	if (err < 0)
		return 1;

	return 0;
}
