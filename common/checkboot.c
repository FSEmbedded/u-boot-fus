/******************************************************************************/
/*
 * Filename: checkboot.c
 *
 * Description: Program to copy UBoot to DDR-RAM location
 *
 *
 *
 ******************************************************************************/

/* HAB includes  */

#include <checkboot.h>
#include <HAB.h>
#include <asm/arch-mx6/sys_proto.h>          // is_cpu_type, added FB
#include <ivt.h>



#include <common.h>
#include <jffs2/jffs2.h>
#include <nand.h>
#include <config.h>

//static u32 CheckUBoot(ulong addr)
u32 CheckUBoot(ulong addr)
{

  struct rvt* hab = (struct rvt*) 0x00000000;

#ifdef HAB_RVT_iMX6
  /*imx6 solo,dual light */
  if(is_cpu_type(MXC_CPU_MX6SOLO))
    {
      hab = (struct rvt*) 0x00000098;
    }else if(is_cpu_type(MXC_CPU_MX6Q))
    {
      hab = (struct rvt*) 0x00000094;
    }
  u32* check_addr = (u32*) 0x10100000;
  u32* save_addr = (u32*) 0x10190000;
#endif
#ifdef HAB_RVT_VYBRID
  hab = (struct rvt*)0x00000054;
  u32* check_addr = (u32*) 0x80100000;
  u32* save_addr = (u32) 0x80170000;
#endif

  uint8_t cid = (uint8_t) 0;
  u32 download_addr = (u32) check_addr;
  u32* current_addr = (u32*) addr;
  u32 temp_val;
  u32* ivt_addr = NULL;
  u8* csf_addr = NULL;
  u32 j = NULL;
  u32 t = NULL;
  int i = NULL;

  for(i=0; i<((CONFIG_UBOOTNB0_SIZE)/4);i++)
    {
      temp_val = *check_addr;
      *check_addr = *current_addr;
      *save_addr = temp_val;
      save_addr++;
      check_addr++;
      current_addr++;
    }
 
  if(IS_UBOOT(download_addr))
    {
      ivt_addr = (u32*) (download_addr + ((CONFIG_UBOOTNB0_SIZE)-0x4));
      csf_addr = (u8*) (ivt_addr[0] + download_addr);
    
    }else if(IS_UBOOT_IVT(download_addr))
    {
      ivt_addr = (u32*) (download_addr);
      csf_addr = (u8*) (ivt_addr[6]);
    }
  /* check functions  */
  size_t size_image = (size_t) CONFIG_UBOOTNB0_SIZE;
  hab->entry();
  t=hab->run_csf(csf_addr,cid);
  if(t == HAB_SUCCESS)
    {
      t = hab->check_target(HAB_TGT_MEMORY, (void*) download_addr, size_image);
      if(t == HAB_SUCCESS)
	{
	  j = HAB_SUCCESS;
	}
    }
  hab->exit();
#ifdef HAB_RVT_iMX6 
  current_addr = (u32*) 0x10100000;
  save_addr = (u32*) 0x10190000;
#endif

#ifdef HAB_RVT_VYBRID
  current_addr = (u32*) 0x80100000;
  save_addr = (u32*) 0x80170000;
#endif
  
  for(i=0; i<((CONFIG_UBOOTNB0_SIZE)/4);i++)
    {
      *current_addr = *save_addr;
      current_addr++;
      save_addr++;
    }

  return j;
}

