/*
 * This file is part of the KNOT Project
 *
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

typedef void (storage_foreach_slave_t)(const char *key,
				       int id,
				       const char *name,
				       const char *address,
				       void *user_data);
typedef void (storage_foreach_source_t)(const char *address,
				       const char *name,
				       const char *type,
				       const char *unit,
				       int interval,
				       void *user_data);

void storage_foreach_slave(int fd,
			   storage_foreach_slave_t func,
			   void *user_data);
void storage_foreach_source(int fd,
			    storage_foreach_source_t func,
			    void *user_data);

int storage_open(const char *pathname);
int storage_close(int fd);

int storage_remove_group(int fd, const char *group);

int storage_write_key_string(int fd, const char *group,
			     const char *key, const char *value);
char *storage_read_key_string(int fd, const char *group, const char *key);
int storage_write_key_int(int fd, const char *group,
			  const char *key, int value);
int storage_read_key_int(int fd, const char *group,
			  const char *key, int *value);
bool storage_has_unit(int fd, const char *group, const char *key);
int get_number_of_data_items(int fd);
char **get_data_item_groups(int fd);
