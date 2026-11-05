# SPDX-License-Identifier: GPL-2.0+
obj-y += board.o mmc.o reset.o

ifdef CONFIG_XPL_BUILD
obj-y += spl.o fel.o fel-request.o ddr.o
endif

ifndef CONFIG_XPL_BUILD
obj-$(CONFIG_T153_PSCI_COLD_SMP) += psci-t153.o psci-entry-t153.o smp-dt.o
endif
