/*
 * fs_board_common.c
 *
 * (C) Copyright 2018
 * Hartmut Keller, F&S Elektronik Systeme GmbH, keller@fs-net.de
 *
 * Common board configuration and information
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <env.h>			/* env_get() */
#include <command.h>			/* run_command() */
#include <common.h>			/* types, get_board_name(), ... */
#include <serial.h>			/* get_serial_device() */
#include <stdio_dev.h>			/* STDIO_NAME_LEN */
#include <asm/gpio.h>			/* gpio_direction_output(), ... */
#include <asm/arch/sys_proto.h>		/* is_mx6*() */
#include <linux/delay.h>
#if CONFIG_IS_ENABLED(MTD_RAW_NAND)
#include <linux/mtd/rawnand.h>		/* struct mtd_info */
#endif
#include <dm/uclass.h>				/* uclass_get_device() */
#include "fs_board_common.h"		/* Own interface */
#include "fs_mmc_common.h"
#ifdef CONFIG_FS_SELFTEST
#include "fs_dram_test.h"
#endif
#include <fuse.h>			/* fuse_read() */
#include <update.h>			/* enum update_action */

#ifdef CONFIG_FS_BOARD_CFG
#include "fs_image_common.h"		/* fs_image_*() */
#endif

/* ============= Functions not available in SPL ============================ */

#ifndef CONFIG_SPL_BUILD

/* String used for system prompt */
static char fs_sys_prompt[32];

/* Store a pointer to the current board info */
static const struct fs_board_info *current_bi;

#ifdef CONFIG_FS_SELFTEST
/* Store DRAM test result for bdinfo */
static char dram_result[64] = "FAILED (Not run)";
#endif

/* ------------- Functions using fs_nboot_args ----------------------------- */

#ifndef CONFIG_FS_BOARD_CFG

/* Addresses of arguments coming from NBoot and going to Linux */
#define NBOOT_ARGS_BASE (CFG_SYS_SDRAM_BASE + 0x00001000)
#define BOOT_PARAMS_BASE (CFG_SYS_SDRAM_BASE + 0x100)

#define ACTION_RECOVER 0x00000040	/* Start recovery instead of update */

/* Copy of the NBoot arguments, split into nboot_args and m4_args */
static struct fs_nboot_args nboot_args;
static struct fs_m4_args m4_args;

#if 0
/* List NBoot args; function can be activated and used for debugging */
void fs_board_show_nboot_args(struct fs_nboot_args *pargs)
{
	printf("dwNumDram = 0x%08x\n", pargs->dwNumDram);
	printf("dwMemSize = 0x%08x\n", pargs->dwMemSize);
	printf("dwFlashSize = 0x%08x\n", pargs->dwFlashSize);
	printf("dwDbgSerPortPA = 0x%08x\n", pargs->dwDbgSerPortPA);
	printf("chBoardType = 0x%02x\n", pargs->chBoardType);
	printf("chBoardRev = 0x%02x\n", pargs->chBoardRev);
	printf("chFeatures1 = 0x%02x\n", pargs->chFeatures1);
	printf("chFeatures2 = 0x%02x\n", pargs->chFeatures2);
}
#endif

/* Get a pointer to the NBoot args */
struct fs_nboot_args *fs_board_get_nboot_args(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	/* As long as GD_FLG_RELOC is not set, we can not access variable
	   nboot_args and therefore have to use the NBoot args at
	   NBOOT_ARGS_BASE. However GD_FLG_RELOC may be set before the NBoot
	   arguments are actually copied from NBOOT_ARGS_BASE to nboot_args
	   (see fs_board_init_common() below). But then at least the .bss
	   section and therefore nboot_args is cleared. We check this by
	   looking at nboot_args.dwDbgSerPortPA. If this is 0, the
	   structure is not yet copied and we still have to look at
	   NBOOT_ARGS_BASE. Otherwise we can (and must) use nboot_args. */
	if ((gd->flags & GD_FLG_RELOC) && nboot_args.dwDbgSerPortPA)
		return &nboot_args;

	return (struct fs_nboot_args *)NBOOT_ARGS_BASE;
}

/* Get board type (zero-based) */
unsigned int fs_board_get_type(void)
{
	int BoardType = fs_board_get_nboot_args()->chBoardType;
#ifdef CONFIG_TARGET_FSIMX6
	if (BoardType >= 29)
		BoardType -= (29 - 8);
#elif CONFIG_TARGET_FSIMX6SX
	if (BoardType == 25)
		BoardType -= (25 - 7);
	else if (BoardType >= 32)
		BoardType -= (32 - 8);
	else
		BoardType -= 8;
#elif CONFIG_TARGET_FSIMX6UL
	BoardType -= 16;
#endif
	return BoardType;
}

