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

#include <errno.h>
#include <stdbool.h>
#include <modbus.h>
#include <string.h>
#include <knot/knot_protocol.h>
#include <ell/ell.h>

#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <asm-generic/ioctls.h>

#include "iface-modbus.h"

#define TCP_PREFIX "tcp://"
#define TCP_PREFIX_SIZE 6
#define RTU_PREFIX "serial://"
#define RTU_PREFIX_SIZE 9
#define RECONNECT_TIMEOUT 5

enum driver_type {
	TCP,
	RTU
};

enum modbus_types_offset {
	TYPE_BOOL = 1,
	TYPE_BYTE = 8,
	TYPE_U16 = 16,
	TYPE_U32 = 32,
	TYPE_U64 = 64
};

union modbus_types {
	uint8_t val_bool;
	uint8_t val_byte;
	uint16_t val_u16;
	uint32_t val_u32;
	uint64_t val_u64;
};

struct l_timeout *connect_to;
struct l_io *modbus_io;
modbus_t *modbus_ctx;
iface_modbus_connected_cb_t conn_cb;
iface_modbus_disconnected_cb_t disconn_cb;

static modbus_t *create_rtu(const char *url)
{
	struct serial_rs485 rs485conf;
	modbus_t *ctx;
	int mode = MODBUS_RTU_RS232;
	int fd;
	int baud_rate;
	int data_bit;
	int stop_bit;
	char parity;
	char port[256];

	/* Ignoring "serial://" */
	if (sscanf(&url[8], "%255[^:]:%d , %c , %d , %d", port, &baud_rate,
		   &parity, &data_bit, &stop_bit) != 5) {
		l_error("Address (%s) not supported: Invalid format", url);
		return NULL;
	}

	fd = open(&url[8], O_RDWR);
	if (fd < 0)
		return NULL;

	memset(&rs485conf, 0, sizeof(rs485conf));
	if (ioctl(fd, TIOCGRS485, &rs485conf) < 0) {
		mode = MODBUS_RTU_RS232;
		l_info("Switching to RS-232 ...");
	} else {
		mode = MODBUS_RTU_RS485;
		l_info("Switching to RS-485 ...");
	}

	close(fd);

	ctx = modbus_new_rtu(port, baud_rate, parity, data_bit, stop_bit);

	if (!ctx)
		return NULL;

	modbus_rtu_set_serial_mode(ctx, mode);
	modbus_rtu_set_rts(ctx, MODBUS_RTU_RTS_NONE);

	return ctx;
}

static modbus_t *create_tcp(const char *url)
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

static void destroy_ctx(modbus_t *ctx)
{
	modbus_close(ctx);
	modbus_free(ctx);
}

static int parse_url(const char *url)
{
	if (strncmp(url, TCP_PREFIX, TCP_PREFIX_SIZE) == 0)
		return TCP;
	else if (strncmp(url, RTU_PREFIX, RTU_PREFIX_SIZE) == 0)
		return RTU;
	else
		return -EINVAL;
}

static void on_disconnected(struct l_io *io, void *user_data)
{
	modbus_close(modbus_ctx);

	if (disconn_cb)
		disconn_cb(user_data);

	l_io_destroy(modbus_io);
	modbus_io = NULL;
	l_timeout_modify(connect_to, RECONNECT_TIMEOUT);
}

static void attempt_connect(struct l_timeout *to, void *user_data)
{
	l_debug("Trying to connect to Modbus");

	if (modbus_connect(modbus_ctx) < 0) {
		l_error("error connecting to Modbus: %s",
			modbus_strerror(errno));
		l_timeout_modify(to, RECONNECT_TIMEOUT);
		return;
	}

	modbus_io = l_io_new(modbus_get_socket(modbus_ctx));
	if (!l_io_set_disconnect_handler(modbus_io, on_disconnected, NULL,
					 NULL))
		l_error("Couldn't set Modbus disconnect handler");

	if (conn_cb)
		conn_cb(user_data);
}

int iface_modbus_read_data(int reg_addr, int bit_offset, knot_value_type *out)
{
	int rc;
	union modbus_types tmp;
	uint8_t byte_tmp[8];
	uint8_t i;

	memset(&tmp, 0, sizeof(tmp));

	switch (bit_offset) {
	case TYPE_BOOL:
		rc = modbus_read_input_bits(modbus_ctx, reg_addr, 1,
					    &tmp.val_bool);
		break;
	case TYPE_BYTE:
		rc = modbus_read_input_bits(modbus_ctx, reg_addr, 8, byte_tmp);
		/**
		 * Store in tmp.val_byte the value read from a Modbus Slave
		 * where each position of byte_tmp corresponds to a bit.
		*/
		for (i = 0; i < sizeof(byte_tmp); i++)
			tmp.val_byte |= byte_tmp[i] << i;
		break;
	case TYPE_U16:
		rc = modbus_read_registers(modbus_ctx, reg_addr, 2,
					   &tmp.val_u16);
		break;
	case TYPE_U32:
		rc = modbus_read_registers(modbus_ctx, reg_addr, 2,
					   (uint16_t *) &tmp.val_u32);
		break;
	case TYPE_U64:
		rc = modbus_read_registers(modbus_ctx, reg_addr, 4,
					   (uint16_t *) &tmp.val_u64);
		break;
	default:
		rc = -EINVAL;
	}

	if (rc < 0) {
		rc = -errno;
		l_error("Failed to read from Modbus: %s (%d)",
			modbus_strerror(errno), rc);
	} else {
		memcpy(out, &tmp, sizeof(tmp));
	}

	return rc;
}

int iface_modbus_start(const char *url, int slave_id,
		       iface_modbus_connected_cb_t connected_cb,
		       iface_modbus_disconnected_cb_t disconnected_cb,
		       void *user_data)
{
	switch (parse_url(url)) {
	case TCP:
		modbus_ctx = create_tcp(url);
		break;
	case RTU:
		modbus_ctx = create_rtu(url);
		break;
	default:
		return -EINVAL;
	}

	if (!modbus_ctx)
		return -errno;

	if (modbus_set_slave(modbus_ctx, slave_id) < 0)
		return -errno;

	conn_cb = connected_cb;
	disconn_cb = disconnected_cb;

	connect_to = l_timeout_create_ms(1, attempt_connect, NULL, NULL);

	return 0;
}

void iface_modbus_stop(void)
{
	if (likely(connect_to))
		l_timeout_remove(connect_to);

	if (modbus_io)
		l_io_destroy(modbus_io);

	if (modbus_ctx)
		destroy_ctx(modbus_ctx);
}
