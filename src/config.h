#ifndef config_h_INCLUDED
#define config_h_INCLUDED

#include "dhcp.h"

#define CONFIG_GATEWAY "gateway"
#define CONFIG_ADDRESS "address"
#define CONFIG_NETMASK "netmask"
#define CONFIG_DNS     "dns"

int parse_config(const char *config_path, struct dhcp_server *srv);

#endif // config_h_INCLUDED
