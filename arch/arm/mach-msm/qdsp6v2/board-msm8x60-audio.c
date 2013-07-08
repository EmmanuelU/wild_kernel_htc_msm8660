/* Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
 * Copyright (c) 2013 Sebastian Sobczyk <sebastiansobczyk@wp.pl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/mfd/pmic8058.h>
#include <linux/mfd/pmic8901.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>

#include <mach/qdsp6v2/audio_dev_ctl.h>
#include <sound/apr_audio.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include <mach/board-msm8660.h>

#include "snddev_icodec.h"
#include "snddev_ecodec.h"
#include "snddev_hdmi.h"
#include "snddev_mi2s.h"
#include <linux/spi/spi_aic3254.h>
#include "snddev_virtual.h"

#undef pr_info
#undef pr_err
#define pr_info(fmt, ...) pr_aud_info(fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...) pr_aud_err(fmt, ##__VA_ARGS__)

static struct q6v2audio_analog_ops default_audio_ops;
static struct q6v2audio_analog_ops *audio_ops = &default_audio_ops;

#ifdef CONFIG_DEBUG_FS
static struct dentry *debugfs_hsed_config;
static void snddev_hsed_config_modify_setting(int type);
static void snddev_hsed_config_restore_setting(void);
#endif

int speaker_enable(void)
{
	if (audio_ops->speaker_enable)
		audio_ops->speaker_enable(1);
	return 0;
}

void speaker_disable(void)
{
	if (audio_ops->speaker_enable)
		audio_ops->speaker_enable(0);
}

int headset_enable(void)
{
	if (audio_ops->headset_enable)
		audio_ops->headset_enable(1);
	return 0;
}

void headset_disable(void)
{
	if (audio_ops->headset_enable)
		audio_ops->headset_enable(0);
}

int handset_enable(void)
{
	if (audio_ops->handset_enable)
		audio_ops->handset_enable(1);
	return 0;
}

void handset_disable(void)
{
	if (audio_ops->handset_enable)
		audio_ops->handset_enable(0);
}

int headset_speaker_enable(void)
{
	if (audio_ops->headset_speaker_enable)
		audio_ops->headset_speaker_enable(1);
	return 0;
}

void headset_speaker_disable(void)
{
	if (audio_ops->headset_speaker_enable)
		audio_ops->headset_speaker_enable(0);
}

int int_mic_enable(void)
{
	if (audio_ops->int_mic_enable)
		audio_ops->int_mic_enable(1);
	return 0;
}

void int_mic_disable(void)
{
	if (audio_ops->int_mic_enable)
		audio_ops->int_mic_enable(0);
}

int back_mic_enable(void)
{
	if (audio_ops->back_mic_enable)
		audio_ops->back_mic_enable(1);
	return 0;
}

void back_mic_disable(void)
{
	if (audio_ops->back_mic_enable)
		audio_ops->back_mic_enable(0);
}

int ext_mic_enable(void)
{
	if (audio_ops->ext_mic_enable)
		audio_ops->ext_mic_enable(1);
	return 0;
}

void ext_mic_disable(void)
{
	if (audio_ops->ext_mic_enable)
		audio_ops->ext_mic_enable(0);
}

int stereo_mic_enable(void)
{
	if (audio_ops->stereo_mic_enable)
		audio_ops->stereo_mic_enable(1);
	return 0;
}

void stereo_mic_disable(void)
{
	if (audio_ops->stereo_mic_enable)
		audio_ops->stereo_mic_enable(0);
}

int usb_headset_enable(void)
{
	if (audio_ops->usb_headset_enable)
		audio_ops->usb_headset_enable(1);
	return 0;
}

void usb_headset_disable(void)
{
	if (audio_ops->usb_headset_enable)
		audio_ops->usb_headset_enable(0);
}

int fm_headset_enable(void)
{
	if (audio_ops->fm_headset_enable)
		audio_ops->fm_headset_enable(1);
	return 0;
}

void fm_headset_disable(void)
{
	if (audio_ops->fm_headset_enable)
		audio_ops->fm_headset_enable(0);
}

int fm_speaker_enable(void)
{
	if (audio_ops->fm_speaker_enable)
		audio_ops->fm_speaker_enable(1);
	return 0;
}

void fm_speaker_disable(void)
{
	if (audio_ops->fm_speaker_enable)
		audio_ops->fm_speaker_enable(0);
}

int voltage_on(void)
{
	if (audio_ops->voltage_on)
		audio_ops->voltage_on(1);
	return 0;
}

void voltage_off(void)
{
	if (audio_ops->voltage_on)
		audio_ops->voltage_on(0);
}

static struct regulator *s3;
static struct regulator *mvs;

static int msm_snddev_enable_dmic_power(void)
{
	int ret;

	pr_aud_err("%s", __func__);
	s3 = regulator_get(NULL, "8058_s3");
	if (IS_ERR(s3)) {
		ret = -EBUSY;
		goto fail_get_s3;
	}

	ret = regulator_set_voltage(s3, 1800000, 1800000);
	if (ret) {
		pr_err("%s: error setting voltage\n", __func__);
		goto fail_s3;
	}

	ret = regulator_enable(s3);
	if (ret) {
		pr_err("%s: error enabling regulator\n", __func__);
		goto fail_s3;
	}

	mvs = regulator_get(NULL, "8901_mvs0");
	if (IS_ERR(mvs))
		goto fail_mvs0_get;

	ret = regulator_enable(mvs);
	if (ret) {
		pr_err("%s: error setting regulator\n", __func__);
		goto fail_mvs0_enable;
	}
	stereo_mic_enable();
	return ret;

fail_mvs0_enable:
	regulator_put(mvs);
	mvs = NULL;
fail_mvs0_get:
	regulator_disable(s3);
fail_s3:
	regulator_put(s3);
	s3 = NULL;
fail_get_s3:
	return ret;
}

static void msm_snddev_disable_dmic_power(void)
{
	int ret;

	pr_aud_err("%s", __func__);
	if (mvs) {
		ret = regulator_disable(mvs);
		if (ret < 0)
			pr_err("%s: error disabling vreg mvs\n", __func__);
		regulator_put(mvs);
		mvs = NULL;
	}

	if (s3) {
		ret = regulator_disable(s3);
		if (ret < 0)
			pr_err("%s: error disabling regulator s3\n", __func__);
		regulator_put(s3);
		s3 = NULL;
	}
	stereo_mic_disable();
}

static struct regulator *snddev_reg_ncp;
static struct regulator *snddev_reg_l10;

static atomic_t preg_ref_cnt;

static int msm_snddev_voltage_on(void)
{
	int rc;
	pr_debug("%s\n", __func__);

	if (atomic_inc_return(&preg_ref_cnt) > 1)
		return 0;

	snddev_reg_l10 = regulator_get(NULL, "8058_l10");
	if (IS_ERR(snddev_reg_l10)) {
		pr_err("%s: regulator_get(%s) failed (%ld)\n", __func__,
			"l10", PTR_ERR(snddev_reg_l10));
		return -EBUSY;
	}

	rc = regulator_set_voltage(snddev_reg_l10, 2600000, 2600000);
	if (rc < 0)
		pr_err("%s: regulator_set_voltage(l10) failed (%d)\n",
			__func__, rc);

	rc = regulator_enable(snddev_reg_l10);
	if (rc < 0)
		pr_err("%s: regulator_enable(l10) failed (%d)\n", __func__, rc);

	snddev_reg_ncp = regulator_get(NULL, "8058_ncp");
	if (IS_ERR(snddev_reg_ncp)) {
		pr_err("%s: regulator_get(%s) failed (%ld)\n", __func__,
			"ncp", PTR_ERR(snddev_reg_ncp));
		return -EBUSY;
	}

	rc = regulator_set_voltage(snddev_reg_ncp, 1800000, 1800000);
	if (rc < 0) {
		pr_err("%s: regulator_set_voltage(ncp) failed (%d)\n",
			__func__, rc);
		goto regulator_fail;
	}

	rc = regulator_enable(snddev_reg_ncp);
	if (rc < 0) {
		pr_err("%s: regulator_enable(ncp) failed (%d)\n", __func__, rc);
		goto regulator_fail;
	}

	return rc;

regulator_fail:
	regulator_put(snddev_reg_ncp);
	snddev_reg_ncp = NULL;
	return rc;
}

static void msm_snddev_voltage_off(void)
{
	int rc;
	pr_debug("%s\n", __func__);

	if (!snddev_reg_ncp)
		goto done;

	if (atomic_dec_return(&preg_ref_cnt) == 0) {
		rc = regulator_disable(snddev_reg_ncp);
		if (rc < 0)
			pr_err("%s: regulator_disable(ncp) failed (%d)\n",
				__func__, rc);
		regulator_put(snddev_reg_ncp);

		snddev_reg_ncp = NULL;
	}

done:
	if (!snddev_reg_l10)
		return;

	rc = regulator_disable(snddev_reg_l10);
	if (rc < 0)
		pr_err("%s: regulator_disable(l10) failed (%d)\n",
			__func__, rc);

	regulator_put(snddev_reg_l10);

	snddev_reg_l10 = NULL;
}

static int msm_snddev_enable_dmic_sec_power(void)
{
	int ret;

	pr_aud_err("%s", __func__);
	ret = msm_snddev_enable_dmic_power();
	if (ret) {
		pr_err("%s: Error: Enabling dmic power failed\n", __func__);
		return ret;
	}
#ifdef CONFIG_PMIC8058_OTHC
	ret = pm8058_micbias_enable(OTHC_MICBIAS_2, OTHC_SIGNAL_ALWAYS_ON);
	if (ret) {
		pr_err("%s: Error: Enabling micbias failed\n", __func__);
		msm_snddev_disable_dmic_power();
		return ret;
	}
#endif
	return 0;
}

static void msm_snddev_disable_dmic_sec_power(void)
{
	pr_aud_err("%s", __func__);
	msm_snddev_disable_dmic_power();

#ifdef CONFIG_PMIC8058_OTHC
	pm8058_micbias_enable(OTHC_MICBIAS_2, OTHC_SIGNAL_OFF);
#endif
}

static struct snddev_icodec_data snddev_iearpiece_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "handset_rx",
	.copp_id = 0,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = handset_enable,
	.pamp_off = handset_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = PLAYBACK_RECEIVER,
	.aic3254_voc_id = CALL_DOWNLINK_IMIC_RECEIVER,
	.default_aic3254_id = PLAYBACK_RECEIVER,
};

static struct platform_device msm_iearpiece_device = {
	.name = "snddev_icodec",
	.id = 0,
	.dev = { .platform_data = &snddev_iearpiece_data },
};

static struct snddev_icodec_data snddev_ihac_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "hac_rx",
	.copp_id = 0,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = handset_enable,
	.pamp_off = handset_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = PLAYBACK_RECEIVER,
	.aic3254_voc_id = HAC,
	.default_aic3254_id = PLAYBACK_RECEIVER,
};

static struct platform_device msm_ihac_device = {
	.name = "snddev_icodec",
	.id = 38,
	.dev = { .platform_data = &snddev_ihac_data },
};

static struct snddev_icodec_data snddev_imic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_tx",
	.copp_id = 1,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = int_mic_enable,
	.pamp_off = int_mic_disable,
	.aic3254_id = VOICERECOGNITION_IMIC,
	.aic3254_voc_id = CALL_UPLINK_IMIC_RECEIVER,
	.default_aic3254_id = VOICERECOGNITION_IMIC,
};

static struct platform_device msm_imic_device = {
	.name = "snddev_icodec",
	.id = 1,
	.dev = { .platform_data = &snddev_imic_data },
};

static struct snddev_icodec_data snddev_nomic_headset_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "nomic_headset_tx",
	.copp_id = 1,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = int_mic_enable,
	.pamp_off = int_mic_disable,
	.aic3254_id = VOICERECORD_IMIC,
	.aic3254_voc_id = CALL_UPLINK_IMIC_HEADSET,
	.default_aic3254_id = VOICERECORD_IMIC,
};

static struct platform_device msm_nomic_headset_tx_device = {
	.name = "snddev_icodec",
	.id = 40,
	.dev = { .platform_data = &snddev_nomic_headset_data },
};

static struct snddev_icodec_data snddev_ihs_stereo_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_stereo_rx",
	.copp_id = 0,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = headset_enable,
	.pamp_off = headset_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = PLAYBACK_HEADSET,
	.aic3254_voc_id = CALL_DOWNLINK_EMIC_HEADSET,
	.default_aic3254_id = PLAYBACK_HEADSET,
};

static struct platform_device msm_headset_stereo_device = {
	.name = "snddev_icodec",
	.id = 34,
	.dev = { .platform_data = &snddev_ihs_stereo_rx_data },
};

static struct snddev_icodec_data snddev_nomic_ihs_stereo_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "nomic_headset_stereo_rx",
	.copp_id = 0,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = headset_enable,
	.pamp_off = headset_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = PLAYBACK_HEADSET,
	.aic3254_voc_id = CALL_DOWNLINK_IMIC_HEADSET,
	.default_aic3254_id = PLAYBACK_HEADSET,
};

static struct platform_device msm_nomic_headset_stereo_device = {
	.name = "snddev_icodec",
	.id = 39,
	.dev = { .platform_data = &snddev_nomic_ihs_stereo_rx_data },
};

static struct snddev_icodec_data snddev_anc_headset_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE | SNDDEV_CAP_ANC),
	.name = "anc_headset_stereo_rx",
	.copp_id = PRIMARY_I2S_RX,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = int_mic_enable,
	.pamp_off = int_mic_disable,
};

static struct platform_device msm_anc_headset_device = {
	.name = "snddev_icodec",
	.id = 51,
	.dev = { .platform_data = &snddev_anc_headset_data },
};

static struct snddev_icodec_data snddev_ispkr_stereo_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "speaker_stereo_rx",
	.copp_id = 0,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = speaker_enable,
	.pamp_off = speaker_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = PLAYBACK_SPEAKER,
	.aic3254_voc_id = CALL_DOWNLINK_IMIC_SPEAKER,
	.default_aic3254_id = PLAYBACK_SPEAKER,
};

static struct platform_device msm_ispkr_stereo_device = {
	.name = "snddev_icodec",
	.id = 2,
	.dev = { .platform_data = &snddev_ispkr_stereo_data },
};

static struct snddev_icodec_data snddev_ispkr_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_mono_tx",
	.copp_id = PRIMARY_I2S_TX,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = int_mic_enable,
	.pamp_off = int_mic_disable,
	.aic3254_id = VOICERECORD_IMIC,
	.aic3254_voc_id = CALL_UPLINK_IMIC_SPEAKER,
	.default_aic3254_id = VOICERECORD_IMIC,
};

static struct platform_device msm_ispkr_mic_device = {
	.name = "snddev_icodec",
	.id = 3,
	.dev = { .platform_data = &snddev_ispkr_mic_data },
};

static struct snddev_icodec_data snddev_dual_mic_endfire_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_dual_mic_endfire_tx",
	.copp_id = PRIMARY_I2S_TX,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
	.aic3254_id = VOICERECORD_IMIC,
	.aic3254_voc_id = CALL_UPLINK_IMIC_RECEIVER,
	.default_aic3254_id = VOICERECORD_IMIC,
};

static struct platform_device msm_hs_dual_mic_endfire_device = {
	.name = "snddev_icodec",
	.id = 14,
	.dev = { .platform_data = &snddev_dual_mic_endfire_data },
};

static struct snddev_icodec_data snddev_dual_mic_spkr_endfire_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_dual_mic_endfire_tx",
	.copp_id = PRIMARY_I2S_TX,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
	.aic3254_id = VOICERECORD_IMIC,
	.aic3254_voc_id = CALL_UPLINK_IMIC_SPEAKER,
	.default_aic3254_id = VOICERECORD_IMIC,
};

static struct platform_device msm_spkr_dual_mic_endfire_device = {
	.name = "snddev_icodec",
	.id = 15,
	.dev = { .platform_data = &snddev_dual_mic_spkr_endfire_data },
};

static struct snddev_icodec_data snddev_hs_dual_mic_broadside_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_dual_mic_broadside_tx",
	.copp_id = PRIMARY_I2S_TX,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_sec_power,
	.pamp_off = msm_snddev_disable_dmic_sec_power,
};

static struct platform_device msm_hs_dual_mic_broadside_device = {
	.name = "snddev_icodec",
	.id = 21,
	.dev = { .platform_data = &snddev_hs_dual_mic_broadside_data },
};

static struct snddev_icodec_data snddev_spkr_dual_mic_broadside_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_dual_mic_broadside_tx",
	.copp_id = PRIMARY_I2S_TX,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_sec_power,
	.pamp_off = msm_snddev_disable_dmic_sec_power,
};

static struct platform_device msm_spkr_dual_mic_broadside_device = {
	.name = "snddev_icodec",
	.id = 18,
	.dev = { .platform_data = &snddev_spkr_dual_mic_broadside_data },
};

static struct snddev_hdmi_data snddev_hdmi_stereo_rx_data = {
	.capability = SNDDEV_CAP_RX ,
	.name = "hdmi_stereo_rx",
	.copp_id = HDMI_RX,
	.channel_mode = 0,
	.default_sample_rate = 48000,
};

static struct platform_device msm_snddev_hdmi_stereo_rx_device = {
	.name = "snddev_hdmi",
	.id = 0,
	.dev = { .platform_data = &snddev_hdmi_stereo_rx_data },
};

static struct snddev_mi2s_data snddev_mi2s_fm_tx_data = {
	.capability = SNDDEV_CAP_TX ,
	.name = "fmradio_stereo_tx",
	.copp_id = MI2S_TX,
	.channel_mode = 2, 
	.sd_lines = MI2S_SD3, 
	.sample_rate = 48000,
};

static struct platform_device msm_mi2s_fm_tx_device = {
	.name = "snddev_mi2s",
	.id = 0,
	.dev = { .platform_data = &snddev_mi2s_fm_tx_data },
};

static struct snddev_mi2s_data snddev_mi2s_fm_rx_data = {
	.capability = SNDDEV_CAP_RX ,
	.name = "fmradio_stereo_rx",
	.copp_id = MI2S_RX,
	.channel_mode = 2, 
	.sd_lines = MI2S_SD3, 
	.sample_rate = 48000,
};

static struct platform_device msm_mi2s_fm_rx_device = {
	.name = "snddev_mi2s",
	.id = 1,
	.dev = { .platform_data = &snddev_mi2s_fm_rx_data },
};

static struct snddev_icodec_data snddev_ifmradio_speaker_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_FM),
	.name = "fmradio_speaker_rx",
	.copp_id = 0,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = fm_speaker_enable,
	.pamp_off = fm_speaker_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = FM_OUT_SPEAKER,
	.aic3254_voc_id = FM_OUT_SPEAKER,
	.default_aic3254_id = FM_OUT_SPEAKER,
};

static struct platform_device msm_ifmradio_speaker_device = {
	.name = "snddev_icodec",
	.id = 9,
	.dev = { .platform_data = &snddev_ifmradio_speaker_data },
};

static struct snddev_icodec_data snddev_ifmradio_headset_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_FM),
	.name = "fmradio_headset_rx",
	.copp_id = 0,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = fm_headset_enable,
	.pamp_off = fm_headset_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = FM_OUT_HEADSET,
	.aic3254_voc_id = FM_OUT_HEADSET,
	.default_aic3254_id = FM_OUT_HEADSET,
};

static struct platform_device msm_ifmradio_headset_device = {
	.name = "snddev_icodec",
	.id = 10,
	.dev = { .platform_data = &snddev_ifmradio_headset_data },
};

static struct snddev_icodec_data snddev_headset_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "headset_mono_tx",
	.copp_id = PRIMARY_I2S_TX,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = ext_mic_enable,
	.pamp_off = ext_mic_disable,
	.aic3254_id = VOICERECOGNITION_EMIC,
	.aic3254_voc_id = CALL_UPLINK_EMIC_HEADSET,
	.default_aic3254_id = VOICERECORD_EMIC,
};

static struct platform_device msm_headset_mic_device = {
	.name = "snddev_icodec",
	.id = 33,
	.dev = { .platform_data = &snddev_headset_mic_data },
};

static struct snddev_icodec_data snddev_ihs_stereo_speaker_stereo_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_stereo_speaker_stereo_rx",
	.copp_id = 0,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = headset_speaker_enable,
	.pamp_off = headset_speaker_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = RING_HEADSET_SPEAKER,
	.aic3254_voc_id = RING_HEADSET_SPEAKER,
	.default_aic3254_id = RING_HEADSET_SPEAKER,
};

static struct platform_device msm_ihs_stereo_speaker_stereo_rx_device = {
	.name = "snddev_icodec",
	.id = 22,
	.dev = { .platform_data = &snddev_ihs_stereo_speaker_stereo_rx_data },
};

static struct snddev_ecodec_data snddev_bt_sco_earpiece_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_rx",
	.copp_id = PCM_RX,
	.channel_mode = 1,
};

static struct snddev_ecodec_data snddev_bt_sco_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_tx",
	.copp_id = PCM_TX,
	.channel_mode = 1,
};

struct platform_device msm_bt_sco_earpiece_device = {
	.name = "msm_snddev_ecodec",
	.id = 0,
	.dev = { .platform_data = &snddev_bt_sco_earpiece_data },
};

struct platform_device msm_bt_sco_mic_device = {
	.name = "msm_snddev_ecodec",
	.id = 1,
	.dev = { .platform_data = &snddev_bt_sco_mic_data },
};

static struct snddev_icodec_data snddev_itty_mono_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE | SNDDEV_CAP_TTY),
	.name = "tty_headset_mono_tx",
	.copp_id = PRIMARY_I2S_TX,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = ext_mic_enable,
	.pamp_off = ext_mic_disable,
	.aic3254_id = TTY_IN_FULL,
	.aic3254_voc_id = TTY_IN_FULL,
	.default_aic3254_id = TTY_IN_FULL,
};

static struct platform_device msm_itty_mono_tx_device = {
	.name = "snddev_icodec",
	.id = 16,
	.dev = { .platform_data = &snddev_itty_mono_tx_data },
};

static struct snddev_icodec_data snddev_itty_mono_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE | SNDDEV_CAP_TTY),
	.name = "tty_headset_mono_rx",
	.copp_id = PRIMARY_I2S_RX,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = headset_enable,
	.pamp_off = headset_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = TTY_OUT_FULL,
	.aic3254_voc_id = TTY_OUT_FULL,
	.default_aic3254_id = TTY_OUT_FULL,
};

static struct platform_device msm_itty_mono_rx_device = {
	.name = "snddev_icodec",
	.id = 17,
	.dev = { .platform_data = &snddev_itty_mono_rx_data },
};

static struct snddev_virtual_data snddev_uplink_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "uplink_rx",
	.copp_id = VOICE_PLAYBACK_TX,
};

static struct platform_device msm_uplink_rx_device = {
	.name = "snddev_virtual",
	.dev = { .platform_data = &snddev_uplink_rx_data },
};

static struct snddev_icodec_data snddev_ihs_mono_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_mono_rx",
	.copp_id = 0,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = headset_enable,
	.pamp_off = headset_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = PLAYBACK_HEADSET,
	.aic3254_voc_id = CALL_DOWNLINK_EMIC_HEADSET,
	.default_aic3254_id = PLAYBACK_HEADSET,
};

static struct platform_device msm_headset_mono_ab_cpls_device = {
	.name = "snddev_icodec",
	.id = 35,  
	.dev = { .platform_data = &snddev_ihs_mono_rx_data },
};

static struct snddev_icodec_data snddev_ihs_ispk_stereo_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_speaker_stereo_rx",
	.copp_id = 0,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = headset_speaker_enable,
	.pamp_off = headset_speaker_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = RING_HEADSET_SPEAKER,
	.aic3254_voc_id = RING_HEADSET_SPEAKER,
	.default_aic3254_id = RING_HEADSET_SPEAKER,
};

static struct platform_device msm_iheadset_ispeaker_rx_device = {
	.name = "snddev_icodec",
	.id = 36,  
	.dev = { .platform_data = &snddev_ihs_ispk_stereo_rx_data },
};

static struct snddev_icodec_data snddev_bmic_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "back_mic_tx",
	.copp_id = 1,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = back_mic_enable,
	.pamp_off = back_mic_disable,
	.aic3254_id = VOICERECORD_EMIC,
	.aic3254_voc_id = VOICERECORD_EMIC,
	.default_aic3254_id = VOICERECORD_EMIC,
};

static struct platform_device msm_bmic_tx_device = {
	.name = "snddev_icodec",
	.id = 50, 
	.dev = { .platform_data = &snddev_bmic_tx_data },
};

static struct snddev_icodec_data snddev_idual_mic_endfire_real_stereo_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "dual_mic_stereo_tx",
	.copp_id = PRIMARY_I2S_TX,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = stereo_mic_enable,
	.pamp_off = stereo_mic_disable,
	.aic3254_id = VIDEORECORD_IMIC,
	.aic3254_voc_id = VOICERECORD_EMIC, /* FIX ME */
	.default_aic3254_id = VIDEORECORD_IMIC,
};

