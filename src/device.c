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
#include <ell/ell.h>
#include <stdio.h>
#include <errno.h>
#include <knot/knot_protocol.h>
#include <knot/knot_types.h>
#include <knot/knot_cloud.h>

#include "storage.h"
#include "settings.h"
#include "conf-parameters.h"
#include "device.h"
#include "device-pvt.h"
#include "iface-modbus.h"
#include "iface-ethernet-ip.h"
#include "sm.h"
#include "event.h"
#include "poll.h"
#include "properties.h"
#include "conf-driver.h"

#define CONNECTED_MASK		0xFF
#define set_conn_bitmask(a, b1, b2) (a) ? (b1) | (b2) : (b1) & ~(b2)
#define DEFAULT_POLLING_INTERVAL 1

struct knot_thing thing;

static void knot_thing_destroy(struct knot_thing *thing)
{
	if (thing->msg_to)
		l_timeout_remove(thing->msg_to);

	l_free(thing->user_token);
	l_free(thing->rabbitmq_url);
	l_free(thing->geral_url);
	l_free(thing->conf_files.credentials_path);
	l_free(thing->conf_files.device_path);
	l_free(thing->conf_files.cloud_path);

	l_hashmap_destroy(thing->data_items, l_free);
}

static void foreach_event_add_data_item(const void *key, void *value,
					void *user_data)
{
	struct knot_data_item *data_item = value;

	event_add_data_item(data_item->sensor_id, data_item->event);
}

static void foreach_update_config(void *data, void *user_data)
{
	knot_msg_config *config = data;

	properties_update_data_item(&thing, thing.conf_files.device_path,
				    config);
}

static void foreach_send_config(const void *key, void *value, void *user_data)
{
	struct knot_data_item *data_item = value;
	struct l_queue *config_queue = user_data;
	knot_msg_config config_aux;

	config_aux.sensor_id = data_item->sensor_id;
	config_aux.schema = data_item->schema;
	config_aux.event = data_item->event;
	l_queue_push_head(config_queue, l_memdup(&config_aux,
						 sizeof(knot_msg_config)));
}

static void on_publish_data(void *data, void *user_data)
{
	struct knot_data_item *data_item;
	int *sensor_id = data;
	int rc;

	data_item = l_hashmap_lookup(thing.data_items,
				     L_INT_TO_PTR(*sensor_id));
	if (!data_item)
		return;

	rc = knot_cloud_publish_data(thing.id, data_item->sensor_id,
				     data_item->schema.value_type,
				     &data_item->current_val,
				     sizeof(data_item->schema.value_type));
	if (rc < 0)
		l_error("Couldn't send data_update for data_item #%d",
			*sensor_id);
}

static void foreach_publish_all_data(const void *key, void *value,
				     void *user_data)
{
	struct knot_data_item *data_item = value;

	on_publish_data(&data_item->sensor_id, NULL);
}

static void on_msg_timeout(struct l_timeout *timeout, void *user_data)
{
	sm_input_event(EVT_TIMEOUT, user_data);
}

static void on_event_timeout(int id)
{
	struct l_queue *list;

	list = l_queue_new();
	l_queue_push_head(list, &id);

	sm_input_event(EVT_PUB_DATA, list);

	l_queue_destroy(list, NULL);
}

static bool on_cloud_receive(const struct knot_cloud_msg *msg, void *user_data)
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
	case CONFIG_MSG:
		if (msg->error)
			sm_input_event(EVT_CFG_UPT_NOT_OK, NULL);
		else
			sm_input_event(EVT_CFG_UPT_OK, msg->list);
		break;
	case LIST_MSG:
	case MSG_TYPES_LENGTH:
	default:
		return true;
	}

	return true;
}

static void conn_handler(enum CONN_TYPE conn, bool is_up)
{
	static uint8_t conn_mask;

	conn_mask = set_conn_bitmask(is_up, conn_mask, conn);

	if (conn_mask != CONNECTED_MASK) {
		sm_input_event(EVT_NOT_READY, NULL);
		return;
	}

	sm_input_event(EVT_READY, NULL);
}

static void on_cloud_disconnected(void *user_data)
{
	l_info("Disconnected from Cloud");

	conn_handler(CLOUD, false);
}

static void on_cloud_connected(void *user_data)
{
	l_info("Connected to Cloud %s", thing.rabbitmq_url);

	conn_handler(CLOUD, true);
}

static void on_driver_disconnected(void *user_data)
{
	enum CONN_TYPE *type_aux = user_data;

	l_info("Disconnected");

	poll_stop();
	conn_handler(*type_aux, false);
}

static void on_driver_connected(void *user_data)
{
	enum CONN_TYPE *type_aux = user_data;

	l_info("Connected to %s", thing.geral_url);

	poll_start();
	conn_handler(*type_aux, true);
}

