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

/**
 *  Device source file
 */

#include <string.h>
#include <knot/knot_protocol.h>
#include <knot/knot_types.h>
#include <ell/ell.h>
#include <errno.h>

#include "modbus.h"
#include "storage.h"
#include "conf-parameters.h"
#include "device.h"

struct knot_thing thing;

struct knot_data_item {
	int sensor_id;
	knot_config config;
	knot_schema schema;
	knot_value_type value;
	struct modbus_source modbus_source;
};

struct knot_thing {
	char token[KNOT_PROTOCOL_TOKEN_LEN];
	char id[KNOT_PROTOCOL_UUID_LEN];
	char name[KNOT_PROTOCOL_DEVICE_NAME_LEN];

	struct modbus_slave modbus_slave;
	struct knot_data_item *data_item;
};


static int set_thing_name(char *filename)
{
	int device_fd;
	char *knot_thing_name;

	device_fd = storage_open(filename);
	if (device_fd < 0)
		return device_fd;

	knot_thing_name = storage_read_key_string(device_fd, THING_GROUP,
						  THING_NAME);
	if (knot_thing_name == NULL || !strcmp(knot_thing_name, "") ||
	    strlen(knot_thing_name) >= KNOT_PROTOCOL_DEVICE_NAME_LEN)
		return -EINVAL;

	strcpy(thing.name, knot_thing_name);
	l_free(knot_thing_name);

	storage_close(device_fd);

	return 0;
}

static int device_set_properties(struct conf_files conf)
{
	int rc;

	rc = set_thing_name(conf.device);
	if (rc == -EINVAL)
		return rc;

	return 0;
}

int device_start(void)
{
	struct conf_files conf;

	conf.credentials = CREDENTIALS_FILENAME;
	conf.device = DEVICE_FILENAME;

	if (device_set_properties(conf))
		return -EINVAL;

	return 0;
}
