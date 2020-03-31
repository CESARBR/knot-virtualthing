/**
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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

#include <errno.h>
#include <stdbool.h>
#include <modbus.h>
#include <string.h>

#include "modbus-interface.h"
#include "modbus-driver.h"

#define TCP_PREFIX "tcp://"
#define TCP_PREFIX_SIZE 6
#define RTU_PREFIX "serial://"
#define RTU_PREFIX_SIZE 9

enum driver_type {
	TCP,
	RTU
};

struct modbus_driver connection_interface;
modbus_t *modbus_ctx;

static int parse_url(const char *url)
{
	if (strncmp(url, TCP_PREFIX, TCP_PREFIX_SIZE) == 0)
		return TCP;
	else if (strncmp(url, RTU_PREFIX, RTU_PREFIX_SIZE) == 0)
		return RTU;
	else
		return -EINVAL;
}

int modbus_start(const char *url)
{
	switch (parse_url(url)) {
	case TCP:
		connection_interface = tcp;
		break;
	case RTU:
		connection_interface = rtu;
		break;
	default:
		return -EINVAL;
	}

	modbus_ctx = connection_interface.create(url);
	if (!modbus_ctx)
		return -errno;

	return 0;
}

void modbus_stop(void)
{
	connection_interface.destroy(modbus_ctx);
}
