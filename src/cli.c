#include "cli.h"

#include <stdio.h>
#include <getopt.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stddef.h>


struct option cli_opts[] = {
	{"config", required_argument, NULL, 'c' },
	{"gateway", required_argument, NULL, 'g' },
	{"dns", required_argument, NULL, 'd' },
	{"address", required_argument, NULL, 'a' },
	{"netmask", required_argument, NULL, 'm' },
	{NULL, 0, NULL, 0}
};

void parse_command_line(int argc, char *argv[], struct cli_args *args) {
	char c;
	while ((c = getopt_long(argc, argv, "c:g:d:a:m:", cli_opts, NULL)) != -1) {
		switch (c) {
		case 'c': args->config_filename = optarg; break;
		case 'g': args->gateway = optarg; break;
		case 'd': args->dns = optarg; break;
		case 'a': args->address = optarg; break;
		case 'm': args->netmask = optarg; break;
		}
	}
}
