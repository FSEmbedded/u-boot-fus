// SPDX-License-Identifier: GPL-2.0+
/*
* Copyright 2024 F&S Elektronik Systeme GmbH
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include <common.h>
#include <env.h>
#include <efi_loader.h>
#include <init.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/global_data.h>
#include <asm/arch-imx9/ccm_regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch-imx9/imx93_pins.h>
#include <asm/arch/clock.h>
#include <power/pmic.h>
#include "../common/tcpc.h"
#include <dm/device.h>
#include <dm/uclass.h>
#include <usb.h>
#include <dwc3-uboot.h>
#include <asm/gpio.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL	(PAD_CTL_DSE(6) | PAD_CTL_FSEL2)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE(6) | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)

static iomux_v3_cfg_t const uart_pads[] = {
	MX93_PAD_UART2_RXD__LPUART2_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX93_PAD_UART2_TXD__LPUART2_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

#if CONFIG_IS_ENABLED(EFI_HAVE_CAPSULE_SUPPORT)
#define IMX_BOOT_IMAGE_GUID \
	EFI_GUID(0xbc550d86, 0xda26, 0x4b70, 0xac, 0x05, \
		 0x2a, 0x44, 0x8e, 0xda, 0x6f, 0x21)

struct efi_fw_image fw_images[] = {
	{
		.image_type_id = IMX_BOOT_IMAGE_GUID,
		.fw_name = u"IMX93-11X11-EVK-RAW",
		.image_index = 1,
	},
};

struct efi_capsule_update_info update_info = {
	.dfu_string = "mmc 0=flash-bin raw 0 0x2000 mmcpart 1",
	.images = fw_images,
};

u8 num_image_type_guids = ARRAY_SIZE(fw_images);
#endif /* EFI_HAVE_CAPSULE_SUPPORT */

int board_early_init_f(void)
{
	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

	init_uart_clk(LPUART2_CLK_ROOT);

	return 0;
}

#if defined(CONFIG_OF_BOARD_SETUP)
int	ft_board_setup(void *fdt_blob, struct bd_info *bd)
{
	return fdt_fixup_memory(fdt_blob,
			CFG_SYS_SDRAM_BASE, gd->bd->bi_dram[0].size);
}
#endif

static int setup_fec(void)
{
	return set_clk_enet(ENET_125MHZ);
}

int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

static int setup_eqos(void)
{
	struct blk_ctrl_wakeupmix_regs *bctrl =
		(struct blk_ctrl_wakeupmix_regs *)BLK_CTRL_WAKEUPMIX_BASE_ADDR;

	if (!IS_ENABLED(CONFIG_TARGET_IMX93_14X14_EVK)) {
		/* set INTF as RGMII, enable RGMII TXC clock */
		clrsetbits_le32(&bctrl->eqos_gpr,
				BCTRL_GPR_ENET_QOS_INTF_MODE_MASK,
				BCTRL_GPR_ENET_QOS_INTF_SEL_RGMII | BCTRL_GPR_ENET_QOS_CLK_GEN_EN);

		return set_clk_eqos(ENET_125MHZ);
	}

	return 0;
}

int board_init(void)
{
#ifdef CONFIG_USB_TCPC
	setup_typec();
#endif

	if (IS_ENABLED(CONFIG_FEC_MXC))
		setup_fec();

	if (IS_ENABLED(CONFIG_DWC_ETH_QOS))
		setup_eqos();

	return 0;
}

int board_late_init(void)
{
#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#endif

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "11X11_EVK");
	env_set("board_rev", "iMX93");
#endif
	return 0;
}

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
	return 0;
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/
