/* Copyright (c) 2010, PANTECH. All rights reserved.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
#include <mach/pmic.h>
#include "mv9335.h"

#define TEMP_POWERSEQ

/* MV9335 core register addresses */
#define REG_MV9335_HW_ID	0x00
#define REG_MV9335_HW_VER	0x01
#define REG_MV9335_MAJOR_VER	0x02
#define REG_MV9335_MINOR_VER	0x03
#define REG_MV9335_STATE	0x0e
#define REG_MV9335_SENSOR_ALIVE	0x2d

/* identify MV9335 firmware version */
#define MV9335_HW_ID		0x93
#define MV9335_HW_VER		0x35
#define MV9335_FW_MAJOR_VER	0x01
#define MV9335_FW_MINOR_VER	0x37
#define MV9335_FW_FNAME		"mv9335_mt9p013_02_70.h"
#define MV9335_FW_CSUM_MSB	0x2c
#define MV9335_FW_CSUM_LSB	0x18
#ifdef F_SKYCAM_MODEL_EF12S
#if 0
#define MV9335_FW_FNAME		"mv9335_mt9p013_02_43.h"
#define MV9335_FW_MAJOR_VER	0x02
#define MV9335_FW_MINOR_VER	0x2b
#define MV9335_FW_CSUM_MSB	0x08
#define MV9335_FW_CSUM_LSB	0x3c
#endif
#endif

/*
 * 02.28 : initial release for 1st IOT
 * 02.30 : set REG_MV9335_HW_VER just after system initialization
 * 02.31 : tune, fix flash GPIO control
 * 02.32 : set REG_MV9335_HW_VER after starting preview
 * 02.34 : fix flash in spot focus mode
 * 02.35 : fix ae/mwb
 * 02.36 : tune, fix AF speed
 * 02.37 : fix flash gpio
 * 02.38 : fix flash gpio and power-on sequence
 * 02.41 : tune, fix AF
 * 02.42 : tune, fusing test
 * 02.43 : tune, apply checksum
 */

#define MV9335_STATE_IDLE	0xbb
#define MV9335_STATE_DONE	0xbb

#define MV9335_I2C_RETRY	3
#define MV9335_I2C_MPERIOD	500

#define MV9335_FOCUS_RECT_AUTO	0
#define MV9335_FOCUS_RECT_USER	1

#define MV9335_CFG_SCLK		24000000

#ifdef F_SKYCAM_MODEL_EF12S
/* '1.8V_CAM_IO' is supplied by a switch, and this GPIO controls 
 * this switch. */
#define GPIO_MV9335_IO_VREG_EN	105
#endif

struct mv9335_work {
	struct work_struct work;
};

static struct mv9335_work *mv9335_sensorw;
static struct i2c_client *mv9335_client;

struct mv9335_ctrl_t {
	struct msm_camera_sensor_info *sensordata;
};

static struct mv9335_ctrl_t *mv9335_ctrl = NULL;

static DECLARE_WAIT_QUEUE_HEAD(mv9335_wait_queue);
DECLARE_MUTEX(mv9335_sem);

#ifdef F_SKYCAM_HW_POWER
struct mv9335_vreg_t {
	const char *name;
	uint16_t mvolt;
#ifdef F_SKYCAM_MODEL_EF12S
	/* Need some delays while turnning on and off power. */
	uint32_t mdelay;
#endif
};

#ifdef F_SKYCAM_MODEL_EF12S
/* MV9335 needs precise power on/off sequence, otherwise some GPIOs 
 * will be unstable. */
static struct mv9335_vreg_t mv9335_vreg_on[] = {
	/* AF_MODULE(2.8V) */
	{
		.name = "gp6",	/* 2.8V_CAM_AF */
		.mvolt = 2800,
		.mdelay = 0
	},
#ifndef TEMP_POWERSEQ
	/* This is also used for P4 PAD source voltage, so shall not be 
	 * turned off. mARM turns on this at boot time because of leakage issue.
	 * See AMSS/products/8650/drivers/pmic/pmic3/app/gen_sw/pm.c 
	 * There is a switch between '1.8V_CAM_IO' and MV9335, and you should 
	 * set 'GPIO_MV9335_IO_VREG_EN' to HIGH to turn on this swtich. */
	/* VDDIO(1.8V) */
	{
		.name = "gp1",	/* 1.8V_CAM_IO */
		.mvolt = 1800,
		.mdelay = 0
	},
	/* VDDA18(1.8V) */
	{
		.name = "rftx",	/* 1.8V_CAM */
		.mvolt = 1800,
		.mdelay = 0
	},
#endif
	/* SENSOR_ANALOG(2.8V) + CVDD(1.2V) + AVDD(1.2V) + VDDA12(1.2V) */
	{
		.name = "gp5",	/* 2.8V_CAM_A, 1.2V_CAM */
		.mvolt = 2800,
		.mdelay = 0	/* MANDATORY: 3ms + 2ms margin. */
	},
#ifndef TEMP_POWERSEQ
	/* FVDD(2.8V) + FVDDIO(2.8V) */
	{
		.name = "wlan",	/* 2.8V_CAM_F */
		.mvolt = 2800,
		.mdelay = 0
	},
#endif
};

static struct mv9335_vreg_t mv9335_vreg_off[] = {
	/* SENSOR_ANALOG(2.8V) + CVDD(1.2V) + AVDD(1.2V) + VDDA12(1.2V) */
	{
		.name = "gp5",	/* 2.8V_CAM_A, 1.2V_CAM */
		.mvolt = 2800,
		.mdelay = 0
	},
	/* FVDD(2.8V) + FVDDIO(2.8V) */
	{
		.name = "wlan",	/* 2.8V_CAM_F */
		.mvolt = 2800,
		.mdelay = 0
	},
	/* VDDA18(1.8V) */
	{
		.name = "rftx",	/* 1.8V_CAM */
		.mvolt = 1800,
		.mdelay = 0
	},
	/* This is also used for P4 PAD source voltage, so shall not be 
	 * turned off. mARM turns on this at boot time because of leakage issue.
	 * See AMSS/products/8650/drivers/pmic/pmic3/app/gen_sw/pm.c 
	 * There is a switch between '1.8V_CAM_IO' and MV9335, and you should 
	 * set 'GPIO_MV9335_IO_VREG_EN' to HIGH to turn off this swtich. */
 	/* VDDIO(1.8V) */
	{
		.name = "gp1",	/* 1.8V_CAM_IO */
		.mvolt = 1800,
		.mdelay = 0
	},
	/* AF_MODULE(2.8V) */
	{
		.name = "gp6",	/* 2.8V_CAM_AF */
		.mvolt = 2800,
		.mdelay = 0
	},
};
#else
static struct mv9335_vreg_t mv9335_vreg[] = {
	{
		.name = "gp6",	/* 2.8V_CAM_D */
		.mvolt = 2800,
	},
	{
		.name = "rftx",	/* 1.8V_CAM */
		.mvolt = 1800,
	},
/* This is also used for P4 PAD source voltage, so shall not be turned off.
 * mARM turns on this at boot time because of leakage issue.
 * See AMSS/products/8650/drivers/pmic/pmic3/app/gen_sw/pm.c */
#if 0
	{
		.name = "gp1",	/* 2.6V_CAM */
		.mvolt = 2600,
	},
#endif
	{
		.name = "gp5",	/* 2.8V_CAM_A, 1.0V_CAM */
		.mvolt = 2800,
	},
};
#endif
#endif

static int32_t mv9335_sensor_mode = SENSOR_PREVIEW_MODE;
static int32_t mv9335_focus_rect_mode = MV9335_FOCUS_RECT_AUTO;

struct mv9335_ver_t {
	uint8_t hw_id;
	uint8_t hw_ver;
	uint8_t fw_major_ver;
	uint8_t fw_minor_ver;
};

static struct mv9335_ver_t mv9335_ver = {0, 0, 0, 0};

#ifdef F_SKYCAM_ADD_CFG_DIMENSION
static struct dimension_cfg mv9335_dimension;
#endif

#ifdef F_SKYCAM_MODEL_EF12S
/* RESET/MCLK shall be LOW before turnning on/off power. */
static int32_t mv9335_gpio_on(void)
{
	const struct msm_camera_sensor_info *sinfo = NULL;
	int32_t rc = 0;

	sinfo = mv9335_ctrl->sensordata;

	rc = gpio_request(sinfo->sensor_reset, "mv9335");
	if (rc < 0) {
		SKYCERR("%s#%d: error. rc=%d\n", __func__, __LINE__, rc);
		goto gpio_on_fail;
	}

	rc = gpio_direction_output(sinfo->sensor_reset, 0);
	if (rc < 0) {
		SKYCERR("%s#%d: error. rc=%d\n", __func__, __LINE__, rc);
		goto gpio_on_fail;
	}

	/* For stability */
	mdelay(10);

gpio_on_fail:
	gpio_free(sinfo->sensor_reset);
	return rc;
}

static int32_t mv9335_gpio_off(void)
{
	return mv9335_gpio_on();
}
#endif

