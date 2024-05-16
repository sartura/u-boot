// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2024 Sartura Ltd.
 *
 * Author: Robert Marko <robert.marko@sartura.hr>
 *
 * Qualcomm QCA8072 and QCA8075 PHY driver
 */

#include <dm/device_compat.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <phy.h>

#define PHY_ID_QCA8072			0x004dd0b2
#define PHY_ID_QCA8075			0x004dd0b1
#define PHY_ID_QCA807X_PSGMII		0x06820805
#define PHY_ID_QCA807X_MASK		GENMASK(31, 0)

#define QCA807X_CHIP_CONFIGURATION 				0x1f
#define QCA807X_MMD7_1000BASE_T_POWER_SAVE_PER_CABLE_LENGTH	0x801a
#define QCA807X_CONTROL_DAC_MASK				GENMASK(2, 0)
/* List of tweaks enabled by this bit:
 * - With both FULL amplitude and FULL bias current: bias current
 *   is set to half.
 * - With only DSP amplitude: bias current is set to half and
 *   is set to 1/4 with cable < 10m.
 * - With DSP bias current (included both DSP amplitude and
 *   DSP bias current): bias current is half the detected current
 *   with cable < 10m.
 */
#define QCA807X_CONTROL_DAC_BIAS_CURRENT_TWEAK			BIT(2)
#define QCA807X_CONTROL_DAC_DSP_BIAS_CURRENT			BIT(1)
#define QCA807X_CONTROL_DAC_DSP_AMPLITUDE			BIT(0)

#define QCA807X_MMD7_LED_100N_1 	0x8074
#define QCA807X_MMD7_LED_100N_2 	0x8075
#define QCA807X_MMD7_LED_1000N_1 	0x8076
#define QCA807X_MMD7_LED_1000N_2 	0x8077
#define QCA807X_LED_TXACT_BLK_EN_2 	BIT(10)
#define QCA807X_LED_RXACT_BLK_EN_2 	BIT(9)
#define QCA807X_LED_GT_ON_EN_2 		BIT(6)
#define QCA807X_LED_HT_ON_EN_2 		BIT(5)
#define QCA807X_LED_BT_ON_EN_2 		BIT(4)

/* PSGMII PHY specific */
#define PSGMII_QSGMII_DRIVE_CONTROL_1 		0xb
#define PSGMII_QSGMII_TX_DRIVER_MASK		GENMASK(7, 4)
#define PQSGMII_TX_DRIVER_140MV				0x0
#define PQSGMII_TX_DRIVER_160MV				0x1
#define PQSGMII_TX_DRIVER_180MV				0x2
#define PQSGMII_TX_DRIVER_200MV				0x3
#define PQSGMII_TX_DRIVER_220MV				0x4
#define PQSGMII_TX_DRIVER_240MV				0x5
#define PQSGMII_TX_DRIVER_260MV				0x6
#define PQSGMII_TX_DRIVER_280MV				0x7
#define PQSGMII_TX_DRIVER_300MV				0x8
#define PQSGMII_TX_DRIVER_320MV				0x9
#define PQSGMII_TX_DRIVER_400MV				0xa
#define PQSGMII_TX_DRIVER_500MV				0xb
#define PQSGMII_TX_DRIVER_600MV				0xc
#define PSGMII_MODE_CTRL 			0x6d
#define PSGMII_MODE_CTRL_AZ_WORKAROUND_MASK 	GENMASK(3, 0)
#define PSGMII_MMD3_SERDES_CONTROL		0x805a

static int qca807x_config(struct phy_device *phydev)
{
	int control_dac, ret = 0;
	bool dac_full_amplitude;
	bool dac_full_bias_current;
	bool dac_disable_bias_current_tweak;
	ofnode node;

	node = phy_get_ofnode(phydev);
	if (!ofnode_valid(node))
		return -EINVAL;

	dac_full_amplitude = ofnode_read_bool(node, "qcom,dac-full-amplitude");
	dac_full_bias_current = ofnode_read_bool(node, "qcom,dac-full-bias-current");
	dac_disable_bias_current_tweak = ofnode_read_bool(node, "qcom,dac-disable-bias-current-tweak");

	/* Check for Combo port */
	if(phy_read(phydev, MDIO_DEVAD_NONE, QCA807X_CHIP_CONFIGURATION)) {
		int psgmii_serdes;

		/* Prevent PSGMII going into hibernation via PSGMII self test */
		psgmii_serdes = phy_read_mmd(phydev, MDIO_MMD_PCS, PSGMII_MMD3_SERDES_CONTROL);
		psgmii_serdes &= ~BIT(1);
		ret = phy_write_mmd(phydev, MDIO_MMD_PCS, PSGMII_MMD3_SERDES_CONTROL, psgmii_serdes);
	}

	control_dac = phy_read_mmd(phydev, MDIO_MMD_AN, QCA807X_MMD7_1000BASE_T_POWER_SAVE_PER_CABLE_LENGTH);
	control_dac &= ~QCA807X_CONTROL_DAC_MASK;
	if (!dac_full_amplitude)
		control_dac |= QCA807X_CONTROL_DAC_DSP_AMPLITUDE;
	if (!dac_full_amplitude)
		control_dac |= QCA807X_CONTROL_DAC_DSP_BIAS_CURRENT;
	if (!dac_disable_bias_current_tweak)
		control_dac |= QCA807X_CONTROL_DAC_BIAS_CURRENT_TWEAK;
	ret = phy_write_mmd(phydev, MDIO_MMD_AN, QCA807X_MMD7_1000BASE_T_POWER_SAVE_PER_CABLE_LENGTH, control_dac);

	phydev->supported = phydev->drv->features;
	phydev->advertising = phydev->drv->features;

	genphy_config_aneg(phydev);
	genphy_restart_aneg(phydev);

	return ret;
}

