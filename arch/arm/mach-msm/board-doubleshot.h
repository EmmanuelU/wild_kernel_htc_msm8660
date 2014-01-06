/* Copyright (C) 2010-2011 HTC Corporation.
 * Copyright (c) 2013 Sebastian Sobczyk <sebastiansobczyk@wp.pl>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ARCH_ARM_MACH_MSM_BOARD_DOUBLESHOT_H
#define __ARCH_ARM_MACH_MSM_BOARD_DOUBLESHOT_H

#include <mach/board.h>
#include <mach/msm_memtypes.h>

#define MSM_RAM_CONSOLE_BASE	MSM_HTC_RAM_CONSOLE_PHYS
#define MSM_RAM_CONSOLE_SIZE	MSM_HTC_RAM_CONSOLE_SIZE

//pyramid fragments:
#define DOUBLESHOT_GPIO_CAM_MCLK     	 (32)
#define DOUBLESHOT_GPIO_CAM_VCM_PD      (58)
#define DOUBLESHOT_GPIO_CAM1_RSTz       (137)
#define DOUBLESHOT_GPIO_CAM2_RSTz       (138)
#define DOUBLESHOT_GPIO_CAM2_PWDN       (140)
#define DOUBLESHOT_GPIO_MCLK_SWITCH     (141)
/////



/* Macros assume PMIC GPIOs start at 0 */
#define PM8058_GPIO_BASE                        NR_MSM_GPIOS
#define PM8058_GPIO_PM_TO_SYS(pm_gpio)                (pm_gpio + PM8058_GPIO_BASE)
#define PM8058_GPIO_SYS_TO_PM(sys_gpio)                (sys_gpio - PM8058_GPIO_BASE)
#define PM8058_IRQ_BASE                                (NR_MSM_IRQS + NR_GPIO_IRQS)

#define PM8901_GPIO_BASE                        (PM8058_GPIO_BASE + \
                                                PM8058_GPIOS + PM8058_MPPS)
#define PM8901_GPIO_PM_TO_SYS(pm_gpio)                (pm_gpio + PM8901_GPIO_BASE)
#define PM8901_GPIO_SYS_TO_PM(sys_gpio)                (sys_gpio - PM901_GPIO_BASE)
#define PM8901_IRQ_BASE                                (PM8058_IRQ_BASE + \
                                                NR_PMIC8058_IRQS)

/*#define GPIO_EXPANDER_GPIO_BASE \
        (PM8901_GPIO_BASE + PM8901_MPPS)
#define GPIO_EXPANDER_IRQ_BASE (PM8901_IRQ_BASE + NR_PMIC8901_IRQS)
*/

#define MSM_RAM_CONSOLE_BASE        MSM_HTC_RAM_CONSOLE_PHYS
#define MSM_RAM_CONSOLE_SIZE        MSM_HTC_RAM_CONSOLE_SIZE



/* Memory map */

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
                defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE) || \
                defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
                defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)
#define QCE_SIZE                0x10000
#define QCE_0_BASE                0x18500000
#endif

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
#define MSM_FB_PRIM_BUF_SIZE (960 * ALIGN(540, 32) * 4 * 3) /* 4 bpp x 3 pages */
#else
#define MSM_FB_PRIM_BUF_SIZE (960 * ALIGN(540, 32) * 4 * 2) /* 4 bpp x 2 pages */
#endif

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
#define MSM_FB_EXT_BUF_SIZE  (1920 * 1080 * 2 * 1) /* 2 bpp x 1 page */
#else
#ifdef CONFIG_FB_MSM_TVOUT
#define MSM_FB_EXT_BUF_SIZE   0x195000 /*tvout = 720 x 576 x 2(bpp) x 2(pages)*/
#else
#define MSM_FB_EXT_BUF_SIZE        0
#endif
#endif

#ifdef CONFIG_FB_MSM_OVERLAY_WRITEBACK
/* width x height x 3 bpp x 2 frame buffer */
#define MSM_FB_WRITEBACK_SIZE roundup(960 * ALIGN(540, 32) * 3 * 2, 4096)
#define MSM_FB_WRITEBACK_OFFSET 0
#else
#define MSM_FB_WRITEBACK_SIZE        0
#define MSM_FB_WRITEBACK_OFFSET 0
#endif

/* Note: must be multiple of 4096 */
//#define MSM_FB_SIZE roundup(MSM_FB_PRIM_BUF_SIZE + MSM_FB_EXT_BUF_SIZE, 4096)

#define MSM_PMEM_MDP_SIZE        0x2000000
#define MSM_PMEM_ADSP_SIZE        0x23AC000
#define MSM_PMEM_ADSP2_SIZE        0x654000 /* 1152 * 1920 * 1.5 * 2 */
#define MSM_PMEM_AUDIO_SIZE        0x239000
#define MSM_PMEM_KERNEL_EBI1_SIZE        0xC7000

