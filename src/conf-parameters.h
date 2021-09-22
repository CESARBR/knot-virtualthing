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

#define CREDENTIALS_GROUP		"Credentials"
#define CREDENTIALS_THING_ID		"ThingId"
#define CREDENTIALS_THING_TOKEN		"ThingToken"

#define CLOUD_GROUP			"Cloud"
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

#define EVENT_LOWER_THRESHOLD		"EventLowerThreshold"
#define EVENT_UPPER_THRESHOLD		"EventUpperThreshold"
#define EVENT_TIME_SEC			"EventTimeSec"
#define EVENT_CHANGE			"EventChange"
#define EVENT_CHANGE_TRUE		1

#define MODBUS_REG_ADDRESS		"ModbusRegisterAddress"
#define MODBUS_BIT_OFFSET		"ModbusBitOffset"
#define MODBUS_TYPE_ENDIANNESS		"ModbusTypeEndianness"

// definition of endianness type
#define MODBUS_ENDIANNESS_TYPE_BIG_ENDIAN		0x01
#define MODBUS_ENDIANNESS_TYPE_MID_BIG_ENDIAN		0x02
#define MODBUS_ENDIANNESS_TYPE_LITTLE_ENDIAN		0x03
#define MODBUS_ENDIANNESS_TYPE_MID_LITTLE_ENDIAN	0x04