static struct platform_device msm_real_stereo_tx_device = {
	.name = "snddev_icodec",
	.id = 26,  
	.dev = { .platform_data = &snddev_idual_mic_endfire_real_stereo_data },
};

static struct snddev_icodec_data snddev_iusb_headset_stereo_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "usb_headset_stereo_rx",
	.copp_id = 0,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = usb_headset_enable,
	.pamp_off = usb_headset_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = USB_AUDIO,
	.aic3254_voc_id = USB_AUDIO,
	.default_aic3254_id = USB_AUDIO,
};

static struct platform_device msm_iusb_headset_rx_device = {
	.name = "snddev_icodec",
	.id = 27,  
	.dev = { .platform_data = &snddev_iusb_headset_stereo_rx_data },
};

static struct snddev_icodec_data snddev_ispkr_mono_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "speaker_mono_rx",
	.copp_id = 0,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = speaker_enable,
	.pamp_off = speaker_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = PLAYBACK_SPEAKER,
	.aic3254_voc_id = CALL_DOWNLINK_IMIC_SPEAKER,
	.default_aic3254_id = PLAYBACK_SPEAKER,
};

static struct platform_device msm_ispkr_mono_device = {
	.name = "snddev_icodec",
	.id = 28,
	.dev = { .platform_data = &snddev_ispkr_mono_data },
};

