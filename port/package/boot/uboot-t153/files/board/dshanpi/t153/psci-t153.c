// SPDX-License-Identifier: GPL-2.0+
/*
 * T153 cold CPU_ON for the opt-in source-1g-smp U-Boot target.
 * CPU_OFF/suspend are deliberately not advertised; hardware qualification
 * is recorded separately from compilation.
 *
 * T153 register sequence derived from the GPL-2.0+ SyterKit SMP sample:
 * https://github.com/YuzukiHD/SyterKit/blob/
 * 7f539e17762639589897db835e58dd638b2bea80/
 * boards/tlt153-minievm/app_sram/smp/main.c
 *
 * No register sequences from A31/H3/R528 are used here. CPU_OFF and suspend
 * remain unsupported: neither the sample nor the available T153 SDK sources
 * establish their complete hardware sequence.
 */
#include <config.h>
#include <asm/io.h>
#include <asm/psci.h>
#include <asm/secure.h>
#include <asm/system.h>
#include <linux/bitops.h>

#if !defined(CONFIG_ARMV7_PSCI_0_1)
#error "This unqualified CPU_ON-only candidate requires PSCI 0.1"
#endif
#if CONFIG_ARMV7_PSCI_NR_CPUS != 4
#error "T153 cold CPU_ON candidate requires four PSCI contexts/stacks"
#endif

#define T153_CPU_SOFT_ENTRY(cpu) (0x07050070U + (cpu) * 4U)
#define T153_CPU_PWR_STATUS(cpu) (0x07051050U + (cpu) * 4U)
#define T153_CPU_CFG(cpu)        (0x07051070U + (cpu) * 4U)
#define T153_CPU_HOTPLUG(cpu)    (0x07051080U + (cpu) * 4U)
#define T153_CPUIDLE_ENABLE     0x07051100U
#define T153_POWER_SW_DELAY     0x07051140U
#define T153_F1F2_CONFIG_DELAY  0x07051144U
#define T153_POWER_OFF          0xffU
#define T153_WAKEUP_MASK        BIT(0)
#define T153_CPU_IRQ_MASK       BIT(4)
#define T153_HOTPLUG_REQ        BIT(16)
#define T153_BOOT_POLL_COUNT    0x100000U

enum t153_cpu_state {
	T153_CPU_OFF = 0,
	T153_CPU_STARTING,
	T153_CPU_RELEASED,
	T153_CPU_FAILED,
};

/* Secure monitor data must remain resident after ordinary U-Boot is gone. */
/* Only CPU0 writes state; each secondary writes its own ready flag. */
static volatile u32 t153_cpu_state[4] __secure_data = {
	T153_CPU_RELEASED, T153_CPU_OFF, T153_CPU_OFF, T153_CPU_OFF,
};
static volatile u32 t153_cpu_ready[4] __secure_data = { 0, 0, 0, 0 };

void t153_psci_cpu_entry(void);

/* Bounded delay matching the sample's iteration-based delay, not microseconds. */
static void __secure t153_delay(unsigned int count)
{
	while (count--)
		asm volatile("nop" : : : "memory");
}

/*
 * Generic armv7_init_nonsec() calls this exactly once on the boot CPU, before
 * secure relocation. Do not put this global sequence in psci_arch_init():
 * that hook runs again for each secondary's first non-secure entry.
 *
 * This operation enables the T153 CPUIDLE sequencer. Normal CPU0 WFI wakeup
 * must therefore be part of qualification, not just secondary arrival.
 */
void psci_board_init(void)
{
	unsigned int cpu;

	for (cpu = 0; cpu < 4; cpu++)
		setbits_le32(T153_CPU_CFG(cpu), T153_CPU_IRQ_MASK);
	writel(24, T153_F1F2_CONFIG_DELAY);
	writel(24, T153_POWER_SW_DELAY);
	writel(0x16aa0000, T153_CPUIDLE_ENABLE);
	writel(0xaa160001, T153_CPUIDLE_ENABLE);
	dsb();
}

/*
 * The initial qualification supports CPU0 serially starting CPU1..CPU3.
 * It does not advertise a general hotplug service or concurrent CPU_ON.
 */