static int32_t mv9335_power_on(void)
{
#ifdef F_SKYCAM_HW_POWER
	struct vreg *vreg = NULL;
	uint32_t vnum = 0;
	uint32_t i = 0;
	int32_t rc = 0;

#ifdef F_SKYCAM_MODEL_EF12S
	rc = mv9335_gpio_on();
	if (rc < 0) {
		SKYCERR("%s#%d: error. rc=%d\n", __func__, __LINE__, rc);
		goto power_on_fail;
	}
#endif

	vnum = ARRAY_SIZE(mv9335_vreg_on);

	for (i = 0; i < vnum; i++) {
#ifdef F_SKYCAM_MODEL_EF12S
		/* '1.8V_CAM_IO' is controlled by a switch. */
		if (!strcmp(mv9335_vreg_on[i].name, "gp1")) {
			rc = gpio_request(GPIO_MV9335_IO_VREG_EN, "mv9335");
			if (!rc) {
				rc = gpio_direction_output(
					GPIO_MV9335_IO_VREG_EN, 1);
			} else {
				SKYCERR("%s#%d: error. i=%d\n", 
					__func__, __LINE__, i);
				goto power_on_fail;
			}
			gpio_free(GPIO_MV9335_IO_VREG_EN);
			if (mv9335_vreg_on[i].mdelay)
				mdelay(mv9335_vreg_on[i].mdelay);
			continue;
		}
#endif

		vreg = vreg_get(NULL, mv9335_vreg_on[i].name);
		if (IS_ERR(vreg)) {
			SKYCERR("%s#%d: -ENOENT. i=%d\n", 
				__func__, __LINE__, i);
			rc = -ENOENT;
			goto power_on_fail;
		}

		rc = vreg_set_level(vreg, mv9335_vreg_on[i].mvolt);
		if (rc) {
			SKYCERR("%s#%d: error. i=%d\n", __func__, __LINE__, i);
			goto power_on_fail;
		}

		rc = vreg_enable(vreg);
		if (rc) {
			SKYCERR("%s#%d: error. i=%d\n", __func__, __LINE__, i);
			goto power_on_fail;
		}

#ifdef TEMP_POWERSEQ
		if (!strcmp(mv9335_vreg_on[i].name, "gp5")) {
			msm_camio_clk_rate_set(MV9335_CFG_SCLK);
			mv9335_ctrl->sensordata->pdata->camera_gpio_on();
		}
#endif

#ifdef F_SKYCAM_MODEL_EF12S
		if (mv9335_vreg_on[i].mdelay)
			mdelay(mv9335_vreg_on[i].mdelay);
#endif
	}

	/* For stability. */
//	mdelay(10);

	SKYCDBG("%s: done.\n", __func__);
	return 0;

power_on_fail:
#ifdef F_SKYCAM_MODEL_EF12S
	gpio_free(GPIO_MV9335_IO_VREG_EN);
#endif
	return rc;
#else
	return 0;
#endif
}

static int32_t mv9335_power_off(void)
{
#ifdef F_SKYCAM_HW_POWER
	struct vreg *vreg = NULL;
	uint32_t vnum = 0;
	uint32_t i = 0;
	int32_t rc = 0;

	pmic_secure_mpp_config_i_sink(PM_MPP_18,PM_MPP__I_SINK__LEVEL_5mA,PM_MPP__I_SINK__SWITCH_DIS); 

#ifdef F_SKYCAM_MODEL_EF12S
	rc = mv9335_gpio_off();
	if (rc < 0) {
		SKYCERR("%s#%d: error. rc=%d\n", __func__, __LINE__, rc);
		goto power_off_fail;
	}
#endif

	vnum = ARRAY_SIZE(mv9335_vreg_off);

	for (i = 0; i < vnum; i++) {
#ifdef F_SKYCAM_MODEL_EF12S
		/* 1.8V_CAM_IO is controlled by a switch. */
		if (!strcmp(mv9335_vreg_off[i].name, "gp1")) {
			rc = gpio_request(GPIO_MV9335_IO_VREG_EN, "mv9335");
			if (!rc) {
				rc = gpio_direction_output(
					GPIO_MV9335_IO_VREG_EN, 0);
			} else {
				SKYCERR("%s#%d: error. i=%d\n", 
					__func__, __LINE__, i);
				goto power_off_fail;
			}
			gpio_free(GPIO_MV9335_IO_VREG_EN);
			if (mv9335_vreg_off[i].mdelay)
				mdelay(mv9335_vreg_off[i].mdelay);
			continue;
		}
#endif
		vreg = vreg_get(NULL, mv9335_vreg_off[i].name);
		if (IS_ERR(vreg)) {
			SKYCERR("%s#%d: -ENOENT. i=%d\n", 
				__func__, __LINE__, i);
			rc = -ENOENT;
			goto power_off_fail;
		}

		rc = vreg_disable(vreg);
		if (rc) {
			SKYCERR("%s#%d: error. i=%d\n", __func__, __LINE__, i);
			goto power_off_fail;
		}

#ifdef TEMP_POWERSEQ
		if (!strcmp(mv9335_vreg_off[i].name, "gp5")) {
			mv9335_ctrl->sensordata->pdata->camera_gpio_off();
		}
#endif

#ifdef F_SKYCAM_MODEL_EF12S
		if (mv9335_vreg_off[i].mdelay)
			mdelay(mv9335_vreg_off[i].mdelay);
#endif
	}

	/* For stability. */
	mdelay(10);

	SKYCDBG("%s: done.\n", __func__);
	return 0;

power_off_fail:
#ifdef F_SKYCAM_MODEL_EF12S
	gpio_free(GPIO_MV9335_IO_VREG_EN);
#endif
	return rc;
#else
	return 0;
#endif
}

static int32_t mv9335_reset(void)
{
	const struct msm_camera_sensor_info *sinfo;
	int32_t rc = 0;

	sinfo = mv9335_ctrl->sensordata;

	rc = gpio_request(sinfo->sensor_reset, "mv9335");
	if (rc < 0) {
		SKYCERR("%s#%d: error. rc=%d\n", __func__, __LINE__, rc);
		goto reset_fail;
	}

	rc = gpio_direction_output(sinfo->sensor_reset, 0);
	if (rc < 0) {
		SKYCERR("%s#%d: error. rc=%d\n", __func__, __LINE__, rc);
		goto reset_fail;
	}

	/* MANDATORY: 10ms + 10% margin. */
	mdelay(12);

	rc = gpio_direction_output(sinfo->sensor_reset, 1);
	if (rc < 0) {
		SKYCERR("%s#%d: error. rc=%d\n", __func__, __LINE__, rc);
		goto reset_fail;
	}

	/* For stability */
	mdelay(10);

	gpio_free(sinfo->sensor_reset);
	SKYCDBG("%s: done.\n", __func__);
	return 0;

reset_fail:
	gpio_free(sinfo->sensor_reset);
	return rc;
}

static int32_t mv9335_i2c_txdata(uint16_t saddr,
	uint8_t *data, uint16_t len)
{
	uint32_t i = 0;
	int32_t rc = 0;

	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = len,
			.buf = data,
		},
	};

	if (len < 2) {
		SKYCERR("%s: -EINVAL\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < MV9335_I2C_RETRY; i++) {
		rc = i2c_transfer(mv9335_client->adapter, msg, 1);
		if (rc >= 0) {
			SKYCDBG("%s: done. [%02x.%02x.%02x] len=%d\n", 
				__func__, saddr, *data, *(data + 1), len);
			return 0;
		}
		SKYCERR("%s: tx retry. [%02x.%02x.%02x] len=%d, rc=%d\n", 
			__func__, saddr, *data, *(data + 1), len, rc);
		msleep(MV9335_I2C_MPERIOD);
	}

	SKYCERR("%s: error. [%02x.%02x.%02x] len=%d, rc=%d\n", 
		__func__, saddr, *data, *(data + 1), len, rc);
/*	return -EIO;*/
/*	return rc;*/
	return 0;
}

static int32_t mv9335_i2c_write(uint16_t saddr, 
	uint8_t addr, uint8_t data)
{
	int32_t rc = 0;
	uint8_t buf[2];

	memset(buf, 0, sizeof(buf));
	buf[0] = addr;
	buf[1] = data;

	rc = mv9335_i2c_txdata(saddr, buf, 2);
	return rc;
}

static int32_t mv9335_i2c_write_table(
	struct mv9335_i2c_reg_conf const *item,
	uint32_t num_items)
{
	uint32_t i = 0;
	int32_t rc = -EFAULT;

	for (i = 0; i < num_items; i++) {
		rc = mv9335_i2c_write(mv9335_client->addr,
				      item->addr, item->data);
		if (rc < 0)
			break;

		if (item->mdelay != 0)
			mdelay(item->mdelay);

		item++;
	}

	return rc;
}

static int32_t mv9335_i2c_rxdata(uint16_t saddr, uint8_t *addr, 
				 uint8_t *data, uint16_t len)
{
	uint32_t i = 0;
	int32_t rc = 0;

	struct i2c_msg msgs[] = {
	{
		.addr   = saddr,
		.flags = 0,
		.len   = 1,
		.buf   = addr,
	},
	{
		.addr   = saddr,
		.flags = I2C_M_RD,
		.len   = len,
		.buf   = data,
	},
	};

	for (i = 0; i < MV9335_I2C_RETRY; i++) {
		rc = i2c_transfer(mv9335_client->adapter, msgs, 2);
		if (rc >= 0) {
			SKYCDBG("%s: done. [%02x.%02x.%02x] len=%d\n",
				__func__, saddr, *addr, *data, len);
			return 0;
		}
		SKYCERR("%s: rx retry. [%02x.%02x.xx] len=%d, rc=%d\n", 
			__func__, saddr, *addr, len, rc);
		msleep(MV9335_I2C_MPERIOD);
	}

	SKYCERR("%s: error. [%02x.%02x.xx] len=%d rc=%d\n", 
		__func__, saddr, *addr, len, rc);
/*	return -EIO;*/
/*	return rc;*/
	return 0;
}