static struct snddev_icodec_data snddev_camcorder_imic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "camcorder_mono_tx",
	.copp_id = 1,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = int_mic_enable,
	.pamp_off = int_mic_disable,
	.aic3254_id = VOICERECOGNITION_IMIC,
	.aic3254_voc_id = CALL_UPLINK_IMIC_RECEIVER,
	.default_aic3254_id = VOICERECOGNITION_IMIC,
};

static struct platform_device msm_camcorder_imic_device = {
	.name = "snddev_icodec",
	.id = 53,
	.dev = { .platform_data = &snddev_camcorder_imic_data },
};

static struct snddev_icodec_data snddev_camcorder_imic_stereo_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "camcorder_stereo_tx",
	.copp_id = PRIMARY_I2S_TX,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = stereo_mic_enable,
	.pamp_off = stereo_mic_disable,
	.aic3254_id = VIDEORECORD_IMIC,
	.aic3254_voc_id = VOICERECORD_EMIC, /* FIX ME */
	.default_aic3254_id = VIDEORECORD_IMIC,
};

static struct platform_device msm_camcorder_imic_stereo_device = {
	.name = "snddev_icodec",
	.id = 54,  
	.dev = { .platform_data = &snddev_camcorder_imic_stereo_data },
};

static struct snddev_icodec_data snddev_camcorder_imic_stereo_rev_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "camcorder_stereo_rev_tx",
	.copp_id = PRIMARY_I2S_TX,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = stereo_mic_enable,
	.pamp_off = stereo_mic_disable,
	.aic3254_id = VIDEORECORD_IMIC,
	.aic3254_voc_id = VOICERECORD_EMIC, /* FIX ME */
	.default_aic3254_id = VIDEORECORD_IMIC,
};

