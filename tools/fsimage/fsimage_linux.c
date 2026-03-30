#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <linux/libfdt.h>
#include <linux/kconfig.h>
#include <command.h>
#include <ctype.h>			/* tolower() */
#include <linux/compiler_attributes.h>
#include "linux_helpers.h"
#include "../../board/F+S/common/fs_image_common.h"
#include "../../include/imx_container.h"

struct fs_header_v1_0 *fs_image_find(struct fs_header_v1_0 *fsh,
		const char *type,
		const char *descr,
		struct index_info *idx_info);

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

// ### from fsimage.c
extern char saved_nboot_buffer[1024*1024*4];
extern char nboot_buffer[1024*1024*4];

#ifdef DEBUG
#define debug(fmt, ...) fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define debug(fmt, ...) do {} while (0)
#endif

#define FIT_DATA_SIZE_PROP	"data-size"
/**
 * Get 'data-size' property from a given image node.
 *
 * @fit: pointer to the FIT image header
 * @noffset: component image node offset
 * @data_size: holds the data-size property
 *
 * returns:
 *     0, on success
 *     -ENOENT if the property could not be found
 */
int fit_image_get_data_size(const void *fit, int noffset, int *data_size)
{
	const fdt32_t *val;

	val = fdt_getprop(fit, noffset, FIT_DATA_SIZE_PROP, NULL);
	if (!val)
		return -ENOENT;

	*data_size = fdt32_to_cpu(*val);

	return 0;
}

#define FIT_DATA_OFFSET_PROP	"data-offset"
/**
 * Get 'data-offset' property from a given image node.
 *
 * @fit: pointer to the FIT image header
 * @noffset: component image node offset
 * @data_offset: holds the data-offset property
 *
 * returns:
 *     0, on success
 *     -ENOENT if the property could not be found
 */
int fit_image_get_data_offset(const void *fit, int noffset, int *data_offset)
{
	const fdt32_t *val;

	val = fdt_getprop(fit, noffset, FIT_DATA_OFFSET_PROP, NULL);
	if (!val)
		return -ENOENT;

	*data_offset = fdt32_to_cpu(*val);

	return 0;
}

/**
 * Get 'data-position' property from a given image node.
 *
 * @fit: pointer to the FIT image header
 * @noffset: component image node offset
 * @data_position: holds the data-position property
 *
 * returns:
 *     0, on success
 *     -ENOENT if the property could not be found
 */

#define FIT_DATA_POSITION_PROP	"data-position"
int fit_image_get_data_position(const void *fit, int noffset,
				int *data_position)
{
	const fdt32_t *val;

	val = fdt_getprop(fit, noffset, FIT_DATA_POSITION_PROP, NULL);
	if (!val)
		return -ENOENT;

	*data_position = fdt32_to_cpu(*val);

	return 0;
}

/**
 * fit_get_name - get FIT node name
 * @fit: pointer to the FIT format image header
 *
 * returns:
 *     NULL, on error
 *     pointer to node name, on success
 */
static inline const char *fit_get_name(const void *fit_hdr,
		int noffset, int *len)
{
	return fdt_get_name(fit_hdr, noffset, len);
}

static void fit_get_debug(const void *fit, int noffset,
		char *prop_name, int err)
{
	debug("Can't get '%s' property from FIT 0x%08lx, node: offset %d, name %s (%s)\n",
	      prop_name, (ulong)fit, noffset, fit_get_name(fit, noffset, NULL),
	      fdt_strerror(err));
}

#define FIT_DATA_PROP		"data"
/**
 * fit_image_get_data - get data property and its size for a given component image node
 * @fit: pointer to the FIT format image header
 * @noffset: component image node offset
 * @data: double pointer to void, will hold data property's data address
 * @size: pointer to size_t, will hold data property's data size
 *
 * fit_image_get_data() finds data property in a given component image node.
 * If the property is found its data start address and size are returned to
 * the caller.
 *
 * returns:
 *     0, on success
 *     -1, on failure
 */
int fit_image_get_data(const void *fit, int noffset,
		       const void **data, size_t *size)
{
	int len;

	*data = fdt_getprop(fit, noffset, FIT_DATA_PROP, &len);
	if (*data == NULL) {
		fit_get_debug(fit, noffset, FIT_DATA_PROP, len);
		*size = 0;
		return -1;
	}

	*size = len;
	return 0;
}

