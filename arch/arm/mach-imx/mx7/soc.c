// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2017-2018 NXP
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/dma.h>
#include <asm/mach-imx/hab.h>
#include <asm/mach-imx/rdc-sema.h>
#include <asm/arch/imx-rdc.h>
#include <asm/arch/crm_regs.h>
#include <dm.h>
#include <env.h>
#include <imx_thermal.h>
#include <fsl_sec.h>
#include <asm/setup.h>
#include <fsl_wdog.h>

#define IOMUXC_GPR1		0x4
#define BM_IOMUXC_GPR1_IRQ	0x1000

#define GPC_LPCR_A7_BSC		0x0
#define GPC_LPCR_M4		0x8
#define GPC_SLPCR		0x14
#define GPC_PGC_ACK_SEL_A7	0x24
#define GPC_IMR1_CORE0		0x30
#define GPC_IMR1_CORE1		0x40
#define GPC_IMR1_M4		0x50
#define GPC_PGC_CPU_MAPPING	0xec
#define GPC_PGC_C0_PUPSCR	0x804
#define GPC_PGC_SCU_TIMING	0x890
#define GPC_PGC_C1_PUPSCR	0x844

#define BM_LPCR_A7_BSC_IRQ_SRC_A7_WAKEUP	0x70000000
#define BM_LPCR_A7_BSC_CPU_CLK_ON_LPM		0x4000
#define BM_LPCR_M4_MASK_DSM_TRIGGER		0x80000000
#define BM_SLPCR_EN_DSM				0x80000000
#define BM_SLPCR_RBC_EN				0x40000000
#define BM_SLPCR_REG_BYPASS_COUNT		0x3f000000
#define BM_SLPCR_VSTBY				0x4
#define BM_SLPCR_SBYOS				0x2
#define BM_SLPCR_BYPASS_PMIC_READY		0x1
#define BM_SLPCR_EN_A7_FASTWUP_WAIT_MODE	0x10000

#define BM_GPC_PGC_ACK_SEL_A7_DUMMY_PUP_ACK	0x80000000
#define BM_GPC_PGC_ACK_SEL_A7_DUMMY_PDN_ACK	0x8000

#define BM_GPC_PGC_CORE_PUPSCR			0x7fff80
#ifdef CONFIG_IMX_SEC_INIT
#include <fsl_caam.h>
#endif

#define IOMUXC_GPR1		0x4
#define BM_IOMUXC_GPR1_IRQ	0x1000

#define GPC_LPCR_A7_BSC		0x0
#define GPC_LPCR_M4		0x8
#define GPC_SLPCR		0x14
#define GPC_PGC_ACK_SEL_A7	0x24
#define GPC_IMR1_CORE0		0x30
#define GPC_IMR1_CORE1		0x40
#define GPC_IMR1_M4		0x50
#define GPC_PGC_CPU_MAPPING	0xec
#define GPC_PGC_C0_PUPSCR	0x804
#define GPC_PGC_SCU_TIMING	0x890
#define GPC_PGC_C1_PUPSCR	0x844

#define BM_LPCR_A7_BSC_IRQ_SRC_A7_WAKEUP	0x70000000
#define BM_LPCR_A7_BSC_CPU_CLK_ON_LPM		0x4000
#define BM_LPCR_M4_MASK_DSM_TRIGGER		0x80000000
#define BM_SLPCR_EN_DSM				0x80000000
#define BM_SLPCR_RBC_EN				0x40000000
#define BM_SLPCR_REG_BYPASS_COUNT		0x3f000000
#define BM_SLPCR_VSTBY				0x4
#define BM_SLPCR_SBYOS				0x2
#define BM_SLPCR_BYPASS_PMIC_READY		0x1
#define BM_SLPCR_EN_A7_FASTWUP_WAIT_MODE	0x10000

#define BM_GPC_PGC_ACK_SEL_A7_DUMMY_PUP_ACK	0x80000000
#define BM_GPC_PGC_ACK_SEL_A7_DUMMY_PDN_ACK	0x8000