static struct platform_device msm_camcorder_imic_stereo_rev_device = {
	.name = "snddev_icodec",
	.id = 55,
	.dev = { .platform_data = &snddev_camcorder_imic_stereo_rev_data },
};

static struct snddev_icodec_data snddev_camcorder_headset_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "camcorder_headset_tx",
	.copp_id = PRIMARY_I2S_TX,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = ext_mic_enable,
	.pamp_off = ext_mic_disable,
	.aic3254_id = VOICERECOGNITION_EMIC,
	.aic3254_voc_id = CALL_UPLINK_EMIC_HEADSET,
	.default_aic3254_id = VOICERECORD_EMIC,
};

static struct platform_device msm_camcorder_headset_mic_device = {
	.name = "snddev_icodec",
	.id = 56,
	.dev = { .platform_data = &snddev_camcorder_headset_mic_data },
};

static struct snddev_icodec_data snddev_vr_iearpiece_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "vr_handset_mono_tx",
	.copp_id = 0,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = handset_enable,
	.pamp_off = handset_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = PLAYBACK_RECEIVER,
	.aic3254_voc_id = CALL_DOWNLINK_IMIC_RECEIVER,
	.default_aic3254_id = PLAYBACK_RECEIVER,
};

static struct platform_device msm_vr_iearpiece_device = {
	.name = "snddev_icodec",
	.id = 57,
	.dev = { .platform_data = &snddev_vr_iearpiece_data },
};

