/******************************************************************************/
/*
 * Filename: checknboot.c
 *
 * Description: Program to copy UBoot to DDR-RAM location
 *
 *
 *
 ******************************************************************************/

#include <checkboot.h>
#include <HAB.h>
#include <common.h>
#include <jffs2/jffs2.h>
#include <nand.h>
#include <config.h>

//static u32 CheckUBoot(ulong addr)
u32 CheckUBoot(ulong addr)
{

#ifdef HAB_RVT_iMX6
	struct rvt* hab = (struct rvt*)0x00000098;
	u32* check_addr = (u32*) 0x10100000;
#endif
#ifdef HAB_RVT_VYBRID
	struct rvt* hab = (struct rvt*)0x00000054;
	u32* check_addr = 0x80100000;
#endif

	uint8_t cid = (uint8_t) 0;
	u32* download_addr = check_addr;
	u32* current_addr = (u32*) addr;
	u32 temp_val;
	u32* ivt_addr = NULL;
	u8* csf_addr = NULL;
	u32 j = NULL;
	int i = NULL;
	for(i=0; i<((CONFIG_UBOOTNB0_SIZE)/4);i++)
	{
		temp_val = *check_addr;
		*check_addr = *current_addr;
		*current_addr = temp_val;
		check_addr++;
		current_addr++;
	}
	ivt_addr = (u32*) (download_addr + ((CONFIG_UBOOTNB0_SIZE)-0x4));
	csf_addr = (u8*) (ivt_addr[0] + download_addr);

	/* check functions  */
	hab->entry();
	j=hab->run_csf(csf_addr,cid);
	hab->exit();
	/* copy Image back to old Download address  */
	*current_addr = addr;
	temp_val = 0x00000000;
	check_addr = download_addr;
	for(i=0; i<((CONFIG_UBOOTNB0_SIZE)/4);i++)
	{
		temp_val = *current_addr;
		*current_addr = *check_addr;
		*check_addr = temp_val;
		check_addr++;
		current_addr++;
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
  u8 csf_val = 0;
  u32 j=NULL;
  ulong size_offset = NULL;
  ivt_addr = (u32*) (addr + ((CONFIG_UBOOTNB0_SIZE)-0x4));
  csf_addr = (u8*) (ivt_addr[0] + addr);
  csf_val = *csf_addr;
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
		{
			printf("\n Error: Images overlapped use other Download Address\n");
		}
	}else
	{
		printf(" Boot-Image can not find CSF-File!\n");
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
                printf("\n!!!6!!!\n");
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