/* Get board revision (major * 100 + minor, e.g. 120 for rev 1.20) */
unsigned int fs_board_get_rev(void)
{
	return fs_board_get_nboot_args()->chBoardRev;
}

ulong board_serial_base(void)
{
	struct fs_nboot_args *pargs = fs_board_get_nboot_args();

	return pargs->dwDbgSerPortPA;
}

/* Get the NBoot version */
const char *fs_board_get_nboot_version(void)
{
	struct fs_nboot_args *pargs = fs_board_get_nboot_args();
	static char version[5];

	memcpy(version, &pargs->dwNBOOT_VER, 4);
	version[4] = 0;

	return &version[0];
}

/* Set RAM size (as given by NBoot) and RAM base */
#ifndef CONFIG_TARGET_FSIMX8M
int dram_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;
	struct fs_nboot_args *pargs = fs_board_get_nboot_args();

	gd->ram_size = pargs->dwMemSize << 20;

	return 0;
}
#endif

#ifndef CONFIG_TARGET_FSIMX7ULP
void board_nand_state(struct mtd_info *mtd, unsigned int state)
{
	/* Save state to pass it to Linux later */
	nboot_args.chECCstate |= (unsigned char)state;
}
#endif
#endif /* !CONFIG_FS_BOARD_CFG */

/* ------------- Functions using BOARD-CFG --------------------------------- */

#ifdef CONFIG_FS_BOARD_CFG

/* Get Pointer to struct cfg_info */
struct cfg_info *fs_board_get_cfg_info(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	return (struct cfg_info *)&gd->cfg_info;
}

/* Get the boot device from BOARD-CFG) */
enum boot_device fs_board_get_boot_dev(void)
{
	return fs_board_get_cfg_info()->boot_dev;
}

/* Get board type (zero-based) */
unsigned int fs_board_get_type(void)
{
	return fs_board_get_cfg_info()->board_type;
}

/* Get board revision (major * 100 + minor, e.g. 120 for rev 1.20) */
unsigned int fs_board_get_rev(void)
{
	return fs_board_get_cfg_info()->board_rev;
}

/* Get the board features */
unsigned int fs_board_get_features(void)
{
	return fs_board_get_cfg_info()->features;
}

/* Get the NBoot version */
const char *fs_board_get_nboot_version(void)
{
	return fs_image_get_nboot_version(NULL);
}

/* Set RAM size; optee will be subtracted in dram_init() */
int board_phys_sdram_size(phys_size_t *size)
{
	*size = (phys_size_t)fs_board_get_cfg_info()->dram_size << 20;

	return 0;
}

#endif /* CONFIG_FS_BOARD_CFG */

/* ------------- Generic functions ----------------------------------------- */

/* Issue reset signal on up to three gpios (~0: gpio unused) */
void fs_board_issue_reset(uint active_us, uint delay_us,
			  uint gpio0, uint gpio1, uint gpio2)
{
	/* Assert reset */
	gpio_direction_output(gpio0, 0);
	if (gpio1 != ~0)
		gpio_direction_output(gpio1, 0);
	if (gpio2 != ~0)
		gpio_direction_output(gpio2, 0);

	/* Delay for the active pulse time */
	udelay(active_us);

	/* De-assert reset */
	gpio_set_value(gpio0, 1);
	if (gpio1 != ~0)
		gpio_set_value(gpio1, 1);
	if (gpio2 != ~0)
		gpio_set_value(gpio2, 1);

	/* Delay some more time if requested */
	if (delay_us)
		udelay(delay_us);
}

#ifdef CONFIG_CMD_UPDATE
/*
 * Returns true if the condition of the GPIO description is met.
 *
 * The given GPIO description contains the number of a GPIO, followed by an
 * optional '-' or '_', followed by an optional "high" or "low" for active
 * high or active low signal. Actually only the first character is checked,
 * 'h' and 'H' mean "high", everything else is taken for "low". Default is
 * active low.
 *
 * Examples:
 *    123_high  GPIO #123, active high
 *    65-low    GPIO #65, active low
 *    13        GPIO #13, active low
 *    0x1fh     GPIO #31, active high (this shows why a dash or
 *              underscore before "high" or "low" makes sense)
 *
 * Remark:
 * We do not have any clue here what the GPIO represents and therefore we do
 * not assume any pad settings. So for example if the GPIO represents a button
 * that is floating in the released state, an external pull-up or pull-down
 * must be used to avoid unintentionally detecting the active state.
 */