// Digest is for compatibility between nboot and linux function signature,
// always NULL when called and unused for Linux implementatiom.
ulong parse_loadaddr(char *filename, void *digest) {
    FILE *file = fopen(filename, "ro");
	if(!file) {
		printf("Error opening %s, exiting...\n", filename);
		return -ENOENT;
	}
	size_t bytes_read = fread(nboot_buffer, 1, 4*1024*1024, file);
	if(!bytes_read) {
		printf("Error reading data from %s, exiting...\n", filename);
		return -EINVAL;
	}
	fclose(file);
	return (ulong)nboot_buffer;
}

ulong get_loadaddr(void){
    return (ulong)saved_nboot_buffer;
}

unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base) {
	return strtoul(cp, endp, base);
}

long simple_strtol(const char *cp, char **endp, unsigned int base) {
	return strtol(cp, endp, base);
}

//TODO: DD klappt das??
#define cpu_to_fdt32(x) __builtin_bswap32(x)

#if 0 //### kann vermutlich weg
/**
 * fdt_find_and_setprop: Find a node and set it's property
 *
 * @fdt: ptr to device tree
 * @node: path of node
 * @prop: property name
 * @val: ptr to new value
 * @len: length of new property value
 * @create: flag to create the property if it doesn't exist
 *
 * Convenience function to directly set a property given the path to the node.
 */
int fdt_find_and_setprop(void *fdt, const char *node, const char *prop,
			 const void *val, int len, int create)
{
	int nodeoff = fdt_path_offset(fdt, node);

	if (nodeoff < 0)
		return nodeoff;

	if ((!create) && (fdt_get_property(fdt, nodeoff, prop, NULL) == NULL))
		return 0; /* create flag not set; so exit quietly */

	return fdt_setprop(fdt, nodeoff, prop, val, len);
}
#endif //###

enum boot_stage_type {
	BT_STAGE_PRIMARY = 0x6,
	BT_STAGE_SECONDARY = 0x9,
	BT_STAGE_RECOVERY = 0xa,
	BT_STAGE_USB = 0x5,
};

int get_bootrom_bootstage(u32 *bstage)
{
	return -ENODEV;
}

int get_container_size(ulong addr, u16 *header_length)
{
	struct container_hdr *phdr;
	struct boot_img_t *img_entry;
	struct signature_block_hdr *sign_hdr;
	u8 i = 0;
	u32 max_offset = 0, img_end;

	phdr = (struct container_hdr *)addr;
	if (!valid_container_hdr(phdr)) {
		debug("Wrong container header\n");
		return -EFAULT;
	}

	max_offset = phdr->length_lsb + (phdr->length_msb << 8);
	if (header_length)
		*header_length = max_offset;

	img_entry = (struct boot_img_t *)(addr + sizeof(struct container_hdr));
	for (i = 0; i < phdr->num_images; i++) {
		img_end = img_entry->offset + img_entry->size;
		if (img_end > max_offset)
			max_offset = img_end;

		debug("img[%u], end = 0x%x\n", i, img_end);

		img_entry++;
	}

	if (phdr->sig_blk_offset != 0) {
		sign_hdr = (struct signature_block_hdr *)(addr + phdr->sig_blk_offset);
		u16 len = sign_hdr->length_lsb + (sign_hdr->length_msb << 8);

		if (phdr->sig_blk_offset + len > max_offset)
			max_offset = phdr->sig_blk_offset + len;

		debug("sigblk, end = 0x%x\n", phdr->sig_blk_offset + len);
	}

	return max_offset;
}

int confirm_yesno(void) {
	char input[8];
	if(!fgets(input, sizeof(input), stdin)) {
		return 0;
	}
	input[strcspn(input, "\n")] = '\0';
	for(char *p = input; *p; ++p) {
		*p = tolower((unsigned char) *p);
	}
	if((strcmp(input, "y") == 0) || (strcmp(input, "yes") == 0)) {
		return 1;
	}
	return 0;
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

#ifdef CONFIG_IMX_HAB

/* ### TODO: Use own authentication function */
int imx_hab_authenticate_image(uint32_t ddr_start, uint32_t image_size,
			       uint32_t ivt_offset)
{
	return -EINVAL;
}

/* ### TODO: Read secure boot fuse from fuse bank */
bool imx_hab_is_enabled(void)
{
	return false;
}

#endif /* CONFIG_IMX_HAB */


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
		return fs_image_do_list(argc, argv);

	if (!strcmp(argv[0], "load"))
		return fs_image_do_load(argc, argv);

	if (!strcmp(argv[0], "save"))
		return fs_image_do_save(argc, argv);

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
