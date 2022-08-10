// SPDX-License-Identifier: (GPL-2.0 OR MIT)

#include <asm/gpio.h>
#include <asm/system.h>
#include <common.h>
#include <config.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <dm/pinctrl.h>
#include <dm/root.h>
#include <errno.h>
#include <fdtdec.h>
#include <linux/io.h>
#include "mscc-common.h"

enum {
	FUNC_CAN0_a,
	FUNC_CAN0_b,
	FUNC_CAN1,
	FUNC_CLKMON,
	FUNC_NONE,
	FUNC_FC0_a,
	FUNC_FC0_b,
	FUNC_FC0_c,
	FUNC_FC1_a,
	FUNC_FC1_b,
	FUNC_FC1_c,
	FUNC_FC2_a,
	FUNC_FC2_b,
	FUNC_FC3_a,
	FUNC_FC3_b,
	FUNC_FC3_c,
	FUNC_FC4_a,
	FUNC_FC4_b,
	FUNC_FC4_c,
	FUNC_FC_SHRD0,
	FUNC_FC_SHRD1,
	FUNC_FC_SHRD2,
	FUNC_FC_SHRD3,
	FUNC_FC_SHRD4,
	FUNC_FC_SHRD5,
	FUNC_FC_SHRD6,
	FUNC_FC_SHRD7,
	FUNC_FC_SHRD8,
	FUNC_FC_SHRD9,
	FUNC_FC_SHRD10,
	FUNC_FC_SHRD11,
	FUNC_FC_SHRD12,
	FUNC_FC_SHRD13,
	FUNC_FC_SHRD14,
	FUNC_FC_SHRD15,
	FUNC_FC_SHRD16,
	FUNC_FC_SHRD17,
	FUNC_FC_SHRD18,
	FUNC_FC_SHRD19,
	FUNC_FC_SHRD20,
	FUNC_GPIO,
	FUNC_IB_TRG_a,
	FUNC_IB_TRG_b,
	FUNC_IB_TRG_c,
	FUNC_IRQ0,
	FUNC_IRQ_IN_a,
	FUNC_IRQ_IN_b,
	FUNC_IRQ_IN_c,
	FUNC_IRQ0_IN,
	FUNC_IRQ_OUT_a,
	FUNC_IRQ_OUT_b,
	FUNC_IRQ_OUT_c,
	FUNC_IRQ0_OUT,
	FUNC_IRQ1,
	FUNC_IRQ1_IN,
	FUNC_IRQ1_OUT,
	FUNC_EXT_IRQ,
	FUNC_MIIM,
	FUNC_MIIM_a,
	FUNC_MIIM_b,
	FUNC_MIIM_c,
	FUNC_MIIM_Sa,
	FUNC_MIIM_Sb,
	FUNC_OB_TRG,
	FUNC_OB_TRG_a,
	FUNC_OB_TRG_b,
	FUNC_PHY_LED,
	FUNC_PCI_WAKE,
	FUNC_MD,
	FUNC_PTP0,
	FUNC_PTP1,
	FUNC_PTP2,
	FUNC_PTP3,
	FUNC_PTPSYNC_0,
	FUNC_PTPSYNC_1,
	FUNC_PTPSYNC_2,
	FUNC_PTPSYNC_3,
	FUNC_PTPSYNC_4,
	FUNC_PTPSYNC_5,
	FUNC_PTPSYNC_6,
	FUNC_PTPSYNC_7,
	FUNC_PWM,
	FUNC_PWM_a,
	FUNC_PWM_b,
	FUNC_QSPI1,
	FUNC_QSPI2,
	FUNC_R,
	FUNC_RECO_a,
	FUNC_RECO_b,
	FUNC_RECO_CLK,
	FUNC_SD,
	FUNC_SFP,
	FUNC_SFP_SD,
	FUNC_SG0,
	FUNC_SG1,
	FUNC_SG2,
	FUNC_SGPIO_a,
	FUNC_SGPIO_b,
	FUNC_SI,
	FUNC_SI2,
	FUNC_TACHO,
	FUNC_TACHO_a,
	FUNC_TACHO_b,
	FUNC_TWI,
	FUNC_TWI2,
	FUNC_TWI3,
	FUNC_TWI_SCL_M,
	FUNC_TWI_SLC_GATE,
	FUNC_TWI_SLC_GATE_AD,
	FUNC_UART,
	FUNC_UART2,
	FUNC_UART3,
	FUNC_USB_H_a,
	FUNC_USB_H_b,
	FUNC_USB_H_c,
	FUNC_USB_S_a,
	FUNC_USB_S_b,
	FUNC_USB_S_c,
	FUNC_PLL_STAT,
	FUNC_EMMC,
	FUNC_EMMC_SD,
	FUNC_REF_CLK,
	FUNC_RCVRD_CLK,
	FUNC_MAX
};

