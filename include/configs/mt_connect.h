/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019 NXP
 */

#ifndef __MT_CONNECT_H
#define __MT_CONNECT_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>
#include "imx_env.h"

#define UBOOT_ITB_OFFSET			0x57C00
#define FSPI_CONF_BLOCK_SIZE		0x1000
#define UBOOT_ITB_OFFSET_FSPI  \
	(UBOOT_ITB_OFFSET + FSPI_CONF_BLOCK_SIZE)
#ifdef CONFIG_FSPI_CONF_HEADER
#define CFG_SYS_UBOOT_BASE  \
	(QSPI0_AMBA_BASE + UBOOT_ITB_OFFSET_FSPI)
#else
#define CFG_SYS_UBOOT_BASE	\
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)
#endif

#ifdef CONFIG_SPL_BUILD
/* malloc f used before GD_FLG_FULL_MALLOC_INIT set */
#define CFG_MALLOC_F_ADDR		0x930000
/* For RAW image gives a error info not panic */

#endif

#define PHY_ANEG_TIMEOUT 20000

#ifdef CONFIG_NAND_BOOT
#define MFG_NAND_PARTITION "mtdparts=gpmi-nand:64m(nandboot),16m(nandfit),64m(nandkernel),16m(nanddtb),8m(nandtee),-(nandrootfs)"
#endif

#ifdef CONFIG_DISTRO_DEFAULTS
#define BOOT_TARGET_DEVICES(func) \
	func(USB, usb, 0) \
	func(MMC, mmc, 1)

#include <config_distro_bootcmd.h>
#else
#define BOOTENV
#endif

/*
 * Another approach is add the clocks for inmates into clks_init_on
 * in clk-imx8mm.c, then clk_ingore_unused could be removed.
 */
#define JH_ROOT_DTB    "imx8mm-evk-root.dtb"

#define JAILHOUSE_ENV \
	"jh_clk= \0 " \
	"jh_root_dtb=" JH_ROOT_DTB "\0" \
	"jh_mmcboot=mw 0x303d0518 0xff; setenv fdtfile ${jh_root_dtb};" \
		"setenv jh_clk clk_ignore_unused mem=1212MB; " \
			   "if run loadimage; then " \
				   "run mmcboot; " \
			   "else run jh_netboot; fi; \0" \
	"jh_netboot=mw 0x303d0518 0xff; setenv fdtfile ${jh_root_dtb}; setenv jh_clk clk_ignore_unused mem=1212MB; run netboot; \0 "

#define SR_IR_V2_COMMAND \
	"nodes=/usbg1 /usbg2 /wdt-reboot /soc@0/caam-sm@100000 /soc@0/bus@30000000/caam_secvio /soc@0/bus@30000000/caam-snvs@30370000 /soc@0/bus@32c00000/lcdif@32e00000 /soc@0/bus@32c00000/csi1_bridge@32e20000 /soc@0/bus@32c00000/mipi_dsi@32e10000 /soc@0/bus@32c00000/mipi_csi@32e30000 /soc@0/bus@32c00000/display-subsystem /audio-codec-bt-sco /sound-bt-sco /audio-codec /sound-wm8524 /dsi-host /rm67199_panel /soc@0/bus@30800000/i2c@30a20000/pca9450@25 /soc@0/bus@30800000/i2c@30a30000/adv7535@3d /soc@0/bus@30800000/i2c@30a30000/tcpc@50 /soc@0/memory-controller@3d400000 /soc@0/bus@30800000/spi@30830000/spi@0 /binman /vpu_h1@38320000 /vpu_g1@38300000 /vpu_g2@38310000 /vpu_v4l2 /gpu@38000000 \0" \
	"sr_ir_v2_cmd=cp.b ${fdtcontroladdr} ${fdt_addr_r} 0x10000;"\
	"fdt addr ${fdt_addr_r};"\
	"for i in ${nodes}; do fdt rm ${i}; done \0"

#define CFG_MFG_ENV_SETTINGS \
	CFG_MFG_ENV_SETTINGS_DEFAULT \
	"initrd_addr=0x43800000\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=0\0"\
	"sd_dev=1\0" \

/* Initial environment variables */
#define CONFIG_BOOTCOMMAND "run distro_bootcmd"

/* Link Definitions */

#define CFG_SYS_INIT_RAM_ADDR		0x40000000
#define CFG_SYS_INIT_RAM_SIZE		0x200000

#define CFG_SYS_SDRAM_BASE		0x40000000
#define PHYS_SDRAM			0x40000000
#define PHYS_SDRAM_SIZE			0x80000000 /* 2GB DDR */

#define TCML_M4_TEXT_SRAM_START		0x007E0000
#define TCML_M4_TEXT_SRAM_SIZE		0x20000

#define CFG_FEC_MXC_PHYADDR		0x1

#define CFG_MXC_UART_BASE		UART_BASE_ADDR(2)

#define CFG_SYS_FSL_USDHC_NUM		1
#define CFG_SYS_FSL_ESDHC_ADDR		0

#define CFG_SYS_NAND_BASE		0x20000000

#ifdef CONFIG_IMX_MATTER_TRUSTY
#define NS_ARCH_ARM64 1
#endif

#endif