static int qca807x_psgmii_config(struct phy_device *phydev)
{
	int psgmii_az, tx_amp, ret = 0;
	u32 tx_drive_strength;
	ofnode node;

	node = phy_get_ofnode(phydev);
	if (!ofnode_valid(node))
		return -EINVAL;

	/* Default to 600mw if not defined */
	if (ofnode_read_u32(node, "qcom,tx-drive-strength-milliwatt",
	                    &tx_drive_strength))
		tx_drive_strength = 600;

	switch (tx_drive_strength) {
	case 140:
		tx_drive_strength = PQSGMII_TX_DRIVER_140MV;
		break;
	case 160:
		tx_drive_strength = PQSGMII_TX_DRIVER_160MV;
		break;
	case 180:
		tx_drive_strength = PQSGMII_TX_DRIVER_180MV;
		break;
	case 200:
		tx_drive_strength = PQSGMII_TX_DRIVER_200MV;
		break;
	case 220:
		tx_drive_strength = PQSGMII_TX_DRIVER_220MV;
		break;
	case 240:
		tx_drive_strength = PQSGMII_TX_DRIVER_240MV;
		break;
	case 260:
		tx_drive_strength = PQSGMII_TX_DRIVER_260MV;
		break;
	case 280:
		tx_drive_strength = PQSGMII_TX_DRIVER_280MV;
		break;
	case 300:
		tx_drive_strength = PQSGMII_TX_DRIVER_300MV;
		break;
	case 320:
		tx_drive_strength = PQSGMII_TX_DRIVER_320MV;
		break;
	case 400:
		tx_drive_strength = PQSGMII_TX_DRIVER_400MV;
		break;
	case 500:
		tx_drive_strength = PQSGMII_TX_DRIVER_500MV;
		break;
	case 600:
		tx_drive_strength = PQSGMII_TX_DRIVER_600MV;
		break;
	default:
		return -EINVAL;
	}

	/* Workaround to enable AZ transmitting ability */
	psgmii_az = phy_read_mmd(phydev, MDIO_MMD_PMAPMD, PSGMII_MODE_CTRL);
	psgmii_az &= ~PSGMII_MODE_CTRL_AZ_WORKAROUND_MASK;
	psgmii_az |= FIELD_PREP(PSGMII_MODE_CTRL_AZ_WORKAROUND_MASK, 0xc);
	ret = phy_write_mmd(phydev, MDIO_MMD_PMAPMD, PSGMII_MODE_CTRL, psgmii_az);
	psgmii_az = phy_read_mmd(phydev, MDIO_MMD_PMAPMD, PSGMII_MODE_CTRL);


	tx_amp = phy_read(phydev, MDIO_DEVAD_NONE, PSGMII_QSGMII_DRIVE_CONTROL_1);
	tx_amp &= ~PSGMII_QSGMII_TX_DRIVER_MASK;
	tx_amp |= FIELD_PREP(PSGMII_QSGMII_TX_DRIVER_MASK, tx_drive_strength);
	ret = phy_write(phydev, MDIO_DEVAD_NONE, PSGMII_QSGMII_DRIVE_CONTROL_1, tx_amp);

	return ret;
}

U_BOOT_PHY_DRIVER(QCA8072_driver) =  {
	.name = "Qualcomm QCA8072",
	.uid = PHY_ID_QCA8072,
	.mask = PHY_ID_QCA807X_MASK,
	.features = PHY_GBIT_FEATURES,
	.config = qca807x_config,
	.startup = genphy_startup,
	.shutdown = genphy_shutdown,
};

U_BOOT_PHY_DRIVER(QCA8075_driver) =  {
	.name = "Qualcomm QCA8075",
	.uid = PHY_ID_QCA8075,
	.mask = PHY_ID_QCA807X_MASK,
	.features = PHY_GBIT_FEATURES,
	.config = qca807x_config,
	.startup = genphy_startup,
	.shutdown = genphy_shutdown,
};

U_BOOT_PHY_DRIVER(QCA807X_PSGMII_driver) =  {
	.name = "Qualcomm QCA807x PSGMII",
	.uid = PHY_ID_QCA807X_PSGMII,
	.mask = PHY_ID_QCA807X_MASK,
	.config = qca807x_psgmii_config,
};
