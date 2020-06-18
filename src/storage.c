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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <ell/ell.h>

#include "storage.h"
#include "conf-parameters.h"

static struct l_hashmap *storage_list = NULL;

static int make_dirs(const char *filename, const mode_t mode)
{
        char dir[PATH_MAX + 1], *previous, *next;
        struct stat st;
        int err;

	memset(&st, 0, sizeof(st));

        err = stat(filename, &st);
        if ((err == 0) && S_ISREG(st.st_mode))
                return 0;

        memset(dir, 0, PATH_MAX + 1);
        dir[0] ='/';

	previous = strchr(filename, '/');

	while (previous) {
		next = strchr(previous + 1, '/');
		if (!next)
			break;

		if (next - previous == 1) {
			previous = next;
			continue;
		}

		strncat(dir, previous + 1, next - previous);
		l_info("mkdir: %s", dir);

		if (mkdir(dir, mode) == -1) {
			err = errno;

			if (err != EEXIST)
				return err;
		}

		previous = next;
	}

	return 0;
}

static int save_settings(int fd, struct l_settings *settings)
{
	char *res;
	size_t res_len;
	int err = 0;

	res = l_settings_to_data(settings, &res_len);

	if (ftruncate(fd, 0) == -1) {
		err = -errno;
		goto failure;
	}

	if (pwrite(fd, res, res_len, 0) < 0)
		err = -errno;

failure:
	l_free(res);

	return err;
}

int storage_open(const char *pathname)
{
	struct l_settings *settings;
	int fd, err;

	err = make_dirs(pathname, S_IRUSR | S_IWUSR | S_IXUSR);
	if (err < 0)
		return err;

	fd = open(pathname, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0)
		return -errno;

	settings = l_settings_new();
	/* Ignore error if file doesn't exists */
	l_settings_load_from_file(settings, pathname);

	if (!storage_list)
		storage_list = l_hashmap_new();

	l_hashmap_insert(storage_list, L_INT_TO_PTR(fd), settings);

	return fd;
}

int storage_close(int fd)
{
	struct l_settings *settings;

	settings = l_hashmap_remove(storage_list, L_INT_TO_PTR(fd));
	if(!settings)
		return -ENOENT;

	l_settings_free(settings);

	return close(fd);
}

void storage_foreach_slave(int fd, storage_foreach_slave_t func,
						void *user_data)
{
	struct l_settings *settings;
	char **groups;
	char *name;
	char *url;
	int i;
	int id;

	settings = l_hashmap_lookup(storage_list, L_INT_TO_PTR(fd));
	if (!settings)
		return;

	groups = l_settings_get_groups(settings);

	for (i = 0; groups[i] != NULL; i++) {
		if (l_settings_get_int(settings, groups[i], "Id", &id) == false)
			continue;

		name = l_settings_get_string(settings, groups[i], "Name");
		if (!name)
			continue;

		url = l_settings_get_string(settings, groups[i], "URL");
		if (url)
			func(groups[i], id, name, url, user_data);

		l_free(url);
		l_free(name);
	}

	l_strfreev(groups);
}

void storage_foreach_source(int fd, storage_foreach_source_t func,
						void *user_data)
{
	struct l_settings *settings;
	char **groups;
	char *name;
	char *type;
	char *unit;
	int interval = 1000;
	int i;

	settings = l_hashmap_lookup(storage_list, L_INT_TO_PTR(fd));
	if (!settings)
		return;

	groups = l_settings_get_groups(settings);

	for (i = 0; groups[i] != NULL; i++) {
		l_settings_get_int(settings, groups[i],
				       "PollingInterval", &interval);

		/* Local name: alias */
		name = l_settings_get_string(settings, groups[i], "Name");

		/* Signature based on D-Bus data types */
		type = l_settings_get_string(settings, groups[i], "Type");

		/* Unit (symbol) based on IEEE 210.1 */
		unit = l_settings_get_string(settings, groups[i], "Unit");

		if (name && type && unit)
				func(groups[i], name, type,
				     unit, interval, user_data);
		else
			l_error("storage: invalid source (%s)", groups[i]);

		/* Always release memory */
		l_free(name);
		l_free(type);
		l_free(unit);
	}

	l_strfreev(groups);
}