static int32_t mv9335_i2c_read(uint16_t saddr,
	uint8_t addr, uint8_t *data)
{
	int32_t rc = 0;
	unsigned char tmp_raddr = 0;

	if (!data)
		return -EIO;

	tmp_raddr = addr;

	rc = mv9335_i2c_rxdata(saddr, &tmp_raddr, data, 1);

	return rc;
}

static int32_t mv9335_poll(uint16_t saddr, uint8_t addr, uint8_t data,
			   uint8_t mperiod, uint32_t retry)
{
	uint8_t rdata = 0;
	uint32_t i = 0;
	int32_t rc = 0;

	for (i = 0; i < retry; i++) {
		rc = mv9335_i2c_read(saddr, addr, &rdata);
		if (rc < 0)
			break;

		if (rdata == data)
			break;

		mdelay(mperiod);
	}

	if (i == retry) {
		SKYCERR("%s: -ETIMEDOUT, mperiod=%d, retry=%d\n", 
			__func__, mperiod, retry);
		rc = -ETIMEDOUT;
	}

	return rc;
}

static int32_t mv9335_cmd(uint16_t saddr, uint8_t addr, uint8_t data)
{
	int32_t rc = 0;
	uint8_t buf[2];

	/* If you poll idle after sending each command, this is redundant. 
	 * ISP shall be in idle state before sending any command here, so you 
	 * should add delay after doing power on/off, reset, and update f/w. */
//	rc = mv9335_poll(saddr, REG_MV9335_STATE, MV9335_STATE_IDLE, 10, 300);
//	if (rc < 0)
//		return rc;

	/* MANDATORY */
	rc = mv9335_i2c_write(saddr, REG_MV9335_STATE, 0x00);
	if (rc < 0)
		return rc;

	memset(buf, 0, sizeof(buf));
	buf[0] = addr;
	buf[1] = data;

	rc = mv9335_i2c_txdata(saddr, buf, 2);
	if (rc < 0)
		return rc;

	rc = mv9335_poll(saddr, REG_MV9335_STATE, MV9335_STATE_IDLE, 10, 300);
	if (rc < 0)
		return rc;

	return 0;
}

uint8_t isp_bin[] = {
	#include MV9335_FW_FNAME
};

static int32_t mv9335_update_fw_boot(void)
{
#define MV9335_SECTOR_SIZE	4096
#define MV9335_NUM_SECTORS	16

	unsigned char tbyte[3] = {0, 0, 0};
	/* R4050, GCC 4.4 limits stack size up to 1K bytes, so you can't
	 * define 64KB array here.
	 */
#if 0
	const uint8_t isp_bin[] = {
		#include MV9335_FW_FNAME
	};
#endif
	uint32_t bin_len = 0;
	uint32_t i = 0;
	int32_t rc = 0;

	SKYCDBG("%s is called.\n", __func__);

	rc = mv9335_reset();
	if (rc < 0) {
		goto update_fw_boot_fail;
	}

#ifdef F_SKYCAM_MODEL_EF12S
	/* Init PLL. */
	rc = mv9335_i2c_write_table(&mv9335_regs.plltbl2[0],
				    mv9335_regs.plltbl2_size);
	if (rc < 0)
		goto update_fw_boot_fail;
#endif

	/* Disable PLL, select flash memory. */
	rc = mv9335_i2c_write(mv9335_client->addr, 0x80, 0x02);
	if (rc < 0)
		goto update_fw_boot_fail;

	/* Enter flash burning mode. */
	rc = mv9335_i2c_write(mv9335_client->addr, 0x5B, 0x01);
	if (rc < 0)
		goto update_fw_boot_fail;

#ifdef F_SKYCAM_MODEL_EF12S
	/* Don't erase 16th sector, it has calibration data. */
	tbyte[0] = 0x00; tbyte[1] = 0x00; tbyte[2] = 0x05;
	for (i = 0; i < 15; i++) {
		tbyte[1] = (i << 4) & 0xf0;
		rc = mv9335_i2c_txdata(0x40 >> 1, &tbyte[0], 3);
		if (rc < 0)
			goto update_fw_boot_fail;
		/* MANATORY: 120ms + 10% margin. */
		mdelay(132);
	}
#else
	/* Erase entire flash. */
	tbyte[0] = 0x00; tbyte[1] = 0x00; tbyte[2] = 0x02;
	rc = mv9335_i2c_txdata(0x40 >> 1, &tbyte[0], 3);
	if (rc < 0)
		goto update_fw_boot_fail;

	/* MANATORY: 120ms + 10% margin. */
	mdelay(132);
#endif

	/* enter flash write mode */
	tbyte[0] = 0x00; tbyte[1] = 0x00; tbyte[2] = 0x50;
	rc = mv9335_i2c_txdata(0x40 >> 1, &tbyte[0], 3);
	if (rc < 0)
		goto update_fw_boot_fail;

	/* For stability. */
	mdelay(10);

	bin_len = sizeof(isp_bin);
	if (bin_len > (MV9335_SECTOR_SIZE * MV9335_NUM_SECTORS)) {
		SKYCERR("%s: binary is too long! %d", __func__, bin_len);
		goto update_fw_boot_fail;
	}

	for (i = 0; i < MV9335_NUM_SECTORS; i++) {
		if (bin_len < MV9335_SECTOR_SIZE)
			break;
		SKYCDBG("writing %d/%d sector...\n", 
			i + 1, MV9335_NUM_SECTORS);
		rc = mv9335_i2c_txdata(0x40 >> 1, 
			&isp_bin[i * MV9335_SECTOR_SIZE], MV9335_SECTOR_SIZE);
		if (rc < 0)
			goto update_fw_boot_fail;
		bin_len -= MV9335_SECTOR_SIZE;
	}

	if (bin_len > 0) {
		SKYCDBG("writing %d/%d sector...\n", 
			i + 1, MV9335_NUM_SECTORS);
		rc = mv9335_i2c_txdata(0x40 >> 1, 
			&isp_bin[i * MV9335_SECTOR_SIZE], bin_len);
		if (rc < 0)
			goto update_fw_boot_fail;
	}

	/* For stability. */
	mdelay(10);

	/* Get out of flash burning mode. */
	rc = mv9335_i2c_write(mv9335_client->addr, 0x5B, 0x00);
	if (rc < 0)
		goto update_fw_boot_fail;

	/* For stability. */
	mdelay(10);

#ifdef F_SKYCAM_MODEL_EF12S
{
	/* Verify checksum. */
	uint8_t csum_msb = 0;
	uint8_t csum_lsb = 0;

	SKYCDBG("%s: verify checksum.\n", __func__);

	rc = mv9335_reset();
	if (rc < 0) {
		goto update_fw_boot_fail;
	}

	rc = mv9335_i2c_write_table(&mv9335_regs.plltbl[0],
				    mv9335_regs.plltbl_size);
	if (rc < 0)
		goto update_fw_boot_fail;

	rc = mv9335_poll(mv9335_client->addr, 
		REG_MV9335_STATE, MV9335_STATE_IDLE, 10, 100);
	if (rc < 0)
		goto update_fw_boot_fail;

	rc = mv9335_cmd(mv9335_client->addr, 0x18, 0x01);
	if (rc < 0)
		goto update_fw_boot_fail;

	rc = mv9335_i2c_read(mv9335_client->addr, 0x90, &csum_msb);
	if (rc < 0)
		goto update_fw_boot_fail;

	rc = mv9335_i2c_read(mv9335_client->addr, 0x91, &csum_lsb);
	if (rc < 0)
		goto update_fw_boot_fail;

	if ((csum_msb != MV9335_FW_CSUM_MSB) || 
		(csum_lsb != MV9335_FW_CSUM_LSB)) {
		rc = -ENODEV;
		goto update_fw_boot_fail;
	}

	SKYCDBG("%s: checksum (0x%02x%02x) is correct.\n", __func__, 
		csum_msb, csum_lsb);
}
#endif

	SKYCDBG("%s: done.\n", __func__);
	return 0;

update_fw_boot_fail:
	SKYCERR("%s: error. rc=%d\n", __func__, rc);
	return rc;
}

