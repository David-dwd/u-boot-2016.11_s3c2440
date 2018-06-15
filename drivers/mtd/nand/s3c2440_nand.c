/*
 * (C) Copyright 2006 OpenMoko, Inc.
 * Author: Harald Welte <laforge@openmoko.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>

#include <nand.h>
#include <asm/arch/s3c24x0_cpu.h>
#include <asm/io.h>

#define S3C2410_NFCONF_EN          (1<<15)
#define S3C2410_NFCONF_512BYTE     (1<<14)
#define S3C2410_NFCONF_4STEP       (1<<13)
#define S3C2410_NFCONF_INITECC     (1<<12)
#define S3C2410_NFCONF_nFCE        (1<<11)
#define S3C2410_NFCONF_TACLS(x)    ((x)<<8)
#define S3C2410_NFCONF_TWRPH0(x)   ((x)<<4)
#define S3C2410_NFCONF_TWRPH1(x)   ((x)<<0)

#define S3C2410_ADDR_NALE 4
#define S3C2410_ADDR_NCLE 8

#ifdef CONFIG_NAND_SPL

/* in the early stage of NAND flash booting, printf() is not available */
#define printf(fmt, args...)

static void nand_read_buf(struct mtd_info *mtd, u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd_to_nand(mtd);

	for (i = 0; i < len; i++)
		buf[i] = readb(this->IO_ADDR_R);
}
#endif

static void s3c24x0_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct s3c24x0_nand *nand = s3c24x0_get_base_nand();

	debug("hwcontrol(): 0x%02x 0x%02x\n", cmd, ctrl);

	if (ctrl & NAND_CLE)
	{
		/* 发命令 */
		writeb(cmd, &nand->nfcmd);
	}
	else if(ctrl & NAND_ALE)
	{
		/* 发地址 */
		writeb(cmd, &nand->nfaddr);     
	}

}

static int s3c24x0_dev_ready(struct mtd_info *mtd)
{
	struct s3c24x0_nand *nand = s3c24x0_get_base_nand();
	debug("dev_ready\n");
	return readl(&nand->nfstat) & 0x01;
}

static void s3c2440_nand_select(struct mtd_info *mtd, int chipnr)
{
	struct s3c24x0_nand *nand = s3c24x0_get_base_nand();
	
	switch (chipnr) {
	case -1: /* 取消选中 */
	   nand->nfcont |= (1<<1);
	   break;
	case 0:  /* 选中 */
	   nand->nfcont &= ~(1<<1);
	   break;
	default:
		BUG();
		break;
	}
}

int board_nand_init(struct nand_chip *nand)
{
	u_int32_t cfg;
	u_int8_t tacls, twrph0, twrph1;
	struct s3c24x0_clock_power *clk_power = s3c24x0_get_base_clock_power();
	struct s3c24x0_nand *nand_reg = s3c24x0_get_base_nand();

	debug("board_nand_init()\n");

	writel(readl(&clk_power->clkcon) | (1 << 4), &clk_power->clkcon);

	/* initialize hardware */
#if defined(CONFIG_S3C24XX_CUSTOM_NAND_TIMING)
	tacls  = CONFIG_S3C24XX_TACLS;
	twrph0 = CONFIG_S3C24XX_TWRPH0;
	twrph1 =  CONFIG_S3C24XX_TWRPH1;
#else
	tacls = 4;
	twrph0 = 8;
	twrph1 = 8;
#endif
	/* 初始化时序 */
	
	cfg = ((tacls-1)<<12)|((twrph0-1)<<8)|((twrph1-1)<<4);
	writel(cfg, &nand_reg->nfconf);
	/* 使能NAND Flash控制器, 初始化ECC, 禁止片选 */
	writel((1<<4)|(1<<1)|(1<<0),  &nand_reg->nfcont);
	
	/* initialize nand_chip data structure */
	nand->IO_ADDR_R = (void *)&nand_reg->nfdata;
	nand->IO_ADDR_W = (void *)&nand_reg->nfdata;

	nand->select_chip = s3c2440_nand_select;

	/* read_buf and write_buf are default */
	/* read_byte and write_byte are default */
#ifdef CONFIG_NAND_SPL
	nand->read_buf = nand_read_buf;
#endif

	/* hwcontrol always must be implemented */
	nand->cmd_ctrl = s3c24x0_hwcontrol;

	nand->dev_ready = s3c24x0_dev_ready;

#ifdef CONFIG_S3C2410_NAND_HWECC
	nand->ecc.hwctl = s3c24x0_nand_enable_hwecc;
	nand->ecc.calculate = s3c24x0_nand_calculate_ecc;
	nand->ecc.correct = s3c24x0_nand_correct_data;
	nand->ecc.mode = NAND_ECC_HW;
	nand->ecc.size = CONFIG_SYS_NAND_ECCSIZE;
	nand->ecc.bytes = CONFIG_SYS_NAND_ECCBYTES;
	nand->ecc.strength = 1;
#else
	nand->ecc.mode = NAND_ECC_SOFT;
#endif

#ifdef CONFIG_S3C2410_NAND_BBT
	nand->bbt_options |= NAND_BBT_USE_FLASH;
#endif

	debug("end of nand_init\n");

	return 0;
}
