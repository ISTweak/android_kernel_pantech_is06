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
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <../../../drivers/staging/android/timed_output.h>
#include <linux/sched.h>
#include <mach/msm_rpcrouter.h>
#include <mach/pmic_vibrator.h>

/* -------------------------------------------------------------------- */
/* debug option */
/* -------------------------------------------------------------------- */
//#define DBG_ENABLE
#ifdef DBG_ENABLE
#define dbg(fmt, args...)   printk("[PM_VIBRATOR] " fmt, ##args)
#else
#define dbg(fmt, args...)
#endif
#define dbg_func_in()       dbg("[FUNC_IN] %s\n", __func__)
#define dbg_func_out()      dbg("[FUNC_OUT] %s\n", __func__)
#define dbg_line()          dbg("[LINE] %d(%s)\n", __LINE__, __func__)

#define VIB_INTENSITY_CONFIG_ENABLE // shpark. enable configure of vibrator's intensity

// shpark. 100510 // syncronize with $w33sm/drivers/pmic/pmic3/proc/lib/sw/pm_lib_rpc.c's RPM ID & VER
#define PM_LIBPROG      0x30000061
#define PM_LIBVERS      0x10001

// shpark. 100510 // rpm proc
#define PM_VIB_MOT_SET_VOLT_PROC		22
#define PM_VIB_MOT_SET_MODE_PROC		23
#define PM_VIB_MOT_SET_POLARITY_PROC	24

#ifdef VIB_INTENSITY_CONFIG_ENABLE 
#define VIB_DEFAULT_INTENSITY	3000
#define VIB_MAX_INTENSITY		3100
#define VIB_MIN_INTENSITY		1200
static int intensity = VIB_DEFAULT_INTENSITY;
#else
#define PMIC_VIBRATOR_LEVEL	(3000)
#endif

static int t_ms = 0;
static bool is_vib_running = false;

static struct work_struct work_vibrator_on;
static struct work_struct work_vibrator_off;
static struct hrtimer vibe_timer;

static void set_pmic_vibrator(int on)
{
	dbg_func_in();

	static struct msm_rpc_endpoint *endpoint;
	struct set_vib_on_off_req {
		struct rpc_request_hdr hdr;
		uint32_t data;
	} req;

	dbg_func_in();

	if (!endpoint) {
		endpoint = msm_rpc_connect(PM_LIBPROG, PM_LIBVERS, 0);
		if (IS_ERR(endpoint)) {
			printk(KERN_ERR "init vib rpc failed!\n"); 
			endpoint = 0;
			return;
		}
	}

	if (on) {
		req.data = cpu_to_be32(intensity);
		is_vib_running = true;
	}
	else
	{
		is_vib_running = false;
		req.data = cpu_to_be32(0);
	}
	
	msm_rpc_call(endpoint, PM_VIB_MOT_SET_VOLT_PROC, &req, sizeof(req), 5*HZ);
	hrtimer_cancel(&vibe_timer);
	if(on)
	{
		hrtimer_start(&vibe_timer,
	      ktime_set(t_ms / 1000, (t_ms % 1000) * 1000000),
	      HRTIMER_MODE_REL);
	}


	dbg_func_out();
}

/* 100524 shpark. call this suspend function at android/drivers/leds/leds-i2c.c */
void vibrator_suspend()
{
	printk("is_vib_running = %d\n", is_vib_running);
	if(is_vib_running)
	{
		set_pmic_vibrator(0);
	}
}

static void pmic_vibrator_on(struct work_struct *work)
{
	dbg_func_in();
  if(!is_vib_running)
	{
		set_pmic_vibrator(1);
	}	
	dbg_func_out();
}

static void pmic_vibrator_off(struct work_struct *work)
{
	dbg_func_in();
  if(is_vib_running)
	{
		set_pmic_vibrator(0);
	}	
	dbg_func_out();
}

static void timed_vibrator_on(struct timed_output_dev *sdev)
{
	dbg_func_in();
	schedule_work(&work_vibrator_on);
	dbg_func_out();
}

static void timed_vibrator_off(struct timed_output_dev *sdev)
{
	dbg_func_in();
	schedule_work(&work_vibrator_off);
	dbg_func_out();
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	dbg_func_in();

	dbg("value = 0x%08X\n", value);

	if (value == 0)
	{
		timed_vibrator_off(dev);
	}
	else
	{
		// high 16bit : intensity
		// low 16bit  : timeout
		intensity = (value >> 16) & 0x0000FFFF;	// get 'intensity'
		t_ms = value & 0x0000FFFF;		// get 'timeout'
		if(t_ms == 0) t_ms = 0x7FFFFFFF;
		if(intensity == 0) intensity = VIB_DEFAULT_INTENSITY;
		else if(intensity > VIB_MAX_INTENSITY) intensity = VIB_MAX_INTENSITY;
		else if(intensity < VIB_MIN_INTENSITY) intensity = VIB_MIN_INTENSITY;
	
		timed_vibrator_on(dev);
	}

	dbg_func_out();
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	int rc;

	dbg_func_in();
	if (hrtimer_active(&vibe_timer)) {
		ktime_t r = hrtimer_get_remaining(&vibe_timer);
		rc = r.tv.sec * 1000 + r.tv.nsec / 1000000;
	} else
		rc = 0;

	dbg_func_out();
	return rc;
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	dbg_func_in();
	timed_vibrator_off(NULL);
	dbg_func_out();
	return HRTIMER_NORESTART;
}

static struct timed_output_dev pmic_vibrator = {
	.name = "vibrator",
	.get_time = vibrator_get_time,
	.enable = vibrator_enable,
};

void __init msm_init_pmic_vibrator(void)
{
	dbg_func_in();
	INIT_WORK(&work_vibrator_on, pmic_vibrator_on);
	INIT_WORK(&work_vibrator_off, pmic_vibrator_off);

	hrtimer_init(&vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vibe_timer.function = vibrator_timer_func;

	timed_output_dev_register(&pmic_vibrator);

	dbg_func_out();
}

MODULE_DESCRIPTION("timed output pmic vibrator device");
MODULE_LICENSE("GPL");

