// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2022 Nikita Travkin <nikita@trvn.ru> */

#include <stdlib.h>
#include <debug.h>
#include <lib/fs.h>
#include <target.h>
#include <display_menu.h>

#include <lk2nd/boot.h>

#include "boot.h"

/* app/aboot/aboot.c */
extern void cmd_boot(const char *arg, void *data, unsigned sz);

/* platform/msm_shared/display_menu.c */
extern struct select_msg_info msg_info;

static char abootimg_hdr[] = "ANDROID!";

struct action_abootimg_data {
	char path[129];
};

static enum lk2nd_boot_aboot_action action_abootimg(void *data)
{
	struct action_abootimg_data *adata = data;
	void *buf = target_get_scratch_address();
	size_t sz = target_get_max_flash_size();
	int ret;

	dprintf(INFO, "abootimg: Booting %s\n", adata->path);

	/*
	 * HACK: Init the fastboot menu mutex to avoid assert,
	 * then fake a key_detect thread exit notfication.
	 */
	msg_lock_init();
	msg_info.info.rel_exit = true;

	ret = fs_load_file(adata->path, buf, sz);
	if (ret > 0)
		cmd_boot(NULL, buf, sz);

	dprintf(CRITICAL, "abootimg: Boot failed!\n");

	return LK2ND_ABOOT_ACTION_FASTBOOT;
}

void action_abootimg_register(char *root)
{
	struct action_abootimg_data *adata;
	struct dirhandle *dirh;
	struct filehandle *fileh;
	struct dirent dirent;
	char path[129], tmp[150];
	int ret, priority;

	ret = fs_open_dir(root, &dirh);
	if (ret < 0) {
		dprintf(INFO, "fs_open_dir ret = %d\n", ret);
		return;
	}

	while (fs_read_dir(dirh, &dirent) >= 0) {
		if (!file_extension_is(dirent.name, ".img"))
			continue;

		snprintf(path, sizeof(path), "%s/%s", root, dirent.name);

		ret = fs_open_file(path, &fileh);
		if (ret < 0) {
			dprintf(INFO, "fs_open_file ret = %d\n", ret);
			continue;
		}

		ret = fs_read_file(fileh, &tmp, 0, 8);
		if (ret == 8 && !strncmp(tmp, abootimg_hdr, 8)) {
			dprintf(INFO, "abootimg: Found %s\n", path);

			if (!strcmp(dirent.name, "boot.img"))
				priority = 50;
			else if (!strcmp(dirent.name, "lk2nd.img"))
				priority = -1;
			else
				priority = 25;

			adata = malloc(sizeof(*adata));
			strcpy(adata->path, path);
			snprintf(tmp, sizeof(tmp), "Image: %s", path);

			lk2nd_boot_add_action(tmp, priority, action_abootimg,
					(void*)adata);
		}

		ret = fs_close_file(fileh);
	}
	fs_close_dir(dirh);
}

