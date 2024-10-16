/*
 * (C) Copyright 2023
 * F&S Elektronik Systeme GmbH
 *
 * common code for ls1028a
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/io.h>
#include <fdt_support.h>
#include <asm/arch-fsl-layerscape/soc.h>

#include "fs_ls1028a_common.h"
#include "fs_eth_common.h"
#include "fs_common.h"
#include "fs_fdt_common.h"

/**
 * @brief read SFP_OUIDR3 register for fused boardtype
 * @return enum board_type <type>
 * @details
 * Board-Type is saved within:
 * SFP_OUIDR3[31:29]
 */
enum board_type fs_get_board()
{
	const uint32_t * const reg = (const uint32_t * const)SFP_OUIDR3_ADDR;
	return (enum board_type)(__raw_readl(reg) >> 29);
}


/**
 * @brief read SFP_OUIDR3 register for fused board-rev
 * @return enum board_rev <rev>
 * @details
 * Board-Revision is saved within:
 * SFP_OUIDR3[28:26]
 */
enum board_rev fs_get_board_rev()
{
	const uint32_t * const reg = (const uint32_t * const)SFP_OUIDR3_ADDR;
	return (enum board_type)((__raw_readl(reg) >> 26) & 0x7);
}

/**
 * @brief read SFP_OUIDR3 register for fused configuration
 * @return enum board_config <config>
 * @details
 * Config-Information is saved within:
 * SFP_OUIDR3[25:22]
 */
enum board_config fs_get_board_config()
{
	const uint32_t * const reg = (const uint32_t * const)SFP_OUIDR3_ADDR;
	return (enum board_config)((__raw_readl(reg) >> 22) & 0xF);
}

static inline uint32_t get_gal1_features(enum board_rev brev, enum board_config bconfig)
{
	uint32_t features = 0;

	/* default features for all revisions */
	features |= FEAT_GAL_ETH_SFP;
	features |= FEAT_GAL_RS485A;
	features |= FEAT_USB1;
	features |= FEAT_USB2;

	if(brev == REV10){
		/*default features for all Configs*/
		features |= FEAT_I2C_TEMP;

		switch(bconfig){
			case FERT1:
				features |= FEAT_GAL_RS232;
				features |= FEAT_GAL_ETH_INTERN_BASET;
				break;
			case FERT2:
				features |= FEAT_GAL_RS485B;
				features |= FEAT_GAL_ETH_INTERN_BASEX;
				features |= FEAT_I2C6;
				break;
			case FERT3:
				features |= FEAT_GAL_RS485B;
				features |= FEAT_GAL_ETH_INTERN_BASEX;
				break;
			default:
				break;
		}
	}

	if(brev >= REV11 && brev <= REV12){
		/*default features for all Configs*/
		features |= 0;

		switch(bconfig){
			case FERT1:
				features |= FEAT_GAL_ETH_INTERN_BASET;
				features |= FEAT_GAL_RS485B;
				break;
			case FERT2:
				features |= FEAT_GAL_RS485B;
				break;
			case FERT3:
				features |= FEAT_GAL_ETH_INTERN_BASEX;
				features |= FEAT_GAL_RS232;
				break;
			default:
				break;
		}
	}

	return features;
}

static inline uint32_t get_gal2_features(enum board_rev brev, enum board_config bconfig)
{
	/* default features for all revisions */
	uint32_t features = 0;

	features |= FEAT_GAL_ETH_RJ45_1;
	features |= FEAT_GAL_ETH_RJ45_4;
	features |= FEAT_GAL_ETH_SFP;
	features |= FEAT_GAL_RS485A;
	features |= FEAT_USB1;
	features |= FEAT_USB2;

	if(brev == REV10){
		/*default features for all Configs*/
		features |= 0;

		switch(bconfig){
			case FERT1:
				features |= FEAT_GAL_ETH_INTERN_BASET;
				features |= FEAT_GAL_RS485B;
				break;
			case FERT2:
				features |= FEAT_GAL_ETH_INTERN_BASEX;
				features |= FEAT_GAL_RS232;
				break;
			case FERT3:
				features |= FEAT_GAL_ETH_INTERN_BASEX;
				features |= FEAT_GAL_RS232;
				break;
			default:
				break;
		}
	}

	return features;
}

/**
 * @brief Read boardtype and config to determind given features
 * @return uint32_t: A feature represents one specific bit
 */