//static int Init_HAB(ulong addr)
int Init_HAB(ulong addr)
{
#ifdef HAB_RVT_iMX6
  u32 download_addr = 0x10100000;
#endif
#ifdef HAB_RVT_VYBRID
  u32 download_addr = 0x80100000;
#endif

  int hab_ok = 0;
  u8 vgl = 0xd4;
  u32* ivt_addr = NULL;
  u8* csf_addr = NULL;
  u8* ivt_self = NULL;
  u8 csf_val = 0;
  u32 j=NULL;
  ulong size_offset = NULL;


  if(IS_UBOOT(addr))
    {
      ivt_addr = (u32*) (addr + ((CONFIG_UBOOTNB0_SIZE)-0x4));
      csf_addr = (u8*) (ivt_addr[0] + addr);
      csf_val = *csf_addr;

    }else if(IS_UBOOT_IVT(addr))
    {
      ivt_addr = (u32*) (addr);
      ivt_self = (u8*)(ivt_addr[5]);
      csf_addr = (u8*) ((u8*)ivt_addr[6] - ivt_self + addr);
      csf_val = *csf_addr;
    }
  if(csf_val == vgl)
    {
      printf("\n Boot-Image with CSF-File\n");

      if(addr != download_addr)
	{
	  if(addr > download_addr)
	    {
	      size_offset = addr - download_addr;
	    }else
	    {
	      size_offset = download_addr - addr;
	    }
	  if(size_offset >= (CONFIG_UBOOTNB0_SIZE))
	    {
	      j = CheckUBoot(addr);
	      if(j == HAB_SUCCESS)
		{
		  printf("\n Boot-Image successfully tested!\n");
		  hab_ok = 1;
		}else
		{
		  printf("\n Boot-Image Test failed!\n");
		  GetHABStatus();
		}
	    }
	}else
	{/*
	   j = CheckUBoot(addr);
	   if(j == HAB_SUCCESS)
	   {
	   printf("\n Boot-Image successfully tested!\n");
	   hab_ok = 1;
	   }else
	   {
	   printf("\n Boot-Image Test failed!\n");
	   GetHABStatus();
	   }*/
	  printf("\n Error: Images overlapped use other Download Address\n");
	}
    }else
    {
      printf("Boot-Image can not find CSF-File!\n");
    }
  return hab_ok;
}


int CheckIfUBoot(int argc, char*const argv[], int *idx, loff_t *off, loff_t *size, loff_t *maxsize, ulong addr)
{
  int hab_ok = 0;
  struct mtd_device *dev;
  struct part_info *part;
  u8 pnum;
  //int ret;
  //ret = find_dev_and_part("UBoot", &dev, &pnum, &part);
  find_dev_and_part("UBoot", &dev, &pnum, &part);
  if(argc == 0)
    {
      *off = 0;
      *size = nand_info[*idx].size;
      *maxsize = * size;
    }
  if(*off == part->offset)
    {
      if((unsigned long long) *size == CONFIG_UBOOTNB0_SIZE)
	{
	  /* HAB check  */
	  hab_ok = Init_HAB(addr);
	}else
	{
	  printf("\n Error: No U-Boot found. Expected Size: %d, current Size: %llu \n", CONFIG_UBOOTNB0_SIZE,(unsigned long long)*size);
	}
    }else
    {
      if((*off < part->offset && (*off+*size) < part->offset) || *off > part->offset)
	{
	  /* andere Partition  */
	  hab_ok = 1;
	}else
	{
	  printf("\nError: Try rewrite UBoot on wrong Address!\n");
	}
    }
  printf("\n");
  return hab_ok;
}


/* 
   Function:    checkTarget
   
   Parameters:  ivtAddr (start address of the IVT)
   
   Return:      hab_status_t (return of the HAB functions [HAB_SUCCESS, ...])
   
   Content:     Checks


*/



int checkTarget(u32 addr, size_t bytes, const char* image)
{

  struct rvt* hab = (struct rvt*) 0x00000000;

#ifdef HAB_RVT_iMX6
  /*imx6 solo,dual light */
  if(is_cpu_type(MXC_CPU_MX6SOLO))
    {
      hab = (struct rvt*) 0x00000098;
    }else if(is_cpu_type(MXC_CPU_MX6Q))
    {
      hab = (struct rvt*) 0x00000094;
    }
#endif
#ifdef HAB_RVT_VYBRID
  hab = (struct rvt*)0x00000054;
#endif

  u8 *csf_addr;
  u8 csf_val;
  u8 check_val = 0xd4;
  u32 hab_state;
  //ptrdiff_t ivt_offset = 0x0;
  ivt_header_t* ivt = (ivt_header_t*)(addr);

  //ivt_addr = ivt->self;
  csf_addr = (u8*)ivt->csf;
  csf_val = *csf_addr;
  if(csf_val == check_val)
    {
      printf("\n>>> %s Image with CSF-File <<<\n", image);
      hab->entry();
      //      hab_state = hab->authenticate_image(0, ivt_offset, (void**)&ivt_addr, (size_t*)&bytes, NULL);
      hab_state = hab->run_csf(csf_addr, 0);
      hab->exit();
      if(hab_state != HAB_SUCCESS)
	{
	  GetHABStatus();
	  return -1;
	}else if(hab_state == HAB_SUCCESS)
	{
	  printf("\n%s Image: successfully tested!\n\n", image);
	  return 0;
	}else
	{
	  return -1;
	}
    }
  return -1;
}