#ifdef F_SKYCAM_ADD_CFG_UPDATE_ISP
static int32_t mv9335_update_fw_user(const struct sensor_cfg_data *scfg)
{
	unsigned char tbyte[3] = {0, 0, 0};
	uint8_t *paddr = NULL;
	int32_t vaddr = 0;
	uint32_t len = 0;
	/* MV9335 has 16 sectors, each sector has 4KB. 
	 * if you set 'frag' to 4KB, fails to i2c_transfer frequently. */
	uint32_t frag = 1024;
	uint32_t i = 0;
	int32_t rc = 0;

	SKYCDBG("%s is called.\n", __func__);

	vaddr = scfg->cfg.isp_bin_vaddr;

	/* don't wanna copy, wanna convert virt to phy only. */
	paddr = kmalloc(64 * 1024 + 4, GFP_KERNEL);
	if (!paddr) {
		SKYCDBG("failed to malloc.\n");
		rc = -ENOMEM;
		goto update_fw_user_fail;
	}

	if (copy_from_user(paddr, (void *)vaddr, 64 * 1024 + 4)) {
		SKYCDBG("failed to copy from user.\n");
		rc = -EFAULT;
		goto update_fw_user_fail;
	}

	len = *(uint32_t *)(paddr + 64 * 1024);

	SKYCDBG("p=0x%08x, v=0x%08x len=%d\n", paddr, vaddr, len);

	rc = mv9335_reset();
	if (rc < 0) {
		goto update_fw_user_fail;
	}

	/* Init PLL. */
	rc = mv9335_i2c_write_table(&mv9335_regs.plltbl2[0],
				    mv9335_regs.plltbl2_size);
	if (rc < 0)
		goto update_fw_user_fail;

	/* Disable PLL, select flash memory. */
	rc = mv9335_i2c_write(mv9335_client->addr, 0x80, 0x02);
	if (rc < 0)
		goto update_fw_user_fail;

	/* Enter flash burning mode. */
	rc = mv9335_i2c_write(mv9335_client->addr, 0x5B, 0x01);
	if (rc < 0)
		goto update_fw_user_fail;

#ifdef F_SKYCAM_MODEL_EF12S
	/* Don't erase 16th sector, it has calibration data. */
	tbyte[0] = 0x00; tbyte[1] = 0x00; tbyte[2] = 0x05;
	for (i = 0; i < 15; i++) {
		tbyte[1] = (i << 4) & 0xf0;
		rc = mv9335_i2c_txdata(0x40 >> 1, &tbyte[0], 3);
		if (rc < 0)
			goto update_fw_user_fail;
		/* MANATORY: 120ms + 10% margin. */
		mdelay(132);
	}
#else
	/* Erase entire flash. */
	tbyte[0] = 0x00;
	tbyte[1] = 0x00;
	tbyte[2] = 0x02;
	rc = mv9335_i2c_txdata(0x40 >> 1, &tbyte[0], 3);
	if (rc < 0)
		goto update_fw_user_fail;

	/* MANATORY: 120ms + 10% margin. */
	mdelay(132);
#endif

	/* enter flash write mode */
	tbyte[0] = 0x00;
	tbyte[1] = 0x00;
	tbyte[2] = 0x50;
	rc = mv9335_i2c_txdata(0x40 >> 1, &tbyte[0], 3);
	if (rc < 0)
		goto update_fw_user_fail;

	/* For stability. */
	mdelay(10);

	for (i = 0; i < (len / frag); i++ ) {
		rc = mv9335_i2c_txdata(0x40 >> 1, paddr + (i * frag), frag);
		if (rc < 0)
			goto update_fw_user_fail;
	}

	if (len % frag) {
		rc = mv9335_i2c_txdata(0x40 >> 1, paddr + (i * frag), 
				       len % frag);
		if (rc < 0)
			goto update_fw_user_fail;
	}

	/* For stability. */
	mdelay(10);

	/* Get out of flash burning mode. */
	rc = mv9335_i2c_write(mv9335_client->addr, 0x5B, 0x00);
	if (rc < 0)
		goto update_fw_user_fail;

	/* For stability. */
	mdelay(10);

	if (paddr)
		kfree(paddr);

	SKYCDBG("%s: done.\n", __func__);
	return 0;

update_fw_user_fail:
	if (paddr)
		kfree(paddr);
	SKYCERR("%s: error. rc=%d\n", __func__, rc);
	return rc;
}
#endif

static int32_t mv9335_start_preview(void)
{
	int32_t rc = 0;

#ifdef F_SKYCAM_MODEL_EF12S
	/* Set preview resolution to 1280x960. (sensor output) */
	rc = mv9335_cmd(mv9335_client->addr, 0x10, 0x09);
	if (rc < 0)
		goto start_preview_fail;

	/* Set preview resolution to 1280x960. (ISP output) */
	rc = mv9335_cmd(mv9335_client->addr, 0x11, 0x09);
	if (rc < 0)
		goto start_preview_fail;

	/* Start preview. */
	rc = mv9335_cmd(mv9335_client->addr, 0x07, 0x01);
	if (rc < 0)
		goto start_preview_fail;

	/* Select preview mode. */
	rc = mv9335_cmd(mv9335_client->addr, 0x08, 0x00);
	if (rc < 0)
		goto start_preview_fail;
#else
	/* Start preview. */
	rc = mv9335_cmd(mv9335_client->addr, 0x07, 0x10);
	if (rc < 0)
		goto start_preview_fail;

	/* Set preview resolution to 1280x960. */
	rc = mv9335_cmd(mv9335_client->addr, 0x10, 0x52);
	if (rc < 0) 
		goto start_preview_fail;
#endif

	SKYCDBG("%s: done.\n", __func__);

	/* If not set, preview images are divided or corrupted. */
/*	mdelay(500);*/

	return 0;

start_preview_fail:
	SKYCERR("%s: error. rc=%d\n", __func__, rc);
	return rc;
}

static int32_t mv9335_start_snapshot(void)
{
	int32_t rc = 0;

#ifdef F_SKYCAM_MODEL_EF12S
	/* Select snapshot mode. */
	rc = mv9335_cmd(mv9335_client->addr, 0x08, 0x01);
	if (rc < 0)
		goto start_snapshot_fail;

	/* Set snapshot resolution to 2560x1920. (sensor output) */
	rc = mv9335_cmd(mv9335_client->addr, 0x10, 0x0e);
	if (rc < 0)
		goto start_snapshot_fail;

	/* Set snapshot resolution to 2560x1920. (ISP output) */
	rc = mv9335_cmd(mv9335_client->addr, 0x11, 0x0e);
	if (rc < 0)
		goto start_snapshot_fail;

	/* Start snapshot. */
	rc = mv9335_cmd(mv9335_client->addr, 0x07, 0x01);
	if (rc < 0)
		goto start_snapshot_fail;
#else
	/* Set snapshot resolution to 2560x1920. */
	rc = mv9335_cmd(mv9335_client->addr, 0x08, 0x50);
	if (rc < 0)
		goto start_snapshot_fail;

	/* Set snapshot format to YUV. */
	rc = mv9335_cmd(mv9335_client->addr, 0x09, 0x10);
	if (rc < 0)
		goto start_snapshot_fail;

	/* Start snapshot. */
	rc = mv9335_cmd(mv9335_client->addr, 0x07, 0x01);
	if (rc < 0)
		goto start_snapshot_fail;
#endif

	SKYCDBG("%s: done.\n", __func__);

	return 0;

start_snapshot_fail:
	SKYCERR("%s: error. rc=%d\n", __func__, rc);
	return rc;
}

static int32_t mv9335_set_sensor_mode(int mode)
{
	int32_t rc = 0;

	if (mode == mv9335_sensor_mode) {
		SKYCDBG("%s: skip, mode=%d\n", __func__, mode);
		return 0;
	}

	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		rc = mv9335_start_preview();
		break;

	case SENSOR_SNAPSHOT_MODE:
		rc = mv9335_start_snapshot();
		break;

	default:
		rc = -EINVAL;
		break;
	}

	if (rc < 0)
		return rc;

	mv9335_sensor_mode = mode;
	return 0;
}

#ifdef F_SKYCAM_FIX_CFG_EFFECT
static int32_t mv9335_set_effect(int8_t effect)
{
	/* none, mono, negative, solarize, sepia, posterize(ERR), 
	 * whiteboard(ERR), blackboard(ERR), aqua */
	uint8_t data[] = {0x00, 0x03, 0x04, 0x01, 0x05, 0, 0, 0, 0x09};
	int32_t rc = 0;

	SKYCDBG("%s: effect=%d\n", __func__, effect);
	if ((effect < 0) || (effect >= sizeof(data)) || (effect == 5) ||
	    (effect == 6) || (effect == 7)) {
		SKYCERR("%s: -EINVAL. effect=%d\n", __func__, effect);
		return -EINVAL;
	}

	rc = mv9335_cmd(mv9335_client->addr, 0x26, data[effect]);
	return rc;
}
#endif

#ifdef F_SKYCAM_FIX_CFG_WB
static int32_t mv9335_set_wb(int32_t wb)
{
	/* D/C, auto, D/C, incandescent, fluorescent, daylight, cloudy */
	uint8_t data[] = {0, 0x01, 0, 0x35, 0x25, 0x15, 0x55};
	int32_t rc = 0;

	SKYCDBG("%s: wb=%d\n", __func__, wb);
	if ((wb < 0) || (wb >= sizeof(data)) || (wb == 0) || (wb == 2)) {
		SKYCERR("%s: -EINVAL. wb=%d\n", __func__, wb);
		return -EINVAL; 
	}

	rc = mv9335_cmd(mv9335_client->addr, 0x1c, data[wb]);
	return rc;
}
#endif

#ifdef F_SKYCAM_FIX_CFG_BRIGHTNESS
static int32_t mv9335_set_brightness(int32_t brightness)
{
	/* -4, -3, -2, -1, 0, +1, +2, +3, +4 */
	uint8_t data[9] = {0x84, 0x83, 0x82, 0x81, 0, 0x01, 0x02, 0x03, 0x04};
	int32_t rc = 0;

	SKYCDBG("%s: brightness=%d\n", __func__, brightness);
	if ((brightness < 0) || (brightness >= 9)) {
		SKYCERR("%s: -EINVAL. brightness=%d\n", __func__, brightness);
		return -EINVAL;
	}

	rc = mv9335_cmd(mv9335_client->addr, 0x19, data[brightness]);
	return rc;
}
#endif

