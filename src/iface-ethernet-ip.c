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

#include <stdio.h>
#include <stdlib.h>
#include <libplctag.h>
#include <ell/ell.h>
#include <errno.h>
#include <knot/knot_types.h>
#include <knot/knot_protocol.h>

#include "iface-ethernet-ip.h"
#include "conf-driver.h"

static iface_ethernet_ip_connected_cb_t conn_cb;
static iface_ethernet_ip_disconnected_cb_t disconn_cb;

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
#define DATA_TIMEOUT 500

static void parse_tag_data_item(struct knot_data_item *data_item)
{
	struct knot_data_item data_item_aux;

	snprintf(data_item_aux.ethernet_ip_data_settings.string_tag_path,
		 512, ETHERNET_IP_TEMPLATE_MODEL,
		 thing_ethernet_ip.geral_url,
		 data_item->ethernet_ip_data_settings.path,
		 thing_ethernet_ip.ethernet_ip_settings.plc_type,
		 data_item->ethernet_ip_data_settings.element_size,
		 data_item->ethernet_ip_data_settings.tag_name);
	printf("%s\n\r",
	       data_item_aux.ethernet_ip_data_settings.string_tag_path);

	*data_item = data_item_aux;
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
			data_item_aux->ethernet_ip_data_settings.tag_name,
			data_item->ethernet_ip_data_settings.tag_name);
		if (!return_aux) {
			data_item->ethernet_ip_data_settings.tag =
				data_item_aux->ethernet_ip_data_settings.tag;
			rc = 0;
			break;
		}
	}

	return rc;
}

static int connect_ethernet_ip(struct knot_data_item *data_item)
{
	int rc = 0;

	data_item->ethernet_ip_data_settings.tag = plc_tag_create(
			data_item->ethernet_ip_data_settings.string_tag_path,
			DATA_TIMEOUT);

	    /* everything OK? */
	if (data_item->ethernet_ip_data_settings.tag < 0) {
		fprintf(stderr, "ERROR %s: Could not create tag %s!\n",
			plc_tag_decode_error(
				data_item->ethernet_ip_data_settings.tag),
				data_item->ethernet_ip_data_settings.tag_name);
		return -EINVAL;
	}

	rc = plc_tag_status(data_item->ethernet_ip_data_settings.tag);
	if (rc != PLCTAG_STATUS_OK) {
		fprintf(stderr, "Error setting up tag internal state. %s\n",
			plc_tag_decode_error(
				data_item->ethernet_ip_data_settings.tag));
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
				data_item->ethernet_ip_data_settings.tag);
			*rc = -EINVAL;
		}
	}
}

int iface_ethernet_ip_read_data(int tag, int reg_addr, uint8_t value_type,
				int value_type_size,
				knot_value_type *out)
{
	int rc;
	union ethernet_ip_types tmp;

	memset(&tmp, 0, sizeof(tmp));

	rc = plc_tag_read(tag, 100);

	if (rc == PLCTAG_STATUS_OK) {
		switch (value_type) {
		//TODO: Update knot_cloud to accept int16
		case KNOT_VALUE_TYPE_BOOL:
			tmp.val_bool = plc_tag_get_uint8(tag, reg_addr);
			break;
		case KNOT_VALUE_TYPE_INT:
			if (value_type_size == 8)
				tmp.val_i8 = plc_tag_get_int8(tag, reg_addr);
			else if (value_type_size == 16)
				tmp.val_i16 = plc_tag_get_int16(tag, reg_addr);
			else if (value_type_size == 32)
				tmp.val_i32 = plc_tag_get_int32(tag, reg_addr);
			else
				rc = 1;
			break;
		case KNOT_VALUE_TYPE_UINT:
			if (value_type_size == 8)
				tmp.val_u8 = plc_tag_get_uint8(tag, reg_addr);
			if (value_type_size == 16)
				tmp.val_u16 = plc_tag_get_uint16(tag, reg_addr);
			else if (value_type_size == 32)
				tmp.val_u32 = plc_tag_get_uint32(tag, reg_addr);
			else
				rc = 1;
			break;
		case KNOT_VALUE_TYPE_INT64:
			tmp.val_i64 = plc_tag_get_int64(tag, reg_addr);
			break;
		case KNOT_VALUE_TYPE_UINT64:
			tmp.val_u64 = plc_tag_get_uint64(tag, reg_addr);
			break;
		default:
			rc = -EINVAL;
		}

		memcpy(out, &tmp, sizeof(tmp));
	} else {
		l_error("Unable to read the data! Got error code %d: %s\n",
			rc, plc_tag_decode_error(rc));
	}

	return rc;
}

int iface_ethernet_ip_start(struct knot_thing thing,
		       iface_ethernet_ip_connected_cb_t connected_cb,
		       iface_ethernet_ip_disconnected_cb_t disconnected_cb,
		       void *user_data)
{
	int rc = 0;

	thing_ethernet_ip = thing;

	if (plc_tag_check_lib_version(REQUIRED_VERSION_MAJOR,
				      REQUIRED_VERSION_MINOR,
				      REQUIRED_VERSION_PATCH)
				      != PLCTAG_STATUS_OK) {
		fprintf(stderr,
			"Required compatible library version %d.%d.%d not available!",
			REQUIRED_VERSION_MAJOR, REQUIRED_VERSION_MINOR,
			REQUIRED_VERSION_PATCH);
		return -EPERM;
	}

	l_hashmap_foreach(thing.data_items, foreach_data_item_ethernet_ip,
			&rc);
	if (rc)
		return -EINVAL;

	conn_cb = connected_cb;
	disconn_cb = disconnected_cb;

	//TODO: Verify connection of ethernet/ip
	//connect_to = l_timeout_create_ms(1, attempt_connect, NULL, NULL);

	return 0;
}
