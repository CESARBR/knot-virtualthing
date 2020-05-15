/*
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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <stdlib.h>
#include <errno.h>
#include <ell/ell.h>

#include "sm-pvt.h"
#include "device.h"

typedef void (*enter_state_t)(void);
typedef enum STATES (*get_next_t)(enum EVENTS, void *user_data);

struct state {
	enter_state_t enter;
	get_next_t get_next;
};

static struct state *current_state;
struct state states[N_OF_STATES];

/* DISCONNECT */
enum STATES get_next_disconnected(enum EVENTS event, void *user_data)
{
	enum STATES next_state;

	switch(event) {
	case EVT_READY:
		if(device_has_credentials())
			next_state = ST_AUTH;
		else {
			next_state = ST_REGISTER;
			device_generate_new_id();
		}
		break;
	case EVT_NOT_READY:
	case EVT_TIMEOUT:
	case EVT_AUTH_OK:
	case EVT_AUTH_NOT_OK:
	case EVT_REG_OK:
	case EVT_REG_NOT_OK:
	case EVT_SCH_OK:
	case EVT_SCH_NOT_OK:
	case EVT_UNREG_REQ:
	case EVT_REG_PERM:
	case EVT_PUB_DATA:
	case EVT_DATA_UPDT:
		next_state = ST_DISCONNECTED;
		break;
	default:
		next_state = ST_ERROR;
	}

	return next_state;
}

static void enter_disconnected(void)
{
	/* No action necessary when entering state disconnected */
}

/* AUTH */
enum STATES get_next_auth(enum EVENTS event, void *user_data)
{
	enum STATES next_state;

	switch(event) {
	case EVT_NOT_READY:
		next_state = ST_DISCONNECTED;
		break;
	case EVT_AUTH_OK:
		next_state =
			device_check_schema_change() ? ST_SCHEMA : ST_ONLINE;
		break;
	case EVT_AUTH_NOT_OK:
	case EVT_UNREG_REQ:
		next_state = ST_UNREGISTER;
		break;
	case EVT_READY:
	case EVT_TIMEOUT:
	case EVT_REG_OK:
	case EVT_REG_NOT_OK:
	case EVT_SCH_OK:
	case EVT_SCH_NOT_OK:
	case EVT_REG_PERM:
	case EVT_PUB_DATA:
	case EVT_DATA_UPDT:
		next_state = ST_AUTH;
		break;
	default:
		next_state = ST_ERROR;
	}

	return next_state;
}

static void enter_auth(void)
{
	int rc;

	rc = device_send_auth_request();

	if(rc < 0)
		l_error("Couldn't send auth message");
}

/* REGISTER */
enum STATES get_next_register(enum EVENTS event, void *user_data)
{
	enum STATES next_state;
	int rc;

	switch(event) {
	case EVT_NOT_READY:
		next_state = ST_DISCONNECTED;
		break;
	case EVT_REG_OK:
		rc = device_store_credentials(user_data);
		if(rc < 0) {
			next_state = ST_ERROR;
			l_error("Failed to write credentials");
			break;
		}
		next_state = ST_AUTH;
		break;
	case EVT_REG_NOT_OK:
		device_generate_new_id();
		next_state = ST_REGISTER;
		break;
	case EVT_UNREG_REQ:
	case EVT_READY:
	case EVT_TIMEOUT:
	case EVT_AUTH_OK:
	case EVT_AUTH_NOT_OK:
	case EVT_SCH_OK:
	case EVT_SCH_NOT_OK:
	case EVT_REG_PERM:
	case EVT_PUB_DATA:
	case EVT_DATA_UPDT:
		next_state = ST_REGISTER;
		break;
	default:
		next_state = ST_ERROR;
	}

	return next_state;
}

static void enter_register(void)
{
	int rc;

	rc = device_send_register_request();
	if(rc < 0)
		l_error("Couldn't send register message");

}

/* SCHEMA */
enum STATES get_next_schema(enum EVENTS event, void *user_data)
{
	int next_state;

	switch(event) {
	case EVT_NOT_READY:
		next_state = ST_DISCONNECTED;
		break;
	case EVT_SCH_OK:
		next_state = ST_ONLINE;
		break;
	case EVT_SCH_NOT_OK:
		next_state = ST_ERROR;
		break;
	case EVT_UNREG_REQ:
		next_state = ST_UNREGISTER;
		break;
	case EVT_READY:
	case EVT_TIMEOUT:
	case EVT_AUTH_OK:
	case EVT_AUTH_NOT_OK:
	case EVT_REG_OK:
	case EVT_REG_NOT_OK:
	case EVT_REG_PERM:
	case EVT_PUB_DATA:
	case EVT_DATA_UPDT:
		next_state = ST_SCHEMA;
		break;
	default:
		next_state = ST_ERROR;
		break;
	}

