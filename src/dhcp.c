#include "rfc2131.h"
#include "leases.h"
#include "dhcp.h"
#include "config.h"
#include "log.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>


struct dhcp_option {
	uint8_t len;
	uint8_t *ptr;
};
struct dhcp_option client_id, parameter_request_list;


size_t write_option8(uint8_t *options, uint8_t option_type, uint8_t option) {
	options[0] = option_type;
	options[1] = 0x01;
	options[2] = option;
	return 3;
}

size_t write_option32(uint8_t *options, uint8_t option_type, uint32_t option) {
	options[0] = option_type;
	options[1] = 0x04;
	uint32_t *view32 = (uint32_t *)(options + 2);
	*view32 = option;
	return 6;
}

size_t write_option(uint8_t *restrict options, uint8_t option_type, void *restrict option, size_t len) {
	options[0] = option_type;
	options[1] = len;
	memcpy(options + 2, option, len);
	return len + 2;
}

struct lease *get_new_lease(
	struct rfc2131_dhcp_msg *dhcp_msg,
	struct dhcp_server *srv
) {
	in_addr_t __addr;
	uint8_t *__u8v;
	char *human_haddr = malloc(64);  // TODO: macro
	human_readable_haddr(human_haddr, dhcp_msg->chaddr, dhcp_msg->hlen);
	hslog(LOGLEVEL_DEBUG, human_haddr, "Got message\n");
	struct lease *lease = NULL;

	if (dhcp_msg->op != RFC2131_OP_BOOTREQUEST) {
		hslog(LOGLEVEL_WARN, human_haddr, "Invalid dhcp operation type\n");
		goto exit;
	}

	if (*(uint32_t *)dhcp_msg->options != RFC2131_MAGIC_COOKIE) {
		hslog(LOGLEVEL_WARN, human_haddr, "Invalid magic cookie\n");
		goto exit;
	}

	uint8_t *option_ptr = dhcp_msg->options + 4;
	uint8_t msgtype = 0;

	while (option_ptr < dhcp_msg->options + 312) {
		uint8_t option_id = *(option_ptr++);
		uint8_t option_len = *(option_ptr++);

		switch (option_id) {
		case RFC2131_OPTION_MSGTYPE:
			msgtype = *option_ptr;
			break;
		case RFC2131_OPTION_CLIENT_ID:
			client_id.len = option_len;
			client_id.ptr = option_ptr;
			break;
		case RFC2131_OPTION_PARAMETER_REQUEST_LIST:
			parameter_request_list.len = option_len;
			parameter_request_list.ptr = option_ptr;
			break;
		}

		option_ptr += option_len;
	}

	switch (msgtype) {
	case RFC2131_OPTION_MSGTYPE_DHCPREQUEST:
		hslog(LOGLEVEL_INFO, human_haddr, "DHCPREQUEST\n");
		for (int i = 0; i < srv->llist.len; i++) {
			// Use a lease if it has been offered in the same transaction or if
			// it was leased to the same hardware address before
			if (srv->llist.lease_vec[i].xid == dhcp_msg->xid ||
				!memcmp(srv->llist.lease_vec[i].chaddr, dhcp_msg->chaddr, dhcp_msg->hlen)
			) {
				lease = &srv->llist.lease_vec[i];
				lease->efd = srv->llist.max_lease_time;
				break;
			}
		}
		timer_arm(&srv->timer, lease);

		if (lease == NULL) {
			lease = leaselist_get_lease(&srv->llist, dhcp_msg->chaddr, dhcp_msg->hlen);
			lease->xid = dhcp_msg->xid;
			memcpy(lease->chaddr, dhcp_msg->chaddr, 16);
			goto exit;
		}

		__addr = htonl(lease->ipaddr);
		__u8v = (uint8_t *)&__addr;
		hslog(LOGLEVEL_INFO, human_haddr, "Leasing %d.%d.%d.%d (xid: %d)\n",
			__u8v[0], __u8v[1], __u8v[2], __u8v[3], lease->xid);

		break;
	case RFC2131_OPTION_MSGTYPE_DHCPDISCOVER:
		hslog(LOGLEVEL_INFO, human_haddr, "DHCPDISCOVER\n");
		lease = leaselist_get_lease(&srv->llist, dhcp_msg->chaddr, dhcp_msg->hlen);
		lease->xid = dhcp_msg->xid;
		__addr = htonl(lease->ipaddr);
		__u8v = (uint8_t *)&__addr;
		hslog(LOGLEVEL_INFO, human_haddr, "Offering %d.%d.%d.%d (xid: %d)\n",
			__u8v[0], __u8v[1], __u8v[2], __u8v[3], lease->xid);
		memcpy(lease->chaddr, dhcp_msg->chaddr, 16);
		goto exit;
	case RFC2131_OPTION_MSGTYPE_DHCPDECLINE:
	case RFC2131_OPTION_MSGTYPE_DHCPRELEASE:
	case RFC2131_OPTION_MSGTYPE_DHCPINFORM:
	default:
		// Invalid dhcp option
		goto exit;
	}
exit:
	free(human_haddr);
	return lease;
}

