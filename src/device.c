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
#include "settings.h"
#include "conf-parameters.h"
#include "device.h"
#include "modbus-interface.h"
#include "cloud.h"

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
	char *user_token;

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
	if (rc <= 0)
		goto error;

	if (aux < MODBUS_MIN_SLAVE_ID || aux > MODBUS_MAX_SLAVE_ID)
		goto error;

	modbus_slave_aux.id = aux;

	modbus_slave_aux.url = storage_read_key_string(device_fd, THING_GROUP,
						       THING_MODBUS_URL);
	if (modbus_slave_aux.url == NULL || !strcmp(modbus_slave_aux.url, ""))
		goto error;
	/* TODO: Check if modbus url is in a valid format */

	storage_close(device_fd);

	thing.modbus_slave = modbus_slave_aux;

	return 0;

error:
	storage_close(device_fd);

	return -EINVAL;
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
	if (rabbitmq_url_aux == NULL || !strcmp(rabbitmq_url_aux, "")) {
		storage_close(rabbitmq_fd);
		return -EINVAL;
	}
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
	if (rc <= 0) {
		storage_close(device_fd);
		return -EINVAL;
	}

	storage_close(device_fd);

	return sensor_id;
}

static int valid_sensor_id(int sensor_id, int n_of_data_items)
{
	if (sensor_id >= n_of_data_items)
		return -EINVAL;

	return 0;
}

static int set_schema(char *filename, char *group_id, int sensor_id)
{
	char *name;
	int rc;
	int aux;
	int device_fd;
	knot_schema schema_aux;

	device_fd = storage_open(filename);
	if (device_fd < 0)
		return device_fd;

	name = storage_read_key_string(device_fd, group_id, SCHEMA_SENSOR_NAME);
	if (name == NULL || !strcmp(name, "") ||
			strlen(name) >= KNOT_PROTOCOL_DATA_NAME_LEN)
		goto error;

	strcpy(schema_aux.name, name);
	l_free(name);

	rc = storage_read_key_int(device_fd, group_id, SCHEMA_VALUE_TYPE, &aux);
	if (rc <= 0)
		goto error;
	schema_aux.value_type = aux;

	rc = storage_read_key_int(device_fd, group_id, SCHEMA_UNIT, &aux);
	if (rc <= 0)
		goto error;
	schema_aux.unit = aux;

	rc = storage_read_key_int(device_fd, group_id, SCHEMA_TYPE_ID, &aux);
	if (rc <= 0)
		goto error;
	schema_aux.type_id = aux;

	rc = knot_schema_is_valid(schema_aux.type_id,
				  schema_aux.value_type,
				  schema_aux.unit);
	if (rc)
		goto error;

	storage_close(device_fd);

	thing.data_item[sensor_id].schema = schema_aux;

	return 0;

error:
	storage_close(device_fd);

	return -EINVAL;
}

