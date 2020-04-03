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

#include "storage.h"
#include "conf-parameters.h"
#include "device.h"

struct knot_thing thing;

struct modbus_slave {
	int id;
	char *url;
};

struct modbus_source {
	int reg_addr;
	int bit_offset;
};

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
	char *rabbitmq_url;
	struct knot_data_item *data_item;
};

static int set_modbus_slave_properties(char *filename)
{
	int rc;
	int device_fd;
	int aux;
	struct modbus_slave modbus_slave_aux;

	device_fd = storage_open(filename);
	if (device_fd < 0)
		return device_fd;

	rc = storage_read_key_int(device_fd, THING_GROUP, THING_MODBUS_SLAVE_ID,
				  &aux);
	if (!rc)
		return -EINVAL;

	if (aux < MODBUS_MIN_SLAVE_ID || aux > MODBUS_MAX_SLAVE_ID)
		return -EINVAL;

	modbus_slave_aux.id = aux;

	modbus_slave_aux.url = storage_read_key_string(device_fd, THING_GROUP,
						       THING_MODBUS_URL);
	if (modbus_slave_aux.url == NULL || !strcmp(modbus_slave_aux.url, ""))
		return -EINVAL;
	/* TODO: Check if modbus url is in a valid format */

	storage_close(device_fd);

	thing.modbus_slave = modbus_slave_aux;

	return 0;
}

static int set_rabbit_mq_url(char *filename)
{
	int rabbitmq_fd;
	char *rabbitmq_url_aux;

	rabbitmq_fd = storage_open(filename);
	if (rabbitmq_fd < 0)
		return rabbitmq_fd;

	rabbitmq_url_aux = storage_read_key_string(rabbitmq_fd, RABBIT_MQ_GROUP,
						   RABBIT_URL);
	if (rabbitmq_url_aux == NULL || !strcmp(rabbitmq_url_aux, ""))
		return -EINVAL;
	/* TODO: Check if rabbit mq url is in a valid format */

	storage_close(rabbitmq_fd);

	thing.rabbitmq_url = rabbitmq_url_aux;

	return 0;
}

static int get_sensor_id(char *filename, char *group_id)
{
	int device_fd;
	int rc;
	int sensor_id;

	device_fd = storage_open(filename);
	if (device_fd < 0)
		return device_fd;

	rc = storage_read_key_int(device_fd, group_id, SCHEMA_SENSOR_ID,
				  &sensor_id);
	if (!rc)
		return -EINVAL;

	storage_close(device_fd);

	return sensor_id;
}

static int valid_sensor_id(int sensor_id, int n_of_data_items)
{
	if (sensor_id >= n_of_data_items)
		return -EINVAL;

	return 0;
}

static int set_data_items(char *filename)
{
	int rc;
	int i;
	int device_fd;
	char **data_item_group;
	int n_of_data_items;
	int sensor_id;

	device_fd = storage_open(filename);

	n_of_data_items = get_number_of_data_items(device_fd);
	thing.data_item = malloc(n_of_data_items*sizeof(struct knot_data_item));
	if (thing.data_item == NULL)
		return -EINVAL;

	data_item_group = get_data_item_groups(device_fd);

	storage_close(device_fd);
	for (i = 0; data_item_group[i] != NULL ; i++) {
		sensor_id = get_sensor_id(filename, data_item_group[i]);
		rc = valid_sensor_id(sensor_id, n_of_data_items);
		if (rc < 0)
			goto error;
		thing.data_item[sensor_id].sensor_id = sensor_id;
		/* TODO: Set schema properties */
		/* TODO: Set config properties */
		/* TODO: Set modbus sensor properties */
	}

	l_strfreev(data_item_group);

	return 0;

error:
	l_strfreev(data_item_group);

	return -EINVAL;
}

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

	rc = set_rabbit_mq_url(conf.rabbit);
	if (rc == -EINVAL)
		return rc;

	rc = set_modbus_slave_properties(conf.device);
	if (rc == -EINVAL)
		return rc;

	rc = set_data_items(conf.device);
	if (rc == -EINVAL)
		return rc;

	return 0;
}

int device_start(void)
{
	struct conf_files conf;

	conf.credentials = CREDENTIALS_FILENAME;
	conf.device = DEVICE_FILENAME;
	conf.rabbit = RABBIT_MQ_FILENAME;

	if (device_set_properties(conf))
		return -EINVAL;

	return 0;
}

void device_destroy(void)
{
	l_free(thing.rabbitmq_url);
	l_free(thing.modbus_slave.url);

	free(thing.data_item);
}
