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

#define DOUBLESHOT_GPIO_KEY_POWER          (125)

#define DOUBLESHOT_GPIO_MBAT_IN            (61)
#define DOUBLESHOT_GPIO_CHG_INT		(126)

#define DOUBLESHOT_GPIO_WIFI_IRQ              (46)
#define DOUBLESHOT_GPIO_WIFI_SHUTDOWN_N       (96)

#define DOUBLESHOT_SENSOR_I2C_SDA		(72)
#define DOUBLESHOT_SENSOR_I2C_SCL		(73)
#define DOUBLESHOT_GYRO_INT               (127)
#define DOUBLESHOT_ECOMPASS_INT           (128)
#define DOUBLESHOT_GSENSOR_INT           (129)

#define DOUBLESHOT_GENERAL_I2C_SDA		(59)
#define DOUBLESHOT_GENERAL_I2C_SCL		(60)

#define DOUBLESHOT_FLASH_EN             (29)
#define DOUBLESHOT_TORCH_EN             (30)

#define DOUBLESHOT_GPIO_AUD_HP_DET        (31)

#define DOUBLESHOT_SPI_DO                 (33)
#define DOUBLESHOT_SPI_DI                 (34)
#define DOUBLESHOT_SPI_CS                 (35)
#define DOUBLESHOT_SPI_CLK                (36)

#define DOUBLESHOT_TP_I2C_SDA           (51)
#define DOUBLESHOT_TP_I2C_SCL           (52)
#define DOUBLESHOT_TP_ATT_N             (65)
#define DOUBLESHOT_TP_ATT_N_XB       (50)

#define GPIO_LCD_TE	(28)

#define GPIO_LCM_RST_N			(66)
#define GPIO_LCM_ID			(50)

#define DOUBLESHOT_AUD_CODEC_RST        (67)

#define DOUBLESHOT_GPIO_BT_HOST_WAKE      (45)
#define DOUBLESHOT_GPIO_BT_UART1_TX       (53)
#define DOUBLESHOT_GPIO_BT_UART1_RX       (54)
#define DOUBLESHOT_GPIO_BT_UART1_CTS      (55)
#define DOUBLESHOT_GPIO_BT_UART1_RTS      (56)
#define DOUBLESHOT_GPIO_BT_SHUTDOWN_N     (100)
#define DOUBLESHOT_GPIO_BT_CHIP_WAKE      (130)
#define DOUBLESHOT_GPIO_BT_RESET_N        (142)

#define DOUBLESHOT_GPIO_USB_ID        (63)
#define DOUBLESHOT_GPIO_MHL_RESET        (70)
#define DOUBLESHOT_GPIO_MHL_INT        (71)
#define DOUBLESHOT_GPIO_MHL_USB_SWITCH        (99)

#define DOUBLESHOT_CAM_CAM1_ID           (10)
#define DOUBLESHOT_CAM_I2C_SDA           (47)
#define DOUBLESHOT_CAM_I2C_SCL           (48)

#define DOUBLESHOT_GPIO_CAM_MCLK     	 (32)
#define DOUBLESHOT_GPIO_CAM_VCM_PD      (58)
#define DOUBLESHOT_GPIO_CAM1_RSTz       (137)
#define DOUBLESHOT_GPIO_CAM2_RSTz       (138)
#define DOUBLESHOT_GPIO_CAM2_PWDN       (140)
#define DOUBLESHOT_GPIO_MCLK_SWITCH     (141)

#define PMGPIO(x) (x-1)
#define DOUBLESHOT_VOL_UP             PMGPIO(16)
#define DOUBLESHOT_VOL_DN             PMGPIO(17)
#define DOUBLESHOT_AUD_HP_EN          PMGPIO(18)
#define DOUBLESHOT_HAP_ENABLE         PMGPIO(19)
#define DOUBLESHOT_AUD_QTR_RESET      PMGPIO(23)
#define DOUBLESHOT_TP_RST             PMGPIO(21)
#define DOUBLESHOT_GREEN_LED          PMGPIO(24)
#define DOUBLESHOT_AMBER_LED          PMGPIO(25)
#define DOUBLESHOT_AUD_MIC_SEL        PMGPIO(26)
#define DOUBLESHOT_CHG_STAT           PMGPIO(33)
#define DOUBLESHOT_SDC3_DET           PMGPIO(34)
#define DOUBLESHOT_PLS_INT            PMGPIO(35)
#define DOUBLESHOT_AUD_REMO_PRES      PMGPIO(37)
#define DOUBLESHOT_WIFI_BT_SLEEP_CLK  PMGPIO(38)

extern int panel_type;

int __init doubleshot_init_mmc(void);
void __init doubleshot_audio_init(void);
int __init doubleshot_init_keypad(void);
int __init doubleshot_wifi_init(void);

void doubleshot_init_fb(void);
void doubleshot_allocate_fb_region(void);
void __init doubleshot_mdp_writeback(struct memtype_reserve* reserve_table);

#endif 
