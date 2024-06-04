#include "config.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <net/if.h>

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
		value = strtok(NULL, "\n");
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
		} else if (!strncmp(key, CONFIG_LOG_LEVEL, sizeof(CONFIG_LOG_LEVEL) - 1)) {
			target = &args->log_level;
		} else if (!strncmp(key, CONFIG_NET_INTERFACE, sizeof(CONFIG_NET_INTERFACE) - 1)) {
			target = &args->net_interface;
		}

		if (target == NULL)
			continue;  // wrong config option

		len = strlen(value) + 1;
		*target = malloc(len);
		strcpy(*target, value);
	}

	free(line);

	return 0;
}


#define PROCESS_ARG(TYPE, NAME) \
void __process_arg_##NAME( \
	const char *restrict arg1, \
	const char *restrict arg2, \
	TYPE *target, \
	TYPE (*func) (const char *), \
	TYPE def \
) { \
	const char *op = NULL; \
	if (arg1) \
		op = arg1; \
	else if (arg2) \
		op = arg2; \
	if (op != NULL) { \
		*target = func(op); \
	} else { \
		*target = def; \
	} \
}
PROCESS_ARG(in_addr_t, addr)
PROCESS_ARG(int, int)
PROCESS_ARG(void*, ptr)

void *__identity(void *arg) {
	return arg;
}

int parse_time(const char *time) {
	char *unit;
	int __time = strtol(time, &unit, 10);
	if (unit == NULL)
		return -1;

	int mul = 1;
	switch (*unit) {
	case 's': mul = 1; break;
	case 'm': mul = 60; break;
	case 'h': mul = 3600; break;
	case 'd': mul = 86400; break;
	default: return -1;
	}

	return __time * mul;
}

void __log_ip_h(const char *target, in_addr_t addr) {
	uint8_t *u8v = (uint8_t *)&addr;
	slog(LOGLEVEL_INFO, "%s is %d.%d.%d.%d\n",
		target, u8v[3], u8v[2], u8v[1], u8v[0]);
}

void __log_ip_n(const char *target, in_addr_t addr) {
	uint8_t *u8v = (uint8_t *)&addr;
	slog(LOGLEVEL_INFO, "%s is %d.%d.%d.%d\n",
		target, u8v[0], u8v[1], u8v[2], u8v[3]);
}

int __log_level(const char *lv) {
	int level = -1;

	for(size_t i = 0; i < LOG_LABELS_LEN; i++) {
		if (!strncmp(LOG_LABELS[i], lv, sizeof(*LOG_LABELS[i]))) {
			level = i+1;
			break;
		}
	}

	return level;
}

int get_dhcp_config(struct dhcp_args *cli, struct dhcp_config *conf) {
	int loglevel = LOGLEVEL_INFO;
	char *ifname;
	struct dhcp_args file;

	memset(&file, 0, sizeof(file));
	if (cli->config_filename != NULL) {
		parse_config(cli->config_filename, &file);
	}
	__process_arg_addr(cli->gateway, file.gateway,
		&conf->gateway, inet_addr, inet_addr("192.168.1.1"));
	__log_ip_n("gateway", conf->gateway);
	__process_arg_addr(cli->dns, file.dns,
		&conf->dns, inet_addr, inet_addr("8.8.8.8"));
	__log_ip_n("dns", conf->dns);
	__process_arg_addr(cli->address, file.address,
		&conf->address, inet_network, inet_network("192.168.1.0"));
	__log_ip_h("network address", conf->address);
	__process_arg_addr(cli->netmask, file.netmask,
		&conf->netmask, inet_network, inet_network("255.255.255.0"));
	__log_ip_h("netmask is", conf->netmask);
	__process_arg_int(cli->lease_time, file.lease_time,
		&conf->lease_time, parse_time, 3600);
	slog(LOGLEVEL_INFO, "lease time is %ds\n", conf->lease_time);
	__process_arg_int(cli->log_level, file.log_level,
		&loglevel, __log_level, LOGLEVEL_INFO);
	slog(LOGLEVEL_INFO, "setting log level to %s\n", LOG_LABELS[loglevel-1]);
	set_log_level(loglevel);
	__process_arg_ptr(cli->net_interface, file.net_interface,
		(void **)&ifname, (void *(*)(const char *))__identity, NULL);

	if (ifname != NULL) {
		if (strlen(ifname) > IFNAMSIZ) {
			slog(LOGLEVEL_ERROR, "The interface name is too long\n");
			return 1;
		}
		
		strncpy(conf->net_interface, ifname, IFNAMSIZ);
	}


	char **chr_view = (char **)&file;
	for (size_t i = 0; i < sizeof(struct dhcp_args) / sizeof(char *); i++) {
		if (chr_view[i] != NULL)
			free(chr_view[i]);
	}

	return 0;
}
