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
 *  Properties source file
 */

#include <string.h>
#include <stdbool.h>
#include <knot/knot_protocol.h>
#include <knot/knot_types.h>
#include <ell/ell.h>
#include <stdio.h>
#include <errno.h>

#include "device.h"
#include "properties.h"
#include "storage.h"
#include "conf-parameters.h"

#define EMPTY_STRING ""

static int erase_thing_id(struct knot_thing *thing, int cred_fd)
{
	int rc;

	rc = storage_write_key_string(cred_fd, CREDENTIALS_GROUP,
				      CREDENTIALS_THING_ID, EMPTY_STRING);
	if (rc < 0)
		l_error("Failed to erase thing id");
	else
		device_clear_thing_id(thing);

	return rc;
}

static int erase_thing_token(struct knot_thing *thing, int cred_fd)
{
	int rc;

	rc = storage_write_key_string(cred_fd, CREDENTIALS_GROUP,
				      CREDENTIALS_THING_TOKEN, EMPTY_STRING);
	if (rc < 0)
		l_error("Failed to erase thing token");

	else
		device_clear_thing_token(thing);

	return rc;
}

static int set_thing_credentials(struct knot_thing *thing, char *filename)
{
	int cred_fd;
	char *thing_id;
	char *thing_token;

	cred_fd = storage_open(filename);
	if (cred_fd < 0)
		return cred_fd;

	thing_id = storage_read_key_string(cred_fd, CREDENTIALS_GROUP,
					  CREDENTIALS_THING_ID);

	thing_token = storage_read_key_string(cred_fd, CREDENTIALS_GROUP,
					     CREDENTIALS_THING_TOKEN);

	device_set_thing_credentials(thing, thing_id, thing_token);

	l_free(thing_id);
	l_free(thing_token);

	storage_close(cred_fd);

	return 0;
}

static int set_rabbit_mq_url(struct knot_thing *thing, char *filename)
{
	int rabbitmq_fd;
	char *rabbitmq_url_aux;

	rabbitmq_fd = storage_open(filename);
	if (rabbitmq_fd < 0)
		return rabbitmq_fd;

	rabbitmq_url_aux = storage_read_key_string(rabbitmq_fd, RABBIT_MQ_GROUP,
						   RABBIT_URL);
	if (rabbitmq_url_aux == NULL || !strcmp(rabbitmq_url_aux, "")) {
		l_free(rabbitmq_url_aux);
		storage_close(rabbitmq_fd);
		return -EINVAL;
	}
	/* TODO: Check if rabbit mq url is in a valid format */

	device_set_thing_rabbitmq_url(thing, rabbitmq_url_aux);

	storage_close(rabbitmq_fd);

	return 0;
}

static int valid_bit_offset(int bit_offset, int value_type)
{
	int value_type_mask;
	int valid_value_type_mask;

	value_type_mask = 1 << value_type;

	switch (bit_offset) {
	case 1:
		valid_value_type_mask = (1 << KNOT_VALUE_TYPE_BOOL);
		break;
	case 8:
		/* KNoT Protocol doesn't have a matching value type */
	case 16:
		/* KNoT Protocol doesn't have a matching value type */
	case 32:
		valid_value_type_mask = (1 << KNOT_VALUE_TYPE_INT) |
					(1 << KNOT_VALUE_TYPE_UINT);
		break;
	case 64:
		valid_value_type_mask = (1 << KNOT_VALUE_TYPE_INT64) |
					(1 << KNOT_VALUE_TYPE_UINT64);
		break;
	default:
		return -EINVAL;
	}

	if (!(value_type_mask & valid_value_type_mask))
		return -EINVAL;

	return 0;
}

static int set_modbus_source_properties(struct knot_thing *thing,
					int fd, char *group_id,
					knot_schema schema,
					int *reg_addr, int *bit_offset)
{
	int rc;
	int reg_addr_aux;
	int bit_offset_aux;

	rc = storage_read_key_int(fd, group_id, MODBUS_REG_ADDRESS,
				  &reg_addr_aux);
	if (rc <= 0)
		return -EINVAL;

	rc = storage_read_key_int(fd, group_id, MODBUS_BIT_OFFSET,
				  &bit_offset_aux);
	if (rc <= 0)
		return -EINVAL;

	rc = valid_bit_offset(bit_offset_aux, schema.value_type);
	if (rc < 0)
		return -EINVAL;

	*bit_offset = bit_offset_aux;
	*reg_addr = reg_addr_aux;

	return 0;
}