void prepare_response(
	struct rfc2131_dhcp_msg *dhcp_msg,
	struct rfc2131_dhcp_msg *dhcp_resp,
	struct dhcp_server *srv,
	struct lease *lease
) {
	if (lease == NULL)
		return;

	dhcp_resp->htype = dhcp_msg->htype;
	dhcp_resp->hlen = dhcp_msg->hlen;
	dhcp_resp->xid = dhcp_msg->xid;
	dhcp_resp->yiaddr = htonl(lease->ipaddr);
	dhcp_resp->giaddr = dhcp_msg->giaddr;
	memcpy(dhcp_resp->chaddr, dhcp_msg->chaddr, 16);
	int i = 4;

	if (lease->efd == LEASE_OFFERED) {
		dhcp_resp->ciaddr = 0;
		i += write_option8(dhcp_resp->options + i, RFC2131_OPTION_MSGTYPE, RFC2131_OPTION_MSGTYPE_DHCPOFFER);
	} else {
		dhcp_resp->ciaddr = dhcp_msg->ciaddr;
		i += write_option8(dhcp_resp->options + i, RFC2131_OPTION_MSGTYPE, RFC2131_OPTION_MSGTYPE_DHCPACK);
	}

	i+= write_option32(dhcp_resp->options + i, RFC2131_OPTION_IP_ADDRESS_LEASE_TIME, htonl((uint32_t)lease->efd));

	if (client_id.len > 0) {
		i+= write_option(dhcp_resp->options + i, RFC2131_OPTION_CLIENT_ID, client_id.ptr, client_id.len);
		client_id.len = 0;
	}

	for (int j = 0; j < parameter_request_list.len; j++) {
		switch (parameter_request_list.ptr[j]) {
		case RFC2131_OPTION_SUBNET_MASK:
			i += write_option32(dhcp_resp->options + i, RFC2131_OPTION_SUBNET_MASK, htonl(srv->llist.netmask)); break;
		case RFC2131_OPTION_DOMAIN_NAME_SERVER:
			i += write_option32(dhcp_resp->options + i, RFC2131_OPTION_DOMAIN_NAME_SERVER, srv->dns); break;
		}
	}
	dhcp_resp->options[i++] = 0xff;
}

void process_dhcp_msg(
	struct rfc2131_dhcp_msg *dhcp_msg,
	struct rfc2131_dhcp_msg *dhcp_resp,
	struct dhcp_server *srv
) {
	struct lease *lease = get_new_lease(dhcp_msg, srv);
	prepare_response(dhcp_msg, dhcp_resp, srv, lease);
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

void create(struct dhcp_args *cli, struct dhcp_server *srv) {
	struct dhcp_args conf;
	int loglevel = LOGLEVEL_INFO;

	memset(&conf, 0, sizeof(conf));
	if (cli->config_filename != NULL)
		parse_config(cli->config_filename, &conf);

	__process_arg_addr(cli->gateway, conf.gateway,
		&srv->gateway, inet_addr, inet_addr("192.168.1.1"));
	__log_ip_n("gateway", srv->gateway);
	__process_arg_addr(cli->dns, conf.dns,
		&srv->dns, inet_addr, inet_addr("8.8.8.8"));
	__log_ip_n("dns", srv->dns);
	__process_arg_addr(cli->address, conf.address,
		&srv->llist.netaddr, inet_network, inet_network("192.168.1.0"));
	__log_ip_h("network address", srv->llist.netaddr);
	__process_arg_addr(cli->netmask, conf.netmask,
		&srv->llist.netmask, inet_network, inet_network("255.255.255.0"));
	__log_ip_h("netmask is", srv->llist.netmask);
	__process_arg_int(cli->lease_time, conf.lease_time,
		&srv->llist.max_lease_time, parse_time, 3600);
	slog(LOGLEVEL_INFO, "lease time is %ds\n", srv->llist.max_lease_time);
	__process_arg_int(cli->log_level, conf.log_level,
		&loglevel, __log_level, LOGLEVEL_INFO);
	slog(LOGLEVEL_INFO, "setting log level to %s\n", LOG_LABELS[loglevel-1]);
	set_log_level(loglevel);

	char **chr_view = (char **)&conf;
	for (size_t i = 0; i < sizeof(struct dhcp_args) / sizeof(char *); i++) {
		if (chr_view[i] != NULL)
			free(chr_view[i]);
	}

	leaselist_init(&srv->llist);
	timer_init(&srv->timer, &srv->llist);
}