static struct snddev_icodec_data snddev_vr_headset_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "vr_headset_mono_tx",
	.copp_id = PRIMARY_I2S_TX,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = ext_mic_enable,
	.pamp_off = ext_mic_disable,
	.aic3254_id = VOICERECOGNITION_EMIC,
	.aic3254_voc_id = CALL_UPLINK_EMIC_HEADSET,
	.default_aic3254_id = VOICERECORD_EMIC,
};

static struct platform_device msm_vr_headset_mic_device = {
	.name = "snddev_icodec",
	.id = 58,
	.dev = { .platform_data = &snddev_vr_headset_mic_data },
};

static struct snddev_icodec_data snddev_ispkr_mono_alt_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "speaker_mono_alt_rx",
	.copp_id = 0,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = speaker_enable,
	.pamp_off = speaker_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = PLAYBACK_SPEAKER,
	.aic3254_voc_id = CALL_DOWNLINK_IMIC_SPEAKER,
	.default_aic3254_id = PLAYBACK_SPEAKER,
};

static struct platform_device msm_ispkr_mono_alt_device = {
	.name = "snddev_icodec",
	.id = 59,
	.dev = { .platform_data = &snddev_ispkr_mono_alt_data },
};

static struct snddev_icodec_data snddev_imic_note_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "imic_note_tx",
	.copp_id = 1,
	.channel_mode = 2,
	.default_sample_rate = 16000,
	.pamp_on = stereo_mic_enable,
	.pamp_off = stereo_mic_disable,
	.aic3254_id = VOICERECORD_IMIC,
	.aic3254_voc_id = CALL_UPLINK_IMIC_RECEIVER,
	.default_aic3254_id = VOICERECORD_IMIC,
};

