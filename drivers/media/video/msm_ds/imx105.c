/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include <mach/camera.h>
#include <linux/debugfs.h>
#include "imx105.h"

//add msm_rotator_control_status
#undef MSM_ROTATOR_IOCTL_CHECK

#define REG_GROUPED_PARAMETER_HOLD			0x0104
#define GROUPED_PARAMETER_HOLD_OFF			0x00
#define GROUPED_PARAMETER_HOLD				0x01
#define REG_MODE_SELECT						0x0100
#define MODE_SELECT_STANDBY_MODE			0x00
#define MODE_SELECT_STREAM					0x01
/* Integration Time */
#define REG_COARSE_INTEGRATION_TIME_HI		0x0202
#define REG_COARSE_INTEGRATION_TIME_LO		0x0203
/* Gain */
#define REG_ANALOGUE_GAIN_CODE_GLOBAL_HI	0x0204
#define REG_ANALOGUE_GAIN_CODE_GLOBAL_LO	0x0205

/* mode setting */
#define REG_FRAME_LENGTH_LINES_HI   0x0340
#define REG_FRAME_LENGTH_LINES_LO	0x0341

// LGE_CAMERA_S : Fast AF - jonghwan.ko@lge.com
#ifdef USE_LG_fast_af
#define	IMX105_STEPS_NEAR_TO_CLOSEST_INF		32 //32 sungmin.woo
#define	IMX105_TOTAL_STEPS_NEAR_TO_FAR			32 //32 sungmin.woo
#else
#define	IMX105_STEPS_NEAR_TO_CLOSEST_INF		32 //32 sungmin.woo
#define	IMX105_TOTAL_STEPS_NEAR_TO_FAR			32 //32 sungmin.woo
#endif
// LGE_CAMERA_E : Fast AF - jonghwan.ko@lge.com

#define IMX105_AF_SLAVE_ADDR	(0x0C >> 1)

#define AF_STEP_RESOLUTION_VAL	0x04
#define AF_STEP_RESOLUTION_SETTING (AF_STEP_RESOLUTION_VAL << 5)
#define AF_SINGLESTEP_TIME_SETTING 0x14 /*1 msec*/
#define AF_SLEW_RATE_SPEED_SETING 0x2
#define AF_RESONANCE_FREQ_VAL 0x15 /*150 Hz*/
#define AF_RESONANCE_FREQ_SETING (AF_RESONANCE_FREQ_VAL << 3)
#define AF_SSTS_RS_VAL (AF_SINGLESTEP_TIME_SETTING | AF_STEP_RESOLUTION_SETTING)
#define AF_SRSS_RFS_VAL (AF_SLEW_RATE_SPEED_SETING | AF_RESONANCE_FREQ_SETING)

/*/*Intelligent slew rate control*/
#define AF_ISRC_MODE_ADDR (0xC4)
/*Slew rate speed setting and resonance frequency setting*/
#define AF_SRSS_RFS_VALUE_ADDR (0xC8)
/*Intelligent slew rate control un-control  DAC code 1*/
#define AF_DIRECT_MODE_ADDR (0xD0)
/*Intelligent slew rate control un-control  DAC code 2*/
#define AF_STEP_MODE_ADDR (0xDC)
/*Single Step Time Setting and Resolution Setting in Step Mode*/
#define AF_SSTS_RS_VALUE_ADDR (0xE0)

// Start LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-24
#if 0
#define AF_MECH_INFINITY (0x03)
// LGE_CAMERA_S : Fast AF - jonghwan.ko@lge.com
#ifdef USE_LG_fast_af
uint16_t AF_OPTICAL_INFINITY;	//sungmin.woo max. 135~576, ave.183~460
#else
#define AF_OPTICAL_INFINITY (190)	//sungmin.woo original = 190
#endif
// LGE_CAMERA_E : Fast AF - jonghwan.ko@lge.com
#else
uint16_t af_infinity;
uint16_t af_optical_infinity;
#endif
// End LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-24
#define AF_DISABLE (0x00)

//NR Register : LGE_BSP_CAMERA::shchang@qualcomm.com
#define REG_2DNR_ENABLE			0x30D0
#define REG_2DNR_GLINKINGSWITCH	0x30CF
#define REG_NR_STRENGTH 		0x3640
#define REG_NR_MFILTER_LEVEL	0x3620

uint16_t imx105_step_position_table[IMX105_TOTAL_STEPS_NEAR_TO_FAR+1];
// LGE_CAMERA_S : Fast AF - jonghwan.ko@lge.com
#ifdef USE_LG_fast_af
uint16_t imx105_l_region_code_per_step = 9;
#else
uint16_t imx105_l_region_code_per_step = 12;
#endif
// LGE_CAMERA_E : Fast AF - jonghwan.ko@lge.com
static int cam_debug_init(void);
static struct dentry *debugfs_base;

#define LGIT_IEF_SWITCH

// Start LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com
#ifdef SHUTTER_LAG_TEST
static int snapshot_flag;
#define SHUTTER_REGSET_FIRST	0
#define SHUTTER_REGSET_SECOND	1
#define SHUTTER_REGSET_STARTED	1
#define SHUTTER_REGSET_NONE		0
#endif
// End LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com
/*============================================================================
							 TYPE DECLARATIONS
============================================================================*/

/* 16bit address - 8 bit context register structure */
#define	IMX105_OFFSET	5
#define	Q8		0x00000100
#define	IMX105_DEFAULT_MASTER_CLK_RATE	24000000

/* Full	Size */
#define	IMX105_FULL_SIZE_WIDTH      3280
#define	IMX105_FULL_SIZE_HEIGHT		2464
#define	IMX105_HRZ_FULL_BLK_PIXELS	256
#define	IMX105_VER_FULL_BLK_LINES	46	//70 LGE_BSP_CAMERA::shchang@qualcom.com_0707 for flicker
#define	IMX105_FULL_SIZE_DUMMY_PIXELS	0
#define	IMX105_FULL_SIZE_DUMMY_LINES	0

/* Quarter Size	*/
#define	IMX105_QTR_SIZE_WIDTH	1640
#define	IMX105_QTR_SIZE_HEIGHT	1232
// Start LGE_BSP_CAMERA::john.park@lge.com 2011-04-27  For 1080p 
#define	IMX105_FHD_SIZE_WIDTH	3280
#define	IMX105_FHD_SIZE_HEIGHT	1232
#define	IMX105_HRZ_FHD_BLK_PIXELS	256
#define	IMX105_VER_FHD_BLK_LINES	36

#define	IMX105_QTR_SIZE_DUMMY_PIXELS	0
#define	IMX105_QTR_SIZE_DUMMY_LINES		0
#define	IMX105_HRZ_QTR_BLK_PIXELS	1896
#define	IMX105_VER_QTR_BLK_LINES	58 //38//34, LGE_BSP_CAMERA::shchang@qualcomm.com ,09-16 : increased Y Blank
// End LGE_BSP_CAMERA::john.park@lge.com 2011-04-27  For 1080p


struct imx105_work_t {
	struct work_struct work;
};

static struct imx105_work_t *imx105_sensorw;
static struct i2c_client *imx105_client;
static int32_t config_csi;


struct imx105_ctrl_t {
	const struct  msm_camera_sensor_info *sensordata;

	uint32_t sensormode;
	uint32_t fps_divider;	/* init to 1 * 0x00000400 */
	uint32_t pict_fps_divider;	/* init to 1 * 0x00000400 */
	uint16_t fps;

	int16_t curr_lens_pos;
	int16_t curr_step_pos;
	uint16_t my_reg_gain;
	uint32_t my_reg_line_count;
	uint16_t total_lines_per_frame;

	enum imx105_resolution_t prev_res;
	enum imx105_resolution_t pict_res;
	enum imx105_resolution_t curr_res;
	enum imx105_test_mode_t  set_test;

	unsigned short imgaddr;
};


static uint8_t imx105_delay_msecs_stdby = 5;
static uint16_t imx105_delay_msecs_stream = 10;

// Start LGE_BSP_CAMERA::shchang@qualcomm.com 2011-05-12
//uint8_t NR_ENABLE = 1;	//shchang@qualcomm.com : 2011-07-11
// End LGE_BSP_CAMERA::shchang@qualcomm.com 2011-05-12
 
static struct imx105_ctrl_t *imx105_ctrl;
static DECLARE_WAIT_QUEUE_HEAD(imx105_wait_queue);
DEFINE_MUTEX(imx105_mut);

/*=============================================================*/

static int imx105_i2c_rxdata(unsigned short saddr,
	unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
		{
			.addr  = saddr << 1,
			.flags = 0,
			.len   = 2,
			.buf   = rxdata,
		},
		{
			.addr  = saddr << 1,
			.flags = I2C_M_RD,
			.len   = 2,
			.buf   = rxdata,
		},
	};
	if (i2c_transfer(imx105_client->adapter, msgs, 2) < 0) {
		CDBG("imx105_i2c_rxdata failed!\n");
		return -EIO;
	}
	return 0;
}

