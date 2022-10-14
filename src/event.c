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
#include "event.h"

#define is_change_flag_set(a) ((a) & KNOT_EVT_FLAG_CHANGE)
#define is_lower_flag_set(a) ((a) & KNOT_EVT_FLAG_LOWER_THRESHOLD)
#define is_upper_flag_set(a) ((a) & KNOT_EVT_FLAG_UPPER_THRESHOLD)

struct data_item_timeout {
	int id;
	unsigned int timeout_sec;
};

timeout_cb_t timeout_cb;
static bool active;
struct l_timeout *timeout_to;

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

static int compare_double(double val1, double val2)
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
	case KNOT_VALUE_TYPE_DOUBLE:
		rc = compare_double(val1.val_d, val2.val_d);
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
	int *time_sec = data;

	l_timeout_modify(to, *time_sec);
	timeout_cb();
}

static int comp_change_lower_flag(knot_value_type current_val,
				  knot_value_type sent_val,
				  knot_value_type lower_limit,
				  int value_type)
{
	int rc = KNOT_ERR_INVALID;

	if (!is_value_equal(current_val, sent_val, value_type))
		if (is_lower_than_threshold(current_val,
				lower_limit, value_type))
			rc = KNOT_STATUS_OK;

	return rc;
}

static int comp_change_upper_flag(knot_value_type current_val,
				  knot_value_type sent_val,
				  knot_value_type upper_limit,
				  int value_type)
{
	int rc = KNOT_ERR_INVALID;

	if (!is_value_equal(current_val, sent_val, value_type))
		if (!is_higher_than_threshold(current_val,
				upper_limit, value_type))
			rc = KNOT_STATUS_OK;

	return rc;
}

static int comp_upper_lower_flag(knot_value_type current_val,
			     knot_value_type lower_limit,
			     knot_value_type upper_limit,
			     int value_type)
{
		int rc = KNOT_ERR_INVALID;

	if (!is_higher_than_threshold(current_val, upper_limit, value_type))
		if (!is_lower_than_threshold(current_val, lower_limit,
					value_type))
			rc = KNOT_STATUS_OK;

	return rc;
}

static int comp_all_flag(knot_value_type current_val,
			 knot_value_type lower_limit,
			 knot_value_type upper_limit,
			 knot_value_type sent_val,
			 int value_type)
{
	int rc = KNOT_ERR_INVALID;

	if (!is_value_equal(current_val, sent_val, value_type))
		if (!is_higher_than_threshold(current_val, upper_limit,
					value_type))
			if (!is_lower_than_threshold(current_val, lower_limit,
					value_type))
				rc = KNOT_STATUS_OK;

	return rc;
}

int event_check_value(knot_event event, knot_value_type current_val,
		      knot_value_type sent_val, int value_type)
{
	int rc = KNOT_ERR_INVALID;

	if (!active)
		return -ENOMSG;

	if (value_type < KNOT_VALUE_TYPE_MIN ||
			value_type > KNOT_VALUE_TYPE_MAX)
		return -EINVAL;

	if (event.event_flags) {
		switch (event.event_flags) {
		case KNOT_EVT_FLAG_CHANGE:
			if (!is_value_equal(current_val, sent_val, value_type))
				rc = KNOT_STATUS_OK;
			break;
		case KNOT_EVT_FLAG_LOWER_THRESHOLD:
			if (!is_lower_than_threshold(current_val,
				event.lower_limit, value_type))
				rc = KNOT_STATUS_OK;
			break;
		case KNOT_EVT_FLAG_UPPER_THRESHOLD:
			if (!is_higher_than_threshold(current_val,
				event.upper_limit, value_type))
				rc = KNOT_STATUS_OK;
			break;
		case KNOT_EVENT_CHANGE_UPPER_FLAG:
			if (!comp_change_upper_flag(current_val, sent_val,
						   event.upper_limit,
						   value_type))
				rc = KNOT_STATUS_OK;
			break;
		case KNOT_EVENT_CHANGE_LOWER_FLAG:
			if (!comp_change_lower_flag(current_val, sent_val,
				event.lower_limit,
				value_type))
				rc = KNOT_STATUS_OK;
			break;
		case KNOT_EVENT_UPPER_LOWER_FLAG:
			if (!comp_upper_lower_flag(current_val,
						event.lower_limit,
						event.upper_limit,
						value_type))
				rc = KNOT_STATUS_OK;
			break;
		case KNOT_EVENT_FLAG_MAX:
			if (!comp_all_flag(current_val,
					event.lower_limit,
					event.upper_limit,
					sent_val,
					value_type))
				rc = KNOT_STATUS_OK;
			break;
		default:
			rc = KNOT_ERR_INVALID;
			break;
		}
	} else
		rc = KNOT_STATUS_OK;

	return rc;
}

int event_start(timeout_cb_t cb, int time_sec)
{
	int *time_sec_aux = l_new(int, 1);

	*time_sec_aux = time_sec;
	timeout_to = l_timeout_create(time_sec,
				      on_sensor_to,
				      time_sec_aux,
				      l_free);
	if (!timeout_to)
		return -ENOMSG;
	timeout_cb = cb;
	active = true;

	return 0;
}

void event_stop(void)
{
	active = false;
	if (timeout_to)
		l_timeout_remove(timeout_to);
}