#define MSM_FB_WRITEBACK_BASE        (0x45C00000)
#define MSM_PMEM_AUDIO_BASE        (0x46400000)
#define MSM_PMEM_ADSP_BASE        (0x40400000)
#define MSM_PMEM_ADSP2_BASE        (MSM_PMEM_ADSP_BASE + MSM_PMEM_ADSP_SIZE)
#define MSM_FB_BASE                (0x70000000 - MSM_FB_SIZE)
#define MSM_PMEM_MDP_BASE        (0x6D600000)
#define MSM_PMEM_KERNEL_EBI1_BASE        (MSM_PMEM_AUDIO_BASE + MSM_PMEM_AUDIO_SIZE)

#define MSM_SMI_BASE          0x38000000
#define MSM_SMI_SIZE          0x4000000

/* Kernel SMI PMEM Region for video core, used for Firmware */
/* and encoder,decoder scratch buffers */
/* Kernel SMI PMEM Region Should always precede the user space */
/* SMI PMEM Region, as the video core will use offset address */
/* from the Firmware base */
#define KERNEL_SMI_BASE       (MSM_SMI_BASE)
#define KERNEL_SMI_SIZE       0x400000

/* User space SMI PMEM Region for video core*/
/* used for encoder, decoder input & output buffers  */
#define USER_SMI_BASE         (KERNEL_SMI_BASE + KERNEL_SMI_SIZE)
#define USER_SMI_SIZE         (MSM_SMI_SIZE - KERNEL_SMI_SIZE)
#define MSM_PMEM_SMIPOOL_BASE USER_SMI_BASE
#define MSM_PMEM_SMIPOOL_SIZE USER_SMI_SIZE

#define PHY_BASE_ADDR1  0x48000000

/* GPIO definition */


/* Direct Keys */
#define DOUBLESHOT_GPIO_KEY_CAM_STEP1   (99)
#define DOUBLESHOT_GPIO_KEY_CAM_STEP2   (124)
#define DOUBLESHOT_GPIO_KEY_POWER       (125)
#define DOUBLESHOT_GPIO_KEY_SLID_INT    (127)

/* Battery */
#define DOUBLESHOT_GPIO_MBAT_IN         (61)
#define DOUBLESHOT_GPIO_CHG_INT                (126)

/* Wifi */
#define DOUBLESHOT_GPIO_WIFI_IRQ              (46)
#define DOUBLESHOT_GPIO_WIFI_SHUTDOWN_N       (96)
#define DOUBLESHOT_GPIO_WIFI_BT_SLEEP_CLK_EN        PMGPIO(38)
/* Sensors */
#define DOUBLESHOT_SENSOR_I2C_SDA                (72)
#define DOUBLESHOT_SENSOR_I2C_SCL                (73)
#define DOUBLESHOT_GSENSOR_INT                (129)
#define DOUBLESHOT_COMPASS_INT                (128)

/* Microp */
#define DOUBLESHOT_GPIO_UP_RESET_N           (39)
#define DOUBLESHOT_GPIO_UP_INT_N           (57)

/* TP */
#define DOUBLESHOT_TP_I2C_SDA           (51)
#define DOUBLESHOT_TP_I2C_SCL           (52)
#define DOUBLESHOT_TP_ATT_N             (65)
#define DOUBLESHOT_TP_ATT_N_XB          (50)

/* General */
#define DOUBLESHOT_GENERAL_I2C_SDA           (59)
#define DOUBLESHOT_GENERAL_I2C_SCL           (60)

/* LCD */
#define GPIO_LCM_ID1_IM1                (50)
#define GPIO_LCM_ID1_IM1_XB                (65)
#define GPIO_LCM_RST_N                        (66)
#define GPIO_LCD_TE	(28)

/* Audio */
#define DOUBLESHOT_AUD_CODEC_RST        (67)
#define DOUBLESHOT_AUD_QTR_TX_MCLK1    (108)
#define DOUBLESHOT_AUD_RX_MCLK1        (109)
#define DOUBLESHOT_AUD_TX_I2S_SD2      (110)

/* BT */
#define DOUBLESHOT_GPIO_BT_HOST_WAKE      (45)
#define DOUBLESHOT_GPIO_BT_UART1_TX       (53)
#define DOUBLESHOT_GPIO_BT_UART1_RX       (54)
#define DOUBLESHOT_GPIO_BT_UART1_CTS      (55)
#define DOUBLESHOT_GPIO_BT_UART1_RTS      (56)
#define DOUBLESHOT_GPIO_BT_SHUTDOWN_N     (100)
#define DOUBLESHOT_GPIO_BT_CHIP_WAKE      (130)
#define DOUBLESHOT_GPIO_BT_RESET_N        (142)

