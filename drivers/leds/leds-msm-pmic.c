/*
 * leds-msm-pmic.c - MSM PMIC LEDs driver.
 *
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
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
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/fs.h>

#include <mach/pmic.h>
#include <linux/miscdevice.h>

#define FEATURE_PANTECH_PMIC_CHG

#define MAX_KEYPAD_BL_LEVEL	16

#if defined(FEATURE_PANTECH_PMIC_CHG)
extern char *saved_command_line;
static int isOfflineChargingMode( void );
#endif

#if defined(FEATURE_PANTECH_PMIC_CHG)
static int isOfflineChargingMode( void )
{
  char* command_line = saved_command_line;
	char* ptr = NULL;

	if( (ptr = strstr(command_line, "androidboot.battchg_pause")) == NULL )
	{
	  return 0;
	}

	if( !strcmp(ptr, "androidboot.battchg_pause=true") )
	{
	  return 1;
	}

	return 0;
}
#endif

static void msm_keypad_bl_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	int ret;

#if (BOARD_VER >= EF12_TP20)
	if(value)
		ret = pmic_secure_mpp_config_i_sink(PM_MPP_14, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);
	else
		ret = pmic_secure_mpp_config_i_sink(PM_MPP_14, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
#else
	ret = pmic_set_led_intensity(LED_KEYPAD, value / MAX_KEYPAD_BL_LEVEL);
#endif

	if (ret)
		dev_err(led_cdev->dev, "can't set keypad backlight\n");
}

static struct led_classdev msm_kp_bl_led = {
	.name			= /*"keyboard-backlight"*/"button-backlight",
	.brightness_set		= msm_keypad_bl_led_set,
	.brightness		= LED_OFF,
};

void sky_led_set(enum led_brightness value)
{
	int ret;

#if (BOARD_VER >= EF12_TP20)
	if(value)
		ret = pmic_secure_mpp_config_i_sink(PM_MPP_14, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);
	else
		ret = pmic_secure_mpp_config_i_sink(PM_MPP_14, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
#else
	ret = pmic_set_led_intensity( EF12_LED_B, (value & LED_B_MASK ) );
	ret = pmic_set_led_intensity( EF12_LED_R, ( (value & LED_R_MASK ) >> 4 ) );
#endif

	if (ret != 0)
		printk("LED set fail!\n");

	return;
}

//EXPORT_SYMBOL(sky_led_set);	// lived 2010.05.17

int led_fops_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
	       
static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.ioctl = led_fops_ioctl,
};

static struct miscdevice led_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "led_fops",
	.fops = &led_fops,
};

/*
	ÆÄ¶û(blue) + ³ì»ö(green) = Ã»·Ï(cyan) 
	³ì»ö(green) + »¡°­(red) = ³ë¶û(yellow) 
	ÆÄ¶û(blue) + »¡°­(red) = ÀÚÈ«(magenta) 
	ÆÄ¶û(blue) + ³ì»ö(green) + »¡°­(red) = Èò»ö(white) 
*/

typedef enum {
	HOMELED_OFF = 0x1000, /* KKLIM 20101108, add 0x1000 */
	HOMELED_RED,
	HOMELED_BULE,
	HOMELED_GREEN,
	HOMELED_CYAN,
	HOMELED_YELLOW,
	HOMELED_MAGENTA,
	HOMELED_WHITE
}HOMELED_COLOR;

