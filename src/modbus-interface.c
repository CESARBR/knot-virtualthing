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

#include "modbus-interface.h"
#include "modbus-driver.h"

#define TCP_PREFIX "tcp://"
#define TCP_PREFIX_SIZE 6
#define RTU_PREFIX "serial://"
#define RTU_PREFIX_SIZE 9

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
	bool val_bool;
	uint8_t val_byte;
	uint16_t val_u16;
	uint32_t val_u32;
	uint64_t val_u64;
};

struct modbus_driver connection_interface;
modbus_t *modbus_ctx;
modbus_conn_cb_t conn_changed_cb;

static int parse_url(const char *url)
{
	if (strncmp(url, TCP_PREFIX, TCP_PREFIX_SIZE) == 0)
		return TCP;
	else if (strncmp(url, RTU_PREFIX, RTU_PREFIX_SIZE) == 0)
		return RTU;
	else
		return -EINVAL;
}

int modbus_read_data(int reg_addr, int bit_offset, knot_value_type *out)
{
	int rc;
	union modbus_types tmp;
	uint8_t byte_tmp[8];
	uint8_t i;

	memset(&tmp, 0, sizeof(tmp));

	switch (bit_offset) {
	case TYPE_BOOL:
		rc = connection_interface.read_bool(modbus_ctx, reg_addr,
						    &tmp.val_bool);
		break;
	case TYPE_BYTE:
		rc = connection_interface.read_byte(modbus_ctx, reg_addr,
						    byte_tmp);
		/**
		 * Store in tmp.val_byte the value read from a Modbus Slave
		 * where each position of byte_tmp corresponds to a bit.
		*/
		for (i = 0; i < sizeof(byte_tmp); i++)
			tmp.val_byte |= byte_tmp[i] << i;
		break;
	case TYPE_U16:
		rc = connection_interface.read_u16(modbus_ctx, reg_addr,
						   &tmp.val_u16);
		break;
	case TYPE_U32:
		rc = connection_interface.read_u32(modbus_ctx, reg_addr,
						   &tmp.val_u32);
		break;
	case TYPE_U64:
		rc = connection_interface.read_u64(modbus_ctx, reg_addr,
						   &tmp.val_u64);
		break;
	default:
		rc = -EINVAL;
	}

	if (rc > 0)
		memcpy(out, &tmp, sizeof(tmp));
	else
		conn_changed_cb(false);

	return rc;
}

int modbus_start(const char *url, int slave_id,
		 modbus_conn_cb_t conn_change_cb)
{
	switch (parse_url(url)) {
	case TCP:
		connection_interface = tcp;
		break;
	case RTU:
		connection_interface = rtu;
		break;
	default:
		return -EINVAL;
	}

	modbus_ctx = connection_interface.create(url);
	if (!modbus_ctx)
		return -errno;

	if (modbus_set_slave(modbus_ctx, slave_id) < 0)
		return -errno;

	if (modbus_connect(modbus_ctx) < 0)
		return -errno;

	conn_changed_cb = conn_change_cb;
	conn_changed_cb(true);

	return 0;
}

void modbus_stop(void)
{
	if (modbus_ctx)
		connection_interface.destroy(modbus_ctx);
}
