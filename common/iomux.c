// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2008
 * Gary Jennejohn, DENX Software Engineering GmbH, garyj@denx.de.
 */

#include <common.h>
#include <console.h>
#include <serial.h>
#include <malloc.h>

void iomux_printdevs(const int console)
{
	int i;
	struct stdio_dev *dev;

	for_each_console_dev(i, console, dev)
		printf("%s ", dev->name);
	printf("\n");
}

int iomux_match_device(struct stdio_dev **set, const int n, struct stdio_dev *sdev)
{
	int i;

	for (i = 0; i < n; i++)
		if (sdev == set[i])
			return i;
	return -ENOENT;
}

/* This tries to preserve the old list if an error occurs. */
int iomux_doenv(const int console, const char *arg)
{
	char *console_args, *temp, **start;
	int i, j, io_flag, cs_idx, repeat;
	struct stdio_dev **cons_set, **old_set;
	struct stdio_dev *dev;

	console_args = strdup(arg);
	if (console_args == NULL)
		return 1;
	/*
	 * Check whether a comma separated list of devices was
	 * entered and count how many devices were entered.
	 * The array start[] has pointers to the beginning of
	 * each device name (up to MAX_CONSARGS devices).
	 *
	 * Have to do this twice - once to count the number of
	 * commas and then again to populate start.
	 */
	i = 0;
	temp = console_args;
	for (;;) {
		/* There's always one entry more than the number of commas. */
		i++;

		temp = strchr(temp, ',');
		if (temp == NULL)
			break;

		temp++;
	}
	start = (char **)malloc(i * sizeof(char *));
	if (start == NULL) {
		free(console_args);
		return 1;
	}
	i = 0;
	start[0] = console_args;
	for (;;) {
		temp = strchr(start[i++], ',');
		if (temp == NULL)
			break;
		*temp = '\0';
		start[i] = temp + 1;
	}
	cons_set = (struct stdio_dev **)calloc(i, sizeof(struct stdio_dev *));
	if (cons_set == NULL) {
		free(start);
		free(console_args);
		return 1;
	}

	io_flag = stdio_file_to_flags(console);
	if (io_flag < 0) {
		free(start);
		free(console_args);
		free(cons_set);
		return 1;
	}

	cs_idx = 0;
	for (j = 0; j < i; j++) {
		/*
		 * Check whether the device exists and is valid.
		 * console_assign() also calls console_search_dev(),
		 * but I need the pointer to the device.
		 */
		dev = console_search_dev(io_flag, start[j]);
		if (dev == NULL)
			continue;
		/*
		 * Prevent multiple entries for a device.
		 */
		 repeat = iomux_match_device(cons_set, cs_idx, dev);
		 if (repeat >= 0)
			continue;
		/*
		 * Try assigning the specified device.
		 * This could screw up the console settings for apps.
		 */
		if (console_assign(console, start[j]) < 0)
			continue;
		cons_set[cs_idx++] = dev;
	}
	free(console_args);
	free(start);
	/* failed to set any console */
	if (cs_idx == 0) {
		free(cons_set);
		return 1;
	}

	old_set = console_devices[console];
	repeat = cd_count[console];

	console_devices[console] = cons_set;
	cd_count[console] = cs_idx;

	/* Stop dropped consoles */
	for (i = 0; i < repeat; i++) {
		j = iomux_match_device(cons_set, cs_idx, old_set[i]);
		if (j == cs_idx)
			console_stop(console, old_set[i]);
	}

	free(old_set);
	return 0;
}

int iomux_replace_device(const int console, const char *old, const char *new)
{
	struct stdio_dev *dev;
	char *arg = NULL;	/* Initial empty list */
	int size = 1;		/* For NUL terminator */
	int i, ret;

	for_each_console_dev(i, console, dev) {
		const char *name = strcmp(dev->name, old) ? dev->name : new;
		char *tmp;

		/* Append name with a ',' (comma) separator */
		tmp = realloc(arg, size + strlen(name) + 1);
		if (!tmp) {
			free(arg);
			return -ENOMEM;
		}

		if (arg) {
			strcat(tmp, ",");
			strcat(tmp, name);
		}
		else
			strcpy(tmp, name);

		arg = tmp;
		size = strlen(tmp) + 1;
	}

	ret = iomux_doenv(console, arg);
	if (ret)
		ret = -EINVAL;

	free(arg);
	return ret;
}
