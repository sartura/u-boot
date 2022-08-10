// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2022 Sartura Ltd.
 */

#include <common.h>
#include <asm/io.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <clk.h>
#include <thermal.h>
#include <linux/bitfield.h>
#include <linux/iopoll.h>

#define TEMP_CTRL		0
#define TEMP_CFG		4
#define  TEMP_CFG_CYCLES	GENMASK(24, 15)
#define  TEMP_CFG_ENA		BIT(0)
#define TEMP_STAT		8
#define  TEMP_STAT_VALID	BIT(12)
#define  TEMP_STAT_TEMP		GENMASK(11, 0)

#define USEC_PER_SEC		1000000UL

#define SPARX5_TEMP_TIMEOUT	200000
#define SPARX5_TEMP_SLEEP	10

#define SPARX5_SYS_CLOCK_RATE	625000000

struct sparx5_temp_priv {
	void __iomem *base;
};

static void sparx5_temp_enable(struct udevice *dev)
{
	struct sparx5_temp_priv *priv = dev_get_priv(dev);

	u32 val = readl(priv->base + TEMP_CFG);
	u32 clk = SPARX5_SYS_CLOCK_RATE / USEC_PER_SEC;

	val &= ~TEMP_CFG_CYCLES;
	val |= FIELD_PREP(TEMP_CFG_CYCLES, clk);
	val |= TEMP_CFG_ENA;

	writel(val, priv->base + TEMP_CFG);
}

static int sparx5_temp_wait_busy(struct udevice *dev)
{
	struct sparx5_temp_priv *priv = dev_get_priv(dev);
	unsigned int valid;

	return readl_poll_sleep_timeout(priv->base + TEMP_STAT, valid,
					(valid & TEMP_STAT_VALID),
					SPARX5_TEMP_SLEEP,
					SPARX5_TEMP_TIMEOUT);
}

int sparx5_thermal_get_temp(struct udevice *dev, int *temp)
{
	struct sparx5_temp_priv *priv = dev_get_priv(dev);
	int value;
	u32 stat;

	if (sparx5_temp_wait_busy(dev))
		return -ETIMEDOUT;

	stat = readl_relaxed(priv->base + TEMP_STAT);
	value = stat & TEMP_STAT_TEMP;
	/*
	 * From register documentation:
	 * Temp(C) = TEMP_SENSOR_STAT.TEMP / 4096 * 352.2 - 109.4
	 */
	value = DIV_ROUND_CLOSEST(value * 3522, 4096) - 1094;
	*temp = DIV_ROUND_CLOSEST(value, 10);

	return 0;
}

static const struct dm_thermal_ops sparx5_thermal_ops = {
	.get_temp = sparx5_thermal_get_temp,
};

static int sparx5_temp_probe(struct udevice *dev)
{
	struct sparx5_temp_priv *priv = dev_get_priv(dev);

	priv->base = dev_remap_addr(dev);
	if (!priv->base) {
		dev_err(dev, "No base reg\n");
		return -ENOENT;
	}

	/* TODO:
	 * Getting clock by index currently crashes in pre-reloc,
	 * so after that is fixed actually get the clock and its
	 * rate during runtime.
	 */

	sparx5_temp_enable(dev);

	return 0;
}

static const struct udevice_id sparx5_temp_ids[] = {
	{ .compatible = "microchip,sparx5-temp" },
	{ }
};

U_BOOT_DRIVER(sparx5_temp) = {
	.name		= "sparx5-temp",
	.id		= UCLASS_THERMAL,
	.of_match	= sparx5_temp_ids,
	.ops		= &sparx5_thermal_ops,
	.probe		= sparx5_temp_probe,
	.priv_auto	= sizeof(struct sparx5_temp_priv),
	.flags  = DM_FLAG_PRE_RELOC,
};
