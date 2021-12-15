/*
 * This file is part of the KNOT Project
 *
 * Copyright (c) 2020, CESAR. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <knot/knot_protocol.h>
#include <ell/ell.h>

#include "settings.h"
#include "device.h"
#include "conf-device.h"

static int log_priority;

static void signal_handler(uint32_t signo, void *user_data)
{
	switch (signo) {
	case SIGINT:
	case SIGTERM:
		l_info("Terminate");
		l_main_quit();
		break;
	}
}

static int detach_daemon(void)
{
	if (daemon(0, 0))
		return -errno;

	return 0;
}

static void set_device_settings(struct device_settings *device_settings,
								struct settings *settings)
{
	device_settings->credentials_path = l_strdup(settings->credentials_path);
	device_settings->device_path = l_strdup(settings->device_path);
	device_settings->cloud_path = l_strdup(settings->cloud_path);
}

static void free_device_settings(struct device_settings *device_settings)
{
	l_free(device_settings->credentials_path);
	l_free(device_settings->device_path);
	l_free(device_settings->cloud_path);
	l_free(device_settings);
}

static void log_stderr_handler(int priority, const char *file, const char *line,
			       const char *func, const char *format, va_list ap)
{
	if (priority > log_priority)
		return;

	switch (priority) {
	case L_LOG_ERR:
		fprintf(stderr, "ERR: %s() ", func);
		break;
	case L_LOG_WARNING:
		fprintf(stderr, "WARN: ");
		break;
	case L_LOG_INFO:
		fprintf(stderr, "INFO: ");
		break;
	case L_LOG_DEBUG:
		fprintf(stderr, "DEBUG: ");
		break;
	default:
		return;
	}

	vfprintf(stderr, format, ap);
}

static void log_enable(int priority)
{
	l_log_set_handler(log_stderr_handler);
	log_priority = priority;

	device_set_log_priority(priority);

	if (log_priority == L_LOG_DEBUG)
		l_debug_enable("*");
}

int main(int argc, char *argv[])
{
	struct settings *settings;
	struct device_settings *device_settings;
	int err;

	settings = settings_load(argc, argv);
	if (settings == NULL)
		return EXIT_FAILURE;

	if (settings->help) {
		settings_free(settings);
		return EXIT_SUCCESS;
	}

	if (!l_main_init()) {
		settings_free(settings);
		return EXIT_FAILURE;
	}

	log_enable(settings->log_level);

	device_settings = l_new(struct device_settings, 1);
	set_device_settings(device_settings, settings);

	l_info("Starting KNoT VirtualThing");

	err = device_start(device_settings);
	if (err) {
		l_error("Failed to start the device: %s (%d). Exiting...",
			strerror(-err), -err);
		l_main_exit();
		settings_free(settings);
		free_device_settings(device_settings);
		return EXIT_FAILURE;
	}
	free_device_settings(device_settings);

	if (settings->detach) {
		err = detach_daemon();
		if (err) {
			l_error("Failed to detach. %s (%d). Exiting...",
				strerror(-err), -err);
			settings_free(settings);
			l_main_exit();
			return EXIT_FAILURE;
		}
	}

	settings_free(settings);

	l_main_run_with_signal(signal_handler, NULL);

	device_destroy();

	l_info("Exiting KNoT VirtualThing");

	l_main_exit();

	return EXIT_SUCCESS;
}