static bool update_check_gpio(const char *gpiodesc)
{
	char *endp;
	int active_state = 0;
	unsigned int gpio = simple_strtoul(gpiodesc, &endp, 0);

	if (endp != gpiodesc) {
		char c = *endp;

		if ((c == '-') || (c == '_'))
			c = *(++endp);
		if ((c == 'h') || (c == 'H'))
			active_state = 1;
		if (!gpio_direction_input(gpio)
		    && (gpio_get_value(gpio) == active_state))
			return true;
	}

	return false;
}

/* Check if recover or update procedure is requested (or none at all). */
enum update_action board_check_for_recover(void)
{
	char *gpiodesc;

#ifndef CONFIG_FS_BOARD_CFG
	/* On some platforms, the check for recovery is already done in NBoot.
	   Then the ACTION_RECOVER bit in the dwAction value is set. */
	if (nboot_args.dwAction & ACTION_RECOVER)
		return UPDATE_ACTION_RECOVER;
#endif

	/* If a recover GPIO is defined, check if it is active. */
	gpiodesc = env_get("recovergpio");
	if (gpiodesc && update_check_gpio(gpiodesc))
		return UPDATE_ACTION_RECOVER;

	/* Handle optional GPIO for update */
	gpiodesc = env_get("updategpio");
	if (gpiodesc && !update_check_gpio(gpiodesc))
		return UPDATE_ACTION_NONE;

	/* Skip check for update when going for fastboot */
	if (is_boot_from_usb())
		return UPDATE_ACTION_NONE;

	return UPDATE_ACTION_UPDATE;
}
#endif /* CONFIG_CMD_UPDATE */

/* Copy NBoot args to variables and prepare command prompt string */
void fs_board_init_common(const struct fs_board_info *board_info)
{
	DECLARE_GLOBAL_DATA_PTR;
	__maybe_unused struct udevice *udev;

#ifndef CONFIG_FS_BOARD_CFG
	struct fs_nboot_args *pargs = (struct fs_nboot_args *)NBOOT_ARGS_BASE;

	/* Save a copy of the NBoot args */
	memcpy(&nboot_args, pargs, sizeof(struct fs_nboot_args));
	nboot_args.dwSize = sizeof(struct fs_nboot_args);
	memcpy(&m4_args, pargs + 1, sizeof(struct fs_m4_args));
	m4_args.dwSize = sizeof(struct fs_m4_args);

	/* For ATAGs, if no device tree is used (basically unused) */
	gd->bd->bi_boot_params = BOOT_PARAMS_BASE;
#endif
	/* There is no arch number if booting Linux with device tree */
	gd->bd->bi_arch_number = 0xFFFFFFFF;

	/* Save a pointer to this board info */
	current_bi = board_info;

	/* Prepare the command prompt */
	sprintf(fs_sys_prompt, "%s # ", board_info->name);

#ifdef CONFIG_IMX_TMU
	/* Initialize thermal sensor */
	uclass_get_device(UCLASS_THERMAL, 0, &udev);
#endif
}

#ifdef CONFIG_BOARD_LATE_INIT
/* If variable has value "undef", update it with a board specific value */
static void setup_var(const char *varname, const char *content, int runvar)
{
	char *envvar = env_get(varname);

	/* If variable is not set or does not contain string "undef", do not
	   change it */
	if (!envvar || strcmp(envvar, "undef"))
		return;

	/* Either set variable directly with value ... */
	if (!runvar) {
		env_set(varname, content);
		return;
	}

	/* ... or set variable by running the variable with name in content */
	content = env_get(content);
	if (content)
		run_command(content, 0);
}

