// SPDX-License-Identifier: GPL-2.0-or-later OR MIT
/*
 * Copyright (c) 2018 Microsemi Corporation
 * Copyright (c) 2022 Sartura Ltd.
 */

#include <common.h>
#include <dm.h>
#include <miiphy.h>
#include <phy.h>
#include <linux/bitfield.h>
#include <linux/iopoll.h>

#define MSCC_MIIM_REG_STATUS			0x0
#define		MSCC_MIIM_STATUS_STAT_PENDING	BIT(2)
#define		MSCC_MIIM_STATUS_STAT_BUSY	BIT(3)
#define MSCC_MIIM_REG_CMD			0x8
#define		MSCC_MIIM_CMD_OPR_WRITE		BIT(1)
#define		MSCC_MIIM_CMD_OPR_READ		BIT(2)
#define		MSCC_MIIM_CMD_WRDATA(x)		((x) << 4)
#define		MSCC_MIIM_CMD_REGAD(x)		((x) << 20)
#define		MSCC_MIIM_CMD_PHYAD(x)		((x) << 25)
#define		MSCC_MIIM_CMD_VLD		BIT(31)
#define MSCC_MIIM_REG_DATA			0xc
#define		MSCC_MIIM_DATA_ERROR		(BIT(16) | BIT(17))
#define		MSCC_MIIM_DATA			GENMASK(15, 0)

#define MSCC_MIIM_MDIO_TIMEOUT    10000
#define MSCC_MIIM_MDIO_SLEEP      10

struct mscc_miim_priv {
	phys_addr_t mdio_base;
};

static int mscc_miim_mdio_wait_busy(struct mscc_miim_priv *priv)
{
	unsigned int busy;

	return readl_poll_sleep_timeout(priv->mdio_base + MSCC_MIIM_REG_STATUS,
					busy, (busy & MSCC_MIIM_STATUS_STAT_BUSY) == 0,
					MSCC_MIIM_MDIO_SLEEP, MSCC_MIIM_MDIO_TIMEOUT);
}

int mscc_miim_mdio_read(struct udevice *dev, int addr, int devad, int reg)
{
	struct mscc_miim_priv *priv = dev_get_priv(dev);
	u32 val;

	if (mscc_miim_mdio_wait_busy(priv))
		return -ETIMEDOUT;

	writel(MSCC_MIIM_CMD_VLD | MSCC_MIIM_CMD_PHYAD(addr) |
	       MSCC_MIIM_CMD_REGAD(reg) | MSCC_MIIM_CMD_OPR_READ,
	       priv->mdio_base + MSCC_MIIM_REG_CMD);

	if (mscc_miim_mdio_wait_busy(priv))
		return -ETIMEDOUT;

	val = readl(priv->mdio_base + MSCC_MIIM_REG_DATA);
	if (val & MSCC_MIIM_DATA_ERROR)
		return -EIO;

	return FIELD_GET(MSCC_MIIM_DATA, val);
}

int mscc_miim_mdio_write(struct udevice *dev, int addr, int devad,
			 int reg, u16 val)
{
	struct mscc_miim_priv *priv = dev_get_priv(dev);

	if (mscc_miim_mdio_wait_busy(priv))
		return -ETIMEDOUT;

	writel(MSCC_MIIM_CMD_VLD | MSCC_MIIM_CMD_PHYAD(addr) |
	       MSCC_MIIM_CMD_REGAD(reg) | MSCC_MIIM_CMD_WRDATA(val) |
	       MSCC_MIIM_CMD_OPR_WRITE, priv->mdio_base + MSCC_MIIM_REG_CMD);

	return 0;
}

static const struct mdio_ops mscc_miim_ops = {
	.read = mscc_miim_mdio_read,
	.write = mscc_miim_mdio_write,
};

static int mscc_miim_bind(struct udevice *dev)
{
	if (ofnode_valid(dev_ofnode(dev)))
		device_set_name(dev, ofnode_get_name(dev_ofnode(dev)));

	return 0;
}

static int mscc_miim_probe(struct udevice *dev)
{
	struct mscc_miim_priv *priv = dev_get_priv(dev);

	priv->mdio_base = dev_read_addr(dev);
	if (priv->mdio_base == FDT_ADDR_T_NONE)
		return -EINVAL;

	return 0;
}

static const struct udevice_id mscc_miim_ids[] = {
	{ .compatible = "mscc,ocelot-miim", },
	{ }
};

U_BOOT_DRIVER(mscc_miim) = {
	.name           = "mscc-miim",
	.id             = UCLASS_MDIO,
	.of_match       = mscc_miim_ids,
	.bind           = mscc_miim_bind,
	.probe          = mscc_miim_probe,
	.ops            = &mscc_miim_ops,
	.priv_auto	  = sizeof(struct mscc_miim_priv),
};
