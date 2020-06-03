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
#include <stdbool.h>
#include <knot/knot_protocol.h>
#include <knot/knot_types.h>
#include <ell/ell.h>
#include <stdio.h>
#include <errno.h>

#include "storage.h"
#include "settings.h"
#include "conf-parameters.h"
#include "device.h"
#include "device-pvt.h"
#include "modbus-interface.h"
#include "cloud.h"
#include "sm.h"
#include "knot-config.h"
#include "poll.h"

#define CONNECTED_MASK		0xFF
#define set_conn_bitmask(a, b1, b2) (a) ? (b1) | (b2) : (b1) & ~(b2)
#define EMPTY_STRING ""
#define DEFAULT_POLLING_INTERVAL 1

struct knot_thing thing;

enum CONN_TYPE {
	MODBUS = 0x0F,
	CLOUD = 0xF0
};

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
	knot_value_type current_val;
	knot_value_type sent_val;
	struct modbus_source modbus_source;
};

struct knot_thing {
	char token[KNOT_PROTOCOL_TOKEN_LEN + 1];
	char id[KNOT_PROTOCOL_UUID_LEN + 1];
	char name[KNOT_PROTOCOL_DEVICE_NAME_LEN];
	char *user_token;

	struct modbus_slave modbus_slave;
	char *rabbitmq_url;
	char *credentials_path;

	int data_item_count;
	struct knot_data_item *data_item;
};

static void knot_thing_destroy(struct knot_thing *thing)
{
	l_free(thing->user_token);
	l_free(thing->rabbitmq_url);
	l_free(thing->modbus_slave.url);
	l_free(thing->credentials_path);

	free(thing->data_item);
}

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
	l_free(modbus_slave_aux.url);
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
		l_free(rabbitmq_url_aux);
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