#define BM_GPC_PGC_CORE_PUPSCR			0x7fff80

#if defined(CONFIG_IMX_THERMAL)
static const struct imx_thermal_plat imx7_thermal_plat = {
	.regs = (void *)ANATOP_BASE_ADDR,
	.fuse_bank = 3,
	.fuse_word = 3,
};

U_BOOT_DEVICE(imx7_thermal) = {
	.name = "imx_thermal",
	.platdata = &imx7_thermal_plat,
};
#endif

#if CONFIG_IS_ENABLED(IMX_RDC)
/*
 * In current design, if any peripheral was assigned to both A7 and M4,
 * it will receive ipg_stop or ipg_wait when any of the 2 platforms enter
 * low power mode. So M4 sleep will cause some peripherals fail to work
 * at A7 core side. At default, all resources are in domain 0 - 3.
 *
 * There are 26 peripherals impacted by this IC issue:
 * SIM2(sim2/emvsim2)
 * SIM1(sim1/emvsim1)
 * UART1/UART2/UART3/UART4/UART5/UART6/UART7
 * SAI1/SAI2/SAI3
 * WDOG1/WDOG2/WDOG3/WDOG4
 * GPT1/GPT2/GPT3/GPT4
 * PWM1/PWM2/PWM3/PWM4
 * ENET1/ENET2
 * Software Workaround:
 * Here we setup some resources to domain 0 where M4 codes will move
 * the M4 out of this domain. Then M4 is not able to access them any longer.
 * This is a workaround for ic issue. So the peripherals are not shared
 * by them. This way requires the uboot implemented the RDC driver and
 * set the 26 IPs above to domain 0 only. M4 code will assign resource
 * to its own domain, if it want to use the resource.
 */
static rdc_peri_cfg_t const resources[] = {
	(RDC_PER_SIM1 | RDC_DOMAIN(0)),
	(RDC_PER_SIM2 | RDC_DOMAIN(0)),
	(RDC_PER_UART1 | RDC_DOMAIN(0)),
	(RDC_PER_UART2 | RDC_DOMAIN(0)),
	(RDC_PER_UART3 | RDC_DOMAIN(0)),
	(RDC_PER_UART4 | RDC_DOMAIN(0)),
	(RDC_PER_UART5 | RDC_DOMAIN(0)),
	(RDC_PER_UART6 | RDC_DOMAIN(0)),
	(RDC_PER_UART7 | RDC_DOMAIN(0)),
	(RDC_PER_SAI1 | RDC_DOMAIN(0)),
	(RDC_PER_SAI2 | RDC_DOMAIN(0)),
	(RDC_PER_SAI3 | RDC_DOMAIN(0)),
	(RDC_PER_WDOG1 | RDC_DOMAIN(0)),
	(RDC_PER_WDOG2 | RDC_DOMAIN(0)),
	(RDC_PER_WDOG3 | RDC_DOMAIN(0)),
	(RDC_PER_WDOG4 | RDC_DOMAIN(0)),
	(RDC_PER_GPT1 | RDC_DOMAIN(0)),
	(RDC_PER_GPT2 | RDC_DOMAIN(0)),
	(RDC_PER_GPT3 | RDC_DOMAIN(0)),
	(RDC_PER_GPT4 | RDC_DOMAIN(0)),
	(RDC_PER_PWM1 | RDC_DOMAIN(0)),
	(RDC_PER_PWM2 | RDC_DOMAIN(0)),
	(RDC_PER_PWM3 | RDC_DOMAIN(0)),
	(RDC_PER_PWM4 | RDC_DOMAIN(0)),
	(RDC_PER_ENET1 | RDC_DOMAIN(0)),
	(RDC_PER_ENET2 | RDC_DOMAIN(0)),
};

static void isolate_resource(void)
{
	imx_rdc_setup_peripherals(resources, ARRAY_SIZE(resources));
}
#endif

#if defined(CONFIG_IMX_HAB) || defined(CONFIG_AVB_ATX)
struct imx_sec_config_fuse_t const imx_sec_config_fuse = {
	.bank = 1,
	.word = 3,
};
#endif

