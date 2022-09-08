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

#define THING_GROUP			"KNoTThing"
#define THING_NAME			"Name"
#define THING_USER_TOKEN		"UserToken"

#define DRIVER_URL			"DriverURL"
#define DRIVER_PROTOCOL_TYPE		"DeviceProtocolType"
#define DRIVER_ID			"DeviceId"
#define DRIVER_TIME_SEC			"DeviceTimeSec"

// ATTENTION: Name Type ONLY used for Ethernet/IP communication.
#define DRIVER_NAME_TYPE		"DeviceNameType"

// ATTENTION: Login, Password and Security ONLY used for OPC UA communication.
#define DRIVER_LOGIN			"DeviceLogin"
#define DRIVER_PASSWORD			"DevicePassword"
#define DRIVER_SECURITY			"DeviceSecurity"
#define DRIVER_PATH_CERTIFICATE		"DevicePathCertificate"
#define DRIVER_PATH_PRIVATE_KEY		"DevicePathPrivateKey"
#define DRIVER_SECURITY_MODE		"DeviceSecurityMode"
#define DRIVER_SECURITY_POLICY		"DeviceSecurityPolicy"

#define DATA_ITEM_GROUP			"DataItem_"
#define DATA_REG_ADDRESS		"RegisterAddress"
#define DATA_TYPE_ENDIANNESS		"TypeEndianness"
#define DATA_VALUE_TYPE_SIZE		"ValueTypeSize"

// ---------------------------------------------- //

/**
 * ATTENTION: Tag name, path and element size ONLY used for
 * Ethernet/IP communication.
 */
#define DATA_IP_TAG_NAME		"DataTagName"
#define DATA_IP_ELEMENT_SIZE		"DataIpElementSize"
#define DATA_IP_PATH			"DataIpPath"

/**
 * ATTENTION: Namespace, Identifier Type and Identifier ONLY
 * used for OPC UA communication.
 */
#define DATA_NAME_SPACE_INDEX		"DataNameSpaceIndex"
#define DATA_IND_TYPE			"DataIdentifierType"
#define DATA_IDENTIFIER			"DataIdentifier"

#define SCHEMA_SENSOR_ID		"SchemaSensorId"
#define SCHEMA_SENSOR_NAME		"SchemaSensorName"
#define SCHEMA_VALUE_TYPE		"SchemaValueType"
#define SCHEMA_UNIT			"SchemaUnit"
#define SCHEMA_TYPE_ID			"SchemaTypeId"

#define EVENT_LOWER_THRESHOLD		"EventLowerThreshold"
#define EVENT_UPPER_THRESHOLD		"EventUpperThreshold"
#define EVENT_CHANGE			"EventChange"
#define EVENT_CHANGE_TRUE		1

