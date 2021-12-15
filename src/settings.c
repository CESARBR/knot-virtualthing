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
#define DEFAULT_AMQP_FILE_PATH		"/etc/knot/cloud.conf"

#define LOG_STRING_ERR			"error"
#define LOG_STRING_WARN			"warn"
#define LOG_STRING_INFO			"info"
#define LOG_STRING_DEBUG		"debug"

static bool detach = true;
static bool help = false;
static int log_level = L_LOG_INFO;

static const struct option main_options[] = {
	{ "credentials-file",	required_argument,	NULL, 'c' },
	{ "dev-file",		required_argument,	NULL, 'd' },
	{ "cloud-file",		required_argument,	NULL, 'p' },
	{ "log",		required_argument,	NULL, 'l' },
	{ "nodetach",		no_argument,		NULL, 'n' },
	{ "help",		no_argument,		NULL, 'h' },
	{ }
};

static void usage(void)
{
	printf("thingd - KNoT VirtualThing\n"
		"Usage:\n");
	printf("\tthingd [options]\n");
	printf("Options:\n"
		"\t-c, --credentials-file  Credentials configuration file "
		"path\n"
		"\t-d, --dev-file          Device configuration file path\n"
		"\t-p, --cloud-file        Cloud configuration file path "
		"amqp://[$USERNAME[:$PASSWORD]\\@]$HOST[:$PORT]/[$VHOST]\n"
		"\t-l, --log               Configure log level, options are:"
		"error | warn | info | debug"
		"\t-n, --nodetach          Disable running in background\n"
		"\t-h, --help              Show help options\n");
}

static int parse_log_level(char *log_name)
{
	int priority;

	if (!strcmp(log_name, LOG_STRING_ERR)) {
		priority = L_LOG_ERR;
	} else if (!strcmp(log_name, LOG_STRING_WARN)) {
		priority = L_LOG_WARNING;
	} else if (!strcmp(log_name, LOG_STRING_INFO)) {
		priority = L_LOG_INFO;
	} else if (!strcmp(log_name, LOG_STRING_DEBUG)) {
		priority = L_LOG_DEBUG;
	} else {
		priority = -1;
	}

	return priority;
}

static int parse_args(int argc, char *argv[], struct settings *settings)
{
	int opt;

	for (;;) {
		opt = getopt_long(argc, argv, "P:c:d:p:l:nh",
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
		case 'p':
			settings->cloud_path = optarg;
			break;
		case 'l':
			settings->log_level = parse_log_level(optarg);
			if (settings->log_level < 0) {
				fprintf(stderr, "ERROR: Invalid log level\n");
				usage();
				return -EINVAL;
			}
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
	settings->cloud_path = DEFAULT_AMQP_FILE_PATH;
	settings->log_level = log_level;
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
