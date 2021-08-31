#include <debug.h>
#include <libfdt.h>
#include <lk2nd.h>
#include <stdlib.h>
#include <platform.h>

#include <platform/iomap.h>

#include "fastboot.h"

/*
 *
 * to generate the FIT image: (-E is important for offset mode)

mkimage -f auto \
	-d vmlinuz \
	-i initramfs \
	-A "arm64" \
	$(for f in dtb/msm8916*; do printf ' -b %s' "$f"; done) \
	$(for f in dtb/apq8016*; do printf ' -b %s' "$f"; done) \
	-E \
	lk2nd.fit

FIXME questions:
 * how to pass _addr stuff???
     - maybe keep hardcoded?
 * how to pass cmdline???
     - can abuse description property of the kernel node
 * how to select -modem dtb???
     - ???

 * most of ^^^ can be solved by providing custom FIT source with own
 * properties like cmdline in config nodes but then need to generate those?

 */


int fit_boot(void *fdt, unsigned sz)
{
	int ret = 0;
	int len = 0;
	int offset = 0, off_conf = 0, off_img = 0;
	const char *val;
	const uint32_t *ival;

	void *kernel_addr = VA((addr_t)(ABOOT_FORCE_KERNEL64_ADDR));
	void *ramdisk_addr = VA((addr_t)(ABOOT_FORCE_RAMDISK_ADDR));
	void *tags_addr = VA((addr_t)(ABOOT_FORCE_TAGS_ADDR));

	/* FIXME */
	kernel_addr =  0x80080000;
	ramdisk_addr = 0x82000000;
	tags_addr =    0x81e00000;

	unsigned char *kernel_raw = NULL;
	unsigned char *ramdisk_raw = NULL;
	unsigned char *dtb_raw = NULL;
	uint32_t kernel_raw_size = 0;
	uint32_t ramdisk_size = 0;
	uint32_t dtb_size = 0;

	/* FIXME */
	const char* cmdline = "earlycon console=tty0 console=ttyMSM0,115200 PMOS_NO_OUTPUT_REDIRECT";


	dprintf(INFO, "=== Trying to boot FIT image ===\n");

	if (!lk2nd_dev.dtb) {
		dprintf(INFO, "fit: DTB name is not given\n");
		return -1;
	}

	ret = fdt_check_header(fdt);
	if (ret != 0) {
		dprintf(INFO, "fit: invalid fdt: %d\n", ret);
		return ret;
	}
	dprintf(INFO, "fit: fdt detected!\n");

	dprintf(INFO, "fit: fdt size=%d\n", fdt_totalsize(fdt));

	/* FIXME: is fdt size guaranteed to be 4byte aligned? */
	kernel_raw = ramdisk_raw = dtb_raw = (unsigned char *)fdt + fdt_totalsize(fdt);

	val = fdt_getprop(fdt, 0, "creator", &len);
	if (len <= 0 ) {
		dprintf(INFO, "fit: creator property is invalid. Not a FIT image? ret: %d\n", len);
		return -1;
	}
	dprintf(INFO, "fit: creator: %s\n", val);

	val = fdt_getprop(fdt, 0, "description", &len);
	dprintf(INFO, "fit: description: %s\n", val);

	offset = fdt_subnode_offset(fdt, 0, "configurations");
	if (offset < 0) {
		dprintf(INFO, "fit: error finding configurations node: %d\n", offset);
		return -1;
	}

	/* need to respect the \0 at the end of the property, thus strlen+1 */
	offset = fdt_node_offset_by_prop_value(fdt, offset, "description", lk2nd_dev.dtb, strlen(lk2nd_dev.dtb)+1);
	if (offset < 0) {
		dprintf(INFO, "fit: error selecting configuration: %d\n", offset);
		return -1;
	}

	val = fdt_getprop(fdt, offset, "description", &len);
	dprintf(INFO, "fit: Found config: %s\n", val);

	off_conf = offset;
	off_img = fdt_subnode_offset(fdt, 0, "images");


	/*
	 * FIXME: fail with non-appended FIT type or implement it
	 */

	/* kernel */
	val = fdt_getprop(fdt, off_conf, "kernel", &len);
	offset = fdt_subnode_offset(fdt, off_img, val);

	val = fdt_getprop(fdt, offset, "type", &len);
	dprintf(INFO, "fit: Kernel description: %s\n", val);

	ival = fdt_getprop(fdt, offset, "data-offset", &len);
	kernel_raw += fdt32_to_cpu(*ival);

	ival = fdt_getprop(fdt, offset, "data-size", &len);
	kernel_raw_size += fdt32_to_cpu(*ival);

	/* ramdisk */
	val = fdt_getprop(fdt, off_conf, "ramdisk", &len);
	offset = fdt_subnode_offset(fdt, off_img, val);

	ival = fdt_getprop(fdt, offset, "data-offset", &len);
	ramdisk_raw += fdt32_to_cpu(*ival);

	ival = fdt_getprop(fdt, offset, "data-size", &len);
	ramdisk_size += fdt32_to_cpu(*ival);

	/* fdt */
	val = fdt_getprop(fdt, off_conf, "fdt", &len);
	offset = fdt_subnode_offset(fdt, off_img, val);

	ival = fdt_getprop(fdt, offset, "data-offset", &len);
	dtb_raw += fdt32_to_cpu(*ival);

	ival = fdt_getprop(fdt, offset, "data-size", &len);
	dtb_size += fdt32_to_cpu(*ival);

	if(is_gzip_package(kernel_raw, kernel_raw_size)) {
		dprintf(INFO, "fit: decompressing kernel, size: 0x%x\n", kernel_raw_size);
		ret = decompress(kernel_raw, kernel_raw_size,
				 kernel_addr, ramdisk_addr - kernel_addr, NULL, NULL);
		if(ret) {
			dprintf(INFO, "kernel decompression failed: %d\n", ret);
			return -1;
		}
	} else {
		memmove(kernel_addr, kernel_raw, kernel_raw_size);
	}

	memmove(ramdisk_addr, ramdisk_raw, ramdisk_size);
	memmove(tags_addr, dtb_raw, dtb_size);

	fastboot_okay("");
	fastboot_stop();

	boot_linux(kernel_addr, tags_addr, (const char *)cmdline, board_machtype(), ramdisk_addr, ramdisk_size);

	return 0;
}
