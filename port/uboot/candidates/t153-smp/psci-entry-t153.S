/* SPDX-License-Identifier: GPL-2.0+ */
/* REVIEW CANDIDATE ONLY; not linked by the default T153 configuration. */
#include <linux/linkage.h>

	.pushsection ._secure.text, "ax"
	.arm
	.align 5
ENTRY(t153_psci_cpu_entry)
	/* The T153 Boot ROM is expected to enter secure ARM state after reset. */
	cpsid if
	mrc p15, 0, r0, c1, c0, 0
	/* Fail closed if unexpectedly handed a live MMU or data cache. */
	tst r0, #5
	bne 1f
	/* Match the T153 sample's low-vector state; generic PSCI sets MVBAR. */
	bic r0, r0, #(1 << 13)
	mcr p15, 0, r0, c1, c0, 0
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0
	mcr p15, 0, r0, c7, c5, 6
	dsb
	isb
	/* Reuse ACTLR.SMP, GIC, timer, secure stack and NS transition code. */
	b psci_cpu_entry
1:
	wfi
	b 1b
ENDPROC(t153_psci_cpu_entry)
	.popsection
