// SPDX-License-Identifier: GPL-2.0-or-later OR MIT
/*
 * Copyright (c) 2018 Microsemi Corporation
 * Copyright (c) 2022 Sartura Ltd.
 */

#include <common.h>
#include <asm/io.h>
#include <asm/armv8/mmu.h>
#include <asm/system.h>
#include <asm/sections.h>
#include <debug_uart.h>
#include <linux/bitfield.h>

#include "mscc_fa_regs_devcpu_gcb.h"
#include "mscc_fa_regs_cpu.h"

#include "fa_helpers.h"

static struct mm_region fa_mem_map[] = {
	{
		.virt = VIRT_SDRAM_1,
		.phys = PHYS_SDRAM_1,
		.size = PHYS_SDRAM_1_SIZE,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = PHYS_SPI,
		.phys = PHYS_SPI,
		.size = 1UL * SZ_1G,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 (UL(3) << 6) | /* Read-only */
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = PHYS_DEVICE_REG,
		.phys = PHYS_DEVICE_REG,
		.size = SZ_1G,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = fa_mem_map;

/*
 * This does not reliably work, GPIO reset should be used.
 * However, keep this if GPIO reset is not present.
 */
__weak void reset_cpu(void)
{
	/* Make sure the core is UN-protected from reset */
	clrbits_le32(MSCC_CPU_RESET_PROT_STAT,
		     MSCC_M_CPU_RESET_PROT_STAT_SYS_RST_PROT_VCORE);

	writel(MSCC_M_DEVCPU_GCB_SOFT_RST_SOFT_CHIP_RST, MSCC_DEVCPU_GCB_SOFT_RST);
}

int print_cpuinfo(void)
{
	u32 chip_id = readl(MSCC_DEVCPU_GCB_CHIP_ID);

	printf("CPU:   Microchip SparX-5 VSC%lx rev %lu",
	       FIELD_GET(MSCC_M_DEVCPU_GCB_CHIP_ID_PART_ID, chip_id),
	       FIELD_GET(MSCC_M_DEVCPU_GCB_CHIP_ID_REV_ID, chip_id));

	return 0;
}

int arch_misc_init(void)
{
	u64 r_start = (u64) _start;

	writel((u32) ((r_start >> 0) >> 2), MSCC_CPU_CPU1_RVBAR_LSB);
	writel((u32) ((r_start >> 32) >> 2), MSCC_CPU_CPU1_RVBAR_MSB);

	clrbits_le32(MSCC_CPU_RESET, MSCC_M_CPU_RESET_CPU_CORE_1_COLD_RST);

	return 0;
}

#define EARLY_PGTABLE_SIZE 0x5000
static inline void early_mmu_setup(void)
{
	unsigned int el = current_el();

	gd->arch.tlb_addr = PHYS_SRAM_ADDR;
	gd->arch.tlb_fillptr = gd->arch.tlb_addr;
	gd->arch.tlb_size = EARLY_PGTABLE_SIZE;

	/* Create early page tables */
	setup_pgtables();

	/* point TTBR to the new table */
	set_ttbr_tcr_mair(el, gd->arch.tlb_addr,
			  get_tcr(NULL, NULL) &
			  ~(TCR_ORGN_MASK | TCR_IRGN_MASK),
			  MEMORY_ATTRIBUTES);

	set_sctlr(get_sctlr() | CR_M);
}

int arch_cpu_init(void)
{
	/*
	 * This function is called before U-Boot relocates itself to speed up
	 * on system running. It is not necessary to run if performance is not
	 * critical. Skip if MMU is already enabled by SPL or other means.
	 */
	if (get_sctlr() & CR_M)
		return 0;

	__asm_invalidate_dcache_all();
	__asm_invalidate_tlb_all();
	early_mmu_setup();
	set_sctlr(get_sctlr() | CR_C);

	/* Mysterious CTX init...  */
	writel(0, 0x600109008);	/* Low */
	writel(0, 0x60010900C);	/* Hi */
	writel(1, 0x600109000);	/* Enable */

	return 0;
}

/*
 * The final tables are identical to early tables, but the tables are
 * in DDR memory instead of device SRAM (which is supposed to be used
 * by the PoE controller).
 */
static inline void final_mmu_setup(void)
{
	u64 tlb_addr_save;
	unsigned int el = current_el();

	/* Use allocated (board_f.c) memory for TLB */
	tlb_addr_save = gd->arch.tlb_addr;

	/* Reset the fill ptr */
	gd->arch.tlb_fillptr = gd->arch.tlb_addr;

	/* Create normal system page tables */
	setup_pgtables();

	/* Create emergency page tables */
	gd->arch.tlb_emerg = gd->arch.tlb_addr = gd->arch.tlb_fillptr;
	setup_pgtables();

	/* Restore normal TLB addr */
	gd->arch.tlb_addr = tlb_addr_save;

	/* Disable cache and MMU */
	dcache_disable();	/* TLBs are invalidated */
	invalidate_icache_all();

	/* point TTBR to the new table */
	set_ttbr_tcr_mair(el, gd->arch.tlb_addr, get_tcr(NULL, NULL),
			  MEMORY_ATTRIBUTES);

	set_sctlr(get_sctlr() | CR_M);
}

void enable_caches(void)
{
	final_mmu_setup();
	__asm_invalidate_tlb_all();
	icache_enable();
	dcache_enable();
}

void boot0(void)
{
	/* Speed up Flash reads */
	clrsetbits_le32(MSCC_CPU_SPI_MST_CFG,
			MSCC_M_CPU_SPI_MST_CFG_CLK_DIV |
			MSCC_M_CPU_SPI_MST_CFG_FAST_READ_ENA,
			MSCC_F_CPU_SPI_MST_CFG_CLK_DIV(28) | /* 250Mhz/X */
			MSCC_F_CPU_SPI_MST_CFG_FAST_READ_ENA(1));

	/* Speed up instructions */
	icache_enable();

	debug_uart_init();

	ddr_setup();

	/* Release GIC reset */
	clrbits_le32(MSCC_CPU_RESET, MSCC_M_CPU_RESET_GIC_RST);
}
