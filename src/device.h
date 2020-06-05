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

struct device_settings {
	char *credentials_path;
	char *device_path;
	char *rabbitmq_path;
};

void device_msg_timeout_create(int seconds);
void device_msg_timeout_modify(int seconds);
void device_msg_timeout_remove(void);
int device_start_config(void);
void device_stop_config(void);
int device_read_data(int id);
int device_send_schema();
int device_has_credentials(void);
int device_store_credentials(char *token);
int device_send_register_request();
void device_generate_new_id();
int device_send_auth_request(void);
int device_check_schema_change(void);
int device_clear_credentials(void);
void device_publish_data_list(struct l_queue *sensor_id_list);
void device_publish_data_all(void);
int device_start(struct device_settings *conf_files);
void device_destroy(void);
