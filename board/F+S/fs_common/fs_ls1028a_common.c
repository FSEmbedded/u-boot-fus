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
#include "fs_ls1028a_common.h"
#include "fs_eth_common.h"
#include "fs_common.h"
#include "fs_fdt_common.h"

/**
 * @brief read SFP_OUIDR3 register for fused boardtype
 * @return enum board_type <type>
 * @details
 * Board-Type is saved within:
 *        SFP_OUIDR3[31:29]
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
 *        SFP_OUIDR3[28:26]
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
 *        SFP_OUIDR3[25:22]
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
     features |= FEAT_I2C1;
     features |= FEAT_I2C2;
     features |= FEAT_GAL_MMC;
	features |= FEAT_GAL_SD;
     
     if(brev == REV10){
          /*default features for all Configs*/
          features |= 0;

          switch(bconfig){
               case H1:
                    features |= FEAT_GAL_RS232;
                    features |= FEAT_GAL_ETH_INTERN_BASET;
                    break;
               case H2:
                    features |= FEAT_GAL_RS485B;
                    features |= FEAT_GAL_ETH_INTERN_BASEX;
                    break;
               case H3:
                    features |= FEAT_GAL_RS485B;
                    features |= FEAT_GAL_ETH_INTERN_BASEX;
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

     if(brev == REV10){
          /*default features for all Configs*/
          features |= 0;

          switch(bconfig){
               case H1:
                    features |= 0;
                    break;
               case H2:
                    features |= 0;
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

void fs_fdt_board_setup(void *blob)
{
     uint32_t features;
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
		ret = fs_fdt_enable_node_by_label(blob, "enetc_port0",0);
		if(ret){
			printf("ERROR: Failed to disable eth0: %s\n",
                         fdt_strerror(ret));
		}
	}else{
          ret = fs_add_mac_prop(blob, "enetc_port0");
		if(ret){
			printf("ERROR: Failed to add MAC-property on eth0: %s\n",
                         fdt_strerror(ret));
		}
     }

	if(!(features & FEAT_ETH_ENETC1)){

		ret = fs_fdt_enable_node_by_label(blob, "enetc_port1",0);
		if(ret) {
			printf("ERROR: Failed to disable eth1: %s\n",
                         fdt_strerror(ret));
		}
	}else{
          ret = fs_add_mac_prop(blob, "enetc_port1");
		if(ret){
			printf("ERROR: Failed to add MAC-property on eth1: %s\n",
                         fdt_strerror(ret));
		}
     }

     /* Disable TSN-Switch, when it is not used */
	if(!(features & FEAT_ETH_SWP1) &&
		!(features & FEAT_ETH_SWP2) &&
		!(features & FEAT_ETH_SWP3) &&
		!(features & FEAT_ETH_SWP4))
	{
		ret = fs_fdt_enable_node_by_label(blob, "enetc_port2", 0);
		if(ret) {
			printf("ERROR: Failed to disable eth2: %s\n",
                         fdt_strerror(ret));
		}

		ret = fs_fdt_enable_node_by_label(blob, "enetc_port3", 0);
		if(ret) {
			printf("ERROR: Failed to disable eth7: %s\n",
                         fdt_strerror(ret));
		}
		ret = fs_fdt_enable_node_by_label(blob, "mscc_felix_port4", 0);
		if(ret) {
			printf("ERROR: Failed to disable TSN-Switch Port 4: %s\n",
                         fdt_strerror(ret));
		}

		ret = fs_fdt_enable_node_by_label(blob, "mscc_felix_port5",0);
		if(ret) {
			printf("ERROR: Failed to disable TSN-Switch Port 5: %s\n",
                         fdt_strerror(ret));
		}

		ret = fs_fdt_enable_node_by_label(blob, "mscc_felix",0);
		if(ret) {
			printf("ERROR: Failed to disable TSN-Switch: \n");
		}
	}

	if(!(features & FEAT_ETH_SWP1)){
		ret = fs_fdt_enable_node_by_label(blob, "mscc_felix_port0",0);
		if(ret) {
			printf("ERROR: Failed to disable eth3: %s\n",
                         fdt_strerror(ret));
		}
	}else{
          ret = fs_add_mac_prop(blob, "mscc_felix_port0");
		if(ret){
			printf("ERROR: Failed to add MAC-property on eth3: %s\n",
                         fdt_strerror(ret));
          }
     }

	if(!(features & FEAT_ETH_SWP2)){

		ret = fs_fdt_enable_node_by_label(blob, "mscc_felix_port1",0);
		if(ret) {
			printf("ERROR: Failed to disable eth4: %s\n",
                         fdt_strerror(ret));
		}
	}else{
          ret = fs_add_mac_prop(blob, "mscc_felix_port1");
		if(ret){
			printf("ERROR: Failed to add MAC-property on eth4: %s\n",
                         fdt_strerror(ret));
          }
     }

	if(!(features & FEAT_ETH_SWP3)){
		ret = fs_fdt_enable_node_by_label(blob, "mscc_felix_port2",0);
		if(ret) {
			printf("ERROR: Failed to disable eth5: %s\n",
                         fdt_strerror(ret));
		}
	}else{
          ret = fs_add_mac_prop(blob, "mscc_felix_port2");
		if(ret){
			printf("ERROR: Failed to add MAC-property on eth5: %s\n",
                         fdt_strerror(ret));
          }
     }

	if(!(features & FEAT_ETH_SWP4)){
		ret = fs_fdt_enable_node_by_label(blob, "mscc_felix_port3",0);
		if(ret) {
			printf("ERROR: Failed to disable eth6: %s\n",
                         fdt_strerror(ret));
		}
	}else{
          ret = fs_add_mac_prop(blob, "mscc_felix_port3");
		if(ret){
			printf("ERROR: Failed to add MAC-property on eth6: %s\n",
                         fdt_strerror(ret));
          }
     }

     /* Setup UART */
     if (features & FEAT_LPUART1) {
          ret = fs_fdt_enable_node_by_label(blob, "lpuart0", 1);
          if(ret) {
               printf("ERROR: Failed to enable lpuart0: %s",
                         fdt_strerror(ret));
          }
     }

     if (features & FEAT_LPUART3) {
          ret = fs_fdt_enable_node_by_label(blob, "lpuart2", 1);
          if(ret) {
               printf("ERROR: Failed to enable lpuart2: %s",
                         fdt_strerror(ret));
          }
     }

     /* Setup I2C */
     if (features & FEAT_I2C1){
          ret = fs_fdt_enable_node_by_label(blob, "i2c0",1);
          if(ret){
               printf("ERROR: Failed to enable i2c0: %s",
                         fdt_strerror(ret));
          }
     }

     if (features & FEAT_I2C2){
          ret = fs_fdt_enable_node_by_label(blob, "i2c1",1);
          if(ret){
               printf("ERROR: Failed to enable i2c1: %s",
                         fdt_strerror(ret));
          }
     }

     if (features & FEAT_I2C3){
          ret = fs_fdt_enable_node_by_label(blob, "i2c2",1);
          if(ret){
               printf("ERROR: Failed to enable i2c2: %s",
                         fdt_strerror(ret));
          }
     }

     if (features & FEAT_I2C4){
          ret = fs_fdt_enable_node_by_label(blob, "i2c3",1);
          if(ret){
               printf("ERROR: Failed to enable i2c3: %s",
                         fdt_strerror(ret));
          }
     }

     if (features & FEAT_I2C5){
          ret = fs_fdt_enable_node_by_label(blob, "i2c4", 1);
          if(ret){
               printf("ERROR: Failed to enable i2c4: %s",
                         fdt_strerror(ret));
          }
     }

     if (features & FEAT_I2C6){
          ret = fs_fdt_enable_node_by_label(blob, "i2c5", 1);
          if(ret){
               printf("ERROR: Failed to enable i2c5: %s",
                         fdt_strerror(ret));
          }
     }

     /* Setup MMC */
     if (!(features & FEAT_GAL_SD)){
          ret = fs_fdt_enable_node_by_label(blob, "esdhc",0);
          if(ret){
               printf("ERROR: Failed to disable esdhc: %s",
                         fdt_strerror(ret));
          }
     }

     if (!(features & FEAT_GAL_MMC)){
          ret = fs_fdt_enable_node_by_label(blob, "esdhc1",0);
          if(ret){
               printf("ERROR: Failed to disable esdhc1: %s",
                         fdt_strerror(ret));
          }
     }
}

void fs_linuxfdt_board_setup(void *blob){

}

void fs_ubootfdt_board_setup(void *blob){

}
