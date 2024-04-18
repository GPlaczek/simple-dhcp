#ifndef cli_h_INCLUDED
#define cli_h_INCLUDED

struct cli_args {
	char *config_filename, *gateway, *dns, *address, *netmask;
};
void parse_command_line(int argc, char *argv[], struct cli_args *args);

#endif // cli_h_INCLUDED
