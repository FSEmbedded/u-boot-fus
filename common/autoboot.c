// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#include <common.h>
#include <autoboot.h>
#include <bootretry.h>
#include <cli.h>
#include <command.h>
#include <console.h>
#include <env.h>
#include <fdtdec.h>
#include <hash.h>
#include <log.h>
#include <malloc.h>
#include <memalign.h>
#include <menu.h>
#include <post.h>
#include <time.h>
#include <asm/global_data.h>
#include <linux/delay.h>
#include <u-boot/sha256.h>
#include <bootcount.h>
#include <update.h>			/* enum update_action */
#ifdef is_boot_from_usb
#include <env.h>
#ifdef CONFIG_FSL_FASTBOOT
#include <fb_fsl.h>
#endif
#endif

DECLARE_GLOBAL_DATA_PTR;

#define XMK_STR(x)	#x
#define MK_STR(x)	XMK_STR(x)

#define MAX_DELAY_STOP_STR 64

#ifndef DEBUG_BOOTKEYS
#define DEBUG_BOOTKEYS 0
#endif
#define debug_bootkeys(fmt, args...)		\
	debug_cond(DEBUG_BOOTKEYS, fmt, ##args)

/* Stored value of bootdelay, used by autoboot_command() */
static int stored_bootdelay;
static int menukey;

#ifdef CONFIG_AUTOBOOT_ENCRYPTION
#define AUTOBOOT_STOP_STR_SHA256 CONFIG_AUTOBOOT_STOP_STR_SHA256
#else
#define AUTOBOOT_STOP_STR_SHA256 ""
#endif

#ifdef CONFIG_USE_AUTOBOOT_MENUKEY
#define AUTOBOOT_MENUKEY CONFIG_USE_AUTOBOOT_MENUKEY
#else
#define AUTOBOOT_MENUKEY 0
#endif

/*
 * Use a "constant-length" time compare function for this
 * hash compare:
 *
 * https://crackstation.net/hashing-security.htm
 */
static int slow_equals(u8 *a, u8 *b, int len)
{
	int diff = 0;
	int i;

	for (i = 0; i < len; i++)
		diff |= a[i] ^ b[i];

	return diff == 0;
}

/**
 * passwd_abort_sha256() - check for a hashed key sequence to abort booting
 *
 * This checks for the user entering a SHA256 hash within a given time.
 *
 * @etime: Timeout value ticks (stop when get_ticks() reachs this)
 * @return 0 if autoboot should continue, 1 if it should stop
 */
static int passwd_abort_sha256(uint64_t etime)
{
	const char *sha_env_str = env_get("bootstopkeysha256");
	u8 sha_env[SHA256_SUM_LEN];
	u8 *sha;
	char *presskey;
	char *c;
	const char *algo_name = "sha256";
	u_int presskey_len = 0;
	int abort = 0;
	int size = sizeof(sha);
	int ret;

	if (sha_env_str == NULL)
		sha_env_str = AUTOBOOT_STOP_STR_SHA256;

	presskey = malloc_cache_aligned(MAX_DELAY_STOP_STR);
	c = strstr(sha_env_str, ":");
	if (c && (c - sha_env_str < MAX_DELAY_STOP_STR)) {
		/* preload presskey with salt */
		memcpy(presskey, sha_env_str, c - sha_env_str);
		presskey_len = c - sha_env_str;
		sha_env_str = c + 1;
	}
	/*
	 * Generate the binary value from the environment hash value
	 * so that we can compare this value with the computed hash
	 * from the user input
	 */
	ret = hash_parse_string(algo_name, sha_env_str, sha_env);
	if (ret) {
		printf("Hash %s not supported!\n", algo_name);
		return 0;
	}

	sha = malloc_cache_aligned(SHA256_SUM_LEN);
	size = SHA256_SUM_LEN;
	/*
	 * We don't know how long the stop-string is, so we need to
	 * generate the sha256 hash upon each input character and
	 * compare the value with the one saved in the environment
	 */
	do {
		if (tstc()) {
			/* Check for input string overflow */
			if (presskey_len >= MAX_DELAY_STOP_STR) {
				free(presskey);
				free(sha);
				return 0;
			}

			presskey[presskey_len++] = getchar();

			/* Calculate sha256 upon each new char */
			hash_block(algo_name, (const void *)presskey,
				   presskey_len, sha, &size);

			/* And check if sha matches saved value in env */
			if (slow_equals(sha, sha_env, SHA256_SUM_LEN))
				abort = 1;
		}
	} while (!abort && get_ticks() <= etime);

	free(presskey);
	free(sha);
	return abort;
}

/**
 * passwd_abort_key() - check for a key sequence to aborted booting
 *
 * This checks for the user entering a string within a given time.
 *
 * @etime: Timeout value ticks (stop when get_ticks() reachs this)
 * @return 0 if autoboot should continue, 1 if it should stop
 */
static int passwd_abort_key(uint64_t etime)
{
	int abort = 0;
	struct {
		char *str;
		u_int len;
		int retry;
	}
	delaykey[] = {
		{ .str = env_get("bootdelaykey"),  .retry = 1 },
		{ .str = env_get("bootstopkey"),   .retry = 0 },
	};

	char presskey[MAX_DELAY_STOP_STR];
	int presskey_len = 0;
	int presskey_max = 0;
	int i;

#  ifdef CONFIG_AUTOBOOT_DELAY_STR
	if (delaykey[0].str == NULL)
		delaykey[0].str = CONFIG_AUTOBOOT_DELAY_STR;
#  endif
#  ifdef CONFIG_AUTOBOOT_STOP_STR
	if (delaykey[1].str == NULL)
		delaykey[1].str = CONFIG_AUTOBOOT_STOP_STR;
#  endif

	for (i = 0; i < sizeof(delaykey) / sizeof(delaykey[0]); i++) {
		delaykey[i].len = delaykey[i].str == NULL ?
				    0 : strlen(delaykey[i].str);
		delaykey[i].len = delaykey[i].len > MAX_DELAY_STOP_STR ?
				    MAX_DELAY_STOP_STR : delaykey[i].len;

		presskey_max = presskey_max > delaykey[i].len ?
				    presskey_max : delaykey[i].len;

		debug_bootkeys("%s key:<%s>\n",
			       delaykey[i].retry ? "delay" : "stop",
			       delaykey[i].str ? delaykey[i].str : "NULL");
	}

	/* In order to keep up with incoming data, check timeout only
	 * when catch up.
	 */
	do {
		if (tstc()) {
			if (presskey_len < presskey_max) {
				presskey[presskey_len++] = getchar();
			} else {
				for (i = 0; i < presskey_max - 1; i++)
					presskey[i] = presskey[i + 1];

				presskey[i] = getchar();
			}
		}

		for (i = 0; i < sizeof(delaykey) / sizeof(delaykey[0]); i++) {
			if (delaykey[i].len > 0 &&
			    presskey_len >= delaykey[i].len &&
				memcmp(presskey + presskey_len -
					delaykey[i].len, delaykey[i].str,
					delaykey[i].len) == 0) {
					debug_bootkeys("got %skey\n",
						delaykey[i].retry ? "delay" :
						"stop");

				/* don't retry auto boot */
				if (!delaykey[i].retry)
					bootretry_dont_retry();
				abort = 1;
			}
		}
	} while (!abort && get_ticks() <= etime);

	return abort;
}

/***************************************************************************
 * Watch for 'delay' seconds for autoboot stop or autoboot delay string.
 * returns: 0 -  no key string, allow autoboot 1 - got key string, abort
 */
static int abortboot_key_sequence(int bootdelay)
{
	int abort;
	uint64_t etime = endtick(bootdelay);

#  ifdef CONFIG_AUTOBOOT_PROMPT
	/*
	 * CONFIG_AUTOBOOT_PROMPT includes the %d for all boards.
	 * To print the bootdelay value upon bootup.
	 */
	printf(CONFIG_AUTOBOOT_PROMPT, bootdelay);
#  endif

	if (IS_ENABLED(CONFIG_AUTOBOOT_ENCRYPTION))
		abort = passwd_abort_sha256(etime);
	else
		abort = passwd_abort_key(etime);
	if (!abort)
		debug_bootkeys("key timeout\n");

	return abort;
}

static int abortboot_single_key(int bootdelay)
{
	int abort = 0;
	unsigned long ts;

	printf("Hit any key to stop autoboot: %2d ", bootdelay);

	/*
	 * Check if key already pressed
	 */
	if (tstc()) {	/* we got a key press	*/
		getchar();	/* consume input	*/
		puts("\b\b\b 0");
		abort = 1;	/* don't auto boot	*/
	}

	while ((bootdelay > 0) && (!abort)) {
		--bootdelay;
		/* delay 1000 ms */
		ts = get_timer(0);
		do {
			if (tstc()) {	/* we got a key press	*/
				int key;

				abort  = 1;	/* don't auto boot	*/
				bootdelay = 0;	/* no more delay	*/
				key = getchar();/* consume input	*/
				if (IS_ENABLED(CONFIG_USE_AUTOBOOT_MENUKEY))
					menukey = key;
				break;
			}
			udelay(10000);
		} while (!abort && get_timer(ts) < 1000);

		printf("\b\b\b%2d ", bootdelay);
	}

	putc('\n');

	return abort;
}

static int abortboot(int bootdelay)
{
	int abort = 0;

	if (bootdelay >= 0) {
		if (IS_ENABLED(CONFIG_AUTOBOOT_KEYED))
			abort = abortboot_key_sequence(bootdelay);
		else
			abort = abortboot_single_key(bootdelay);
	}

	if (IS_ENABLED(CONFIG_SILENT_CONSOLE) && abort)
		gd->flags &= ~GD_FLG_SILENT;

	return abort;
}

static void process_fdt_options(const void *blob)
{
#ifdef CONFIG_SYS_TEXT_BASE
	ulong addr;

	/* Add an env variable to point to a kernel payload, if available */
	addr = fdtdec_get_config_int(gd->fdt_blob, "kernel-offset", 0);
	if (addr)
		env_set_addr("kernaddr", (void *)(CONFIG_SYS_TEXT_BASE + addr));

	/* Add an env variable to point to a root disk, if available */
	addr = fdtdec_get_config_int(gd->fdt_blob, "rootdisk-offset", 0);
	if (addr)
		env_set_addr("rootaddr", (void *)(CONFIG_SYS_TEXT_BASE + addr));
#endif /* CONFIG_SYS_TEXT_BASE */
}

const char *bootdelay_process(void)
{
	char *s;
	int bootdelay;

	bootcount_inc();

	s = env_get("bootdelay");
	bootdelay = (int)simple_strtol(s ? s : MK_STR(CONFIG_BOOTDELAY),
				       NULL, 10);

	if (IS_ENABLED(CONFIG_OF_CONTROL))
		bootdelay = fdtdec_get_config_int(gd->fdt_blob, "bootdelay",
						  bootdelay);

#if defined(is_boot_from_usb)
	if (is_boot_from_usb() && env_get("bootcmd_mfg")) {
		disconnect_from_pc();
		printf("Boot from USB for mfgtools\n");
		bootdelay = 0;
		env_set_default("Use default environment for \
				 mfgtools\n", 0);
	} else if (is_boot_from_usb()) {
		printf("Boot from USB for uuu\n");
		bootdelay = 0;
#ifdef CONFIG_FSL_FASTBOOT
	/* bootcmd needs to be cleared for fastboot_setup */
		env_set("bootcmd","");
	/* will determine usb from get_boot_device() */
		fastboot_setup();
#endif
	} else {
		printf("Normal Boot\n");
	}
#endif

	debug("### main_loop entered: bootdelay=%d\n\n", bootdelay);

	if (IS_ENABLED(CONFIG_AUTOBOOT_MENU_SHOW))
		bootdelay = menu_show(bootdelay);
	bootretry_init_cmd_timeout();

#ifdef CONFIG_POST
	if (gd->flags & GD_FLG_POSTFAIL) {
		s = env_get("failbootcmd");
	} else
#endif /* CONFIG_POST */
	if (bootcount_error())
		s = env_get("altbootcmd");
	else
		s = env_get("bootcmd");

#if defined(is_boot_from_usb)
	if (is_boot_from_usb() && env_get("bootcmd_mfg")) {
		s = env_get("bootcmd_mfg");
		printf("Run bootcmd_mfg: %s\n", s);
	}
#endif

	if (IS_ENABLED(CONFIG_OF_CONTROL))
		process_fdt_options(gd->fdt_blob);
	stored_bootdelay = bootdelay;

	return s;
}

#ifdef CONFIG_CMD_UPDATE
/*
 * Board-specific code can reimplement board_check_for_recover() if needed
 */
enum update_action __board_check_for_recover(void) {
	return UPDATE_ACTION_UPDATE;
}
enum update_action board_check_for_recover(void)
	__attribute__((weak, alias("__board_check_for_recover")));
#endif

void autoboot_command(const char *s)
{
	debug("### main_loop: bootcmd=\"%s\"\n", s ? s : "<UNDEFINED>");

	if (s && (stored_bootdelay == -2 ||
		 (stored_bootdelay != -1 && !abortboot(stored_bootdelay)))) {
#ifdef CONFIG_CMD_UPDATE
		/* Before the boot command is executed, check if we should
		   load a system recovery or update script; which of these
		   should be tested is platform dependend. For example a
		   special button must be pressed at boot time to start a
		   recovery. So you have to override board_check_recovery() in
		   this case. By default we only check for updates. */
		if (update_script(board_check_for_recover(), NULL, NULL, 0))
#endif
		{
			bool lock;
			int prev;

			lock = IS_ENABLED(CONFIG_AUTOBOOT_KEYED) &&
				!IS_ENABLED(CONFIG_AUTOBOOT_KEYED_CTRLC);
			if (lock)
				prev = disable_ctrlc(1); /* disable Ctrl-C checking */

			run_command_list(s, -1, 0);

			if (lock)
				disable_ctrlc(prev);	/* restore Ctrl-C checking */

#ifdef CONFIG_CMD_UPDATE
			/* The bootcmd usually only returns if booting failed.
			   Then we assume that the system is not correctly
			   installed and try to load an install script */
			update_script(UPDATE_ACTION_INSTALL, NULL, NULL, 0);
#endif
		}
	}

	if (IS_ENABLED(CONFIG_USE_AUTOBOOT_MENUKEY) &&
	    menukey == AUTOBOOT_MENUKEY) {
		s = env_get("menucmd");
		if (s)
			run_command_list(s, -1, 0);
	}
}
