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
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <asm-generic/ioctls.h>

#include <ell/ell.h>

#include <modbus.h>

#include "options.h"
#include "driver.h"

static modbus_t *create(const char *url)
{
	struct serial_rs485 rs485conf;
	modbus_t *ctx;
	int mode = MODBUS_RTU_RS232;
	int fd;

	/*
	 * FIXME: Parse serial configuration encoded at url
	 * serial://dev/ttyUSB0:115200,'N',8,1
	 */

	/* Ignoring "serial:/" */
	l_info("RTU: %s", url);

	fd = open(&url[8], O_RDWR);
	if (fd < 0)
		return NULL;

	memset(&rs485conf, 0, sizeof(rs485conf));
	if (ioctl (fd, TIOCGRS485, &rs485conf) < 0) {
		 mode = MODBUS_RTU_RS232;
                 l_info("Switching to RS-232 ...");
         } else {
		 mode = MODBUS_RTU_RS485;
                 l_info("Switching to RS-485 ...");
	 }

	close(fd);

	ctx = modbus_new_rtu(&url[8], serial_opts.baud, serial_opts.parity,
				serial_opts.data_bit, serial_opts.stop_bit);

	if (!ctx)
		return NULL;

	modbus_rtu_set_serial_mode(ctx, mode);
	modbus_rtu_set_rts(ctx, MODBUS_RTU_RTS_NONE);

	return ctx;
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

struct modbus_driver rtu = {
	.name = "rtu",
	.create = create,
	.destroy = destroy,
	.read_bool = read_bool,
	.read_byte = read_byte,
	.read_u16 = read_u16,
	.read_u32 = read_u32,
	.read_u64 = read_u64,
};
