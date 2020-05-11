/**
 * This file is part of the KNOT Project
 *
 * Copyright (c) 2019, CESAR. All rights reserved.
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
 *  Message Queue header file
 */

typedef bool (*mq_read_cb_t) (const char *exchange,
				const char *routing_key,
				const char *body,
				void *user_data);
typedef void (*mq_connected_cb_t) (void *user_data);
typedef void (*mq_disconnected_cb_t) (void *user_data);

int mq_bind_queue(amqp_bytes_t queue,
				const char *exchange,
				const char *routing_key);
amqp_bytes_t mq_declare_new_queue(const char *name);
int8_t mq_publish_direct_persistent_msg_rpc(amqp_bytes_t queue,
					    const char *exchange,
					    const char *routing_key,
					    amqp_table_entry_t *headers,
					    size_t num_headers,
					    uint64_t expiration_ms,
					    amqp_bytes_t reply_to,
					    const char *correlation_id,
					    const char *body);
int8_t mq_publish_direct_persistent_msg(amqp_bytes_t queue,
					const char *exchange,
					const char *routing_key,
					amqp_table_entry_t *headers,
					size_t num_headers,
					uint64_t expiration_ms,
					const char *body);
int mq_set_read_cb(amqp_bytes_t queue, mq_read_cb_t read_cb, void *user_data);
int mq_start(char *url, mq_connected_cb_t connected_cb,
	     mq_disconnected_cb_t disconnected_cb, void *user_data);
void mq_stop(void);
