/*
 * This file is part of the KNOT Project
 *
 * Copyright (c) 2019, CESAR. All rights reserved.
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
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <ell/ell.h>
#include <json-c/json.h>
#include <amqp.h>

#include <knot/knot_protocol.h>

#include "mq.h"
#include "parser.h"
#include "cloud.h"

#define MQ_QUEUE_FOG_OUT "thingd-fogOut"
#define MQ_QUEUE_REPLY "thingd-reply"

/* Exchanges */
#define MQ_EXCHANGE_DEVICE "device"
#define MQ_EXCHANGE_DATA_SENT "data.sent"

/* Headers */
#define MQ_AUTHORIZATION_HEADER "Authorization"

#define MQ_MSG_EXPIRATION_TIME_MS 2000

 /* Southbound traffic (commands) */
#define MQ_EVENT_PREFIX_DEVICE "device"
#define MQ_EVENT_POSTFIX_DATA_UPDATE "data.update"
#define MQ_EVENT_POSTFIX_DATA_REQUEST "data.request"

#define MQ_EVENT_DEVICE_REGISTERED "device.registered"
#define MQ_EVENT_DEVICE_UNREGISTERED "device.unregistered"
#define MQ_EVENT_DEVICE_SCHEMA_UPDATED "device.schema.updated"

 /* Northbound traffic (control, measurements) */
#define MQ_CMD_DEVICE_REGISTER "device.register"
#define MQ_CMD_DEVICE_UNREGISTER "device.unregister"
#define MQ_CMD_DEVICE_AUTH "device.auth"
#define MQ_CMD_SCHEMA_SENT "device.schema.sent"

cloud_cb_t cloud_cb;
amqp_bytes_t queue_reply;
amqp_bytes_t queue_fog;
char *user_auth_token;
char *cloud_events[MSG_TYPES_LENGTH];
amqp_table_entry_t headers[1];

static void cloud_msg_destroy(struct cloud_msg *msg)
{
	if (msg->type == UPDATE_MSG || msg->type == REQUEST_MSG)
		l_queue_destroy(msg->list, l_free);

	l_free(msg);
}

static int map_routing_key_to_msg_type(const char *routing_key)
{
	int msg_type;

	for (msg_type = UPDATE_MSG; msg_type < MSG_TYPES_LENGTH; msg_type++) {
		if (!strcmp(routing_key, cloud_events[msg_type]))
			return msg_type;
	}

	return -1;
}

static struct cloud_msg *create_msg(const char *routing_key, json_object *jso)
{
	struct cloud_msg *msg = l_new(struct cloud_msg, 1);

	msg->type = map_routing_key_to_msg_type(routing_key);

	switch (msg->type) {
	case UPDATE_MSG:
		msg->error = NULL;
		msg->device_id = parser_get_key_str_from_json_obj(jso, "id");
		if (!msg->device_id) {
			l_error("Malformed JSON message");
			goto err;
		}

		msg->list = parser_update_to_list(jso);
		if (!msg->list) {
			l_error("Malformed JSON message");
			goto err;
		}

		break;
	case REQUEST_MSG:
		msg->error = NULL;
		msg->device_id = parser_get_key_str_from_json_obj(jso, "id");
		if (!msg->device_id) {
			l_error("Malformed JSON message");
			goto err;
		}

		msg->list = parser_request_to_list(jso);
		if (!msg->list) {
			l_error("Malformed JSON message");
			goto err;
		}

		break;
	case REGISTER_MSG:
		msg->device_id = parser_get_key_str_from_json_obj(jso, "id");
		if (!msg->device_id ||
				!parser_is_key_str_or_null(jso, "error")) {
			l_error("Malformed JSON message");
			goto err;
		}

		msg->token = parser_get_key_str_from_json_obj(jso, "token");
		if (!msg->token) {
			l_error("Malformed JSON message");
			goto err;
		}

		msg->error = parser_get_key_str_from_json_obj(jso, "error");
		break;
	case UNREGISTER_MSG:
		msg->device_id = parser_get_key_str_from_json_obj(jso, "id");
		if (!msg->device_id ||
				!parser_is_key_str_or_null(jso, "error")) {
			l_error("Malformed JSON message");
			goto err;
		}

		msg->error = parser_get_key_str_from_json_obj(jso, "error");
		break;
	case AUTH_MSG:
		msg->device_id = parser_get_key_str_from_json_obj(jso, "id");
		if (!msg->device_id ||
				!parser_is_key_str_or_null(jso, "error")) {
			l_error("Malformed JSON message");
			goto err;
		}

		msg->error = parser_get_key_str_from_json_obj(jso, "error");
		break;
	case SCHEMA_MSG:
		msg->device_id = parser_get_key_str_from_json_obj(jso, "id");
		if (!msg->device_id ||
				!parser_is_key_str_or_null(jso, "error")) {
			l_error("Malformed JSON message");
			goto err;
		}

		msg->error = parser_get_key_str_from_json_obj(jso, "error");
		break;
	case MSG_TYPES_LENGTH:
	default:
		l_error("Unknown event %s", routing_key);
		goto err;
	}

