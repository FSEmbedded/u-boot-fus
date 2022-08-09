#include <common.h>
#include <dm.h>
#include <mmc.h>
#include "mmc_test.h"
#include "selftest.h"
#include "check_config.h"
#include "common/fus_sdio.h"
#include "serial_test.h" // mute_debug_port()
#include <asm/gpio.h>



static char* get_name(struct mmc *mmc,char* name){

	name[0] = (char)  mmc->cid[0] & 0xff;
	name[1] = (char) (mmc->cid[1] >> 24) & 0xff;
	name[2] = (char) (mmc->cid[1] >> 16) & 0xff;
	name[3] = (char) (mmc->cid[1] >> 8)  & 0xff;
	name[4] = (char)  mmc->cid[1] & 0xff;
	name[5] = '\0';

	return name;
}


int test_mmc(char * szStrBuffer){

	struct udevice *dev;
	struct uclass *uc;
	const void *fdt = gd->fdt_blob;
	int node;
	int err;
	char mmc_name [6];
	int uiManfid;

	/* Clear reason-string */
	szStrBuffer[0] = '\0';

	if (uclass_get(UCLASS_MMC, &uc))
		return 1;

	uclass_foreach_dev(dev, uc) {

		struct mmc *mmc;

		mmc = find_mmc_device(dev->seq);

		if (!mmc) {
			return 1;
		}

		/* Check if MMC eMMC or SD.
		 * The respective property has to be set in the device tree usdhc node.
		 * If no property is set, the mmc device will be tested as GPIO.
		 */
		node = dev_of_offset(dev);

		if (fdt_get_property(fdt, node, "is-sdcard", NULL))
			printf("SD card ..............");
		else if (fdt_get_property(fdt, node, "is-emmc", NULL))
			if (has_feature(FEAT_EMMC))
				printf("EMMC .................");
			else
				continue;
		else if (fdt_get_property(fdt, node, "is-wlan", NULL))
			if (has_feature(FEAT_WLAN)){
				/* If a reset is found, pull it for 500ms */
				struct gpio_desc desc;
				if (!gpio_request_by_name(dev, "rst-gpios", 0, &desc, GPIOD_IS_OUT)){
					dm_gpio_set_value(&desc,0);
					mdelay(500);
					dm_gpio_set_value(&desc,1);
					mdelay(500);
				}
				printf("WLAN .................");
			}
			else
				continue;
		else{
			continue;
		}

		/* Card detect is checked within mmc_init().
		 * If card detect is not available, the property "non-removable"
		 * has to be set in the respective device tree usdhc node.
		 */
		mute_debug_port(1);
		err = mmc_init(mmc);
		mute_debug_port(0);
		if (!err){
			sprintf(szStrBuffer,"Name=%s, %dMB",
					get_name(mmc,mmc_name),(u32)(mmc->capacity/1024)/1024);
			/* No need to check for SDIO anymore */
			test_OkOrFail(err,1,szStrBuffer);
			continue;
		}

		if (err){
			/* mmc_init() has to be called before sdio_init() because it
			 * initializes some power settings. So we always check for MMC and SD
			 * first, even if we know its SDIO
			 */
			mute_debug_port(1);
			err = sdio_init(mmc, &uiManfid);
			mute_debug_port(0);
			if (!err){

				if( 0x97 == uiManfid )
				{
					sprintf(szStrBuffer,"%s SDIO Card", "Texas Instruments");
				}
				else if( 0x271 == uiManfid )
				{
					sprintf(szStrBuffer,"%s SDIO Card", "Atheros");

				}
				else if( 0x2D0 == uiManfid )
				{
					sprintf(szStrBuffer,"%s SDIO Card", "Broadcom");
				}
				else if( 0x2DF == uiManfid )
				{
					sprintf(szStrBuffer,"%s SDIO Card", "Silex");
				}
				else
				{
					sprintf(szStrBuffer,"unknown SDIO Card detected (0x%x)SDIO Card",uiManfid);
				}

			}else {
				sprintf(szStrBuffer,"no Card detected");
			}
		}

		test_OkOrFail(err,1,szStrBuffer);
	}

	return 0;
}