char *sparx5_function_names[] = {
	[FUNC_CAN0_a]		= "can0_a",
	[FUNC_CAN0_b]		= "can0_b",
	[FUNC_CAN1]		= "can1",
	[FUNC_CLKMON]		= "clkmon",
	[FUNC_NONE]		= "none",
	[FUNC_FC0_a]		= "fc0_a",
	[FUNC_FC0_b]		= "fc0_b",
	[FUNC_FC0_c]		= "fc0_c",
	[FUNC_FC1_a]		= "fc1_a",
	[FUNC_FC1_b]		= "fc1_b",
	[FUNC_FC1_c]		= "fc1_c",
	[FUNC_FC2_a]		= "fc2_a",
	[FUNC_FC2_b]		= "fc2_b",
	[FUNC_FC3_a]		= "fc3_a",
	[FUNC_FC3_b]		= "fc3_b",
	[FUNC_FC3_c]		= "fc3_c",
	[FUNC_FC4_a]		= "fc4_a",
	[FUNC_FC4_b]		= "fc4_b",
	[FUNC_FC4_c]		= "fc4_c",
	[FUNC_FC_SHRD0]		= "fc_shrd0",
	[FUNC_FC_SHRD1]		= "fc_shrd1",
	[FUNC_FC_SHRD2]		= "fc_shrd2",
	[FUNC_FC_SHRD3]		= "fc_shrd3",
	[FUNC_FC_SHRD4]		= "fc_shrd4",
	[FUNC_FC_SHRD5]		= "fc_shrd5",
	[FUNC_FC_SHRD6]		= "fc_shrd6",
	[FUNC_FC_SHRD7]		= "fc_shrd7",
	[FUNC_FC_SHRD8]		= "fc_shrd8",
	[FUNC_FC_SHRD9]		= "fc_shrd9",
	[FUNC_FC_SHRD10]	= "fc_shrd10",
	[FUNC_FC_SHRD11]	= "fc_shrd11",
	[FUNC_FC_SHRD12]	= "fc_shrd12",
	[FUNC_FC_SHRD13]	= "fc_shrd13",
	[FUNC_FC_SHRD14]	= "fc_shrd14",
	[FUNC_FC_SHRD15]	= "fc_shrd15",
	[FUNC_FC_SHRD16]	= "fc_shrd16",
	[FUNC_FC_SHRD17]	= "fc_shrd17",
	[FUNC_FC_SHRD18]	= "fc_shrd18",
	[FUNC_FC_SHRD19]	= "fc_shrd19",
	[FUNC_FC_SHRD20]	= "fc_shrd20",
	[FUNC_GPIO]		= "gpio",
	[FUNC_IB_TRG_a]		= "ib_trig_a",
	[FUNC_IB_TRG_b]		= "ib_trig_b",
	[FUNC_IB_TRG_c]		= "ib_trig_c",
	[FUNC_IRQ0]		= "irq0",
	[FUNC_IRQ_IN_a]		= "irq_in_a",
	[FUNC_IRQ_IN_b]		= "irq_in_b",
	[FUNC_IRQ_IN_c]		= "irq_in_c",
	[FUNC_IRQ0_IN]		= "irq0_in",
	[FUNC_IRQ_OUT_a]	= "irq_out_a",
	[FUNC_IRQ_OUT_b]	= "irq_out_b",
	[FUNC_IRQ_OUT_c]	= "irq_out_c",
	[FUNC_IRQ0_OUT]		= "irq0_out",
	[FUNC_IRQ1]		= "irq1",
	[FUNC_IRQ1_IN]		= "irq1_in",
	[FUNC_IRQ1_OUT]		= "irq1_out",
	[FUNC_EXT_IRQ]		= "ext_irq",
	[FUNC_MIIM]		= "miim",
	[FUNC_MIIM_a]		= "miim_a",
	[FUNC_MIIM_b]		= "miim_b",
	[FUNC_MIIM_c]		= "miim_c",
	[FUNC_MIIM_Sa]		= "miim_slave_a",
	[FUNC_MIIM_Sb]		= "miim_slave_b",
	[FUNC_PHY_LED]		= "phy_led",
	[FUNC_PCI_WAKE]		= "pci_wake",
	[FUNC_MD]		= "md",
	[FUNC_OB_TRG]		= "ob_trig",
	[FUNC_OB_TRG_a]		= "ob_trig_a",
	[FUNC_OB_TRG_b]		= "ob_trig_b",
	[FUNC_PTP0]		= "ptp0",
	[FUNC_PTP1]		= "ptp1",
	[FUNC_PTP2]		= "ptp2",
	[FUNC_PTP3]		= "ptp3",
	[FUNC_PTPSYNC_0]	= "ptpsync_0",
	[FUNC_PTPSYNC_1]	= "ptpsync_1",
	[FUNC_PTPSYNC_2]	= "ptpsync_2",
	[FUNC_PTPSYNC_3]	= "ptpsync_3",
	[FUNC_PTPSYNC_4]	= "ptpsync_4",
	[FUNC_PTPSYNC_5]	= "ptpsync_5",
	[FUNC_PTPSYNC_6]	= "ptpsync_6",
	[FUNC_PTPSYNC_7]	= "ptpsync_7",
	[FUNC_PWM]		= "pwm",
	[FUNC_PWM_a]		= "pwm_a",
	[FUNC_PWM_b]		= "pwm_b",
	[FUNC_QSPI1]		= "qspi1",
	[FUNC_QSPI2]		= "qspi2",
	[FUNC_R]		= "reserved",
	[FUNC_RECO_a]		= "reco_a",
	[FUNC_RECO_b]		= "reco_b",
	[FUNC_RECO_CLK]		= "reco_clk",
	[FUNC_SD]		= "sd",
	[FUNC_SFP]		= "sfp",
	[FUNC_SFP_SD]		= "sfp_sd",
	[FUNC_SG0]		= "sg0",
	[FUNC_SG1]		= "sg1",
	[FUNC_SG2]		= "sg2",
	[FUNC_SGPIO_a]		= "sgpio_a",
	[FUNC_SGPIO_b]		= "sgpio_b",
	[FUNC_SI]		= "si",
	[FUNC_SI2]		= "si2",
	[FUNC_TACHO]		= "tacho",
	[FUNC_TACHO_a]		= "tacho_a",
	[FUNC_TACHO_b]		= "tacho_b",
	[FUNC_TWI]		= "twi",
	[FUNC_TWI2]		= "twi2",
	[FUNC_TWI3]		= "twi3",
	[FUNC_TWI_SCL_M]	= "twi_scl_m",
	[FUNC_TWI_SLC_GATE]	= "twi_slc_gate",
	[FUNC_TWI_SLC_GATE_AD]	= "twi_slc_gate_ad",
	[FUNC_USB_H_a]		= "usb_host_a",
	[FUNC_USB_H_b]		= "usb_host_b",
	[FUNC_USB_H_c]		= "usb_host_c",
	[FUNC_USB_S_a]		= "usb_slave_a",
	[FUNC_USB_S_b]		= "usb_slave_b",
	[FUNC_USB_S_c]		= "usb_slave_c",
	[FUNC_UART]		= "uart",
	[FUNC_UART2]		= "uart2",
	[FUNC_UART3]		= "uart3",
	[FUNC_PLL_STAT]		= "pll_stat",
	[FUNC_EMMC]		= "emmc",
	[FUNC_EMMC_SD]		= "emmc_sd",
	[FUNC_REF_CLK]		= "ref_clk",
	[FUNC_RCVRD_CLK]	= "rcvrd_clk",
};