static int on_poll_receive(int id)
{
	struct knot_data_item *data_item;
	struct l_queue *list;
	int rc;

	data_item = l_hashmap_lookup(thing.data_items, L_INT_TO_PTR(id));
	if (!data_item)
		return -EINVAL;
#if DRIVER_MODBUS
	rc = iface_modbus_read_data(data_item->reg_addr,
				    data_item->value_type_size,
				    &data_item->current_val,
				    data_item->endianness_type_sensor);
#elif DRIVER_ETHERNET_IP
	rc = iface_ethernet_ip_read_data(
				    data_item->ethernet_ip_data_settings.tag,
				    data_item->reg_addr,
				    data_item->schema.value_type,
				    data_item->value_type_size,
				    &data_item->current_val);
#endif

	if (event_check_value(data_item->event,
			      data_item->current_val,
			      data_item->sent_val,
			      data_item->schema.value_type) > 0) {
		data_item->sent_val = data_item->current_val;
		list = l_queue_new();
		l_queue_push_head(list, &id);

		sm_input_event(EVT_PUB_DATA, list);

		l_queue_destroy(list, NULL);
	}

	return rc;
}

static void foreach_data_item_polling(const void *key, void *value,
				      void *user_data)
{
	struct knot_data_item *data_item = value;
	int *rc = user_data;

	if (poll_create(DEFAULT_POLLING_INTERVAL, data_item->sensor_id,
			on_poll_receive)) {
		l_error("Fail on create poll to read data item with id: %d",
			data_item->sensor_id);
		*rc = -1;
	}
}

static int create_data_item_polling(void)
{
	int rc = 0;

	l_hashmap_foreach(thing.data_items, foreach_data_item_polling,
			  &rc);
	if (rc)
		poll_destroy();

	return rc;
}

void device_set_log_priority(int priority)
{
	knot_cloud_set_log_priority(priority);
}

char *device_get_id(void)
{
	return thing.id;
}

void device_set_thing_name(struct knot_thing *thing, const char *name)
{
	strcpy(thing->name, name);
}

void device_set_thing_ethernet_ip_slave(struct knot_thing *thing,
					const char *type_plc)
{
	strcpy(thing->ethernet_ip_settings.plc_type, type_plc);
}

void device_set_thing_url(struct knot_thing *thing, const char *url)
{
	strcpy(thing->geral_url, url);
}

void device_set_thing_user_token(struct knot_thing *thing, char *token)
{
	thing->user_token = token;
}

void device_set_thing_modbus_slave(struct knot_thing *thing, int slave_id)
{
	thing->modbus_slave.id = slave_id;
}


void device_set_new_data_item(struct knot_thing *thing, int sensor_id,
			      knot_schema schema, knot_event event,
			      int reg_addr, int value_type_size,
			      int endianness_type, void *driver_buffer)
{
	struct knot_data_item *data_item_aux;
#ifdef DRIVER_ETHERNET_IP
	struct ethernet_ip_data_settings *ethernet_ip = (struct ethernet_ip_data_settings *) driver_buffer;
#endif /*DRIVER_ETHERNET_IP*/

	data_item_aux = l_new(struct knot_data_item, 1);
	data_item_aux->sensor_id = sensor_id;
	data_item_aux->schema = schema;
	data_item_aux->event = event;
	data_item_aux->reg_addr = reg_addr;
	data_item_aux->value_type_size = value_type_size;
	data_item_aux->endianness_type_sensor = endianness_type;
#ifdef DRIVER_ETHERNET_IP
	data_item_aux->ethernet_ip_data_settings.element_size = ethernet_ip->element_size;
	strcpy(data_item_aux->ethernet_ip_data_settings.path,
	       ethernet_ip->path);
	strcpy(data_item_aux->ethernet_ip_data_settings.tag_name,
	       ethernet_ip->tag_name);
#endif /*DRIVER_ETHERNET_IP*/

	l_hashmap_insert(thing->data_items,
			 L_INT_TO_PTR(data_item_aux->sensor_id),
			 data_item_aux);
}

void device_update_config_data_item(struct knot_thing *thing,
				    knot_msg_config *config)
{
	struct knot_data_item *data_item;

	data_item = l_hashmap_lookup(thing->data_items,
				     L_INT_TO_PTR(config->sensor_id));
	if (!data_item)
		return;

	data_item->schema.type_id = config->schema.type_id;
	data_item->schema.unit = config->schema.unit;
	data_item->schema.value_type = config->schema.value_type;
	strncpy(data_item->schema.name, config->schema.name,
		KNOT_PROTOCOL_DATA_NAME_LEN);

	if (!(config->event.event_flags & KNOT_EVT_FLAG_UNREGISTERED)) {
		data_item->event.event_flags = config->event.event_flags;
		data_item->event.time_sec = config->event.time_sec;
		knot_value_assign_limit(config->schema.value_type,
					config->event.lower_limit,
					&data_item->event.lower_limit);
		knot_value_assign_limit(config->schema.value_type,
					config->event.upper_limit,
					&data_item->event.upper_limit);
	}
}

void *device_data_item_lookup(struct knot_thing *thing, int sensor_id)
{
	return l_hashmap_lookup(thing->data_items, L_INT_TO_PTR(sensor_id));
}

