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

#include "sm-pvt.h"

typedef void (*enter_state_t)(void);
typedef enum STATES (*get_next_t)(enum EVENTS, void *user_data);

struct state {
	enter_state_t enter;
	get_next_t get_next;
};

static struct state current_state;
struct state states[N_OF_STATES];

/* DISCONNECT */
enum STATES get_next_disconnected(enum EVENTS event, void *user_data)
{
	//TODO: Implement transitions
	return ST_DISCONNECTED;
}

static void enter_disconnected(void)
{
	//TODO: Implement expected state behavior
}

/* AUTH */
enum STATES get_next_auth(enum EVENTS event, void *user_data)
{
	//TODO: Implement transitions
	return ST_AUTH;
}

static void enter_auth(void)
{
	//TODO: Implement expected state behavior
}

/* REGISTER */
enum STATES get_next_register(enum EVENTS event, void *user_data)
{
	//TODO: Implement transitions
	return ST_REGISTER;
}

static void enter_register(void)
{
	//TODO: Implement expected state behavior
}

/* SCHEMA */
enum STATES get_next_schema(enum EVENTS event, void *user_data)
{
	//TODO: Implement transitions
	return ST_SCHEMA;
}

static void enter_schema(void)
{
	//TODO: Implement expected state behavior
}

/* ONLINE */
enum STATES get_next_online(enum EVENTS event, void *user_data)
{
	//TODO: Implement transitions
	return ST_ONLINE;
}

static void enter_online(void)
{
	//TODO: Implement expected state behavior
}

/* UNREGISTER */
enum STATES get_next_unregister(enum EVENTS event, void *user_data)
{
	//TODO: Implement transitions
	return ST_UNREGISTER;
}

static void enter_unregister(void)
{
	//TODO: Implement expected state behavior
}

/* ERROR */
enum STATES get_next_error(enum EVENTS event, void *user_data)
{
	//TODO: Implement transitions
	return ST_ERROR;
}

static void enter_error(void)
{
	//TODO: Implement expected state behavior
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

	enum STATES id = current_state.get_next(event, user_data);
	struct state next = states[id];
	if (&next != &current_state) {
		if (next.enter)
			next.enter();
		current_state = next;
	}
}

int sm_start(void)
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
	current_state = states[ST_DISCONNECTED];

	return 0;
}
