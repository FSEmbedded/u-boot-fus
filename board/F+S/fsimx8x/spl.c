/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <spl.h>
#include <malloc.h>
#include <errno.h>
#include <netdev.h>
#include <fsl_ifc.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <environment.h>
#include <fsl_esdhc.h>
#include <i2c.h>
#include "pca953x.h"

#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <asm/mach-imx/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <dm.h>
#include <imx8_hsio.h>
#include <usb.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/video.h>
#include <asm/mach-imx/dma.h>
#include <asm/arch/video_common.h>

DECLARE_GLOBAL_DATA_PTR;

#define SPL_DRC_0

#ifdef SPL_DRC_0

	#if 0
		#define U32(X)      ((uint32_t) (X))
		#define DATA4(A, V) *((volatile uint32_t*)(A)) = U32(V)
		#define SET_BIT4(A, V) *((volatile uint32_t*)(A)) |= U32(V)
		#define CLR_BIT4(A, V) *((volatile uint32_t*)(A)) &= ~(U32(V))
		#define CHECK_BITS_SET4(A, M) while((*((volatile uint32_t*)(A))         \
			& U32(M)) != ((uint32_t)(M))){printf("CHECK_BITS_SET4: 0x%x 0x%x 0x%x\n", (A), *((volatile uint32_t*)(A)), U32(M));}
		#define CHECK_BITS_CLR4(A, M) while((*((volatile uint32_t*)(A))         \
			& U32(M)) != U32(0U)){printf("CHECK_BITS_CLR4: 0x%x 0x%x 0x%x\n", (A), *((volatile uint32_t*)(A)), U32(M));}
		#define CHECK_ANY_BIT_SET4(A, M) while((*((volatile uint32_t*)(A))      \
			& U32(M)) == U32(0U)){printf("CHECK_ANY_BIT_SET4: 0x%x 0x%x 0x%x\n", (A), *((volatile uint32_t*)(A)), U32(M));}
		#define CHECK_ANY_BIT_CLR4(A, M) while((*((volatile uint32_t*)(A))      \
			& U32(M)) == U32(M)){printf("CHECK_ANY_BIT_CLR4: 0x%x 0x%x 0x%x\n", (A), *((volatile uint32_t*)(A)), U32(M));}
	#else
		#define U32(X)      ((uint32_t) (X))
		#define DATA4(A, V) *((volatile uint32_t*)(A)) = U32(V)
		#define SET_BIT4(A, V) *((volatile uint32_t*)(A)) |= U32(V)
		#define CLR_BIT4(A, V) *((volatile uint32_t*)(A)) &= ~(U32(V))
		#define CHECK_BITS_SET4(A, M) while((*((volatile uint32_t*)(A))         \
			& U32(M)) != ((uint32_t)(M))){}
		#define CHECK_BITS_CLR4(A, M) while((*((volatile uint32_t*)(A))         \
			& U32(M)) != U32(0U)){}
		#define CHECK_ANY_BIT_SET4(A, M) while((*((volatile uint32_t*)(A))      \
			& U32(M)) == U32(0U)){}
		#define CHECK_ANY_BIT_CLR4(A, M) while((*((volatile uint32_t*)(A))      \
			& U32(M)) == U32(M)){}
	#endif

#endif

#define ESDHC_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define GPMI_NAND_PAD_CTRL	 ((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) \
				  | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define ESDHC_CLK_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))


#define ENET_INPUT_PAD_CTRL	((SC_PAD_CONFIG_OD_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_18V_10MA << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define ENET_NORMAL_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_18V_10MA << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define FSPI_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define GPIO_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define I2C_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_LOW << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define UART_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))
#ifdef CONFIG_FSL_ESDHC

#define USDHC1_CD_GPIO	IMX_GPIO_NR(4, 22)

static struct fsl_esdhc_cfg usdhc_cfg[CONFIG_SYS_FSL_USDHC_NUM] = {
	{USDHC1_BASE_ADDR, 0, 8},
	{USDHC2_BASE_ADDR, 0, 4},
};

static iomux_cfg_t emmc0[] = {
	SC_P_EMMC0_CLK | MUX_PAD_CTRL(ESDHC_CLK_PAD_CTRL),
	SC_P_EMMC0_CMD | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA0 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA1 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA2 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA3 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA4 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA5 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA6 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA7 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_RESET_B | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_STROBE | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
};