static int assign_limit(int value_type, int value, knot_value_type *limit)
{
	switch (value_type) {
	case KNOT_VALUE_TYPE_INT:
		limit->val_i = value;
		break;
	case KNOT_VALUE_TYPE_FLOAT:
		/* Storage doesn't give support to float numbers */
		break;
	case KNOT_VALUE_TYPE_BOOL:
		limit->val_b = value;
		break;
	case KNOT_VALUE_TYPE_RAW:
		/* Storage doesn't give support to raw numbers */
		break;
	case KNOT_VALUE_TYPE_INT64:
		/* Storage doesn't give support to int64 values */
		break;
	case KNOT_VALUE_TYPE_UINT:
		/* Storage doesn't give support to uint values */
		break;
	case KNOT_VALUE_TYPE_UINT64:
		/* Storage doesn't give support to uint64 values */
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int set_config(char *filename, char *group_id, int sensor_id)
{
	int rc;
	int aux;
	int device_fd;
	knot_config config_aux;

	device_fd = storage_open(filename);
	if (device_fd < 0)
		return device_fd;

	config_aux.event_flags = 0;

	rc = storage_read_key_int(device_fd, group_id, CONFIG_LOWER_THRESHOLD,
				  &aux);
	if (rc > 0) {
		config_aux.event_flags |= KNOT_EVT_FLAG_LOWER_THRESHOLD;
		assign_limit(thing.data_item[sensor_id].schema.value_type, aux,
			     &config_aux.lower_limit);
	}

	rc = storage_read_key_int(device_fd, group_id, CONFIG_UPPER_THRESHOLD,
				  &aux);
	if (rc > 0) {
		config_aux.event_flags |= KNOT_EVT_FLAG_UPPER_THRESHOLD;
		assign_limit(thing.data_item[sensor_id].schema.value_type, aux,
			     &config_aux.upper_limit);
	}

	rc = storage_read_key_int(device_fd, group_id, CONFIG_TIME_SEC, &aux);
	if (rc > 0) {
		config_aux.event_flags |= KNOT_EVT_FLAG_TIME;
		config_aux.time_sec = aux;
	}

	rc = storage_read_key_int(device_fd, group_id, CONFIG_CHANGE, &aux);
	if (rc > 0)
		config_aux.event_flags |= KNOT_EVT_FLAG_CHANGE;

	rc = knot_config_is_valid(config_aux.event_flags,
				  thing.data_item[sensor_id].schema.value_type,
				  config_aux.time_sec,
				  &config_aux.lower_limit,
				  &config_aux.upper_limit);
	if (rc) {
		storage_close(device_fd);
		return -EINVAL;
	}

	storage_close(device_fd);

	thing.data_item[sensor_id].config = config_aux;

	return 0;
}

static int set_modbus_source_properties(char *filename, char *group_id,
					int sensor_id)
{
	int rc;
	int device_fd;
	struct modbus_source modbus_source_aux;

	device_fd = storage_open(filename);
	if (device_fd < 0)
		return device_fd;

	rc = storage_read_key_int(device_fd, group_id, MODBUS_REG_ADDRESS,
				  &modbus_source_aux.reg_addr);
	if (rc <= 0)
		goto error;

	rc = storage_read_key_int(device_fd, group_id, MODBUS_BIT_OFFSET,
				  &modbus_source_aux.bit_offset);
	if (rc <= 0)
		goto error;

	storage_close(device_fd);

	thing.data_item[sensor_id].modbus_source = modbus_source_aux;

	return 0;

error:
	storage_close(device_fd);

	return -EINVAL;
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
	if (thing.data_item == NULL) {
		storage_close(device_fd);
		return -EINVAL;
	}

	data_item_group = get_data_item_groups(device_fd);

	storage_close(device_fd);
	for (i = 0; data_item_group[i] != NULL ; i++) {
		sensor_id = get_sensor_id(filename, data_item_group[i]);
		rc = valid_sensor_id(sensor_id, n_of_data_items);
		if (rc < 0)
			goto error;
		thing.data_item[sensor_id].sensor_id = sensor_id;
		rc = set_schema(filename, data_item_group[i], sensor_id);
		if (rc < 0)
			goto error;
		rc = set_config(filename, data_item_group[i], sensor_id);
		if (rc < 0)
			goto error;
		rc = set_modbus_source_properties(filename, data_item_group[i],
						  sensor_id);
		if (rc < 0)
			goto error;
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
	    strlen(knot_thing_name) >= KNOT_PROTOCOL_DEVICE_NAME_LEN) {
		l_free(knot_thing_name);
		storage_close(device_fd);
		return -EINVAL;
	}

	strcpy(thing.name, knot_thing_name);
	l_free(knot_thing_name);

	storage_close(device_fd);

	return 0;
}

static int set_thing_user_token(char *filename)
{
	int device_fd;
	char *user_token;

	device_fd = storage_open(filename);
	if (device_fd < 0)
		return device_fd;

	user_token = storage_read_key_string(device_fd, THING_GROUP,
					     THING_USER_TOKEN);
	if (user_token == NULL || !strcmp(user_token, "")) {
		storage_close(device_fd);
		return -EINVAL;
	}
	thing.user_token = user_token;

	storage_close(device_fd);

	return 0;
}

static int device_set_properties(struct conf_files conf)
{
	int rc;

	rc = set_thing_name(conf.device);
	if (rc < 0)
		return rc;

	rc = set_rabbit_mq_url(conf.rabbit);
	if (rc < 0)
		return rc;

	rc = set_thing_user_token(conf.device);
	if (rc < 0)
		return rc;

	rc = set_modbus_slave_properties(conf.device);
	if (rc < 0)
		return rc;

	rc = set_data_items(conf.device);
	if (rc < 0)
		return rc;

	return 0;
}

static void on_cloud_connected(void *user_data)
{
	/* TODO: Set read callback function to handle incoming messages */

	/**
	 * TODO: Call function to inform the state machine the success
	 * connection with the cloud
	 */
}

static void on_cloud_disconnected(void *user_data)
{
	/**
	 * TODO: Call function to inform the state machine the disconnection
	 * with the cloud
	 */
}

int device_read_data(int id)
{
	return modbus_read_data(thing.data_item[id].modbus_source.reg_addr,
				thing.data_item[id].modbus_source.bit_offset,
				&thing.data_item[id].value);
}

int device_start(struct settings *settings)
{
	struct conf_files conf;
	int err;

	conf.credentials = settings->credentials_path;
	conf.device = settings->device_path;
	conf.rabbit = settings->rabbitmq_path;

	if (device_set_properties(conf))
		return -EINVAL;

	modbus_start(thing.modbus_slave.url);

	err = cloud_start(thing.rabbitmq_url, thing.user_token,
			  on_cloud_connected, on_cloud_disconnected, NULL);
	if (err < 0)
		return err;

	return 0;
}

void device_destroy(void)
{
	modbus_stop();

	l_free(thing.rabbitmq_url);
	l_free(thing.modbus_slave.url);

	free(thing.data_item);
}
