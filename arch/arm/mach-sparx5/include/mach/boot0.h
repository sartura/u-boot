/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Special initialization for Microchip SparX-5
 * - Does DDR init
 */

.extern boot0

#ifdef CONFIG_ARMV8_MULTIENTRY
	branch_if_master x0, 2f
	/* Secondary: Already in RAM, relocated. Just goto reset. */
	b	reset
2:
#endif

	/* Establish stack to be able to call "C" */
	ldr	x0, =(BOOT0_SP_ADDR)
	bic	sp, x0, #0xf	/* 16-byte alignment for ABI compliance */

	/*
	 * Enable the "SRAM" / shared memory by putting
	 * the Cortex-M3 sub-CPU into reset.
	 * See register MSCC_CPU_SUBCPU_SYS_CFG (CPU:CPU_REGS:SUBCPU_SYS_CFG).
	 */
        dmb     sy
        mov     x0, #0x94
        movk    x0, #0x6, lsl #32	/* Reg addr: 0x600000094 */
        mov     w1, #0x2		/* Set BIT(1) to force reset*/
        str     w1, [x0]		/* Store/write to MSCC_CPU_SUBCPU_SYS_CFG */
        dmb     sy
        mov     w1, #0x3		/* Set BIT(1) to limit the reset to M3 core only*/
        str     w1, [x0]		/* Store/write to MSCC_CPU_SUBCPU_SYS_CFG */
        dmb     sy

	/* "C" early boot init */
	bl	boot0

	b	reset
