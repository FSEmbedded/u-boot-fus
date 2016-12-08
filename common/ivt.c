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
  int i = 0;

  //  if(((u32)src_addr + image_length) < (u32) dst_addr)
  if((u32)src_addr  < (u32) dst_addr)  
    {
      //u32 *src_addr = (u32*) (srcaddr + image_length);
      //u32 *dst_addr = (u32*) (dstaddr + image_length);
      src_addr = (u32*)(srcaddr + image_length);
      dst_addr = (u32*)(dstaddr + image_length);
      //image_length += (u32)8;
      for(i=(image_length/4); i>=0; i--)
	{
	  *dst_addr = *src_addr;
	  dst_addr--;
	  src_addr--;
	}
    }//else if((u32)src_addr > ((u32)dst_addr + image_length))
  else if((u32)src_addr > (u32)dst_addr)
    {
      //u32 *src_addr = (u32*) srcaddr;
      //u32 *dst_addr = (u32*) dstaddr;
      src_addr = (u32*)srcaddr;
      dst_addr = (u32*)dstaddr;
      for(i=0; i<=image_length/4;i++)
	{
	  *dst_addr = *src_addr;
	  dst_addr++;
	  src_addr++;
	}
    }
#if 0 
  else if(src_addr < dst_addr)
    {
      u32 *src_addr = (u32*) (srcaddr + image_length);
      u32 *dst_addr = (u32*) (dstaddr + image_length);
      
      for(i=image_length/4; i>=0;i--)
	{
	  *dst_addr = *src_addr;
	  dst_addr--;
	  src_addr--;
	}
    }else if((u32)src_addr > (u32)dst_addr)
    {
      u32 *src_addr = (u32*) srcaddr;
      u32 *dst_addr = (u32*) dstaddr;
      for(i=0; i<image_length/4;i++)
	{
	  *dst_addr = *src_addr;
	  dst_addr++;
	  src_addr++;
	}
    }else if(((u32)src_addr + image_length) > (u32) dst_addr)
    {
      u32 *src_addr = (u32*) srcaddr;
      u32 *dst_addr = (u32*) dstaddr;
      
      for(i=0; i<image_length/4;i++)
	{
	  *dst_addr = *src_addr;
	  dst_addr++;
	  src_addr++;
	}
    }
#endif
  else if((u32) src_addr == (u32) dst_addr)
    {
    }
}


/*
  Function:   handleIVT

  Parameters: u32 addr -> Address where the Kernel is loaded to.

  Return:     integer -> whether the check function was successfull or not.

  Content:    Checks if loaded image is a Kernel or not. Loads it to the check address.
  Calls the check function (located in checkboot.c).

*/


int handleIVT(u32 addr, int argc, loff_t *off, loff_t *size, u32 length)
{
  u32 *kernel_check_addr = (u32*) 0x10800000;
  int ret = 1;
  
  struct mtd_device *dev;
  struct part_info *part;
  u8 pnum;
      
  /* check kernel image  */

 
  if(argc == 0)
    {
      if((u32)kernel_check_addr != addr)
	memExchange(addr, (u32)kernel_check_addr, length);
      
      ret = checkTarget((u32)kernel_check_addr, (size_t) length, "uImage");
      removeHABHeader(addr, length);
      if(!ret == 0)
	return -1;
    }
  else
    {
      memExchange(addr, (u32)kernel_check_addr, length);
      find_dev_and_part("Kernel", &dev, &pnum, &part);
      if(*off == part->offset && *size == length)
	{
	  ret = checkTarget(addr, (size_t) length, "uImage");
	}else
	{
	  printf("\n No Kernel found!!\n");
	  printf("\n Aborting ...\n");
	  return -1;
	}
      
      if(!ret == 0)
	return -1;
      else
	ret = 1;
    }
  return ret;
}


void removeHABHeader(u32 addr, u32 length)
{
  u32 *checkaddr = (u32*)(KERNEL_CHECK_ADDR + HAB_HEADER);
  
   memExchange((u32) checkaddr, addr, (length - HAB_HEADER));  
}

int makeSaveCopy(u32 srcaddr, u32 length)
{
  u32 *checkaddr = (u32*)0x10800000;
  u32 *saveaddr = (u32*)(srcaddr + length);

  while((u32)saveaddr % (u32)0x10000 != 0)
    {
      saveaddr++;
    }
  
  if(srcaddr == (u32)checkaddr)
    {
      printf("\n WARNING: Making savecopy starting at: 0x%x, 0x%x Bytes!!\n", (u32)saveaddr, length);
      memExchange(srcaddr, (u32)saveaddr, length);
    }else if((srcaddr + length) > (u32)checkaddr)
    {
      printf("\n WARNING: Not enough space between checkaddress (0x10800000) and Loadaddress. Making savecopy starting at: 0x%x, 0x%x Bytes!!\n", (u32)saveaddr, length);
      memExchange(srcaddr, (u32) saveaddr, length);
    }else
    {
    }
  return 0;
}

int getSaveCopy(u32 srcaddr, u32 length)
{
  u32 *checkaddr = (u32*)0x10800000;
  u32 *saveaddr = (u32*)(srcaddr + length);

  while((u32)saveaddr % (u32)0x10000 != 0)
    {
      saveaddr++;
    }
  if(srcaddr == (u32)checkaddr)
    {
      printf("\n WARNING: Copy image back to Loadaddress: 0x%x, 0x%x Bytes!!\n", srcaddr, length);
      memExchange((u32)saveaddr, srcaddr, length);
    }else if((srcaddr + length) > (u32)checkaddr)
    {
      printf("\n WARNING: Copy image back to Loadaddress: 0x%x, 0x%x Bytes!!\n", srcaddr, length);
      memExchange((u32) saveaddr, srcaddr, length);
    }else
    {
    }
  return 0;
}