/* Show NBoot version and set up all board specific variables */
void fs_board_late_init_common(const char *serial_name)
{
	const char *envvar;
#ifdef CONFIG_FS_MMC_COMMON
	int usdhc_boot_device = get_usdhc_boot_device();
	int mmc_boot_device = get_mmc_boot_device();
#else // support FSVYBRID
	int usdhc_boot_device = 0;
	int mmc_boot_device = 0;
#endif
	bool is_nand = (fs_board_get_boot_dev() == NAND_BOOT);
	const char *bd_kernel, *bd_fdt, *bd_rootfs;
#ifdef CONFIG_ARCH_MX7ULP
	const char *bd_auxcore;
#endif
	char var_name[20];
	bool conflict = false;

#ifdef CONFIG_FS_BOARD_CFG
	ulong found_cfg = (ulong)fs_image_get_cfg_addr();
	ulong expected_cfg = (ulong)fs_image_get_regular_cfg_addr();

	printf("CFG:   Found at 0x%lx", found_cfg);
	if (found_cfg != expected_cfg) {
		printf(" *** Warning - expected at 0x%lx", expected_cfg);
		conflict = true;
	}
	putc('\n');
#endif

	printf("NBoot: %s", fs_board_get_nboot_version());
	if (conflict)
		puts(" *** Warning - not compatible with current U-Boot!");
	putc('\n');

#ifdef CONFIG_FS_SELFTEST
	/* Save dram_result for bdinfo */
	fs_test_ram(dram_result);
#endif

	/* Set sercon variable if not already set */
	envvar = env_get("sercon");
	if (!envvar || !strcmp(envvar, "undef")) {
#ifdef CONFIG_DM_SERIAL
		char sercon[STDIO_NAME_LEN];

		snprintf(sercon, STDIO_NAME_LEN, "%s%d", serial_name,
			 serial_get_alias_seq());
		env_set("sercon", sercon);
#else
		env_set("sercon", default_serial_console()->name);
#endif
	}

	/* Set platform variable if not already set */
	envvar = env_get("platform");
	if (!envvar || !strcmp(envvar, "undef")) {
		char lcasename[20];
		char *p = current_bi->name;
		char *l = lcasename;
		char c;
		int len = 0;

		do {
			c = *p++;
			if ((c >= 'A') && (c <= 'Z'))
				c += 'a' - 'A';
			*l++ = c;
			len++;
		} while (c);

#ifdef CONFIG_MX6QDL
		/* In case of regular i.MX, add 'dl' or 'q' */
		if (is_mx6sdl())
			sprintf(lcasename, "%sdl", lcasename);
		else if (is_mx6dq())
			sprintf(lcasename, "%sq", lcasename);
#elif defined(CONFIG_MX6UL) || defined(CONFIG_MX6ULL)
		/*
		 * In case of i.MX6ULL, append a second 'l' if the name already
		 * have a substring with 'ul', otherwise append 'ull'.
		 * This results in the names efusa7ull, cubea7ull,
		 * picocom1.2ull, cube2.0ull, picocoremx6ul100 ...
		 */
		if (is_mx6ull()) {
			int i = 0;
			bool found = false;
			p = lcasename;
			do {
				if (*p == 'u' && *++p == 'l')
				{
					i += 2;
					*l = '\0';
					do {
						//*l-- = *l;
						*l = l[-1];
						l--;
						len--;
					} while (i != len);
					*l = 'l';
					found = true;
					break;
				}
				p++;
				i++;
			} while (*p);

			if (!found) {
				l--;
				/* Names have > 2 chars, so negative index
				 * is valid
				 */
				if ((l[-2] != 'u') || (l[-1] != 'l')) {
					*l++ = 'u';
					*l++ = 'l';
				}
				*l++ = 'l';
				*l++ = '\0';
			}
		}
#endif
		env_set("platform", lcasename);
	}

	/* Set usdhcdev variable if not already set */
	envvar = env_get("usdhcdev");
	if (!envvar || !strcmp(envvar, "undef")) {
		char usdhcdev[STDIO_NAME_LEN];

		sprintf(usdhcdev, "%c", '0' + usdhc_boot_device);
		env_set("usdhcdev", usdhcdev);
	}

	/* Set mmcdev variable if not already set */
	envvar = env_get("mmcdev");
	if (!envvar || !strcmp(envvar, "undef")) {
		char mmcdev[STDIO_NAME_LEN];

		sprintf(mmcdev, "%c", '0' + mmc_boot_device);
		env_set("mmcdev", mmcdev);
	}

	/* Set some variables with a direct value */
#if defined(CONFIG_FS_SELFTEST)
	env_set("bootdelay", "0");
	env_set("updatecheck", "");
	env_set("installcheck", "");
	env_set("recovercheck", "");
#else
	setup_var("bootdelay", current_bi->bootdelay, 0);
	setup_var("updatecheck", current_bi->updatecheck, 0);
	setup_var("installcheck", current_bi->installcheck, 0);
	setup_var("recovercheck", current_bi->recovercheck, 0);
#endif
#if CONFIG_IS_ENABLED(MTD_RAW_NAND)
	setup_var("mtdids", MTDIDS_DEFAULT, 0);
	setup_var("partition", MTDPART_DEFAULT, 0);
#endif
#ifdef CONFIG_FS_BOARD_MODE_RO
	setup_var("mode", "ro", 0);
#else
	setup_var("mode", "rw", 0);
#endif
	/* Set boot devices for kernel, device tree and rootfs */
	if (is_nand) {
		bd_rootfs = "ubifs";
		if (current_bi->flags & BI_FLAGS_UBIONLY)
			bd_kernel = bd_rootfs;
		else
			bd_kernel = "nand";
		bd_fdt = bd_kernel;
#ifdef CONFIG_ARCH_MX7ULP
		bd_auxcore = bd_kernel;
#endif
	} else {
		bd_kernel = "mmc";
		bd_fdt = bd_kernel;
		bd_rootfs = bd_kernel;
#ifdef CONFIG_ARCH_MX7ULP
		bd_auxcore = bd_kernel;
#endif
	}
	setup_var("bd_kernel", bd_kernel, 0);
	setup_var("bd_fdt", bd_fdt, 0);
	setup_var("bd_rootfs", bd_rootfs, 0);
#ifdef CONFIG_ARCH_MX7ULP
	setup_var("bd_auxcore", bd_auxcore, 0);
#endif

	setup_var("console", current_bi->console, 1);
	setup_var("login", current_bi->login, 1);
	setup_var("mtdparts", current_bi->mtdparts, 1);
	setup_var("network", current_bi->network, 1);
	setup_var("init", current_bi->init, 1);
	setup_var("bootfdt", "set_bootfdt", 1);
	setup_var("fdtfile", "set_bootfdt", 1);
	setup_var("bootargs", "set_bootargs", 1);
#ifdef CONFIG_ARCH_MX7ULP
	setup_var("bootauxfile", "power_mode_switch.img", 0);
#endif


	/* Set some variables by runnning another variable */
#ifdef CONFIG_FS_UPDATE_SUPPORT
	sprintf(var_name, ".kernel_%s_A", bd_kernel);
	setup_var("kernel", var_name, 1);
	sprintf(var_name, ".fdt_%s_A", bd_fdt);
	setup_var("fdt", var_name, 1);
	sprintf(var_name, ".rootfs_%s_A", bd_rootfs);
	setup_var("rootfs", var_name, 1);
#else
	sprintf(var_name, ".kernel_%s", bd_kernel);
	setup_var("kernel", var_name, 1);
	sprintf(var_name, ".fdt_%s", bd_fdt);
	setup_var("fdt", var_name, 1);
	sprintf(var_name, ".rootfs_%s", bd_rootfs);
	setup_var("rootfs", var_name, 1);
#ifdef CONFIG_ARCH_MX7ULP
	sprintf(var_name, ".auxcore_%s", bd_auxcore);
	setup_var("auxcore", var_name, 1);
#endif
#endif

}
#endif /* CONFIG_BOARD_LATE_INIT */

