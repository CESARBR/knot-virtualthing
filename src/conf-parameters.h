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

/**
 *  Configuration files parameters header file
 */

struct conf_files {
	char *device;
	char *rabbit;
	char *credentials;
};

#define CREDENTIALS_GROUP		"Credentials"
#define CREDENTIALS_THING_ID		"ThingId"
#define CREDENTIALS_THING_TOKEN		"ThingToken"

#define RABBIT_MQ_GROUP			"RabbitMQ"
#define RABBIT_URL			"Url"

#define DATA_ITEM_GROUP			"DataItem_"

#define THING_GROUP			"KNoTThing"
#define THING_NAME			"Name"
#define THING_USER_TOKEN		"UserToken"
#define THING_MODBUS_SLAVE_ID		"ModbusSlaveId"
#define THING_MODBUS_URL		"ModbusURL"
#define MODBUS_MIN_SLAVE_ID		0
#define MODBUS_MAX_SLAVE_ID		255

#define SCHEMA_SENSOR_ID		"SchemaSensorId"
#define SCHEMA_SENSOR_NAME		"SchemaSensorName"
#define SCHEMA_VALUE_TYPE		"SchemaValueType"
#define SCHEMA_UNIT			"SchemaUnit"
#define SCHEMA_TYPE_ID			"SchemaTypeId"

#define CONFIG_LOWER_THRESHOLD		"ConfigLowerThreshold"
#define CONFIG_UPPER_THRESHOLD		"ConfigUpperThreshold"
#define CONFIG_TIME_SEC			"ConfigTimeSec"
#define CONFIG_CHANGE			"ConfigChange"

#define MODBUS_REG_ADDRESS		"ModbusRegisterAddress"
#define MODBUS_BIT_OFFSET		"ModbusBitOffset"