#ifdef F_SKYCAM_FIX_CFG_LED_MODE
static int32_t mv9335_set_led_mode(int32_t led_mode)
{

/*SYS_LJH 20101008

Android common/camera.h has 4 LED mode 
  LED_MODE_OFF,
  LED_MODE_AUTO,
  LED_MODE_ON,
  LED_MODE_TORCH,
 so add Torch mode   */

#if 0
	/* off, auto, on */
	uint8_t data[3] = {0x00, 0x10, 0x01};
#else
	/* off, auto, on, torch */
	uint8_t data[4] = {0x00, 0x10, 0x01,0x01};
#endif
	SKYCDBG("%s: led_mode=%d\n", __func__, led_mode);
	if ((led_mode < 0) || (led_mode >= sizeof(data))) {
		SKYCERR("%s: -EINVAL, led_mode=%d\n", __func__, led_mode);
		return -EINVAL;
	}

	return mv9335_cmd(mv9335_client->addr, 0x27, data[led_mode]);
}
#endif

#ifdef F_SKYCAM_FIX_CFG_AF
static int32_t mv9335_auto_focus(void)
{
#define AF_POLL_PERIOD	100
#define AF_POLL_RETRY	30

	uint8_t rdata = 0;
	uint8_t focus_rect_mode = 0;
	uint8_t i = 0;
	int32_t rc = 0;

	focus_rect_mode = (uint8_t)(mv9335_focus_rect_mode & 0x000000ff);
#ifdef F_SKYCAM_MODEL_EF12S
	/* You should check 0x0e before every I2C R/W operation. */
	rc = mv9335_cmd(mv9335_client->addr, 0x1d, focus_rect_mode);
#else
	rc = mv9335_i2c_write(mv9335_client->addr, 0x1d, focus_rect_mode);
#endif
	if (rc < 0)
		return rc;

	/* You don't need to poll ISP state. */
	rc = mv9335_i2c_write(mv9335_client->addr, 0x0c, 0x00);
	if (rc < 0)
		return rc;

	rc = mv9335_i2c_write(mv9335_client->addr, 0x23, 0x01);
	if (rc < 0)
		return rc;

	/* AF should be done within (AF_POLL_PERIOD x AF_POLL_RETRY) msecs. */
	for (i = 0; i < AF_POLL_RETRY; i++) {
		rc = mv9335_i2c_read(mv9335_client->addr, 0x0c, &rdata);
		if (rc < 0)
			return rc;

		if (rdata == 0xbb) {
			SKYCDBG("AF: got focus\n");
			break;
		}

		/* If you failed to get focus, move lens to infinity. */
		if (rdata == 0xcc) {
			SKYCDBG("AF: failed to get focus\n");
#ifdef F_SKYCAM_MODEL_EF12S
			/* MV9337 does never return AF fail, but MV9335 does, 
			 * so ignore AF fail to retain best lens position. */
			mdelay(AF_POLL_PERIOD);
#else
			rc = mv9335_i2c_write(mv9335_client->addr, 0x23, 0x00);
			if (rc < 0)
				return rc;
#endif
			break;
		}

		mdelay(AF_POLL_PERIOD);
		//msleep_interruptible(AF_POLL_PERIOD);
	}

	/* If timeout occurs, move lens to infinity. */
	if (i == AF_POLL_RETRY) {
		SKYCERR("AF: timeout\n");
		rc = mv9335_i2c_write(mv9335_client->addr, 0x23, 0x00);
		if (rc < 0)
			return rc;
	}

	/* Last preview frame is not focused for some reason,
	 * so add delay here to view several frames after AF is done. */
	mdelay(AF_POLL_PERIOD);

	/* You might fail to get focus,
	 * - if the distance between lens and object is too close.
	 * - if you shake phone while doing AF.
	 * Camera app can be stuck if you return error here, so you should 
	 * return success. */
	return 0;
}
#endif

#ifdef F_SKYCAM_FIX_CFG_ISO
static int32_t mv9335_set_iso(int32_t iso)
{
	/* auto, ISO_HJR(ERR), ISO100, ISO200, ISO400, ISO800 */
	uint8_t data[] = {0x00, 0, 0x01, 0x02, 0x03, 0x04};

	SKYCDBG("%s: iso=%d\n", __func__, iso);
	if ((iso < 0) || (iso >= sizeof(data)) || (iso == 1)) {
		SKYCERR("%s: -EINVAL, iso=%d\n", __func__, iso);
		return -EINVAL;
	}

	return mv9335_cmd(mv9335_client->addr, 0x17, data[iso]);
}
#endif

#ifdef F_SKYCAM_FIX_CFG_SCENE_MODE
static int32_t mv9335_set_scene_mode(int32_t scene_mode)
{
	/* off, auto, beach, indoor, landscape, night, party, portrait,
	 * snow, sports, sunset, text */
	uint8_t data[] = {0x00, 0x01, 0x21, 0x0b, 0x09, 0x0d, 0x23, 0x05, 
		0x13, 0x0f, 0x1d, 0x1b};

	SKYCDBG("%s: scene_mode=%d\n", __func__, scene_mode);
	if ((scene_mode < 0) || (scene_mode >= sizeof(data))) {
		SKYCERR("%s: -EINVAL, scene_mode=%d\n", __func__, scene_mode);
		return -EINVAL;
	}

	return mv9335_cmd(mv9335_client->addr, 0x13, data[scene_mode]);
}
#endif

#ifdef F_SKYCAM_FIX_CFG_FOCUS_STEP
static int32_t mv9335_set_focus_step(int32_t focus_step)
{
	/* 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 */
	uint8_t data[] = {0x00, 0x10, 0x20, 0x30, 0x40, 
			  0x50, 0x60, 0x70, 0x80, 0xff};


	SKYCDBG("%s: focus_step=%d\n", __func__, focus_step);
	if ((focus_step < 0) || (focus_step >= sizeof(data))) {
		SKYCERR("%s: -EINVAL, focus_step=%d\n", __func__, focus_step);
		return -EINVAL;
	}

	return mv9335_cmd(mv9335_client->addr, 0x24, data[focus_step]);
}
#endif

#ifdef F_SKYCAM_ADD_CFG_SZOOM
static int32_t mv9335_set_szoom(int32_t szoom)
{
	/* 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 */
	uint8_t data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 
			  0x06, 0x07, 0x08, 0x09};

	SKYCDBG("%s: szoom=%d\n", __func__, szoom);
	if ((szoom < 0) || (szoom >= sizeof(data))) {
		SKYCERR("%s: -EINVAL, szoom=%d\n", __func__, szoom);
		return -EINVAL;
	}

	return  mv9335_cmd(mv9335_client->addr, 0x2b, data[szoom]);
}
#endif

#ifdef F_SKYCAM_ADD_CFG_ANTISHAKE
static int32_t mv9335_set_antishake(int32_t antishake)
{
	/* off, on */
	uint8_t data[] = {0x00, 0x01};

	SKYCDBG("%s: antishake=%d\n", __func__, antishake);
	if ((antishake < 0) || (antishake >= sizeof(data))) {
		SKYCERR("%s: -EINVAL, antishake=%d\n", __func__, antishake);
		return -EINVAL;
	}

	return mv9335_cmd(mv9335_client->addr, 0x2f, data[antishake]);
}
#endif

#ifdef F_SKYCAM_FIX_CFG_EXPOSURE
static int32_t mv9335_set_exposure_mode(int32_t exposure)
{
	/* frameaverage, centerweighted, spotmetering */
	/* default: centerweighted */
	uint8_t data[] = {0x05, 0x01, 0x03};

	SKYCDBG("%s: exposure=%d\n", __func__, exposure);
	if ((exposure < 0) || (exposure >= sizeof(data))) {
		SKYCERR("%s: -EINVAL, exposure=%d\n", __func__, exposure);
		return -EINVAL;
	}

	return mv9335_cmd(mv9335_client->addr, 0x15, data[exposure]);
}
#endif

#ifdef F_SKYCAM_FIX_CFG_FOCUS_RECT
static int32_t mv9335_set_focus_rect(uint32_t focus_rect)
{
	uint8_t xleft, xright, ytop, ybottom, xc, yc;
	int32_t rc = 0;

	SKYCDBG("%s: focus_rect=0x%08x\n", __func__, focus_rect);

	xleft   = (uint8_t)((focus_rect & 0x0f000000) >> 24);
	xright  = (uint8_t)((focus_rect & 0x000f0000) >> 16);
	ytop    = (uint8_t)((focus_rect & 0x00000f00) >> 8);
	ybottom = (uint8_t)((focus_rect & 0x0000000f));

	if ((xleft == 0) && (xright == 0) && (ytop == 0) && (ybottom == 0)) {
		mv9335_focus_rect_mode = MV9335_FOCUS_RECT_AUTO;
		return 0;
	} else {
		mv9335_focus_rect_mode = MV9335_FOCUS_RECT_USER;

		xc = xleft * 16 + xright;
		yc = ytop * 16 + ybottom;

		rc = mv9335_cmd(mv9335_client->addr, 0x21, xc);
		if (rc < 0)
			return rc;

		rc = mv9335_cmd(mv9335_client->addr, 0x22, yc);
		if (rc < 0)
			return rc;
	}

	return 0;
}
#endif

