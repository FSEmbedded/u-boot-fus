/*
 * fus_sdio.c
 *
 *  Created on: May 8, 2020
 *      Author: developer
 */
#include <config.h>
#include <common.h>
#include <command.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <errno.h>
#include <mmc.h>
#include <part.h>
#include <power/regulator.h>
#include <malloc.h>
#include <memalign.h>
#include <linux/list.h>
#include <div64.h>

#include "fus_sdio.h"

#define SD_CMD_GO_IDLE_STATE		0	/* mandatory for SDIO */
#define SD_CMD_SEND_OPCOND		1
#define SD_CMD_MMC_SET_RCA		3
#define SD_CMD_IO_SEND_OP_COND		5	/* mandatory for SDIO */
#define SD_CMD_SELECT_DESELECT_CARD	7
#define SD_CMD_SEND_CSD			9
#define SD_CMD_LOCK_UNLOCK		42
#define SD_CMD_IO_RW_DIRECT		52	/* mandatory for SDIO */
#define SD_CMD_IO_RW_EXTENDED		53	/* mandatory for SDIO */


#define SD_RESPONSE_R5(x)  (x & 0xFF)

#define DATA_SZIE 50


static int mmc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
	return dm_mmc_send_cmd(mmc->dev, cmd, data);
}

static int sdio_get_cis(struct mmc *mmc, int *uiManufactureID){

	struct mmc_cmd cmd;
	int err = 0;
	u32 dwCisPointer = 0;
	u32 dwTpCode = 0x00;
	u32 dwSize = 0x00;

	u32 dwData[DATA_SZIE];

	cmd.cmdidx = SD_CMD_IO_RW_DIRECT;
	cmd.resp_type = MMC_RSP_R5;

	/* Get pointer to cis */
	for( int i=0; i<0x3; i++ )
	{
		cmd.cmdarg = (0x09 + i)<<9;
		err = mmc_send_cmd(mmc, &cmd, NULL);
		if (err)
			return err;
		dwCisPointer |= SD_RESPONSE_R5(cmd.response[0]) << (i*8);
	}

	pr_debug("dwCisPointer = 0x%x\n",dwCisPointer);

	if (dwCisPointer == 0x00){
		return -1;
	}

	while(1){

		/* Get code */
		cmd.cmdarg = ((dwCisPointer++)<<9);
		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;

		dwTpCode = SD_RESPONSE_R5(cmd.response[0]);

		/* Now we are done */
		if(dwTpCode == 0xFF)
			break;

		/* Get size */
		cmd.cmdarg = ((dwCisPointer++)<<9);
		err = mmc_send_cmd(mmc, &cmd, NULL);
		if (err)
			return err;

		dwSize = SD_RESPONSE_R5(cmd.response[0]);
		pr_debug("dwTpCode = 0x%x dwSize = 0x%x\n",dwTpCode,dwSize);

		/* Bug at Broadcom WIFI:
		 * There is a NullTuple with size 0xFF, which triggers the error dwSize >= DATA_SZIE.
		 * So even if we get CIS correctly we get an error. To prevent this, we don't handle
		 * NullTuples (0x0).
		 */
		if (dwTpCode)
		{
			/* Get data */
			if (dwSize < DATA_SZIE){
				for(int i=0; i<dwSize; i++ )
				{
					cmd.cmdarg = ((dwCisPointer++)<<9);
					err = mmc_send_cmd(mmc, &cmd, NULL);

					if (err)
						return err;

					dwData[i] = SD_RESPONSE_R5(cmd.response[0]);
					pr_debug("dwData[i] = 0x%x\n",dwData[i]);
				}
				if( dwTpCode == 0x20 )
				{
					*uiManufactureID = (((dwData[0] | (dwData[1] << 8)) & 0xFFFFFF) );
					pr_debug("uiManufactureID = 0x%x\n",*uiManufactureID);
				}
			}else
			{
				pr_debug("Data Size to big: got %i, max: %i",dwSize,DATA_SZIE);
				return -1;
			}
		}
	}

	return err;
}