static int32_t imx105_i2c_txdata(unsigned short saddr,
				unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr << 1,
			.flags = 0,
			.len = length,
			.buf = txdata,
		 },
	};
	if (i2c_transfer(imx105_client->adapter, msg, 1) < 0) {
		CDBG("imx105_i2c_txdata faild 0x%x\n", imx105_client->addr);
		return -EIO;
	}

	return 0;
}


static int32_t imx105_i2c_read(unsigned short raddr,
	unsigned short *rdata, int rlen)
{
	int32_t rc = 0;
	unsigned char buf[2];
	if (!rdata)
		return -EIO;
	memset(buf, 0, sizeof(buf));
	buf[0] = (raddr & 0xFF00) >> 8;
	buf[1] = (raddr & 0x00FF);
	rc = imx105_i2c_rxdata(imx105_client->addr, buf, rlen);
	if (rc < 0) {
		CDBG("imx105_i2c_read 0x%x failed!\n", raddr);
		return rc;
	}
	*rdata = (rlen == 2 ? buf[0] << 8 | buf[1] : buf[0]);
	return rc;
}

static int32_t imx105_i2c_write_b_sensor(unsigned short waddr, uint8_t bdata)
{
	int32_t rc = -EFAULT;
	unsigned char buf[3];
	memset(buf, 0, sizeof(buf));
	buf[0] = (waddr & 0xFF00) >> 8;
	buf[1] = (waddr & 0x00FF);
	buf[2] = bdata;
	rc = imx105_i2c_txdata(imx105_client->addr, buf, 3);
	if (rc < 0) {
		CDBG("i2c_write_b failed, addr = 0x%x, val = 0x%x!\n",
			waddr, bdata);
	}
	return rc;
}

static int32_t imx105_i2c_write_w_table(struct imx105_i2c_reg_conf const
					 *reg_conf_tbl, int num)
{
	int i;
	int32_t rc = -EIO;
	for (i = 0; i < num; i++) {
		rc = imx105_i2c_write_b_sensor(reg_conf_tbl->waddr,
			reg_conf_tbl->wdata);
		if (rc < 0)
			break;
		reg_conf_tbl++;
	}
	return rc;
}

static int16_t imx105_i2c_write_b_af(uint8_t baddr,
					uint8_t bdata)
{
	int32_t rc;
	unsigned char buf[2];
	memset(buf, 0, sizeof(buf));
	buf[0] = baddr;
	buf[1] = bdata;
	rc = imx105_i2c_txdata(IMX105_AF_SLAVE_ADDR, buf, 2);
	if (rc < 0)
		CDBG("afi2c_write failed, saddr = 0x%x addr = 0x%x, val =0x%x!",
			IMX105_AF_SLAVE_ADDR, baddr, bdata);
	return rc;
}

// Start LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-24
static int32_t imx105_i2c_read_w_eeprom(unsigned short raddr,
	unsigned short *rdata)
{
	int32_t rc = 0;
	unsigned char buf[2];
	if (!rdata)
		return -EIO;
	/*Read 2 bytes in sequence */
	/*Big Endian address:*/
	buf[0] = (raddr & 0x00FF);
	buf[1] = (raddr & 0xFF00) >> 8;
	rc = imx105_i2c_rxdata(IMX105_EEPROM_SLAVE_ADDR, buf, 1);
	if (rc < 0) {
		CDBG("imx105_i2c_read_eeprom 0x%x failed!\n", raddr);
		return rc;
	}
	*rdata = buf[0];
	raddr += 1;
	/* Read Second byte of data */
	buf[0] = (raddr & 0x00FF);
	buf[1] = (raddr & 0xFF00) >> 8;
	rc = imx105_i2c_rxdata(IMX105_EEPROM_SLAVE_ADDR, buf, 1);
	if (rc < 0) {
		CDBG("imx105_i2c_read_eeprom 0x%x failed!\n", raddr);
		return rc;
	}
	*rdata = (*rdata << 8) | buf[0];
	return rc;
}
// End LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-24

static void imx105_get_pict_fps(uint16_t fps, uint16_t *pfps)
{
	/* input fps is preview fps in Q8 format */
	uint16_t preview_frame_length_lines, snapshot_frame_length_lines;
	uint32_t divider;
	/* Total frame_length_lines for preview */
	preview_frame_length_lines = IMX105_QTR_SIZE_HEIGHT +
		IMX105_VER_QTR_BLK_LINES;
	/* Total frame_length_lines for snapshot */
	snapshot_frame_length_lines = IMX105_FULL_SIZE_HEIGHT +
		IMX105_VER_FULL_BLK_LINES;

	divider = preview_frame_length_lines * 0x00010000/
		snapshot_frame_length_lines;

	/*Verify PCLK settings and frame sizes.*/
	*pfps = (uint16_t) ((uint32_t)fps * divider / 0x10000);
	/* 2 is the ratio of no.of snapshot channels
	to number of preview channels */

}

// Start LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-17
static uint16_t imx105_get_prev_lines_pf(void)
{
	if (imx105_ctrl->prev_res == QTR_SIZE)
		return IMX105_QTR_SIZE_HEIGHT + IMX105_VER_QTR_BLK_LINES;
	else if(imx105_ctrl->pict_res == FHD_SIZE)	
		return IMX105_FHD_SIZE_HEIGHT + IMX105_VER_FHD_BLK_LINES;		
	else
		return IMX105_FULL_SIZE_HEIGHT + IMX105_VER_FULL_BLK_LINES;

}

static uint16_t imx105_get_prev_pixels_pl(void)
{
	if (imx105_ctrl->prev_res == QTR_SIZE)
		return IMX105_QTR_SIZE_WIDTH + IMX105_HRZ_QTR_BLK_PIXELS;
	else if(imx105_ctrl->pict_res == FHD_SIZE)	
		return IMX105_FHD_SIZE_WIDTH + IMX105_HRZ_FHD_BLK_PIXELS;		
	else
		return IMX105_FULL_SIZE_WIDTH + IMX105_HRZ_FULL_BLK_PIXELS;
}

static uint16_t imx105_get_pict_lines_pf(void)
{
		if (imx105_ctrl->pict_res == QTR_SIZE)
			return IMX105_QTR_SIZE_HEIGHT +
				IMX105_VER_QTR_BLK_LINES;
		else if(imx105_ctrl->pict_res == FHD_SIZE)
			return IMX105_FHD_SIZE_HEIGHT +
				IMX105_VER_FHD_BLK_LINES;			
		else
			return IMX105_FULL_SIZE_HEIGHT +
				IMX105_VER_FULL_BLK_LINES;
}

static uint16_t imx105_get_pict_pixels_pl(void)
{
	if (imx105_ctrl->pict_res == QTR_SIZE)
		return IMX105_QTR_SIZE_WIDTH +
			IMX105_HRZ_QTR_BLK_PIXELS;
	else if(imx105_ctrl->pict_res == FHD_SIZE)
		return (IMX105_FHD_SIZE_WIDTH +
			IMX105_HRZ_FHD_BLK_PIXELS);
	else
		return IMX105_FULL_SIZE_WIDTH +
			IMX105_HRZ_FULL_BLK_PIXELS;
}

static uint32_t imx105_get_pict_max_exp_lc(void)
{
	if (imx105_ctrl->pict_res == QTR_SIZE)
		return (IMX105_QTR_SIZE_HEIGHT +
			IMX105_VER_QTR_BLK_LINES)*24;
	else if(imx105_ctrl->pict_res == FHD_SIZE)
		return (IMX105_FHD_SIZE_HEIGHT +
			IMX105_VER_FHD_BLK_LINES)*24;		
	else
		return (IMX105_FULL_SIZE_HEIGHT +
			IMX105_VER_FULL_BLK_LINES)*24;
}

static int32_t imx105_set_fps(struct fps_cfg	*fps)
{
	uint16_t total_lines_per_frame;
	int32_t rc = 0;
	imx105_ctrl->fps_divider = fps->fps_div;
	imx105_ctrl->pict_fps_divider = fps->pict_fps_div;

	if (imx105_ctrl->curr_res  == QTR_SIZE)
		total_lines_per_frame = (uint16_t)(((IMX105_QTR_SIZE_HEIGHT +
		IMX105_VER_QTR_BLK_LINES) *
		imx105_ctrl->fps_divider) / 0x400);
	else if(imx105_ctrl->curr_res  == FHD_SIZE)		//shchang@qualcomm.com for FHD fixed FPS
		total_lines_per_frame = (uint16_t)(((IMX105_FHD_SIZE_HEIGHT +
		IMX105_VER_FHD_BLK_LINES) *
		imx105_ctrl->fps_divider) / 0x400);		
	else
		total_lines_per_frame = (uint16_t)(((IMX105_FULL_SIZE_HEIGHT +
			IMX105_VER_FULL_BLK_LINES) *
			imx105_ctrl->pict_fps_divider) / 0x400);

	rc = imx105_i2c_write_b_sensor(REG_FRAME_LENGTH_LINES_HI,
		((total_lines_per_frame & 0xFF00) >> 8));

	rc = imx105_i2c_write_b_sensor(REG_FRAME_LENGTH_LINES_LO,
		(total_lines_per_frame & 0x00FF));

	return rc;
}
// End LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-17


