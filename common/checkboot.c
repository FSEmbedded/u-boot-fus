/*
 * Filename: checkboot.c
 *
 * Description: Contains function to check an image.
 *
 */

#include <common.h>		//ALIGN, le16_to_cpu
#include <jffs2/jffs2.h>	//find_dev_and_part, struct part_info
#include <nand.h>
#include <asm/io.h>		//readl function
/* HAB includes  */
#include <checkboot.h>
#include <hab.h>


/*
 * Function:   GetLoaderType(u32 addr)
 *
 * Parameters: addr -> start address of image
 *
 * Return:     get the type of the image.
 *
 * Content:    we need the name of the image to set the name as a string so we
 *             can get access to the flash partition with this string name.
 */
LOADER_TYPE GetLoaderType(u32 addr)
{
	if (IS_UBOOT(addr))
		return LOADER_UBOOT;

	if (IS_UBOOT_IVT(addr))
		return LOADER_UBOOT_IVT;

	if (IS_KERNEL(addr))
		return LOADER_KERNEL;

	if (IS_KERNEL_IVT(addr))
		return LOADER_KERNEL_IVT;

	if (IS_DEVTREE(addr))
		return LOADER_FDT;

	if (IS_DEVTREE_IVT(addr))
		return LOADER_FDT_IVT;

	return LOADER_NONE;
}


/*
 * Function:   memExchange(u32 srcaddr, u32 dstaddr, u32 length)
 *
 * Parameters: u32 srcaddr -> start address of the given image
 *             u32 dstaddr -> destination address of the given image
 *             u32 length  -> length of the image
 *
 * Return:     -
 *
 * Content:    Copies memory blocks from 'srcaddr' to 'dstaddr' with 'length'.
 */
void memExchange(u32 srcaddr, u32 dstaddr, u32 length)
{
	u32 *src_addr = (u32*) srcaddr;
	u32 *dst_addr = (u32*) dstaddr;
	u32 image_length = (length + 3) & ~3;
	int i = 0;
	if((u32)src_addr  < (u32) dst_addr) {
		src_addr = (u32*)(srcaddr + image_length);
		dst_addr = (u32*)(dstaddr + image_length);
		for(i=((image_length)/4); i>=0; i--) {
				*dst_addr = *src_addr;
				dst_addr--;
				src_addr--;
			}
	} else if((u32)src_addr > (u32)dst_addr) {
		src_addr = (u32*)srcaddr;
		dst_addr = (u32*)dstaddr;
		for(i=0; i<=image_length/4;i++) {
			*dst_addr = *src_addr;
			dst_addr++;
			src_addr++;
		}
	}
}


/*
 * Function:   makeSaveCopy(u32 srcaddr, u32 length)
 *
 * Parameters: srcaddr -> start address of image
 *             length  -> length of the image
 *
 * Return:     get save address of the image.
 *
 * Content:    before the image will be checked it must be saved to another
 *             RAM address. This is necessary because if a encrypted image
 *             will be checked it will be decrypted at directly at the RAM
 *             address so we only have a decrypted image but we want to store
 *             an encrypted image in the flash. So thats why we need a save
 *             copy of the original image.
 */
u32 makeSaveCopy(u32 srcaddr, u32 length)
{
	ivt_header_t *ivt = (ivt_header_t *) srcaddr;
	u32 *checkaddr = 0x0;
	u32 *saveaddr = 0x0;

	checkaddr = (u32*) ivt->self;

	if(srcaddr < (u32)checkaddr)
		saveaddr = (u32*)(checkaddr + length);
	else
		saveaddr = (u32*)(srcaddr + length);

	memExchange(srcaddr, (u32)saveaddr, length);

	return (u32)saveaddr;
}


/*
 * Function:   getImageLength(u32 addr)
 *
 * Parameters: addr -> start address of image
 *
 * Return:     length of the image
 *
 * Content:    get the image length from the ivt.
 */
u32 getImageLength(u32 addr)
{
	ivt_header_t* ivt = (ivt_header_t*)((u32)addr);
	signed long offset = (signed long)((signed long)addr - (signed long)ivt->self);
	boot_data_t *data = (boot_data_t*)((u32)ivt->boot_data + offset);
	return data->length;
}


/*
 * Function:   check_flash_partition(char *img_name, loff_t *off, u32 length)
 *
 * Parameters: img_name -> start address of image to be checked
 *             off      -> image length
 *             length   -> image name
 *
 * Return:      0 -> check successful
 *             -1 -> check unsuccessful
 *
 * Content:    Checks if the image will be written to the correct partition
 *             and checks if the image fits in the partition.
 */
int check_flash_partition(char *img_name, loff_t *off, u32 length)
{
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;
	int ret = -1;

	find_dev_and_part(img_name, &dev, &pnum, &part);
	if(*off != part->offset) {
		printf("\nWrong partition!!\n");
		printf("\nAborting ...\n\n");
		ret = -1;
	} else if(part->size < length) {
		printf("\nPartition is too small!!\n");
		printf("\nAborting ...\n\n");
		ret = -1;
	} else {
		ret = 0;
	}

	return ret;
}