static struct platform_device msm_imic_note_device = {
	.name = "snddev_icodec",
	.id = 60,
	.dev = { .platform_data = &snddev_imic_note_data },
};

static struct snddev_icodec_data snddev_ispkr_note_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "ispkr_note_rx",
	.copp_id = 0,
	.channel_mode = 2,
	.default_sample_rate = 16000,
	.pamp_on = speaker_enable,
	.pamp_off = speaker_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = PLAYBACK_SPEAKER,
	.aic3254_voc_id = CALL_DOWNLINK_IMIC_SPEAKER,
	.default_aic3254_id = PLAYBACK_SPEAKER,
};

static struct platform_device msm_ispkr_note_device = {
	.name = "snddev_icodec",
	.id = 61,
	.dev = { .platform_data = &snddev_ispkr_note_data },
};

static struct snddev_icodec_data snddev_emic_note_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "emic_note_tx",
	.copp_id = 1,
	.channel_mode = 1,
	.default_sample_rate = 16000,
	.pamp_on = ext_mic_enable,
	.pamp_off = ext_mic_disable,
	.aic3254_id = VOICERECORD_EMIC,
	.aic3254_voc_id = VOICERECORD_EMIC,
	.default_aic3254_id = VOICERECORD_EMIC,
};

static struct platform_device msm_emic_note_device = {
	.name = "snddev_icodec",
	.id = 62,
	.dev = { .platform_data = &snddev_emic_note_data },
};

