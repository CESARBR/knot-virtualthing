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

#include "knot-config.h"

#define is_change_flag_set(a) ((a) & KNOT_EVT_FLAG_CHANGE)
#define is_lower_flag_set(a) ((a) & KNOT_EVT_FLAG_LOWER_THRESHOLD)
#define is_upper_flag_set(a) ((a) & KNOT_EVT_FLAG_UPPER_THRESHOLD)

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

int config_check_value(knot_config config, knot_value_type value,
		       int value_type)
{
	static knot_value_type last_value;
	int rc;

	if (value_type < KNOT_VALUE_TYPE_MIN ||
			value_type > KNOT_VALUE_TYPE_MAX)
		return -EINVAL;

	if (is_change_flag_set(config.event_flags) &&
			is_value_equal(value, last_value, value_type))
		rc = 1;
	else if (is_lower_flag_set(config.event_flags) &&
			is_lower_than_threshold(value,
				config.lower_limit, value_type))
		rc = 1;
	else if (is_upper_flag_set(config.event_flags) &&
			is_higher_than_threshold(value,
				config.upper_limit, value_type))
		rc = 1;
	else
		rc = 0;

	last_value = value;

	return rc;
}
