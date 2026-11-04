// SPDX-License-Identifier: GPL-2.0-only
/* Read-only eMMC EXT_CSD inspection. No switch/write/erase command exists here. */
#include <errno.h>
#include <fcntl.h>
#include <linux/mmc/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	struct mmc_ioc_cmd cmd = {0};
	struct stat st;
	uint8_t data[512] = {0};
	const char *number;
	int fd, ret;

	if (argc != 3 || strcmp(argv[1], "--device") ||
	    strncmp(argv[2], "/dev/mmcblk", 11)) {
		fprintf(stderr, "Usage: t153-mmc-info --device /dev/mmcblkN\n");
		return 2;
	}
	number = argv[2] + 11;
	if (!*number || strspn(number, "0123456789") != strlen(number)) {
		fprintf(stderr, "Require the whole eMMC user-area device\n");
		return 2;
	}
	fd = open(argv[2], O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
	if (fd < 0) { perror("open eMMC read-only"); return 1; }
	if (fstat(fd, &st) || !S_ISBLK(st.st_mode)) {
		fprintf(stderr, "Not a whole block device\n");
		close(fd); return 1;
	}
	cmd.opcode = 8; /* MMC_SEND_EXT_CSD */
	cmd.flags = 0x35; /* MMC_RSP_R1 | MMC_CMD_ADTC */
	cmd.blksz = sizeof(data);
	cmd.blocks = 1;
	cmd.data_timeout_ns = 1000000000;
	mmc_ioc_cmd_set_data(cmd, data);
	ret = ioctl(fd, MMC_IOC_CMD, &cmd);
	if (ret) { perror("read EXT_CSD"); close(fd); return 1; }
	/* R1 status error bits; do not accept a failed command's zero buffer. */
	if (cmd.response[0] & 0xfdffa080U) {
		fprintf(stderr, "EXT_CSD R1 error: %08x\n", cmd.response[0]);
		close(fd); return 1;
	}
	if (data[192] < 3 || data[192] > 8) {
		fprintf(stderr, "Unsupported EXT_CSD revision: %u\n", data[192]);
		close(fd); return 1;
	}
	printf("PARTITION_CONFIG=%u\nBOOT_BUS_CONDITIONS=%u\n", data[179], data[177]);
	return close(fd) ? 1 : 0;
}
