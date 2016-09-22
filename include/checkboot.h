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

int CheckIfUBoot(int argc, char*const argv[], int *idx, loff_t *off, loff_t *size, loff_t *maxsize, ulong addr);
//static int Init_HAB(ulong addr);
//static u32 CheckUBoot(ulong addr);
int Init_HAB(ulong addr);
u32 CheckUBoot(ulong addr);
#endif