/* Return the board name (board specific) */
char *get_board_name(void)
{
	return current_bi->name;
}

/* Return the system prompt (board specific) */
char *get_sys_prompt(void)
{
	return fs_sys_prompt;
}

#ifdef CONFIG_FS_SELFTEST
/* Return dram_result for bdinfo */
char *get_dram_result(void)
{
	return dram_result;
}
#endif

#endif /* ! CONFIG_SPL_BUILD */

/* ============= Functions also available in SPL =========================== */

struct boot_dev_name {
	enum boot_device boot_dev;
	const char *name;
};

const struct boot_dev_name boot_dev_names[] = {
	{USB_BOOT,  "USB"},
	{USB2_BOOT, "USB2"},
	{NAND_BOOT, "NAND"},
	{MMC1_BOOT, "MMC1"},
	{MMC2_BOOT, "MMC2"},
	{MMC3_BOOT, "MMC3"},
	{SD1_BOOT,  "SD1"},
	{SD2_BOOT,  "SD2"},
	{SD3_BOOT,  "SD3"},
};

/* Get the boot device number from the string */
enum boot_device fs_board_get_boot_dev_from_name(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(boot_dev_names); i++) {
		if (!strcmp(boot_dev_names[i].name, name))
			return boot_dev_names[i].boot_dev;
	}
	return UNKNOWN_BOOT;
}

/* Get the string from the boot device number */
const char *fs_board_get_name_from_boot_dev(enum boot_device boot_dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(boot_dev_names); i++) {
		if (boot_dev_names[i].boot_dev == boot_dev)
			return boot_dev_names[i].name;
	}

	return "(unknown)";
}

