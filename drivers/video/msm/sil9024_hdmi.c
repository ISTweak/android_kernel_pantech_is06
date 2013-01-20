//=============================================================================
// File       : sil9024_hdmi.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2009/09/21      leecy          Draft
//=============================================================================


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <mach/hardware.h>
#include <linux/io.h>

#include <asm/system.h>
#include <asm/mach-types.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/clk.h>

#include <linux/pm_qos_params.h>

#include "msm_fb.h"
#include "msm_fb_panel.h"

#include <sil9024_dev.h>

//#define SKY_LIVED_HDMI


#define SKY_HDMI_720P

#ifdef SKY_HDMI_720P
#define SKY_HDMI_DIMENSION_WIDTH      1280
#define SKY_HDMI_DIMENSION_HEIGHT      720
#elif defined(SKY_HDMI_480P)
#define SKY_HDMI_DIMENSION_WIDTH      720
#define SKY_HDMI_DIMENSION_HEIGHT     480
#endif


static struct clk *mdp_lcdc_pclk_clk;  //for HDMI
static struct clk *mdp_lcdc_pad_pclk_clk; //for HDMI

static int mdp_lcdc_pclk_clk_rate_old;
static int mdp_lcdc_pad_pclk_clk_rate_old;

#define USE_ATTRIBUTE

#ifdef USE_ATTRIBUTE
static int connection_state_flag = 0;
#endif

//static int mdp_lcdc_pclk_clk_rate;
//static int mdp_lcdc_pad_pclk_clk_rate;
#ifdef CONFIG_FB_MSM_LCDC
extern int mdp_lcdc_pclk_clk_rate;
extern int mdp_lcdc_pad_pclk_clk_rate;
#endif


extern int sky_hdmi_start(struct platform_device *dev);
extern int sky_hdmi_stop(struct platform_device *dev);
static int sky_hdmi_off(struct platform_device *pdev);
static int sky_hdmi_on(struct platform_device *pdev);


//static struct lcdc_platform_data *lcdc_pdata;

static int sky_hdmi_on(struct platform_device *pdev)
{
    //int ret = 0;
    struct msm_fb_data_type *mfd;
	unsigned long panel_pixclock_freq , pm_qos_freq;

    printk("%s  \n", __FUNCTION__);

    //tl2796_screen_off();    // AM-OLED screen off

    mfd = platform_get_drvdata(pdev);
    panel_pixclock_freq = mfd->fbi->var.pixclock;

    printk("%s : panel_pixclock_freq %d  \n", __FUNCTION__, mfd->fbi->var.pixclock/100);

	mdp_lcdc_pclk_clk_rate_old= clk_get_rate(mdp_lcdc_pclk_clk);
	mdp_lcdc_pad_pclk_clk_rate_old = clk_get_rate(mdp_lcdc_pad_pclk_clk);

     printk("%s : panel_pixclock_freq %d, %d  \n", __FUNCTION__, mdp_lcdc_pclk_clk_rate_old/100, mdp_lcdc_pad_pclk_clk_rate_old/100);

#if 1
	if (panel_pixclock_freq > 62000000)
		/* pm_qos_freq should be in Khz */
		pm_qos_freq = panel_pixclock_freq / 1000 ;
	else
		pm_qos_freq = 62000;

	pm_qos_update_requirement(PM_QOS_SYSTEM_BUS_FREQ , "lcdc",
						pm_qos_freq);
#endif

	//mfd = platform_get_drvdata(pdev);

#ifdef SKY_LIVED_HDMI
	clk_enable(mdp_lcdc_pclk_clk);
	clk_enable(mdp_lcdc_pad_pclk_clk);
    printk("%s  1\n", __FUNCTION__);

#if 0
	if (lcdc_pdata && lcdc_pdata->lcdc_power_save)
		lcdc_pdata->lcdc_power_save(1);
	if (lcdc_pdata && lcdc_pdata->lcdc_gpio_config)
		ret = lcdc_pdata->lcdc_gpio_config(1);
#endif
	clk_set_rate(mdp_lcdc_pclk_clk, mfd->fbi->var.pixclock);
    clk_set_rate(mdp_lcdc_pad_pclk_clk, mfd->fbi->var.pixclock);

    printk("%s  2\n", __FUNCTION__);

	mdp_lcdc_pclk_clk_rate = clk_get_rate(mdp_lcdc_pclk_clk);
	mdp_lcdc_pad_pclk_clk_rate = clk_get_rate(mdp_lcdc_pad_pclk_clk);

    printk("%s: mdp_lcdc_pclk_clk_rate : %d \n", __FUNCTION__, mdp_lcdc_pclk_clk_rate/100);
    // TODO :: LCDC clk change........
    //sky_hdmi_start(pdev);
    sky_hdmi_switch(1);
#endif  // SKY_LIVED_HDMI
   return 0;
}

