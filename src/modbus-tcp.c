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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <ell/ell.h>

#include <modbus.h>

#include "modbus-driver.h"

static modbus_t *create(const char *url)
{
	char hostname[128];
	char port[8];

	memset(hostname, 0, sizeof(hostname));
	memset(port, 0, sizeof(port));

	/* Ignoring "tcp://" */
	if (sscanf(&url[6], "%127[^:]:%7s", hostname, port) != 2) {
		l_error("Address (%s) not supported: Invalid format", url);
		return NULL;
	}

	return modbus_new_tcp_pi(hostname, port);
}

static void destroy(modbus_t *ctx)
{
	modbus_close(ctx);
	modbus_free(ctx);
}

static int read_bool(modbus_t *ctx, uint16_t addr, bool *out)
{
	uint8_t val_u8 = 0;
	int ret;

	ret = modbus_read_input_bits(ctx, addr, 1, &val_u8);
	if (ret > 0)
		*out = (val_u8 ? true : false);

	return ret;
}

static int read_byte(modbus_t *ctx, uint16_t addr, uint8_t *out)
{
	return modbus_read_input_bits(ctx, addr, 8, out);
}

static int read_u16(modbus_t *ctx, uint16_t addr, uint16_t *out)
{
	return modbus_read_registers(ctx, addr, 1, out);
}

static int read_u32(modbus_t *ctx, uint16_t addr, uint32_t *out)
{
	return modbus_read_registers(ctx, addr, 2, (uint16_t *) out);
}

static int read_u64(modbus_t *ctx, uint16_t addr, uint64_t *out)
{
	return modbus_read_registers(ctx, addr, 4, (uint16_t *) out);
}

struct modbus_driver tcp = {
	.name = "tcp",
	.create = create,
	.destroy = destroy,
	.read_bool = read_bool,
	.read_byte = read_byte,
	.read_u16 = read_u16,
	.read_u32 = read_u32,
	.read_u64 = read_u64,
};