static bool is_mx7d(void)
{
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[1];
	struct fuse_bank1_regs *fuse =
		(struct fuse_bank1_regs *)bank->fuse_regs;
	int val;

	val = readl(&fuse->tester4);
	if (val & 1)
		return false;
	else
		return true;
}

u32 get_cpu_rev(void)
{
	struct mxc_ccm_anatop_reg *ccm_anatop = (struct mxc_ccm_anatop_reg *)
						 ANATOP_BASE_ADDR;
	u32 reg = readl(&ccm_anatop->digprog);
	u32 type = (reg >> 16) & 0xff;

	if (!is_mx7d())
		type = MXC_CPU_MX7S;

	reg &= 0xff;
	return (type << 12) | reg;
}

#ifdef CONFIG_REVISION_TAG
u32 __weak get_board_rev(void)
{
	return get_cpu_rev();
}
#endif

static void imx_enet_mdio_fixup(void)
{
	struct iomuxc_gpr_base_regs *gpr_regs =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/*
	 * The management data input/output (MDIO) requires open-drain,
	 * i.MX7D TO1.0 ENET MDIO pin has no open drain, but TO1.1 supports
	 * this feature. So to TO1.1, need to enable open drain by setting
	 * bits GPR0[8:7].
	 */

	if (soc_rev() >= CHIP_REV_1_1) {
		setbits_le32(&gpr_regs->gpr[0],
			     IOMUXC_GPR_GPR0_ENET_MDIO_OPEN_DRAIN_MASK);
	}
}

static void init_cpu_basic(void)
{
	imx_enet_mdio_fixup();

#ifdef CONFIG_APBH_DMA
	/* Start APBH DMA */
	mxs_dma_init();
#endif
}

#ifndef CONFIG_SKIP_LOWLEVEL_INIT
/* enable all periherial can be accessed in nosec mode */
static void init_csu(void)
{
	int i = 0;

	for (i = 0; i < CSU_NUM_REGS; i++)
		writel(CSU_INIT_SEC_LEVEL0, CSU_IPS_BASE_ADDR + i * 4);
}

static void imx_gpcv2_init(void)
{
	u32 val, i;

	/*
	 * Force IOMUXC irq pending, so that the interrupt to GPC can be
	 * used to deassert dsm_request signal when the signal gets
	 * asserted unexpectedly.
	 */
	val = readl(IOMUXC_GPR_BASE_ADDR + IOMUXC_GPR1);
	val |= BM_IOMUXC_GPR1_IRQ;
	writel(val, IOMUXC_GPR_BASE_ADDR + IOMUXC_GPR1);

	/* Initially mask all interrupts */
	for (i = 0; i < 4; i++) {
		writel(~0, GPC_IPS_BASE_ADDR + GPC_IMR1_CORE0 + i * 4);
		writel(~0, GPC_IPS_BASE_ADDR + GPC_IMR1_CORE1 + i * 4);
		writel(~0, GPC_IPS_BASE_ADDR + GPC_IMR1_M4 + i * 4);
	}

	/* set SCU timing */
	writel((0x59 << 10) | 0x5B | (0x2 << 20),
	       GPC_IPS_BASE_ADDR + GPC_PGC_SCU_TIMING);

	/* only external IRQs to wake up LPM and core 0/1 */
	val = readl(GPC_IPS_BASE_ADDR + GPC_LPCR_A7_BSC);
	val |= BM_LPCR_A7_BSC_IRQ_SRC_A7_WAKEUP;
	writel(val, GPC_IPS_BASE_ADDR + GPC_LPCR_A7_BSC);

	/* set C0 power up timming per design requirement */
	val = readl(GPC_IPS_BASE_ADDR + GPC_PGC_C0_PUPSCR);
	val &= ~BM_GPC_PGC_CORE_PUPSCR;
	val |= (0x1A << 7);
	writel(val, GPC_IPS_BASE_ADDR + GPC_PGC_C0_PUPSCR);

	/* set C1 power up timming per design requirement */
	val = readl(GPC_IPS_BASE_ADDR + GPC_PGC_C1_PUPSCR);
	val &= ~BM_GPC_PGC_CORE_PUPSCR;
	val |= (0x1A << 7);
	writel(val, GPC_IPS_BASE_ADDR + GPC_PGC_C1_PUPSCR);

	/* dummy ack for time slot by default */
	writel(BM_GPC_PGC_ACK_SEL_A7_DUMMY_PUP_ACK |
		BM_GPC_PGC_ACK_SEL_A7_DUMMY_PDN_ACK,
		GPC_IPS_BASE_ADDR + GPC_PGC_ACK_SEL_A7);

	/* mask M4 DSM trigger */
	writel(readl(GPC_IPS_BASE_ADDR + GPC_LPCR_M4) |
		 BM_LPCR_M4_MASK_DSM_TRIGGER,
		 GPC_IPS_BASE_ADDR + GPC_LPCR_M4);

	/* set mega/fast mix in A7 domain */
	writel(0x1, GPC_IPS_BASE_ADDR + GPC_PGC_CPU_MAPPING);

	/* DSM related settings */
	val = readl(GPC_IPS_BASE_ADDR + GPC_SLPCR);
	val &= ~(BM_SLPCR_EN_DSM | BM_SLPCR_VSTBY | BM_SLPCR_RBC_EN |
		BM_SLPCR_SBYOS | BM_SLPCR_BYPASS_PMIC_READY |
		BM_SLPCR_REG_BYPASS_COUNT);
	val |= BM_SLPCR_EN_A7_FASTWUP_WAIT_MODE;
	writel(val, GPC_IPS_BASE_ADDR + GPC_SLPCR);

	/*
	 * disabling RBC need to delay at least 2 cycles of CKIL(32K)
	 * due to hardware design requirement, which is
	 * ~61us, here we use 65us for safe
	 */
	udelay(65);
}