// Start LGE_BSP_CAMERA::shchang@qualcomm.com 2011-07-11
#if 0
static int32_t imx105_write_NoiseReduction(uint16_t gain)
{
	int32_t rc = -1;
	//standby
	rc = imx105_i2c_write_b_sensor(REG_MODE_SELECT, MODE_SELECT_STANDBY_MODE);
	if (rc < 0)
		return rc;
	msleep(imx105_delay_msecs_stdby);	

	rc = imx105_i2c_write_b_sensor(REG_GROUPED_PARAMETER_HOLD,
					GROUPED_PARAMETER_HOLD);
	if (rc < 0)
		return rc;	

	//real_gain = (float)(256.0 / (256.0 - (float)gain));
	printk("[QCTK] Gain = %d\n", gain);
	//Gain : 0~234
	//Just use Sony 2DNR option ~15 ~ 127
	#if 0
	printk("Func : NoiseReduction - Writing low light NR Value..\n");

	rc = imx105_i2c_write_b_sensor(REG_2DNR_ENABLE,0x79);	
	rc = imx105_i2c_write_b_sensor(REG_2DNR_GLINKINGSWITCH,0x0E);	
	rc = imx105_i2c_write_b_sensor(REG_NR_STRENGTH,0x7F);	//127
	rc = imx105_i2c_write_b_sensor(REG_NR_MFILTER_LEVEL,0x1E);	
	#else
	if(gain > 220)
	{
		printk("Func : NoiseReduction - Writing low light NR Value..\n");
		
		rc = imx105_i2c_write_b_sensor(REG_2DNR_ENABLE,0x79);	
		rc = imx105_i2c_write_b_sensor(REG_2DNR_GLINKINGSWITCH,0x0E);	
		rc = imx105_i2c_write_b_sensor(REG_NR_STRENGTH,0x52);	//7f, 82
		rc = imx105_i2c_write_b_sensor(REG_NR_MFILTER_LEVEL,0x1E);	
	}
	else if(gain > 200)
	{
		printk("Func : NoiseReduction - Writing indoor NR Value..\n");
		
		rc = imx105_i2c_write_b_sensor(REG_2DNR_ENABLE,0x79);	
		rc = imx105_i2c_write_b_sensor(REG_2DNR_GLINKINGSWITCH,0x0E);	
		rc = imx105_i2c_write_b_sensor(REG_NR_STRENGTH,0x29);	//52
		rc = imx105_i2c_write_b_sensor(REG_NR_MFILTER_LEVEL,0x1E);		
	}
	else if(gain > 150)
	{
		printk("Func : NoiseReduction - Writing bright2 light NR Value..\n");
		
		rc = imx105_i2c_write_b_sensor(REG_2DNR_ENABLE,0x79);	
		rc = imx105_i2c_write_b_sensor(REG_2DNR_GLINKINGSWITCH,0x0E);	
		rc = imx105_i2c_write_b_sensor(REG_NR_STRENGTH,0x1D);	//25, //29
		rc = imx105_i2c_write_b_sensor(REG_NR_MFILTER_LEVEL,0x1E);		
	}
	else if(gain > 80)
	{
		printk("Func : NoiseReduction - Writing bright1 light NR Value..\n");
		
		rc = imx105_i2c_write_b_sensor(REG_2DNR_ENABLE,0x79);	
		rc = imx105_i2c_write_b_sensor(REG_2DNR_GLINKINGSWITCH,0x0E);	
		rc = imx105_i2c_write_b_sensor(REG_NR_STRENGTH,0x10);	//15
		rc = imx105_i2c_write_b_sensor(REG_NR_MFILTER_LEVEL,0x1E);		
	}
	else 
	{
		printk("Func : NoiseReduction - Writing bright1 light NR Value..\n");
		
		rc = imx105_i2c_write_b_sensor(REG_2DNR_ENABLE,0x79);	
		rc = imx105_i2c_write_b_sensor(REG_2DNR_GLINKINGSWITCH,0x0E);	
		rc = imx105_i2c_write_b_sensor(REG_NR_STRENGTH,0x0F);	//15
		rc = imx105_i2c_write_b_sensor(REG_NR_MFILTER_LEVEL,0x1E);		
	}
	#endif	

	if(rc<0)
	{
		printk("Func : NoiseReduction - Writing NR Value is failed..\n");
		return rc;
	}

	//remove group hold
	rc = imx105_i2c_write_b_sensor(REG_GROUPED_PARAMETER_HOLD,
					GROUPED_PARAMETER_HOLD_OFF);
	if (rc < 0)
		return rc;	


	/* turn on streaming */
	rc = imx105_i2c_write_b_sensor(REG_MODE_SELECT, MODE_SELECT_STREAM);
	if (rc < 0)
		return rc;
	msleep(imx105_delay_msecs_stream);	

	return rc;

}
#endif
// End LGE_BSP_CAMERA::shchang@qualcomm.com 2011-07-11

// Start LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com 
#ifdef SHUTTER_LAG_TEST
static int32_t imx105_set_shutter_time(int reg_set, uint16_t gain, uint32_t line)
{
	int32_t rc = 0;
	uint8_t intg_time_msb,intg_time_lsb;
	
	if(reg_set == SHUTTER_REGSET_FIRST)
	{
		printk(" %s : SHUTTER_REGSET_FIRST \n",__func__);
		//register 1 set before setting gain 
		imx105_i2c_write_b_sensor(0x0104,0x01);
		imx105_i2c_write_b_sensor(0x034E,0x00);
		imx105_i2c_write_b_sensor(0x034F,0x01);
		imx105_i2c_write_b_sensor(0x3041,0x08);	
	
	}
	else
	{
		printk(" %s : SHUTTER_REGSET_SECOND \n",__func__);
		// register 1 set after setting gain
		rc = imx105_i2c_write_w_table(
			imx105_regs.shutter_tbl,
			imx105_regs.shuttertbl_size);		

		// delay for short frame EOF
		usleep(200);

		// compute shutter time using line value
		intg_time_msb = (uint8_t) ((line & 0xFF00) >> 8);
		intg_time_lsb = (uint8_t) (line & 0x00FF);	

		// set the shutter time 
		rc = imx105_i2c_write_b_sensor(REG_COARSE_INTEGRATION_TIME_HI,
						intg_time_msb);					
		rc = imx105_i2c_write_b_sensor(REG_COARSE_INTEGRATION_TIME_LO,
						intg_time_lsb);

		// register 2 set that separate 1080p preview and norma preview for CTS Test.
		if(imx105_ctrl->prev_res == RES_PREVIEW_1080)
		{
			
			CDBG("____  SHUTTER_REGSET PREVIEW MODE IS 1080P _____\n");
			// When preview mode is 1080p, it need a mode register set.
			rc = imx105_i2c_write_w_table(
				imx105_regs.shutter2_1080p_tbl,
				imx105_regs.shutter2_1080ptbl_size);									

		}
		else
		{
			CDBG("____  SHUTTER_REGSET PREVIEW MODE IS PREVIEW _____\n");
			
			rc = imx105_i2c_write_w_table(
				imx105_regs.shutter2_tbl,
				imx105_regs.shutter2tbl_size);									
		}
		
		// delay for capture frame
		msleep(2);

		// register 3 set
		imx105_i2c_write_b_sensor(0x30B1,0x03);
		
		snapshot_flag=SHUTTER_REGSET_NONE;		
		
	
	}

	return rc;
}
#endif
// End LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com 