static iomux_cfg_t usdhc1_sd[] = {
	SC_P_USDHC1_CLK | MUX_PAD_CTRL(ESDHC_CLK_PAD_CTRL),
	SC_P_USDHC1_CMD | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA0 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA1 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA2 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA3 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_WP    | MUX_PAD_CTRL(ESDHC_PAD_CTRL), /* Mux for WP */
	SC_P_USDHC1_CD_B  | MUX_MODE_ALT(4) | MUX_PAD_CTRL(ESDHC_PAD_CTRL), /* Mux for CD,  GPIO4 IO22 */
	SC_P_USDHC1_VSELECT | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
};

void spl_dram_init(void)
{
	//ddr_init(&dram_timing);
	// writel(val,addr);

#ifdef SPL_DRC_0

	puts("DRAM_INIT: Start of function spl_dram_init()\n");

	DATA4(0x41C80208U, 0x1U);
	DATA4(0x41C80040U, 0xbU);
	DATA4(0x41C80204U, 0x1U);
	DATA4(0x5c000000U, 0xC1080020U);
	DATA4(0x5c000020U, 0x00000203U);
	DATA4(0x5c000024U, 0x0124F800U);
	DATA4(0x5c000050U, 0x0021F000U);
	DATA4(0x5c000064U, 0x0049006CU);
	DATA4(0x5c0000d0U, 0x40030495U);
	DATA4(0x5c0000d4U, 0x00770000U);
	DATA4(0x5c0000dcU, 0x00440024U);
	DATA4(0x5c0000e0U, 0x00F10000U);
	DATA4(0x5c0000f4U, 0x0000066FU);
	DATA4(0x5c000100U, 0x1618141AU);
	DATA4(0x5c000104U, 0x00050526U);
	DATA4(0x5c000108U, 0x060E1514U);
	DATA4(0x5c00010cU, 0x00909000U);
	DATA4(0x5c000110U, 0x0B04060BU);
	DATA4(0x5c000114U, 0x02030909U);
	DATA4(0x5c000118U, 0x02020006U);
	DATA4(0x5c00011cU, 0x00000301U);
	DATA4(0x5c000130U, 0x00020510U);
	DATA4(0x5c000134U, 0x0B100002U);
	DATA4(0x5c000138U, 0x00000071U);
	DATA4(0x5c000180U, 0x02580012U);
	DATA4(0x5c000184U, 0x01E0493EU);
	DATA4(0x5c000190U, 0x0499820AU);
	DATA4(0x5c000194U, 0x00070303U);
	DATA4(0x5c0001b4U, 0x00001708U);
	DATA4(0x5c0001b0U, 0x00000005U);
	DATA4(0x5c0001a0U, 0x00400003U);
	DATA4(0x5c0001a4U, 0x008000A0U);
	DATA4(0x5c0001a8U, 0x80000000U);
	DATA4(0x5c000200U, 0x0000001FU);
	DATA4(0x5c00020cU, 0x00000000U);
	DATA4(0x5c000210U, 0x00001F1FU);
	DATA4(0x5c000204U, 0x00080808U);
	DATA4(0x5c000214U, 0x07070707U);
	DATA4(0x5c000218U, 0x0F070707U);
	DATA4(0x5c0001c0U, 0x00000007U);
	DATA4(0x5c000244U, 0x00002211U);

	DATA4(0x5c000490U, 0x00000001U);
	DATA4(0x5c000038U, 0x003F0001U);
	DATA4(0x5c000198U, 0x0700B100U);
	DATA4(0x5c002190U, 0x00808000U);
	DATA4(0x5c000030U, 0x0000010AU);
	DATA4(0x5c000034U, 0x00402010U);
	DATA4(0x41c80208U, 0x1U);
	DATA4(0x41c80040U, 0xfU);
	DATA4(0x41c80204U, 0x1U);


	DATA4(0x5c010b04U, 0x55556000U);
	DATA4(0x5c010b08U, 0xAAAA0000U);
	DATA4(0x5c010b0cU, 0xFFE18587U);
	DATA4(0x5c010100U, 0x0000040DU);
	DATA4(0x5c010248U, 0x0001000AU);
	DATA4(0x5c010728U, 0x00061032U);
	DATA4(0x5c01072cU, 0x00004578U);
	DATA4(0x5c010828U, 0x00071032U);
	DATA4(0x5c01082cU, 0x00004685U);
	DATA4(0x5c010928U, 0x00016578U);
	DATA4(0x5c01092cU, 0x00004203U);
	DATA4(0x5c010a28U, 0x00015867U);
	DATA4(0x5c010a2cU, 0x00004320U);
	DATA4(0x5c010240U, 0x00141032U);

	DATA4(0x5c010244U, 0x0103AAAAU);


	SET_BIT4(0x5c010014U, 0x000A0000U);

	DATA4(0x5c010010U, 0x87001E00U);

	DATA4(0x5c010018U, 0x00F0A193U);
	DATA4(0x5c01001cU, 0x050A1080U);
	DATA4(0x5c010040U, 0x4B025810U);
	DATA4(0x5c010044U, 0x3A981518U);
	DATA4(0x5c010068U, 0x001C0000U);
	DATA4(0x5c0117c4U, 0x001C0000U);
	DATA4(0x5c010680U, 0x008B2C58U);
	DATA4(0x5c010684U, 0x0001BBBBU);
	DATA4(0x5c0106a4U, 0x0001B9BBU);

	DATA4(0x5c010004U, 0x32U);
	DATA4(0x5c010004U, 0x33U);
	DATA4(0x5c010184U, 0x44U);
	DATA4(0x5c010188U, 0x24U);
	DATA4(0x5c01018cU, 0xF1U);
	DATA4(0x5c0101acU, 0x54U);
	DATA4(0x5c0101d8U, 0x15U);
	DATA4(0x5c0101b0U, 0x48U);
	DATA4(0x5c0101b8U, 0x48U);
	DATA4(0x5c010110U, 0x0C331A09U);
	DATA4(0x5c010114U, 0x28300411U);
	DATA4(0x5c010118U, 0x006960E2U);
	DATA4(0x5c01011cU, 0x01800501U);
	DATA4(0x5c010120U, 0x00D82B0CU);
	DATA4(0x5c010124U, 0x194C160DU);
	DATA4(0x5c010048U, 0x000A3DEFU);
	DATA4(0x5c01004cU, 0x00249F00U);
	DATA4(0x5c010050U, 0x00000960U);
	DATA4(0x5c010054U, 0x0003A980U);
	DATA4(0x5c010058U, 0x027004B0U);
	DATA4(0x5c0104dcU, 0x00000001U);
	DATA4(0x5c010098U, 0x00000000U);
	DATA4(0x5c0104dcU, 0x00000000U);
	DATA4(0x5c010098U, 0x00000000U);
	DATA4(0x5c010500U, 0x30070800U);
	DATA4(0x5c010514U, 0x09000000U);
	DATA4(0x5c010504U, 0x44000000U);
	DATA4(0x5c010528U, 0xF0032008U);
	DATA4(0x5c01052cU, 0x07F0018FU);
	SET_BIT4(0x5c010024U, 0x4U);
	DATA4(0x5c010028U, 0x00033200U);
	DATA4(0x5c010714U, 0x09092020U);
	DATA4(0x5c010814U, 0x09092020U);
	DATA4(0x5c010914U, 0x09092020U);
	DATA4(0x5c010a14U, 0x09092020U);
	DATA4(0x5c010710U, 0x0E00BF3CU);
	DATA4(0x5c010810U, 0x0E00BF3CU);
	DATA4(0x5c010910U, 0x0E00BF3CU);
	DATA4(0x5c010a10U, 0x0E00BF3CU);
	DATA4(0x5c0117ecU, 0x001C1600U);
	DATA4(0x5c010020U, 0x001900B1U);
	DATA4(0x5c0117f0U, 0x79000000U);

	CHECK_BITS_SET4(0x5c010030U, 0x1U);
	CHECK_BITS_CLR4(0x5c010030U, 0x7FF40000U);
	DATA4(0x5c010004U, 0x180U);
	DATA4(0x5c010004U, 0x181U);
	CHECK_BITS_SET4(0x5c010030U, 0x1U);
	CHECK_BITS_CLR4(0x5c010030U, 0x7FF40000U);
	DATA4(0x5c010004U, 0x100U);
	DATA4(0x5c010004U, 0x101U);
	CHECK_BITS_SET4(0x5c010030U, 0x1U);
	CHECK_BITS_CLR4(0x5c010030U, 0x7FF40000U);
	CLR_BIT4(0x5c010250U, 0x00000001U);
	SET_BIT4(0x5c010028U, 0x00000001U);
	CHECK_BITS_SET4(0x5c010034U, 0x40000000U);
	SET_BIT4(0x5c010014U, 0x00020040U);
	DATA4(0x5c01041cU, 0x00010100U);
	DATA4(0x5c010420U, 0x700003FFU);
	DATA4(0x5c010428U, 0x00003FFFU);
	DATA4(0x5c010200U, 0x000071C7U);
	DATA4(0x5c010204U, 0x00010236U);
	DATA4(0x5c010004U, 0x200U);
	DATA4(0x5c010004U, 0x201U);
	CHECK_BITS_SET4(0x5c010030U, 0x1U);
	CHECK_BITS_CLR4(0x5c010030U, 0x00200000U);
	DATA4(0x5c0117dcU, 0x012240F7U);
	DATA4(0x5c010004U, 0x400U);
	DATA4(0x5c010004U, 0x401U);
	CHECK_BITS_SET4(0x5c010030U, 0x1U);
	CHECK_BITS_CLR4(0x5c010030U, 0x00400000U);
	DATA4(0x5c0117dcU, 0x01224000U);
	DATA4(0x5c010004U, 0x0010F800U);
	DATA4(0x5c010004U, 0x0010F801U);
	CHECK_BITS_SET4(0x5c010030U, 0x1U);
	CHECK_BITS_CLR4(0x5c010030U, 0x7FF40000U);
	DATA4(0x5c010004U, 0x00020000U);
	DATA4(0x5c010004U, 0x00020001U);
	CHECK_BITS_SET4(0x5c010030U, 0x1U);
	CHECK_BITS_CLR4(0x5c010030U, 0x00080000U);
	CLR_BIT4(0x5c010014U, 0x00020040U);
	CLR_BIT4(0x5c01070cU, 0x08000000U);
	CLR_BIT4(0x5c01080cU, 0x08000000U);
	CLR_BIT4(0x5c01090cU, 0x08000000U);
	CLR_BIT4(0x5c010a0cU, 0x08000000U);
	DATA4(0x5c010250U, 0x20188005U);
	DATA4(0x5c010254U, 0xA8AA0000U);
	DATA4(0x5c010258U, 0x00070200U);
	CLR_BIT4(0x5c010028U, 0x1U);
	DATA4(0x41c80504U, 0x400U);
	CHECK_BITS_SET4(0x5c000004U, 0x1U);

	puts("DRAM_INIT: End of function spl_dram_init()\n");

#endif
}