#ifdef CONFIG_FS_BOARD_CFG

#include <fdtdec.h>

#ifdef CONFIG_IMX8
/* Definitions in boot_cfg (fuse bank 0, word 18) */
#define BOOT_CFG_DEVSEL_SHIFT 0
#define BOOT_CFG_DEVSEL_MASK (0x3F << BOOT_CFG_DEVSEL_SHIFT)
/*
 * Return the boot device as programmed in the fuses. This may differ from the
 * currently active boot device. For example the board can currently boot from
 * USB (returned by spl_boot_device()), but is basically fused to boot from
 * NAND (returned here).
 */
enum boot_device fs_board_get_boot_dev_from_fuses(void)
{
	u32 val;
	enum boot_device boot_dev = USB_BOOT;

	/* boot_cfg is in fuse bank 0, word 18 */
	if (sc_misc_otp_fuse_read(-1, 18, &val)) {
		puts("Error reading boot_cfg\n");
		return boot_dev;
	}

	switch ((val & BOOT_CFG_DEVSEL_MASK) >> BOOT_CFG_DEVSEL_SHIFT) {
	case 0x0: // eFuse
	printf("eFuse\n");
		boot_dev = get_boot_device();
		break;

	case 0x1: // USB
	printf("USB\n");
		boot_dev = USB_BOOT;
		break;

	case 0x2: // eMMC0
	printf("eMMC0\n");
		boot_dev = MMC1_BOOT;
		break;

	case 0x3: // SD1
	printf("SD1\n");
		boot_dev = SD2_BOOT;
		break;

	case 0x4: // NAND(128 pages)
	case 0x5: // NAND( 32 pages)
	printf("NAND\n");
		boot_dev = NAND_BOOT;
		break;

	case 0x6: // FlexSPI(default)
	case 0x7: // FlexSPI(Hyperflash 3.0)
	printf("FlexSPI\n");
		boot_dev = FLEXSPI_BOOT;
		break;

	default:
	printf("Default\n");
		break;
	}

	return boot_dev;
}

#define IMG_CNTN_SET1_OFFSET_SHIFT 24
#define IMG_CNTN_SET1_OFFSET_MASK 0x1f
u32 fs_board_get_secondary_offset(void)
{
	u32 val;

	/* Secondary boot image offset is in fuse bank 0, word 720 */
	if (sc_misc_otp_fuse_read(-1, 720, &val)) {
		puts("Error reading secondary image offset from fuses\n");
		return 0;
	}

	val >>= IMG_CNTN_SET1_OFFSET_SHIFT;
	val &= IMG_CNTN_SET1_OFFSET_MASK;
	if (val == 0)
		val = 2;
	else if (val == 2)
		val = 0;
	val += 20;
	val = 1 << val;

	return val;
}

#elif CONFIG_IMX8MM
/* Definitions in boot_cfg (fuse bank 1, word 3) */
#define BOOT_CFG_DEVSEL_SHIFT 12
#define BOOT_CFG_DEVSEL_MASK (7 << BOOT_CFG_DEVSEL_SHIFT)
#define BOOT_CFG_PORTSEL_SHIFT 10
#define BOOT_CFG_PORTSEL_MASK (3 << BOOT_CFG_PORTSEL_SHIFT)
/*
 * Return the boot device as programmed in the fuses. This may differ from the
 * currently active boot device. For example the board can currently boot from
 * USB (returned by spl_boot_device()), but is basically fused to boot from
 * NAND (returned here).
 */
enum boot_device fs_board_get_boot_dev_from_fuses(void)
{
	u32 val;
	u32 port;
	enum boot_device boot_dev = USB_BOOT;

	/* boot_cfg is in fuse bank 1, word 3 */
	if (fuse_read(1, 3, &val)) {
		puts("Error reading boot_cfg\n");
		return boot_dev;
	}

	port = (val & BOOT_CFG_PORTSEL_MASK) >> BOOT_CFG_PORTSEL_SHIFT;
	switch ((val & BOOT_CFG_DEVSEL_MASK) >> BOOT_CFG_DEVSEL_SHIFT) {
	case BOOT_TYPE_SD:
		boot_dev = SD1_BOOT + port;
		break;

	case BOOT_TYPE_MMC:
		boot_dev = MMC1_BOOT + port;
		break;

	case BOOT_TYPE_NAND:
		boot_dev = NAND_BOOT;
		break;

	default:
		break;
	}