static int32_t imx105_write_exp_gain(uint16_t gain, uint32_t line)
{
//	static uint16_t max_legal_gain  = 0x00E0;
	static uint16_t max_legal_gain  = 0x00EA;	
	uint8_t gain_msb, gain_lsb;
	uint8_t intg_time_msb, intg_time_lsb;
	uint8_t frame_length_line_msb, frame_length_line_lsb;
	uint16_t frame_length_lines;
	int32_t rc = -1;

// LGE_CAMERA_S : Fixed framerate - jonghwan.ko@lge.com
// Start LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-17
	CDBG("imx105_write_exp_gain : gain = %d line = %d", gain, line);
	if (imx105_ctrl->curr_res  == QTR_SIZE) {
		frame_length_lines = IMX105_QTR_SIZE_HEIGHT +
			IMX105_VER_QTR_BLK_LINES;
		frame_length_lines = frame_length_lines *
			imx105_ctrl->fps_divider / 0x400;
	}
// Start LGE_BSP_CAMERA::john.park@lge.com 2011-04-27  For 1080p 
	else if (imx105_ctrl->curr_res  == FHD_SIZE) {
		frame_length_lines = IMX105_FHD_SIZE_HEIGHT +
			IMX105_VER_FHD_BLK_LINES;
	frame_length_lines = frame_length_lines *
		imx105_ctrl->fps_divider / 0x400;
	}
// End LGE_BSP_CAMERA::john.park@lge.com 2011-04-27  For 1080p
	else {
		frame_length_lines = IMX105_FULL_SIZE_HEIGHT +
			IMX105_VER_FULL_BLK_LINES;
		frame_length_lines = frame_length_lines *
			imx105_ctrl->pict_fps_divider / 0x400;
	}
// End LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-17
// LGE_CAMERA_E : Fixed framerate - jonghwan.ko@lge.com

	if (line > (frame_length_lines - IMX105_OFFSET))
		frame_length_lines = line + IMX105_OFFSET;

	CDBG("imx105 setting line = %d\n", line);


	CDBG("imx105 setting frame_length_lines = %d\n",
					frame_length_lines);

	if (gain > max_legal_gain)
		/* range: 0 to 234(12x) 224(8x) */
		gain = max_legal_gain;

	CDBG("[QCTK]imx105_write_exp_gain : gain = %d line = %d", gain, line);	//LGE_BSP_CAMERA::shchang@qualcomm.com

	/* update gain registers */
	gain_msb = (uint8_t) ((gain & 0xFF00) >> 8);
	gain_lsb = (uint8_t) (gain & 0x00FF);

	frame_length_line_msb = (uint8_t) ((frame_length_lines & 0xFF00) >> 8);
	frame_length_line_lsb = (uint8_t) (frame_length_lines & 0x00FF);

	/* update line count registers */
	intg_time_msb = (uint8_t) ((line & 0xFF00) >> 8);
	intg_time_lsb = (uint8_t) (line & 0x00FF);

	rc = imx105_i2c_write_b_sensor(REG_GROUPED_PARAMETER_HOLD,
					GROUPED_PARAMETER_HOLD);
	if (rc < 0)
		return rc;

	
	CDBG("imx105 setting REG_ANALOGUE_GAIN_CODE_GLOBAL_HI = 0x%X\n",
					gain_msb);
	rc = imx105_i2c_write_b_sensor(REG_ANALOGUE_GAIN_CODE_GLOBAL_HI,
					gain_msb);
	if (rc < 0)
		return rc;
	CDBG("imx105 setting REG_ANALOGUE_GAIN_CODE_GLOBAL_LO = 0x%X\n",
					gain_lsb);
	rc = imx105_i2c_write_b_sensor(REG_ANALOGUE_GAIN_CODE_GLOBAL_LO,
					gain_lsb);
	if (rc < 0)
		return rc;

	CDBG("imx105 setting REG_FRAME_LENGTH_LINES_HI = 0x%X\n",
					frame_length_line_msb);
	rc = imx105_i2c_write_b_sensor(REG_FRAME_LENGTH_LINES_HI,
			frame_length_line_msb);
	if (rc < 0)
		return rc;

	CDBG("imx105 setting REG_FRAME_LENGTH_LINES_LO = 0x%X\n",
			frame_length_line_lsb);
	rc = imx105_i2c_write_b_sensor(REG_FRAME_LENGTH_LINES_LO,
			frame_length_line_lsb);
	if (rc < 0)
		return rc;
// Start LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com		
#ifdef SHUTTER_LAG_TEST
	if(snapshot_flag == SHUTTER_REGSET_NONE)
	{
#endif	
// End LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com
	CDBG("imx105 setting REG_COARSE_INTEGRATION_TIME_HI = 0x%X\n",
					intg_time_msb);
	rc = imx105_i2c_write_b_sensor(REG_COARSE_INTEGRATION_TIME_HI,
					intg_time_msb);
	if (rc < 0)
		return rc;

	CDBG("imx105 setting REG_COARSE_INTEGRATION_TIME_LO = 0x%X\n",
					intg_time_lsb);
	rc = imx105_i2c_write_b_sensor(REG_COARSE_INTEGRATION_TIME_LO,
					intg_time_lsb);
	if (rc < 0)
		return rc;
// Start LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com		
#ifdef SHUTTER_LAG_TEST
	}
#endif	
// End LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com
	// Start LGE_BSP_CAMERA::shchang@qualcomm.com 2011-07-11

	//real_gain = (float)(256.0 / (256.0 - (float)gain));
	CDBG("[QCTK] Gain = %d\n", gain);
	//Gain : 0~234
	//Just use Sony 2DNR option ~15 ~ 127
	if(gain > 220)
	{
		CDBG("Func : NoiseReduction - Writing low light NR Value..\n");
		
		//rc = imx105_i2c_write_b_sensor(REG_2DNR_ENABLE,0x79);	
		//rc = imx105_i2c_write_b_sensor(REG_2DNR_GLINKINGSWITCH,0x0E);	
		rc = imx105_i2c_write_b_sensor(REG_NR_STRENGTH,0x52);	//7f, 82
		//rc = imx105_i2c_write_b_sensor(REG_NR_MFILTER_LEVEL,0x1E);	
	}
	else if(gain > 200)
	{
		CDBG("Func : NoiseReduction - Writing indoor NR Value..\n");
		
		//rc = imx105_i2c_write_b_sensor(REG_2DNR_ENABLE,0x79);	
		//rc = imx105_i2c_write_b_sensor(REG_2DNR_GLINKINGSWITCH,0x0E);	
		rc = imx105_i2c_write_b_sensor(REG_NR_STRENGTH,0x29);	//52
		//rc = imx105_i2c_write_b_sensor(REG_NR_MFILTER_LEVEL,0x1E);		
	}
	else if(gain > 150)
	{
		CDBG("Func : NoiseReduction - Writing bright2 light NR Value..\n");
		
		//rc = imx105_i2c_write_b_sensor(REG_2DNR_ENABLE,0x79);	
		//rc = imx105_i2c_write_b_sensor(REG_2DNR_GLINKINGSWITCH,0x0E);	
		rc = imx105_i2c_write_b_sensor(REG_NR_STRENGTH,0x1D);	//25, //29
		//rc = imx105_i2c_write_b_sensor(REG_NR_MFILTER_LEVEL,0x1E);		
	}
	else if(gain > 80)
	{
		CDBG("Func : NoiseReduction - Writing bright1 light NR Value..\n");
		
		//rc = imx105_i2c_write_b_sensor(REG_2DNR_ENABLE,0x79);	
		//rc = imx105_i2c_write_b_sensor(REG_2DNR_GLINKINGSWITCH,0x0E);	
		rc = imx105_i2c_write_b_sensor(REG_NR_STRENGTH,0x10);	//15
		//rc = imx105_i2c_write_b_sensor(REG_NR_MFILTER_LEVEL,0x1E);		
	}
	else 
	{
		CDBG("Func : NoiseReduction - Writing bright1 light NR Value..\n");
		
		//rc = imx105_i2c_write_b_sensor(REG_2DNR_ENABLE,0x79);	
		//rc = imx105_i2c_write_b_sensor(REG_2DNR_GLINKINGSWITCH,0x0E);	
		rc = imx105_i2c_write_b_sensor(REG_NR_STRENGTH,0x0F);	//15
		//rc = imx105_i2c_write_b_sensor(REG_NR_MFILTER_LEVEL,0x1E);		
	}

	if(rc<0)
	{
		printk("Func : NoiseReduction - Writing NR Value is failed..\n");
		return rc;
	}
	// End LGE_BSP_CAMERA::shchang@qualcomm.com 2011-07-11 


	rc = imx105_i2c_write_b_sensor(REG_GROUPED_PARAMETER_HOLD,
					GROUPED_PARAMETER_HOLD_OFF);
	if (rc < 0)
		return rc;

	return rc;
}

static int32_t imx105_set_pict_exp_gain(uint16_t gain, uint32_t line)
{
	int32_t rc = 0;
	#if 0	//shchang@qualcomm.com : 2011-07-11
	// Start LGE_BSP_CAMERA::shchang@qualcomm.com 2011-05-12
	if(NR_ENABLE)
	{
		//Add Set NR
		rc = imx105_write_NoiseReduction(gain);
		if(rc<0) CDBG("Writing IMX105 Noise reduction is failed...\n");
	}
	// End LGE_BSP_CAMERA::shchang@qualcomm.com 2011-05-12 
	#endif	
// Start LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com 	
#ifdef SHUTTER_LAG_TEST	
	rc = imx105_set_shutter_time(SHUTTER_REGSET_FIRST, gain, line);	
	rc = imx105_write_exp_gain(gain, line);
	rc = imx105_set_shutter_time(SHUTTER_REGSET_SECOND, gain, line);
#else
	rc = imx105_write_exp_gain(gain, line);
#endif
// End LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com 
	return rc;
}

