#include "cli.h"
#include "config.h"

#include <stdio.h>
#include <getopt.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stddef.h>


struct option cli_opts[] = {
	{"config", required_argument, NULL, 'c' },
	{CONFIG_GATEWAY, required_argument, NULL, 'g' },
	{CONFIG_DNS, required_argument, NULL, 'd' },
	{CONFIG_ADDRESS, required_argument, NULL, 'a' },
	{CONFIG_NETMASK, required_argument, NULL, 'm' },
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