	return boot_dev;
}

#elif defined(CONFIG_IMX8MN) || defined(CONFIG_IMX8MP)
/* Definitions in boot_cfg (fuse bank 1, word 3) */
#define BOOT_CFG_DEVSEL_SHIFT 12
#define BOOT_CFG_DEVSEL_MASK (15 << BOOT_CFG_DEVSEL_SHIFT)
#define BOOT_CFG_FORCE_ALT_USDHC BIT(11)
#define BOOT_CFG_ALT_SEL_SHIFT 9
#define BOOT_CFG_ALT_SEL_MASK (3 << BOOT_CFG_ALT_SEL_SHIFT)
enum boot_device usdhc_alt[] = {
	SD1_BOOT,
	MMC1_BOOT,
	MMC2_BOOT,
	SD3_BOOT,
};
/*
 * Return the boot device as programmed in the fuses. This may differ from the
 * currently active boot device. For example the board can currently boot from
 * USB (returned by spl_boot_device()), but is basically fused to boot from
 * NAND (returned here).
 */
enum boot_device fs_board_get_boot_dev_from_fuses(void)
{
	u32 val;
	u32 alt;
	bool force_alt_usdhc;
	enum boot_device boot_dev = USB_BOOT;

	/* boot_cfg is in fuse bank 1, word 3 */
	if (fuse_read(1, 3, &val)) {
		puts("Error reading boot_cfg\n");
		return boot_dev;
	}

	force_alt_usdhc = val & BOOT_CFG_FORCE_ALT_USDHC;
	alt = (val & BOOT_CFG_ALT_SEL_MASK) >> BOOT_CFG_ALT_SEL_SHIFT;
	switch ((val & BOOT_CFG_DEVSEL_MASK) >> BOOT_CFG_DEVSEL_SHIFT) {
	case 0x1: // USB0 / USB1
		boot_dev = get_boot_device();
	case 0x2: // eMMC(SD3)
		boot_dev = MMC3_BOOT;
		if (force_alt_usdhc)
			boot_dev = usdhc_alt[alt];
		break;

	case 0x3: // SD(SD2)
		boot_dev = SD2_BOOT;
		if (force_alt_usdhc)
			boot_dev = usdhc_alt[alt];
		break;

	case 0x4: // NAND(256 pages)
	case 0x5: // NAND(512 pages)
		boot_dev = NAND_BOOT;
		break;

	default:
		break;
	}

	return boot_dev;
}

#define IMG_CNTN_SET1_OFFSET_SHIFT 19
#define IMG_CNTN_SET1_OFFSET_MASK 0x0f
u32 fs_board_get_secondary_offset(void)
{
	u32 val;

	/* Secondary boot image offset is in fuse bank 2, word 1 */
	if (fuse_read(2, 1, &val)) {
		puts("Error reading secondary image offset from fuses\n");
		return 0;
	}

	val >>= IMG_CNTN_SET1_OFFSET_SHIFT;
	val &= IMG_CNTN_SET1_OFFSET_MASK;
	if (val == 0)
		val = 2;
	else if (val == 2)
		val = 0;
	val += 20;
	val = 1 << val;

	return val;
}

#elif   defined(CONFIG_IMX93) || defined(CONFIG_IMX91)
/* Definitions in boot_cfg (fuse bank 3, word 0) */
#define BOOT_CFG_BOOT_MODE_SHIFT 0
#define BOOT_CFG_BOOT_MODE_MASK GENMASK(3, BOOT_CFG_BOOT_MODE_SHIFT)

/*
 * Return the boot device as programmed in the fuses. This may differ from the
 * currently active boot device. For example the board can currently boot from
 * USB (returned by spl_boot_device()), but is basically fused to boot from
 * NAND (returned here).
 */
enum boot_device fs_board_get_boot_dev_from_fuses(void)
{
	u32 val;
	u32 boot_mode;

	enum boot_device boot_dev = USB_BOOT;

	/* boot_mode is in fuse bank 3, word 1 */
	if (fuse_read(3, 0, &val)) {
		puts("Error reading boot_cfg\n");
		return boot_dev;
	}

	boot_mode = (val & BOOT_CFG_BOOT_MODE_MASK);

	switch (boot_mode) {
	case 0x2: // eMMC(USDHC1)
		boot_dev = MMC1_BOOT;
		break;	
	case 0x3: // SD(USDHC2)
		boot_dev = SD2_BOOT;
		break;
	case 0x4: // NOR(FLEXSPI)
		boot_dev = FLEXSPI_BOOT;
		break;
	case 0x5: // NAND(FLEXSPI)
		boot_dev = FLEXSPI_NAND_BOOT;
		break;
	default:
		break;
	}