static int assign_limit(int value_type, knot_value_type value,
			knot_value_type *limit)
{
	switch (value_type) {
	case KNOT_VALUE_TYPE_INT:
		limit->val_i = value.val_i;
		break;
	case KNOT_VALUE_TYPE_FLOAT:
		limit->val_f = value.val_f;
		break;
	case KNOT_VALUE_TYPE_BOOL:
		limit->val_b = value.val_b;
		break;
	case KNOT_VALUE_TYPE_INT64:
		limit->val_i64 = value.val_i64;
		break;
	case KNOT_VALUE_TYPE_UINT:
		limit->val_u = value.val_u;
		break;
	case KNOT_VALUE_TYPE_UINT64:
		limit->val_u64 = value.val_u64;
		break;
	case KNOT_VALUE_TYPE_RAW:
		/* Storage doesn't give support to raw */
	default:
		return -EINVAL;
	}

	return 0;
}

static int get_upper_limit(int fd, char *group_id, int value_type,
			   knot_value_type *temp)
{
	int rc;
	knot_value_type_int val_i;
	knot_value_type_float val_f;
	knot_value_type_bool val_b;
	knot_value_type_int64 val_i64;
	knot_value_type_uint val_u;
	knot_value_type_uint64 val_u64;

	switch (value_type) {
	case KNOT_VALUE_TYPE_INT:
		rc = storage_read_key_int(fd, group_id, CONFIG_UPPER_THRESHOLD,
					  &val_i);
		temp->val_i = val_i;
		break;
	case KNOT_VALUE_TYPE_FLOAT:
		rc = storage_read_key_float(fd, group_id,
					    CONFIG_UPPER_THRESHOLD,
					    &val_f);
		temp->val_f = val_f;
		break;
	case KNOT_VALUE_TYPE_BOOL:
		rc = storage_read_key_bool(fd, group_id, CONFIG_UPPER_THRESHOLD,
					   &val_b);
		temp->val_b = val_b;
		break;
	case KNOT_VALUE_TYPE_INT64:
		rc = storage_read_key_int64(fd, group_id,
					    CONFIG_UPPER_THRESHOLD,
					    &val_i64);
		temp->val_i64 = val_i64;
		break;
	case KNOT_VALUE_TYPE_UINT:
		rc = storage_read_key_uint(fd, group_id, CONFIG_UPPER_THRESHOLD,
					   &val_u);
		temp->val_u = val_u;
		break;
	case KNOT_VALUE_TYPE_UINT64:
		rc = storage_read_key_uint64(fd, group_id,
					     CONFIG_UPPER_THRESHOLD,
					     &val_u64);
		temp->val_u64 = val_u64;
		break;
	case KNOT_VALUE_TYPE_RAW:
		/* Storage doesn't give support to raw */
	default:
		return -EINVAL;
	}

	return rc;
}

static int get_lower_limit(int fd, char *group_id, int value_type,
			   knot_value_type *temp)
{
	int rc;
	knot_value_type_int val_i;
	knot_value_type_float val_f;
	knot_value_type_bool val_b;
	knot_value_type_int64 val_i64;
	knot_value_type_uint val_u;
	knot_value_type_uint64 val_u64;

	switch (value_type) {
	case KNOT_VALUE_TYPE_INT:
		rc = storage_read_key_int(fd, group_id, CONFIG_LOWER_THRESHOLD,
					  &val_i);
		temp->val_i = val_i;
		break;
	case KNOT_VALUE_TYPE_FLOAT:
		rc = storage_read_key_float(fd, group_id,
					    CONFIG_LOWER_THRESHOLD,
					    &val_f);
		temp->val_f = val_f;
		break;
	case KNOT_VALUE_TYPE_BOOL:
		rc = storage_read_key_bool(fd, group_id, CONFIG_LOWER_THRESHOLD,
					   &val_b);
		temp->val_b = val_b;
		break;
	case KNOT_VALUE_TYPE_INT64:
		rc = storage_read_key_int64(fd, group_id,
					    CONFIG_LOWER_THRESHOLD,
					    &val_i64);
		temp->val_i64 = val_i64;
		break;
	case KNOT_VALUE_TYPE_UINT:
		rc = storage_read_key_uint(fd, group_id, CONFIG_LOWER_THRESHOLD,
					   &val_u);
		temp->val_u = val_u;
		break;
	case KNOT_VALUE_TYPE_UINT64:
		rc = storage_read_key_uint64(fd, group_id,
					     CONFIG_LOWER_THRESHOLD,
					     &val_u64);
		temp->val_u64 = val_u64;
		break;
	case KNOT_VALUE_TYPE_RAW:
		/* Storage doesn't give support to raw */
	default:
		return -EINVAL;
	}

	return rc;
}