// Start LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-24
static int16_t imx105_af_init(void)
{
	int32_t rc = 0;

	uint8_t i;
	unsigned short vcmcodeinf, vcmcode10cm, vcmcode100cm, vcmcodesc, vcmcodeos;
	unsigned short epromaddr;

	/* Infinity AF Position*/
	epromaddr = 0x001A;
	rc = imx105_i2c_read_w_eeprom(epromaddr, &vcmcodeinf);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, epromaddr);
		return rc;
	}
	CDBG("%s the Infinity AF Position DAC Value is %d\n", __func__, vcmcodeinf);

	/* 1m AF Position*/
	epromaddr = 0x001C;
	rc = imx105_i2c_read_w_eeprom(epromaddr, &vcmcode100cm);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, epromaddr);
		return rc;
	}
	CDBG("%s the 1m AF Position DAC Value is %d\n", __func__, vcmcode100cm);

	/* 10cm AF Position*/
	epromaddr = 0x001E;
	rc = imx105_i2c_read_w_eeprom(epromaddr, &vcmcode10cm);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, epromaddr);
		return rc;
	}
	CDBG("%s the 10cm AF Position DAC Value is %d\n", __func__, vcmcode10cm);

	/* Start current*/
	epromaddr = 0x0020;
	rc = imx105_i2c_read_w_eeprom(epromaddr, &vcmcodesc);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, epromaddr);
		return rc;
	}
	CDBG("%s the vcm  Start current DAC Value is %d\n", __func__, vcmcodesc);

	/*Operating sensitivity*/
	epromaddr = 0x0022; 
	rc = imx105_i2c_read_w_eeprom(epromaddr, &vcmcodeos);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, epromaddr);
		return rc;
	}
	CDBG("%s the vcm Operating sensitivity DAC Value is %d\n", __func__, vcmcodeos);
	af_infinity = vcmcodesc;
	af_optical_infinity = vcmcodeinf;
	imx105_step_position_table[0] = af_optical_infinity;
	for (i = 1; i <= IMX105_TOTAL_STEPS_NEAR_TO_FAR; i++)
			imx105_step_position_table[i] =
			imx105_step_position_table[i-1] +
			imx105_l_region_code_per_step;

	rc = imx105_i2c_write_b_af(AF_SRSS_RFS_VALUE_ADDR,
				AF_SRSS_RFS_VAL);
	if (rc < 0)
		return rc;
	
	rc = imx105_i2c_write_b_af(AF_SSTS_RS_VALUE_ADDR,
				AF_SSTS_RS_VAL);
	if (rc < 0)
		return rc;
	
	imx105_ctrl->curr_lens_pos = 0;

	CDBG("%s imx105_ctrl->curr_lens_pos = %d\n", __func__, imx105_ctrl->curr_lens_pos);
	
	return rc;
}
// End LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-24

static int32_t imx105_move_focus(int direction,
	int32_t num_steps)
{

	int32_t rc = 0;
	int16_t step_direction, dest_lens_position, dest_step_position;
	uint8_t codeval_msb, codeval_lsb;
	if (direction == MOVE_NEAR)
		step_direction = 1;
	else if (direction == MOVE_FAR)
		step_direction = -1;
	else{
		pr_err("Illegal focus direction\n");
		return -EINVAL;
	}

	dest_step_position = imx105_ctrl->curr_step_pos +
			(step_direction * num_steps);
	if (dest_step_position < 0)
		dest_step_position = 0;
	else if (dest_step_position > IMX105_TOTAL_STEPS_NEAR_TO_FAR)
		dest_step_position = IMX105_TOTAL_STEPS_NEAR_TO_FAR;

	if (dest_step_position == imx105_ctrl->curr_step_pos) {
		CDBG("imx105_move_focus ==  imx105_ctrl->curr_step_pos:exit\n");
		return rc;
	}
	dest_lens_position = imx105_step_position_table[dest_step_position];
	CDBG("%s line %d index =[%d] = value = %d\n",
		 __func__, __LINE__, dest_step_position, dest_lens_position);
	if (imx105_ctrl->curr_lens_pos != dest_lens_position) {
		codeval_msb = (AF_ISRC_MODE_ADDR |
					((dest_lens_position & 0x0300)>>8));
		codeval_lsb = dest_lens_position & 0x00FF;
		rc = imx105_i2c_write_b_af(codeval_msb, codeval_lsb);
		if (rc < 0) {
			CDBG("imx105 I2C Failed line %d\n", __LINE__);
			return rc;
		}
		//usleep(10);  
		usleep(12); //LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-24
	}
	imx105_ctrl->curr_lens_pos = dest_lens_position;
	imx105_ctrl->curr_step_pos = dest_step_position;

	return rc;
}

// Start LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-24
static int32_t imx105_set_default_focus(void)
{
	int32_t rc = 0;
	CDBG("%s ==  enter\n", __func__);
	if(imx105_ctrl->curr_step_pos != 0) {
		rc = imx105_move_focus(MOVE_FAR,
			imx105_ctrl->curr_step_pos);
		msleep(30);
	} else {
	rc = imx105_i2c_write_b_af(AF_DIRECT_MODE_ADDR,
				af_infinity);
	if (rc < 0)
		return rc;
	usleep(50);
	CDBG("%s defaultfocus directmode\n", __func__);
	rc = imx105_i2c_write_b_af(AF_STEP_MODE_ADDR,
				af_optical_infinity);
	if (rc < 0)
		return rc;
	msleep(50);
	}
	CDBG("%s defaultfocus step mode\n", __func__);
	CDBG("%s ==  exit\n", __func__);
	imx105_ctrl->curr_lens_pos = af_optical_infinity;
	imx105_ctrl->curr_step_pos = 0;

	return rc;
}
// End LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-24