uint32_t fs_get_board_features()
{
	enum board_type btype;
	enum board_rev brev;
	enum board_config bconfig;
	uint32_t features = 0;

	btype = fs_get_board();
	brev = fs_get_board_rev();
	bconfig = fs_get_board_config();

	/*default features for all Board-Types*/
	features |= FEAT_GAL_ETH_RJ45_2;
	features |= FEAT_GAL_ETH_RJ45_3;
	features |= FEAT_I2C1;
	features |= FEAT_GAL_MMC;

	switch(btype){
		case GAL1:
			features |= get_gal1_features(brev, bconfig);
			break;
		case GAL2:
			features |= get_gal2_features(brev, bconfig);
		break;
	}

	return features;
}

void fs_get_modelname(char* buf, int buf_size){
	enum board_type btype = fs_get_board();
	enum board_rev brev = fs_get_board_rev();
	enum board_config bconfig = fs_get_board_config();

	switch(btype)
	{
		case GAL1:
			strlcpy(buf, "GAL1 ", buf_size);
			break;
		case GAL2:
			strlcpy(buf, "GAL2 ", buf_size);
			break;
		default:
			strlcpy(buf, "UNKNOWN ", buf_size);
	}

	strlcat(buf,"REV", buf_size);

	switch(brev)
	{
		case REV10:
			strlcat(buf, "1.00 ", buf_size);
			break;
		case REV11:
			strlcat(buf, "1.10 ", buf_size);
			break;
		case REV12:
			strlcat(buf, "1.20 ", buf_size);
			break;
		default:
			strlcat(buf, "UNKNOWN ", buf_size);
	}

	strlcat(buf,"H", buf_size);

	switch(bconfig)
	{
		case FERT1:
			strlcat(buf, "1", buf_size);
			break;
		case FERT2:
			strlcat(buf, "2", buf_size);
			break;
		case FERT3:
			strlcat(buf, "3", buf_size);
			break;
		case FERT4:
			strlcat(buf, "4", buf_size);
			break;
		case FERT5:
			strlcat(buf, "5", buf_size);
			break;
		case FERT6:
			strlcat(buf, "6", buf_size);
			break;
		case FERT7:
			strlcat(buf, "7", buf_size);
			break;
		case FERT8:
			strlcat(buf, "8", buf_size);
			break;
		case FERT9:
			strlcat(buf, "9", buf_size);
			break;
		case FERT10:
			strlcat(buf, "10", buf_size);
			break;
		case FERT11:
			strlcat(buf, "11", buf_size);
			break;
		case FERT12:
			strlcat(buf, "12", buf_size);
			break;
		case FERT13:
			strlcat(buf, "13", buf_size);
			break;
		case FERT14:
			strlcat(buf, "14", buf_size);
			break;
		case FERT15:
			strlcat(buf, "15", buf_size);
			break;
		case FERT16:
			strlcat(buf, "16", buf_size);
			break;
		default:
			strlcat(buf, "UNKNOWN", buf_size);
	}
}

