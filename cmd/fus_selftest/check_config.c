/*
 * check_config.c
 *
 *  Created on: Mar 08, 2021
 *      Author: developer
 */
#include <common.h>
#include "check_config.h"
#include "selftest.h"
#include "../../board/F+S/common/fs_board_common.h"/* fs_board_*() */
#include "../../board/F+S/common/fs_image_common.h"/* fs_image_*() */

int has_feature(int feature){

	unsigned int board_features = fs_board_get_features();

	if ((board_features & feature))
		return 1;
	else
		return 0;
}

int get_board_fert(char *fert)
{
	char id[MAX_DESCR_LEN + 1] = "\0";
	char *tmp;
	struct fs_header_v1_0 *fsh = fs_image_get_cfg_addr();

	*fert = '\0';

	if (fs_image_match(fsh, "BOARD-CFG", NULL)) {
		memcpy(id, fsh->param.descr, MAX_DESCR_LEN);
		id[MAX_DESCR_LEN] = '\0';
	}
	else
		return -1;

	tmp = strchr(id, '.');
	if (tmp) {
		*tmp = '\0';
		tmp = strchr(id, '-');
		if (tmp) {
			tmp++;
			for (int i=0; tmp[i]; i++) {
				/* To lowercase */
				if (('A' <= tmp[i]) && (tmp[i] <= 'Z'))
					tmp[i] += 32;
			}
			strcpy(fert, tmp);
		}
	}

	return 0;
}