int __secure psci_cpu_on(u32 function_id, u32 mpidr, u32 pc, u32 context_id)
{
	unsigned int cpu = mpidr;
	unsigned int timeout;
	u32 state;

	(void)function_id;
	if ((mpidr & ~3U) || !cpu || !pc || (pc & 3U))
		return ARM_PSCI_RET_INVAL;
	if (psci_get_cpu_id() != 0)
		return ARM_PSCI_RET_DENIED;

	state = t153_cpu_state[cpu];
	if (state == T153_CPU_RELEASED)
		return ARM_PSCI_RET_ALREADY_ON;
	if (state == T153_CPU_STARTING)
		return ARM_PSCI_RET_ON_PENDING;
	if (state != T153_CPU_OFF)
		return ARM_PSCI_RET_DENIED;

	/* Preserve target/context in the generic resident secure context table. */
	psci_save(cpu, pc, context_id);
	writel((u32)t153_psci_cpu_entry, T153_CPU_SOFT_ENTRY(cpu));
	t153_cpu_state[cpu] = T153_CPU_STARTING;
	dsb();
	setbits_le32(T153_CPU_HOTPLUG(cpu), T153_WAKEUP_MASK);

	/* Source sample's precondition: the target is genuinely powered off. */
	if ((readl(T153_CPU_PWR_STATUS(cpu)) & 0xffU) != T153_POWER_OFF) {
		clrbits_le32(T153_CPU_HOTPLUG(cpu), T153_WAKEUP_MASK);
		t153_cpu_state[cpu] = T153_CPU_FAILED;
		dsb();
		return ARM_PSCI_RET_DENIED;
	}

	t153_delay(1000);
	setbits_le32(T153_CPU_HOTPLUG(cpu), T153_HOTPLUG_REQ);
	dsb();

	for (timeout = 0; timeout < T153_BOOT_POLL_COUNT; timeout++) {
		if (!(readl(T153_CPU_HOTPLUG(cpu)) & T153_HOTPLUG_REQ) &&
		    t153_cpu_ready[cpu]) {
			/* Both hardware completion and execution of our stub observed. */
			t153_cpu_state[cpu] = T153_CPU_RELEASED;
			dsb();
			asm volatile("sev" : : : "memory");
			return ARM_PSCI_RET_SUCCESS;
		}
		t153_delay(100);
	}

	/* No undocumented cancel/reset/power-off writes on failure. */
	t153_cpu_state[cpu] = T153_CPU_FAILED;
	dsb();
	asm volatile("sev" : : : "memory");
	return ARM_PSCI_RET_INTERNAL_FAILURE;
}

/*
 * Generic psci_cpu_entry calls this after secure stack/per-core GIC setup,
 * but before it obtains the kernel PC/context and enters non-secure Linux.
 */
void __secure psci_arch_cpu_entry(void)
{
	u32 cpu = psci_get_cpu_id();

	if (cpu == 0 || cpu >= 4)
		goto parked;
	clrbits_le32(T153_CPU_HOTPLUG(cpu), T153_WAKEUP_MASK);
	if (t153_cpu_state[cpu] != T153_CPU_STARTING)
		goto parked;

	/* Never overwrite a concurrent CPU0 timeout/FAILED decision. */
	t153_cpu_ready[cpu] = 1;
	dsb();
	while (t153_cpu_state[cpu] == T153_CPU_STARTING)
		asm volatile("wfe" : : : "memory");
	if (t153_cpu_state[cpu] == T153_CPU_RELEASED) {
		dsb();
		return;
	}

parked:
	/* This is an error holding pen in reserved SRAM, NOT PSCI CPU_OFF. */
	for (;;)
		asm volatile("wfi" : : : "memory");
}

s32 __secure psci_cpu_off(void)
{
	return ARM_PSCI_RET_NI;
}

int __secure psci_cpu_suspend(u32 function_id, u32 state, u32 pc, u32 context)
{
	(void)function_id;
	(void)state;
	(void)pc;
	(void)context;
	return ARM_PSCI_RET_NI;
}
