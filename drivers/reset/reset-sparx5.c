// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <dm.h>
#include <asm/io.h>
#include <reset.h>
#include <reset-uclass.h>
#include <linux/bitfield.h>
#include <linux/iopoll.h>

struct reset_props {
	u32 protect_reg;
	u32 protect_bit;
	u32 reset_reg;
	u32 reset_bit;
};

struct sparx5_reset_priv {
	void __iomem *gcb_base;
	void __iomem *cpu_base;
	const struct reset_props *props;
};

static int sparx5_reset_assert(struct reset_ctl *rst)
{
	struct sparx5_reset_priv *priv = dev_get_priv(rst->dev);
	unsigned int val;

	/* Make sure the core is PROTECTED from reset */
	setbits_le32(priv->cpu_base + priv->props->protect_reg, priv->props->protect_bit);

	/* Start soft reset */
	setbits_le32(priv->gcb_base + priv->props->reset_reg, priv->props->reset_bit);

	/* Wait for soft reset done */
	return readl_poll_sleep_timeout(priv->gcb_base + priv->props->reset_reg,
					val, (val & priv->props->reset_bit) == 0,
					1, 100);
}

static int sparx5_reset_deassert(struct reset_ctl *rst)
{
	/* Reset is self-clearing */
	return 0;
}

static const struct reset_ops sparx5_reset_ops = {
	.rst_assert = sparx5_reset_assert,
	.rst_deassert = sparx5_reset_deassert,
};

static int sparx5_reset_probe(struct udevice *dev)
{
	struct sparx5_reset_priv *priv = dev_get_priv(dev);

	priv->gcb_base = dev_remap_addr_name(dev, "gcb");
	if (!priv->gcb_base)
		return -ENODEV;

	priv->cpu_base = dev_remap_addr_name(dev, "cpu");
	if (!priv->cpu_base)
		return -ENODEV;

	priv->props = (void *)dev_get_driver_data(dev);
	if (!priv->props)
		return -EINVAL;

	return 0;
}

static const struct reset_props reset_props_sparx5 = {
	.protect_reg    = 0x84,
	.protect_bit    = BIT(10),
	.reset_reg      = 0x0,
	.reset_bit      = BIT(1),
};

static const struct reset_props reset_props_lan966x = {
	.protect_reg    = 0x88,
	.protect_bit    = BIT(5),
	.reset_reg      = 0x0,
	.reset_bit      = BIT(1),
};

static const struct udevice_id sparx5_reset_ids[] = {
	{
		.compatible = "microchip,sparx5-switch-reset",
		.data = (ulong)&reset_props_sparx5,
	}, {
		.compatible = "microchip,lan966x-switch-reset",
		.data = (ulong)&reset_props_lan966x,
	},
	{ }
};

U_BOOT_DRIVER(sparx5_reset) = {
	.name		= "sparx5-switch-reset",
	.id		= UCLASS_RESET,
	.of_match	= sparx5_reset_ids,
	.ops		= &sparx5_reset_ops,
	.probe		= sparx5_reset_probe,
	.priv_auto	= sizeof(struct sparx5_reset_priv),
};
