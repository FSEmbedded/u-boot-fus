// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2026 F&S Elektronik Systeme GmbH
 *
 * Linux version of fdt_print.c
 */
#include <stdio.h>			/* Must be included before puts macro */

/* Avoid extra newline of puts() in Linux libc */
#define puts(arg) printf("%s", arg)

#include "../../common/fdt_print.c"
