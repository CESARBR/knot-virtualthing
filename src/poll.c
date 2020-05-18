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
#include <ell/util.h>
#include <ell/queue.h>
#include <ell/timeout.h>

#include "poll.h"

struct poll_data {
	int id;
	int interval;
	poll_read_cb_t read_cb;
};

struct l_queue *poll_timeouts;

static void on_poll_timeout(struct l_timeout *to, void *user_data)
{
	struct poll_data *data;

	data = (struct poll_data *) user_data;
	data->read_cb(data->id);

	l_timeout_modify(to, data->interval);
}

static void timeout_destroy(void *user_data)
{
	struct l_timeout *to;

	to = (struct l_timeout *) user_data;
	l_timeout_remove(to);
}

int poll_create(int interval, int id, poll_read_cb_t read_cb)
{
	struct poll_data *data;
	struct l_timeout *to;

	data = l_new(struct poll_data, 1);
	data->id = id;
	data->read_cb = read_cb;
	data->interval = interval;

	to = l_timeout_create(data->interval, on_poll_timeout, data, l_free);
	if (!to)
		return -ENOMSG;

	if (!poll_timeouts)
		poll_timeouts = l_queue_new();
	l_queue_push_head(poll_timeouts, to);

	return 0;
}

void poll_destroy(void)
{
	if (poll_timeouts)
		l_queue_destroy(poll_timeouts, timeout_destroy);
}
