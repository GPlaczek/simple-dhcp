#ifndef cli_h_INCLUDED
#define cli_h_INCLUDED

#include "dhcp.h"

void parse_command_line(int argc, char *argv[], struct dhcp_args *args);

#endif // cli_h_INCLUDED