static int set_config(struct knot_thing *thing, int fd, char *group_id,
		      knot_schema schema, knot_config *config)
{
	int rc;
	int aux;
	int value_type_aux;
	knot_value_type tmp_value_type;
	knot_config config_aux;

	memset(&config_aux, 0, sizeof(knot_config));

	value_type_aux = schema.value_type;

	rc = get_lower_limit(fd, group_id, value_type_aux, &tmp_value_type);

	if (rc > 0) {
		config_aux.event_flags |= KNOT_EVT_FLAG_LOWER_THRESHOLD;
		assign_limit(value_type_aux, tmp_value_type,
			     &config_aux.lower_limit);
	}

	rc = get_upper_limit(fd, group_id, value_type_aux, &tmp_value_type);

	if (rc > 0) {
		config_aux.event_flags |= KNOT_EVT_FLAG_UPPER_THRESHOLD;
		assign_limit(value_type_aux, tmp_value_type,
			     &config_aux.upper_limit);
	}

	rc = storage_read_key_int(fd, group_id, CONFIG_TIME_SEC, &aux);
	if (rc > 0) {
		config_aux.event_flags |= KNOT_EVT_FLAG_TIME;
		config_aux.time_sec = aux;
	}

	rc = storage_read_key_int(fd, group_id, CONFIG_CHANGE, &aux);
	if (rc > 0)
		config_aux.event_flags |= KNOT_EVT_FLAG_CHANGE;

	rc = knot_config_is_valid(config_aux.event_flags,
				  schema.value_type,
				  config_aux.time_sec,
				  &config_aux.lower_limit,
				  &config_aux.upper_limit);
	if (rc)
		return -EINVAL;

	*config = config_aux;

	return 0;
}

static int set_schema(struct knot_thing *thing, int fd, char *group_id,
		      knot_schema *schema)
{
	int rc;
	int aux;
	char *name;
	knot_schema schema_aux;

	name = storage_read_key_string(fd, group_id, SCHEMA_SENSOR_NAME);
	if (name == NULL || !strcmp(name, "") ||
			strlen(name) >= KNOT_PROTOCOL_DATA_NAME_LEN)
		return -EINVAL;

	strcpy(schema_aux.name, name);
	l_free(name);

	rc = storage_read_key_int(fd, group_id, SCHEMA_VALUE_TYPE, &aux);
	if (rc <= 0)
		return -EINVAL;
	schema_aux.value_type = aux;

	rc = storage_read_key_int(fd, group_id, SCHEMA_UNIT, &aux);
	if (rc <= 0)
		return -EINVAL;
	schema_aux.unit = aux;

	rc = storage_read_key_int(fd, group_id, SCHEMA_TYPE_ID, &aux);
	if (rc <= 0)
		return -EINVAL;
	schema_aux.type_id = aux;

	rc = knot_schema_is_valid(schema_aux.type_id,
				  schema_aux.value_type,
				  schema_aux.unit);
	if (rc)
		return -EINVAL;

	*schema = schema_aux;

	return 0;
}

static int set_sensor_id(struct knot_thing *thing, int fd, char *group_id,
			 int *sensor_id)
{
	int rc;
	int sensor_id_aux;

	rc = storage_read_key_int(fd, group_id, SCHEMA_SENSOR_ID,
				  &sensor_id_aux);
	if (rc <= 0)
		return -EINVAL;

	if (device_data_item_lookup(thing, L_INT_TO_PTR(sensor_id_aux)))
		return -EINVAL;

	*sensor_id = sensor_id_aux;

	return 0;
}

static int set_data_items(struct knot_thing *thing, int fd)
{
	int rc;
	int i;
	char **data_item_group;

	int sensor_id;
	int reg_addr;
	int bit_offset;
	knot_schema schema;
	knot_config config;

	data_item_group = get_data_item_groups(fd);

	for (i = 0; data_item_group[i] != NULL ; i++) {
		rc = set_sensor_id(thing, fd, data_item_group[i], &sensor_id);
		if (rc < 0)
			goto error;

		rc = set_schema(thing, fd, data_item_group[i], &schema);
		if (rc < 0)
			goto error;

		rc = set_config(thing, fd, data_item_group[i], schema, &config);
		if (rc < 0)
			goto error;

		rc = set_modbus_source_properties(thing, fd, data_item_group[i],
						  schema, &reg_addr,
						  &bit_offset);
		if (rc < 0)
			goto error;

		device_set_new_data_item(thing, sensor_id, schema, config,
					 reg_addr, bit_offset);
	}

	l_strfreev(data_item_group);

	return 0;

error:
	l_strfreev(data_item_group);

	return -EINVAL;
}

