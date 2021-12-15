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
#include <modbus/modbus.h>
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

#include "conf-parameters.h"
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
	float val_float;
	uint8_t val_bool;
	uint8_t val_byte;
	uint16_t val_u16;
	uint32_t val_u32;
	uint64_t val_u64;
};

static struct l_timeout *connect_to;
static struct l_io *modbus_io;
static modbus_t *modbus_ctx;
static iface_modbus_connected_cb_t conn_cb;
static iface_modbus_disconnected_cb_t disconn_cb;

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

static modbus_t *create_ctx(const char *url)
{
	if (strncmp(url, TCP_PREFIX, TCP_PREFIX_SIZE) == 0) {
		return create_tcp(url);
	} else if (strncmp(url, RTU_PREFIX, RTU_PREFIX_SIZE) == 0) {
		return create_rtu(url);
	} else {
		l_error("Address (%s) not supported: Invalid prefix", url);
		errno = -EINVAL;
		return NULL;
	}
}

static void on_disconnected(struct l_io *io, void *user_data)
{
	if (disconn_cb)
		disconn_cb(user_data);

	if (connect_to)
		l_timeout_modify(connect_to, RECONNECT_TIMEOUT);
}

static void attempt_connect(struct l_timeout *to, void *user_data)
{
	l_debug("Trying to connect to Modbus");

	/* Check and close if a connection is already up */
	if (modbus_get_socket(modbus_ctx) != -1)
		modbus_close(modbus_ctx);

	/* Check and destroy if an IO is already allocated */
	if (modbus_io) {
		l_io_destroy(modbus_io);
		modbus_io = NULL;
	}

	if (modbus_connect(modbus_ctx) < 0) {
		l_error("error connecting to Modbus: %s",
			modbus_strerror(errno));
		goto retry;
	}

	modbus_io = l_io_new(modbus_get_socket(modbus_ctx));
	if (!modbus_io)
		goto connection_close;

	if (!l_io_set_disconnect_handler(modbus_io, on_disconnected, NULL,
					 NULL)) {
		l_error("Couldn't set Modbus disconnect handler");
		goto io_destroy;
	}

	if (conn_cb)
		conn_cb(user_data);

	return;

io_destroy:
	l_io_destroy(modbus_io);
	modbus_io = NULL;
connection_close:
	modbus_close(modbus_ctx);
retry:
	l_timeout_modify(to, RECONNECT_TIMEOUT);
}

static void iface_modbus_config_endianness_type_recv_32_bits(uint32_t *src,
				int endianness_type)
{
	switch (endianness_type) {
	case ENDIANNESS_TYPE_BIG_ENDIAN:
		*src = ((*src & 0xffff0000u) >> 16)|
			((*src & 0x0000ffffu) << 16);
		break;
	case ENDIANNESS_TYPE_MID_BIG_ENDIAN:
		*src = ((((*src) & 0xff000000u) >> 24)|
			(((*src) & 0x00ff0000u) >> 8)|
			(((*src) & 0x0000ff00u) << 8)|
			(((*src) & 0x000000ffu) << 24));
		break;
	case ENDIANNESS_TYPE_LITTLE_ENDIAN:
		*src = ((((*src) & 0xff000000u) >> 8)|
			(((*src) & 0x00ff0000u) << 8)|
			(((*src) & 0x0000ff00u) >> 8)|
			(((*src) & 0x000000ffu) << 8));
		break;
	case ENDIANNESS_TYPE_MID_LITTLE_ENDIAN:
		break;
	default:
		src = NULL;
		break;
	}
}

static void iface_modbus_config_endianness_type_recv_64_bits(uint64_t *src,
				int endianness_type)
{
	if (endianness_type == ENDIANNESS_TYPE_MID_BIG_ENDIAN ||
		endianness_type == ENDIANNESS_TYPE_LITTLE_ENDIAN){
		*src = ((*src & 0xff00000000000000u) >> 8)|
			((*src & 0x00ff000000000000u) << 8)|
			((*src & 0x0000ff0000000000u) >> 8)|
			((*src & 0x000000ff00000000u) << 8)|
			((*src & 0x00000000ff000000u) >> 8)|
			((*src & 0x0000000000ff0000u) << 8)|
			((*src & 0x000000000000ff00u) >> 8)|
			((*src & 0x00000000000000ffu) << 8);
	}
	if (endianness_type == ENDIANNESS_TYPE_MID_BIG_ENDIAN ||
		endianness_type == ENDIANNESS_TYPE_BIG_ENDIAN){
		*src = ((*src & 0xffff000000000000u) >> 48)|
			((*src & 0x0000ffff00000000u) >> 16)|
			((*src & 0x00000000ffff0000u) << 16)|
			((*src & 0x000000000000ffffu) << 48);
	}
}

int iface_modbus_read_data(int reg_addr, int bit_offset, knot_value_type *out,
			   int endianness_type)
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
		rc = modbus_read_registers(modbus_ctx, reg_addr, 1,
					   &tmp.val_u16);
		break;
	case TYPE_U32:
		rc = modbus_read_registers(modbus_ctx, reg_addr, 2,
					   (uint16_t *) &tmp.val_u32);
		iface_modbus_config_endianness_type_recv_32_bits(&tmp.val_u32,
						endianness_type);
		break;

	case TYPE_U64:
		rc = modbus_read_registers(modbus_ctx, reg_addr, 4,
					   (uint16_t *) &tmp.val_u64);
		iface_modbus_config_endianness_type_recv_64_bits(&tmp.val_u64,
						endianness_type);
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
	modbus_ctx = create_ctx(url);
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
	l_timeout_remove(connect_to);
	connect_to = NULL;

	l_io_destroy(modbus_io);
	modbus_io = NULL;

	modbus_close(modbus_ctx);
	modbus_free(modbus_ctx);
}