/*
 * Function:   checkTarget(u32 addr, size_t bytes, const char* image)
 *
 * Parameters: addr  -> start address of image to be checked
 *             bytes -> image length
 *             image -> image name
 *
 * Return:      0 -> test successful
 *             -1 -> test unsuccessful
 *
 * Content:    Checks the image, based on the address given to the function.
 */
int checkTarget(u32 addr, size_t bytes, const char* image)
{
	struct rvt* hab = (struct rvt*)GetHABAddress();
	ivt_header_t* ivt = {0};
	ptrdiff_t ivt_offset = 0x0;
	u32 * ivt_addr = 0x0;
	u32 hab_state = 0x0;
	int ret = -1;

	ivt = (ivt_header_t*)addr;
	ivt_addr = ivt->self;

	hab->entry();
	hab_state = hab->authenticate_image(0, ivt_offset, (void**)&ivt_addr, (size_t*)&bytes, NULL);
	hab->exit();
	if(hab_state == HAB_SUCCESS || hab_state == (u32)ivt->entry) {
		printf("sucessfully authenticated\n\n");
		ret = 0;
		goto exit;
	} else {
		GetHABStatus();
		printf("authentication failed!\n\n");
		goto exit;
	}

exit:
	return ret;
}


/*
 * Function:   Init_HAB(u32 addr, OPTIONS eOption, loff_t *off, loff_t *size)
 *
 * Parameters: u32 addr        -> address in RAM where the kernel is loaded
 *             OPTIONS eOption -> enum what to do with the image
 *             loff_t *off     -> start address in flash where to write
 *             loff_t *size    -> image size
 *
 * Return:     bool -> verification of the image
 *
 * Content:    Prepare the image that it can be verified.
 */
bool Init_HAB(u32 addr, OPTIONS eOption, loff_t *off, loff_t *size)
{
	/* HAB Variables */
	bool status_hab = false;
	ivt_header_t *ivt = {0};
	u32 *check_addr = NULL;
	u32 csf_offset = 0;
	u32 save_addr = 0x0;
	u32 length = (u32)*size;
	u8 *csf_addr = NULL;
	u8 csf_val = 0;
	u8 check_val = 0xd4;
	char *img_name = "";
	int ret = 0;

	/* get image type */
	LOADER_TYPE boot_loader_type = GetLoaderType(addr);

	/* check which image type we have and set img_name */
	if(boot_loader_type & (LOADER_UBOOT_IVT | LOADER_UBOOT)) {
		img_name = "UBoot";
	} else if(boot_loader_type & (LOADER_KERNEL_IVT | LOADER_KERNEL)) {
		img_name = "Kernel";
	} else if(boot_loader_type & (LOADER_FDT_IVT | LOADER_FDT)) {
		img_name = "FDT";
	} else {
		printf("invalid image type!\n");
		return false;
	}

	/* if image have ivt, then set csf header */
	if(boot_loader_type & (LOADER_KERNEL_IVT | LOADER_FDT_IVT | LOADER_UBOOT_IVT)) {
		length = getImageLength(addr);
		ivt = (ivt_header_t*)addr;
		check_addr = ivt->self;
		/* get CSF header of the current image */
		if((u32)check_addr != addr) {
			csf_offset = ((u32)ivt->csf - (u32)ivt->self);
			csf_addr = (u8*)((u32*)(addr + csf_offset));
		} else {
			csf_addr = (u8*)ivt->csf;
		}
		csf_val = *csf_addr;
	}

	printf("\ntesting %s ...\n", img_name);

	if(csf_val == check_val) {
		printf("found CSF\n");
		/* we only need a backup of the image if we want to save it to flash.
		 * Thats why we make a save copy of the image.
		 */
		if (eOption == BACKUP_IMAGE) {
			ret = check_flash_partition(img_name, off, length);
			if(ret != 0) {
				status_hab = false;
				goto exit;
			}
			save_addr = makeSaveCopy(addr, length);
		}

		if((addr != (u32)check_addr)) {
			memExchange(addr, (u32) check_addr, length);
		}

		ret = checkTarget((u32)check_addr, (size_t) length, img_name);
		if(ret == 0)
			status_hab = true;

		if (eOption == BACKUP_IMAGE) {
			/* get original image saved by 'makeSaveCopy' */
			memExchange(save_addr, addr, length);
		}
		else if(eOption == CUT_IVT) {
			memExchange(((u32)check_addr + HAB_HEADER), addr, length);
		}
	} else {
		printf("no CSF found!\n");
		if (eOption == BACKUP_IMAGE) {
			ret = check_flash_partition(img_name, off, length);
			if(ret != 0) {
				status_hab = false;
				goto exit;
			}
		}

		if(!(readl(0x021BC460) & 0x00000002) && (status_hab == false)) {
			printf("Security Config is not closed, can proceed with unsecure %s\n\n", img_name);
			status_hab = true;
		}
	}

exit:
	return status_hab;
}