int board_mmc_init(bd_t *bis)
{
	int i, ret;
	sc_ipc_t ipcHndl = 0;

#ifdef CONFIG_NAND_MXS
	return 0;
#endif
        ipcHndl = gd->arch.ipc_channel_handle;
	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-boot device node)    (Physical Port)
	 * mmc0                    USDHC1
	 * mmc1                    USDHC2
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			ret = sc_pm_set_resource_power_mode(ipcHndl, SC_R_SDHC_0, SC_PM_PW_MODE_ON);
                        if (ret != SC_ERR_NONE)
                                return ret;

			imx8_iomux_setup_multiple_pads(emmc0, ARRAY_SIZE(emmc0));
			init_clk_usdhc(0);
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
			break;
		case 1:
	                ret = sc_pm_set_resource_power_mode(ipcHndl, SC_R_SDHC_1, SC_PM_PW_MODE_ON);
                        if (ret != SC_ERR_NONE)
                                return ret;
                        ret = sc_pm_set_resource_power_mode(ipcHndl, SC_R_GPIO_4, SC_PM_PW_MODE_ON);
                        if (ret != SC_ERR_NONE)
                                return ret;

			imx8_iomux_setup_multiple_pads(usdhc1_sd, ARRAY_SIZE(usdhc1_sd));
			init_clk_usdhc(1);
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			gpio_request(USDHC1_CD_GPIO, "sd1_cd");
			gpio_direction_input(USDHC1_CD_GPIO);
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return 0;
		}
		ret = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
		if (ret) {
			printf("Warning: failed to initialize mmc dev %d\n", i);
			return ret;
		}
	}

	return 0;
}

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC1_BASE_ADDR:
		ret = 1; /* eMMC */
		break;
	case USDHC2_BASE_ADDR:
		ret = !gpio_get_value(USDHC1_CD_GPIO);
		break;
	}

	return ret;
}