static struct snddev_icodec_data snddev_ihs_note_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_note_rx",
	.copp_id = 0,
	.channel_mode = 2,
	.default_sample_rate = 16000,
	.pamp_on = headset_enable,
	.pamp_off = headset_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = PLAYBACK_HEADSET,
	.aic3254_voc_id = CALL_DOWNLINK_EMIC_HEADSET,
	.default_aic3254_id = PLAYBACK_HEADSET,
};

static struct platform_device msm_headset_note_device = {
	.name = "snddev_icodec",
	.id = 63,
	.dev = { .platform_data = &snddev_ihs_note_data },
};

static struct snddev_icodec_data snddev_ihs_mono_speaker_mono_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_mono_speaker_mono_rx",
	.copp_id = 0,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = headset_speaker_enable,
	.pamp_off = headset_speaker_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = RING_HEADSET_SPEAKER,
	.aic3254_voc_id = RING_HEADSET_SPEAKER,
	.default_aic3254_id = RING_HEADSET_SPEAKER,
};

static struct platform_device msm_ihs_mono_speaker_mono_rx_device = {
	.name = "snddev_icodec",
	.id = 64,
	.dev = { .platform_data = &snddev_ihs_mono_speaker_mono_rx_data },
};

static struct snddev_icodec_data snddev_fm_ihs_mono_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "fm_headset_mono_tx",
	.copp_id = PRIMARY_I2S_TX,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = NULL,
	.pamp_off = NULL,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
};

static struct platform_device msm_fm_ihs_mono_tx_device = {
	.name = "snddev_icodec",
	.id = 65,
	.dev = { .platform_data = &snddev_fm_ihs_mono_tx_data },
};

static struct snddev_icodec_data snddev_beats_ihs_stereo_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "beats_headset_stereo_rx",
	.copp_id = 0,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = headset_enable,
	.pamp_off = headset_disable,
	.voltage_on = voltage_on,
	.voltage_off = voltage_off,
	.aic3254_id = PLAYBACK_HEADSET,
	.aic3254_voc_id = CALL_DOWNLINK_EMIC_HEADSET,
	.default_aic3254_id = PLAYBACK_HEADSET,
};

static struct platform_device msm_beats_headset_stereo_device = {
	.name = "snddev_icodec",
	.id = 66,
	.dev = { .platform_data = &snddev_beats_ihs_stereo_rx_data },
};

static struct snddev_icodec_data snddev_beats_headset_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "beats_headset_mono_tx",
	.copp_id = PRIMARY_I2S_TX,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = ext_mic_enable,
	.pamp_off = ext_mic_disable,
	.aic3254_id = VOICERECOGNITION_EMIC,
	.aic3254_voc_id = CALL_UPLINK_EMIC_HEADSET,
	.default_aic3254_id = VOICERECORD_EMIC,
};

