// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2022 Nikita Travkin <nikita@trvn.ru> */

#include <stdlib.h>
#include <debug.h>
#include <list.h>
#include <lib/bio.h>
#include <lib/fs.h>

#include <lk2nd/boot.h>

#include "boot.h"

/**
 * dump_devices() - Log a table with all block devices.
 */
void lk2nd_boot_dump_devices()
{
	struct bdev_struct *bdevs = bio_get_bdevs();
	bdev_t *entry;

	dprintf(INFO, "block devices:\n");
	dprintf(INFO, " | dev    | label      | size       | Sub |\n");
	list_for_every_entry(&bdevs->list, entry, bdev_t, node) {
		dprintf(INFO, " | %-6s | %-10s | %6lld %s | %-3s |\n",
			entry->name,
			(entry->label ? entry->label : ""),
			entry->size / (entry->size > 1024 * 1024 ? 1024*1024 : 1024),
			(entry->size > 1024 * 1024 ? "MiB" : "KiB"),
			(entry->is_subdev ? "Yes" : "")
			);
	}
}

/**
 * print_file_tree(char *root, char *prefix) - Pretty print the fs tree.
 * @root:   path to recursively print.
 * @prefix: Prefix for the tree, user should put "" here.
 */
void print_file_tree(char *root, char *prefix)
{
	struct filehandle *fileh;
	struct dirhandle *dirh;
	struct dirent dirent, next;
	struct file_stat stat;
	char path[129], pref[128];
	int ret, tmp;

	ret = fs_open_dir(root, &dirh);
	if (ret < 0) {
		dprintf(INFO, "fs_open_dir ret = %d\n", ret);
		return;
	}

	tmp = fs_read_dir(dirh, &dirent);

	while (tmp >= 0) {
		if (!strcmp(dirent.name, ".") || !strcmp(dirent.name, "..") || *dirent.name == '\0') {
			tmp = fs_read_dir(dirh, &dirent);
			continue;
		}

		snprintf(path, sizeof(path), "%s/%s", root, dirent.name);

		ret = fs_open_file(path, &fileh);
		if (ret < 0) {
			dprintf(INFO, "fs_open_file ret = %d\n", ret);
			tmp = fs_read_dir(dirh, &dirent);
			continue;
		}
		ret = fs_stat_file(fileh, &stat);
		ret = fs_close_file(fileh);

		tmp = fs_read_dir(dirh, &next);

		dprintf(INFO, "%s%s-- %s%s  [%lld %s]\n", prefix, (tmp < 0 ? "`" : "|"),
				dirent.name, (stat.is_dir ? "/" : ""),
				stat.size / (stat.size > 1024 * 1024 ? 1024*1024 : 1024),
				(stat.size > 1024 * 1024 ? "MiB" : "KiB"));

		strcat(path, "/");
		if (stat.is_dir) {
			strcpy(pref, prefix);
			strcat(pref, (tmp < 0 ? "    " : "|   "));
			print_file_tree(path, pref);
		}
		dirent = next;
	}
	fs_close_dir(dirh);
}

/**
 * file_extension_is() - Chech if the extension of a given name matches
 * @name:      File name.
 * @extension: Extension to check for.
 *
 * Returns: True if the extension matches.
 */
bool file_extension_is(char *name, char *extension)
{
	char *dot = strrchr(name, '.');

	if (!dot || (dot && strcmp(dot, extension)))
		return false;

	return true;
}
