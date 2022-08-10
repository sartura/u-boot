// SPDX-License-Identifier: GPL-2.0-or-later OR MIT
/*
 * Copyright (c) 2018 Microsemi Corporation
 * Copyright (c) 2022 Sartura Ltd.
 */

#include <common.h>
#include <miiphy.h>
#include <asm/armv8/mmu.h>
#include <asm/io.h>
#include <linux/sizes.h>
#include <linux/delay.h>
#include <debug_uart.h>
#include <spi.h>
#include <asm/sections.h>
#include <env_internal.h>
#include <command.h>
#include <wait_bit.h>

#include "mscc_fa_regs_devcpu_gcb.h"
#include "mscc_fa_regs_cpu.h"
#include "mscc_fa_regs_subcpu_sys_cfg.h"

#include "fa_helpers.h"

DECLARE_GLOBAL_DATA_PTR;

static inline void mscc_gpio_set_alternate_0(int gpio, int mode)
{
	u32 mask = BIT(gpio);
	u32 val0, val1;

	val0 = readl(MSCC_DEVCPU_GCB_GPIO_ALT(0));
	val1 = readl(MSCC_DEVCPU_GCB_GPIO_ALT(1));
	if (mode == 1) {
		val0 |= mask;
		val1 &= ~mask;
	} else if (mode == 2) {
		val0 &= ~mask;
		val1 |= mask;
	} else if (mode == 3) {
		val0 |= mask;
		val1 |= mask;
	} else {
		val0 &= ~mask;
		val1 &= ~mask;
	}
	writel(val0, MSCC_DEVCPU_GCB_GPIO_ALT(0));
	writel(val1, MSCC_DEVCPU_GCB_GPIO_ALT(1));
}

static inline void mscc_gpio_set_alternate_1(int gpio, int mode)
{
	u32 mask = BIT(gpio);
	u32 val0, val1;

	val0 = readl(MSCC_DEVCPU_GCB_GPIO_ALT1(0));
	val1 = readl(MSCC_DEVCPU_GCB_GPIO_ALT1(1));
	if (mode == 1) {
		val0 |= mask;
		val1 &= ~mask;
	} else if (mode == 2) {
		val0 &= ~mask;
		val1 |= mask;
	} else if (mode == 3) {
		val0 |= mask;
		val1 |= mask;
	} else {
		val0 &= ~mask;
		val1 &= ~mask;
	}
	writel(val0, MSCC_DEVCPU_GCB_GPIO_ALT1(0));
	writel(val1, MSCC_DEVCPU_GCB_GPIO_ALT1(1));
}

static inline void mscc_gpio_set_alternate(int gpio, int mode)
{
	if (gpio < 32)
		mscc_gpio_set_alternate_0(gpio, mode);
	else
		mscc_gpio_set_alternate_1(gpio - 32, mode);
}

#ifdef CONFIG_DEBUG_UART_BOARD_INIT
void board_debug_uart_init(void)
{
	mscc_gpio_set_alternate(10, 1);
	mscc_gpio_set_alternate(11, 1);
}
#endif

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_1_SIZE;

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	if (of_machine_is_compatible("microchip,sparx5-pcb134")) {
		/* reg 23 - PHY Control Register #1 */
		/* Setup skew and RX idle clock, datasheet table 36. */
		/* 15:12 - AC/Media Interface Mode Select:
		 * SGMII CAT5:
		 * Modified Clause 37 auto-negotiation disabled,
		 * 625MHz SCLK Clock Disabled
		 */
		phy_write(phydev, MDIO_DEVAD_NONE, MII_RESV1, 0xba20); /* Set SGMII mode */
		phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, 0x9040); /* Reset */
		phy_write(phydev, MDIO_DEVAD_NONE, MII_ADVERTISE, 0x0de1); /* Setup ANEG */
		phy_write(phydev, MDIO_DEVAD_NONE, MII_CTRL1000, 0x0200); /* Setup ANEG */
		phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, 0x1240); /* Restart ANEG */
	}

	return 0;
}

int board_init(void)
{
	/* CS0 alone on boot master region 0 - SPI NOR flash */
	writel((u16) ~BIT(0), MSCC_DEVCPU_GCB_SPI_MASTER_SS0_MASK);

	return 0;
}
