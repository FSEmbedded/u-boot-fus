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

int has_feature(int feature){

	unsigned int board_features = fs_board_get_features();

	if ((board_features & feature))
		return 1;
	else
		return 0;
}


