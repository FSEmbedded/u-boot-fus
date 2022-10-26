#include <common.h>
#include <dm/device.h>
#include <dm.h>
#include <asm/arch/clock.h>
#include <asm/io.h>
//#include <asm/arch/imx-regs-imx8mp.h> // LPUART_BASE_ADDR
#include <asm/mach-imx/iomux-v3.h>


static int fracpll_configure_audioPll1(void)
{

	return 0;
}

u64 sai_baseaddr(struct udevice *dev) {

	return 0;
}

int config_sai(struct udevice *dev) {


    return 0;

}

int config_sgtl(struct udevice *dev){

	return 0;
}

int run_audioTest(struct udevice *dev,uint8_t* data_send,uint8_t* data_recv, uint32_t len){

	return 0;
}


