// SPDX-License-Identifier: GPL-2.0+
/*
 * PIO command engine adapted from U-Boot 2026.07 drivers/mmc/sunxi_mmc.c.
 * (C) Copyright 2007-2011 Allwinner Technology Co., Ltd.
 * Aaron <leafy.myeh@allwinnertech.com>
 * T153 platform glue: oscillator only, SDR <= 12 MHz, no media writes.
 */
#include <dm.h>
#include <errno.h>
#include <log.h>
#include <mmc.h>
#include <time.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/bitops.h>
#include <linux/iopoll.h>
#include <linux/build_bug.h>
#include "sunxi_mmc.h"
#include "t153.h"

struct sunxi_mmc_plat {
	struct mmc_config cfg;
	struct mmc mmc;
};

struct sunxi_mmc_priv {
	unsigned int mmc_no;
	u32 *mclkreg;
	unsigned int fatal_err;
	struct sunxi_mmc *reg;
};

static int mmc_set_mod_clk(struct sunxi_mmc_priv *priv, unsigned int hz)
{
	unsigned int div;

	if (!hz || hz > 12000000)
		return -EINVAL;
	/* 24 MHz oscillator, 2x SDR clock; 5-bit linear M divider. */
	div = DIV_ROUND_UP(24000000U, hz * 2);
	if (!div || div > 32)
		return -EINVAL;
	clrbits_le32(priv->mclkreg, BIT(31));
	writel(BIT(31) | (div - 1), priv->mclkreg);
	return 0;
}

static int mmc_update_clk(struct sunxi_mmc_priv *priv)
{
	unsigned int cmd;
	unsigned timeout_msecs = 2000;
	unsigned long start = get_timer(0);

	cmd = SUNXI_MMC_CMD_START |
	      SUNXI_MMC_CMD_UPCLK_ONLY |
	      SUNXI_MMC_CMD_WAIT_PRE_OVER;

	writel(cmd, &priv->reg->cmd);
	while (readl(&priv->reg->cmd) & SUNXI_MMC_CMD_START) {
		if (get_timer(start) > timeout_msecs)
			return -1;
	}

	/* clock update sets various irq status bits, clear these */
	writel(readl(&priv->reg->rint), &priv->reg->rint);

	return 0;
}

static int mmc_config_clock(struct sunxi_mmc_priv *priv, struct mmc *mmc)
{
	unsigned rval = readl(&priv->reg->clkcr);

	/* Disable Clock */
	rval &= ~SUNXI_MMC_CLK_ENABLE;
	writel(rval, &priv->reg->clkcr);
	if (mmc_update_clk(priv))
		return -1;

	/* Set mod_clk to new rate */
	if (mmc_set_mod_clk(priv, mmc->clock))
		return -1;

	/* Clear internal divider */
	rval &= ~SUNXI_MMC_CLK_DIVIDER_MASK;
	writel(rval, &priv->reg->clkcr);

	/* T153 new timing uses a 2x module clock. */
	setbits_le32(&priv->reg->ntsr, SUNXI_MMC_NTSR_MODE_SEL_NEW);
	/* 180-degree drive phase; TM1 on SD, TM4 on eMMC. */
	clrsetbits_le32((void *)priv->reg + 0x140, 3 << 16, 3 << 16);
	if (priv->mmc_no == 0)
		clrsetbits_le32(&priv->reg->ntsr, 3 << 4, 1 << 4);
	else {
		writel(SUNXI_MMC_CAL_DL_SW_EN, &priv->reg->samp_dl);
		setbits_le32((void *)priv->reg + 0x104, BIT(0));
	}

	/* Re-enable Clock */
	rval |= SUNXI_MMC_CLK_ENABLE;
	writel(rval, &priv->reg->clkcr);
	if (mmc_update_clk(priv))
		return -1;

	return 0;
}

static int sunxi_mmc_set_ios_common(struct sunxi_mmc_priv *priv,
				    struct mmc *mmc)
{
	debug("set ios: bus_width: %x, clock: %d\n",
	      mmc->bus_width, mmc->clock);

	/* Change clock first */
	if (mmc->clock && mmc_config_clock(priv, mmc) != 0) {
		priv->fatal_err = 1;
		return -EINVAL;
	}

	/* Change bus width */
	if (mmc->bus_width == 8)
		writel(0x2, &priv->reg->width);
	else if (mmc->bus_width == 4)
		writel(0x1, &priv->reg->width);
	else
		writel(0x0, &priv->reg->width);

	return 0;
}

