#include "config.h"
#include "dhcp.h"

#include <stdio.h>
#include <string.h>

int parse_config(const char *config_path, struct dhcp_server *srv) {
	FILE *fp;
	char *line = NULL,
	     *key = NULL,
	     *value = NULL;
	size_t len = 0;
	ssize_t read;
	in_addr_t addr; 

	fp = fopen(config_path, "r");
	if (fp == NULL)
		return 1;

	while ((read = getline(&line, &len, fp)) != -1) {
		key = strtok(line, "=");
		if (key == NULL)
			continue;
		value = strtok(NULL, "=");
		if (value == NULL)
			continue;
		printf("%s\n", value);

		if (!strncmp(key, CONFIG_GATEWAY, sizeof(CONFIG_GATEWAY) - 1)) {
			addr = inet_addr(value);
			if (addr == INADDR_NONE)
				continue;

			srv->gateway = addr;
		} else if (!strncmp(key, CONFIG_ADDRESS, sizeof(CONFIG_ADDRESS) - 1)) {
			addr = inet_addr(value);
			if (addr == INADDR_NONE)
				continue;

			srv->llist.netaddr = addr;
		} else if (!strncmp(key, CONFIG_NETMASK, sizeof(CONFIG_NETMASK) - 1)) {
			addr = inet_addr(value);
			if (addr == INADDR_NONE)
				continue;

			srv->llist.netmask = addr;
		} else if (!strncmp(key, CONFIG_DNS, sizeof(CONFIG_DNS) - 1)) {
			addr = inet_addr(value);
			if (addr == INADDR_NONE)
				continue;

			srv->dns = addr;
		}
	}

	return 0;
}