MSCC_P(0,  SG0,       PLL_STAT,  NONE);
MSCC_P(1,  SG0,       NONE,      NONE);
MSCC_P(2,  SG0,       NONE,      NONE);
MSCC_P(3,  SG0,       NONE,      NONE);
MSCC_P(4,  SG1,       NONE,      NONE);
MSCC_P(5,  SG1,       NONE,      NONE);
MSCC_P(6,  IRQ0_IN,   IRQ0_OUT,  SFP);
MSCC_P(7,  IRQ1_IN,   IRQ1_OUT,  SFP);
MSCC_P(8,  PTP0,      NONE,      SFP);
MSCC_P(9,  PTP1,      SFP,       TWI_SCL_M);
MSCC_P(10, UART,      NONE,      NONE);
MSCC_P(11, UART,      NONE,      NONE);
MSCC_P(12, SG1,       NONE,      NONE);
MSCC_P(13, SG1,       NONE,      NONE);
MSCC_P(14, TWI,       TWI_SCL_M, NONE);
MSCC_P(15, TWI,       NONE,      NONE);
MSCC_P(16, SI,        TWI_SCL_M, SFP);
MSCC_P(17, SI,        TWI_SCL_M, SFP);
MSCC_P(18, SI,        TWI_SCL_M, SFP);
MSCC_P(19, PCI_WAKE,  TWI_SCL_M, SFP);
MSCC_P(20, IRQ0_OUT,  TWI_SCL_M, SFP);
MSCC_P(21, IRQ1_OUT,  TACHO,     SFP);
MSCC_P(22, TACHO,     IRQ0_OUT,  TWI_SCL_M);
MSCC_P(23, PWM,       UART3,     TWI_SCL_M);
MSCC_P(24, PTP2,      UART3,     TWI_SCL_M);
MSCC_P(25, PTP3,      SI,        TWI_SCL_M);
MSCC_P(26, UART2,     SI,        TWI_SCL_M);
MSCC_P(27, UART2,     SI,        TWI_SCL_M);
MSCC_P(28, TWI2,      SI,        SFP);
MSCC_P(29, TWI2,      SI,        SFP);
MSCC_P(30, SG2,       SI,        PWM);
MSCC_P(31, SG2,       SI,        TWI_SCL_M);
MSCC_P(32, SG2,       SI,        TWI_SCL_M);
MSCC_P(33, SG2,       SI,        SFP);
MSCC_P(34, NONE,      TWI_SCL_M, EMMC);
MSCC_P(35, SFP,       TWI_SCL_M, EMMC);
MSCC_P(36, SFP,       TWI_SCL_M, EMMC);
MSCC_P(37, SFP,       NONE,      EMMC);
MSCC_P(38, NONE,      TWI_SCL_M, EMMC);
MSCC_P(39, SI2,       TWI_SCL_M, EMMC);
MSCC_P(40, SI2,       TWI_SCL_M, EMMC);
MSCC_P(41, SI2,       TWI_SCL_M, EMMC);
MSCC_P(42, SI2,       TWI_SCL_M, EMMC);
MSCC_P(43, SI2,       TWI_SCL_M, EMMC);
MSCC_P(44, SI,        SFP,       EMMC);
MSCC_P(45, SI,        SFP,       EMMC);
MSCC_P(46, NONE,      SFP,       EMMC);
MSCC_P(47, NONE,      SFP,       EMMC);
MSCC_P(48, TWI3,      SI,        SFP);
MSCC_P(49, TWI3,      NONE,      SFP);
MSCC_P(50, SFP,       NONE,      TWI_SCL_M);
MSCC_P(51, SFP,       SI,        TWI_SCL_M);
MSCC_P(52, SFP,       MIIM,      TWI_SCL_M);
MSCC_P(53, SFP,       MIIM,      TWI_SCL_M);
MSCC_P(54, SFP,       PTP2,      TWI_SCL_M);
MSCC_P(55, SFP,       PTP3,      PCI_WAKE);
MSCC_P(56, MIIM,      SFP,       TWI_SCL_M);
MSCC_P(57, MIIM,      SFP,       TWI_SCL_M);
MSCC_P(58, MIIM,      SFP,       TWI_SCL_M);
MSCC_P(59, MIIM,      SFP,       NONE);
MSCC_P(60, RECO_CLK,  NONE,      NONE);
MSCC_P(61, RECO_CLK,  NONE,      NONE);
MSCC_P(62, RECO_CLK,  PLL_STAT,  NONE);
MSCC_P(63, RECO_CLK,  NONE,      NONE);