#ifdef F_SKYCAM_ADD_CFG_DIMENSION
static int32_t mv9335_set_dimension(struct dimension_cfg *dimension)
{
	memcpy(&mv9335_dimension, dimension, sizeof(struct dimension_cfg));
	SKYCDBG("%s: preview=%dx%d, snapshot=%dx%d\n", __func__,
		dimension->prev_dx, dimension->prev_dy,
		dimension->pict_dx, dimension->pict_dy);
	return 0;
}
#endif

#ifdef F_SKYCAM_FIX_CFG_ANTIBANDING
static int32_t mv9335_set_antibanding(int32_t antibanding)
{
	/* MtekVision says AUTO is not reliable */
	/* off, 60hz, 50hz, auto(ERR) */
	/* default: 60hz */
	uint8_t data[] = {0x00, 0x01, 0x02, 0x03};

	SKYCDBG("%s: antibanding=%d\n", __func__, antibanding);
	if ((antibanding < 0) || (antibanding >= sizeof(data))) {
		SKYCERR("%s: -EINVAL, antibanding=%d\n", 
			__func__, antibanding);
		return -EINVAL;
	}

	return mv9335_cmd(mv9335_client->addr, 0x16, data[antibanding]);
}
#endif

#ifdef F_SKYCAM_FIX_CFG_FOCUS_MODE
static int32_t mv9335_set_focus_mode(int32_t focus_mode)
{
	/* infinity(ERR), macro, auto */
	/* default: auto */
	uint8_t data[] = {0, 0x01, 0x00};

	SKYCDBG("%s: focus_mode=%d\n", __func__, focus_mode);
	if ((focus_mode < 0) || (focus_mode >= sizeof(data)) || 
		(focus_mode == 0)) {
		SKYCERR("%s: -EINVAL, focus_mode=%d\n", 
			__func__, focus_mode);
		return -EINVAL;
	}

	return mv9335_cmd(mv9335_client->addr, 0x1f, data[focus_mode]);
}
#endif

#ifdef F_SKYCAM_FIX_CFG_PREVIEW_FPS
static int32_t mv9335_set_preview_fps(int32_t preview_fps)
{
	/* 0 : variable 30fps, 1 ~ 30 : fixed fps */
	/* default: variable 8 ~ 30fps */
	uint8_t data = 0;

	if ((preview_fps < C_SKYCAM_MIN_PREVIEW_FPS) || 
		(preview_fps > C_SKYCAM_MAX_PREVIEW_FPS)) {
		SKYCERR("%s: -EINVAL, preview_fps=%d\n", 
			__func__, preview_fps);
		return -EINVAL;
	}

	SKYCDBG("%s: preview_fps=%d\n", __func__, preview_fps);

	data = (uint8_t)(preview_fps & 0x000000ff);
	/* use variable 30fps instead of fixed one. */
	if (data == C_SKYCAM_MAX_PREVIEW_FPS)
		data = 0;

	return mv9335_cmd(mv9335_client->addr, 0x1b, data);
}
#endif

static int32_t mv9335_sensor_init_probe(void)
{
	int32_t rc = 0;

	/* Reset ISP. */
	rc = mv9335_reset();
	if (rc < 0)
		goto sensor_init_probe_fail;

	/* Init PLL. */
	rc = mv9335_i2c_write_table(&mv9335_regs.plltbl[0],
				    mv9335_regs.plltbl_size);
	if (rc < 0)
		goto sensor_init_probe_fail;

#ifdef F_SKYCAM_MODEL_EF12S
	/* MV9335 writes 0x35 to 0x01 after finishing initialization. */
	rc = mv9335_poll(mv9335_client->addr, REG_MV9335_HW_VER, MV9335_HW_VER, 
		50, 20);
	if (rc < 0) {
		SKYCERR("%s: -ETIMEDOUT.\n", __func__);
		return -ETIMEDOUT;
	}
#endif

	if ((mv9335_ver.hw_id == MV9335_HW_ID) &&
		(mv9335_ver.hw_ver == MV9335_HW_VER) &&
		(mv9335_ver.fw_major_ver == MV9335_FW_MAJOR_VER) &&
		(mv9335_ver.fw_minor_ver == MV9335_FW_MINOR_VER)) {
		goto sensor_init_probe_done;
	}

	/* Read product id, version of ISP. */
	rc = mv9335_i2c_read(mv9335_client->addr, REG_MV9335_HW_ID, 
			     &mv9335_ver.hw_id);
	if (rc < 0)
		goto sensor_init_probe_fail;

	rc = mv9335_i2c_read(mv9335_client->addr, REG_MV9335_HW_VER, 
			     &mv9335_ver.hw_ver);
	if (rc < 0)
		goto sensor_init_probe_fail;

	/* Read fw version of ISP. */
	rc = mv9335_i2c_read(mv9335_client->addr, REG_MV9335_MAJOR_VER, 
			     &mv9335_ver.fw_major_ver);
	if (rc < 0)
		goto sensor_init_probe_fail;

	rc = mv9335_i2c_read(mv9335_client->addr, REG_MV9335_MINOR_VER, 
			     &mv9335_ver.fw_minor_ver);
	if (rc < 0)
		goto sensor_init_probe_fail;

sensor_init_probe_done:
	SKYCDBG("%s: done. version=%02x.%02x.%02x.%02x\n", __func__,
		mv9335_ver.hw_id, mv9335_ver.hw_ver,
		mv9335_ver.fw_major_ver, mv9335_ver.fw_minor_ver);
	return 0;

sensor_init_probe_fail:
	SKYCERR("%s: -EIO.\n", __func__);
	return -EIO;
}

int32_t mv9335_sensor_init(const struct msm_camera_sensor_info *data)
{
#if 0//def F_SKYCAM_INVALIDATE_CAMERA_CLIENT
	uint8_t sensor_alive = 0;
#endif
	int32_t rc = 0;

	SKYCDBG("%s is called.\n", __func__);

#ifndef F_SKYCAM_MODEL_EF12S
	/* 'mv9335_gpio_on()' needs 'mv9335_ctrl'. */
	rc = mv9335_power_on();
	if (rc < 0)
		goto sensor_init_fail;
#endif

	mv9335_ctrl = kzalloc(sizeof(struct mv9335_ctrl_t), GFP_KERNEL);
	if (!mv9335_ctrl) {
		SKYCERR("%s#%d: -ENOMEM.\n", __func__, __LINE__);
		rc = -ENOMEM;
		goto sensor_init_fail;
	}

	if (data == NULL) {
		SKYCERR("%s#%d: -EINVAL.\n", __func__, __LINE__);
		rc = -EINVAL;
		goto sensor_init_fail;
	}

	mv9335_ctrl->sensordata = (struct msm_camera_sensor_info *)data;

	/* Input MCLK = 24MHz. */
#ifndef TEMP_POWERSEQ
	msm_camio_clk_rate_set(MV9335_CFG_SCLK);
	mdelay(5);
#endif

#ifdef F_SKYCAM_MODEL_EF12S
	/* MADATORY: MCLK should be enabled before supplying power. */
	rc = mv9335_power_on();
	if (rc < 0)
		goto sensor_init_fail;
#endif

#ifdef TEMP_POWERSEQ
{
	const struct msm_camera_sensor_info *sinfo;
	int32_t myrc = 0;

	mdelay(5);

	sinfo = mv9335_ctrl->sensordata;

	myrc = gpio_request(sinfo->sensor_reset, "mv9335");
	if (myrc < 0) {
		SKYCERR("%s#%d: error. rc=%d\n", __func__, __LINE__, myrc);
		goto sensor_init_fail;
	}

	myrc = gpio_direction_output(sinfo->sensor_reset, 1);
	if (myrc < 0) {
		SKYCERR("%s#%d: error. rc=%d\n", __func__, __LINE__, myrc);
		goto sensor_init_fail;
	}

	mdelay(5);

	myrc = gpio_direction_output(sinfo->sensor_reset, 0);
	if (myrc < 0) {
		SKYCERR("%s#%d: error. rc=%d\n", __func__, __LINE__, myrc);
		goto sensor_init_fail;
	}

	mdelay(5);

	/* 1.8V_CAM_IO is controlled by a switch. */
	myrc = gpio_request(GPIO_MV9335_IO_VREG_EN, "mv9335");
	if (!myrc) {
		myrc = gpio_direction_output(GPIO_MV9335_IO_VREG_EN, 1);
	} else {
		SKYCERR("%s#%d: error.\n", __func__, __LINE__);
		goto sensor_init_fail;
	}
	gpio_free(GPIO_MV9335_IO_VREG_EN);

	mdelay(5);

{
	struct vreg *vreg = NULL;
	vreg = vreg_get(NULL, "rftx");
	if (IS_ERR(vreg)) {
		SKYCERR("%s#%d: -ENOENT\n", __func__, __LINE__);
		rc = -ENOENT;
		goto sensor_init_fail;
	}

	myrc = vreg_set_level(vreg, 1800);
	if (myrc) {
		SKYCERR("%s#%d: error. myrc=%d\n", __func__, __LINE__, myrc);
		goto sensor_init_fail;
	}

	myrc = vreg_enable(vreg);
	if (myrc) {
		SKYCERR("%s#%d: error. myrc=%d\n", __func__, __LINE__, myrc);
		goto sensor_init_fail;
	}
}
{
	struct vreg *vreg = NULL;
	vreg = vreg_get(NULL, "wlan");
	if (IS_ERR(vreg)) {
		SKYCERR("%s#%d: -ENOENT\n", __func__, __LINE__);
		rc = -ENOENT;
		goto sensor_init_fail;
	}

	myrc = vreg_set_level(vreg, 2800);
	if (myrc) {
		SKYCERR("%s#%d: error. myrc=%d\n", __func__, __LINE__, myrc);
		goto sensor_init_fail;
	}

	myrc = vreg_enable(vreg);
	if (myrc) {
		SKYCERR("%s#%d: error. myrc=%d\n", __func__, __LINE__, myrc);
		goto sensor_init_fail;
	}
}

	gpio_free(sinfo->sensor_reset);
	SKYCDBG("trigger reset done\n");
}
#endif

#if 0//defined(F_SKYCAM_MODEL_EF12S) && defined(F_SKYCAM_INVALIDATE_CAMERA_CLIENT)
/* Check whether camera module is connected or not. */
{
	uint8_t alive = 0;

	rc = mv9335_reset();
	if (rc < 0)
		goto sensor_init_fail;

	rc = mv9335_i2c_read(mv9335_client->addr, REG_MV9335_HW_ID, &alive);
	if ((rc < 0) || (alive != MV9335_HW_ID)) {
		rc = -ENODEV;
		goto sensor_init_fail;
	}
}
#endif

	msm_camio_camif_pad_reg_reset();

	rc = mv9335_sensor_init_probe();
	if (rc < 0)
		goto sensor_init_fail;

#if 0//def F_SKYCAM_INVALIDATE_CAMERA_CLIENT
	/* Check whether image sensor is dead or not. */
	rc = mv9335_i2c_read(mv9335_client->addr, REG_MV9335_SENSOR_ALIVE, 
			     &sensor_alive);
	if ((rc < 0) || (sensor_alive == 0)) {
		SKYCERR("%s#%d: -ENODEV.\n", __func__, __LINE__);
		rc = -ENODEV;
		goto sensor_init_fail;
	}
#endif

#ifndef F_SKYCAM_MODEL_EF12S
	/* ISP is already in preview state if the value of 'REG_MV9335_HW_VER'
	 * is 'MV9335_HW_VER'. */
	/* Change sensor op mode to preview. */
	rc = mv9335_start_preview();
	if (rc < 0)
		goto sensor_init_fail;
#endif

	mv9335_sensor_mode = SENSOR_PREVIEW_MODE;
	pmic_secure_mpp_config_i_sink(PM_MPP_18,PM_MPP__I_SINK__LEVEL_5mA,PM_MPP__I_SINK__SWITCH_ENA);

	SKYCDBG("%s: done.\n", __func__);
	return 0;

sensor_init_fail:

	(void)mv9335_power_off();

	if (mv9335_ctrl) {
		kfree(mv9335_ctrl);
		mv9335_ctrl = NULL;
	}
	return rc;
}