static int sky_hdmi_off(struct platform_device *pdev)
{
   printk("%s  \n", __FUNCTION__);

#ifdef SKY_LIVED_HDMI
   sky_hdmi_stop(pdev);     
#endif
   // TODO :: LCDC clk change........
   clk_set_rate(mdp_lcdc_pclk_clk, 24576 * 1000 - 500);
   clk_set_rate(mdp_lcdc_pad_pclk_clk, 24576 * 1000 - 500);

   mdp_lcdc_pclk_clk_rate_old= clk_get_rate(mdp_lcdc_pclk_clk);
   mdp_lcdc_pad_pclk_clk_rate_old = clk_get_rate(mdp_lcdc_pad_pclk_clk);

   return 0;
}

#ifdef USE_ATTRIBUTE
static ssize_t sky_hdmi_show_connection_state(struct device *dev, 
                     struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%d\n", connection_state_flag);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(connection_state, 0664, sky_hdmi_show_connection_state, NULL);

static struct attribute *sky_hdmi_attrs[] = {
   &dev_attr_connection_state.attr,
	 NULL,
};

static struct attribute_group sky_hdmi_attr_grp = {
	.attrs = sky_hdmi_attrs,
};

void set_connection_cable_flag(int connect)
{
    connection_state_flag = connect;
}

static int sky_hdmi_sysfs_init(struct platform_device *pdev)
{
	if(!sysfs_create_group(&pdev->dev.kobj, &sky_hdmi_attr_grp))
		pr_info("Created the sky hdmi sysfs entry successfully \n");

	return 0;
}
#endif

static int __init sky_hdmi_probe(struct platform_device *pdev)
{

   printk("%s start.  pdev->name %s \n", __FUNCTION__, pdev->name);//func debug....
   msm_fb_add_device(pdev);

//   sky_hdmi_drv_init();
//   sky_hdmi_test_start(); //for test....  

	mdp_lcdc_pclk_clk = clk_get(NULL, "mdp_lcdc_pclk_clk");
	if (IS_ERR(mdp_lcdc_pclk_clk)) {
		printk(KERN_ERR "error: can't get mdp_lcdc_pclk_clk!\n");
		return IS_ERR(mdp_lcdc_pclk_clk);
	}
	mdp_lcdc_pad_pclk_clk = clk_get(NULL, "mdp_lcdc_pad_pclk_clk");
	if (IS_ERR(mdp_lcdc_pad_pclk_clk)) {
		printk(KERN_ERR "error: can't get mdp_lcdc_pad_pclk_clk!\n");
		return IS_ERR(mdp_lcdc_pad_pclk_clk);
	}

#ifdef USE_ATTRIBUTE
	sky_hdmi_sysfs_init(pdev);
#endif

   printk("%s end\n", __FUNCTION__);//func debug....
   return 0;
}

static struct platform_driver this_driver = {
	.probe  = sky_hdmi_probe,
	.driver = {
		.name   = "sky_hdmi",
	},
};

static struct msm_fb_panel_data sky_hdmi_panel_data = {
	.on = sky_hdmi_on,
	.off = sky_hdmi_off,
};

static struct platform_device this_device = {
	.name   = "sky_hdmi",
	.id	= 0,
	.dev	= {
		.platform_data = &sky_hdmi_panel_data,
	}
};

static int __init sky_hdmi_init(void)
{
   int ret;
   struct msm_panel_info pinfo;

#ifdef SKY_HDMI_480P
//480p 
   pinfo.xres = 720;
   pinfo.yres = 480;
   pinfo.type = LCDC_PANEL;
   pinfo.pdest = DISPLAY_1;
   pinfo.bl_max = 8;
   pinfo.wait_cycle = 0;
   pinfo.bpp = 24;
   pinfo.fb_num = 2;
   pinfo.clk_rate = 27027 * 1000 - 500;

   pinfo.lcdc.h_back_porch =60;
   pinfo.lcdc.h_front_porch = 19;
   pinfo.lcdc.h_pulse_width = 62;
   pinfo.lcdc.v_back_porch = 30;
   pinfo.lcdc.v_front_porch = 9;
   pinfo.lcdc.v_pulse_width = 6;
   pinfo.lcdc.border_clr = 0;  /* blk */
   pinfo.lcdc.underflow_clr = 0xff;  /* blue */
   pinfo.lcdc.hsync_skew = 0;
#else
//720p
    pinfo.xres = 1280;
    pinfo.yres = 720;
    pinfo.type = LCDC_PANEL;
    pinfo.pdest = DISPLAY_1;
    pinfo.bl_max = 8;
    pinfo.wait_cycle = 0;
    pinfo.bpp = 24;
    pinfo.fb_num = 2;
    pinfo.clk_rate = 74250 * 1000 - 500;

    pinfo.lcdc.h_back_porch =220; //81
    pinfo.lcdc.h_front_porch = 109;//110; //81 
    pinfo.lcdc.h_pulse_width = 41;//40; //60
    pinfo.lcdc.v_back_porch = 20;
    pinfo.lcdc.v_front_porch = 5;
    pinfo.lcdc.v_pulse_width = 5;
    pinfo.lcdc.border_clr = 0;	/* blk */
    pinfo.lcdc.underflow_clr = 0xff;	/* blue */
    pinfo.lcdc.hsync_skew = 0;

#endif

   sky_hdmi_panel_data.panel_info = pinfo;

   ret = platform_driver_register(&this_driver);
   if (!ret) {
         ret = platform_device_register(&this_device);
         if (ret)
            platform_driver_unregister(&this_driver);
   }

   printk("sky_hdmi_init ret val %d!!\n", ret);

   return ret;
}



#if 1
//static struct lcdc_platform_data *lcdc_pdata;

static int sky_hdmi_480p_on(struct platform_device *pdev)
{
    //int ret = 0;
    struct msm_fb_data_type *mfd;
    unsigned long panel_pixclock_freq , pm_qos_freq;

    printk("%s  \n", __FUNCTION__);

    mfd = platform_get_drvdata(pdev);
    panel_pixclock_freq = mfd->fbi->var.pixclock;

    printk("%s : panel_pixclock_freq %d  \n", __FUNCTION__, mfd->fbi->var.pixclock/100);

    mdp_lcdc_pclk_clk_rate_old= clk_get_rate(mdp_lcdc_pclk_clk);
    mdp_lcdc_pad_pclk_clk_rate_old = clk_get_rate(mdp_lcdc_pad_pclk_clk);

    printk("%s : panel_pixclock_freq %d, %d  \n", __FUNCTION__, mdp_lcdc_pclk_clk_rate_old/100, mdp_lcdc_pad_pclk_clk_rate_old/100);

    if (panel_pixclock_freq > 62000000)
        /* pm_qos_freq should be in Khz */
        pm_qos_freq = panel_pixclock_freq / 1000 ;
    else
        pm_qos_freq = 62000;

    pm_qos_update_requirement(PM_QOS_SYSTEM_BUS_FREQ , "lcdc", pm_qos_freq);

#ifdef SKY_LIVED_HDMI
    clk_enable(mdp_lcdc_pclk_clk);
    clk_enable(mdp_lcdc_pad_pclk_clk);
    printk("%s  1\n", __FUNCTION__);

#if 0
    if (lcdc_pdata && lcdc_pdata->lcdc_power_save)
        lcdc_pdata->lcdc_power_save(1);
    if (lcdc_pdata && lcdc_pdata->lcdc_gpio_config)
        ret = lcdc_pdata->lcdc_gpio_config(1);
#endif
    clk_set_rate(mdp_lcdc_pclk_clk, mfd->fbi->var.pixclock);
    clk_set_rate(mdp_lcdc_pad_pclk_clk, mfd->fbi->var.pixclock);

    printk("%s  2\n", __FUNCTION__);

    mdp_lcdc_pclk_clk_rate = clk_get_rate(mdp_lcdc_pclk_clk);
    mdp_lcdc_pad_pclk_clk_rate = clk_get_rate(mdp_lcdc_pad_pclk_clk);

    printk("%s: mdp_lcdc_pclk_clk_rate : %d \n", __FUNCTION__, mdp_lcdc_pclk_clk_rate/100);
    // TODO :: LCDC clk change........
    //sky_hdmi_start(pdev);
    sky_hdmi_switch(2);
#endif  // SKY_LIVED_HDMI
    return 0;
}

static int sky_hdmi_480p_off(struct platform_device *pdev)
{
   printk("%s  \n", __FUNCTION__);

#ifdef SKY_LIVED_HDMI
   sky_hdmi_stop(pdev);     
#endif
   // TODO :: LCDC clk change........
   clk_set_rate(mdp_lcdc_pclk_clk, 24576 * 1000 - 500);
   clk_set_rate(mdp_lcdc_pad_pclk_clk, 24576 * 1000 - 500);

   mdp_lcdc_pclk_clk_rate_old= clk_get_rate(mdp_lcdc_pclk_clk);
   mdp_lcdc_pad_pclk_clk_rate_old = clk_get_rate(mdp_lcdc_pad_pclk_clk);

   return 0;
}

static int __init sky_hdmi_480p_probe(struct platform_device *pdev)
{
    printk("%s start.  pdev->name %s \n", __FUNCTION__, pdev->name);//func debug....
    msm_fb_add_device(pdev);

//    sky_hdmi_drv_init();
    //   sky_hdmi_test_start(); //for test....  

    mdp_lcdc_pclk_clk = clk_get(NULL, "mdp_lcdc_pclk_clk");
    if (IS_ERR(mdp_lcdc_pclk_clk)) {
        printk(KERN_ERR "error: can't get mdp_lcdc_pclk_clk!\n");
        return IS_ERR(mdp_lcdc_pclk_clk);
    }
    mdp_lcdc_pad_pclk_clk = clk_get(NULL, "mdp_lcdc_pad_pclk_clk");

    if (IS_ERR(mdp_lcdc_pad_pclk_clk)) {
        printk(KERN_ERR "error: can't get mdp_lcdc_pad_pclk_clk!\n");
        return IS_ERR(mdp_lcdc_pad_pclk_clk);
    }

    printk("%s end\n", __FUNCTION__);//func debug....
    return 0;
}

static struct platform_driver this_480p_driver = {
	.probe  = sky_hdmi_480p_probe,
	.driver = {
		.name   = "sky_hdmi_480p",
	},
};

static struct msm_fb_panel_data sky_hdmi_480p_panel_data = {
	.on = sky_hdmi_480p_on,
	.off = sky_hdmi_480p_off,
};

static struct platform_device this_480p_device = {
	.name   = "sky_hdmi_480p",
	.id	= 0,
	.dev	= {
		.platform_data = &sky_hdmi_480p_panel_data,
	}
};

static int __init sky_hdmi_480p_init(void)
{
   int ret;
   struct msm_panel_info pinfo;

//480p 
   //pinfo.xres = 640;
   pinfo.xres = 720;
   pinfo.yres = 480;
   pinfo.type = HDMI_PANEL;
   pinfo.pdest = DISPLAY_1;
   pinfo.bl_max = 8;
   pinfo.wait_cycle = 0;
   pinfo.bpp = 24;
   pinfo.fb_num = 2;
   //pinfo.clk_rate = 25175 * 1000 - 500;
   pinfo.clk_rate = 27027 * 1000 - 500;

#if 0
   pinfo.lcdc.h_back_porch =48;//60;
   pinfo.lcdc.h_front_porch = 16;
   pinfo.lcdc.h_pulse_width = 96;//62;
   pinfo.lcdc.v_back_porch = 33;//30;
   pinfo.lcdc.v_front_porch = 10;//9;
   pinfo.lcdc.v_pulse_width = 2; //6;
#else
   pinfo.lcdc.h_back_porch =60;
   pinfo.lcdc.h_front_porch = 16;
   pinfo.lcdc.h_pulse_width = 62;
   pinfo.lcdc.v_back_porch = 30;
   pinfo.lcdc.v_front_porch = 9;
   pinfo.lcdc.v_pulse_width = 6;
#endif
   pinfo.lcdc.border_clr = 0;  /* blk */
   pinfo.lcdc.underflow_clr = 0xff;  /* blue */
   pinfo.lcdc.hsync_skew = 0;

   sky_hdmi_480p_panel_data.panel_info = pinfo;

   ret = platform_driver_register(&this_480p_driver);
   if (!ret) {
         ret = platform_device_register(&this_480p_device);
         if (ret)
            platform_driver_unregister(&this_480p_driver);
   }

   printk("sky_hdmi_480p_init ret val %d!!\n", ret);

   return ret;
}

module_init(sky_hdmi_480p_init);
module_init(sky_hdmi_init);

#endif