#endif /* CONFIG_FSL_ESDHC */

void spl_board_init(void)
{
#if defined(CONFIG_QSPI_BOOT)
        sc_ipc_t ipcHndl = 0;

        ipcHndl = gd->arch.ipc_channel_handle;
        if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_FSPI_0, SC_PM_PW_MODE_ON)) {
                puts("Warning: failed to initialize FSPI0\n");
        }
#else
	#ifdef SPL_DRC_0
        sc_ipc_t ipcHndl = 0;
		sc_pm_clock_rate_t rate = SC_1200MHZ;

        ipcHndl = gd->arch.ipc_channel_handle;
/*
        if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_DB, SC_PM_PW_MODE_ON)) {
                puts("Warning: failed to initialize DB\n");
        }*/
		if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_DRC_0, SC_PM_PW_MODE_OFF)) {
                puts("Warning: failed to turn off DRC_0\n");
        }

        if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_DRC_0, SC_PM_PW_MODE_ON)) {
                puts("Warning: failed to initialize DRC_0\n");
        }

		if (sc_pm_get_clock_rate(ipcHndl, SC_R_DRC_0, SC_PM_CLK_MISC0, &rate)) {
				puts("Warning: failed to get DRC_0 clkrate\n");
		}
/*
		if (sc_pm_clock_enable(ipcHndl, SC_R_DRC_0, SC_PM_CLK_MISC0, SC_FALSE, SC_TRUE)) {
				puts("Warning: failed to turn off DRC_0 clk\n");
		}
		if (sc_pm_clock_enable(ipcHndl, SC_R_DRC_0, SC_PM_CLK_MISC0, SC_TRUE, SC_TRUE)) {
				puts("Warning: failed to initialize DRC_0 clk\n");
		}*/
		printf("DRC_0 cloackrate: %d Hz\n",rate);
		//sc_err_t sc_pm_clock_enable(ipcHndl, SC_R_DRC_0, SC_PM_CLK_MISC0, SC_TRUE, SC_TRUE);
		//sc_err_t sc_pm_get_clock_rate(ipcHndl, SC_R_DRC_0, SC_PM_CLK_MISC0, &rate);
		//sc_pm_set_clock_rate(SC_PT, SC_R_DRC_0, SC_PM_CLK_MISC0, &ddr_pll);
		//if(sc_pm_set_clock_rate(ipcHndl, SC_R_DRC_0, SC_PM_CLK_MISC0, &rate)) {
		//		puts("Warning: failed to set clockrate of DRC_0\n");
		//}
	#endif