static void set_epdc_qos(void)
{
	writel(0, REGS_QOS_BASE);  /*  Disable clkgate & soft_reset */
	writel(0, REGS_QOS_BASE + 0x60);  /*  Enable all masters */
	writel(0, REGS_QOS_EPDC);   /*  Disable clkgate & soft_reset */
	writel(0, REGS_QOS_PXP0);   /*  Disable clkgate & soft_reset */
	writel(0, REGS_QOS_PXP1);   /*  Disable clkgate & soft_reset */

	writel(0x0f020722, REGS_QOS_EPDC + 0xd0);   /*  WR, init = 7 with red flag */
	writel(0x0f020722, REGS_QOS_EPDC + 0xe0);   /*  RD,  init = 7 with red flag */

	writel(1, REGS_QOS_PXP0);   /*  OT_CTRL_EN =1 */
	writel(1, REGS_QOS_PXP1);   /*  OT_CTRL_EN =1 */

	writel(0x0f020222, REGS_QOS_PXP0 + 0x50);   /*  WR,  init = 2 with red flag */
	writel(0x0f020222, REGS_QOS_PXP1 + 0x50);   /*  WR,  init = 2 with red flag */
	writel(0x0f020222, REGS_QOS_PXP0 + 0x60);   /*  rD,  init = 2 with red flag */
	writel(0x0f020222, REGS_QOS_PXP1 + 0x60);   /*  rD,  init = 2 with red flag */
	writel(0x0f020422, REGS_QOS_PXP0 + 0x70);   /*  tOTAL,  init = 4 with red flag */
	writel(0x0f020422, REGS_QOS_PXP1 + 0x70);   /*  TOTAL,  init = 4 with red flag */

	writel(0xe080, IOMUXC_GPR_BASE_ADDR + 0x0034); /* EPDC AW/AR CACHE ENABLE */
}

bool is_usb_boot(void)
{
	if (gd->flags & GD_FLG_ARCH_IMX_USB_BOOT)
		return true;

	return false;
}

