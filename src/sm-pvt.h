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

#include "sm.h"

enum STATES {
	ST_DISCONNECTED,
	ST_AUTH,
	ST_REGISTER,
	ST_SCHEMA,
	ST_ONLINE,
	ST_UNREGISTER,
	ST_ERROR,
	N_OF_STATES
};

enum STATES get_next_disconnected(enum EVENTS event, void *user_data);
enum STATES get_next_auth(enum EVENTS event, void *user_data);
enum STATES get_next_register(enum EVENTS event, void *user_data);
enum STATES get_next_schema(enum EVENTS event, void *user_data);
enum STATES get_next_online(enum EVENTS event, void *user_data);
enum STATES get_next_unregister(enum EVENTS event, void *user_data);
enum STATES get_next_error(enum EVENTS event, void *user_data);