	return msg;
err:
	cloud_msg_destroy(msg);
	return NULL;
}

/**
 * Callback function to consume and parse the received message from AMQP queue
 * and call the respective handling callback function. In case of a error on
 * parse, the message is consumed, but not used.
 *
 * Returns true if the message envelope was consumed or returns false otherwise.
 */
static bool on_cloud_receive_message(const char *exchange,
				     const char *routing_key,
				     const char *body, void *user_data)
{
	struct cloud_msg *msg;
	bool consumed = true;
	json_object *jso;

	jso = json_tokener_parse(body);
	if (!jso) {
		l_error("Error on parse JSON object");
		return false;
	}

	msg = create_msg(routing_key, jso);
	if (msg) {
		consumed = cloud_cb(msg, user_data);
		cloud_msg_destroy(msg);
	}

	json_object_put(jso);

	return consumed;
}
static int create_fog_queue(const char *id)
{
	char queue_fog_name[100];
	int msg_type;
	int err;

	snprintf(queue_fog_name, sizeof(queue_fog_name), "%s-%s",
		 MQ_QUEUE_FOG_OUT, id);

	queue_fog = mq_declare_new_queue(queue_fog_name);
	if (queue_fog.bytes == NULL) {
		l_error("Error on declare a new queue");
		return -1;
	}

	for (msg_type = UPDATE_MSG; msg_type < MSG_TYPES_LENGTH; msg_type++) {
		err = mq_prepare_direct_queue(queue_fog, MQ_EXCHANGE_DEVICE,
					      cloud_events[msg_type]);
		if (err) {
			l_error("Error on set up queue to consume");
			return -1;
		}
	}

	err = mq_consumer_queue(queue_fog);
	if (err) {
		l_error("Error on start a queue consumer");
		return -1;
	}

	return 0;
}

static int create_reply_queue(const char *id)
{
	char queue_reply_name[100];

	snprintf(queue_reply_name, sizeof(queue_reply_name), "%s-%s",
		 MQ_QUEUE_REPLY, id);

	queue_reply = mq_declare_new_queue(queue_reply_name);
	if (queue_reply.bytes == NULL) {
		l_error("Error on declare a new queue");
		return -1;
	}

	if (mq_consumer_queue(queue_reply)) {
		l_error("Error on start a queue consumer");
		return -1;
	}

	return 0;
}

static void destroy_cloud_queues(void)
{
	if (queue_reply.bytes) {
		if (mq_delete_queue(queue_reply))
			l_error("Error when delete Reply Queue");

		amqp_bytes_free(queue_reply);
		queue_reply = amqp_empty_bytes;
	}

	if (queue_fog.bytes) {
		if (mq_delete_queue(queue_fog))
			l_error("Error when delete Fog Queue");

		amqp_bytes_free(queue_fog);
		queue_fog = amqp_empty_bytes;
	}
}

static void destroy_cloud_events(void)
{
	int msg_type;

	for (msg_type = UPDATE_MSG; msg_type < MSG_TYPES_LENGTH; msg_type++) {
		if (cloud_events[msg_type] != NULL) {
			l_free(cloud_events[msg_type]);
			cloud_events[msg_type] = NULL;
		}
	}
}

static int set_cloud_events(const char *id)
{
	char binding_key_reply[100];
	char binding_key_update[100];
	char binding_key_request[100];

	snprintf(binding_key_reply, sizeof(binding_key_reply), "%s-%s",
		 MQ_QUEUE_REPLY, id);

	snprintf(binding_key_update, sizeof(binding_key_update), "%s.%s.%s",
		 MQ_EVENT_PREFIX_DEVICE, id, MQ_EVENT_POSTFIX_DATA_UPDATE);

	snprintf(binding_key_request, sizeof(binding_key_request), "%s.%s.%s",
		 MQ_EVENT_PREFIX_DEVICE, id, MQ_EVENT_POSTFIX_DATA_REQUEST);

	/* Free cloud_events if already allocated */
	destroy_cloud_events();

	cloud_events[UPDATE_MSG] = l_strdup(binding_key_update);
	cloud_events[REQUEST_MSG] = l_strdup(binding_key_request);
	cloud_events[REGISTER_MSG] = l_strdup(MQ_EVENT_DEVICE_REGISTERED);
	cloud_events[UNREGISTER_MSG] = l_strdup(MQ_EVENT_DEVICE_UNREGISTERED);
	cloud_events[AUTH_MSG] = l_strdup(binding_key_reply);
	cloud_events[SCHEMA_MSG] = l_strdup(MQ_EVENT_DEVICE_SCHEMA_UPDATED);

	return 0;
}

