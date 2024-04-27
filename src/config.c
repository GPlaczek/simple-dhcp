#include "config.h"
#include "dhcp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_BUF_SIZE 256

int parse_config(const char *config_path, struct dhcp_args *args) {
	FILE *fp;
	char *line = NULL, *key = NULL, *value = NULL, **target = NULL;
	size_t len = 0;
	ssize_t read;

	line = malloc(sizeof(char) * LINE_BUF_SIZE);

	fp = fopen(config_path, "r");
	if (fp == NULL)
		return 1;

	while ((read = getline(&line, &len, fp)) != -1) {
		target = NULL;

		key = strtok(line, "=");
		if (key == NULL)
			continue;
		value = strtok(NULL, "=");
		if (value == NULL)
			continue;

		if (!strncmp(key, CONFIG_GATEWAY, sizeof(CONFIG_GATEWAY) - 1)) {
			target = &args->gateway;
		} else if (!strncmp(key, CONFIG_ADDRESS, sizeof(CONFIG_ADDRESS) - 1)) {
			target = &args->address;
		} else if (!strncmp(key, CONFIG_NETMASK, sizeof(CONFIG_NETMASK) - 1)) {
			target = &args->netmask;
		} else if (!strncmp(key, CONFIG_DNS, sizeof(CONFIG_DNS) - 1)) {
			target = &args->dns;
		} else if (!strncmp(key, CONFIG_LEASE_TIME, sizeof(CONFIG_LEASE_TIME) - 1)) {
			target = &args->lease_time;
		}

		if (target == NULL)
			continue;  // wrong config option

		len = strlen(value) + 1;
		*target = malloc(len);
		strcpy(*target, value);
	}

	return 0;
}
