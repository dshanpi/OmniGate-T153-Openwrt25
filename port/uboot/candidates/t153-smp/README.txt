T153 cold-SMP review candidate -- NOT ENABLED / NOT LINKED / NOT HARDWARE-TESTED

This directory is outside port/uboot/files and patches. Existing apply/build
scripts do not install it. CPU0/384MiB production defaults are unchanged.

Evidence and license
- SyterKit fixed source revision: 7f539e17762639589897db835e58dd638b2bea80.
  boards/tlt153-minievm/app_sram/smp/main.c and smp.S are GPL-2.0+.
  https://github.com/YuzukiHD/SyterKit/tree/7f539e17762639589897db835e58dd638b2bea80/boards/tlt153-minievm/app_sram/smp
- Feature introduction: fbd8bb0a9192ec7f2062265fbd5a963e6ef6bfa0 (2026-09-02).
  Its commit message says "add smp sample"; no hardware transcript was found.
- T153 SDK independently agrees on CPUIDLE global/cluster bases, CPU0 config,
  hotplug offsets, and bit0 -> bit16 -> wait clear -> clear bit0 wakeup.
  SDK paths relative to /home/ubuntu/T153MX/T153_Tina_V1.0:
  brandy/brandy-2.0/u-boot-bsp/arch/arm/include/asm/arch/plat-sun8iw22p1/cpu_autogen.h
  rtos/lichee/baremetal/projects/t153_e907/sram_lp/src/standby/platform_config.h
  rtos/lichee/rtos-components/aw/pm/plat_sun8iw22p1/e907/pm_cpux_suspend.c
- SOFT_ENTRY offset/per-core stride, PWRS_STAT, enable keys and delay=24 are
  presently supported by the T153 SyterKit sample only, not a located TRM.
- No A31/H3/R528 reset, clamp, power-off or L1-invalidate register is used.

CPU_OFF finding
No complete T153 per-core power-off source sequence was found. The SDK E907
pm_cpux_suspend code only waits for the cluster to have powered off; CPU-side
suspend is delegated to the binary PSCI implementation. This candidate returns
NOT_IMPLEMENTED for CPU_OFF and CPU_SUSPEND. Its error WFI loop is not power-off.
Do not advertise hotplug or use multi-core kexec with this first candidate.
Linux arch/arm/kernel/reboot.c explicitly requires all secondary CPUs to be
fully shut down before kexec; a RAM/SRAM holding pen does not satisfy that.

Integration to prepare in a SEPARATE opt-in configuration
1. Add these two objects to the T153 board Makefile only behind a new opt-in
   symbol, for example CONFIG_T153_PSCI_COLD_SMP. Do not link generic sunxi
   psci.c: its SoC power/register paths do not apply to T153.
2. Select CPU_V7_HAS_NONSEC and ARCH_SUPPORT_PSCI; enable ARMV7_NONSEC,
   ARMV7_PSCI, ARMV7_PSCI_0_1, ARMV7_PSCI_NR_CPUS=4. Keep unsupported PSCI
   versions off. Use non-secure SVC initially; virtualization is independent.
3. Reserve a confirmed on-chip SRAM window for the generic secure section,
   data and all four monitor stacks. Supply explicit ARMV7_SECURE_BASE and
   MAX_SIZE with link-time fit checks, outside live BROM/FEL scratch and any
   retained loader/firmware region. Do not accept the old SUN8I default address
   merely because its Kconfig label looks similar. Normal U-Boot DRAM code is
   reclaimed by Linux and cannot host this monitor indefinitely.
4. Use generic armv7_init_nonsec/psci_cpu_entry/nonsec_virt.S. Verify physical
   GIC base against the T153 map; set the generic GIC override only if required.
   CNTFRQ is 24MHz. The supplied T153 entry checks MMU/D-cache are off, masks
   IRQ/FIQ and then enters generic PSCI. It assumes the BROM's secondary entry
   is secure ARM state; the sample does not independently prove an NS handoff.
5. All callbacks used after boot must be in ._secure.text; data must be in
   ._secure.data. Inspect final map/relocations and undefined calls: no printf,
   malloc, normal U-Boot udelay or overwritten DRAM helper may be reachable.
   Verify secure relocation and D-cache clean/I-cache invalidate happen before
   programming SOFT_ENTRY. Candidate polling is bounded iteration timing from
   the source sample, not calibrated microseconds.
6. Linux CPU nodes can use enable-method="psci" with PSCI 0.1 and its CPU_ON
   function ID. The current arch/arm/lib/psci-dt.c UNCONDITIONALLY adds
   cpu_off/cpu_suspend/migrate for PSCI 0.1. A candidate-gated change in that
   function must remove any pre-existing versions of these three properties
   and skip re-adding them; setting a CPU_ON-only input DTS is insufficient.
   Retain only compatible="arm,psci", method="smc" and cpu_on=<0x95c1ba60>.
   Linux drivers/firmware/psci/psci.c initializes only the functions named in
   the 0.1 node; arch/arm/kernel/psci_smp.c then rejects CPU offline with
   EOPNOTSUPP when cpu_off is absent. Remove maxcpus=1 only in the candidate
   image, keep a CPU0 fallback build and proven reset-to-FEL.
7. The ready/release handshake is extra defensive code, not upstream evidence:
   a late secondary stays in resident SRAM after failure instead of entering
   Linux after CPU_ON reported timeout. CPU0 serially starts CPU1..CPU3 only.
   CPU0 is the only state writer; secondaries write separate ready flags, so
   a late ready notification cannot overwrite a concurrent FAILED decision.

Qualification boundary
A working watchdog/full-reset -> FEL recovery path must precede trial boots.
First test cpu_on success + CPU0 periodic timer/WFI wakeup + four logical CPUs.
Then test per-core workloads, cross-core cached memory visibility, interrupts,
network/storage integrity and a full-system reset back to FEL. The sample uses
SRAM with MMU/D-cache disabled and does not establish those Linux properties.
Do not attempt CPU offline/online, deep idle, suspend or multi-core kexec until
a separately supported T153 CPU_OFF path is implemented and validated.

Translation-unit validation
The C and assembly candidates were compiled independently with GCC 14.3.0 and
the current U-Boot 2026.07 generated headers. The check injected PSCI 0.1 and
four contexts; it did not configure or link a PSCI firmware. See COMPILE-REVIEW.txt.
Runtime references resolve only to the expected resident generic PSCI helpers
and local secure data/entry at object level. Full-link relocation, SRAM fit,
TrustZone permissions, cache coherence and hardware operation remain untested.

This directory contains isolated review sources only. No default configuration,
live build tree, boot image or board state was changed while preparing it.
Temporary objects and detailed logs are under u-boot-work/host-tests/t153-smp-review.