	return boot_dev;
}

#define IMG_CNTN_SET1_OFFSET_SHIFT 8
#define IMG_CNTN_SET1_OFFSET_MASK GENMASK(15, IMG_CNTN_SET1_OFFSET_SHIFT)
u32 fs_board_get_secondary_offset(void)
{
	u32 val;
	unsigned long offset;


	/* Secondary boot image offset is in fuse bank 3, word 8 */
	if (fuse_read(3, 8, &val)) {
		puts("Error reading secondary image offset from fuses\n");
		return 0;
	}

	val &= IMG_CNTN_SET1_OFFSET_MASK;
	val >>= IMG_CNTN_SET1_OFFSET_SHIFT;

	/**
	 *  TODO: Value CNT_N = 254 and CNT_N = 255 are currently not supported
	 */
	if (val == 0)
		val = 2;
	else if (val == 2 || val >= 10 )
		val = 0;

	val += 20;
	offset = 1 << val;

	return offset;
}
#elif defined(CONFIG_IMX8ULP)
/* Definitions in boot_cfg (fuse bank 4, word 1) */
#define BOOT_CFG_BOOT_TYPE_SHIFT	29
#define BOOT_CFG_BOOT_TYPE_MASK		GENMASK(30, 29)
#define BOOT_CFG_BOOT_IFACE_SHIFT 	26
#define BOOT_CFG_BOOT_IFACE_MASK	GENMASK(28, 26)
#define BOOT_CFG_BOOT_DEV_SHIFT 	25
#define BOOT_CFG_BOOT_DEV_MASK		GENMASK(25, 25)

/*
 * Return the boot device as programmed in the fuses. This may differ from the
 * currently active boot device. For example the board can currently boot from
 * USB (returned by spl_boot_device()), but is basically fused to boot from
 * NAND (returned here).
 */
enum boot_device fs_board_get_boot_dev_from_fuses(void)
{
	u32 val;
	u32 boot_type;
	u32 boot_iface;
	u32 boot_devtype;

	enum boot_device boot_dev = USB_BOOT;

	if (fuse_read(4, 1, &val)) {
		puts("Error reading boot_cfg\n");
	 	return boot_dev;
	}

	boot_type = val & BOOT_CFG_BOOT_TYPE_MASK;
	boot_type = boot_type >> BOOT_CFG_BOOT_TYPE_SHIFT;

	boot_iface = val & BOOT_CFG_BOOT_IFACE_MASK;
	boot_iface = boot_iface >> BOOT_CFG_BOOT_IFACE_SHIFT;

	boot_devtype = val & BOOT_CFG_BOOT_DEV_MASK;
	boot_devtype = boot_devtype >> BOOT_CFG_BOOT_DEV_SHIFT;

	if(boot_type != 0x0)
		goto non_usdhc;

	boot_dev = SD1_BOOT;
	boot_dev += boot_iface;

	if(!boot_devtype)
		boot_dev += MMC1_BOOT - SD1_BOOT;

	return boot_dev;

	non_usdhc:
	switch (boot_type) {
	case 0x1: // NAND(FLEXSPI)
	 	boot_dev = FLEXSPI_NAND_BOOT;
	 	break;
	case 0x2: // NOR(FLEXSPI)
	 	boot_dev = FLEXSPI_BOOT;
	 	break;
	default:
		break;
	}

	return boot_dev;
}

u32 fs_board_get_secondary_offset(void)
{
	/**
	 *  TODO: There is no description for secondary cntr offset on mmc
	 * devices
	 */
	return 0;
}
#endif

#endif /* CONFIG_FS_BOARD_CFG */

#if CONFIG_IS_ENABLED(AHAB_BOOT)
static bool imx_ele_ahab_is_enabled(void)
{
	u32 lc;

	lc = readl(FSB_BASE_ADDR + 0x41c);
	lc &= 0x3ff;
	
	// if lc != 0x8 then lifecycle is not OEM open
	return !!(lc != 0x8);
}

#endif

bool fs_board_is_closed(void)
{
#if CONFIG_IS_ENABLED(AHAB_BOOT)
	return imx_ele_ahab_is_enabled();
#elif CONFIG_IS_ENABLED(IMX_HAB)
	return imx_hab_is_enabled();
#endif

	return false;
}