int32_t mv9335_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	int32_t rc = 0;

	if (copy_from_user(&cfg_data, (void *)argp,
			   sizeof(struct sensor_cfg_data))) {
		SKYCERR("%s#%d: -EFAULT.\n", __func__, __LINE__);
		return -EFAULT;
	}

	/* down(&mv9335_sem); */

	switch (cfg_data.cfgtype) {
	case CFG_SET_MODE:
		rc = mv9335_set_sensor_mode(cfg_data.mode);
		SKYCDBG("%s: CFG_SET_MODE, rc=%d\n", __func__, rc);
		break;
#ifdef F_SKYCAM_FIX_CFG_EFFECT
	case CFG_SET_EFFECT:
		rc = mv9335_set_effect(cfg_data.cfg.effect);
		SKYCDBG("%s: CFG_SET_EFFECT, rc=%d\n", __func__, rc);
		break;
#endif
#ifdef F_SKYCAM_FIX_CFG_WB
	case CFG_SET_WB:
		rc = mv9335_set_wb(cfg_data.cfg.wb);
		SKYCDBG("%s: CFG_SET_WB, rc=%d\n", __func__, rc);
		break;
#endif
#ifdef F_SKYCAM_FIX_CFG_BRIGHTNESS
	case CFG_SET_BRIGHTNESS:
		rc = mv9335_set_brightness(cfg_data.cfg.brightness);
		SKYCDBG("%s: CFG_SET_BRIGHTNESS, rc=%d\n", __func__, rc);
		break;
#endif
#ifdef F_SKYCAM_FIX_CFG_LED_MODE
	case CFG_SET_LED_MODE:
		rc = mv9335_set_led_mode(cfg_data.cfg.led_mode);
		SKYCDBG("%s: CFG_SET_LED_MODE, rc=%d\n", __func__, rc);
		break;
#endif
#ifdef F_SKYCAM_FIX_CFG_AF
	case CFG_AUTO_FOCUS:
		rc = mv9335_auto_focus();
		SKYCDBG("%s: CFG_AUTO_FOCUS, rc=%d\n", __func__, rc);
		break;
#endif
#ifdef F_SKYCAM_FIX_CFG_ISO
	case CFG_SET_ISO:
		rc = mv9335_set_iso(cfg_data.cfg.iso);
		SKYCDBG("%s: CFG_SET_ISO, rc=%d\n", __func__, rc);
		break;
#endif
#ifdef F_SKYCAM_FIX_CFG_SCENE_MODE
	case CFG_SET_SCENE_MODE:
		rc = mv9335_set_scene_mode(cfg_data.cfg.scene_mode);
		SKYCDBG("%s: CFG_SET_SCENE_MODE, rc=%d\n", __func__, rc);
		break;
#endif
#ifdef F_SKYCAM_FIX_CFG_FOCUS_STEP
	case CFG_SET_FOCUS_STEP:
		rc = mv9335_set_focus_step(cfg_data.cfg.focus_step);
		SKYCDBG("%s: CFG_SET_FOCUS_STEP, rc=%d\n", __func__, rc);
		break;
#endif
#ifdef F_SKYCAM_ADD_CFG_SZOOM
	case CFG_SET_SZOOM:
		rc = mv9335_set_szoom(cfg_data.cfg.szoom);
		SKYCDBG("%s: CFG_SET_SZOOM, rc=%d\n", __func__, rc);
		break;
#endif
#ifdef F_SKYCAM_ADD_CFG_ANTISHAKE
	case CFG_SET_ANTISHAKE:
		rc = mv9335_set_antishake(cfg_data.cfg.antishake);
		SKYCDBG("%s: CFG_SET_ANTISHAKE, rc=%d\n", __func__, rc);
		break;
#endif
#ifdef F_SKYCAM_FIX_CFG_EXPOSURE
	case CFG_SET_EXPOSURE_MODE:
		rc = mv9335_set_exposure_mode(cfg_data.cfg.exposure);
		SKYCDBG("%s: CFG_SET_EXPOSURE_MODE, rc=%d\n", __func__, rc);
		break;
#endif
#ifdef F_SKYCAM_FIX_CFG_FOCUS_RECT
	case CFG_SET_FOCUS_RECT:
		rc = mv9335_set_focus_rect(cfg_data.cfg.focus_rect);
		SKYCDBG("%s: CFG_SET_FOCUS_RECT, rc=%d\n", __func__, rc);
		break;
#endif
#ifdef F_SKYCAM_ADD_CFG_UPDATE_ISP
	case CFG_UPDATE_ISP:
		rc = mv9335_update_fw_user(&cfg_data);
		SKYCDBG("%s: CFG_UPDATE_ISP, rc=%d\n", __func__, rc);
		break;
#endif
#ifdef F_SKYCAM_ADD_CFG_DIMENSION
	case CFG_SET_DIMENSION:
		rc = mv9335_set_dimension(&cfg_data.cfg.dimension);
		SKYCDBG("%s: CFG_SET_DIMENSION, rc=%d\n", __func__, rc);
		break;
#endif
#ifdef F_SKYCAM_FIX_CFG_ANTIBANDING
	case CFG_SET_ANTIBANDING:
		rc = mv9335_set_antibanding(cfg_data.cfg.antibanding);
		SKYCDBG("%s: CFG_SET_ANTIBANDING, rc=%d\n", __func__, rc);
		break;
#endif
#ifdef F_SKYCAM_FIX_CFG_FOCUS_MODE
	case CFG_SET_FOCUS_MODE:
#ifdef F_SKYCAM_MODEL_EF12S
		/* MV9335 don't have focus mode, and we don't have UI menu 
		 * for it. Anyway always use AUTO mode.*/
		rc = 0;
#else
		rc = mv9335_set_focus_mode(cfg_data.cfg.focus_mode);
#endif
		SKYCDBG("%s: CFG_SET_FOCUS_MODE, rc=%d\n", __func__, rc);
		break;
#endif
#ifdef F_SKYCAM_FIX_CFG_PREVIEW_FPS
	case CFG_SET_PREVIEW_FPS:
		rc = mv9335_set_preview_fps(cfg_data.cfg.preview_fps);
		SKYCDBG("%s: CFG_SET_PREVIEW_FPS, rc=%d\n", __func__, rc);
		break;
#endif

	default:
		rc = -EINVAL;
		SKYCERR("%s#%d: -EINVAL.\n", __func__, __LINE__);
		break;
	}

	/* up(&mv9335_sem); */

	return rc;
}

int32_t mv9335_sensor_release(void)
{
	int32_t rc = 0;

	SKYCDBG("%s is called.\n", __func__);

	/* CAMIOs already have been unregistered, so you can't turn off
	 * LED or reset AF actuator here. */

	rc = mv9335_power_off();
	if (rc < 0)
		return rc;

	/* down(&mv9335_sem); */
	kfree(mv9335_ctrl);
	mv9335_ctrl = NULL;
	/* up(&mv9335_sem); */

	SKYCDBG("%s: done.\n", __func__);
	return 0;
}

