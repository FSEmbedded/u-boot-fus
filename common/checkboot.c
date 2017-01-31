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


/*
   Function:    checkTarget

   Parameters:  ivtAddr (start address of the IVT)

   Return:      hab_status_t (return of the HAB functions [HAB_SUCCESS, ...])

   Content:     Checks the image, based on the address given to the function.

*/



int checkTarget(u32 addr, size_t bytes, const char* image)
{

	struct rvt* hab = (struct rvt*) 0x00000000;

#ifdef HAB_RVT_iMX6
	/*imx6 solo,dual light */
	if(is_cpu_type(MXC_CPU_MX6SOLO))
		{
			hab = (struct rvt*) 0x00000098;
		}
	if(is_cpu_type(MXC_CPU_MX6Q))
		{
			hab = (struct rvt*) 0x00000094;
		}
	if(is_cpu_type(MXC_CPU_MX6UL))
		{
			hab = (struct rvt*) 0x00000100;
		}
#endif
#ifdef HAB_RVT_VYBRID
	hab = (struct rvt*)0x00000054;
#endif


	u8 *csf_addr;
	u32 * ivt_addr;
	u8 csf_val;
	u8 check_val = 0xd4;
	u32 hab_state;
	ptrdiff_t ivt_offset = 0x0;
	ivt_header_t* ivt;// = (ivt_header_t*)((u32)KERNEL_CHECK_ADDR);

	if(IS_KERNEL(addr))
		ivt = (ivt_header_t*)((u32)KERNEL_CHECK_ADDR);
	if(IS_DEVTREE(addr))
		ivt = (ivt_header_t*)((u32)DEVTREE_CHECK_ADDR);
	if(IS_UBOOT_IVT(addr))
		ivt = (ivt_header_t*)((u32)UBOOT_CHECK_ADDR);


	ivt_addr = ivt->self;
	csf_addr = (u8*)ivt->csf;
	csf_val = *csf_addr;
	if(csf_val == check_val)
		{

			printf("\n>>> %s Image with CSF-File <<<\n", image);
			hab->entry();
			hab_state = hab->authenticate_image(0, ivt_offset, (void**)&ivt_addr, (size_t*)&bytes, NULL);
			//hab_state = hab->run_csf(csf_addr, 0);
			hab->exit();
			if(hab_state == HAB_SUCCESS || hab_state == (u32)ivt->entry)
				{
					printf("%s Image: successfully tested!\n\n", image);
					return 0;
				}else
				{
					GetHABStatus();
					printf("\n%s Image: test failed!\n\n", image);
					return -1;
				}
		}else
		{
			printf("\n%s Image: test failed!\n", image);
			printf("\nCouldn'f find CSF file!!\n");
			return -1;
		}
}
