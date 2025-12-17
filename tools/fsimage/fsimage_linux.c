#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <linux/libfdt.h>
#include <linux/kconfig.h>
#include <command.h>
#include <linux/compiler_attributes.h>
#include "linux_helpers.h"
#include "../../board/F+S/common/fs_image_common.h"
#include "../../include/imx_container.h"

struct fs_header_v1_0 *fs_image_find(struct fs_header_v1_0 *fsh,
		const char *type,
		const char *descr,
		struct index_info *idx_info);

int do_fsimage_list(int argc, char * const argv[]);
int do_fsimage_load(int argc, char * const argv[]);
int do_fsimage_save(int argc, char * const argv[]);

#define MAX_NBOOT_SIZE (4 * 1024 * 1024)
#define MAX_BOARD_CFG_SIZE (2 * 1024)

char saved_nboot_buffer[MAX_NBOOT_SIZE];
char nboot_buffer[MAX_NBOOT_SIZE];
static char saved_board_cfg_buffer[MAX_BOARD_CFG_SIZE];

static int load_saved_nboot(void)
{
	size_t bytes_read;
	const char *fname = "/dev/mmcblk0boot1";
	FILE *hwpart = fopen(fname, "ro");

	if (!hwpart) {
		fprintf(stderr, "Error opening %s, exiting...\n", fname);
		return -ENOENT;
	}
	
	bytes_read = fread(saved_nboot_buffer + FSH_SIZE, 1,
			   MAX_NBOOT_SIZE - FSH_SIZE, hwpart);
	if (!bytes_read) {
		fprintf(stderr, "Error reading from %s, exiting...\n", fname);
		fclose(hwpart);
		return -EINVAL;
	}
	fclose(hwpart);

	return 0;
}

//### TODO: Statt sich an den Containern entlang zu hangeln, sollte man
//### einfach gezielt das BOARD-ID Image über die F&S-Header-Kette suchen.
int extract_board_config(void)
{
	u64 search = (u64)(saved_nboot_buffer + 0x40);
	int i;
	char board_id[MAX_DESCR_LEN + 1];
	char *point;
	struct fs_header_v1_0* fsh;
	struct fs_header_v1_0 *cfg_fsh;
	void *cfg_img;
	struct index_info idx_info;

	/* Search the second container header, i.e. the BOARD-INFO container */
	for (i = 0; i < 2; i++) {
		do {
			/* Next container is 1K aligned */
			search += 0x400;
		} while (!valid_container_hdr((struct container_hdr *)search));
	}

	/* Get BOARD-ID which is two F&S headers before */
	fsh = (struct fs_header_v1_0 *)(search - 2 * FSH_SIZE);
	strncpy(board_id, fsh->param.descr, MAX_DESCR_LEN);
	board_id[MAX_DESCR_LEN] = 0;

	/* Strip board revision */
	point = strchr(board_id, '.');
	if (point)
		*point = 0;

	/* Find the BOARD-CFG header for this BOARD-ID in index */
	cfg_fsh = fs_image_find(++fsh , "BOARD-CFG", board_id, &idx_info);

	/* Find the real BOARD-CFG image */
	cfg_img = fs_image_find_cfg_fdt_idx(&idx_info);

	/* Copy header and image to saved_board_cfg_buffer[] */
	memcpy(saved_board_cfg_buffer, (void *)cfg_fsh, FSH_SIZE);
	memcpy(saved_board_cfg_buffer + FSH_SIZE, cfg_img,
	       fs_image_get_size(cfg_fsh, false));

	/* Check if BOARD-CFG is valid */
	if (!fs_image_is_ocram_cfg_valid()) {
		fprintf(stderr, "Error, no valid BOARD-CFG found.\n");
		return -EINVAL;
	}

	/*
	 * Set the current board_id name and the compare_id that is used in
	 * fs_image_find_board_cfg().
	 */
	fs_image_set_board_id_from_cfg();

	return 0;
}