int arch_cpu_init(void)
{
	if (is_usbotg_boot_enabled())
		gd->flags |= GD_FLG_ARCH_IMX_USB_BOOT;

	init_aips();

	init_csu();
	/* Disable PDE bit of WMCR register */
	imx_wdog_disable_powerdown();

	init_cpu_basic();

	set_epdc_qos();

#if CONFIG_IS_ENABLED(IMX_RDC)
	isolate_resource();
#endif

	init_snvs();

	imx_gpcv2_init();
#ifdef CONFIG_IMX_SEC_INIT
	/* Secure init function such RNG */
	imx_sec_init();
#endif

	return 0;
}
#else
int arch_cpu_init(void)
{
	init_cpu_basic();

	return 0;
}
#endif

#ifdef CONFIG_ARCH_MISC_INIT
int arch_misc_init(void)
{
#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	if (is_mx7d())
		env_set("soc", "imx7d");
	else
		env_set("soc", "imx7s");
#endif

#ifdef CONFIG_FSL_CAAM
	sec_init();
#endif

	return 0;
}
#endif

#ifdef CONFIG_SERIAL_TAG
/*
 * OCOTP_TESTER
 * i.MX 7Solo Applications Processor Reference Manual, Rev. 0.1, 08/2016
 * OCOTP_TESTER describes a unique ID based on silicon wafer
 * and die X/Y position
 *
 * OCOTOP_TESTER offset 0x410
 * 31:0 fuse 0
 * FSL-wide unique, encoded LOT ID STD II/SJC CHALLENGE/ Unique ID
 *
 * OCOTP_TESTER1 offset 0x420
 * 31:24 fuse 1
 * The X-coordinate of the die location on the wafer/SJC CHALLENGE/ Unique ID
 * 23:16 fuse 1
 * The Y-coordinate of the die location on the wafer/SJC CHALLENGE/ Unique ID
 * 15:11 fuse 1
 * The wafer number of the wafer on which the device was fabricated/SJC
 * CHALLENGE/ Unique ID
 * 10:0 fuse 1
 * FSL-wide unique, encoded LOT ID STD II/SJC CHALLENGE/ Unique ID
 */
void get_board_serial(struct tag_serialnr *serialnr)
{
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[0];
	struct fuse_bank0_regs *fuse =
		(struct fuse_bank0_regs *)bank->fuse_regs;

	serialnr->low = fuse->tester0;
	serialnr->high = fuse->tester1;
}
#endif

void set_wdog_reset(struct wdog_regs *wdog)
{
	u32 reg = readw(&wdog->wcr);
	/*
	 * Output WDOG_B signal to reset external pmic or POR_B decided by
	 * the board desgin. Without external reset, the peripherals/DDR/
	 * PMIC are not reset, that may cause system working abnormal.
	 */
	reg = readw(&wdog->wcr);
	reg |= 1 << 3;
	/*
	 * WDZST bit is write-once only bit. Align this bit in kernel,
	 * otherwise kernel code will have no chance to set this bit.
	 */
	reg |= 1 << 0;
	writew(reg, &wdog->wcr);
}

void s_init(void)
{
	/* clock configuration. */
	clock_init();

#if defined(CONFIG_ANDROID_SUPPORT)
        /* Enable RTC */
        writel(0x21, 0x30370038);
#endif
	return;
}

void reset_misc(void)
{
#ifndef CONFIG_SPL_BUILD
#if defined(CONFIG_VIDEO_MXS) && !defined(CONFIG_DM_VIDEO)
	lcdif_power_down();
#endif
#endif
}

#ifdef CONFIG_IMX_TRUSTY_OS
#ifdef CONFIG_MX7D
void smp_set_core_boot_addr(unsigned long addr, int corenr)
{
            return;
}

void smp_waitloop(unsigned previous_address)
{
            return;
}
#endif
#endif

void reset_cpu(ulong addr)
{
	struct watchdog_regs *wdog = (struct watchdog_regs *)WDOG1_BASE_ADDR;

	/* Clear WDA to trigger WDOG_B immediately */
	writew((WCR_WDE | WCR_SRS), &wdog->wcr);

	while (1) {
		/*
		 * spin for .5 seconds before reset
		 */
	}
}
