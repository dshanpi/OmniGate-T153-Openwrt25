/* SPDX-License-Identifier: GPL-2.0+ */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#define BIT(n) (1U << (n))
extern u32 ddr_test_read(unsigned long addr);
extern void ddr_test_write(u32 value, unsigned long addr);
extern void ddr_test_delay(unsigned long usec);
#define readl(a) ddr_test_read((unsigned long)(a))
#define writel(v, a) ddr_test_write((v), (unsigned long)(a))
#define udelay(n) ddr_test_delay(n)
#define dsb() do {} while (0)