static int mmc_trans_data_by_cpu(struct sunxi_mmc_priv *priv, struct mmc *mmc,
				 struct mmc_data *data)
{
	const int reading = !!(data->flags & MMC_DATA_READ);
	const uint32_t status_bit = reading ? SUNXI_MMC_STATUS_FIFO_EMPTY :
					      SUNXI_MMC_STATUS_FIFO_FULL;
	unsigned i;
	unsigned *buff = (unsigned int *)(reading ? data->dest : data->src);
	unsigned word_cnt = (data->blocksize * data->blocks) >> 2;
	unsigned timeout_msecs = word_cnt >> 6;
	uint32_t status;
	unsigned long  start;

	if (timeout_msecs < 2000)
		timeout_msecs = 2000;

	/* Always read / write data through the CPU */
	setbits_le32(&priv->reg->gctrl, SUNXI_MMC_GCTRL_ACCESS_BY_AHB);

	start = get_timer(0);

	for (i = 0; i < word_cnt;) {
		unsigned int in_fifo;

		if (get_timer(start) > timeout_msecs ||
		    (readl(&priv->reg->rint) & SUNXI_MMC_RINT_INTERRUPT_ERROR_BIT))
			return -ETIMEDOUT;

		while ((status = readl(&priv->reg->status)) & status_bit) {
			if (get_timer(start) > timeout_msecs)
				return -1;
		}

		/*
		 * For writing we do not easily know the FIFO size, so have
		 * to check the FIFO status after every word written.
		 * TODO: For optimisation we could work out a minimum FIFO
		 * size across all SoCs, and use that together with the current
		 * fill level to write chunks of words.
		 */
		if (!reading) {
			writel(buff[i++], &priv->reg->fifo);
			continue;
		}

		/*
		 * The status register holds the current FIFO level, so we
		 * can be sure to collect as many words from the FIFO
		 * register without checking the status register after every
		 * read. That saves half of the costly MMIO reads, effectively
		 * doubling the read performance.
		 * Some SoCs (A20) report a level of 0 if the FIFO is
		 * completely full (value masked out?). Use a safe minimal
		 * FIFO size in this case.
		 */
		in_fifo = SUNXI_MMC_STATUS_FIFO_LEVEL(status);
		if (in_fifo == 0 && (status & SUNXI_MMC_STATUS_FIFO_FULL))
			in_fifo = 32;
		for (; in_fifo > 0 && i < word_cnt; in_fifo--)
			buff[i++] = readl_relaxed(&priv->reg->fifo);
		dmb();
	}

	return 0;
}

static int mmc_rint_wait(struct sunxi_mmc_priv *priv, struct mmc *mmc,
			 uint timeout_msecs, uint done_bit, const char *what)
{
	unsigned int status;
	unsigned long start = get_timer(0);

	do {
		status = readl(&priv->reg->rint);
		if ((get_timer(start) > timeout_msecs) ||
		    (status & SUNXI_MMC_RINT_INTERRUPT_ERROR_BIT)) {
			debug("%s timeout %x\n", what,
			      status & SUNXI_MMC_RINT_INTERRUPT_ERROR_BIT);
			return -ETIMEDOUT;
		}
	} while (!(status & done_bit));

	return 0;
}

static int sunxi_mmc_send_cmd_common(struct sunxi_mmc_priv *priv,
				     struct mmc *mmc, struct mmc_cmd *cmd,
				     struct mmc_data *data)
{
	unsigned int cmdval = SUNXI_MMC_CMD_START;
	unsigned int timeout_msecs;
	int error = 0;
	unsigned int status = 0;
	unsigned int bytecnt = 0;

	if (priv->fatal_err)
		return -EIO;
	if (data && (data->flags & MMC_DATA_WRITE))
		return -EROFS;
	if (data && (!data->blocks || !data->blocksize ||
		     (data->blocksize & 3) || data->blocksize > 512 ||
		     data->blocks > CONFIG_SYS_MMC_MAX_BLK_COUNT))
		return -EINVAL;
	if (cmd->resp_type & MMC_RSP_BUSY)
		debug("mmc cmd %d check rsp busy\n", cmd->cmdidx);
	if (cmd->cmdidx == 12)
		return 0;

	if (!cmd->cmdidx)
		cmdval |= SUNXI_MMC_CMD_SEND_INIT_SEQ;
	if (cmd->resp_type & MMC_RSP_PRESENT)
		cmdval |= SUNXI_MMC_CMD_RESP_EXPIRE;
	if (cmd->resp_type & MMC_RSP_136)
		cmdval |= SUNXI_MMC_CMD_LONG_RESPONSE;
	if (cmd->resp_type & MMC_RSP_CRC)
		cmdval |= SUNXI_MMC_CMD_CHK_RESPONSE_CRC;

