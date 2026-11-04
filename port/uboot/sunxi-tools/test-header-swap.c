// SPDX-License-Identifier: GPL-2.0-only
/* Execute the production pass_fel_information against an SRAM byte model.
 * No USB code is linked; unused production sections are discarded by ld.
 */
#define main uncalled_sunxi_fel_main
#include "fel.c"
#undef main

static uint8_t memory[0x80000];
static uint32_t expected_header, last_read, last_write;
static unsigned writes;

void aw_fel_read(feldev_handle *dev, uint32_t offset, void *buf, size_t len)
{
	(void)dev;
	assert(offset == expected_header + 0x14 && len == 4);
	assert((uint64_t)offset + len <= sizeof(memory));
	last_read = offset;
	memcpy(buf, memory + offset, len);
}

void aw_fel_write(feldev_handle *dev, const void *buf, uint32_t offset, size_t len)
{
	(void)dev;
	assert(offset == expected_header + 0x18 && len == 8);
	assert((uint64_t)offset + len <= sizeof(memory));
	last_write = offset;
	writes++;
	memcpy(memory + offset, buf, len);
}

static void header(uint32_t address)
{
	memset(memory + address, 0, 96);
	memcpy(memory + address + 4, "eGON.BT0", 8);
	memcpy(memory + address + 0x14, "SPL\x02", 4);
}

static void swap_buffers(const sram_swap_buffers *swap)
{
	for (; swap && swap->size; swap++) {
		assert((uint64_t)swap->buf1 + swap->size <= sizeof(memory));
		assert((uint64_t)swap->buf2 + swap->size <= sizeof(memory));
		for (uint32_t i = 0; i < swap->size; i++) {
			uint8_t value = memory[swap->buf1 + i];
			memory[swap->buf1 + i] = memory[swap->buf2 + i];
			memory[swap->buf2 + i] = value;
		}
	}
}

static void pass_check(soc_info_t *soc, uint32_t address)
{
	feldev_handle dev = { .soc_info = soc };
	uint32_t data[2];
	expected_header = address;
	writes = 0;
	pass_fel_information(&dev, 0x4f100000, 0);
	assert(writes == 1 && last_read == address + 0x14 && last_write == address + 0x18);
	memcpy(data, memory + address + 0x18, sizeof(data));
	assert(le32toh(data[0]) == 0x4f100000 && le32toh(data[1]) == 0);
}

int main(void)
{
	/* Use the actual pinned T153 table, including both thunk swaps. */
	soc_info_t *t153 = get_soc_info_from_id(0x1922);
	assert(t153 && t153->spl_addr == 0x40000);
	assert(t153->swap_buffers[0].buf1 == 0x40000);
	assert(t153->swap_buffers[0].buf2 == 0x66c00);
	assert(t153->swap_buffers[0].size == 0x1400);
	memset(memory, 0xa5, sizeof(memory));
	header(0x66c00); /* aw_fel_write_and_execute_spl stages header here. */
	swap_buffers(t153->swap_buffers);
	assert(!memcmp(memory + 0x40004, "eGON.BT0", 8));
	memcpy(memory + 0x40008, ".FEL", 4); /* thunk's pre-SPL signature change */
	swap_buffers(t153->swap_buffers); /* thunk restores BootROM after SPL */
	assert(!memcmp(memory + 0x66c04, "eGON.FEL", 8));
	pass_check(t153, 0x66c00);
	for (uint32_t i = 0x40000; i < 0x41400; i++)
		assert(memory[i] == 0xa5); /* Never modify the restored BootROM workspace. */

	/* A10 does not swap its header: existing platforms keep the same address. */
	soc_info_t *a10 = get_soc_info_from_id(0x1623);
	assert(a10 && a10->spl_addr == 0);
	header(0);
	pass_check(a10, 0);

	/* A header inside (not at the beginning of) a swap keeps its displacement. */
	sram_swap_buffers table[] = { {0x20000, 0x30000, 0x400}, {0, 0, 0} };
	soc_info_t interior = { .spl_addr = 0x20100, .swap_buffers = table };
	header(0x30100);
	pass_check(&interior, 0x30100);

	/* Empty/absent swap tables retain the original SPL address. */
	sram_swap_buffers none[] = { {0, 0, 0} };
	soc_info_t plain = { .spl_addr = 0x20000, .swap_buffers = none };
	header(0x20000);
	pass_check(&plain, 0x20000);
	plain.swap_buffers = NULL;
	pass_check(&plain, 0x20000);

	/* Bad or unsupported signatures must not trigger any write. */
	feldev_handle dev = { .soc_info = t153 };
	expected_header = 0x66c00;
	writes = 0;
	memcpy(memory + 0x66c14, "BAD\x02", 4);
	pass_fel_information(&dev, 0x4f100000, 0);
	assert(writes == 0);
	header(0x66c00);
	memory[0x66c17] = 0;
	pass_fel_information(&dev, 0x4f100000, 0);
	assert(writes == 0);
	memory[0x66c17] = 0x20;
	pass_fel_information(&dev, 0x4f100000, 0);
	assert(writes == 0);
	puts("PASS: production FEL header handoff; real T153 double-swap, A10, offset/no-swap and invalid signatures");
	return 0;
}
