// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2026 F&S Elektronik Systeme GmbH
 *
 * Linux version of fs_image_common.c.
 */
#include <linux/kconfig.h>		/* Get kconfig macros only */
#include <linux/libfdt.h>
#include "linux_helpers.h"
#include <asm/mach-imx/hab.h>		/* struct ivt, ... */
#include <stdio.h>
#include <errno.h>
#include "crc32.h"

/* Include the original file */
#include "../../board/F+S/common/fs_image_common.c"
