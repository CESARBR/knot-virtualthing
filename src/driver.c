/**
 * This file is part of the KNOT Project
 *
 * Copyright (c) 2021, CESAR. All rights reserved.
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

#include <stdio.h>
#include <ell/ell.h>
#include <knot/knot_protocol.h>

#include "driver.h"
#include "conf-device.h"
#include "iface-modbus.h"
#include "iface-ethernet-ip.h"
#include "iface-opc-ua.h"

static driver_protocols_type map_settings_protocols(char *protocol)
{
	if (!strcmp(protocol, "MODBUS"))
		return MODBUS;
	else if (!strcmp(protocol, "ETHERNET/IP"))
		return ETHERNET_IP;
	else if (!strcmp(protocol, "OPC_UA"))
		return OPC_UA;
	return -1;
}

static struct driver_ops *create_ethernet_ip_driver(void)
{
	struct driver_ops *ethernet_ip_ops = l_new(struct driver_ops, 1);

	ethernet_ip_ops->type = ETHERNET_IP;
	ethernet_ip_ops->start = iface_ethernet_ip_start;
	ethernet_ip_ops->stop = iface_ethernet_ip_stop;
	ethernet_ip_ops->read = iface_ethernet_ip_read_data;

	return ethernet_ip_ops;
}

static struct driver_ops *create_modbus_driver(void)
{
	struct driver_ops *modbus_ops = l_new(struct driver_ops, 1);

	modbus_ops->type = MODBUS;
	modbus_ops->start = iface_modbus_start;
	modbus_ops->stop = iface_modbus_stop;
	modbus_ops->read = iface_modbus_read_data;

	return modbus_ops;
}

static struct driver_ops *create_opc_ua_driver(void)
{
	struct driver_ops *opc_ua_ops = l_new(struct driver_ops, 1);

	opc_ua_ops->type = OPC_UA;
	opc_ua_ops->start = iface_opc_ua_start;
	opc_ua_ops->stop = iface_opc_ua_stop;
	opc_ua_ops->read = iface_opc_ua_read_data;
	opc_ua_ops->config = iface_opc_ua_config;

	return opc_ua_ops;
}

struct driver_ops *create_driver(char *protocol)
{
	driver_protocols_type protocol_aux;

	protocol_aux = map_settings_protocols(protocol);

	switch (protocol_aux) {
	case MODBUS:
		return create_modbus_driver();
	case ETHERNET_IP:
		return create_ethernet_ip_driver();
	case OPC_UA:
		return create_opc_ua_driver();
	default:
		return NULL;
	}
}

void destroy_driver(struct driver_ops *driver)
{
	l_free(driver);
}