/**
 * cloud_register_device:
 * @id: device id
 * @name: device name
 *
 * Requests cloud to add a device.
 * The confirmation that the cloud received the message comes from a callback
 * set in function cloud_read_start with message type REGISTER_MSG.
 *
 * Returns: 0 if successful and a KNoT error otherwise.
 */
int cloud_register_device(const char *id, const char *name)
{
	json_object *jobj_device;
	const char *json_str;
	int result;

	jobj_device = parser_device_json_create(id, name);
	if (!jobj_device)
		return KNOT_ERR_CLOUD_FAILURE;

	json_str = json_object_to_json_string(jobj_device);

	headers[0].value.value.bytes = amqp_cstring_bytes(user_auth_token);

	/**
	 * Exchange
	 *	Type: Direct
	 *	Name: device
	 * Routing Key
	 *	Name: device.register
	 * Headers
	 *	[0]: User Token
	 * Expiration
	 *	2000 ms
	 */
	result = mq_publish_direct_message(MQ_EXCHANGE_DEVICE,
					   MQ_CMD_DEVICE_REGISTER,
					   headers, 1,
					   MQ_MSG_EXPIRATION_TIME_MS,
					   json_str);
	if (result < 0)
		result = KNOT_ERR_CLOUD_FAILURE;

	json_object_put(jobj_device);

	return result;
}

/**
 * cloud_unregister_device:
 * @id: device id
 *
 * Requests cloud to remove a device.
 * The confirmation that the cloud received the message comes from a callback
 * set in function cloud_read_start  with message type UNREGISTER_MSG.
 *
 * Returns: 0 if successful and a KNoT error otherwise.
 */
int cloud_unregister_device(const char *id)
{
	json_object *jobj_unreg;
	const char *json_str;
	int result;

	jobj_unreg = parser_unregister_json_create(id);
	if (!jobj_unreg)
		return KNOT_ERR_CLOUD_FAILURE;

	json_str = json_object_to_json_string(jobj_unreg);

	headers[0].value.value.bytes = amqp_cstring_bytes(user_auth_token);

	/**
	 * Exchange
	 *	Type: Direct
	 *	Name: device
	 * Routing Key
	 *	Name: device.unregister
	 * Headers
	 *	[0]: User Token
	 * Expiration
	 *	2000 ms
	 */
	result = mq_publish_direct_message(MQ_EXCHANGE_DEVICE,
					   MQ_CMD_DEVICE_UNREGISTER,
					   headers, 1,
					   MQ_MSG_EXPIRATION_TIME_MS,
					   json_str);
	if (result < 0)
		return KNOT_ERR_CLOUD_FAILURE;

	json_object_put(jobj_unreg);

	return 0;
}

/**
 * cloud_auth_device:
 * @id: device id
 * @token: device token
 *
 * Requests cloud to auth a device.
 * The confirmation that the cloud received the message comes from a callback
 * set in function cloud_read_start.
 *
 * Returns: 0 if successful and a KNoT error otherwise.
 */
int cloud_auth_device(const char *id, const char *token)
{
	json_object *jobj_auth;
	const char *json_str;
	int result;

	if (!queue_reply.bytes) {
		l_error("Reply queue not declared");
		return KNOT_ERR_CLOUD_FAILURE;
	}

	jobj_auth = parser_auth_json_create(id, token);
	if (!jobj_auth)
		return KNOT_ERR_CLOUD_FAILURE;

	json_str = json_object_to_json_string(jobj_auth);

	headers[0].value.value.bytes = amqp_cstring_bytes(user_auth_token);

	/**
	 * Exchange
	 *	Type: Direct
	 *	Name: device
	 * Routing Key
	 *	Name: device.auth
	 * Headers
	 *	[0]: User Token
	 * Expiration
	 *	2000 ms
	 */
	result = mq_publish_direct_message_rpc(MQ_EXCHANGE_DEVICE,
					       MQ_CMD_DEVICE_AUTH,
					       headers, 1,
					       MQ_MSG_EXPIRATION_TIME_MS,
					       queue_reply, id,
					       json_str);
	if (result < 0)
		result = KNOT_ERR_CLOUD_FAILURE;

	json_object_put(jobj_auth);

	return result;
}

