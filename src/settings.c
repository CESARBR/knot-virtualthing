/*
 * This file is part of the KNOT Project
 *
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <getopt.h>

#include <ell/ell.h>

#include "settings.h"

#define DEFAULT_CREDENTIALS_FILE_PATH	"/etc/knot/credentials.conf"
#define DEFAULT_DEVICE_FILE_PATH	"/etc/knot/device.conf"
#define DEFAULT_AMQP_FILE_PATH		"/etc/knot/rabbitmq.conf"

static bool detach = true;
static bool help = false;

static void usage(void)
{
	printf("thingd - KNoT VirtualThing\n"
		"Usage:\n");
	printf("\tthingd [options]\n");
	printf("Options:\n"
		"\t-c, --credentials-file  Credentials configuration file "
		"path\n"
		"\t-d, --dev-file          Device configuration file path\n"
		"\t-r, --rabbitmq-url      Connect with a different url "
		"amqp://[$USERNAME[:$PASSWORD]\\@]$HOST[:$PORT]/[$VHOST]\n"
		"\t-n, --nodetach          Disable running in background\n"
		"\t-h, --help              Show help options\n");
}

static const struct option main_options[] = {
	{ "credentials-file",	required_argument,	NULL, 'c' },
	{ "dev-file",		required_argument,	NULL, 'd' },
	{ "rabbitmq-url",	required_argument,	NULL, 'r' },
	{ "nodetach",		no_argument,		NULL, 'n' },
	{ "help",		no_argument,		NULL, 'h' },
	{ }
};

static int parse_args(int argc, char *argv[], struct settings *settings)
{
	int opt;

	for (;;) {
		opt = getopt_long(argc, argv, "c:d:r:nh",
				  main_options, NULL);
		if (opt < 0)
			break;

		switch (opt) {
		case 'c':
			settings->credentials_path = optarg;
			break;
		case 'd':
			settings->device_path = optarg;
			break;
		case 'r':
			settings->rabbitmq_path = optarg;
			break;
		case 'n':
			settings->detach = false;
			break;
		case 'h':
			usage();
			settings->help = true;
			return 0;
		default:
			return -EINVAL;
		}
	}

	if (argc - optind > 0) {
		fprintf(stderr, "Invalid command line parameters\n");
		return -EINVAL;
	}

	return 0;
}

struct settings *settings_load(int argc, char *argv[])
{
	struct settings *settings;

	settings = l_new(struct settings, 1);

	settings->credentials_path = DEFAULT_CREDENTIALS_FILE_PATH;
	settings->device_path = DEFAULT_DEVICE_FILE_PATH;
	settings->rabbitmq_path = DEFAULT_AMQP_FILE_PATH;
	settings->detach = detach;
	settings->help = help;

	if (parse_args(argc, argv, settings) < 0) {
		settings_free(settings);
		return NULL;
	}

	return settings;
}

void settings_free(struct settings *settings)
{
	l_free(settings);
}