static int32_t mv9335_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int32_t rc = 0;

	SKYCDBG("%s is called.\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		SKYCERR("%s#%d: -ENOTSUPP.\n", __func__, __LINE__);
		rc = -ENOTSUPP;
		goto probe_fail;
	}

	mv9335_sensorw = kzalloc(sizeof(struct mv9335_work), GFP_KERNEL);

	if (!mv9335_sensorw) {
		SKYCERR("%s#%d: -ENOMEM.\n", __func__, __LINE__);
		rc = -ENOMEM;
		goto probe_fail;
	}

	i2c_set_clientdata(client, mv9335_sensorw);
	init_waitqueue_head(&mv9335_wait_queue);
	mv9335_client = client;

	SKYCDBG("%s: done.\n", __func__);
	return 0;

probe_fail:
	kfree(mv9335_sensorw);
	mv9335_sensorw = NULL;
	return rc;
}

static int32_t __exit mv9335_i2c_remove(struct i2c_client *client)
{
	struct mv9335_work *sensorw = i2c_get_clientdata(client);

	SKYCDBG("%s is called.\n", __func__);

	free_irq(client->irq, sensorw);
	mv9335_client = NULL;
	mv9335_sensorw = NULL;
	kfree(sensorw);

	SKYCDBG("%s: done.\n", __func__);
	return 0;
}

static const struct i2c_device_id mv9335_i2c_id[] = {
	{"mv9335", 0},
	{ },
};

static struct i2c_driver mv9335_i2c_driver = {
	.id_table = mv9335_i2c_id,
	.probe  = mv9335_i2c_probe,
	.remove = __exit_p(mv9335_i2c_remove),
	.driver = {
		.name = "mv9335",
	},
};

static int mv9335_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{
	int rc = 0;

	SKYCDBG("%s is called.\n", __func__);

	if (!info) {
		SKYCERR("%s#%d: -EFAULT.\n", __func__, __LINE__);
		return -EFAULT;
	}

	mv9335_ctrl = kzalloc(sizeof(struct mv9335_ctrl_t), GFP_KERNEL);
	if (!mv9335_ctrl) {
		SKYCERR("%s#%d: -ENOMEM.\n", __func__, __LINE__);
		rc = -ENOMEM;
		goto probe_init_fail;
	}

	mv9335_ctrl->sensordata = (struct msm_camera_sensor_info *)info;

	rc = i2c_add_driver(&mv9335_i2c_driver);
	if (rc < 0 || mv9335_client == NULL) {
		SKYCERR("%s#%d: -ENOTSUPP.\n", __func__, __LINE__);
		rc = -ENOTSUPP;
		goto probe_init_fail;
	}

#ifdef F_SKYCAM_MODEL_EF12S
	/* MADATORY: MCLK should be enabled before supplying power. */
#ifndef TEMP_POWERSEQ
	msm_camio_clk_rate_set(MV9335_CFG_SCLK);
	mdelay(5);
#endif
#endif

	rc = mv9335_power_on();
	if (rc)
		goto probe_init_fail;

#ifndef F_SKYCAM_MODEL_EF12S
	/* Input MCLK = 24MHz. */
	msm_camio_clk_rate_set(MV9335_CFG_SCLK);
	/* MANDATORY: 5ms. */
	mdelay(5);
#endif

#ifdef TEMP_POWERSEQ
{
	const struct msm_camera_sensor_info *sinfo;
	int32_t myrc = 0;

	mdelay(5);

	sinfo = mv9335_ctrl->sensordata;

	myrc = gpio_request(sinfo->sensor_reset, "mv9335");
	if (myrc < 0) {
		SKYCERR("%s#%d: error. rc=%d\n", __func__, __LINE__, myrc);
		goto probe_init_fail;
	}

	myrc = gpio_direction_output(sinfo->sensor_reset, 1);
	if (myrc < 0) {
		SKYCERR("%s#%d: error. rc=%d\n", __func__, __LINE__, myrc);
		goto probe_init_fail;
	}

	mdelay(5);

	myrc = gpio_direction_output(sinfo->sensor_reset, 0);
	if (myrc < 0) {
		SKYCERR("%s#%d: error. rc=%d\n", __func__, __LINE__, myrc);
		goto probe_init_fail;
	}

	mdelay(5);

	/* 1.8V_CAM_IO is controlled by a switch. */
	myrc = gpio_request(GPIO_MV9335_IO_VREG_EN, "mv9335");
	if (!myrc) {
		myrc = gpio_direction_output(GPIO_MV9335_IO_VREG_EN, 1);
	} else {
		SKYCERR("%s#%d: error.\n", __func__, __LINE__);
		goto probe_init_fail;
	}
	gpio_free(GPIO_MV9335_IO_VREG_EN);

	mdelay(5);

{
	struct vreg *vreg = NULL;
	vreg = vreg_get(NULL, "rftx");
	if (IS_ERR(vreg)) {
		SKYCERR("%s#%d: -ENOENT\n", __func__, __LINE__);
		rc = -ENOENT;
		goto probe_init_fail;
	}

	myrc = vreg_set_level(vreg, 1800);
	if (myrc) {
		SKYCERR("%s#%d: error. myrc=%d\n", __func__, __LINE__, myrc);
		goto probe_init_fail;
	}

	myrc = vreg_enable(vreg);
	if (myrc) {
		SKYCERR("%s#%d: error. myrc=%d\n", __func__, __LINE__, myrc);
		goto probe_init_fail;
	}
}
{
	struct vreg *vreg = NULL;
	vreg = vreg_get(NULL, "wlan");
	if (IS_ERR(vreg)) {
		SKYCERR("%s#%d: -ENOENT\n", __func__, __LINE__);
		rc = -ENOENT;
		goto probe_init_fail;
	}

	myrc = vreg_set_level(vreg, 2800);
	if (myrc) {
		SKYCERR("%s#%d: error. myrc=%d\n", __func__, __LINE__, myrc);
		goto probe_init_fail;
	}

	myrc = vreg_enable(vreg);
	if (myrc) {
		SKYCERR("%s#%d: error. myrc=%d\n", __func__, __LINE__, myrc);
		goto probe_init_fail;
	}
}

	mdelay(5);

	gpio_free(sinfo->sensor_reset);
	SKYCDBG("trigger reset done\n");
}
#endif

#if defined(F_SKYCAM_MODEL_EF12S) && defined(F_SKYCAM_INVALIDATE_CAMERA_CLIENT)
/* Check whether camera module is connected or not. */
{
	uint8_t alive = 0;

	rc = mv9335_reset();
	if (rc < 0)
		goto probe_init_fail;

	rc = mv9335_i2c_read(mv9335_client->addr, REG_MV9335_HW_ID, &alive);
	if ((rc < 0) || (alive != MV9335_HW_ID)) {
		rc = -ENODEV;
		goto probe_init_fail;
	}
}
#endif

	rc = mv9335_sensor_init_probe();
	if ((rc < 0) && (rc != -ETIMEDOUT))
		goto probe_init_fail;

	/* Identify firmware version and update if necessary. */
	if ((mv9335_ver.hw_id != MV9335_HW_ID) ||
	    (mv9335_ver.hw_ver != MV9335_HW_VER) ||
	    (mv9335_ver.fw_major_ver != MV9335_FW_MAJOR_VER) ||
	    (mv9335_ver.fw_minor_ver != MV9335_FW_MINOR_VER))
#ifdef F_SKYCAM_ADD_CFG_UPDATE_ISP
//		;
		rc = mv9335_update_fw_boot();
		if (rc < 0)
			goto probe_init_fail;
#endif

	s->s_init = mv9335_sensor_init;
	s->s_release = mv9335_sensor_release;
	s->s_config = mv9335_sensor_config;

	rc = mv9335_power_off();
	if (rc < 0)
		goto probe_init_fail;

	if (mv9335_ctrl) {
		kfree(mv9335_ctrl);
		mv9335_ctrl = NULL;
	}

	SKYCDBG("%s: done.\n", __func__);
	return 0;

probe_init_fail:
#ifdef F_SKYCAM_MODEL_EF12S
	/* 'mv9335_gpio_on()' needs 'mv9335_ctrl'. */
	(void)mv9335_power_off();
#endif
	if (mv9335_ctrl) {
		kfree(mv9335_ctrl);
		mv9335_ctrl = NULL;
	}
#ifndef F_SKYCAM_MODEL_EF12S
	(void)mv9335_power_off();
#endif
	SKYCERR("%s#%d: error. rc=%d\n", __func__, __LINE__, rc);
	return rc;
}

static int __mv9335_probe(struct platform_device *pdev)
{
	SKYCDBG("%s is called.\n", __func__);
	return msm_camera_drv_start(pdev, mv9335_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __mv9335_probe,
	.driver = {
		.name = "msm_camera_mv9335",
		.owner = THIS_MODULE,
	},
};

static int __init mv9335_init(void)
{
	SKYCDBG("%s is called.\n", __func__);
	return platform_driver_register(&msm_camera_driver);
}

void mv9335_exit(void)
{
	SKYCDBG("%s is called.\n", __func__);
	i2c_del_driver(&mv9335_i2c_driver);
}

module_init(mv9335_init);
module_exit(mv9335_exit);
