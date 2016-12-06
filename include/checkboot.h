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

#define BOOT_OFFS        0x40

/* UBoot IVT defines (so tftp recognizes the uboot with ivt)  */
#define IS_UBOOT(pAddr)      (*(u32*)(pAddr + 0x3c) == 0x12345678)
#define IS_UBOOT_IVT(pAddr)  (*(u32*)(pAddr + 0x5c) == 0x12345678)
/* IH_MAGIC changed endian style */
#define IS_KERNEL(pAddr)     (*(u32*)(pAddr + 0x40) == 0x56190527)


#include <common.h>
#include <HAB.h>

int CheckIfUBoot(int argc, char*const argv[], int *idx, loff_t *off, loff_t *size, loff_t *maxsize, ulong addr);
//static int Init_HAB(ulong addr);
//static u32 CheckUBoot(ulong addr);
int Init_HAB(ulong addr);
u32 CheckUBoot(ulong addr);
int checkTarget(u32 addr, size_t bytes, const char* image);


#endif
