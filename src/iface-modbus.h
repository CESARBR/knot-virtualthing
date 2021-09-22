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

extern struct modbus_driver tcp;
extern struct modbus_driver rtu;

typedef void (*iface_modbus_connected_cb_t) (void *user_data);
typedef void (*iface_modbus_disconnected_cb_t) (void *user_data);

int iface_modbus_read_data(int reg_addr, int bit_offset, knot_value_type *out,
		       int endianness_type);
int iface_modbus_start(const char *url, int slave_id,
		       iface_modbus_connected_cb_t connected_cb,
		       iface_modbus_disconnected_cb_t disconnected_cb,
		       void *user_data);
void iface_modbus_stop(void);
