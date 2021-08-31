#include <debug.h>
#include <libfdt.h>

#include "fastboot.h"

int fit_boot(void *fdt, unsigned sz)
{
	int ret = 0;
	int len = 0;
	int offset = 0;
	char tmp[32] = { 0 };
	const char *val;

	dprintf(INFO, "=== Trying to boot FIT image ===\n");
	ret = fdt_check_header(fdt);
	if (ret != 0) {
		dprintf(INFO, "fit: invalid fdt: %d\n", ret);
		return ret;
	}
	dprintf(INFO, "fit: detected correct fdt!\n");

	val = fdt_getprop(fdt, 0, "creator", &len);
	if (len <= 0 ) {
		dprintf(INFO, "fit: creator property is invalid. Not a FIT image? ret: %d\n", len);
		return -1;
	}
	dprintf(INFO, "fit: creator: %s\n", val);

	val = fdt_getprop(fdt, 0, "description", &len);
	dprintf(INFO, "fit: description: %s\n", val);

	offset = fdt_subnode_offset_namelen(fdt, 0, "configurations", strlen("configurations"));
	if (offset < 0) {
		dprintf(INFO, "fit: error finding configurations node: %d\n", offset);
		return -1;
	}

	val = fdt_getprop(fdt, offset, "default", &len);
	dprintf(INFO, "fit: default config: %s\n", val);


	fastboot_okay("");
	//fastboot_stop();
	return 0;
}