static int set_config(char *filename, char *group_id, int sensor_id)
{
	int rc;
	int aux;
	int device_fd;
	int value_type_aux;
	knot_value_type tmp_value_type;
	knot_config config_aux;

	memset(&config_aux, 0, sizeof(knot_config));

	device_fd = storage_open(filename);
	if (device_fd < 0)
		return device_fd;

	value_type_aux = thing.data_item[sensor_id].schema.value_type;

	rc = get_lower_limit(device_fd, group_id, value_type_aux,
			     &tmp_value_type);

	if (rc > 0) {
		config_aux.event_flags |= KNOT_EVT_FLAG_LOWER_THRESHOLD;
		assign_limit(value_type_aux, tmp_value_type,
			     &config_aux.lower_limit);
	}

	rc = get_upper_limit(device_fd, group_id, value_type_aux,
			     &tmp_value_type);

	if (rc > 0) {
		config_aux.event_flags |= KNOT_EVT_FLAG_UPPER_THRESHOLD;
		assign_limit(value_type_aux, tmp_value_type,
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
	int sensor_id;

	device_fd = storage_open(filename);
	if (device_fd < 0)
		return device_fd;

	thing.data_item_count = get_number_of_data_items(device_fd);
	thing.data_item = malloc(thing.data_item_count *
						sizeof(struct knot_data_item));
	if (thing.data_item == NULL) {
		storage_close(device_fd);
		return -EINVAL;
	}

	data_item_group = get_data_item_groups(device_fd);

	storage_close(device_fd);
	for (i = 0; data_item_group[i] != NULL ; i++) {
		sensor_id = get_sensor_id(filename, data_item_group[i]);
		rc = valid_sensor_id(sensor_id, thing.data_item_count);
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
		l_free(user_token);
		storage_close(device_fd);
		return -EINVAL;
	}
	thing.user_token = user_token;

	storage_close(device_fd);

	return 0;
}

static int set_thing_credentials(char *filename)
{
	int cred_fd;
	char *thingid;
	char *thingtoken;

	cred_fd = storage_open(filename);
	if (cred_fd < 0)
		return cred_fd;

	thingid = storage_read_key_string(cred_fd, CREDENTIALS_GROUP,
					  CREDENTIALS_THING_ID);

	thingtoken = storage_read_key_string(cred_fd, CREDENTIALS_GROUP,
					     CREDENTIALS_THING_TOKEN);

	strncpy(thing.id, thingid, KNOT_PROTOCOL_UUID_LEN);
	strncpy(thing.token, thingtoken, KNOT_PROTOCOL_TOKEN_LEN);
	l_free(thingid);
	l_free(thingtoken);

	storage_close(cred_fd);

	return 0;
}

static int device_set_properties(struct device_settings *conf_files)
{
	int rc;

	rc = set_thing_name(conf_files->device_path);
	if (rc < 0)
		return rc;

	rc = set_rabbit_mq_url(conf_files->rabbitmq_path);
	if (rc < 0)
		return rc;

	rc = set_thing_user_token(conf_files->device_path);
	if (rc < 0)
		return rc;

	rc = set_modbus_slave_properties(conf_files->device_path);
	if (rc < 0)
		return rc;

	rc = set_data_items(conf_files->device_path);
	if (rc < 0)
		return rc;

	rc = set_thing_credentials(conf_files->credentials_path);
	if (rc < 0)
		return rc;

	thing.credentials_path = l_strdup(conf_files->credentials_path);

	return 0;
}

static void conn_handler(enum CONN_TYPE conn, bool is_up)
{
	static uint8_t conn_mask;

	conn_mask = set_conn_bitmask(is_up, conn_mask, conn);

	if(conn_mask != CONNECTED_MASK) {
		sm_input_event(EVT_NOT_READY, NULL);
		return;
	}

	sm_input_event(EVT_READY, NULL);
}

static bool on_cloud_receive(const struct cloud_msg *msg, void *user_data)
{
	switch (msg->type) {
	case UPDATE_MSG:
		if (!msg->error)
			sm_input_event(EVT_DATA_UPDT, msg->list);
		break;
	case REQUEST_MSG:
		if (!msg->error)
			sm_input_event(EVT_PUB_DATA, msg->list);
		break;
	case REGISTER_MSG:
		if (msg->error)
			sm_input_event(EVT_REG_NOT_OK, NULL);
		else
			sm_input_event(EVT_REG_OK, (char *) msg->token);
		break;
	case UNREGISTER_MSG:
		if (!msg->error)
			sm_input_event(EVT_UNREG_REQ, NULL);
		break;
	case AUTH_MSG:
		if (msg->error)
			sm_input_event(EVT_AUTH_NOT_OK, NULL);
		else
			sm_input_event(EVT_AUTH_OK, NULL);
		break;
	case SCHEMA_MSG:
		if (msg->error)
			sm_input_event(EVT_SCH_NOT_OK, NULL);
		else
			sm_input_event(EVT_SCH_OK, NULL);
		break;
	default:
		return true;
	}

	return true;
}

static void on_cloud_connected(void *user_data)
{
	int err;

	err = cloud_set_read_handler(on_cloud_receive, NULL);
	if (err < 0)
		return;

	l_info("Connected to Cloud %s", thing.rabbitmq_url);

	conn_handler(CLOUD, true);
}

static void on_cloud_disconnected(void *user_data)
{
	l_info("Disconnected from Cloud");

	conn_handler(CLOUD, false);
}

static int erase_thing_token(int cred_fd)
{
	int rc;

	rc = storage_write_key_string(cred_fd, CREDENTIALS_GROUP,
				      CREDENTIALS_THING_TOKEN, EMPTY_STRING);
	if (rc < 0)
		l_error("Failed to erase thing token");

	else
		thing.token[0] = '\0';

	return rc;
}

static int erase_thing_id(int cred_fd)
{
	int rc;

	rc = storage_write_key_string(cred_fd, CREDENTIALS_GROUP,
				      CREDENTIALS_THING_ID, EMPTY_STRING);
	if (rc < 0)
		l_error("Failed to erase thing id");
	else
		thing.id[0] = '\0';

	return rc;
}

static void on_modbus_connected(void *user_data)
{
	l_info("Connected to Modbus %s", thing.modbus_slave.url);

	poll_start();
	conn_handler(MODBUS, true);
}

static void on_modbus_disconnected(void *user_data)
{
	l_info("Disconnected from Modbus");

	poll_stop();
	conn_handler(MODBUS, false);
}

static void on_publish_data(void *data, void *user_data)
{
	struct knot_data_item data_item;
	int *sensor_id = data;
	int rc;

	if (*sensor_id >= thing.data_item_count)
		return;

	data_item = thing.data_item[*sensor_id];

	rc = cloud_publish_data(thing.id, *sensor_id,
				data_item.schema.value_type,
				&data_item.current_val,
				sizeof(data_item.schema.value_type));
	if (rc < 0)
		l_error("Couldn't send data_update for data_item #%d",
			*sensor_id);
}

static void on_config_timeout(int id)
{
	struct l_queue *list;

	list = l_queue_new();
	l_queue_push_head(list, &id);

	sm_input_event(EVT_PUB_DATA, list);

	l_queue_destroy(list, NULL);
}

static int create_data_item_polling(void)
{
	int i;
	int rc;

	for (i = 0; i < thing.data_item_count; i++) {
		rc = poll_create(DEFAULT_POLLING_INTERVAL, i,
				 device_read_data);
		if (rc < 0) {
			poll_destroy();
			return rc;
		}
	}

	return 0;
}

int device_start_config(void)
{
	int rc;
	int i;

	rc = config_start(on_config_timeout);
	if (rc < 0) {
		l_error("Failed to start config");
		return rc;
	}

	for (i = 0; i < thing.data_item_count; i++)
		config_add_data_item(thing.data_item[i].sensor_id,
				     thing.data_item[i].config);

	return 0;
}

void device_stop_config(void)
{
	config_stop();
}

int device_read_data(int id)
{
	struct l_queue *list;
	int rc;

	rc = modbus_read_data(thing.data_item[id].modbus_source.reg_addr,
			      thing.data_item[id].modbus_source.bit_offset,
			      &thing.data_item[id].current_val);
	if (config_check_value(thing.data_item[id].config,
			       thing.data_item[id].current_val,
			       thing.data_item[id].sent_val,
			       thing.data_item[id].schema.value_type) > 0) {
		thing.data_item[id].sent_val = thing.data_item[id].current_val;
		list = l_queue_new();
		l_queue_push_head(list, &id);

		sm_input_event(EVT_PUB_DATA, list);

		l_queue_destroy(list, NULL);
	}

	return rc;
}

int device_check_schema_change(void)
{
	/* TODO: Add schema change verification */
	return 1;
}

int device_send_auth_request(void)
{
	return cloud_auth_device(thing.id, thing.token);
}

int device_has_credentials(void)
{
	return thing.token[0] != '\0';
}

int device_clear_credentials(void)
{
	int rc;
	int cred_fd;

	cred_fd = storage_open(thing.credentials_path);
	if (cred_fd < 0) {
		l_error("Failed to open credentials file");
		return cred_fd;
	}

	rc = erase_thing_token(cred_fd);
	if (rc < 0)
		goto error;

	rc = erase_thing_id(cred_fd);
	if (rc < 0)
		goto error;

	storage_close(cred_fd);

	return rc;

error:
	l_error("Failed to clear device credentials");
	storage_close(cred_fd);

	return -EINVAL;
}

int device_store_credentials(char *token)
{
	int rc;
	int cred_fd;

	if (strlen(token) > KNOT_PROTOCOL_TOKEN_LEN)
		return -EINVAL;

	cred_fd = storage_open(thing.credentials_path);

	if (cred_fd < 0) {
		l_error("Failed to open credentials file");
		return cred_fd;
	}

	rc = storage_write_key_string(cred_fd, CREDENTIALS_GROUP,
				      CREDENTIALS_THING_TOKEN, token);
	if(rc < 0)
		goto error;

	strncpy(thing.token, token, KNOT_PROTOCOL_TOKEN_LEN);

	rc = storage_write_key_string(cred_fd, CREDENTIALS_GROUP,
				      CREDENTIALS_THING_ID, thing.id);
	if(rc < 0) {
		erase_thing_token(cred_fd);
		goto error;
	}

	storage_close(cred_fd);

	return rc;

error:
	l_error("Failed to store device credentials");
	storage_close(cred_fd);

	return -EINVAL;
}

char *device_get_id(void)
{
	return thing.id;
}

void device_generate_new_id()
{
	uint64_t id; /* knot id uses 16 characters which fits inside a uint64 */

	l_getrandom(&id, sizeof(id));
	sprintf(thing.id, "%"PRIx64, id); /* PRIx64 formats the string as hex */
}

int device_send_register_request()
{
	return cloud_register_device(thing.id, thing.name);
}

int device_send_schema()
{
	struct l_queue *schema_queue;
	knot_msg_schema schema_aux;
	int size;
	int rc;
	int i;

	schema_queue = l_queue_new();
	size = sizeof(knot_msg_schema);

	for(i = 0; i < thing.data_item_count; i++) {
		schema_aux.sensor_id = i;
		schema_aux.values = thing.data_item[i].schema;
		l_queue_push_head(schema_queue, l_memdup(&schema_aux, size));
	}

	rc = cloud_update_schema(thing.id, schema_queue);

	l_queue_destroy(schema_queue, l_free);

	return rc;
}

void device_publish_data_list(struct l_queue *sensor_id_list)
{
	l_queue_foreach(sensor_id_list, on_publish_data, NULL);
}

void device_publish_data_all(void)
{
	int sensor_id;

	for (sensor_id = 0; sensor_id < thing.data_item_count; sensor_id++)
		on_publish_data(&sensor_id, NULL);
}

int device_start(struct device_settings *conf_files)
{
	int err;

	if (device_set_properties(conf_files)) {
		l_error("Failed to set device properties");
		return -EINVAL;
	}

	sm_start();

	err = create_data_item_polling();
	if (err < 0) {
		l_error("Failed to create the device polling");
		knot_thing_destroy(&thing);
		return err;
	}

	err = modbus_start(thing.modbus_slave.url, thing.modbus_slave.id,
			   on_modbus_connected, on_modbus_disconnected, NULL);
	if (err < 0) {
		l_error("Failed to initialize Modbus");
		poll_destroy();
		knot_thing_destroy(&thing);
		return err;
	}

	err = cloud_start(thing.rabbitmq_url, thing.user_token,
			  on_cloud_connected, on_cloud_disconnected, NULL);
	if (err < 0) {
		l_error("Failed to initialize Cloud");
		poll_destroy();
		modbus_stop();
		knot_thing_destroy(&thing);
		return err;
	}

	l_info("Device \"%s\" has started successfully", thing.name);

	return 0;
}

void device_destroy(void)
{
	config_stop();

	poll_destroy();
	cloud_stop();
	modbus_stop();

	knot_thing_destroy(&thing);
}