static int32_t imx105_sensor_setting(int update_type, int rt)
{
	int32_t rc = 0;
	struct msm_camera_csi_params imx105_csi_params;
	switch (update_type) {
	case REG_INIT:
		// Start LGE_BSP_CAMERA::john.park@lge.com 2011-04-27  For 1080p 
		if (rt == RES_PREVIEW || rt == RES_CAPTURE || rt == RES_PREVIEW_1080) {
		// End LGE_BSP_CAMERA::john.park@lge.com 2011-04-27  For 1080p
			CDBG("Sensor setting Init = %d\n", rt);
			cam_debug_init();
			/* reset fps_divider */
			imx105_ctrl->fps = 30 * Q8;
			imx105_ctrl->fps_divider = 1 * 0x400;
			CDBG("%s: %d\n", __func__, __LINE__);
			/* stop streaming */
			rc = imx105_i2c_write_b_sensor(REG_MODE_SELECT,
				MODE_SELECT_STANDBY_MODE);
			if (rc < 0)
				return rc;

			/*imx105_delay_msecs_stdby*/
			msleep(imx105_delay_msecs_stdby);
			rc = imx105_i2c_write_b_sensor(
				REG_GROUPED_PARAMETER_HOLD,
				GROUPED_PARAMETER_HOLD);
				if (rc < 0)
					return rc;

			#if 1		//shchang@qualcomm.com : 2011-07-11
			printk("Func : Init - Writing bright1 light NR Value..\n");

			rc = imx105_i2c_write_b_sensor(REG_2DNR_ENABLE,0x79);	
			rc = imx105_i2c_write_b_sensor(REG_2DNR_GLINKINGSWITCH,0x0E);	
			rc = imx105_i2c_write_b_sensor(REG_NR_STRENGTH,0x0F);	//15
			rc = imx105_i2c_write_b_sensor(REG_NR_MFILTER_LEVEL,0x1E);	

			if (rc < 0)
				return rc;
			#endif

			rc = imx105_i2c_write_w_table(imx105_regs.init_tbl,
				imx105_regs.inittbl_size);
			if (rc < 0)
				return rc;

			rc = imx105_i2c_write_b_sensor(
				REG_GROUPED_PARAMETER_HOLD,
				GROUPED_PARAMETER_HOLD_OFF);
			if (rc < 0)
				return rc;
			CDBG("%s: %d\n", __func__, __LINE__);
			/*imx105_delay_msecs_stdby*/
			msleep(imx105_delay_msecs_stdby);

			return rc;
		}
		break;

	case UPDATE_PERIODIC:
		// Start LGE_BSP_CAMERA::john.park@lge.com 2011-04-27  For 1080p 
// Start LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com		
#ifdef SHUTTER_LAG_TEST
		if (rt == RES_PREVIEW ||  rt == RES_PREVIEW_1080) {
#else
		if (rt == RES_PREVIEW || rt == RES_CAPTURE || rt == RES_PREVIEW_1080) {
#endif		
// End LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com
		// End LGE_BSP_CAMERA::john.park@lge.com 2011-04-27  For 1080p
			CDBG("%s: %d\n", __func__, __LINE__);
//Start : LGE BSP_CAMERA shchang@qualcomm.com 09-19- More stable MIPI Sequence

			/* 1. Stop streaming */
			rc = imx105_i2c_write_b_sensor(REG_MODE_SELECT,
				MODE_SELECT_STANDBY_MODE);
			if (rc < 0)
				return rc;

			/* 2. Delay for stopping streaming */
			msleep(imx105_delay_msecs_stdby);

			rc = imx105_i2c_write_b_sensor(
				REG_GROUPED_PARAMETER_HOLD,
				GROUPED_PARAMETER_HOLD);
				if (rc < 0)
					return rc;

			/* 3. Write sensor setting */
			/* write mode settings */
			if (rt == RES_PREVIEW) {
// Start LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com			
#ifdef SHUTTER_LAG_TEST
				snapshot_flag = SHUTTER_REGSET_NONE;
#endif
// End LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com

				rc = imx105_i2c_write_w_table(
					imx105_regs.prev_tbl,
					imx105_regs.prevtbl_size);
				if (rc < 0)
					return rc;
			}
			// Start LGE_BSP_CAMERA::john.park@lge.com 2011-04-27  For 1080p 
			else if (rt == RES_PREVIEW_1080) {
				rc = imx105_i2c_write_w_table(
					imx105_regs.prev_1080_tbl,
					imx105_regs.prevtbl_1080_size);
				if (rc < 0)
					return rc;
			} 
			// End LGE_BSP_CAMERA::john.park@lge.com 2011-04-27  For 1080p
			else {
				rc = imx105_i2c_write_w_table(
					imx105_regs.snap_tbl,
					imx105_regs.snaptbl_size);
				if (rc < 0)
					return rc;
			}

			rc = imx105_i2c_write_b_sensor(
				REG_GROUPED_PARAMETER_HOLD,
				GROUPED_PARAMETER_HOLD_OFF);
			if (rc < 0)
				return rc;

			/* 4. CSID/CSI - PHY registers configuration*/
			/* config mipi csi controller */
			if (config_csi == 0) {
				imx105_csi_params.lane_cnt = 2;
				imx105_csi_params.data_format = CSI_10BIT;
				imx105_csi_params.lane_assign = 0xe4;
				imx105_csi_params.dpcm_scheme = 0;
				imx105_csi_params.settle_cnt = 0x14;

				rc = msm_camio_csi_config(&imx105_csi_params);
				if (rc < 0)
					CDBG("config csi controller failed\n");

				msleep(imx105_delay_msecs_stream);
				config_csi = 1;
			}

			/* 5. stop streaming */
			CDBG("IMX105 Turn on streaming\n");

			/* turn on streaming */
			rc = imx105_i2c_write_b_sensor(REG_MODE_SELECT,
				MODE_SELECT_STREAM);
			if (rc < 0)
				return rc;
			msleep(imx105_delay_msecs_stream);
		}
//End : LGE BSP_CAMERA : shchang@qualcomm.com 09-19
// Start LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com		
#ifdef SHUTTER_LAG_TEST		
		else if(rt == RES_CAPTURE)
		{
				snapshot_flag = SHUTTER_REGSET_STARTED;
		}
#endif		
// End LGE_BSP_CAMERA : shutter time reg table - jonghwan.ko@lge.com
		
		break;
	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

static int32_t imx105_video_config(int mode)
{

	int32_t	rc = 0;
	/* change sensor resolution	if needed */
	if (imx105_ctrl->prev_res == QTR_SIZE)
		rc = imx105_sensor_setting(
		UPDATE_PERIODIC, RES_PREVIEW);
	// Start LGE_BSP_CAMERA::john.park@lge.com 2011-04-27  For 1080p 
	else if (imx105_ctrl->prev_res == FHD_SIZE)
		rc = imx105_sensor_setting(
		UPDATE_PERIODIC, RES_PREVIEW_1080);
	// End LGE_BSP_CAMERA::john.park@lge.com 2011-04-27  For 1080p
	else
		rc = imx105_sensor_setting(
		UPDATE_PERIODIC, RES_CAPTURE);

	if (rc < 0)
		return rc;

	imx105_ctrl->curr_res = imx105_ctrl->prev_res;
	imx105_ctrl->sensormode = mode;
	return rc;
}

static int32_t imx105_snapshot_config(int mode)
{
	int32_t rc = 0;
	/* change sensor resolution if needed */
	if (imx105_ctrl->curr_res != imx105_ctrl->pict_res) {
		if (imx105_ctrl->pict_res == QTR_SIZE)
			rc = imx105_sensor_setting(
			UPDATE_PERIODIC, RES_PREVIEW);
		else
			rc = imx105_sensor_setting(
			UPDATE_PERIODIC, RES_CAPTURE);

		if (rc < 0)
			return rc;
}
	imx105_ctrl->curr_res = imx105_ctrl->pict_res;
	imx105_ctrl->sensormode = mode;
	return rc;
}

static int32_t imx105_raw_snapshot_config(int mode)
{
	int32_t rc = 0;
	/* change sensor resolution if needed */
	if (imx105_ctrl->curr_res != imx105_ctrl->pict_res) {
		if (imx105_ctrl->pict_res == QTR_SIZE)
			rc = imx105_sensor_setting(
			UPDATE_PERIODIC, RES_PREVIEW);
		else
			rc = imx105_sensor_setting(
			UPDATE_PERIODIC, RES_CAPTURE);

		if (rc < 0)
			return rc;

	}
	imx105_ctrl->curr_res = imx105_ctrl->pict_res;
	imx105_ctrl->sensormode = mode;
	return rc;
}


static int32_t imx105_set_sensor_mode(int mode,
	int res)
{
	int32_t rc = 0;
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		imx105_ctrl->prev_res = res;
		rc = imx105_video_config(mode);
		break;
	case SENSOR_SNAPSHOT_MODE:
		imx105_ctrl->pict_res = res;
		rc = imx105_snapshot_config(mode);
		break;
	case SENSOR_RAW_SNAPSHOT_MODE:
		imx105_ctrl->pict_res = res;
		rc = imx105_raw_snapshot_config(mode);
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}
static int32_t imx105_power_down(void)
{
	// Start LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-24
	if(imx105_ctrl->curr_lens_pos != 0) {
		imx105_i2c_write_b_af(AF_ISRC_MODE_ADDR,
			af_infinity);
		msleep(200);
	}
	// End LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-24
	imx105_i2c_write_b_af(AF_DISABLE,
			AF_DISABLE);
	imx105_i2c_write_b_sensor(REG_MODE_SELECT,
		MODE_SELECT_STANDBY_MODE);
	msleep(imx105_delay_msecs_stdby);
	return 0;

}


static int imx105_probe_init_done(const struct msm_camera_sensor_info *data)
{
	gpio_set_value_cansleep(data->sensor_reset, 0);
	gpio_free(data->sensor_reset);
// Start LGE_BSP_CAMERA::john.park@lge.com 2011-06-03  separation of camera power
	imx105_ctrl->sensordata->pdata->camera_power_off();
// End LGE_BSP_CAMERA::john.park@lge.com 2011-06-03  separation of camera power	
	return 0;
}

// Start LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-24
static int imx105_read_eeprom_data(struct sensor_cfg_data *cfg)
{
	int32_t rc = 0;
	uint16_t eepromdata = 0;
	//uint8_t addr = 0;
	uint16_t addr = 0;
#ifdef USE_LG_fast_af
	uint8_t i;
	uint16_t fastaf_stepsize = 0;
	//uint16_t fastaf_totalstepmargin = 1;
	uint16_t fastaf_infinitymargin = 30;
	uint16_t fastaf_macromargin = 30;
#endif
	printk("[QCTK_EEPROM] Start reading EEPROM\n");

	//Start for Read debigging
	addr = 0x0000;
	rc = imx105_i2c_read_w_eeprom(addr, &eepromdata);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	
	printk("[QCTK_EEPROM] Product version = 0x%x\n", eepromdata);	

	//End for Read debugging
	
	addr = 0x0010;
	rc = imx105_i2c_read_w_eeprom(addr, &eepromdata);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	cfg->cfg.calib_info.r_over_g = eepromdata;
	printk("[QCTK_EEPROM] r_over_g = 0x%4x\n", cfg->cfg.calib_info.r_over_g);	

	addr = 0x0012;
	rc = imx105_i2c_read_w_eeprom(addr, &eepromdata);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	cfg->cfg.calib_info.b_over_g = eepromdata;
	printk("[QCTK_EEPROM] b_over_g = 0x%4x\n", cfg->cfg.calib_info.b_over_g);	


	addr = 0x0014;
	rc = imx105_i2c_read_w_eeprom(addr, &eepromdata);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	cfg->cfg.calib_info.gr_over_gb = eepromdata;

#ifdef USE_LG_fast_af
	// infinity af position
	addr = 0x001A;
	rc = imx105_i2c_read_w_eeprom(addr, &eepromdata);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	cfg->cfg.calib_info.af_pos_inf = eepromdata;
	printk("[QCTK_EEPROM] af_pos_inf = %d\n", cfg->cfg.calib_info.af_pos_inf);	
	
	// 1m af position
	addr = 0x1C; // sungmin.woo
	rc = imx105_i2c_read_w_eeprom(addr, &eepromdata);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	cfg->cfg.calib_info.af_pos_1m = eepromdata;
	printk("[QCTK_EEPROM] af_pos_1m = %d\n", cfg->cfg.calib_info.af_pos_1m);	
	
	// 100mm(macro) af position
	addr = 0x1E; // sungmin.woo let's use this for 100mm af position
	rc = imx105_i2c_read_w_eeprom(addr, &eepromdata);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	cfg->cfg.calib_info.stroke_amt = eepromdata;
	printk("[QCTK_EEPROM] macro pos = %d\n", cfg->cfg.calib_info.stroke_amt);	

	//remaining parameter

	cfg->cfg.calib_info.macro_2_inf =  cfg->cfg.calib_info.stroke_amt - cfg->cfg.calib_info.af_pos_inf;
	printk("[QCTK_EEPROM] macro_2_inf = %d\n", cfg->cfg.calib_info.macro_2_inf);	
	cfg->cfg.calib_info.inf_2_macro = cfg->cfg.calib_info.macro_2_inf;
	printk("[QCTK_EEPROM] inf_2_macro = %d\n", cfg->cfg.calib_info.inf_2_macro);	

	
	printk("Reset focus range from eeprom for fast af\n");	
	printk("**********************************\n");
	fastaf_infinitymargin = (uint16_t)(cfg->cfg.calib_info.af_pos_inf/10);
	fastaf_macromargin = fastaf_infinitymargin;
	printk("fastaf_infinitymargin = %d\n", fastaf_infinitymargin);	
	fastaf_stepsize =  (uint16_t)((cfg->cfg.calib_info.macro_2_inf + fastaf_infinitymargin + fastaf_macromargin )/(IMX105_TOTAL_STEPS_NEAR_TO_FAR));
	imx105_step_position_table[0] = cfg->cfg.calib_info.af_pos_inf - fastaf_infinitymargin;
	imx105_l_region_code_per_step = fastaf_stepsize;
	printk("imx105_l_region_code_per_step = %d\n", fastaf_stepsize);	
	
	for (i = 1; i <= IMX105_TOTAL_STEPS_NEAR_TO_FAR; i++)
	{	
		imx105_step_position_table[i] = imx105_step_position_table[i-1] + imx105_l_region_code_per_step;
	}

	for(i = 0; i <= IMX105_TOTAL_STEPS_NEAR_TO_FAR; i++)
	{	
		printk("imx105_step_position_table[%d] = [%d]\n",i, imx105_step_position_table[i]);	
	}

	AF_OPTICAL_INFINITY = cfg->cfg.calib_info.af_pos_inf - fastaf_infinitymargin;
	//AF_OPTICAL_INFINITY = 0;
	printk("AF_OPTICAL_INFINITY = %d\n", AF_OPTICAL_INFINITY);	
	
	printk("**********************************\n");	

#else
	//: Infinite AF Position
	addr = 0x001A;
	rc = imx105_i2c_read_w_eeprom(addr, &eepromdata);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	cfg->cfg.calib_info.macro_2_inf = eepromdata;

	//: 1m AF Position
	addr = 0x001C;
	rc = imx105_i2c_read_w_eeprom(addr, &eepromdata);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	cfg->cfg.calib_info.inf_2_macro = eepromdata;

	// : 10cm AF Position
	addr = 0x001E;
	rc = imx105_i2c_read_w_eeprom(addr, &eepromdata);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	cfg->cfg.calib_info.stroke_amt = eepromdata;

	// : Start current
	addr = 0x0020;
	rc = imx105_i2c_read_w_eeprom(addr, &eepromdata);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	cfg->cfg.calib_info.af_pos_1m = eepromdata;

	// : Operating sensitivity
	addr = 0x0022;
	rc = imx105_i2c_read_w_eeprom(addr, &eepromdata);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	cfg->cfg.calib_info.af_pos_inf = eepromdata;
#endif
	return rc;
}
// End LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-24 


static int imx105_probe_init_sensor(const struct msm_camera_sensor_info *data)
{
	int32_t rc = 0;
	unsigned short chipidl, chipidh;
	CDBG("%s: %d\n", __func__, __LINE__);
	rc = gpio_request(data->sensor_reset, "imx105");
	CDBG(" imx105_probe_init_sensor\n");
	if (!rc) {
		CDBG("sensor_reset = %d\n", rc);
		CDBG(" imx105_probe_init_sensor 1\n");
		gpio_direction_output(data->sensor_reset, 0);
		msleep(50);
		CDBG(" imx105_probe_init_sensor 1\n");
		gpio_set_value_cansleep(data->sensor_reset, 1);
		msleep(50);
	} else {
		CDBG(" imx105_probe_init_sensor 2\n");
		goto init_probe_done;
	}
	msleep(20);
	/* 3. Read sensor Model ID: */
	rc = imx105_i2c_read(0x0000, &chipidh, 1);
	if (rc < 0) {
		CDBG(" imx105_probe_init_sensor 3\n");
		goto init_probe_fail;
	}
	rc = imx105_i2c_read(0x0001, &chipidl, 1);
	if (rc < 0) {
		CDBG(" imx105_probe_init_sensor 4\n");
		goto init_probe_fail;
	}
	CDBG("imx105 model_id = 0x%x  0x%x\n", chipidh, chipidl);
	/* 4. Compare sensor ID to IMX105 ID: */
	if (chipidh != 0x01 || chipidl != 0x05) {
		rc = -ENODEV;
		CDBG("imx105_probe_init_sensor fail chip id doesnot match\n");
		goto init_probe_fail;
	}
	goto init_probe_done;
	init_probe_fail:
	CDBG(" imx105_probe_init_sensor fails\n");
	imx105_probe_init_done(data);
	init_probe_done:
	CDBG(" imx105_probe_init_sensor finishes\n");
	return rc;
	}

#ifdef MSM_ROTATOR_IOCTL_CHECK
static int sensor_open = 0;

int  is_imx105_sensor_open(void)
{
	return sensor_open;
}
#endif

#ifdef LGIT_IEF_SWITCH
extern int mipi_lgit_lcd_ief_off(void);
extern int mipi_lgit_lcd_ief_on(void);
#endif

int imx105_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int32_t rc = 0;
	CDBG("%s: %d\n", __func__, __LINE__);
	imx105_ctrl = kzalloc(sizeof(struct imx105_ctrl_t), GFP_KERNEL);
	if (!imx105_ctrl) {
		CDBG("imx105_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}
	imx105_ctrl->fps_divider = 1 * 0x00000400;
	imx105_ctrl->pict_fps_divider = 1 * 0x00000400;
	imx105_ctrl->fps = 30 * Q8;
	imx105_ctrl->set_test = TEST_OFF;
	imx105_ctrl->prev_res = QTR_SIZE;
	imx105_ctrl->pict_res = FULL_SIZE;
	imx105_ctrl->curr_res = INVALID_SIZE;
	config_csi = 0;

	if (data)
		imx105_ctrl->sensordata = data;
	CDBG("%s: %d\n", __func__, __LINE__);

#ifdef MSM_ROTATOR_IOCTL_CHECK
	sensor_open = 1;
#endif

// Start LGE_BSP_CAMERA::john.park@lge.com 2011-06-03  separation of camera power
	data->pdata->camera_power_on();
	if (rc < 0) {
		printk(KERN_ERR "[ERROR]%s:failed to power on!\n", __func__);
		return rc;
	}
	msm_camio_clk_rate_set(IMX105_DEFAULT_MASTER_CLK_RATE);
	
	rc = imx105_probe_init_sensor(data);
	if (rc < 0) {
		CDBG("Calling imx105_sensor_open_init fail\n");
		goto probe_fail;
	}

#ifdef LGIT_IEF_SWITCH
	mipi_lgit_lcd_ief_off();
#endif
	
// End LGE_BSP_CAMERA::john.park@lge.com 2011-06-03  separation of camera power
	CDBG("%s: %d\n", __func__, __LINE__);
	rc = imx105_sensor_setting(REG_INIT, RES_PREVIEW);
	CDBG("%s: %d\n", __func__, __LINE__);
	if (rc < 0)
		goto init_fail;
	rc = imx105_af_init();
	if (rc < 0)
		goto init_fail;
	else
		goto init_done;
	probe_fail:
	CDBG("%s probe failed\n", __func__);
	kfree(imx105_ctrl);
	return rc;
	init_fail:
	CDBG(" imx105_sensor_open_init fail\n");
	CDBG("%s: %d\n", __func__, __LINE__);
	imx105_probe_init_done(data);
	kfree(imx105_ctrl);
	init_done:
	CDBG("%s: %d\n", __func__, __LINE__);
	CDBG("imx105_sensor_open_init done\n");
	return rc;
}

static int imx105_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&imx105_wait_queue);
	return 0;
}

static const struct i2c_device_id imx105_i2c_id[] = {
	{"imx105", 0},
	{ }
};

static int imx105_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		CDBG("i2c_check_functionality failed\n");
		goto probe_failure;
	}

	imx105_sensorw = kzalloc(sizeof(struct imx105_work_t), GFP_KERNEL);
	if (!imx105_sensorw) {
		CDBG("kzalloc failed.\n");
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, imx105_sensorw);
	imx105_init_client(client);
	imx105_client = client;

	msleep(50);

	CDBG("imx105_probe success! rc = %d\n", rc);
	return 0;

	probe_failure:
	CDBG("imx105_probe failed! rc = %d\n", rc);
	return rc;
}

