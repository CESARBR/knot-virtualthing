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

#define DRIVER_MAX_PROTOCOL_TYPE_LEN	25
#define DRIVER_MAX_NAME_TYPE_LEN	25
#define DRIVER_MAX_URL_LEN		50

#define DRIVER_MAX_TYPE_TAG_LEN			30
#define DRIVER_MAX_TYPE_PATH_LEN		5
#define DRIVER_MAX_TYPE_STRING_CONNECT_LEN	512

#define DRIVER_MIN_ID		0
#define DRIVER_MAX_ID		255

struct knot_thing;
struct knot_data_item;

typedef void (*driver_connected_cb_t) (void *user_data);
typedef void (*driver_disconnected_cb_t) (void *user_data);

typedef enum DRIVER_PROTOCOLS {
	MODBUS,
	ETHERNET_IP,
	OPC_UA
} driver_protocols_type;

struct driver_ops {
	driver_protocols_type type;

	int (*read)(struct knot_data_item *data_item);
	int (*start)(struct knot_thing thing_props,
		     driver_connected_cb_t connected_cb,
		     driver_disconnected_cb_t disconnected_cb,
		     void *user_data);
	void (*stop)(void);
	int (*config)(struct knot_data_item *data_item,
		      struct knot_thing *thing_props);
};

struct driver_ops *create_driver(char *type);
void destroy_driver(struct driver_ops *driver);
