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

  Content:    Copies memory block freom srcaddr to dstaddr with length.

*/



void memExchange(u32 srcaddr, u32 dstaddr, u32 length)
{
  u32 *src_addr = (u32*) srcaddr;
  u32 *dst_addr = (u32*) dstaddr;
  u32 image_length = length;
  u32 temp_val;

  int i = 0;

  if(((u32)src_addr + image_length) < (u32) dst_addr)
    {
      u32 *src_addr = (u32*) srcaddr;
      u32 *dst_addr = (u32*) dstaddr;
      
      for(i=0; i<image_length/4;i++)
	{
	  temp_val = *dst_addr;
	  *dst_addr = *src_addr;
	  *src_addr = temp_val;
	  dst_addr++;
	  src_addr++;
	}
    }else if((u32)src_addr > ((u32)dst_addr + image_length))
    {
      u32 *src_addr = (u32*) srcaddr;
      u32 *dst_addr = (u32*) dstaddr;
      for(i=0; i<image_length/4;i++)
	{
	  temp_val = *dst_addr;
	  *dst_addr = *src_addr;
	  *src_addr = temp_val;
	  dst_addr++;
	  src_addr++;
	}
    }else if(src_addr < dst_addr)
    {
      u32 *src_addr = (u32*) (srcaddr + image_length);
      u32 *dst_addr = (u32*) (dstaddr + image_length);
      
      for(i=image_length/4; i>=0;i--)
	{
	  temp_val = *dst_addr;
	  *dst_addr = *src_addr;
	  *src_addr = temp_val;
	  dst_addr--;
	  src_addr--;
	}
    }else if(src_addr > dst_addr)
    {
      u32 *src_addr = (u32*) srcaddr;
      u32 *dst_addr = (u32*) dstaddr;
      for(i=0; i<image_length/4;i++)
	{
	  temp_val = *dst_addr;
	  *dst_addr = *src_addr;
	  *src_addr = temp_val;
	  dst_addr++;
	  src_addr++;
	}
    }else if((u32) src_addr == (u32) dst_addr)
    {
      
    }
}


/*
  Function:   handleIVT

  Parameters: u32 addr -> Address where the Kernel is loaded to.

  Return:     integer -> whether the check function was successfull or not.

  Content:    Checks if loaded image is a Kernel or not. Loads it to the check address.
  Calls the check function (located in checkboot.c) and removes the IVT.

*/


int handleIVT(u32 addr, int argc, loff_t *off, loff_t *size)
{

  u32 *kernel_check_addr = (u32*) 0x10800000;
  ivt_header_t* ivt = (ivt_header_t*)(kernel_check_addr);
  boot_data_t *data = ivt->boot_data;
  u32 offset = (u32) ((u32)(data) - (u32)(ivt->self));
  data = (boot_data_t*) ((u32)(data) - offset + BOOT_DATA_OFFSET);
  u32 kernel_length = data->length;
  int ret = 1;
    
  struct mtd_device *dev;
  struct part_info *part;
  u8 pnum;
      
  /* check kernel image  */

  /* cmd nand write?  */

  if(argc == 0)
    {
      ret = checkTarget((u32)kernel_check_addr, (size_t) kernel_length, "uImage");
      if(!ret == 0)
	return -1;
    }
  else
    {
      find_dev_and_part("UBoot", &dev, &pnum, &part);
      if(*off == part->offset && *size == kernel_length)
	ret = checkTarget(addr, (size_t) kernel_length, "uImage");
      if(!ret != 0)
	return -1;
      else
	return -1;
    }

  return ret;
}


void removeHABHeader(u32 addr, u32 length)
{
  u32 *checkaddr = (u32*)(KERNEL_CHECK_ADDR + HAB_HEADER);
  
   memExchange((u32) checkaddr, addr, (length - HAB_HEADER));
  
}
