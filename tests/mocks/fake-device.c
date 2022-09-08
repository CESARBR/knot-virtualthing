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

#include <knot/knot_protocol.h>
#include <ell/ell.h>

#include "src/device.h"
#include "fake-device.h"

int start_event_rc;
int schema_change_rc;
int cred_rc;
int store_cred_rc;

int device_start_event(void)
{
	return start_event_rc;
}

void device_stop_event(void)
{
	/* purposely left empty as no behaviour expected/required */
}

int device_send_config(void)
{
	return 0;
}

int device_has_thing_token(void)
{
	return cred_rc;
}

int device_store_credentials_on_file(char *token)
{
	return store_cred_rc;
}

int device_send_register_request(void)
{
	return 0;
}

void device_generate_thing_id(void)
{
	/* purposely left empty as no behaviour expected/required */
}

int device_send_auth_request(void)
{
	return 0;
}

int device_check_schema_change(void)
{
	return schema_change_rc;
}

int device_clear_credentials_on_file(void)
{
	return 0;
}

void device_publish_data(void)
{
	/* purposely left empty as no behaviour expected/required */
}

void device_set_schema_change_rc(int rc)
{
	schema_change_rc = rc;
}

void device_set_start_event_rc(int rc)
{
	start_event_rc = rc;
}

void device_set_has_cred_rc(int rc)
{
	cred_rc = rc;
}

void device_set_store_cred_rc(int rc)
{
	store_cred_rc = rc;
}