static int set_modbus_slave_properties(struct knot_thing *thing, int fd)
{
	int rc;
	int aux;
	int id;
	char *url;

	rc = storage_read_key_int(fd, THING_GROUP, THING_MODBUS_SLAVE_ID, &aux);
	if (rc <= 0)
		return -EINVAL;

	if (aux < MODBUS_MIN_SLAVE_ID || aux > MODBUS_MAX_SLAVE_ID)
		return -EINVAL;

	id = aux;

	url = storage_read_key_string(fd, THING_GROUP, THING_MODBUS_URL);
	if (url == NULL || !strcmp(url, ""))
		return -EINVAL;
	/* TODO: Check if modbus url is in a valid format */

	device_set_thing_modbus_slave(thing, id, url);

	return 0;
}

static int set_thing_user_token(struct knot_thing *thing, int fd)
{
	char *user_token;

	user_token = storage_read_key_string(fd, THING_GROUP, THING_USER_TOKEN);
	if (user_token == NULL || !strcmp(user_token, "")) {
		l_free(user_token);
		return -EINVAL;
	}
	device_set_thing_user_token(thing, user_token);

	return 0;
}

static int set_thing_name(struct knot_thing *thing, int fd)
{
	char *knot_thing_name;

	knot_thing_name = storage_read_key_string(fd, THING_GROUP, THING_NAME);
	if (knot_thing_name == NULL || !strcmp(knot_thing_name, "") ||
	    strlen(knot_thing_name) >= KNOT_PROTOCOL_DEVICE_NAME_LEN) {
		l_free(knot_thing_name);
		return -EINVAL;
	}

	device_set_thing_name(thing, knot_thing_name);
	l_free(knot_thing_name);

	return 0;
}

static int set_thing_properties(struct knot_thing *thing, char *filename)
{
	int device_fd;
	int rc;

	device_fd = storage_open(filename);
	if (device_fd < 0)
		return device_fd;

	rc = set_thing_name(thing, device_fd);
	if (rc < 0) {
		storage_close(device_fd);
		return rc;
	}

	rc = set_thing_user_token(thing, device_fd);
	if (rc < 0) {
		storage_close(device_fd);
		return rc;
	}

	rc = set_modbus_slave_properties(thing, device_fd);
	if (rc < 0) {
		storage_close(device_fd);
		return rc;
	}

	rc = set_data_items(thing, device_fd);
	if (rc < 0) {
		storage_close(device_fd);
		return rc;
	}

	storage_close(device_fd);

	return rc;
}

int properties_create_device(struct knot_thing *thing,
			     struct device_settings *conf_files)
{
	int rc;

	rc = set_thing_properties(thing, conf_files->device_path);
	if (rc < 0)
		return rc;

	rc = set_rabbit_mq_url(thing, conf_files->rabbitmq_path);
	if (rc < 0)
		return rc;

	rc = set_thing_credentials(thing, conf_files->credentials_path);
	if (rc < 0)
		return rc;

	device_set_thing_credentials_path(thing, conf_files->credentials_path);

	return 0;
}

int properties_clear_credentials(struct knot_thing *thing, char *filename)
{
	int rc;
	int cred_fd;

	cred_fd = storage_open(filename);
	if (cred_fd < 0) {
		l_error("Failed to open credentials file");
		return cred_fd;
	}

	rc = erase_thing_token(thing, cred_fd);
	if (rc < 0)
		goto error;

	rc = erase_thing_id(thing, cred_fd);
	if (rc < 0)
		goto error;

	storage_close(cred_fd);

	return rc;

error:
	l_error("Failed to clear device credentials");
	storage_close(cred_fd);

	return -EINVAL;
}

int properties_store_credentials(struct knot_thing *thing, char *filename,
				 char *id, char *token)
{
	int rc;
	int cred_fd;

	if (strlen(token) > KNOT_PROTOCOL_TOKEN_LEN)
		return -EINVAL;

	cred_fd = storage_open(filename);

	if (cred_fd < 0) {
		l_error("Failed to open credentials file");
		return cred_fd;
	}

	rc = storage_write_key_string(cred_fd, CREDENTIALS_GROUP,
				      CREDENTIALS_THING_TOKEN, token);
	if (rc < 0)
		goto error;

	rc = storage_write_key_string(cred_fd, CREDENTIALS_GROUP,
				      CREDENTIALS_THING_ID, id);
	if (rc < 0) {
		storage_write_key_string(cred_fd, CREDENTIALS_GROUP,
					 CREDENTIALS_THING_TOKEN,
					 EMPTY_STRING);
		goto error;
	}

	storage_close(cred_fd);

	return rc;

error:
	l_error("Failed to store device credentials");
	storage_close(cred_fd);

	return -EINVAL;
}