int storage_write_key_string(int fd, const char *group,
			     const char *key, const char *value)
{
	struct l_settings *settings;

	settings = l_hashmap_lookup(storage_list, L_INT_TO_PTR(fd));
	if (!settings)
		return -EIO;

	if (l_settings_set_string(settings, group, key, value) == false)
		return -EINVAL;

	return save_settings(fd, settings);
}

char *storage_read_key_string(int fd, const char *group, const char *key)
{
	struct l_settings *settings;

	settings = l_hashmap_lookup(storage_list, L_INT_TO_PTR(fd));
	if (!settings)
		return NULL;

	return l_settings_get_string(settings, group, key);
}

int storage_write_key_int(int fd, const char *group, const char *key, int value)
{
	struct l_settings *settings;

	settings = l_hashmap_lookup(storage_list, L_INT_TO_PTR(fd));
	if (!settings)
		return -EINVAL;

	if (l_settings_set_int(settings, group, key, value) == false)
		return -EINVAL;

	return save_settings(fd, settings);
}

int storage_read_key_int(int fd, const char *group, const char *key, int *value)
{
	struct l_settings *settings;

	settings = l_hashmap_lookup(storage_list, L_INT_TO_PTR(fd));
	if (!settings)
		return -EINVAL;

	return l_settings_get_int(settings, group, key, value);
}

int storage_read_key_float(int fd, const char *group, const char *key,
			   float *value)
{
	struct l_settings *settings;

	settings = l_hashmap_lookup(storage_list, L_INT_TO_PTR(fd));
	if (!settings)
		return -EINVAL;

	return l_settings_get_float(settings, group, key, value);
}

int storage_read_key_bool(int fd, const char *group, const char *key,
			  uint8_t *value)
{
	struct l_settings *settings;
	bool result;

	settings = l_hashmap_lookup(storage_list, L_INT_TO_PTR(fd));
	if (!settings)
		return -EINVAL;

	if (!l_settings_get_bool(settings, group, key, &result))
		return 0;

	*value = result;

	return 1;
}

int storage_read_key_int64(int fd, const char *group, const char *key,
			   int64_t *value)
{
	struct l_settings *settings;

	settings = l_hashmap_lookup(storage_list, L_INT_TO_PTR(fd));
	if (!settings)
		return -EINVAL;

	return l_settings_get_int64(settings, group, key, value);
}

int storage_read_key_uint(int fd, const char *group, const char *key,
			  uint32_t *value)
{
	struct l_settings *settings;

	settings = l_hashmap_lookup(storage_list, L_INT_TO_PTR(fd));
	if (!settings)
		return -EINVAL;

	return l_settings_get_uint(settings, group, key, value);
}

int storage_read_key_uint64(int fd, const char *group, const char *key,
			    uint64_t *value)
{
	struct l_settings *settings;

	settings = l_hashmap_lookup(storage_list, L_INT_TO_PTR(fd));
	if (!settings)
		return -EINVAL;

	return l_settings_get_uint64(settings, group, key, value);
}

int storage_remove_group(int fd, const char *group)
{
	struct l_settings *settings;

	settings = l_hashmap_lookup(storage_list, L_INT_TO_PTR(fd));
	if (!settings)
		return -EINVAL;

	if (l_settings_remove_group(settings, group) == false)
		return -EINVAL;

	return save_settings(fd, settings);
}

bool storage_has_unit(int fd, const char *group, const char *key)
{
	struct l_settings *settings;

	settings = l_hashmap_lookup(storage_list, L_INT_TO_PTR(fd));
	if (!settings)
		return false;

	return l_settings_has_key(settings, group, key);
}

char **get_data_item_groups(int fd)
{
	struct l_settings *settings;
	char **all_groups;
	int group_index;
	int aux;

	settings = l_hashmap_lookup(storage_list, L_INT_TO_PTR(fd));

	if (!settings)
		return NULL;

	all_groups = l_settings_get_groups(settings);

	for (group_index = 0; all_groups[group_index] != NULL; group_index++) {
		aux = strncmp(all_groups[group_index], DATA_ITEM_GROUP,
						strlen(DATA_ITEM_GROUP));
		if (aux)
			l_settings_remove_group(settings,
						all_groups[group_index]);
	}

	l_strfreev(all_groups);

	return l_settings_get_groups(settings);
}
