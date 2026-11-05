// SPDX-License-Identifier: GPL-2.0+
/* Final board fixup runs after arch_fixup_fdt() and its generic fdt_psci(). */
#ifdef T153_SMP_DT_HOST_TEST
#include <libfdt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define CONFIG_ARMV7_SECURE_BASE 0x50000
#define CONFIG_ARMV7_SECURE_MAX_SIZE 0x8000
#define ARM_PSCI_FN_CPU_ON 0x95c1ba60
#else
#include <config.h>
#include <linux/libfdt.h>
#include <asm/psci.h>
#include <stdio.h>
#include <string.h>
#endif

int t153_smp_fixup_dt(void *fdt)
{
	int node, cpus, ret, length, i;
	unsigned int seen = 0;
	const fdt32_t *reg;
	const char *type;
	uint64_t start, size;

	/* Replace the complete node: appending arm,psci would accidentally
	 * leave the input DTS's arm,psci-1.0 first in the compatible list.
	 */
	node = fdt_path_offset(fdt, "/psci");
	if (node >= 0) {
		ret = fdt_del_node(fdt, node);
		if (ret)
			return ret;
	}
	node = fdt_add_subnode(fdt, 0, "psci");
	if (node < 0)
		return node;
	ret = fdt_setprop_string(fdt, node, "compatible", "arm,psci");
	if (ret)
		return ret;
	ret = fdt_setprop_string(fdt, node, "method", "smc");
	if (ret)
		return ret;
	ret = fdt_setprop_u32(fdt, node, "cpu_on", ARM_PSCI_FN_CPU_ON);
	if (ret)
		return ret;
	ret = fdt_setprop_string(fdt, node, "status", "okay");
	if (ret)
		return ret;
	cpus = fdt_path_offset(fdt, "/cpus");
	if (cpus < 0)
		return cpus;
	fdt_for_each_subnode(node, fdt, cpus) {
		type = fdt_getprop(fdt, node, "device_type", &length);
		if (!type || length != 4 || memcmp(type, "cpu", 4))
			continue;
		reg = fdt_getprop(fdt, node, "reg", &length);
		if (!reg || length != 4 || fdt32_to_cpu(*reg) > 3)
			return -FDT_ERR_BADVALUE;
		i = fdt32_to_cpu(*reg);
		if (seen & (1U << i))
			return -FDT_ERR_BADVALUE;
		seen |= 1U << i;
		ret = fdt_setprop_string(fdt, node, "status", "okay");
		if (ret)
			return ret;
		ret = fdt_setprop_string(fdt, node, "enable-method", "psci");
		if (ret)
			return ret;
	}
	if (seen != 15)
		return -FDT_ERR_BADVALUE;

	/* SRAM is outside the Linux DRAM bank. Reserve it explicitly as well
	 * so this resident firmware remains visible in the handoff contract.
	 * This does not claim hardware TrustZone memory isolation.
	 */
	for (i = 0; i < fdt_num_mem_rsv(fdt); i++) {
		ret = fdt_get_mem_rsv(fdt, i, &start, &size);
		if (ret)
			return ret;
		if (start == CONFIG_ARMV7_SECURE_BASE &&
		    size == CONFIG_ARMV7_SECURE_MAX_SIZE)
			break;
	}
	if (i == fdt_num_mem_rsv(fdt)) {
		ret = fdt_add_mem_rsv(fdt, CONFIG_ARMV7_SECURE_BASE,
				      CONFIG_ARMV7_SECURE_MAX_SIZE);
		if (ret)
			return ret;
	}
	puts("Source boot: 4 Cortex-A7 CPUs; PSCI 0.1 CPU_ON; SRAM 0x50000..0x57fff.\n");
	return 0;
}
