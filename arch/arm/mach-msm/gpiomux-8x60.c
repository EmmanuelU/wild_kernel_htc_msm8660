/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/module.h>
#include <mach/irqs.h>
#include <asm/mach-types.h>
#include <mach/gpiomux.h>
#include "gpiomux-8x60.h"

static struct gpiomux_setting spi_active = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting spi_suspended_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting spi_suspended_cs_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting i2c_active = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting i2c_suspended_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting msm_snddev_active_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting msm_snddev_suspend_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting aux_pcm_active_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting aux_pcm_suspend_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting uart1dm_active = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting uart1dm_suspended = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting lcdc_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting lcdc_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdp_vsync_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting tm_active = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting tm_suspended = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mdp_vsync_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting pmic_suspended_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm8x60_gsbi_configs[] __initdata = {
	{
		.gpio      = 33,
		.settings = {
			[GPIOMUX_SUSPENDED] = &spi_suspended_config,
			[GPIOMUX_ACTIVE]    = &spi_active,
		},
	},
	{
		.gpio      = 34,
		.settings = {
			[GPIOMUX_SUSPENDED] = &spi_suspended_config,
			[GPIOMUX_ACTIVE]    = &spi_active,
		},
	},
	{
		.gpio      = 35,
		.settings = {
			[GPIOMUX_SUSPENDED] = &spi_suspended_cs_config,
			[GPIOMUX_ACTIVE]    = &spi_active,
		},
	},
	{
		.gpio      = 36,
		.settings = {
			[GPIOMUX_SUSPENDED] = &spi_suspended_config,
			[GPIOMUX_ACTIVE]    = &spi_active,
		},
	},
	{
		.gpio      = 43,
		.settings = {
			[GPIOMUX_SUSPENDED] = &i2c_suspended_config,
			[GPIOMUX_ACTIVE]    = &i2c_active,
		},
	},
	{
		.gpio      = 44,
		.settings = {
			[GPIOMUX_SUSPENDED] = &i2c_suspended_config,
			[GPIOMUX_ACTIVE]    = &i2c_active,
		},
	},
};

static struct msm_gpiomux_config msm8x60_uart_configs[] __initdata = {
	{ 
		.gpio      = 53,
		.settings = {
			[GPIOMUX_ACTIVE]    = &uart1dm_active,
			[GPIOMUX_SUSPENDED] = &uart1dm_suspended,
		},
	},
	{ 
		.gpio      = 54,
		.settings = {
			[GPIOMUX_ACTIVE]    = &uart1dm_active,
			[GPIOMUX_SUSPENDED] = &uart1dm_suspended,
		},
	},
	{ 
		.gpio      = 55,
		.settings = {
			[GPIOMUX_ACTIVE]    = &uart1dm_active,
			[GPIOMUX_SUSPENDED] = &uart1dm_suspended,
		},
	},
	{ 
		.gpio      = 56,
		.settings = {
			[GPIOMUX_ACTIVE]    = &uart1dm_active,
			[GPIOMUX_SUSPENDED] = &uart1dm_suspended,
		},
	},
};

static struct msm_gpiomux_config msm8x60_tmg200_configs[] __initdata = {
	{
		.gpio = 61,
		.settings = {
			[GPIOMUX_ACTIVE] = &tm_active,
			[GPIOMUX_SUSPENDED] = &tm_suspended,
		},
	},
};

static struct msm_gpiomux_config msm8x60_aux_pcm_configs[] __initdata = {
	{
		.gpio = 111,
		.settings = {
			[GPIOMUX_ACTIVE]    = &aux_pcm_active_config,
			[GPIOMUX_SUSPENDED] = &aux_pcm_suspend_config,
		},
	},
	{
		.gpio = 112,
		.settings = {
			[GPIOMUX_ACTIVE]    = &aux_pcm_active_config,
			[GPIOMUX_SUSPENDED] = &aux_pcm_suspend_config,
		},
	},
	{
		.gpio = 113,
		.settings = {
			[GPIOMUX_ACTIVE]    = &aux_pcm_active_config,
			[GPIOMUX_SUSPENDED] = &aux_pcm_suspend_config,
		},
	},
	{
		.gpio = 114,
		.settings = {
			[GPIOMUX_ACTIVE]    = &aux_pcm_active_config,
			[GPIOMUX_SUSPENDED] = &aux_pcm_suspend_config,
		},
	},
};

static struct msm_gpiomux_config msm8x60_snd_configs[] __initdata = {
	{
		.gpio = 108,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm_snddev_active_config,
			[GPIOMUX_SUSPENDED] = &msm_snddev_suspend_config,
		},
	},
	{
		.gpio = 109,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm_snddev_active_config,
			[GPIOMUX_SUSPENDED] = &msm_snddev_suspend_config,
		},
	},
};

static struct msm_gpiomux_config msm8x60_lcdc_configs[] __initdata = {
	
	{
		.gpio = 0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 2,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 3,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 4,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 5,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 6,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 7,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 8,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 9,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 10,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 11,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 12,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 13,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 14,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 15,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 16,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 17,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 18,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 19,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 20,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 21,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 22,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 23,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 24,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 25,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 26,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
	{
		.gpio = 27,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcdc_active_cfg,
			[GPIOMUX_SUSPENDED] = &lcdc_suspend_cfg,
		},
	},
};

static struct msm_gpiomux_config msm8x60_mdp_vsync_configs[] __initdata = {
	{
		.gpio = 28,
		.settings = {
			[GPIOMUX_ACTIVE]    = &mdp_vsync_active_cfg,
			[GPIOMUX_SUSPENDED] = &mdp_vsync_suspend_cfg,
		},
	},
};

static struct msm_gpiomux_config msm8x60_pmic_configs[] __initdata = {
	{
		.gpio = 88,
		.settings = {
			[GPIOMUX_SUSPENDED] = &pmic_suspended_cfg,
		},
	},
	{
		.gpio = 91,
		.settings = {
			[GPIOMUX_SUSPENDED] = &pmic_suspended_cfg,
		},
	},
};

struct msm_gpiomux_configs
msm8x60_pyramid_gpiomux_cfgs[] __initdata = {
	{msm8x60_gsbi_configs, ARRAY_SIZE(msm8x60_gsbi_configs)},
	{msm8x60_uart_configs, ARRAY_SIZE(msm8x60_uart_configs)},
	{msm8x60_aux_pcm_configs, ARRAY_SIZE(msm8x60_aux_pcm_configs)},
	{msm8x60_snd_configs, ARRAY_SIZE(msm8x60_snd_configs)},
	{msm8x60_lcdc_configs, ARRAY_SIZE(msm8x60_lcdc_configs)},
	{msm8x60_mdp_vsync_configs, ARRAY_SIZE(msm8x60_mdp_vsync_configs)},
	{msm8x60_pmic_configs, ARRAY_SIZE(msm8x60_pmic_configs)},
	{msm8x60_tmg200_configs, ARRAY_SIZE(msm8x60_tmg200_configs)},
	{NULL, 0},
};

void __init msm8x60_init_gpiomux(struct msm_gpiomux_configs *cfgs)
{
	int rc;

	rc = msm_gpiomux_init(NR_GPIO_IRQS);
	if (rc) {
		pr_err("%s failure: %d\n", __func__, rc);
		return;
	}

	while (cfgs->cfg) {
		msm_gpiomux_install(cfgs->cfg, cfgs->ncfg);
		++cfgs;
	}
}
