/*
 * (C) Copyright 2023
 * F&S Elektronik Systeme GmbH
 *
 * Common ETH code used on GAL boards
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <net.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <fdt_support.h>
#include "fs_eth_common.h"
#include "fs_common.h"
#include "fs_fdt_common.h"

DECLARE_GLOBAL_DATA_PTR;

/**
 * @brief Get total number of addresses
 * 
 * @param reg Fuse-Address for MAC
 * @return uint8_t Value
 * 
 * @details
 * The Number is stored within the third SFP-OUID Register
 *      3 Bit in SFP_OUIDR3[18:16]
 */
static uint8_t get_macaddr_count(const u32 * const reg){
    return (uint8_t)((__raw_readl(reg) >> 16) & 0x7);
}

/**
 * @brief Get the first Mac-Address
 * 
 * @param reg Fuse-Address for MAC
 * @return uint64_t Value
 * 
 * The MAC-address is stored in OEM Unique ID/Scratch Pad Fuse Register 3-4
 * within SFP
 * 
 * @details
 * The first register stores the first 2 Bytes from vendor address.
 * The second register stores the last 4 Bytes from MAC-Address
 * 
 * Register-MAP:
 *      Byte 1 in SFP_OUIDR3[15:8]
	    Byte 2 in SFP_OUIDR3[7:0]
        Byte 3 in SFP_OUIDR4[31:24]
        Byte 4 in SFP_OUIDR4[23:16]
        Byte 5 in SFP_OUIDR4[15:8]
        Byte 6 in SFP_OUIDR4[7:0]
 */
static uint64_t get_macaddr(const u32 * const reg){
    uint64_t val;

    val = (uint64_t)(__raw_readl(reg) & 0x0000FFFF) << 32;
    val |= __raw_readl(reg+1);
    return val;
}

/**
 * @brief write Mac address as a String in environment
 * 
 * @param macaddr MAC-ADDR as a 64 bit value
 * @param index current number of MAC-Address
 * @return 0 if OK, other value on error
 */
static int write_macaddr(uint64_t macaddr, uint8_t index){
    uchar addr_buf[6];

    addr_buf[0] = (uchar)(macaddr >> 40) & 0xFF;
    addr_buf[1] = (uchar)(macaddr >> 32) & 0xFF;
    addr_buf[2] = (uchar)(macaddr >> 24) & 0xFF;
    addr_buf[3] = (uchar)(macaddr >> 16) & 0xFF;
    addr_buf[4] = (uchar)(macaddr >> 8) & 0xFF;
    addr_buf[5] = (uchar) macaddr & 0xFF;

    return eth_env_set_enetaddr_by_index("eth",index, addr_buf);
}

/**
 * @brief Set Environment Variables for all MAC_Addresses in UBOOT 
 * 
 */
void fs_set_macaddrs(){
    const u32 * const ouid_reg3 = (const u32 * const)SFP_OUIDR3_ADDR;
    uint64_t macaddr;   //fused MAC-Address
    uint8_t mac_cnt;    //Number of MAC-Addresses
    const void *fdt = gd->fdt_blob;
    char alias[5] = {"ethX"}; //Alias to look for eth-node

    mac_cnt = get_macaddr_count(ouid_reg3);
    macaddr = get_macaddr(ouid_reg3);

    /*
     * To ensure a clear relationship between ethernet index
     * and device, the alias names are used in the fdt to set
     * the correct environment.
     */
    
    int index=-1;
    while(mac_cnt>0){
        const char * path;
        int node;

        index++;
        alias[3] = '0' + index; //Set correct alias for lookup

        /*Break loop, when alias/node is not found*/
        path = fdt_get_alias(fdt,alias);
        if(!path){
            break;
        }

        node = fdt_path_offset(fdt,path);
        if(!node){
            break;
        }

        /* Not every eth-dev gets its own MAC */
        if(!fdt_get_property(fdt, node, "fused_mac", NULL)){
            continue;
        }

        /* set mac-addr */
        write_macaddr(macaddr, index);
        
        macaddr++;
        mac_cnt--;
    };

}

int fs_add_mac_prop(void *blob, const char* label)
{
    const char* path;
    int ofnode, ret;
               
    path = fs_fdt_get_label(blob, label);
    ofnode = fdt_path_offset((const void*)blob,path);
    if(ofnode < 0)
        return ofnode;

    ret = fdt_setprop(blob, ofnode, "fused_mac", NULL, 0);
    return ret;
}