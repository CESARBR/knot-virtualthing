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
 *  Properties header file
 */

int properties_create_device(struct knot_thing *thing,
			     struct device_settings *settings);

int properties_clear_credentials(struct knot_thing *thing, char *filename);
int properties_store_credentials(struct knot_thing *thing, char *filename,
				 char *id, char *token);
int properties_update_data_item(struct knot_thing *thing, char *filename,
				knot_msg_config *config);