static struct platform_device msm_beats_headset_mic_device = {
	.name = "snddev_icodec",
	.id = 67,
	.dev = { .platform_data = &snddev_beats_headset_mic_data },
};

#ifdef CONFIG_DEBUG_FS
static void snddev_hsed_config_modify_setting(int type)
{
	struct platform_device *device;
	struct snddev_icodec_data *icodec_data;

	device = &msm_headset_stereo_device;
	icodec_data = (struct snddev_icodec_data *)device->dev.platform_data;

	if (icodec_data) {
		if (type == 1) {
			icodec_data->voltage_on = NULL;
			icodec_data->voltage_off = NULL;
		} else if (type == 2) {
			icodec_data->voltage_on = NULL;
			icodec_data->voltage_off = NULL;
		}
	}
}

static void snddev_hsed_config_restore_setting(void)
{
	struct platform_device *device;
	struct snddev_icodec_data *icodec_data;

	device = &msm_headset_stereo_device;
	icodec_data = (struct snddev_icodec_data *)device->dev.platform_data;

	if (icodec_data) {
		icodec_data->voltage_on = msm_snddev_voltage_on;
		icodec_data->voltage_off = msm_snddev_voltage_off;
	}
}

static ssize_t snddev_hsed_config_debug_write(struct file *filp,
	const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	char *lb_str = filp->private_data;
	char cmd;

	if (get_user(cmd, ubuf))
		return -EFAULT;

	if (!strcmp(lb_str, "msm_hsed_config")) {
		switch (cmd) {
		case '0':
			snddev_hsed_config_restore_setting();
			break;

		case '1':
			snddev_hsed_config_modify_setting(1);
			break;

		case '2':
			snddev_hsed_config_modify_setting(2);
			break;

		default:
			break;
		}
	}
	return cnt;
}

static int snddev_hsed_config_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static const struct file_operations snddev_hsed_config_debug_fops = {
	.open = snddev_hsed_config_debug_open,
	.write = snddev_hsed_config_debug_write
};
#endif

static struct platform_device *snd_devices_common[] __initdata = {
	&msm_uplink_rx_device,
};

static struct platform_device *snd_devices_pyramid[] __initdata = {
	&msm_iearpiece_device,
	&msm_imic_device,
	&msm_ispkr_stereo_device,
	&msm_snddev_hdmi_stereo_rx_device,
	&msm_headset_mic_device,
	&msm_ispkr_mic_device,
	&msm_bt_sco_earpiece_device,
	&msm_bt_sco_mic_device,
	&msm_headset_stereo_device,
	&msm_itty_mono_tx_device,
	&msm_itty_mono_rx_device,
	&msm_mi2s_fm_tx_device,
	&msm_mi2s_fm_rx_device,
	&msm_ifmradio_speaker_device,
	&msm_ifmradio_headset_device,
	&msm_hs_dual_mic_endfire_device,
	&msm_spkr_dual_mic_endfire_device,
	&msm_hs_dual_mic_broadside_device,
	&msm_spkr_dual_mic_broadside_device,
	&msm_ihs_stereo_speaker_stereo_rx_device,
	&msm_headset_mono_ab_cpls_device,
	&msm_iheadset_ispeaker_rx_device,
	&msm_bmic_tx_device,
	&msm_anc_headset_device,
	&msm_real_stereo_tx_device,
	&msm_ihac_device,
	&msm_nomic_headset_tx_device,
	&msm_nomic_headset_stereo_device,
	&msm_iusb_headset_rx_device,
	&msm_ispkr_mono_device,
	&msm_camcorder_imic_device,
	&msm_camcorder_imic_stereo_device,
	&msm_camcorder_imic_stereo_rev_device,
	&msm_camcorder_headset_mic_device,
	&msm_vr_iearpiece_device,
	&msm_vr_headset_mic_device,
	&msm_ispkr_mono_alt_device,
	&msm_imic_note_device,
	&msm_ispkr_note_device,
	&msm_emic_note_device,
	&msm_headset_note_device,
	&msm_ihs_mono_speaker_mono_rx_device,
	&msm_fm_ihs_mono_tx_device,
	&msm_beats_headset_stereo_device,
	&msm_beats_headset_mic_device,
};

void htc_8x60_register_analog_ops(struct q6v2audio_analog_ops *ops)
{
	audio_ops = ops;
}

void __init msm_snddev_init(void)
{
	int i;
	int dev_id;

	atomic_set(&preg_ref_cnt, 0);

	platform_add_devices(snd_devices_pyramid, ARRAY_SIZE(snd_devices_pyramid));

	for (i = 0, dev_id = 0; i < ARRAY_SIZE(snd_devices_common); i++)
		snd_devices_common[i]->id = dev_id++;

	platform_add_devices(snd_devices_common,
		ARRAY_SIZE(snd_devices_common));

#ifdef CONFIG_DEBUG_FS
	debugfs_hsed_config = debugfs_create_file("msm_hsed_config",
				S_IFREG | S_IRUGO, NULL,
		(void *) "msm_hsed_config", &snddev_hsed_config_debug_fops);
#endif
}
