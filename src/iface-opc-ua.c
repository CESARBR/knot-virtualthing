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


#include <errno.h>
#include <stdbool.h>
#include <open62541/client.h>
#include <string.h>
#include <knot/knot_protocol.h>
#include <ell/ell.h>

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>

#include "conf-device.h"
#include "iface-opc-ua.h"

#define RECONNECT_TIMEOUT 5

static struct l_timeout *connect_to;
static struct l_io *opc_ua_io;
static iface_opc_ua_connected_cb_t conn_cb;
static iface_opc_ua_disconnected_cb_t disconn_cb;
struct knot_thing thing_opc_ua;
static UA_Client *opc_ua_client;


int iface_opc_ua_read_data(struct knot_data_item *data_item)
{
	return 0;
}

static void on_disconnected(struct l_io *io, void *user_data)
{
	if (disconn_cb)
		disconn_cb(user_data);

	if (connect_to)
		l_timeout_modify(connect_to, RECONNECT_TIMEOUT);
}

static void attempt_connect(struct l_timeout *to, void *user_data)
{
	UA_StatusCode retval = 0;

	l_debug("Trying to connect to OPC UA Server");

	/* Check and destroy if an IO is already allocated */
	if (opc_ua_io) {
		l_io_destroy(opc_ua_io);
		opc_ua_io = NULL;
	}

	retval = UA_Client_connect(opc_ua_client,
				   thing_opc_ua.geral_url);
	if (retval != UA_STATUSCODE_GOOD) {
		l_error("Error connecting to OPC UA Server: %#010x",
			retval);
		goto retry;
	}

	opc_ua_io = l_io_new(retval);
	if (!opc_ua_io) {
		l_error("Error setting up io state");
		goto connection_close;
	}

	if (!l_io_set_disconnect_handler(opc_ua_io, on_disconnected, user_data,
					NULL)) {
		l_error("Couldn't set OPC UA disconnect handler");
		goto io_destroy;
	}

	if (conn_cb)
		conn_cb(user_data);

	return;

io_destroy:
	l_io_destroy(opc_ua_io);
	opc_ua_io = NULL;
connection_close:
	UA_Client_disconnect(opc_ua_client);
retry:
	l_timeout_modify(to, RECONNECT_TIMEOUT);
}

int iface_opc_ua_start(struct knot_thing thing,
		       iface_opc_ua_connected_cb_t connected_cb,
		       iface_opc_ua_disconnected_cb_t disconnected_cb,
		       void *user_data)
{
	UA_StatusCode retval = 0;

	thing_opc_ua = thing;

	opc_ua_client = UA_Client_new();
	retval = UA_ClientConfig_setDefault(UA_Client_getConfig(opc_ua_client));
	if (retval != UA_STATUSCODE_GOOD)
		return -(int)retval;

	conn_cb = connected_cb;
	disconn_cb = disconnected_cb;

	connect_to = l_timeout_create_ms(1, attempt_connect,
					 NULL, NULL);

	return 0;
}

void iface_opc_ua_stop(void)
{
	l_timeout_remove(connect_to);
	connect_to = NULL;

	l_io_destroy(opc_ua_io);
	opc_ua_io = NULL;

	UA_Client_disconnect(opc_ua_client);
	UA_Client_delete(opc_ua_client);
}
