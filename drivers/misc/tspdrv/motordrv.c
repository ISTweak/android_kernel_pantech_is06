/* Copyright (c) 2009-2010, Code PANTECH. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <asm/irq.h>
#include <linux/miscdevice.h>
#include <mach/hardware.h>
#include <mach/io.h>
#include <mach/system.h>
#include <mach/gpio.h>
#include "motordrv.h"
#ifdef APDS9801_POWER_PROCESS_ENABLE
#include <mach/vreg.h>
#include <mach/mpp.h>
#endif

#include <mach/sky_smem.h>


//#define DBG_ENABLE
#ifdef DBG_ENABLE
#define dbg(fmt, args...) printk(fmt, ##args)
#else
#define dbg(fmt, args...)
#endif

static _pwm_type pwm;

unsigned long motor_read_mdr(void);
unsigned long motor_read_nsr(void);

void motor_disable(void)
{
  sky_smem_pmic_vib_mot_control(0);
#if (BOARD_VER == EF12_WS10)
	 gpio_set_value(GPIO_MOTOR_EN, 0);
#endif

	dbg(KERN_ERR "[SKY MOTOR] motor_disble\n");
}

void motor_enable(void)
{
   sky_smem_pmic_vib_mot_control(1200);
#if (BOARD_VER == EF12_WS10)
   gpio_set_value(GPIO_MOTOR_EN, 1);
#endif
	dbg(KERN_ERR "[SKY MOTOR] motor_enable\n");
}


void motor_gen_pwm(uint32_t destClk)
{
	uint32_t m,n;
	uint32_t crntGap, savedGap=0xFFFFFFFF;
	uint32_t crntDestClk, savedDestClk;
	uint32_t tmpM=GP_M_VAL_MIN, tmpN=GP_N_VAL_MIN;

	for(m=GP_M_VAL_MIN; m<=GP_M_VAL_MAX; m++)
	{
		for(n=m; n<=GP_N_VAL_MAX; n++)
		{
			crntDestClk = ((GP_CLK_SRC/GP_PREV_DIV) / n ) * m;
			crntGap = abssub(crntDestClk, GP_CLK_DEST(destClk));

			if(crntGap < savedGap)
			{
				tmpM = m;
				tmpN = n;
				savedDestClk = crntDestClk;
				savedGap = crntGap;
			} 
			if(!savedGap) break;
		}
		if(!savedGap) break;
	}

	pwm.rate = 100;
	pwm.valM = tmpM;
	pwm.valN = tmpN;
	pwm.valD = tmpN >> 1;	// 50% duty cycle
	pwm.multiplier = pwmmul(tmpM, tmpN);

	dbg("[GEN_PWM] valM=%d | valN=%d | valD=%d | mul=%d\n", pwm.valM, pwm.valN, pwm.valD, pwm.multiplier);

	pwm.mdr = ( (pwm.valM<<16)&0xFFFF0000) | ((~(pwm.valD<<1))&0x0000FFFF );
	pwm.nsr = ( ( (~(pwm.valN - pwm.valM)) << 16 ) & 0xFFFF0000);
	pwm.nsr|= (1 << 11);	// GP_ROOT_ENABLE
	pwm.nsr|= (0 << 10);	// GP_CLK_NOT_INVERTED
	pwm.nsr|= (1 << 9);		// GP_CLK_BRANCH_ENABLE
	pwm.nsr|= (1 << 8);		// MNCNTR_ENABLE
	pwm.nsr|= (0 << 7);		// MNCNTR_RST_INACTIVE
	pwm.nsr|= (3 << 5);		// MNCNTR_SINGLEEDGE_MODE
	pwm.nsr|= (0 << 3);		// PRE_DIV_SEL_BYPASS
	pwm.nsr|= (0 << 0);		// SRC_SEL_TCXO

	dbg("[GEN_PWM] mdr=0x%08X | nsr=0x%08X\n", pwm.mdr, pwm.nsr);
}

unsigned long motor_read_mdr(void)
{
	return HWIO_GP_MD_REG_ADDR;
}

unsigned long motor_read_nsr(void)
{
	return HWIO_GP_NS_REG_ADDR;
}


void motor_set_pwm(signed char nForce)
{
	signed int nForce32;

		dbg(KERN_ERR "[SKY MOTOR] motor_set_pwm(%d)\n",nForce);

	nForce32 = (nForce < 0) ? -1 : 1;
	nForce32 *= nForce;
	nForce32 *= pwm.multiplier;
	nForce32 = nForce32 >> 8;
	nForce32 = nForce32 * pwm.rate / 100;
	nForce32 = (nForce < 0) ? (pwm.valD - nForce32) : (pwm.valD + nForce32);
	dbg("nForce[%d],M[%d],N[%d],D[%d],mul[%d],rate[%d],nForce32[%d]\n", nForce, pwm.valM, pwm.valN, pwm.valD, pwm.multiplier, pwm.rate, nForce32);

	pwm.mdr = ( (pwm.valM<<16)&0xFFFF0000) | ((~(nForce32<<1))&0x0000FFFF );
	HWIO_GP_MD_REG_ADDR = pwm.mdr;
	HWIO_GP_NS_REG_ADDR = pwm.nsr;
	dbg("[SET_PWM] MDR[0x%08X] | NSR[0x%08X]\n", pwm.mdr, pwm.nsr);
}

int  motor_init(void)
{
#if (BOARD_VER == EF12_WS10)
	unsigned gpioConfig;
	int rc;

	/* gpio config */
	gpioConfig = GPIO_CFG(GPIO_MOTOR_EN, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA);
	rc = gpio_tlmm_config(gpioConfig, GPIO_ENABLE);
	if(rc) goto gpio_tlmm_config_err;
