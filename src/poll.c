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
#include <ell/util.h>
#include <ell/queue.h>
#include <ell/timeout.h>

#include "poll.h"

struct poll_data {
	int id;
	int interval;
	poll_read_cb_t read_cb;
};

struct poll_entry {
	struct poll_data *data;
	struct l_timeout *to;
};

struct l_queue *poll_entries;
bool active;

static void on_poll_timeout(struct l_timeout *to, void *user_data)
{
	struct poll_data *data;

	if (!active)
		return;

	data = (struct poll_data *) user_data;
	data->read_cb(data->id);

	l_timeout_modify(to, data->interval);
}

static void entry_destroy(void *user_data)
{
	struct poll_entry *entry;

	entry = (struct poll_entry *) user_data;
	l_timeout_remove(entry->to);
	l_free(entry);
}

static void poll_timer_start(void *data, void *user_data)
{
	struct poll_entry *entry;

	entry = (struct poll_entry *) data;

	l_timeout_modify(entry->to, entry->data->interval);
}

void poll_start(void)
{
	active = true;
	l_queue_foreach(poll_entries, poll_timer_start, NULL);
}

void poll_stop(void)
{
	active = false;
}

int poll_create(int interval, int id, poll_read_cb_t read_cb)
{
	struct poll_entry *entry;
	struct poll_data *data;
	struct l_timeout *to;

	data = l_new(struct poll_data, 1);
	data->id = id;
	data->read_cb = read_cb;
	data->interval = interval;

	to = l_timeout_create(0, on_poll_timeout, data, l_free);
	if (!to)
		return -ENOMSG;

	entry = l_new(struct poll_entry, 1);
	entry->data = data;
	entry->to = to;

	if (!poll_entries)
		poll_entries = l_queue_new();
	l_queue_push_head(poll_entries, entry);

	return 0;
}

void poll_destroy(void)
{
	if (poll_entries) {
		l_queue_destroy(poll_entries, entry_destroy);
		poll_entries = NULL;
	}
}
