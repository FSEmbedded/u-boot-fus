/*
 * nand_test.c
 *
 *  Created on: May 20, 2020
 *      Author: developer
 */

#include <common.h>
#include <nand.h>
#include "selftest.h"
#include <linux/mtd/mtd.h>

int test_nand(char * szStrBuffer){

	struct mtd_info *mtd;
	uint64_t dwSize = 0;
	uint64_t nBad = 0;
	uint64_t nReserved = 0;
	uint64_t nUnknown = 0;

	/* Clear reason-string */
	szStrBuffer[0] = '\0';

	for (int i = 0; i < CONFIG_SYS_MAX_NAND_DEVICE; i++){

		mtd = get_nand_dev_by_index(i);
		if (!mtd){
			printf("NAND Flash............" );
			test_OkOrFail(-1, 1, "No access");
			return -1;
		}

		dwSize =(mtd->size >> 20);
		printf("NAND Flash: %lldMB%s....",
			 dwSize, (dwSize > 999) ? "" : (dwSize < 100) ? ".." : ".");

		for (uint64_t off = 0; off < mtd->size; off += mtd->erasesize){
			if (mtd_block_isbad(mtd, off))
				nBad++;
			if (mtd_block_isreserved(mtd, off))
				nReserved++;
		}

        sprintf(szStrBuffer, "%lld bad, %lld reserved, %lld unknown",
                     nBad, nReserved, nUnknown);

        test_OkOrFail(0, 1, szStrBuffer);

	}


	return 0;
}
