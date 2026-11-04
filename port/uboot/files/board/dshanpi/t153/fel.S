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
