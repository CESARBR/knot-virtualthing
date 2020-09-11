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
 *  Device header file
 */

struct knot_data_item;
struct knot_thing;

struct device_settings {
	char *credentials_path;
	char *device_path;
	char *cloud_path;
};

void device_set_log_priority(int priority);
void device_set_thing_name(struct knot_thing *thing, const char *name);
void device_set_thing_user_token(struct knot_thing *thing, char *token);
void device_set_thing_modbus_slave(struct knot_thing *thing, int slave_id,
				   char *url);
void device_set_new_data_item(struct knot_thing *thing, int sensor_id,
			      knot_schema schema, knot_event event,
			      int reg_addr, int bit_offset);
void *device_data_item_lookup(struct knot_thing *thing, int sensor_id);
void device_set_thing_rabbitmq_url(struct knot_thing *thing, char *url);
void device_set_thing_credentials(struct knot_thing *thing, const char *id,
				  const char *token);
void device_generate_thing_id(void);
void device_clear_thing_id(struct knot_thing *thing);
void device_clear_thing_token(struct knot_thing *thing);
int device_has_thing_token(void);
int device_store_credentials_on_file(char *token);
int device_clear_credentials_on_file(void);

int device_update_config(struct l_queue *config_list);

int device_check_schema_change(void);

int device_send_register_request(void);
int device_send_auth_request(void);
int device_send_config(void);
void device_publish_data_list(struct l_queue *sensor_id_list);
void device_publish_data_all(void);

void device_msg_timeout_create(int seconds);
void device_msg_timeout_modify(int seconds);
void device_msg_timeout_remove(void);

int device_start_event(void);
void device_stop_event(void);

int device_start_read_cloud(void);

int device_start(struct device_settings *conf_files);
void device_destroy(void);
