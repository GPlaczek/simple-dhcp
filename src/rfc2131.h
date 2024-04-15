#ifndef rfc2131_h_INCLUDED
#define rfc2131_h_INCLUDED

#include <stdint.h>
#include <arpa/inet.h>

struct rfc2131_dhcp_msg {
	uint8_t op, htype, hlen, hops;
	uint32_t xid;
	uint16_t secs, flags;
	struct in_addr ciaddr, yiaddr, siaddr, giaddr;
	uint8_t chaddr[16], sname[64], file[128], options[312];
} __attribute__((__packed__));

#endif // rfc2131_h_INCLUDED