/* ------------- Functions needed to avoid large libraries ----------------- */

u32 fdt_getprop_u32_default_node(const void *fdt, int off, int cell,
				const char *prop, const u32 dflt)
{
	const fdt32_t *val;
	int len;

	val = fdt_getprop(fdt, off, prop, &len);

	/* Check if property exists */
	if (!val)
		return dflt;

	/* Check if property is long enough */
	if (len < ((cell + 1) * sizeof(uint32_t)))
		return dflt;

	return fdt32_to_cpu(*val);
}


/* ------------- Functions that differ from U-Boot ------------------------- */

static char arch[64];

/* Return the F&S architecture */
const char *fs_image_get_arch(void)
{
	const char *fname = "/sys/bdinfo/arch";
	size_t bytes_read;
	FILE *fp = fopen(fname, "ro");

	if (!fp) {
		fprintf(stderr, "Error: Cannot open %s!\n", fname);
		return NULL;
	}

	bytes_read = fread(arch, 1, 64, fp);
	if (!bytes_read) {
		fprintf(stderr, "Error: Cannot read %s!\n", fname);
		fclose(fp);	
		return NULL;
	}

	arch[bytes_read - 1] = '\0';
	fclose(fp);	

	return arch;
}

/* Return the address of the board configuration */
void *fs_image_get_cfg_addr(void)
{
	return (void *)saved_board_cfg_buffer;
}


int fs_image_get_start_copy(void)
{
	int start_copy = 1;

	printf("TODO: Cannot determine SPL start copy, assuming Primary\n");
	printf("Booted from %s SPL, so starting with copy %d\n", \
	       start_copy ? "Primary" : "Secondary", start_copy);

	return start_copy;
}

int fs_image_get_start_copy_uboot(void)
{
	int start_copy = 1;

	printf("TODO: Cannot determine UBOOT start copy, assuming Primary\n");
	printf("Booted from %s UBOOT, so starting with copy %d\n", \
	       start_copy ? "Primary" : "Secondary", start_copy);

	return start_copy;
}

unsigned int fuse_read(int bank, int word, uint32_t *buf)
{
	const char *fname = "/sys/bus/nvmem/devices/fsb_s400_fuse0/nvmem";
	FILE *nvmem = fopen(fname, "rb");

	if (!nvmem)
		return -EIO;

	fseek(nvmem, (bank * 8 + word) * 4, SEEK_SET);
	fread(&buf, 4, 1, nvmem);
	fclose(nvmem);

	return 0;
}


/* ------------- Linux command line handling ------------------------------- */

const char usage[] =
	"Usage:\n"
	"fsimage list <file>]\n"
	"    - List the content of the F&S image <file>\n"
	"fsimage load [-f] [uboot | nboot] <file>\n"
	"    - Verify the current NBoot or U-Boot and store in <file>\n"
	"fsimage save [-f] [-e <n>] [-b <n>] <file>\n"
	"    - Save the F&S image at the right place (NBoot, U-Boot)\n"
	"\n";


int do_fsimage(int argc, char *argv[])
{
	/* Drop argv[0] ("fsimage") */
	argc--;
	argv++;

	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[0], "list"))
		return do_fsimage_list(argc, argv);

	if (!strcmp(argv[0], "load"))
		return do_fsimage_load(argc, argv);

	if (!strcmp(argv[0], "save"))
		return do_fsimage_save(argc, argv);

	return CMD_RET_USAGE;
}

int main(int argc, char *argv[])
{
	int status;

	/* Load NBoot from flash, store in saved_nboot_buffer[] */
	if (load_saved_nboot() < 0)
		return 1;

	/* Extract BOARD-CFG from NBoot, store in saved_board_cfg_buffer[] */
	if (extract_board_config() < 0)
		return 1;

	status = do_fsimage(argc, argv);
	if (status == CMD_RET_USAGE) {
		fprintf(stderr, "%s\n", usage);
		return 1;
	}

	return status == CMD_RET_FAILURE;
}
