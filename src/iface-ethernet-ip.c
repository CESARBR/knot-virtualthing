/**
 * This file is part of the KNOT Project
 *
 * Copyright (c) 2021, CESAR. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <libplctag.h>
#include <ell/ell.h>
#include <errno.h>
#include <knot/knot_types.h>
#include <knot/knot_protocol.h>

#include "conf-device.h"
#include "iface-ethernet-ip.h"

#define RECONNECT_TIMEOUT 5

static iface_ethernet_ip_connected_cb_t conn_cb;
static iface_ethernet_ip_disconnected_cb_t disconn_cb;
static struct l_timeout *connect_to;
struct knot_thing thing_ethernet_ip;

union ethernet_ip_types {
	float val_float;
	uint8_t val_bool;
	int8_t val_i8;
	uint8_t val_u8;
	int16_t val_i16;
	uint16_t val_u16;
	int32_t val_i32;
	uint32_t val_u32;
	int64_t val_i64;
	uint64_t val_u64;
};

#define REQUIRED_VERSION_MAJOR 2
#define REQUIRED_VERSION_MINOR 1
#define REQUIRED_VERSION_PATCH 4
#define ETHERNET_IP_TEMPLATE_MODEL "protocol=ab-eip&gateway=%s&path=%s&cpu=%s&elem_count=%d&name=%s"
#define DATA_TIMEOUT 1000

static void parse_tag_data_item(struct knot_data_item *data_item)
{
	char string_tag_path_aux[DRIVER_MAX_TYPE_STRING_CONNECT_LEN];

	snprintf(string_tag_path_aux,
		 512, ETHERNET_IP_TEMPLATE_MODEL,
		 thing_ethernet_ip.geral_url,
		 data_item->path,
		 thing_ethernet_ip.name_type,
		 data_item->element_size,
		 data_item->tag_name);
	printf("%s\n\r",
	       string_tag_path_aux);

	strcpy(data_item->string_tag_path,
	       (const char *)string_tag_path_aux);
}

static int verify_tag_name_created(struct knot_data_item *data_item)
{
	struct knot_data_item *data_item_aux;
	int i;
	int rc = -1;

	for (i = 0; i < data_item->sensor_id; i++) {
		int return_aux;

		data_item_aux = l_hashmap_lookup(thing_ethernet_ip.data_items,
						 L_INT_TO_PTR(i));
		if (!data_item_aux)
			rc = -EINVAL;

		return_aux = strcmp(
			data_item_aux->tag_name,
			data_item->tag_name);
		if (!return_aux) {
			data_item->tag =
				data_item_aux->tag;
			strcpy(data_item->string_tag_path,
			       (const char *)data_item_aux->string_tag_path);
			rc = 0;
			break;
		}
	}

	return rc;
}

static int put_tag_name_created(struct knot_data_item *data_item)
{
	struct knot_data_item *data_item_aux;
	int i;
	int rc = -1;

	for (i = 0; i < thing_ethernet_ip.number_sensor; i++) {
		int return_aux;

		data_item_aux = l_hashmap_lookup(thing_ethernet_ip.data_items,
						 L_INT_TO_PTR(i));
		if (!data_item_aux) {
			rc = -EINVAL;
			break;
		}

		if (!strcmp(data_item_aux->tag_name, data_item->tag_name)) {
			data_item_aux->tag = data_item->tag;
			rc = 0;
			break;
		}
	}

	return rc;
}

static int connect_ethernet_ip(struct knot_data_item *data_item)
{
	int rc = 0;

	data_item->tag = plc_tag_create(
			data_item->string_tag_path,
			DATA_TIMEOUT);

	if (data_item->tag < 0) {
		l_error("%s: Could not create tag %s!",
			plc_tag_decode_error(
				data_item->tag),
				data_item->tag_name);
		return -EINVAL;
	}

	rc = plc_tag_status(data_item->tag);
	if (rc != PLCTAG_STATUS_OK) {
		l_error("Error setting up tag internal state. %s",
			plc_tag_decode_error(
				data_item->tag));
		return -EINVAL;
	}

	return rc;
}

static void foreach_data_item_ethernet_ip(const void *key, void *value,
				      void *user_data)
{
	struct knot_data_item *data_item = value;
	int *rc = user_data;
	int return_aux = 0;

	return_aux = verify_tag_name_created(data_item);
	if (return_aux) {
		parse_tag_data_item(data_item);
		return_aux = connect_ethernet_ip(data_item);
		if (return_aux) {
			plc_tag_destroy(
				data_item->tag);
			*rc = -EINVAL;
		}
	}
}

static void foreach_stop_ethernet_ip(const void *key, void *value,
				      void *user_data)
{
	struct knot_data_item *data_item = value;
	int *rc = user_data;

	*rc = plc_tag_status(data_item->tag);
	if (rc != PLCTAG_STATUS_OK) {
		l_error("Error setting up tag internal state. %s\n",
			plc_tag_decode_error(*rc));
		plc_tag_destroy(data_item->tag);
	}
}

static void on_disconnected(void *user_data)
{
	if (disconn_cb)
		disconn_cb(user_data);

	if (connect_to)
		l_timeout_modify(connect_to, RECONNECT_TIMEOUT);
}

static void attempt_connect(struct l_timeout *to, void *user_data)
{
	int rc = 0;

	l_debug("Trying to connect to Ethernet/Ip");

	l_hashmap_foreach(thing_ethernet_ip.data_items,
			  foreach_data_item_ethernet_ip, &rc);
	if (rc) {
		l_error("error connecting to Ethernet/Ip");
		goto retry;
	}

	if (conn_cb)
		conn_cb(user_data);

	return;

retry:
	on_disconnected(NULL);
	l_timeout_modify(to, RECONNECT_TIMEOUT);
}

int iface_ethernet_ip_read_data(struct knot_data_item *data_item)
{
	int rc;
	union ethernet_ip_types tmp;
	int elem_size = 0;

	memset(&tmp, 0, sizeof(tmp));

	rc = plc_tag_read(data_item->tag, DATA_TIMEOUT);

	if (rc == PLCTAG_STATUS_OK) {
		elem_size = plc_tag_get_int_attribute(data_item->tag,
						      "elem_size", 0);
		switch (data_item->schema.value_type) {
		//TODO: Update knot_cloud to accept int16
		case KNOT_VALUE_TYPE_BOOL:
			if (data_item->value_type_size == 1)
				tmp.val_bool = (uint8_t)
					plc_tag_get_bit(data_item->tag, 0);
			else if (data_item->value_type_size == 8)
				tmp.val_bool =
					plc_tag_get_uint8(data_item->tag,
					    data_item->reg_addr * elem_size);
			else
				rc = 1;
			break;
		case KNOT_VALUE_TYPE_FLOAT:
			tmp.val_float = plc_tag_get_float32(data_item->tag,
					    data_item->reg_addr * elem_size);
			break;
		case KNOT_VALUE_TYPE_INT:
			if (data_item->value_type_size == 8)
				tmp.val_i8 =
					plc_tag_get_int8(data_item->tag,
					    data_item->reg_addr * elem_size);
			else if (data_item->value_type_size == 16)
				tmp.val_i16 =
					plc_tag_get_int16(data_item->tag,
					    data_item->reg_addr * elem_size);
			else if (data_item->value_type_size == 32)
				tmp.val_i32 =
					plc_tag_get_int32(data_item->tag,
					    data_item->reg_addr * elem_size);
			else
				rc = 1;
			break;
		case KNOT_VALUE_TYPE_UINT:
			if (data_item->value_type_size == 8)
				tmp.val_u8 =
					plc_tag_get_uint8(data_item->tag,
					    data_item->reg_addr * elem_size);
			if (data_item->value_type_size == 16)
				tmp.val_u16 =
					plc_tag_get_uint16(data_item->tag,
					    data_item->reg_addr * elem_size);
			else if (data_item->value_type_size == 32)
				tmp.val_u32 =
					plc_tag_get_uint32(data_item->tag,
					    data_item->reg_addr * elem_size);
			else
				rc = 1;
			break;
		case KNOT_VALUE_TYPE_INT64:
			tmp.val_i64 = plc_tag_get_int64(data_item->tag,
					data_item->reg_addr * elem_size);
			break;
		case KNOT_VALUE_TYPE_UINT64:
			tmp.val_u64 = plc_tag_get_uint64(data_item->tag,
					data_item->reg_addr * elem_size);
			break;
		default:
			rc = -EINVAL;
		}

		memcpy(&data_item->current_val, &tmp, sizeof(tmp));
	} else {
		l_error("Unable to read the data! Got error code %d: %s\n",
			rc, plc_tag_decode_error(rc));
		plc_tag_destroy(data_item->tag);
		data_item->tag = plc_tag_create(data_item->string_tag_path,
						DATA_TIMEOUT);
		put_tag_name_created(data_item);

	}

	return rc;
}

int iface_ethernet_ip_start(struct knot_thing thing,
		       iface_ethernet_ip_connected_cb_t connected_cb,
		       iface_ethernet_ip_disconnected_cb_t disconnected_cb,
		       void *user_data)
{
	thing_ethernet_ip = thing;

	if (plc_tag_check_lib_version(REQUIRED_VERSION_MAJOR,
				      REQUIRED_VERSION_MINOR,
				      REQUIRED_VERSION_PATCH)
				      != PLCTAG_STATUS_OK) {
		l_debug("Required compatible library version %d.%d.%d not available!",
			REQUIRED_VERSION_MAJOR, REQUIRED_VERSION_MINOR,
			REQUIRED_VERSION_PATCH);
		return -EPERM;
	}

	conn_cb = connected_cb;
	disconn_cb = disconnected_cb;

	connect_to = l_timeout_create_ms(1, attempt_connect,
					 NULL, NULL);

	return 0;
}

void iface_ethernet_ip_stop(void)
{
	int rc = 0;

	l_hashmap_foreach(thing_ethernet_ip.data_items,
			  foreach_stop_ethernet_ip, &rc);
	if (rc)
		l_error("error disconnect to Ethernet/Ip");

}
