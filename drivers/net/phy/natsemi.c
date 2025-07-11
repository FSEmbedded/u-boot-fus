// SPDX-License-Identifier: GPL-2.0+
/*
 * National Semiconductor PHY drivers
 *
 * Copyright 2010-2011 Freescale Semiconductor, Inc.
 * author Andy Fleming
 */
#include <common.h>
#include <phy.h>

/* NatSemi DP83630 */

#define DP83630_PHY_PAGESEL_REG		0x13
#define DP83630_PHY_PTP_COC_REG		0x14
#define DP83630_PHY_PTP_CLKOUT_EN	(1<<15)
#define DP83630_PHY_RBR_REG		0x17

static int dp83630_config(struct phy_device *phydev)
{
	int ptp_coc_reg;

	phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, BMCR_RESET);
	phy_write(phydev, MDIO_DEVAD_NONE, DP83630_PHY_PAGESEL_REG, 0x6);
	ptp_coc_reg = phy_read(phydev, MDIO_DEVAD_NONE,
			       DP83630_PHY_PTP_COC_REG);
	ptp_coc_reg &= ~DP83630_PHY_PTP_CLKOUT_EN;
	phy_write(phydev, MDIO_DEVAD_NONE, DP83630_PHY_PTP_COC_REG,
		  ptp_coc_reg);
	phy_write(phydev, MDIO_DEVAD_NONE, DP83630_PHY_PAGESEL_REG, 0);

	genphy_config_aneg(phydev);

	return 0;
}

U_BOOT_PHY_DRIVER(dp83630) = {
	.name = "NatSemi DP83630",
	.uid = 0x20005ce1,
	.mask = 0xfffffff0,
	.features = PHY_BASIC_FEATURES,
	.config = &dp83630_config,
	.startup = &genphy_startup,
	.shutdown = &genphy_shutdown,
};


/* DP83865 Link and Auto-Neg Status Register */
#define MIIM_DP83865_LANR      0x11
#define MIIM_DP83865_SPD_MASK  0x0018
#define MIIM_DP83865_SPD_1000  0x0010
#define MIIM_DP83865_SPD_100   0x0008
#define MIIM_DP83865_DPX_FULL  0x0002


/* NatSemi DP83865 */
static int dp838xx_config(struct phy_device *phydev)
{
	phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, BMCR_RESET);
	genphy_config_aneg(phydev);

	return 0;
}

static int dp83865_parse_status(struct phy_device *phydev)
{
	int mii_reg;

	mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MIIM_DP83865_LANR);

	switch (mii_reg & MIIM_DP83865_SPD_MASK) {

	case MIIM_DP83865_SPD_1000:
		phydev->speed = SPEED_1000;
		break;

	case MIIM_DP83865_SPD_100:
		phydev->speed = SPEED_100;
		break;

	default:
		phydev->speed = SPEED_10;
		break;

	}

	if (mii_reg & MIIM_DP83865_DPX_FULL)
		phydev->duplex = DUPLEX_FULL;
	else
		phydev->duplex = DUPLEX_HALF;

	return 0;
}

static int dp83865_startup(struct phy_device *phydev)
{
	int ret;

	ret = genphy_update_link(phydev);
	if (ret)
		return ret;

	return dp83865_parse_status(phydev);
}


U_BOOT_PHY_DRIVER(dp83865) = {
	.name = "NatSemi DP83865",
	.uid = 0x20005c70,
	.mask = 0xfffffff0,
	.features = PHY_GBIT_FEATURES,
	.config = &dp838xx_config,
	.startup = &dp83865_startup,
	.shutdown = &genphy_shutdown,
};

/* NatSemi DP83848 */
#define MIIM_DP83848_RBR 0x17
#define MIIM_DP83848_PHYCR 0x19

static int dp83848_config(struct phy_device *phydev)
{
	int reg;

#if 0
	/* Switch to RMII rev. 1.0 (instead of rev. 1.2) */
	reg = phy_read(phydev, MDIO_DEVAD_NONE, MIIM_DP83848_RBR);
	reg |= 0x10;
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_DP83848_RBR, reg);
#endif

	reg = phy_read(phydev, MDIO_DEVAD_NONE, MIIM_DP83848_PHYCR);
	/* LED Mode 2: Show ethernet activity on LINK LED */
	reg &= ~0x20;
	/* Auto-MDIX: Enable automatic crossover detection */
	reg |= 0x8000;
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_DP83848_PHYCR, reg);

	genphy_config(phydev);

	return 0;
}

static int dp83848_parse_status(struct phy_device *phydev)
{
	int mii_reg;

	mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);

	if(mii_reg & (BMSR_100FULL | BMSR_100HALF)) {
		phydev->speed = SPEED_100;
	} else {
		phydev->speed = SPEED_10;
	}

	if (mii_reg & (BMSR_10FULL | BMSR_100FULL)) {
		phydev->duplex = DUPLEX_FULL;
	} else {
		phydev->duplex = DUPLEX_HALF;
	}

	return 0;
}

static int dp83848_startup(struct phy_device *phydev)
{
	int ret;

	ret = genphy_update_link(phydev);
	if (ret)
		return ret;

	return dp83848_parse_status(phydev);
}

U_BOOT_PHY_DRIVER(dp83848) = {
	.name = "NatSemi DP83848",
	.uid = 0x20005c90,
	.mask = 0xfffffff0,
//###??	.mask = 0x2000ff90,
	.features = PHY_BASIC_FEATURES,
	.config = &dp83848_config,
	.startup = &dp83848_startup,
	.shutdown = &genphy_shutdown,
};