#endif

	 dbg(KERN_ERR "[SKY MOTOR] motor_init\n");
	return 1;

#if (BOARD_VER == EF12_WS10)
gpio_tlmm_config_err:
	dbg("gpio_tlmm_config failed.\n");
	return 0;
#endif
}

static int drvargToForce(unsigned long da)
{
	int force;

	if(da & 0x00FF0000) force = (-1)*da;
	else force = da;

	return force;
	
}

void motor_set_rate(uint32_t rate)
{
	pwm.rate = rate;
	if(pwm.rate > 100) pwm.rate = 100;
	else if(pwm.rate < 0 ) pwm.rate = 0;
}

uint32_t motor_get_rate(void)
{
	return pwm.rate;
}
///////////////////////////////////////////////////////////////////////////
// app 에서 write 시 호출
static ssize_t store_vibration_rate(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	uint32_t rate;
	dbg("[motor setup] <pre> rate = %d\n", pwm.rate);
	sscanf(buf, "%d\n", &rate);
	dbg("[motor setup] <post> rate = %d\n", rate);
	motor_set_rate(rate);
	return count;
}

// app 에서 read 시 호출
static ssize_t show_vibration_rate(struct device *dev, struct device_attribute *attr, char *buf) {
	int count;
	count = sprintf(buf, "%d\n", pwm.rate);
	return count;
}

static DEVICE_ATTR(motorrate, S_IRUGO | S_IWUSR, show_vibration_rate, store_vibration_rate);

static struct attribute *dev_attrs[] = {
	&dev_attr_motorrate.attr,
	NULL,
};

static struct attribute_group dev_attr_grp = {
	.attrs = dev_attrs,
};
///////////////////////////////////////////////////////////////////////////

static int motor_misc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int motor_misc_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int motor_misc_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	unsigned long reg;

	switch (cmd)
	{
dbg(KERN_ERR "motor_misc_ioctl cmd=%d\n", cmd);
		case MOTOR_IOCTL_CMD_INIT:
			dbg("MOTOR_IOCTL_CMD_INIT\n");
			motor_init();
			break;
		case MOTOR_IOCTL_CMD_DISABLE:
			dbg("MOTOR_IOCTL_CMD_DISABLE\n");
			motor_disable();
			break;
		case MOTOR_IOCTL_CMD_ENABLE:
			dbg("MOTOR_IOCTL_CMD_ENABLE\n");
			motor_enable();
			break;
		case MOTOR_IOCTL_CMD_SET_PWM:
			dbg("MOTOR_IOCTL_CMD_SET_PWM(%d)\n",drvargToForce((int)argp));
			motor_set_pwm(drvargToForce((int)argp));
			break;
		case MOTOR_IOCTL_CMD_GEN_PWM:
			dbg("MOTOR_IOCTL_CMD_GEN_PWM(%d)\n",(int)argp);
			motor_gen_pwm((int)argp);
			break;
		case MOTOR_IOCTL_CMD_SET_RATE:
			dbg("MOTOR_IOCTL_CMD_SET_RATE(%d)\n",(int)argp);
			motor_set_rate((int)argp);
			break;
		case MOTOR_IOCTL_CMD_READ_MDR:
			dbg("MOTOR_IOCTL_CMD_READ_MDR\n");
			reg = motor_read_mdr();
			if(copy_to_user(argp, &reg, sizeof(unsigned long))) return -EFAULT;
			break;
		case MOTOR_IOCTL_CMD_READ_NSR:
			dbg("MOTOR_IOCTL_CMD_READ_NSR\n");
			reg = motor_read_nsr();
			if(copy_to_user(argp, &reg, sizeof(unsigned long))) return -EFAULT;
			break;
		default:
			return -ENOTTY;
	}

	return 0;
}

static struct file_operations motor_misc_fops = {
	.owner = THIS_MODULE,
	.open = motor_misc_open,
	.release = motor_misc_release,
	.ioctl = motor_misc_ioctl,
};

static struct miscdevice motor_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "motordrv",
	.fops = &motor_misc_fops,
};

///////////////////////////////////////////////////////////////////////////

static struct platform_device motor_plat_device = {
	.name = "motordrv",
	.id = -1,
	.dev = {
		.platform_data = NULL,
		.release = NULL,
	},
};

///////////////////////////////////////////////////////////////////////////

static int __init motordrv_init(void) 
{
	int rc;

dbg(KERN_ERR "motordrv_init\n");
	rc = misc_register(&motor_misc_device);
	if (rc)
		dbg(KERN_ERR "motordrv_init FAILED: motor_misc_device rc=%d\n", rc);

	rc = platform_device_register(&motor_plat_device);
	if (rc)
		dbg(KERN_ERR "motordrv_init FAILED: motor_plat_device rc=%d\n", rc);

	rc = sysfs_create_group(&motor_plat_device.dev.kobj, &dev_attr_grp);
	if(rc)
		dbg(KERN_ERR "motordrv_init FAILED: create dev.attrs rc=%d\n", rc);

	return rc;
}

static void __exit motordrv_exit(void)
{
	sysfs_remove_group(&motor_plat_device.dev.kobj, &dev_attr_grp);
	misc_deregister(&motor_misc_device);
	platform_device_unregister(&motor_plat_device);
}


module_init(motordrv_init);
module_exit(motordrv_exit);