static int sdio_get_funk_cis(struct mmc *mmc, u32 uFunc){

	int err;
	struct mmc_cmd cmd;
	u32 dwFuncCisPtr = 0x00;
	u32 dwTpCode = 0x00;
	u32 dwSize = 0x00;
	u32 dwData[DATA_SZIE];

	u32 uAddress = (0x100 * uFunc) + 0x09;

	cmd.cmdidx = SD_CMD_IO_RW_DIRECT;
	cmd.resp_type = MMC_RSP_R5;

	/* Get pointer to funk cis */
	for( int i=0; i<0x3; i++ )
		{
		cmd.cmdarg = (uAddress +i) << 9;
		err = mmc_send_cmd(mmc, &cmd, NULL);
		if (err)
			return err;
		dwFuncCisPtr |= SD_RESPONSE_R5(cmd.response[0]) << (i*8);
		}

	if (dwFuncCisPtr == 0x00){
		return -1;
	}

	while(1){

		/* Get code */
		cmd.cmdarg = ((dwFuncCisPtr++)<<9);
		err = mmc_send_cmd(mmc, &cmd, NULL);
		if (err)
			return err;
		dwTpCode = SD_RESPONSE_R5(cmd.response[0]);

		/* Now we are done */
		if(dwTpCode == 0xFF)
			break;

		/* Get size */
		cmd.cmdarg = ((dwFuncCisPtr++)<<9);
		err = mmc_send_cmd(mmc, &cmd, NULL);
		if (err)
			return err;
		dwSize = SD_RESPONSE_R5(cmd.response[0]);

		/* Get data */
		if (dwSize < DATA_SZIE){
			for(int i=0; i<dwSize; i++ )
			{
				cmd.cmdarg = ((dwFuncCisPtr++)<<9);
				if (err)
					return err;
				dwData[i] = SD_RESPONSE_R5(cmd.response[0]);
				pr_debug("Data %x\n",dwData[i]);
			}
		}else
		{
			pr_debug("Data Size to big: got %i, max: %i",dwSize,DATA_SZIE);
			return -1;
		}

	}
	return err;
}

static int sdio_get_rca(struct mmc *mmc){

	struct mmc_cmd cmd;
	int err;

	u32 card_address = 0x00 << 16;
	u8 *card_rca;
	u32 card_status;

	/* get rca */
	cmd.cmdidx = SD_CMD_MMC_SET_RCA;
	cmd.resp_type = MMC_RSP_R6;
	cmd.cmdarg = card_address;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		return err;

	/* rca is in bytes 2 and 3 */
	card_rca = (u8*)&cmd.response[0];
	mmc->rca = card_rca[2] | (card_rca[3]) << 8;

	/* Check card status */
	card_status = card_rca[0];
	card_status |= (card_rca[1]) << 8;
	card_status &= 0xE000;

	if (card_status)
		return -1;

	return err;

}

static int sdio_card_select(struct mmc *mmc, u8 select){

	int err;
	struct mmc_cmd cmd;

	/* Select card */
	cmd.cmdidx = SD_CMD_SELECT_DESELECT_CARD;
	cmd.resp_type = MMC_RSP_R1b;

	if (select)
		cmd.cmdarg = mmc->rca  << 16;
	else
		cmd.cmdarg = 0x0 << 16;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	return err;
}

static int sdio_get_fbr(struct mmc *mmc, u32 uFunc){

	int err;
	struct mmc_cmd cmd;
	/* get fbr  */
	u32 uAddress = 0x100*uFunc;
	cmd.cmdidx = SD_CMD_IO_RW_DIRECT;
	cmd.resp_type = MMC_RSP_R5;

	for( int i=0; i<0x12; i++ )
		{
		cmd.cmdarg = (uAddress +i) << 9;
		err = mmc_send_cmd(mmc, &cmd, NULL);
		if (err)
			return err;
		}
	return err;
}

static int sdio_send_op_cond(struct mmc *mmc)
{
	int err = 0;
	struct mmc_cmd cmd;

	cmd.cmdidx = SD_CMD_IO_SEND_OP_COND;
	cmd.resp_type = MMC_RSP_R4;
	cmd.cmdarg = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		return err;

	cmd.cmdarg  = cmd.response[0]& 0xFFFFFF;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		return err;


	if( cmd.response[0] & (1<<31) ){
		return 0;
	}
	else{
		return -1;
	}
}

int sdio_init(struct mmc *mmc, int *uiManufactureID){

	int err = 0;

	err = sdio_send_op_cond(mmc);
	if (err)
		return err;
	pr_debug("sdio_send_op_cond done\n");

	err = sdio_get_rca(mmc);
	if (err)
		return err;
	pr_debug("sdio_get_rca done \n");

	err = sdio_card_select(mmc,1);
	if (err)
		return err;
	pr_debug("sdio_card_select done\n");

	err = sdio_get_fbr(mmc,1);
	if (err)
		return err;
	pr_debug("sdio_get_fbr done \n");

	err = sdio_get_cis(mmc, uiManufactureID);
	if (err)
		return err;
	pr_debug("sdio_get_cis done \n");

	err = sdio_get_funk_cis(mmc,1);
	if (err)
		return err;
	pr_debug("sdio_get_funk_cis 1 done\n");

	err = sdio_get_funk_cis(mmc,2);
	if (err)
		return err;
	pr_debug("sdio_get_funk_cis 2 done\n");

	return err;
}