int led_fops_ioctl(struct inode *inode, struct file *filp,
	       unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;

//	printk("led_fops_ioctl cmd: %d\n", cmd);

#if (BOARD_VER >= JMASAI_PT10)
    /* KKLIM 20101108, remove MPP_13 */
	switch (cmd) 
	{	
		case HOMELED_RED:
			pmic_secure_mpp_config_i_sink(PM_MPP_22, PM_MPP__I_SINK__LEVEL_10mA, PM_MPP__I_SINK__SWITCH_ENA);
			pmic_secure_mpp_config_i_sink(PM_MPP_19, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
			//pmic_secure_mpp_config_i_sink(PM_MPP_13, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);			
			break;
		case HOMELED_BULE:
			pmic_secure_mpp_config_i_sink(PM_MPP_22, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
			pmic_secure_mpp_config_i_sink(PM_MPP_19, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);
			//pmic_secure_mpp_config_i_sink(PM_MPP_13, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);			
			break;
		case HOMELED_GREEN:
			pmic_secure_mpp_config_i_sink(PM_MPP_22, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
			pmic_secure_mpp_config_i_sink(PM_MPP_19, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
			//pmic_secure_mpp_config_i_sink(PM_MPP_13, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);			
			break;
		case HOMELED_CYAN:
			pmic_secure_mpp_config_i_sink(PM_MPP_22, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
			pmic_secure_mpp_config_i_sink(PM_MPP_19, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);
			//pmic_secure_mpp_config_i_sink(PM_MPP_13, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);			
			break;
		case HOMELED_YELLOW:
			pmic_secure_mpp_config_i_sink(PM_MPP_22, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);
			pmic_secure_mpp_config_i_sink(PM_MPP_19, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
			//pmic_secure_mpp_config_i_sink(PM_MPP_13, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);			
			break;
		case HOMELED_MAGENTA:
			pmic_secure_mpp_config_i_sink(PM_MPP_22, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);
			pmic_secure_mpp_config_i_sink(PM_MPP_19, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);
			//pmic_secure_mpp_config_i_sink(PM_MPP_13, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);			
			break;
		case HOMELED_WHITE:
			pmic_secure_mpp_config_i_sink(PM_MPP_22, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);
			pmic_secure_mpp_config_i_sink(PM_MPP_19, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);
			//pmic_secure_mpp_config_i_sink(PM_MPP_13, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);			
			break;			
		default:
			pmic_secure_mpp_config_i_sink(PM_MPP_22, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
			pmic_secure_mpp_config_i_sink(PM_MPP_19, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
			//pmic_secure_mpp_config_i_sink(PM_MPP_13, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);			
	     		break;
	}

#elif (BOARD_VER >= EF12_TP20)
	switch (cmd) 
	{	
		case HOMELED_RED:
			pmic_secure_mpp_config_i_sink(PM_MPP_22, PM_MPP__I_SINK__LEVEL_10mA, PM_MPP__I_SINK__SWITCH_ENA);
			pmic_secure_mpp_config_i_sink(PM_MPP_19, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
			pmic_secure_mpp_config_i_sink(PM_MPP_13, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);			
			break;
		case HOMELED_BULE:
			pmic_secure_mpp_config_i_sink(PM_MPP_22, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
			pmic_secure_mpp_config_i_sink(PM_MPP_19, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);
			pmic_secure_mpp_config_i_sink(PM_MPP_13, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);			
			break;
		case HOMELED_GREEN:
			pmic_secure_mpp_config_i_sink(PM_MPP_22, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
			pmic_secure_mpp_config_i_sink(PM_MPP_19, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
			pmic_secure_mpp_config_i_sink(PM_MPP_13, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);			
			break;
		case HOMELED_CYAN:
			pmic_secure_mpp_config_i_sink(PM_MPP_22, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
			pmic_secure_mpp_config_i_sink(PM_MPP_19, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);
			pmic_secure_mpp_config_i_sink(PM_MPP_13, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);			
			break;
		case HOMELED_YELLOW:
			pmic_secure_mpp_config_i_sink(PM_MPP_22, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);
			pmic_secure_mpp_config_i_sink(PM_MPP_19, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
			pmic_secure_mpp_config_i_sink(PM_MPP_13, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);			
			break;
		case HOMELED_MAGENTA:
			pmic_secure_mpp_config_i_sink(PM_MPP_22, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);
			pmic_secure_mpp_config_i_sink(PM_MPP_19, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);
			pmic_secure_mpp_config_i_sink(PM_MPP_13, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);			
			break;
		case HOMELED_WHITE:
			pmic_secure_mpp_config_i_sink(PM_MPP_22, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);
			pmic_secure_mpp_config_i_sink(PM_MPP_19, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);
			pmic_secure_mpp_config_i_sink(PM_MPP_13, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA);			
			break;			
		default:
			pmic_secure_mpp_config_i_sink(PM_MPP_22, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
			pmic_secure_mpp_config_i_sink(PM_MPP_19, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);
			pmic_secure_mpp_config_i_sink(PM_MPP_13, PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS);			
	     		break;
	}
#endif

 	return true;
}

EXPORT_SYMBOL(led_fops_ioctl);

static int msm_pmic_led_probe(struct platform_device *pdev)
{
#if 0
	int rc;

	rc = led_classdev_register(&pdev->dev, &msm_kp_bl_led);
	if (rc) {
		dev_err(&pdev->dev, "unable to register led class driver\n");
		return rc;
	}
	msm_keypad_bl_led_set(&msm_kp_bl_led, LED_OFF);
	return rc;
#else
	int val, i, j;
	int res = led_classdev_register(&pdev->dev, &msm_kp_bl_led);

  #if (BOARD_VER >= EF12_TP20)
	res = misc_register(&led_device);
	if (res) {
		printk("touch diag register failed\n");
	}
  #endif

#if defined(FEATURE_PANTECH_PMIC_CHG)
  if( !isOfflineChargingMode() )
  {
    val = 0;
  	for(i=0; i<9; i++)
  	{
  		val = i*8 - 1;
  		if(val<0) val = 0;
	  	msm_keypad_bl_led_set((struct led_classdev *)&pdev->dev, val);
		  for(j=0; j<100000; j++) {}
  	}
  }
#else
	val = 0;
	for(i=0; i<9; i++)
	{
		val = i*8 - 1;
		if(val<0) val = 0;
		msm_keypad_bl_led_set((struct led_classdev *)&pdev->dev, val);
		for(j=0; j<100000; j++) {}
	}
#endif
	return res;
#endif
}

static int __devexit msm_pmic_led_remove(struct platform_device *pdev)
{
	led_classdev_unregister(&msm_kp_bl_led);

	return 0;
}

#ifdef CONFIG_PM
static int msm_pmic_led_suspend(struct platform_device *dev,
		pm_message_t state)
{
	led_classdev_suspend(&msm_kp_bl_led);

	return 0;
}

static int msm_pmic_led_resume(struct platform_device *dev)
{
	led_classdev_resume(&msm_kp_bl_led);

	return 0;
}
#else
#define msm_pmic_led_suspend NULL
#define msm_pmic_led_resume NULL
#endif

static struct platform_driver msm_pmic_led_driver = {
	.probe		= msm_pmic_led_probe,
	.remove		= __devexit_p(msm_pmic_led_remove),
	.suspend	= msm_pmic_led_suspend,
	.resume		= msm_pmic_led_resume,
	.driver		= {
		.name	= "pmic-leds",
		.owner	= THIS_MODULE,
	},
};

static int __init msm_pmic_led_init(void)
{
	return platform_driver_register(&msm_pmic_led_driver);
}
module_init(msm_pmic_led_init);

static void __exit msm_pmic_led_exit(void)
{
	platform_driver_unregister(&msm_pmic_led_driver);
}
module_exit(msm_pmic_led_exit);

MODULE_DESCRIPTION("MSM PMIC LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:pmic-leds");
