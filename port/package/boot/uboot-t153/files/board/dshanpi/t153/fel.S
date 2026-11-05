/* SPDX-License-Identifier: GPL-2.0+ */
/* Based on upstream ARMv7 sunxi/fel_utils.S, Copyright 2015 Google, Inc. */
#include <linux/linkage.h>
ENTRY(save_boot_params)
	ldr r0, =t153_fel_context
	str sp, [r0]
	str lr, [r0, #4]
	mrs lr, cpsr
	str lr, [r0, #8]
	mrc p15, 0, lr, c1, c0, 0
	str lr, [r0, #12]
	mrc p15, 0, lr, c12, c0, 0
	str lr, [r0, #16]
	b save_boot_params_ret
ENDPROC(save_boot_params)

ENTRY(t153_return_to_fel)
	ldr r0, =t153_fel_context
	ldr sp, [r0]
	ldr lr, [r0, #4]
	dsb sy
	ldr r1, [r0, #16]
	mcr p15, 0, r1, c12, c0, 0
	isb sy
	ldr r1, [r0, #12]
	mcr p15, 0, r1, c1, c0, 0
	isb sy
	ldr r1, [r0, #8]
	msr cpsr, r1
	bx lr
ENDPROC(t153_return_to_fel)

/*
 * Fresh BootROM FEL entry, only called before DDR after a cold-media EFEX
 * request. This is NOT the USB loader's saved-SP/LR return route above.
 * T153 vendor boot0 uses ARM physical 0x20 after disabling I/C/A/M and
 * waiting 100 us / 10 ms using the 24 MHz architectural counter.
 */
ENTRY(t153_enter_fel_fresh)
	cpsid if
	mrc p15, 0, r4, c1, c0, 0
	mrrc p15, 0, r0, r1, c14
	ldr r5, =2400
1:
	mrrc p15, 0, r2, r3, c14
	sub r2, r2, r0
	cmp r2, r5
	blo 1b
	bic r4, r4, #0x1000
	bic r4, r4, #7
	mcr p15, 0, r4, c1, c0, 0
	isb sy
	mov r2, #0
	mcr p15, 0, r2, c7, c5, 0
	mcr p15, 0, r2, c7, c5, 6
	dsb sy
	isb sy
	mrrc p15, 0, r0, r1, c14
	ldr r5, =240000
2:
	mrrc p15, 0, r2, r3, c14
	sub r2, r2, r0
	cmp r2, r5
	blo 2b
	/* Unlike vendor boot0, upstream start.S has changed VBAR to _start. */
	ldr r0, =t153_fel_context
	ldr r1, [r0, #16]
	dsb sy
	mcr p15, 0, r1, c12, c0, 0
	isb sy
	mov r0, #0x20
	mov r2, #0
	mcr p15, 0, r2, c7, c5, 6
	bx r0
ENDPROC(t153_enter_fel_fresh)
