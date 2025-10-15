#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "../../board/F+S/common/fs_image_common.h"
#include "../../board/F+S/common/linux_helpers.h"
#include "../../include/imx_container.h"

struct fs_header_v1_0 *fs_image_find(struct fs_header_v1_0 *fsh,
		const char *type,
		const char *descr,
		struct index_info *idx_info);

int do_fsimage_list(int argc, char * const argv[]);
int do_fsimage_load(int argc, char * const argv[]);
int do_fsimage_save(int argc, char * const argv[]);
int fs_image_check_saved_cfg(void);

char saved_nboot_buffer[1024*1024*4];
char nboot_buffer[1024*1024*4];
char saved_board_cfg_buffer[4*1024*1024];

int load_saved_nboot(void) {
	FILE *hwpart = fopen("/dev/mmcblk0boot1", "ro");
	if(!hwpart) {
		printf("Error opening %s, exiting...\n", "/dev/mmcblk0boot1");
		return -ENOENT;
	}
	size_t bytes_read = fread(saved_nboot_buffer + 0x40, 1, 4*1024*1024 - 0x40, hwpart);
	if(!bytes_read) {
		printf("Error reading data from %s, exiting... (%lx)\n", "/dev/mmcblk0boot1", bytes_read);
		return -EINVAL;
	}
	fclose(hwpart);

    return 0;
}

void extract_board_config(void){
	u64 search = (u64)(saved_nboot_buffer + 0x40);
	for(int i = 0; i < 2; i++) {
		do {
			search += 0x400;
		} while(!valid_container_hdr((struct container_hdr *)search));
	}

	char *fert = ((struct fs_header_v1_0*)(search - 0x80))->param.descr;
	char *point = strchr(fert, '.');
	int diff = point - fert;
	char fert_no_rev[256];
	strncpy(fert_no_rev, fert, diff);
	fert_no_rev[diff] = 0;

	struct index_info idx_info;
	struct fs_header_v1_0* fsh_cfg = fs_image_find((struct fs_header_v1_0 *)(search - 0x40) , "BOARD-CFG", fert_no_rev, &idx_info);
	void * test = fs_image_find_cfg_fdt_idx(&idx_info);

	memcpy(saved_board_cfg_buffer, (void *)fsh_cfg, 0x40);
	memcpy(saved_board_cfg_buffer + 0x40, test, fs_image_get_size(fsh_cfg, false));
}

struct cmd_tbl_fsimage {
    char* name;
    int (*cmd)(int, char**);
};

//const struct cmd_tbl_fsimage cmd_fsimage_sub[] = {
//    { "list", do_fsimage_list },
//    { "load", do_fsimage_load },
//    { "save", do_fsimage_save }
//};

const struct cmd_tbl_fsimage *find_cmd_tbl(const char *cmd, const struct cmd_tbl_fsimage *table,
			     int table_len) {
    for(int i = 0; i < table_len; i++) {
        if (!strcmp(table[i].name, cmd))
            return &table[i];
    }
    return 0;
}

int do_fsimage(int argc, char *argv[]) {
    const struct cmd_tbl_fsimage *cp;

	if (argc < 2)
        return -EINVAL;

	/* Drop argv[0] ("fsimage") */
	argc--;
	argv++;

	if(!strncmp(argv[0], "list", sizeof("list"))) {
		return do_fsimage_list(argc, argv);
	} else if (!strncmp(argv[0], "load", sizeof("load"))) {
		return do_fsimage_load(argc, argv);
	}else if (!strncmp(argv[0], "save", sizeof("save"))) {
		return do_fsimage_save(argc, argv);
	} else {
		printf("usage...\n");
	}
	return 0;
}

int main(int argc, char *argv[])
{
	load_saved_nboot();
	extract_board_config();
	fs_image_check_saved_cfg();
	do_fsimage(argc, argv);
	return 0;
}