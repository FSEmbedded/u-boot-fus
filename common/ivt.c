/* File: ivt.c

   Author: Frieder Baumgratz
   Date: 29.11.2016

   Content: Functions to handle the IVT in the Kernel and the Device Tree.
   Furthermore the check functions are called.

*/

#include <ivt.h>
#include <jffs2/jffs2.h>


/*
  Function:   memExchange(...)

  Parameters: u32 srcaddr
  u32 dstaddr
  u32 length

  Return:     -

  Content:    Copies memory blocks from 'srcaddr' to 'dstaddr' with 'length'.

*/



void memExchange(u32 srcaddr, u32 dstaddr, u32 length)
{
	u32 *src_addr = (u32*) srcaddr;
	u32 *dst_addr = (u32*) dstaddr;
	u32 image_length = (length + 3) & ~3;
	int i = 0;
	if((u32)src_addr  < (u32) dst_addr)
		{
			printf("\nWarning: Copy image from address: 0x%x to 0x%x, 0x%x Bytes!!\n", (u32)src_addr, (u32)dst_addr, length);

			src_addr = (u32*)(srcaddr + image_length);
			dst_addr = (u32*)(dstaddr + image_length);
			for(i=((image_length)/4); i>=0; i--)
				{
					*dst_addr = *src_addr;
					dst_addr--;
					src_addr--;
				}
		}else if((u32)src_addr > (u32)dst_addr)
		{
			printf("\nWarning: Copy image from address: 0x%x to 0x%x, 0x%x Bytes!!\n", (u32)src_addr, (u32)dst_addr, length);

			src_addr = (u32*)srcaddr;
			dst_addr = (u32*)dstaddr;
			for(i=0; i<=image_length/4;i++)
				{
					*dst_addr = *src_addr;
					dst_addr++;
					src_addr++;
				}
		}
}


/*
  Function:   handleIVT

  Parameters: u32 addr -> Address where the Kernel is loaded to.

  Return:     integer -> whether the check function was successfull or not.

  Content:    Checks if loaded image is a Kernel or not. Loads it to the check address.
  Calls the check function (located in checkboot.c).

*/


int handleIVT(u32 addr, u8 is_write, loff_t *off, loff_t *size, u32 length)
{
	u32 *check_addr = 0x0;
	char *img_name = "";
	ivt_header_t *ivt = (ivt_header_t *) addr;
	if(IS_KERNEL(addr))
		{
			/*check_addr = (u32*) KERNEL_CHECK_ADDR;*/
			check_addr = (u32*) ivt->self;
			img_name = "uImage";
		}else if(IS_DEVTREE(addr))
		{
			/*check_addr = (u32*) DEVTREE_CHECK_ADDR;*/
			check_addr = (u32*) ivt->self;
			img_name = "Device Tree";
		}else if(IS_UBOOT_IVT(addr))
		{
			check_addr = (u32*) UBOOT_CHECK_ADDR;
			img_name = "UBoot IVT";
		}
	int ret = 1;
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;

	/* check images */
	if(is_write == 0)    /* nand write ? -> no  */
		{
			if((u32)check_addr != addr)
				{
					memExchange(addr, (u32)check_addr, length);
				}

			ret = checkTarget((u32)check_addr, (size_t) length, img_name);

			/* remove HAB header  */
			//memExchange(((u32)check_addr + HAB_HEADER), addr, (length - HAB_HEADER));
			memExchange(((u32)check_addr + HAB_HEADER), addr, (length));

			if(!ret == 0)
				return -1;
		}
	else                /* nand write ? -> yes  */
		{
			u32 save_addr = 0x0;

			if(IS_KERNEL(addr))
				find_dev_and_part("Kernel", &dev, &pnum, &part);
			else if(IS_DEVTREE(addr))
				find_dev_and_part("FDT", &dev, &pnum, &part);
			else if(IS_UBOOT_IVT(addr))
				find_dev_and_part("UBoot", &dev, &pnum, &part);
			else
				{
					printf("\n Can't find IVT!!\n");
					printf("\n Aborting ...\n");
					return -1;
				}
			if(*off == part->offset)
				{
					if(part->size >= length)
						{
							signed long offset = 0x0;
							if((u32) addr > (u32)check_addr)
								offset = (signed long)((signed long)addr - (signed long)check_addr);
							else
								offset = (signed long)((signed long)check_addr - (signed long)addr);

							if((addr == (u32)check_addr))
								{
									save_addr = makeSaveCopy(addr, length);
								}else if(offset < length)
								{
									save_addr = makeSaveCopy(addr, length);
									memExchange(addr, (u32) check_addr, length);
								}else
								{
									memExchange(addr, (u32)check_addr, length);
								}

							ret = checkTarget((u32)check_addr, (size_t) length, img_name);

							if((addr == (u32)check_addr) || offset < length)
								{
									/* get save image saved by 'makeSaveCopy' */
									memExchange(save_addr, addr, length);
								}else
								{
									/*memExchange((u32)check_addr, addr, length);*/
								}
						}else
						{
							printf("\n Partition is too small!!\n");
							printf("\n Aborting ...\n\n");
							return -1;
						}
				}else
				{
					printf("\n Wrong partition!!\n");
					printf("\n Aborting ...\n\n");
					return -1;
				}

			if(!ret == 0)
				return -1;
			else
				ret = 1;
		}
	return ret;
}


u32 makeSaveCopy(u32 srcaddr, u32 length)
{
	printf("\nWarning, making savecopy...\n");
	u32 *checkaddr = 0x0;
	u32 save_offset = 0x0;
	ivt_header_t *ivt = (ivt_header_t *) srcaddr;
	if(IS_KERNEL(srcaddr))
		{
			checkaddr = (u32*) ivt->self;
			save_offset = 0x1000;
		}
	if(IS_DEVTREE(srcaddr))
		{
			checkaddr = (u32*) ivt->self;
			save_offset = 0x1000;
		}
	if(IS_UBOOT_IVT(srcaddr))
		{
			checkaddr = (u32*)UBOOT_CHECK_ADDR;
			save_offset = 0x100000;
		}
	u32 *saveaddr = (u32*)(srcaddr + length);

	while((u32)saveaddr % save_offset != 0)
		{
			saveaddr++;
		}

	if(srcaddr == (u32) checkaddr)
		{
			memExchange(srcaddr, (u32)saveaddr, length);
		}else if((srcaddr + length) > (u32)checkaddr)
		{
			memExchange(srcaddr, (u32)saveaddr, length);
		}else
		{
		}
	return (u32)saveaddr;
}


u32 getImageLength(u32 addr)
{
	ivt_header_t* ivt = (ivt_header_t*)((u32)addr);
	signed long offset = (signed long)((signed long)addr - (signed long)ivt->self);
	boot_data_t *data = (boot_data_t*)((u32)ivt->boot_data + offset);
	return data->length;
}