	if (data) {
		if ((u32)(long)data->dest & 0x3) {
			error = -1;
			goto out;
		}

		cmdval |= SUNXI_MMC_CMD_DATA_EXPIRE|SUNXI_MMC_CMD_WAIT_PRE_OVER;
		if (data->flags & MMC_DATA_WRITE)
			cmdval |= SUNXI_MMC_CMD_WRITE;
		if (data->blocks > 1)
			cmdval |= SUNXI_MMC_CMD_AUTO_STOP;
		writel(data->blocksize, &priv->reg->blksz);
		writel(data->blocks * data->blocksize, &priv->reg->bytecnt);
	}

	debug("mmc %d, cmd %d(0x%08x), arg 0x%08x\n", priv->mmc_no,
	      cmd->cmdidx, cmdval | cmd->cmdidx, cmd->cmdarg);
	writel(cmd->cmdarg, &priv->reg->arg);

	if (!data)
		writel(cmdval | cmd->cmdidx, &priv->reg->cmd);

	/*
	 * transfer data and check status
	 * STATREG[2] : FIFO empty
	 * STATREG[3] : FIFO full
	 */
	if (data) {
		int ret = 0;

		bytecnt = data->blocksize * data->blocks;
		debug("trans data %d bytes\n", bytecnt);
		writel(cmdval | cmd->cmdidx, &priv->reg->cmd);
		ret = mmc_trans_data_by_cpu(priv, mmc, data);
		if (ret) {
			error = readl(&priv->reg->rint) &
				SUNXI_MMC_RINT_INTERRUPT_ERROR_BIT;
			error = -ETIMEDOUT;
			goto out;
		}
	}

	error = mmc_rint_wait(priv, mmc, 1000, SUNXI_MMC_RINT_COMMAND_DONE,
			      "cmd");
	if (error)
		goto out;

	if (data) {
		timeout_msecs = 120;
		debug("cacl timeout %x msec\n", timeout_msecs);
		error = mmc_rint_wait(priv, mmc, timeout_msecs,
				      data->blocks > 1 ?
				      SUNXI_MMC_RINT_AUTO_COMMAND_DONE :
				      SUNXI_MMC_RINT_DATA_OVER,
				      "data");
		if (error)
			goto out;
	}

	if (cmd->resp_type & MMC_RSP_BUSY) {
		unsigned long start = get_timer(0);
		timeout_msecs = 2000;

		do {
			status = readl(&priv->reg->status);
			if (get_timer(start) > timeout_msecs) {
				debug("busy timeout\n");
				error = -ETIMEDOUT;
				goto out;
			}
		} while (status & SUNXI_MMC_STATUS_CARD_DATA_BUSY);
	}

	if (cmd->resp_type & MMC_RSP_136) {
		cmd->response[0] = readl(&priv->reg->resp3);
		cmd->response[1] = readl(&priv->reg->resp2);
		cmd->response[2] = readl(&priv->reg->resp1);
		cmd->response[3] = readl(&priv->reg->resp0);
		debug("mmc resp 0x%08x 0x%08x 0x%08x 0x%08x\n",
		      cmd->response[3], cmd->response[2],
		      cmd->response[1], cmd->response[0]);
	} else {
		cmd->response[0] = readl(&priv->reg->resp0);
		debug("mmc resp 0x%08x\n", cmd->response[0]);
	}
out:
	if (error < 0) {
		writel(SUNXI_MMC_GCTRL_RESET, &priv->reg->gctrl);
		mmc_update_clk(priv);
	}
	writel(0xffffffff, &priv->reg->rint);
	writel(readl(&priv->reg->gctrl) | SUNXI_MMC_GCTRL_FIFO_RESET,
	       &priv->reg->gctrl);

	return error;
}

