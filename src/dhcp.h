#ifndef dhcp_h_INCLUDED
#define dhcp_h_INCLUDED

#include "rfc2131.h"
#include "leases.h"
#include "cli.h"
#include "timer.h"

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
void create(struct cli_args *cli, struct dhcp_server *srv);
void del(struct dhcp_server *srv);

#endif // dhcp_h_INCLUDED