/* USB */
#define DOUBLESHOT_GPIO_USB_ID            (63)
#define DOUBLESHOT_GPIO_MHL_RESET        (70)
#define DOUBLESHOT_GPIO_MHL_INT        (71)
#define DOUBLESHOT_GPIO_MHL_USB_SWITCH        (99)

/* DOCK */
#define DOUBLESHOT_GPIO_DOCK_PIN          (71)

/* Camera */
#define DOUBLESHOT_CAM_CAM1_ID           (10)
#define DOUBLESHOT_CAM_I2C_SDA           (47)
#define DOUBLESHOT_CAM_I2C_SCL           (48)




/* Flashlight */
#define DOUBLESHOT_FLASH_EN             (29)
#define DOUBLESHOT_TORCH_EN             (30)

/* Accessory */
#define DOUBLESHOT_GPIO_AUD_HP_DET        (31)

/* SPI */
#define DOUBLESHOT_SPI_DO                 (33)
#define DOUBLESHOT_SPI_DI                 (34)
#define DOUBLESHOT_SPI_CS                 (35)
#define DOUBLESHOT_SPI_CLK                (36)

/* PMIC */

/* PMIC GPIO definition */
#define PMGPIO(x) (x-1)
#define DOUBLESHOT_FMTX_ANT_SW        PMGPIO(8)
#define DOUBLESHOT_KEYMATRIX_DRV1     PMGPIO(9)
#define DOUBLESHOT_KEYMATRIX_DRV2     PMGPIO(10)
#define DOUBLESHOT_KEYMATRIX_DRV3     PMGPIO(11)
#define DOUBLESHOT_KEYMATRIX_DRV4     PMGPIO(12)
#define DOUBLESHOT_KEYMATRIX_DRV5     PMGPIO(13)
#define DOUBLESHOT_KEYMATRIX_DRV6     PMGPIO(14)
#define DOUBLESHOT_KEYMATRIX_DRV7     PMGPIO(15)
#define DOUBLESHOT_VOL_UP             PMGPIO(16)
#define DOUBLESHOT_VOL_DN             PMGPIO(17)
#define DOUBLESHOT_AUD_HANDSET_ENO    PMGPIO(18)
#define DOUBLESHOT_AUD_SPK_ENO        PMGPIO(19)
#define DOUBLESHOT_WIRELESS_CHG_OK1   PMGPIO(20)
#define DOUBLESHOT_AUD_QTR_RESET      PMGPIO(23)
#define DOUBLESHOT_AUD_HPTV_DET_HP    PMGPIO(24)
#define DOUBLESHOT_AUD_HPTV_DET_TV    PMGPIO(25)
#define DOUBLESHOT_AUD_MIC_SEL        PMGPIO(26)
#define DOUBLESHOT_CHG_STAT              PMGPIO(33)
#define DOUBLESHOT_PLS_INT            PMGPIO(35)
#define DOUBLESHOT_AUD_TVOUT_HP_SEL   PMGPIO(36)
#define DOUBLESHOT_AUD_REMO_PRES      PMGPIO(37)
#define DOUBLESHOT_WIFI_BT_SLEEP_CLK  PMGPIO(38)

// pyramid
#define DOUBLESHOT_AUD_HP_EN          PMGPIO(18)
#define DOUBLESHOT_SDC3_DET           PMGPIO(34)
// --- these are already allocated for HPTV
//#define DOUBLESHOT_GREEN_LED          PMGPIO(24)
//#define DOUBLESHOT_AMBER_LED          PMGPIO(25)


#define DOUBLESHOT_LAYOUTS                        { \
                { { 0,  1, 0}, {-1,  0,  0}, {0, 0,  1} }, \
                { { 0, -1, 0}, { 1,  0,  0}, {0, 0, -1} }, \
                { {-1,  0, 0}, { 0, -1,  0}, {0, 0,  1} }, \
                { {-1,  0, 0}, { 0,  0, -1}, {0, 1,  0} }  \
                        }
#define DOUBLESHOT_TP_RST             PMGPIO(21)

extern int panel_type;

int __init doubleshot_init_mmc(void);
void __init doubleshot_audio_init(void);
int __init doubleshot_init_keypad(void);
unsigned int doubleshot_get_engineerid(void);
int __init doubleshot_wifi_init(void);

void doubleshot_init_fb(void);
void doubleshot_allocate_fb_region(void);
void __init doubleshot_mdp_writeback(struct memtype_reserve* reserve_table);

#endif 
