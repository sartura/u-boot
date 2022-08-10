/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */

#ifndef _CONFIG_SPARX5_H
#define _CONFIG_SPARX5_H

#include <linux/sizes.h>

/* 32-bit register interface for NS16550 */
#define CONFIG_SYS_NS16550_MEM32

/* Memory config */
#define PHYS_SPI			0x000000000
#define PHYS_NAND			0x400000000
#define PHYS_DEVICE_REG			0x600000000
#define PHYS_DDR			0x700000000
#define VIRT_DDR			0x700000000 /* 1:1 */
#define PHYS_SDRAM_1_SIZE		CONFIG_SYS_SDRAM_SIZE

#define PHYS_SDRAM_1                    PHYS_DDR
#define VIRT_SDRAM_1                    VIRT_DDR
#define CONFIG_SYS_SDRAM_BASE		VIRT_SDRAM_1

/* BOOT0 config */
#define PHYS_SRAM_ADDR			0x630000000
#define PHYS_SRAM_SIZE			SZ_64K
#define BOOT0_SP_ADDR			(PHYS_SRAM_ADDR + PHYS_SRAM_SIZE)

/* GICv3 */
#define GICD_BASE                       (0x600300000)
#define GICR_BASE                       (0x600340000)

#endif /* _CONFIG_SPARX5_H */
