#ifndef __IVT_H__
#define __IVT_H__

#include <common.h>
#include <HAB.h>
#include <image.h>
#include <checkboot.h>

#define BOOT_DATA_OFFSET   0x20
#define HAB_HEADER         0x40

#ifdef CONFIG_FSIMX6
#define UBOOT_CHECK_ADDR   0x10100000
#define KERNEL_CHECK_ADDR  0x10800000
#define DEVTREE_CHECK_ADDR 0x11000000
#endif
#ifdef CONFIG_FSIMX6UL
#define UBOOT_CHECK_ADDR   0x80100000
#define KERNEL_CHECK_ADDR  0x80800000
#define DEVTREE_CHECK_ADDR 0x81000000
#endif
#ifdef CONFIG_FSIMX6SX
#define UBOOT_CHECK_ADDR   0x80100000
#define KERNEL_CHECK_ADDR  0x80800000
#define DEVTREE_CHECK_ADDR 0x81000000
#endif
#ifdef CONFIG_FSVYBRID
#define UBOOT_CHECK_ADDR   0x80100000
#define KERNEL_CHECK_ADDR  0x80800000
#define DEVTREE_CHECK_ADDR 0x81000000
#endif
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



int handleIVT(u32 addr, u8 is_write, loff_t *off, loff_t *size, u32 length);
void memExchange(u32 srcaddr, u32 dstaddr, u32 length);
u32 makeSaveCopy(u32 srcaddr, u32 length);
u32 getImageLength(u32 addr);

#endif /*__IVT_H__*/