#define SPARX5_PIN(n) {					\
	.name = "GPIO_"#n,					\
	.drv_data = &mscc_pin_##n				\
}

const struct mscc_pin_data sparx5_pins[] = {
	SPARX5_PIN(0),
	SPARX5_PIN(1),
	SPARX5_PIN(2),
	SPARX5_PIN(3),
	SPARX5_PIN(4),
	SPARX5_PIN(5),
	SPARX5_PIN(6),
	SPARX5_PIN(7),
	SPARX5_PIN(8),
	SPARX5_PIN(9),
	SPARX5_PIN(10),
	SPARX5_PIN(11),
	SPARX5_PIN(12),
	SPARX5_PIN(13),
	SPARX5_PIN(14),
	SPARX5_PIN(15),
	SPARX5_PIN(16),
	SPARX5_PIN(17),
	SPARX5_PIN(18),
	SPARX5_PIN(19),
	SPARX5_PIN(20),
	SPARX5_PIN(21),
	SPARX5_PIN(22),
	SPARX5_PIN(23),
	SPARX5_PIN(24),
	SPARX5_PIN(25),
	SPARX5_PIN(26),
	SPARX5_PIN(27),
	SPARX5_PIN(28),
	SPARX5_PIN(29),
	SPARX5_PIN(30),
	SPARX5_PIN(31),
	SPARX5_PIN(32),
	SPARX5_PIN(33),
	SPARX5_PIN(34),
	SPARX5_PIN(35),
	SPARX5_PIN(36),
	SPARX5_PIN(37),
	SPARX5_PIN(38),
	SPARX5_PIN(39),
	SPARX5_PIN(40),
	SPARX5_PIN(41),
	SPARX5_PIN(42),
	SPARX5_PIN(43),
	SPARX5_PIN(44),
	SPARX5_PIN(45),
	SPARX5_PIN(46),
	SPARX5_PIN(47),
	SPARX5_PIN(48),
	SPARX5_PIN(49),
	SPARX5_PIN(50),
	SPARX5_PIN(51),
	SPARX5_PIN(52),
	SPARX5_PIN(53),
	SPARX5_PIN(54),
	SPARX5_PIN(55),
	SPARX5_PIN(56),
	SPARX5_PIN(57),
	SPARX5_PIN(58),
	SPARX5_PIN(59),
	SPARX5_PIN(60),
	SPARX5_PIN(61),
	SPARX5_PIN(62),
	SPARX5_PIN(63),
};

