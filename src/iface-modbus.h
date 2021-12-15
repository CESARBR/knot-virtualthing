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

typedef void (*iface_modbus_connected_cb_t) (void *user_data);
typedef void (*iface_modbus_disconnected_cb_t) (void *user_data);

int iface_modbus_read_data(struct knot_data_item *data_item);
int iface_modbus_start(struct knot_thing thing,
		       iface_modbus_connected_cb_t connected_cb,
		       iface_modbus_disconnected_cb_t disconnected_cb,
		       void *user_data);
void iface_modbus_stop(void);
