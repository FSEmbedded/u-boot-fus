// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2026 F&S Elektronik Systeme GmbH
 *
 * Linux version of fs_cntr_common.c.
 */
#include <linux/kconfig.h>		/* Get kconfig macros only */
#include <linux/libfdt.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../include/u-boot/sha256.h"
#include "../../include/u-boot/sha512.h"
#include "linux_helpers.h"

/* Include the original file */
#include "../../board/F+S/common/fs_cntr_common.c"
