#ifndef dhcp_h_INCLUDED
#define dhcp_h_INCLUDED

#include "rfc2131.h"
#include "leases.h"
#include "timer.h"

struct dhcp_args {
	char *config_filename, *gateway, *dns, *address, *netmask, *lease_time, *log_level;
};

struct dhcp_server {
	in_addr_t gateway, dns;
	struct leaselist llist;
	struct timer timer;
};

void process_dhcp_msg(
	struct rfc2131_dhcp_msg *dhcp_msg,
	struct rfc2131_dhcp_msg *dhcp_resp,
	struct dhcp_server *srv
);
void create(struct dhcp_args *cli, struct dhcp_server *srv);
void del(struct dhcp_server *srv);

#endif // dhcp_h_INCLUDED