static int __exit imx105_remove(struct i2c_client *client)
{
	struct imx105_work_t_t *sensorw = i2c_get_clientdata(client);
	free_irq(client->irq, sensorw);
	imx105_client = NULL;
	kfree(sensorw);
	return 0;
}

static struct i2c_driver imx105_i2c_driver = {
	.id_table = imx105_i2c_id,
	.probe  = imx105_i2c_probe,
	.remove = __exit_p(imx105_i2c_remove),
	.driver = {
		.name = "imx105",
	},
};

int imx105_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cdata;
	long   rc = 0;
	if (copy_from_user(&cdata,
		(void *)argp,
		sizeof(struct sensor_cfg_data)))
		return -EFAULT;
	mutex_lock(&imx105_mut);
	CDBG("imx105_sensor_config: cfgtype = %d\n",
	cdata.cfgtype);
		switch (cdata.cfgtype) {
		case CFG_GET_PICT_FPS:
			imx105_get_pict_fps(
				cdata.cfg.gfps.prevfps,
				&(cdata.cfg.gfps.pictfps));

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_GET_PREV_L_PF:
			cdata.cfg.prevl_pf =
			imx105_get_prev_lines_pf();

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_GET_PREV_P_PL:
			cdata.cfg.prevp_pl =
				imx105_get_prev_pixels_pl();

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_GET_PICT_L_PF:
			cdata.cfg.pictl_pf =
				imx105_get_pict_lines_pf();

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_GET_PICT_P_PL:
			cdata.cfg.pictp_pl =
				imx105_get_pict_pixels_pl();

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_GET_PICT_MAX_EXP_LC:
			cdata.cfg.pict_max_exp_lc =
				imx105_get_pict_max_exp_lc();

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_SET_FPS:
		case CFG_SET_PICT_FPS:
			rc = imx105_set_fps(&(cdata.cfg.fps));
			break;

		case CFG_SET_EXP_GAIN:
			rc =
				imx105_write_exp_gain(
					cdata.cfg.exp_gain.gain,
					cdata.cfg.exp_gain.line);
			break;

		case CFG_SET_PICT_EXP_GAIN:
			rc =
				imx105_set_pict_exp_gain(
				cdata.cfg.exp_gain.gain,
				cdata.cfg.exp_gain.line);
			break;

		case CFG_SET_MODE:
			rc = imx105_set_sensor_mode(cdata.mode,
					cdata.rs);
			break;

		case CFG_PWR_DOWN:
			rc = imx105_power_down();
			break;
		// Start LGE_BSP_CAMERA::shchang@qualcomm.com 2011-05-31			
		case CFG_GET_CALIB_DATA:
			rc = imx105_read_eeprom_data(&cdata);
			if (rc < 0)
				break;
			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(cdata)))
				rc = -EFAULT;
			break;			
		// End LGE_BSP_CAMERA::shchang@qualcomm.com 2011-05-31			
		case CFG_GET_AF_MAX_STEPS:
			cdata.max_steps = IMX105_STEPS_NEAR_TO_CLOSEST_INF;
			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;
		case CFG_MOVE_FOCUS:
			rc =
				imx105_move_focus(
				cdata.cfg.focus.dir,
				cdata.cfg.focus.steps);
			break;

		case CFG_SET_DEFAULT_FOCUS:
			rc =
				imx105_set_default_focus();
			break;

		case CFG_SET_EFFECT:
		default:
			rc = -EFAULT;
			break;
		}

	mutex_unlock(&imx105_mut);

	return rc;
}

