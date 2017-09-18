/*
 *
 * Filename: checkboot.h
 *
 * Description: Program to copy UBoot to DDR-RAM location and/or authenticate
 *
 *
 */

#ifndef __checkboot_H__
#define __checkboot_H__


#include <common.h>


/* UBoot IVT defines (so uboot recognizes the uboot with ivt)  */
#define IS_UBOOT(pAddr)      	(__le32_to_cpup((__le32*)(pAddr + 0x3c)) == 0x12345678)
#define IS_UBOOT_IVT(pAddr)  	(__le32_to_cpup((__le32*)(pAddr + 0x3c + HAB_HEADER)) == 0x12345678)
#define IS_KERNEL(pAddr)     	(__be32_to_cpup((__be32*)(pAddr)) == 0x27051956)
#define IS_KERNEL_IVT(pAddr)	(__be32_to_cpup((__be32*)(pAddr + HAB_HEADER)) == 0x27051956)
#define IS_DEVTREE(pAddr)	(__be32_to_cpup((__be32*)(pAddr)) == 0xd00dfeed)
#define IS_DEVTREE_IVT(pAddr)	(__be32_to_cpup((__be32*)(pAddr + HAB_HEADER)) == 0xd00dfeed)

#define HAB_HEADER         0x40


typedef struct boot_data {
  u32*            start;        /* start of image in RAM                            */
  u32             length;       /* length of complete image                         */
  u32             plugin_flag;  /* plugin flag                                      */
  u32*            reserved1;    /* reserved 1 pointer (free to use)                 */
  u32*            reserved2;    /* reserved 2 pointer (free to use)                 */
  u32*            reserved3;    /* reserved 3 pointer (free to use)                 */
  u32*            reserved4;    /* reserved 4 pointer (free to use)                 */
  u32*            reserved5;    /* reserved 5 pointer (free to use)                 */
} boot_data_t;


typedef struct ivt_header {
  u32            header;        /* IVT Header                                       */
  u32*           entry;         /* IVT entry pointer                                */
  u32            reserved1;     /* IVT reserved1                                    */
  u32*           dcd;           /* IVT dcd pointer                                  */
  boot_data_t*   boot_data;     /* IVT boot data pointer                            */
  u32*           self;          /* IVT self pointer                                 */
  u32*           csf;           /* IVT csf pointer                                  */
  u32            reserved2;     /* IVT reserved2                                    */
} ivt_header_t;


typedef enum eLoaderType {
	LOADER_NONE = 0,
	LOADER_UBOOT = 1,
	LOADER_UBOOT_IVT = 2,
	LOADER_KERNEL = 4,
	LOADER_KERNEL_IVT = 8,
	LOADER_FDT = 16,
	LOADER_FDT_IVT = 32,
} LOADER_TYPE;


typedef enum eOptions {
    DO_NOTHING   = 0x0,
    BACKUP_IMAGE = 0x1,
    CUT_IVT      = 0x2,
} OPTIONS;


LOADER_TYPE GetLoaderType(u32 addr);
void memExchange(u32 srcaddr, u32 dstaddr, u32 length);
u32 makeSaveCopy(u32 srcaddr, u32 length);
u32 getImageLength(u32 addr);
int check_flash_partition(char *img_name, loff_t *off, u32 length);
int checkTarget(u32 addr, size_t bytes, const char* image);
bool Init_HAB(u32 addr, OPTIONS eOption, loff_t *off, loff_t *size);


#endif