/**
 * cloud_update_schema:
 *
 * Requests cloud to update the device schema.
 * The confirmation that the cloud received the message comes from a callback
 * set in function cloud_read_start with message type SCHEMA_MSG.
 *
 * Returns: 0 if successful and a KNoT error otherwise.
 */
int cloud_update_schema(const char *id, struct l_queue *schema_list)
{
	json_object *jobj_schema;
	const char *json_str;
	int result;

	jobj_schema = parser_schema_create_object(id, schema_list);
	if (!jobj_schema)
		return KNOT_ERR_CLOUD_FAILURE;

	json_str = json_object_to_json_string(jobj_schema);

	headers[0].value.value.bytes = amqp_cstring_bytes(user_auth_token);

	/**
	 * Exchange
	 *	Type: Direct
	 *	Name: device
	 * Routing Key
	 *	Name: device.schema.sent
	 * Headers
	 *	[0]: User Token
	 * Expiration
	 *	2000 ms
	 */
	result = mq_publish_direct_message(MQ_EXCHANGE_DEVICE,
					   MQ_CMD_SCHEMA_SENT,
					   headers, 1,
					   MQ_MSG_EXPIRATION_TIME_MS,
					   json_str);
	if (result < 0)
		result = KNOT_ERR_CLOUD_FAILURE;

	json_object_put(jobj_schema);

	return result;
}

/**
 * cloud_publish_data:
 * @id: device id
 * @sensor_id: schema sensor id
 * @value_type: schema value type defined in KNoT protocol
 * @value: value to be sent
 * @kval_len: length of @value
 *
 * Sends device's data to cloud.
 *
 * Returns: 0 if successful and a KNoT error otherwise.
 */
int cloud_publish_data(const char *id, uint8_t sensor_id, uint8_t value_type,
		       const knot_value_type *value,
		       uint8_t kval_len)
{
	json_object *jobj_data;
	const char *json_str;
	int result;

	jobj_data = parser_data_create_object(id, sensor_id, value_type, value,
					      kval_len);
	if (!jobj_data)
		return KNOT_ERR_CLOUD_FAILURE;

	json_str = json_object_to_json_string(jobj_data);

	headers[0].value.value.bytes = amqp_cstring_bytes(user_auth_token);

	/**
	 * Exchange
	 *	Type: Fanout
	 *	Name: data.sent
	 * Headers
	 *	[0]: User Token
	 * Expiration
	 *	2000 ms
	 */
	result = mq_publish_fanout_message(MQ_EXCHANGE_DATA_SENT,
					   headers, 1,
					   MQ_MSG_EXPIRATION_TIME_MS,
					   json_str);
	if (result < 0)
		result = KNOT_ERR_CLOUD_FAILURE;

	json_object_put(jobj_data);

	return result;
}

/**
 * cloud_read_start:
 * @id: thing id
 * @read_handler_cb: callback to handle message received from cloud
 * @user_data: user data provided to callbacks
 *
 * Start Cloud to receive messages on read_handler_cb function.
 *
 * Returns: 0 if successful and -1 otherwise.
 */
int cloud_read_start(const char *id, cloud_cb_t read_handler_cb,
		     void *user_data)
{
	cloud_cb = read_handler_cb;

	/* Delete queues if already declared */
	destroy_cloud_queues();

	if (set_cloud_events(id))
		return -1;

	if (create_fog_queue(id))
		return -1;

	if (create_reply_queue(id))
		return -1;

	if (mq_set_read_cb(on_cloud_receive_message, user_data)) {
		l_error("Error on set up read callback");
		return -1;
	}

	return 0;
}

int cloud_start(char *url, char *user_token, cloud_connected_cb_t connected_cb,
		cloud_disconnected_cb_t disconnected_cb, void *user_data)
{
	user_auth_token = l_strdup(user_token);
	headers[0].key = amqp_cstring_bytes(MQ_AUTHORIZATION_HEADER);
	headers[0].value.kind = AMQP_FIELD_KIND_UTF8;
	headers[0].value.value.bytes = amqp_cstring_bytes(user_token);

	return mq_start(url, connected_cb, disconnected_cb, user_data);
}

void cloud_stop(void)
{
	destroy_cloud_queues();

	destroy_cloud_events();
	mq_stop();
}