static int imx105_sensor_release(void)
{
	int rc = -EBADF;
	mutex_lock(&imx105_mut);
	imx105_power_down();
	gpio_set_value_cansleep(imx105_ctrl->sensordata->sensor_reset,
		0);
	gpio_free(imx105_ctrl->sensordata->sensor_reset);

// Start LGE_BSP_CAMERA::john.park@lge.com 2011-06-03  separation of camera power
	imx105_ctrl->sensordata->pdata->camera_power_off();
// End LGE_BSP_CAMERA::john.park@lge.com 2011-06-03  separation of camera power
	
	kfree(imx105_ctrl);
	imx105_ctrl = NULL;
	CDBG("imx105_release completed\n");
	mutex_unlock(&imx105_mut);

#ifdef LGIT_IEF_SWITCH
	mipi_lgit_lcd_ief_on();
#endif

#ifdef MSM_ROTATOR_IOCTL_CHECK	
	sensor_open = 0;
	set_rotator_ctl_result(0);
#endif

	return rc;
}

static int imx105_sensor_probe(const struct msm_camera_sensor_info *info,
		struct msm_sensor_ctrl *s)
{
	int rc = 0;
	rc = i2c_add_driver(&imx105_i2c_driver);
	if (rc < 0 || imx105_client == NULL) {
		rc = -ENOTSUPP;
		goto probe_fail;
	}
	if (rc < 0)
		goto probe_fail;
	s->s_init = imx105_sensor_open_init;
	s->s_release = imx105_sensor_release;
	s->s_config  = imx105_sensor_config;
	s->s_camera_type = BACK_CAMERA_2D;
	s->s_mount_angle  = 90; // fix camera sensor orientation by john.park@lge.com

	return rc;

	probe_fail:
	CDBG("imx105_sensor_probe: SENSOR PROBE FAILS!\n");
	return rc;
}

static int __imx105_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, imx105_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __imx105_probe,
	.driver = {
		.name = "msm_camera_imx105",
		.owner = THIS_MODULE,
	},
};

static int __init imx105_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(imx105_init);

void imx105_exit(void)
{
	i2c_del_driver(&imx105_i2c_driver);
}


MODULE_DESCRIPTION("Sony 8 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");

static int imx105_set_af_codestep(void *data, u64 val)
{
	imx105_l_region_code_per_step = val;
	imx105_af_init();
	return 0;
}

static int imx105_get_af_codestep(void *data, u64 *val)
{
	*val = imx105_l_region_code_per_step;
	return 0;
}

static uint16_t imx105_linear_total_step = IMX105_TOTAL_STEPS_NEAR_TO_FAR;
static int imx105_set_linear_total_step(void *data, u64 val)
{
	imx105_linear_total_step = val;
	return 0;
}

static int imx105_af_linearity_test(void *data, u64 *val)
{
	int i = 0;

	imx105_set_default_focus();
	msleep(3000);
	for (i = 0; i < imx105_linear_total_step; i++) {
		imx105_move_focus(MOVE_NEAR, 1);
		CDBG("moved to index =[%d]\n", i);
		msleep(1000);
	}

	for (i = 0; i < imx105_linear_total_step; i++) {
		imx105_move_focus(MOVE_FAR, 1);
		CDBG("moved to index =[%d]\n", i);
		msleep(1000);
	}
	return 0;
}

static uint16_t imx105_step_val = IMX105_TOTAL_STEPS_NEAR_TO_FAR;
static uint8_t imx105_step_dir = MOVE_NEAR;
static int imx105_af_step_config(void *data, u64 val)
{
	imx105_step_val = val & 0xFFFF;
	imx105_step_dir = (val >> 16) & 0x1;
	return 0;
}

static int imx105_af_step(void *data, u64 *val)
{
	int i = 0;
	int dir = MOVE_NEAR;
	imx105_set_default_focus();
	if (imx105_step_dir == 1)
		dir = MOVE_FAR;

	// Start LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-24
	for (i = 0; i < imx105_step_val; i+=4) {
		imx105_move_focus(dir, 4);
		msleep(1000);
	}
	imx105_set_default_focus();
	// End LGE_BSP_CAMERA::shchang@qualcomm.com 2011-06-24
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(af_codeperstep, imx105_get_af_codestep,
			imx105_set_af_codestep, "%llu\n");

DEFINE_SIMPLE_ATTRIBUTE(af_linear, imx105_af_linearity_test,
			imx105_set_linear_total_step, "%llu\n");

DEFINE_SIMPLE_ATTRIBUTE(af_step, imx105_af_step,
			imx105_af_step_config, "%llu\n");

static int cam_debug_init(void)
{
	struct dentry *cam_dir;
	debugfs_base = debugfs_create_dir("sensor", NULL);
	if (!debugfs_base)
		return -ENOMEM;

	cam_dir = debugfs_create_dir("imx105", debugfs_base);
	if (!cam_dir)
		return -ENOMEM;

	if (!debugfs_create_file("af_codeperstep", S_IRUGO | S_IWUSR, cam_dir,
							 NULL, &af_codeperstep))
		return -ENOMEM;
	if (!debugfs_create_file("af_linear", S_IRUGO | S_IWUSR, cam_dir,
							 NULL, &af_linear))
		return -ENOMEM;
	if (!debugfs_create_file("af_step", S_IRUGO | S_IWUSR, cam_dir,
							 NULL, &af_step))
		return -ENOMEM;

	return 0;
}

