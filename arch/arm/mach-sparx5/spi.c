// SPDX-License-Identifier: GPL-2.0-or-later OR MIT
/*
 * Copyright (c) 2018 Microsemi Corporation
 * Copyright (c) 2022 Sartura Ltd.
 */

#include <common.h>
#include <dm.h>
#include <spi.h>
#include <asm/io.h>
#include <linux/delay.h>

#include "mscc_fa_regs_cpu.h"

#define SPI_OWNER_MASK				\
	MSCC_M_CPU_GENERAL_CTRL_IF_SI2_OWNER |	\
	MSCC_M_CPU_GENERAL_CTRL_IF_SI_OWNER

#define SPI_OWNER_SPI1_MASK			  \
	MSCC_F_CPU_GENERAL_CTRL_IF_SI2_OWNER(1) | \
	MSCC_F_CPU_GENERAL_CTRL_IF_SI_OWNER(2)

#define SPI_OWNER_SPI2_MASK			  \
	MSCC_F_CPU_GENERAL_CTRL_IF_SI2_OWNER(2) | \
	MSCC_F_CPU_GENERAL_CTRL_IF_SI_OWNER(1)

static bool spi_cs_init;
static u32 spi2mask;

static inline void spi_set_owner(u32 cs)
{
	u32 owbits;
	if (spi2mask & BIT(cs)) {
		owbits = SPI_OWNER_SPI2_MASK;
	} else {
		owbits = SPI_OWNER_SPI1_MASK;
	}
	clrsetbits_le32(MSCC_CPU_GENERAL_CTRL,
			SPI_OWNER_MASK, owbits);
	udelay(1);
}

static void spi_manage_init(struct udevice *dev)
{
	struct udevice *paren = dev_get_parent(dev);
	spi2mask = dev_read_u32_default(paren, "interface-mapping-mask", 0);
	debug("spi2mask = %08x\n", spi2mask);
}

void external_cs_manage(struct udevice *dev, bool nen)
{
	bool enable = !nen;
	u32 cs = spi_chip_select(dev);
	if (!spi_cs_init) {
		spi_manage_init(dev);
		spi_cs_init = true;
	}
	if (enable) {
		u32 mask = ~BIT(cs); /* Active low */
		/* CS - start disabled */
		clrsetbits_le32(MSCC_CPU_SS_FORCE,
				MSCC_M_CPU_SS_FORCE_SS_FORCE,
				MSCC_F_CPU_SS_FORCE_SS_FORCE(0xffff));
		/* CS override drive enable */
		clrsetbits_le32(MSCC_CPU_SS_FORCE_ENA,
				MSCC_M_CPU_SS_FORCE_ENA_SS_FORCE_ENA,
				MSCC_F_CPU_SS_FORCE_ENA_SS_FORCE_ENA(true));
		udelay(1);
		/* Set owner */
		spi_set_owner(cs);
		/* CS - enable now */
		clrsetbits_le32(MSCC_CPU_SS_FORCE,
				MSCC_M_CPU_SS_FORCE_SS_FORCE,
				MSCC_F_CPU_SS_FORCE_SS_FORCE(mask));

	} else {
		/* CS value */
		clrsetbits_le32(MSCC_CPU_SS_FORCE,
				MSCC_M_CPU_SS_FORCE_SS_FORCE,
				MSCC_F_CPU_SS_FORCE_SS_FORCE(0xffff));
		udelay(1);
		/* CS override drive disable */
		clrsetbits_le32(MSCC_CPU_SS_FORCE_ENA,
				MSCC_M_CPU_SS_FORCE_ENA_SS_FORCE_ENA,
				MSCC_F_CPU_SS_FORCE_ENA_SS_FORCE_ENA(false));
		udelay(1);
		/* SI owner = SIBM */
		clrsetbits_le32(MSCC_CPU_GENERAL_CTRL,
				SPI_OWNER_MASK,
				MSCC_F_CPU_GENERAL_CTRL_IF_SI2_OWNER(2) |
				MSCC_F_CPU_GENERAL_CTRL_IF_SI_OWNER(1));
	}
}
