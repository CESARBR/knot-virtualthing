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

#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <knot/knot_protocol.h>
#include <knot/knot_types.h>
#include <ell/util.h>
#include <ell/queue.h>
#include <ell/timeout.h>

#include "knot-config.h"

#define is_timeout_flag_set(a) ((a) & KNOT_EVT_FLAG_TIME)
#define is_change_flag_set(a) ((a) & KNOT_EVT_FLAG_CHANGE)
#define is_lower_flag_set(a) ((a) & KNOT_EVT_FLAG_LOWER_THRESHOLD)
#define is_upper_flag_set(a) ((a) & KNOT_EVT_FLAG_UPPER_THRESHOLD)

struct data_item_timeout {
	int id;
	unsigned int timeout_sec;
};

struct l_queue *sensor_timeouts;
timeout_cb_t timeout_cb;

static int compare_int(int val1, int val2)
{
	if (val1 < val2)
		return -1;
	if (val1 > val2)
		return 1;

	return 0;
}

static int compare_float(float val1, float val2)
{
	if (val1 < val2)
		return -1;
	if (val1 > val2)
		return 1;

	return 0;
}

static int compare_bool(bool val1, bool val2)
{
	if (val1 == false && val2 == true)
		return -1;
	if (val1 == true && val2 == false)
		return 1;

	return 0;
}

static int compare_raw(unsigned char *val1, unsigned char *val2)
{
	/* TODO: compare raw */
	return 1;
}

static int compare_knot_value(knot_value_type val1, knot_value_type val2,
			      int value_type)
{
	int rc;

	switch (value_type) {
	case KNOT_VALUE_TYPE_INT:
		rc = compare_int(val1.val_i, val2.val_i);
		break;
	case KNOT_VALUE_TYPE_FLOAT:
		rc = compare_float(val1.val_f, val2.val_f);
		break;
	case KNOT_VALUE_TYPE_BOOL:
		rc = compare_bool(val1.val_b, val2.val_b);
		break;
	case KNOT_VALUE_TYPE_RAW:
		rc = compare_raw(val1.raw, val2.raw);
		break;
	default:
		rc = 1;
	}

	return rc;
}

static bool is_value_equal(knot_value_type new_value,
			   knot_value_type old_value,
			   int value_type)
{
	return compare_knot_value(new_value, old_value, value_type) == 0;
}

static bool is_lower_than_threshold(knot_value_type value,
				    knot_value_type threshold,
				    int value_type)
{
	return compare_knot_value(value, threshold, value_type) < 0;
}

static bool is_higher_than_threshold(knot_value_type value,
				     knot_value_type threshold,
				     int value_type)
{
	return compare_knot_value(value, threshold, value_type) > 0;
}

static void on_sensor_to(struct l_timeout *to, void *data)
{
	struct data_item_timeout *data_item_to_info = data;

	timeout_cb(data_item_to_info->id);
	l_timeout_modify(to, data_item_to_info->timeout_sec);
}

static void timeout_destroy(void *data)
{
	struct l_timeout *to = data;

	l_timeout_remove(to);
}

int config_check_value(knot_config config, knot_value_type current_val,
		       knot_value_type sent_val, int value_type)
{
	int rc;

	if (value_type < KNOT_VALUE_TYPE_MIN ||
			value_type > KNOT_VALUE_TYPE_MAX)
		return -EINVAL;

	if (is_change_flag_set(config.event_flags) &&
			!is_value_equal(current_val, sent_val, value_type))
		rc = 1;
	else if (is_lower_flag_set(config.event_flags) &&
			is_lower_than_threshold(current_val,
				config.lower_limit, value_type))
		rc = 1;
	else if (is_upper_flag_set(config.event_flags) &&
			is_higher_than_threshold(current_val,
				config.upper_limit, value_type))
		rc = 1;
	else
		rc = 0;

	return rc;
}

void config_add_data_item(int id, knot_config config)
{
	struct data_item_timeout *data = l_new(struct data_item_timeout, 1);

	data->id = id;
	data->timeout_sec = config.time_sec;

	if (is_timeout_flag_set(config.event_flags)) {
		struct l_timeout *to = l_timeout_create(config.time_sec,
							on_sensor_to,
							data,
							l_free);
		l_queue_push_head(sensor_timeouts, to);
	}
}

int config_start(timeout_cb_t cb)
{
	sensor_timeouts = l_queue_new();
	if (!sensor_timeouts)
		return -ENOMSG;
	timeout_cb = cb;

	return 0;
}

void config_stop(void)
{
	if (sensor_timeouts) {
		l_queue_destroy(sensor_timeouts, timeout_destroy);
		sensor_timeouts = NULL;
	}
}