void fs_fdt_board_setup(void *blob)
{
	uint32_t features;
	char str_buf[MODELSTRLEN] = {0};
	int namelen;
	int ret = 0;

	features = fs_get_board_features();

	/* Realloc FDT-Blob to next full page-size.
	 * If NOSPACE Error appiers, increase extrasize.
	 */
	ret = fdt_shrink_to_minimum(blob, 0);
	if(ret < 0){
		printf("failed to shrink FDT-Blob: %s", fdt_strerror(ret));
	}

	/* Setup Network Interfaces */
	if(!(features & FEAT_ETH_ENETC0)){
		fs_fdt_enable_node_by_label(blob, "enetc_port0",0);
	}else{
		fs_add_mac_prop(blob, "enetc_port0");
	}

	if(!(features & FEAT_ETH_ENETC1)){
		fs_fdt_enable_node_by_label(blob, "enetc_port1",0);
	}else{
		fs_add_mac_prop(blob, "enetc_port1");
	}

	/* Disable TSN-Switch, when it is not used */
	if(!(features & FEAT_ETH_SWP1) &&
		!(features & FEAT_ETH_SWP2) &&
		!(features & FEAT_ETH_SWP3) &&
		!(features & FEAT_ETH_SWP4))
	{
		fs_fdt_enable_node_by_label(blob, "enetc_port2", 0);
		fs_fdt_enable_node_by_label(blob, "enetc_port3", 0);
		fs_fdt_enable_node_by_label(blob, "mscc_felix_port4", 0);
		fs_fdt_enable_node_by_label(blob, "mscc_felix_port5",0);
		fs_fdt_enable_node_by_label(blob, "mscc_felix",0);
	}

	if(!(features & FEAT_ETH_SWP1)){
		fs_fdt_enable_node_by_label(blob, "mscc_felix_port0",0);

	}else{
		fs_add_mac_prop(blob, "mscc_felix_port0");
	}

	if(!(features & FEAT_ETH_SWP2)){
		fs_fdt_enable_node_by_label(blob, "mscc_felix_port1",0);
	}else{
		fs_add_mac_prop(blob, "mscc_felix_port1");
	}

	if(!(features & FEAT_ETH_SWP3)){
		fs_fdt_enable_node_by_label(blob, "mscc_felix_port2",0);
	}else{
		fs_add_mac_prop(blob, "mscc_felix_port2");
	}

	if(!(features & FEAT_ETH_SWP4)){
		fs_fdt_enable_node_by_label(blob, "mscc_felix_port3",0);
	}else{
		fs_add_mac_prop(blob, "mscc_felix_port3");
	}

	/* Setup UART */
	if (features & FEAT_LPUART1) {
		fs_fdt_enable_node_by_label(blob, "lpuart0", 1);
	}

	if (features & FEAT_LPUART1_RS485){
		fs_fdt_setprop_by_label(blob, "lpuart0",
			"linux,rs485-enabled-at-boot-time", NULL, 0);
	}

	if (features & FEAT_LPUART2){
		fs_fdt_enable_node_by_label(blob, "lpuart1", 1);
	}

	if (features & FEAT_LPUART3) {
		fs_fdt_enable_node_by_label(blob, "lpuart2", 1);
	}

	if (features & FEAT_LPUART3_RS485){
		fs_fdt_setprop_by_label(blob, "lpuart2",
		"linux,rs485-enabled-at-boot-time", NULL, 0);
	}

	if (features & FEAT_LPUART4) {
		fs_fdt_enable_node_by_label(blob, "lpuart3", 1);
	}

	if (features & FEAT_LPUART5) {
		fs_fdt_enable_node_by_label(blob, "lpuart4", 1);
	}

	if (features & FEAT_LPUART6) {
		fs_fdt_enable_node_by_label(blob, "lpuart5", 1);
	}

	/* Setup I2C */
	if (features & FEAT_I2C1){
		fs_fdt_enable_node_by_label(blob, "i2c0",1);
	}

	if (features & FEAT_I2C2){
		fs_fdt_enable_node_by_label(blob, "i2c1",1);
	}

	if (features & FEAT_I2C3){
		fs_fdt_enable_node_by_label(blob, "i2c2",1);
	}

	if (features & FEAT_I2C4){
		fs_fdt_enable_node_by_label(blob, "i2c3",1);
	}

	if (features & FEAT_I2C5){
		fs_fdt_enable_node_by_label(blob, "i2c4", 1);
	}

	if (features & FEAT_I2C6){
		fs_fdt_enable_node_by_label(blob, "i2c5", 1);
	}

	/* Setup USB */
	if (features & FEAT_USB1){
		fs_fdt_enable_node_by_label(blob, "usb0", 1);
	}

	if (features & FEAT_USB2){
		fs_fdt_enable_node_by_label(blob, "usb1", 1);
	}

	/* Setup ESDHC */
	if (features & FEAT_ESDHC1){
		fs_fdt_enable_node_by_label(blob, "esdhc1",1);
	}

	/* fixup /model */
	fs_get_modelname(str_buf, MODELSTRLEN);
	namelen = strlen(str_buf);
	fs_fdt_setprop_by_path(blob, "/", "model", (const char *)str_buf, namelen);

	cpu_name(str_buf);
	namelen = strlen(str_buf);
	fs_fdt_setprop_by_path(blob, "/", "cpu-model", (const char *)str_buf, namelen);
}

void fs_linuxfdt_board_setup(void *blob){
	uint32_t features;

	features = fs_get_board_features();

	/* On GAL-Boards the SD-Adapter is not supposed to be used
	 * in Linux. The SD-Adapter should only be used for 
	 * Board-Bringup in U-Boot.
	 */
	if(!(features & FEAT_ESDHC0)){
		fs_fdt_enable_node_by_label(blob, "esdhc",0);
	}

	if(features & FEAT_I2C_TEMP){
		fs_fdt_enable_node_by_label(blob, "temp-sensor",1);
	}
}

void fs_ubootfdt_board_setup(void *blob){
	uint32_t features;

	features = fs_get_board_features();

	/* On GAL-Boards the SD-Adapter can be always used in U-Boot.
	 * This provides the opportunity for a emergency bringup.
	 */
	if (get_boot_src() == BOOT_SOURCE_SD_MMC || features & FEAT_ESDHC0){
		fs_fdt_enable_node_by_label(blob, "esdhc",1);
	}
}