void device_set_thing_rabbitmq_url(struct knot_thing *thing, char *url)
{
	thing->rabbitmq_url = url;
}

void device_set_thing_credentials(struct knot_thing *thing, const char *id,
				  const char *token)
{
	strncpy(thing->id, id, KNOT_PROTOCOL_UUID_LEN);
	strncpy(thing->token, token, KNOT_PROTOCOL_TOKEN_LEN);
}

void device_generate_thing_id(void)
{
	uint64_t id; /* knot id uses 16 characters which fits inside a uint64 */

	l_getrandom(&id, sizeof(id));
	sprintf(thing.id, "%"PRIx64, id); /* PRIx64 formats the string as hex */
}

void device_clear_thing_id(struct knot_thing *thing)
{
	thing->id[0] = '\0';
}

void device_clear_thing_token(struct knot_thing *thing)
{
	thing->token[0] = '\0';
}

int device_has_thing_token(void)
{
	return thing.token[0] != '\0';
}

int device_store_credentials_on_file(char *token)
{
	int rc;

	rc = properties_store_credentials(&thing,
					  thing.conf_files.credentials_path,
					  thing.id, token);
	if (rc < 0)
		return -1;

	strncpy(thing.token, token, KNOT_PROTOCOL_TOKEN_LEN);

	return 0;
}

int device_clear_credentials_on_file(void)
{
	return properties_clear_credentials(&thing,
					    thing.conf_files.credentials_path);
}

int device_start_event(void)
{
	int rc;

	rc = event_start(on_event_timeout);
	if (rc < 0) {
		l_error("Failed to start event");
		return rc;
	}

	l_hashmap_foreach(thing.data_items, foreach_event_add_data_item, NULL);

	return 0;
}

void device_stop_event(void)
{
	event_stop();
}

int device_update_config(struct l_queue *config_list)
{
	device_stop_event();

	l_queue_foreach(config_list, foreach_update_config, NULL);

	device_start_event();

	return 0;
}

int device_check_schema_change(void)
{
	/* TODO: Add schema change verification */
	return 1;
}

int device_send_register_request(void)
{
	return knot_cloud_register_device(thing.id, thing.name);
}

int device_send_auth_request(void)
{
	return knot_cloud_auth_device(thing.id, thing.token);
}

int device_send_config(void)
{
	struct l_queue *config_queue;
	int rc;

	config_queue = l_queue_new();

	l_hashmap_foreach(thing.data_items, foreach_send_config, config_queue);

	rc = knot_cloud_update_config(thing.id, config_queue);

	l_queue_destroy(config_queue, l_free);

	return rc;
}

void device_publish_data_list(struct l_queue *sensor_id_list)
{
	l_queue_foreach(sensor_id_list, on_publish_data, NULL);
}

void device_publish_data_all(void)
{
	l_hashmap_foreach(thing.data_items, foreach_publish_all_data, NULL);
}

void device_msg_timeout_create(int seconds)
{
	if (thing.msg_to)
		return;

	thing.msg_to = l_timeout_create(seconds, on_msg_timeout, NULL, NULL);
}

void device_msg_timeout_modify(int seconds)
{
	l_timeout_modify(thing.msg_to, seconds);
}

void device_msg_timeout_remove(void)
{
	l_timeout_remove(thing.msg_to);
	thing.msg_to = NULL;
}

int device_start_read_cloud(void)
{
	return knot_cloud_read_start(thing.id, on_cloud_receive, NULL);
}

int device_start(struct device_settings *conf_files)
{
	int err;

	thing.data_items = l_hashmap_new();
	if (properties_create_device(&thing, conf_files)) {
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

#if DRIVER_MODBUS
	err = iface_modbus_start(thing.geral_url, thing.modbus_slave.id,
				 on_driver_connected, on_driver_disconnected,
				 NULL);
#elif DRIVER_ETHERNET_IP
	err = iface_ethernet_ip_start(thing,
				 on_driver_connected, on_driver_disconnected,
				 NULL);
#endif

	if (err < 0) {
		l_error("Failed to initialize Driver");
		poll_destroy();
		knot_thing_destroy(&thing);
		return err;
	}

	err = knot_cloud_start(thing.rabbitmq_url, thing.user_token,
			       on_cloud_connected, on_cloud_disconnected, NULL);
	if (err < 0) {
		l_error("Failed to initialize Cloud");
		poll_destroy();
		iface_modbus_stop();
		knot_thing_destroy(&thing);
		return err;
	}

	l_info("Device \"%s\" has started successfully", thing.name);

	thing.conf_files.credentials_path =
					l_strdup(conf_files->credentials_path);
	thing.conf_files.device_path = l_strdup(conf_files->device_path);
	thing.conf_files.cloud_path = l_strdup(conf_files->cloud_path);

	return 0;
}

void device_destroy(void)
{
	event_stop();

	poll_destroy();
	knot_cloud_stop();
	iface_modbus_stop();

	knot_thing_destroy(&thing);
}