#endif
        /* DDR initialization */
        spl_dram_init();

        puts("Normal Boot\n");
}

void spl_board_prepare_for_boot(void)
{
#if defined(CONFIG_QSPI_BOOT)
        sc_ipc_t ipcHndl = 0;

        ipcHndl = gd->arch.ipc_channel_handle;
        if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_FSPI_0, SC_PM_PW_MODE_OFF)) {
                puts("Warning: failed to turn off FSPI0\n");
        }
#else
	#ifdef SPL_DRC_0
        sc_ipc_t ipcHndl = 0;

        ipcHndl = gd->arch.ipc_channel_handle;
/*
        if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_DB, SC_PM_PW_MODE_OFF)) {
                puts("Warning: failed to turn off DB\n");
        }*/
        if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_DRC_0, SC_PM_PW_MODE_OFF)) {
                puts("Warning: failed to turn off DRC_0\n");
        }
	#endif
#endif
}

void board_init_f(ulong dummy)
{
        /* Clear global data */
        memset((void *)gd, 0, sizeof(gd_t));

        arch_cpu_init();

        board_early_init_f();

        timer_init();

        preloader_console_init();

        /* Clear the BSS. */
        memset(__bss_start, 0, __bss_end - __bss_start);

        board_init_r(NULL, 0);
}

