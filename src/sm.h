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

enum EVENTS {
	EVT_READY,
	EVT_NOT_READY,
	EVT_TIMEOUT,
	EVT_AUTH_OK,
	EVT_AUTH_NOT_OK,
	EVT_REG_OK,
	EVT_REG_NOT_OK,
	EVT_SCH_OK,
	EVT_SCH_NOT_OK,
	EVT_UNREG_REQ,
	EVT_REG_PERM,
	EVT_PUB_DATA,
	EVT_DATA_UPDT
};

void sm_start(void);
void sm_input_event(enum EVENTS, void *user_data);
