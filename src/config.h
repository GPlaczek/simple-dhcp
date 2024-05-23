#ifndef config_h_INCLUDED
#define config_h_INCLUDED

#include <netinet/in.h>
#include <net/if.h>


#define CONFIG_GATEWAY       "gateway"
#define CONFIG_ADDRESS       "address"
#define CONFIG_NETMASK       "netmask"
#define CONFIG_DNS           "dns"
#define CONFIG_LEASE_TIME    "lease-time"
#define CONFIG_LOG_LEVEL     "log-level"
#define CONFIG_NET_INTERFACE "net-interface"

struct dhcp_args {
	char *config_filename, *gateway, *dns, *address,
	     *netmask, *lease_time, *log_level, *net_interface;
};

struct dhcp_config {
	in_addr_t gateway, dns, address, netmask;
	int lease_time, log_level;
	char net_interface[IFNAMSIZ];
};

int parse_config(const char *config_path, struct dhcp_args *args);
int get_dhcp_config(struct dhcp_args *cli, struct dhcp_config *conf);

#endif // config_h_INCLUDED
