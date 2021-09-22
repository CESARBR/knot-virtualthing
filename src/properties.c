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
	if (cred_fd < 0) {
		l_error("Failed to open credentials file");
		return cred_fd;
	}

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

static int set_rabbit_mq_url(struct knot_thing *thing, int rabbitmq_fd)
{
	char *rabbitmq_url_aux;

	rabbitmq_url_aux = storage_read_key_string(rabbitmq_fd, CLOUD_GROUP,
						   RABBIT_URL);
	if (rabbitmq_url_aux == NULL || !strcmp(rabbitmq_url_aux, "")) {
		l_free(rabbitmq_url_aux);
		return -EINVAL;
	}
	/* TODO: Check if rabbit mq url is in a valid format */

	device_set_thing_rabbitmq_url(thing, rabbitmq_url_aux);

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
					int *reg_addr, int *bit_offset,
					int *endianness_type)
{
	int rc;
	int reg_addr_aux;
	int bit_offset_aux;
	int endianness_type_aux;

	rc = storage_read_key_int(fd, group_id, MODBUS_REG_ADDRESS,
				  &reg_addr_aux);
	if (rc <= 0)
		return -EINVAL;

	rc = storage_read_key_int(fd, group_id, MODBUS_TYPE_ENDIANNESS,
				  &endianness_type_aux);
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
	*endianness_type = endianness_type_aux;
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
		rc = storage_read_key_int(fd, group_id, EVENT_UPPER_THRESHOLD,
					  &val_i);
		temp->val_i = val_i;
		break;
	case KNOT_VALUE_TYPE_FLOAT:
		rc = storage_read_key_float(fd, group_id,
					    EVENT_UPPER_THRESHOLD,
					    &val_f);
		temp->val_f = val_f;
		break;
	case KNOT_VALUE_TYPE_BOOL:
		rc = storage_read_key_bool(fd, group_id, EVENT_UPPER_THRESHOLD,
					   &val_b);
		temp->val_b = val_b;
		break;
	case KNOT_VALUE_TYPE_INT64:
		rc = storage_read_key_int64(fd, group_id,
					    EVENT_UPPER_THRESHOLD,
					    &val_i64);
		temp->val_i64 = val_i64;
		break;
	case KNOT_VALUE_TYPE_UINT:
		rc = storage_read_key_uint(fd, group_id, EVENT_UPPER_THRESHOLD,
					   &val_u);
		temp->val_u = val_u;
		break;
	case KNOT_VALUE_TYPE_UINT64:
		rc = storage_read_key_uint64(fd, group_id,
					     EVENT_UPPER_THRESHOLD,
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
		rc = storage_read_key_int(fd, group_id, EVENT_LOWER_THRESHOLD,
					  &val_i);
		temp->val_i = val_i;
		break;
	case KNOT_VALUE_TYPE_FLOAT:
		rc = storage_read_key_float(fd, group_id,
					    EVENT_LOWER_THRESHOLD,
					    &val_f);
		temp->val_f = val_f;
		break;
	case KNOT_VALUE_TYPE_BOOL:
		rc = storage_read_key_bool(fd, group_id, EVENT_LOWER_THRESHOLD,
					   &val_b);
		temp->val_b = val_b;
		break;
	case KNOT_VALUE_TYPE_INT64:
		rc = storage_read_key_int64(fd, group_id,
					    EVENT_LOWER_THRESHOLD,
					    &val_i64);
		temp->val_i64 = val_i64;
		break;
	case KNOT_VALUE_TYPE_UINT:
		rc = storage_read_key_uint(fd, group_id, EVENT_LOWER_THRESHOLD,
					   &val_u);
		temp->val_u = val_u;
		break;
	case KNOT_VALUE_TYPE_UINT64:
		rc = storage_read_key_uint64(fd, group_id,
					     EVENT_LOWER_THRESHOLD,
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

static int set_limit(int fd, const char *group_id, const char *key,
		     int value_type, knot_value_type value)
{
	int rc;

	switch (value_type) {
	case KNOT_VALUE_TYPE_INT:
		rc = storage_write_key_int(fd, group_id, key, value.val_i);
		break;
	case KNOT_VALUE_TYPE_FLOAT:
		rc = storage_write_key_float(fd, group_id, key, value.val_f);
		break;
	case KNOT_VALUE_TYPE_BOOL:
		rc = storage_write_key_bool(fd, group_id, key, value.val_b);
		break;
	case KNOT_VALUE_TYPE_INT64:
		rc = storage_write_key_int64(fd, group_id, key, value.val_i64);
		break;
	case KNOT_VALUE_TYPE_UINT:
		rc = storage_write_key_uint(fd, group_id, key, value.val_u);
		break;
	case KNOT_VALUE_TYPE_UINT64:
		rc = storage_write_key_uint64(fd, group_id, key, value.val_u64);
		break;
	case KNOT_VALUE_TYPE_RAW:
		/* Storage doesn't give support to raw */
	default:
		return -EINVAL;
	}

	return rc;
}

static int set_event(struct knot_thing *thing, int fd, char *group_id,
		     knot_schema schema, knot_event *event)
{
	int rc;
	int aux;
	int value_type_aux;
	knot_value_type tmp_value_type;
	knot_event event_aux;

	memset(&event_aux, 0, sizeof(knot_event));

	value_type_aux = schema.value_type;

	rc = get_lower_limit(fd, group_id, value_type_aux, &tmp_value_type);

	if (rc > 0) {
		event_aux.event_flags |= KNOT_EVT_FLAG_LOWER_THRESHOLD;
		knot_value_assign_limit(value_type_aux, tmp_value_type,
					&event_aux.lower_limit);
	}

	rc = get_upper_limit(fd, group_id, value_type_aux, &tmp_value_type);

	if (rc > 0) {
		event_aux.event_flags |= KNOT_EVT_FLAG_UPPER_THRESHOLD;
		knot_value_assign_limit(value_type_aux, tmp_value_type,
					&event_aux.upper_limit);
	}

	rc = storage_read_key_int(fd, group_id, EVENT_TIME_SEC, &aux);
	if (rc > 0) {
		event_aux.event_flags |= KNOT_EVT_FLAG_TIME;
		event_aux.time_sec = aux;
	}

	rc = storage_read_key_int(fd, group_id, EVENT_CHANGE, &aux);
	if (rc > 0)
		event_aux.event_flags |= KNOT_EVT_FLAG_CHANGE;

	rc = knot_event_is_valid(event_aux.event_flags,
				 schema.value_type,
				 event_aux.time_sec,
				 &event_aux.lower_limit,
				 &event_aux.upper_limit);
	if (rc)
		return -EINVAL;

	*event = event_aux;

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

	if (device_data_item_lookup(thing, sensor_id_aux))
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
	int endianness_type;
	knot_schema schema;
	knot_event event;

	data_item_group = get_data_item_groups(fd);

	if (!data_item_group) {
		l_error("Failed to read DataItem groups");
		goto error;
	}

	for (i = 0; data_item_group[i] != NULL ; i++) {
		rc = set_sensor_id(thing, fd, data_item_group[i], &sensor_id);
		if (rc < 0) {
			l_error("Failed to set Sensor ID on %s",
				data_item_group[i]);
			goto error;
		}

		rc = set_schema(thing, fd, data_item_group[i], &schema);
		if (rc < 0) {
			l_error("Failed to set Schema on %s",
				data_item_group[i]);
			goto error;
		}

		rc = set_event(thing, fd, data_item_group[i], schema, &event);
		if (rc < 0) {
			l_error("Failed to set event on %s",
				data_item_group[i]);
			goto error;
		}

		rc = set_modbus_source_properties(thing, fd, data_item_group[i],
						  schema, &reg_addr,
						  &bit_offset,
						  &endianness_type);
		if (rc < 0) {
			l_error("Failed to set Modbus Source properties on %s",
				data_item_group[i]);
			goto error;
		}

		device_set_new_data_item(thing, sensor_id, schema, event,
					 reg_addr, bit_offset, endianness_type);
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

	user_token = storage_read_key_string(fd, CLOUD_GROUP, THING_USER_TOKEN);
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
	if (device_fd < 0) {
		l_error("Failed to open device file");
		return device_fd;
	}

	rc = set_thing_name(thing, device_fd);
	if (rc < 0) {
		l_error("Failed to set Thing name");
		storage_close(device_fd);
		return rc;
	}

	rc = set_modbus_slave_properties(thing, device_fd);
	if (rc < 0) {
		l_error("Failed to set Modbus Slave properties");
		storage_close(device_fd);
		return rc;
	}

	rc = set_data_items(thing, device_fd);
	if (rc < 0) {
		l_error("Failed to set KNoT Data items");
		storage_close(device_fd);
		return rc;
	}

	storage_close(device_fd);

	return rc;
}

static int set_cloud_properties(struct knot_thing *thing, char *filename)
{
	int rc;
	int cloud_fd;

	cloud_fd = storage_open(filename);
	if (cloud_fd < 0) {
		l_error("Failed to open cloud file");
		return cloud_fd;
	}

	rc = set_rabbit_mq_url(thing, cloud_fd);
	if (rc < 0) {
		l_error("Failed to set RabbitMQ path");
		storage_close(cloud_fd);
		return rc;
	}

	rc = set_thing_user_token(thing, cloud_fd);
	if (rc < 0) {
		l_error("Failed to set User Token");
		storage_close(cloud_fd);
		return rc;
	}

	storage_close(cloud_fd);

	return rc;
}

static int update_event_data_item(int fd, const char *group_id,
				  int value_type, knot_event *event)
{
	int rc;

	if (event->event_flags & KNOT_EVT_FLAG_TIME) {
		rc = storage_write_key_int(fd, group_id, EVENT_TIME_SEC,
					   event->time_sec);
		if (rc < 0) {
			l_error("Failed to set new time sec");
			return rc;
		}
	} else if (!storage_has_unit(fd, group_id, EVENT_TIME_SEC)) {
		storage_remove_key(fd, group_id, EVENT_TIME_SEC);
	}

	if (event->event_flags & KNOT_EVT_FLAG_CHANGE) {
		rc = storage_write_key_int(fd, group_id, EVENT_CHANGE,
					   EVENT_CHANGE_TRUE);
		if (rc < 0) {
			l_error("Failed to set new change flag");
			return rc;
		}
	} else if (!storage_has_unit(fd, group_id, EVENT_CHANGE)) {
		storage_remove_key(fd, group_id, EVENT_CHANGE);
	}

	if (event->event_flags & KNOT_EVT_FLAG_LOWER_THRESHOLD) {
		rc = set_limit(fd, group_id, EVENT_LOWER_THRESHOLD, value_type,
			       event->lower_limit);
		if (rc < 0) {
			l_error("Failed to set new lower threshold");
			return rc;
		}
	} else if (!storage_has_unit(fd, group_id, EVENT_LOWER_THRESHOLD)) {
		storage_remove_key(fd, group_id, EVENT_LOWER_THRESHOLD);
	}

	if (event->event_flags & KNOT_EVT_FLAG_UPPER_THRESHOLD) {
		rc = set_limit(fd, group_id, EVENT_UPPER_THRESHOLD, value_type,
			       event->upper_limit);
		if (rc < 0) {
			l_error("Failed to set new upper threshold");
			return rc;
		}
	} else if (!storage_has_unit(fd, group_id, EVENT_UPPER_THRESHOLD)) {
		storage_remove_key(fd, group_id, EVENT_UPPER_THRESHOLD);
	}

	return 0;
}

static int update_schema_data_item(int fd, const char *group_id,
				   knot_schema *schema)
{
	int rc;

	rc = storage_write_key_int(fd, group_id, SCHEMA_TYPE_ID,
				   schema->type_id);
	if (rc < 0) {
		l_error("Failed to set new type id");
		return rc;
	}

	rc = storage_write_key_int(fd, group_id, SCHEMA_UNIT, schema->unit);
	if (rc < 0) {
		l_error("Failed to set new unit");
		return rc;
	}

	rc = storage_write_key_int(fd, group_id, SCHEMA_VALUE_TYPE,
				   schema->value_type);
	if (rc < 0) {
		l_error("Failed to set new value_type");
		return rc;
	}

	rc = storage_write_key_string(fd, group_id, SCHEMA_SENSOR_NAME,
				      schema->name);
	if (rc < 0) {
		l_error("Failed to set new name");
		return rc;
	}

	return rc;
}

static bool equal_sensor_id(int fd, const char *group_id, int sensor_id)
{
	int sensor_id_aux;
	int rc;

	rc = storage_read_key_int(fd, group_id, SCHEMA_SENSOR_ID,
				  &sensor_id_aux);
	if (rc <= 0)
		return false;

	return sensor_id == sensor_id_aux ? true : false;
}

int properties_create_device(struct knot_thing *thing,
			     struct device_settings *conf_files)
{
	int rc;

	rc = set_thing_properties(thing, conf_files->device_path);
	if (rc < 0) {
		l_error("Failed to set Thing properties");
		return rc;
	}

	rc = set_cloud_properties(thing, conf_files->cloud_path);
	if (rc < 0) {
		l_error("Failed to set Cloud properties");
		return rc;
	}

	rc = set_thing_credentials(thing, conf_files->credentials_path);
	if (rc < 0) {
		l_error("Failed to set Thing credentials");
		return rc;
	}

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

int properties_update_data_item(struct knot_thing *thing, char *filename,
				knot_msg_config *config)
{
	char **data_item_group;
	int device_fd;
	int i;
	bool has_err;

	device_fd = storage_open(filename);
	if (device_fd < 0) {
		l_error("Failed to open device file");
		return device_fd;
	}

	/* Update values on knot_thing struct */
	device_update_config_data_item(thing, config);

	has_err = false;
	data_item_group = get_data_item_groups(device_fd);

	for (i = 0; data_item_group[i] != NULL; i++) {
		if (!equal_sensor_id(device_fd, data_item_group[i],
				     config->sensor_id))
			continue;

		if (update_schema_data_item(device_fd, data_item_group[i],
					    &config->schema) < 0) {
			has_err = true;
			l_error("Error on set schema property");
		}

		if (!(config->event.event_flags & KNOT_EVT_FLAG_UNREGISTERED)) {
			if (update_event_data_item(device_fd,
						   data_item_group[i],
						   config->schema.value_type,
						   &config->event) < 0) {
				has_err = true;
				l_error("Error on set event property");
			}
		}
		break;
	}

	storage_close(device_fd);

	if (has_err)
		return -1;

	return 0;
}
