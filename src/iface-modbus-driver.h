/*
 * This file is part of the KNOT Project
 *
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

struct modbus_driver {
	const char *name; /* tcp or rtu */
	modbus_t *(*create) (const char *url); /* url includes path and settings */
	void (*destroy) (modbus_t *ctx);

	int (*read_bool) (modbus_t *ctx, uint16_t addr, bool *out);
	int (*read_byte) (modbus_t *ctx, uint16_t addr, uint8_t *out);
	int (*read_u16) (modbus_t *ctx, uint16_t addr, uint16_t *out);
	int (*read_u32) (modbus_t *ctx, uint16_t addr, uint32_t *out);
	int (*read_u64) (modbus_t *ctx, uint16_t addr, uint64_t *out);
};