static int sunxi_mmc_reset(void *regs)
{
	u32 val;
	int ret;
	/* Reset controller */
	writel(SUNXI_MMC_GCTRL_RESET, regs + SUNXI_MMC_GCTRL);
	ret = readl_poll_timeout(regs + SUNXI_MMC_GCTRL, val,
				 !(val & SUNXI_MMC_GCTRL_RESET), 10000);
	if (ret)
		return ret;
	writel(0xffffffff, regs + 0x08); /* response/data timeout */
	writel(0, regs + 0x30); /* interrupts are polled */
	writel(0xffffffff, regs + 0x38);

	{
		/* Reset card */
		writel(SUNXI_MMC_HWRST_ASSERT, regs + SUNXI_MMC_HWRST);
		udelay(10);
		writel(SUNXI_MMC_HWRST_DEASSERT, regs + SUNXI_MMC_HWRST);
		udelay(300);

		/* Setup FIFO R/W threshold. Needed on H616. */
		writel(SUNXI_MMC_THLDC_READ_THLD(512) |
		       SUNXI_MMC_THLDC_WRITE_EN |
		       SUNXI_MMC_THLDC_READ_EN, regs + SUNXI_MMC_THLDC);
	}
	return 0;
}


static int t153_mmc_set_ios(struct udevice *dev)
{
	struct sunxi_mmc_plat *plat = dev_get_plat(dev);
	return sunxi_mmc_set_ios_common(dev_get_priv(dev), &plat->mmc);
}

static int t153_mmc_send_cmd(struct udevice *dev, struct mmc_cmd *cmd,
			   struct mmc_data *data)
{
	struct sunxi_mmc_plat *plat = dev_get_plat(dev);
	return sunxi_mmc_send_cmd_common(dev_get_priv(dev), &plat->mmc, cmd, data);
}

static int t153_mmc_getcd(struct udevice *dev)
{
	/* SD has no reliable card-detect wiring in the initial board port. */
	return 1;
}

static const struct dm_mmc_ops t153_mmc_ops = {
	.send_cmd = t153_mmc_send_cmd,
	.set_ios = t153_mmc_set_ios,
	.get_cd = t153_mmc_getcd,
};

static int t153_mmc_probe(struct udevice *dev)
{
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct sunxi_mmc_plat *plat = dev_get_plat(dev);
	struct sunxi_mmc_priv *priv = dev_get_priv(dev);
	struct mmc_config *cfg = &plat->cfg;
	void *ccu = (void *)0x02002000;
	unsigned int pin;
	int ret;

	BUILD_BUG_ON(offsetof(struct sunxi_mmc, fifo) != 0x200);
	BUILD_BUG_ON(offsetof(struct sunxi_mmc, samp_dl) != 0x144);
	priv->reg = dev_read_addr_ptr(dev);
	if ((uintptr_t)priv->reg == 0x04020000)
		priv->mmc_no = 0;
	else if ((uintptr_t)priv->reg == 0x04022000)
		priv->mmc_no = 2;
	else
		return -EINVAL;

	cfg->name = dev->name;
	cfg->voltages = MMC_VDD_32_33 | MMC_VDD_33_34;
	cfg->b_max = CONFIG_SYS_MMC_MAX_BLK_COUNT;
	cfg->f_min = 400000;
	cfg->f_max = 12000000;
	ret = mmc_of_parse(dev, cfg);
	if (ret)
		return ret;

	if (priv->mmc_no == 0) {
		for (pin = 0; pin < 6; pin++)
			t153_pin(5, pin, 1);
	} else {
		for (pin = 0; pin < 10; pin++)
			t153_pin(2, pin, 1);
		t153_pin(2, 11, 1); /* eMMC reset; PC10 data strobe is not used. */
	}

	priv->mclkreg = ccu + 0xd00 + priv->mmc_no * 0x10;
	setbits_le32(ccu + 0x5c0, BIT(5 + priv->mmc_no));
	setbits_le32(ccu + 0xd0c + priv->mmc_no * 0x10, BIT(0) | BIT(16));
	ret = mmc_set_mod_clk(priv, 400000);
	if (ret)
		return ret;
	ret = sunxi_mmc_reset(priv->reg);
	if (ret)
		return ret;
	upriv->mmc = &plat->mmc;
	return 0;
}

static int t153_mmc_bind(struct udevice *dev)
{
	struct sunxi_mmc_plat *plat = dev_get_plat(dev);
	return mmc_bind(dev, &plat->mmc, &plat->cfg);
}

static const struct udevice_id t153_mmc_ids[] = {
	{ .compatible = "dshanpi,t153-ram-mmc" },
	{ }
};

U_BOOT_DRIVER(t153_ram_mmc) = {
	.name = "t153_ram_mmc",
	.id = UCLASS_MMC,
	.of_match = t153_mmc_ids,
	.bind = t153_mmc_bind,
	.probe = t153_mmc_probe,
	.ops = &t153_mmc_ops,
	.plat_auto = sizeof(struct sunxi_mmc_plat),
	.priv_auto = sizeof(struct sunxi_mmc_priv),
};
