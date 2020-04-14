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

typedef void (*modbus_conn_cb_t) (bool);

int modbus_read_data(int reg_addr, int bit_offset, knot_value_type *out);
int modbus_start(const char *url, modbus_conn_cb_t conn_change_cb);
void modbus_stop(void);
