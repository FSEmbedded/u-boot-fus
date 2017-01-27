/*
 *
 * Filename: checkboot.h
 *
 * Description: Program to copy UBoot to DDR-RAM location and/or authenticate
 *
 *
 * */

#ifndef __checkboot_H__
#define __checkboot_H__

#include <common.h>
#include <HAB.h>

/* UBoot IVT defines (so uboot recognizes the uboot with ivt)  */

#define IS_UBOOT(pAddr)      (__le32_to_cpup((__le32*)(pAddr + 0x3c)) == 0x12345678)
#define IS_UBOOT_IVT(pAddr)  (__le32_to_cpup((__le32*)(pAddr + 0x7c)) == 0x12345678)
#define IS_KERNEL(pAddr)     (__be32_to_cpup((__be32*)(pAddr + 0x40)) == 0x27051956)
#define IS_DEVTREE(pAddr)    (__be32_to_cpup((__be32*)(pAddr + 0x40)) == 0xd00dfeed)

int checkTarget(u32 addr, size_t bytes, const char* image);


#endif