static const unsigned long sparx5_gpios[] = {
	[MSCC_GPIO_OUT_SET] = 0x00,
	[MSCC_GPIO_OUT_CLR] = 0x08,
	[MSCC_GPIO_OUT] = 0x10,
	[MSCC_GPIO_IN] = 0x18,
	[MSCC_GPIO_OE] = 0x20,
	[MSCC_GPIO_INTR] = 0x28,
	[MSCC_GPIO_INTR_ENA] = 0x30,
	[MSCC_GPIO_INTR_IDENT] = 0x38,
	[MSCC_GPIO_ALT0] = 0x40,
	[MSCC_GPIO_ALT1] = 0x48,
};

static int sparx5_gpio_probe(struct udevice *dev)
{
	struct gpio_dev_priv *uc_priv;

	uc_priv = dev_get_uclass_priv(dev);
	uc_priv->bank_name = "sparx5-gpio";
	uc_priv->gpio_count = ARRAY_SIZE(sparx5_pins);

	return 0;
}

static struct driver sparx5_gpio_driver = {
	.name	= "sparx5-gpio",
	.id	= UCLASS_GPIO,
	.probe	= sparx5_gpio_probe,
	.ops	= &mscc_gpio_ops,
};

int sparx5_pinctrl_probe(struct udevice *dev)
{
	int ret;

	ret = mscc_pinctrl_probe(dev, FUNC_MAX, sparx5_pins,
				 ARRAY_SIZE(sparx5_pins),
				 sparx5_function_names,
				 sparx5_gpios);
	if (ret)
		return ret;

	ret = device_bind(dev, &sparx5_gpio_driver, "sparx5-gpio", NULL,
			  dev_ofnode(dev), NULL);

	return ret;
}

static const struct udevice_id sparx5_pinctrl_of_match[] = {
	{.compatible = "microchip,sparx5-pinctrl"},
	{},
};

U_BOOT_DRIVER(sparx5_pinctrl) = {
	.name = "sparx5-pinctrl",
	.id = UCLASS_PINCTRL,
	.of_match = of_match_ptr(sparx5_pinctrl_of_match),
	.probe = sparx5_pinctrl_probe,
	.priv_auto = sizeof(struct mscc_pinctrl),
	.ops = &mscc_pinctrl_ops,
};
