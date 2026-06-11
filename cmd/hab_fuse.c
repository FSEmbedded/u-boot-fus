#include <common.h>
#include <command.h>
#include <console.h>
#include <fuse.h>

static int do_hab_fuse(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[]) {
	unsigned long table_addr = 0;
	unsigned long parse_addr = 0;
	unsigned long fuse_word[8];
	bool force = false;
	unsigned long hash_offset = 0;
	unsigned long crc1 = 0;
	unsigned long crc2 = 0;

#ifndef CONFIG_IMX8M
	printf("This command is only meant to bes used on i.MX8M Processors.\nPlease refer to the F&S Secure Boot Manual to fuse other platforms.\n");
	return 0;
#endif

	argc--;
	argv++;
	while (argc > 0) {
		if(simple_strtoul(argv[0], NULL, 16) != 0) {
			table_addr = simple_strtoul(argv[0], NULL, 16);
		} else if(!strcmp(argv[0], "-f")) {
			force = true;
		} else {
			return -1;
		}
		argc--;
		argv++;
	}
	if(table_addr == 0) {
		table_addr = simple_strtoul(env_get("loadaddr"), NULL, 16);
	}

	//prepare parsing area
	parse_addr = table_addr + 0x80;
	memset((void *)parse_addr, 0, 16);

	//find hash offset
	while (*(char*)(table_addr + hash_offset) != 'X') {
		hash_offset++;
		if(hash_offset == 0xC0)
		return -1;
	}

	//parse delivered crc
	memcpy((void *)parse_addr, (const void *)table_addr, hash_offset);
	crc1 = simple_strtoul((void *)parse_addr, NULL, 10);
	memset((void *)parse_addr, 0, 16);

	//set table pointer to the first byte
	table_addr += hash_offset + 1;

	//calculate crc
	for(int i = 0; i < 32; i++) {
		memcpy((void *)parse_addr, (const void *)(table_addr + i * 2), 2);
		crc2 += simple_strtoul((const char *)parse_addr, NULL, 16);
	}

	//check crc
	if(crc1 != crc2) {
		printf("CRC check failed\n");
		return -1;
	} else {
		printf("CRC check successfull\n");
	}

	//inform user
	printf("The following fuses will be blown:\n");
	for(int i = 0; i < 8; i++) {
		for(int j = 6; j >= 0; j -= 2) {
			memcpy((void *)(parse_addr + j), (const void *)table_addr, 2);
			table_addr += 2;
		}
		fuse_word[i] = simple_strtoul((const char *)parse_addr, NULL, 16);
		memset((void *)parse_addr, 0, 16);

		printf("\tword %x fuse %x: 0x%08lx\n", 6 + (i/4), i % 4, fuse_word[i]);
	}

	//ask for permission
	if(force == false) {
		printf("THIS IS A PERMANENT CHANGE!!! \nAre you sure? [Y/n]\n");
		if (!confirm_yesno())
			return 0;
	}

	//fuse
	for(int i = 0; i < 8; i++) {
		printf("\tBlowing fuse %x, word %x with value 0x%08lx... ", 6 + (i/4), i % 4, fuse_word[i]);
		fuse_prog(6 + (i/4), i % 4, fuse_word[i]);
		printf("Blown!\n");
	}

	return 0;
}

static int do_hab_close(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[]) {
	bool force = false;

#ifndef CONFIG_IMX8M
    printf("This command is only meant to bes used on i.MX8M Processors.\nPlease refer to the F&S Secure Boot Manual to fuse other platforms.\n");
    return 0;
#endif

	//process input
	argc--;
	argv++;
	while (argc > 0) {
		if(!strcmp(argv[0], "-f"))
			force = true;
		else
			return -1;
		argc--;
		argv++;
	}

	//ask for permission
	if(force == false) {
		printf("THIS IS A PERMANENT CHANGE!!! \nAre you sure? [Y/n]\n");
		if (!confirm_yesno())
			return 0;
	}

	//fuse
	printf("\tBlowing fuse %x, word %x with value 0x%08x... ", 1, 3, 0x02000000);
	fuse_prog(1, 3, 0x02000000);
	printf("Blown!\n");

	return 0;
}

U_BOOT_CMD(hab_fuse, 3, 0, do_hab_fuse, "usage", "[addr] [-f]");
U_BOOT_CMD(hab_close, 2, 0, do_hab_close, "usage", "[-f]");