	return next_state;
}

static void enter_schema(void)
{
	int rc;

	rc = device_send_schema();

	if(rc < 0)
		l_error("Failure sending schema");
}

/* ONLINE */
enum STATES get_next_online(enum EVENTS event, void *user_data)
{
	int next_state;

	switch(event) {
	case EVT_NOT_READY:
		device_stop_config();
		next_state = ST_DISCONNECTED;
		break;
	case EVT_PUB_DATA:
		device_publish_data_list(user_data);
		next_state = ST_ONLINE;
		break;
	case EVT_DATA_UPDT:
		/* TODO: Write to modbus actuator */
		next_state = ST_ONLINE;
		break;
	case EVT_TIMEOUT:
	case EVT_SCH_OK:
	case EVT_SCH_NOT_OK:
	case EVT_UNREG_REQ:
	case EVT_READY:
	case EVT_AUTH_OK:
	case EVT_AUTH_NOT_OK:
	case EVT_REG_OK:
	case EVT_REG_NOT_OK:
	case EVT_REG_PERM:
		next_state = ST_ONLINE;
		break;
	default:
		next_state = ST_ERROR;
		break;
	}

	return next_state;
}

static void enter_online(void)
{
	int err;

	device_publish_data_all();
	err = device_start_config();
	if (err < 0)
		l_error("Couldn't start config");
}

/* UNREGISTER */
enum STATES get_next_unregister(enum EVENTS event, void *user_data)
{
	int next_state;

	switch(event) {
	case EVT_REG_PERM:
		device_generate_new_id();
		next_state = ST_REGISTER;
		break;
	case EVT_NOT_READY:
	case EVT_PUB_DATA:
	case EVT_DATA_UPDT:
	case EVT_TIMEOUT:
	case EVT_SCH_OK:
	case EVT_SCH_NOT_OK:
	case EVT_UNREG_REQ:
	case EVT_READY:
	case EVT_AUTH_OK:
	case EVT_AUTH_NOT_OK:
	case EVT_REG_OK:
	case EVT_REG_NOT_OK:
		next_state = ST_UNREGISTER;
		break;
	default:
		next_state = ST_ERROR;
	}

	return next_state;
}

static void enter_unregister(void)
{
	int rc;

	rc = device_clear_credentials();
	if(rc < 0)
		l_error("Something went wrong when cleaning credentials");
}

/* ERROR */
enum STATES get_next_error(enum EVENTS event, void *user_data)
{
	int next_state;

	switch(event) {
	case EVT_REG_PERM:
	case EVT_NOT_READY:
	case EVT_PUB_DATA:
	case EVT_DATA_UPDT:
	case EVT_TIMEOUT:
	case EVT_SCH_OK:
	case EVT_SCH_NOT_OK:
	case EVT_UNREG_REQ:
	case EVT_READY:
	case EVT_AUTH_OK:
	case EVT_AUTH_NOT_OK:
	case EVT_REG_OK:
	case EVT_REG_NOT_OK:
	default:
		next_state = ST_ERROR;
		break;
	}

	return next_state;
}

static void enter_error(void)
{
	/*  TODO: Add usuability to warn user of error state */
}

/* STATE INDEPENDENT */
static struct state sm_create_state(enter_state_t enter, get_next_t next)
{
	struct state st = {
		enter,
		next
	};

	return st;
};

void sm_input_event(enum EVENTS event, void *user_data)
{
	enum STATES id = current_state->get_next(event, user_data);
	struct state *next = &states[id];

	if (next != current_state) {
		if (next->enter)
			next->enter();
		current_state = next;
	}
}

void sm_start(void)
{
	states[ST_DISCONNECTED] = sm_create_state(enter_disconnected,
						  get_next_disconnected);
	states[ST_AUTH] = sm_create_state(enter_auth, get_next_auth);
	states[ST_REGISTER] = sm_create_state(enter_register,
					      get_next_register);
	states[ST_SCHEMA] = sm_create_state(enter_schema, get_next_schema);
	states[ST_ONLINE] = sm_create_state(enter_online, get_next_online);
	states[ST_UNREGISTER] = sm_create_state(enter_unregister,
						get_next_unregister);
	states[ST_ERROR] = sm_create_state(enter_error, get_next_error);
	current_state = &states[ST_DISCONNECTED];
}
