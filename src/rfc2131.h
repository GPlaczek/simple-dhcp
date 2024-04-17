#ifndef rfc2131_h_INCLUDED
#define rfc2131_h_INCLUDED

#include <stdint.h>
#include <arpa/inet.h>

#define RFC2131_OP_BOOTREQUEST 1
#define RFC2131_OP_BOOTREPLY   2

// this is rfc1497 actually
#define RFC2131_MAGIC_COOKIE   0x63538263

#define RFC2131_OPTION_MSGTYPE 0x35

#define RFC2131_OPTION_MSGTYPE_DHCPDISCOVER 0x01
#define RFC2131_OPTION_MSGTYPE_DHCPOFFER    0x02
#define RFC2131_OPTION_MSGTYPE_DHCPREQUEST  0x03
#define RFC2131_OPTION_MSGTYPE_DHCPDECLINE  0x04
#define RFC2131_OPTION_MSGTYPE_DHCPACK      0x05
#define RFC2131_OPTION_MSGTYPE_DHCPNAK      0x06
#define RFC2131_OPTION_MSGTYPE_DHCPRELEASE  0x07
#define RFC2131_OPTION_MSGTYPE_DHCPINFORM   0x08


struct rfc2131_dhcp_msg {
	uint8_t op, htype, hlen, hops;
	uint32_t xid;
	uint16_t secs, flags;
	in_addr_t ciaddr, yiaddr, siaddr, giaddr;
	uint8_t chaddr[16], sname[64], file[128], options[312];
} __attribute__((__packed__));

#endif // rfc2131_h_INCLUDED
