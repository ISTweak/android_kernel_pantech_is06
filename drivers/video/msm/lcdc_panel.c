/* Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
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

#include "msm_fb.h"
#include <linux/clk.h>
//#define SKY_LCDC_IC_CONTROL

#ifdef CONFIG_PANTECH_ERROR_LOG
#include <linux/delay.h>
#include <linux/gpio.h>
#include "mdp.h"
#include "mdp4.h"

#ifdef CONFIG_FB_MSM_MDP40
#define LCDC_BASE	0xC0000
#else
#define LCDC_BASE	0xE0000
#endif

#endif/*CONFIG_PANTECH_ERROR_LOG*/

#define LCD_MSG

#define FEATURE_PANTECH_PMIC_CHG

//20100717 jjangu_device
#if (BOARD_VER < JMASAI_PT10)
#define LCD_RST           16
#else
#define LCD_RST           28
#endif
#define SPI_SCLK          17
#define SPI_SDO		  18
#define SPI_CS		  20
//20100809 jjangu_device
#if (BOARD_VER >= JMASAI_PT20)
#define LCD_DIM_CON   147
#define MAX8848Y_BLU_MAX  10
#define MAX8848Y_BLU_OFF  MAX8848Y_BLU_MAX
#define T_SHDN  15     //typ 8ms
#define T_INT   300   //min 120us
#define T_HI    3    //min 1us
#define T_LO    2    //min 1us max 500us

#define MAX_CMD  127
u8 parameter_list[MAX_CMD];
static u8 set_value = 0; 
static boolean first_set_start = FALSE;
static u8 circle_count = 0;
static u8 max_current_level = MAX8848Y_BLU_MAX;
static boolean initialized = FALSE;

struct panel_state_type{
	boolean disp_initialized;
	boolean display_on;
	boolean disp_powered_up;
};

static struct panel_state_type panel_state = { 0 };

#endif

//static struct clk *mdp_lcdc_pclk_clk;  //for test.
//static struct clk *mdp_lcdc_pad_pclk_clk; //for test.


#ifdef CONFIG_EF12_BOARD
#include <linux/spi/spi.h>
#include <linux/leds.h>

#include <mach/sky_smem.h>

//extern void sky_led_set(enum led_brightness value);

#ifdef LCD_MSG
#define ENTER_FUNC() printk(KERN_ERR "[IRMAVEP] %s start\n", __FUNCTION__);
#define EXIT_FUNC() printk(KERN_ERR "[IRMAVEP] %s exit \n", __FUNCTION__);
#else
#define ENTER_FUNC()
#define EXIT_FUNC()
#endif

#define RGB_SPI_REGISTER_TAG	0x70
#define RGB_SPI_DATA_TAG		0x72
#define SPECIAL_HANDLE_REG		0xEF

#define GPIO_HIGH_VALUE	1
#define GPIO_LOW_VALUE	0

struct spi_device *pspi;

extern struct fb_info *registered_fb[FB_MAX]; 

#define NOP()	do {asm volatile ("NOP");} while(0);

static unsigned char bit_shift[8] = { (1 << 7), /* MSB */ 
  (1 << 6),
  (1 << 5),
  (1 << 4),
  (1 << 3),
  (1 << 2),
  (1 << 1),
  (1 << 0)                   /* LSB */
};

/* define spi function */
//20100809 jjangu_device
#if (BOARD_VER < JMASAI_PT20)
static int send_spi_command (u8 reg, u16 data);
static void change_brightness (u8 *brightness, int size);
#else
static void send_spi_command(u8 reg, u8 count, u8 *param);
static void change_brightness (u8 brightness, int size);
static void lcd_setup(void);
#endif

#if defined(FEATURE_PANTECH_PMIC_CHG)
extern char *saved_command_line;
static boolean isOfflineChargingMode( void );
#endif

static void lcdc_set_backlight (struct msm_fb_data_type *mfd);
void spi_init(void);

/* variable to handle suspend and resume */
static u8 is_from_wakeup = 0;

#endif /* CONFIG_EF12_BOARD */

#ifdef CONFIG_PANTECH_ERROR_LOG
static void send_spi_command_gpio(unsigned char reg, unsigned short data);
static atomic_t is_gpio_spi = ATOMIC_INIT(0);
#endif

#if defined(FEATURE_PANTECH_PMIC_CHG)
static boolean isOfflineChargingMode( void )
{
  char* command_line = saved_command_line;
	char* ptr = NULL;

	if( (ptr = strstr(command_line, "androidboot.battchg_pause")) == NULL )
	{
	  return FALSE;
	}

	if( !strcmp(ptr, "androidboot.battchg_pause=true") )
	{
	  return TRUE;
	}

	return FALSE;
}
#endif

static int send_spi_command_test (u8 reg, u16 data)
{
   u8 cmd[8];
   struct spi_message msg;
   struct spi_transfer index_xfer = {
         .len		= 2,
         .cs_change	= 1,
         .bits_per_word = 16,
   };
   struct spi_transfer value_xfer = {
         .len		= 2,
         .bits_per_word = 16,      
   };

   spi_message_init(&msg);

   /* register index */
   cmd[0] = RGB_SPI_REGISTER_TAG;
   cmd[1] = reg & 0xff;
   index_xfer.tx_buf = cmd;
   spi_message_add_tail(&index_xfer, &msg);

   /* register value */
   cmd[3] = RGB_SPI_DATA_TAG;
   cmd[4] = data & 0xff;
   value_xfer.tx_buf = cmd + 3;
   spi_message_add_tail(&value_xfer, &msg);

   return spi_sync(pspi, &msg);
}

static int lcdc_panel_on(struct platform_device *pdev)
{
#ifdef CONFIG_EF12_BOARD
    ENTER_FUNC();
//20100809 jjangu_device
#if (BOARD_VER < JMASAI_PT20)

#ifdef CONFIG_PANTECH_ERROR_LOG
	if(atomic_read(&is_gpio_spi) == 1) {
		send_spi_command_gpio(0x1D, 0xA0);
		mdelay(150);
		send_spi_command_gpio(0x14, 0x03);
	}
	else {
#endif
		send_spi_command (0x1D, 0xA0);
		//mdelay(250);
		//msleep(250);

		msleep(150);

		send_spi_command(0x14, 0x03);

#ifdef CONFIG_PANTECH_ERROR_LOG
	}
#endif

#else
        if(panel_state.disp_initialized == FALSE)
        {
           ENTER_FUNC();
        
           spi_init();
#if 1
           // Reset LCD
           gpio_set_value(LCD_RST, GPIO_LOW_VALUE);
           mdelay(20);
           gpio_set_value(LCD_RST, GPIO_HIGH_VALUE);
           mdelay(20);
      
           lcd_setup();
#endif
           panel_state.disp_initialized = TRUE;

           EXIT_FUNC();
        }
        else
        {
          send_spi_command(0x11, 0, parameter_list);
        }
        //mdelay(120);
#endif
	//sky_led_set(LED_R_FULL_B_FULL);	// lived 2010.05.17 Keypad LED on
//20101002 jjangu_device
#ifdef SKY_STATE_LCD
	panel_state.display_on = TRUE;
       sky_smem_set_lcd_state(panel_state.display_on);
#endif	   
    EXIT_FUNC();
#endif /* CONFIG_EF10_BOARD */
	return 0;
}

static int lcdc_panel_off(struct platform_device *pdev)
{
#ifdef CONFIG_EF12_BOARD
    ENTER_FUNC();
//20100809 jjangu_device
#if (BOARD_VER < JMASAI_PT20)

    send_spi_command(0x14, 0x00);
    mdelay(50); //wait for several frame :: guide
	//msleep(50); //wait for several frame :: guide    
    send_spi_command (0x1D, 0xA1);
    mdelay(200);
	//msleep(200);

    is_from_wakeup = 1;

	//sky_led_set(LED_OFF);	// lived 2010.05.17 Keypad LED off

#else
    send_spi_command(0x10, 0, parameter_list);
    mdelay(120);
#endif
//20101002 jjangu_device
#ifdef SKY_STATE_LCD
    panel_state.display_on = FALSE;
    sky_smem_set_lcd_state(panel_state.display_on);
#endif
    EXIT_FUNC();

#endif /* CONFIG_EF10_BOARD */
	return 0;
}

static int __init lcdc_panel_probe(struct platform_device *pdev)
{
#ifdef LCD_MSG
    printk("%s start.  pdev->name %s\n", __FUNCTION__, pdev->name);//func debug....
#endif

	msm_fb_add_device(pdev);

#ifdef LCD_MSG
    printk("%s end\n", __FUNCTION__);//func debug....
#endif
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = lcdc_panel_probe,
	.driver = {
		.name   = "lcdc_panel",
	},
};

static struct msm_fb_panel_data lcdc_panel_data = {
	.on = lcdc_panel_on,
	.off = lcdc_panel_off,
#ifdef CONFIG_EF12_BOARD
       .set_backlight = lcdc_set_backlight,
#endif /* CONFIG_EF12_BOARD */
};

static int lcdc_dev_id;

int lcdc_device_register(struct msm_panel_info *pinfo)
{
	struct platform_device *pdev = NULL;
	int ret;

	pdev = platform_device_alloc("lcdc_panel", ++lcdc_dev_id);
	if (!pdev)
		return -ENOMEM;

	lcdc_panel_data.panel_info = *pinfo;
	ret = platform_device_add_data(pdev, &lcdc_panel_data,
		sizeof(lcdc_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

//20100809 jjangu_device
#if (BOARD_VER >= JMASAI_PT20)
     ENTER_FUNC();
     if(panel_state.disp_initialized == FALSE)
     {
       spi_init();
#if 1
       // Reset LCD
       gpio_set_value(LCD_RST, GPIO_LOW_VALUE);
       mdelay(20);
       gpio_set_value(LCD_RST, GPIO_HIGH_VALUE);
       mdelay(20);
  
       lcd_setup();
#endif
       panel_state.disp_initialized = TRUE;
     }
     EXIT_FUNC();
#endif

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init lcdc_panel_init(void)
{
	return platform_driver_register(&this_driver);
}

#ifdef CONFIG_EF12_BOARD
//20100809 jjangu_device
#if (BOARD_VER < JMASAI_PT20)
static int send_spi_command (u8 reg, u16 data)
{
   u8 cmd[8];
   struct spi_message msg;
   struct spi_transfer index_xfer = {
         .len		= 2,
         .cs_change	= 1,
         .bits_per_word = 16,
   };
   struct spi_transfer value_xfer = {
         .len		= 2,
         .bits_per_word = 16,      
   };

   spi_message_init(&msg);

   /* register index */
   cmd[0] = RGB_SPI_REGISTER_TAG;
   cmd[1] = reg & 0xff;
   index_xfer.tx_buf = cmd;
   spi_message_add_tail(&index_xfer, &msg);

   /* register value */
   cmd[3] = RGB_SPI_DATA_TAG;
   cmd[4] = data & 0xff;
   value_xfer.tx_buf = cmd + 3;
   spi_message_add_tail(&value_xfer, &msg);

   return spi_sync(pspi, &msg);
}
#else
static void send_spi_command(u8 command, u8 count, u8 *parameter_list)
{
   u8 i,j = 0;

   /* Enable the Chip Select */
   gpio_set_value(SPI_CS, GPIO_LOW_VALUE);
   NOP();
   NOP();

   gpio_set_value(SPI_SCLK, GPIO_LOW_VALUE);
   NOP();
//CMD
  /* DNC : COMMAND = 0, PARAMETER = 1  */
   gpio_set_value(SPI_SDO, GPIO_LOW_VALUE);

   /* #2: Drive the Clk High*/
   NOP(); 
   gpio_set_value(SPI_SCLK, GPIO_HIGH_VALUE);   
   NOP();
   NOP();
   
   /* Before we enter here the Clock should be Low ! */
   for(i=0;i<8;i++)
   {
       gpio_set_value(SPI_SCLK, GPIO_LOW_VALUE);
       NOP();
      /* #1: Drive the Data (High or Low) */
      if(command & bit_shift[i])
        gpio_set_value(SPI_SDO, GPIO_HIGH_VALUE);        
      else
        gpio_set_value(SPI_SDO, GPIO_LOW_VALUE);        
      /* #2: Drive the Clk High*/
      NOP(); 
      gpio_set_value(SPI_SCLK, GPIO_HIGH_VALUE); 
      NOP();
      NOP();
   }

//PARAMETER
   for(j=0; j<count; j++)
   {
      gpio_set_value(SPI_SCLK, GPIO_LOW_VALUE);
      NOP();
     /* DNC : COMMAND = 0, PARAMETER = 1  */
      gpio_set_value(SPI_SDO, GPIO_HIGH_VALUE); 
      /* #2: Drive the Clk High*/
      NOP(); 
      gpio_set_value(SPI_SCLK, GPIO_HIGH_VALUE);    
      NOP();
      NOP();
      
      for(i=0;i<8;i++)
      {
         gpio_set_value(SPI_SCLK, GPIO_LOW_VALUE);
         NOP();
         /* #1: Drive the Data (High or Low) */
         if(parameter_list[j] & bit_shift[i])
            gpio_set_value(SPI_SDO, GPIO_HIGH_VALUE); 
         else
            gpio_set_value(SPI_SDO, GPIO_LOW_VALUE); 
   
         /* #2: Drive the Clk High*/
         NOP(); 
         gpio_set_value(SPI_SCLK, GPIO_HIGH_VALUE);    
         NOP(); 
         NOP();     
      }  
   }
      /* Idle state of SDO (MOSI) is Low */
      gpio_set_value(SPI_SDO, GPIO_LOW_VALUE); 
      gpio_set_value(SPI_SCLK, GPIO_LOW_VALUE);

   /* Now Disable the Chip Select */
   NOP();
   NOP();
   gpio_set_value(SPI_CS, GPIO_HIGH_VALUE);

   NOP();
}
#endif

#if 1
static int send_spi_special_command(u8 reg, u16 data)
{
   u8 cmd[8];
   struct spi_message msg;
   struct spi_transfer index_xfer = {
         .len		= 2,
         .cs_change	= 1,
         .bits_per_word = 16,		
   };
   struct spi_transfer value_xfer = {
         .len		= 2,
         .cs_change	= 1,      
         .bits_per_word = 16,      
   };

   struct spi_transfer value_xfer2 = {
         .len		= 2,
         .bits_per_word = 16,      
   };

   spi_message_init(&msg);

   /* register index */
   cmd[0] = RGB_SPI_REGISTER_TAG;
   cmd[1] = reg & 0xff;
   index_xfer.tx_buf = cmd;
   spi_message_add_tail(&index_xfer, &msg);

   /* register value */
   cmd[3] = RGB_SPI_DATA_TAG;
   cmd[4] = (data >> 8) & 0xff;
   value_xfer.tx_buf = cmd + 3;
   spi_message_add_tail(&value_xfer, &msg);

   cmd[6] = RGB_SPI_DATA_TAG;
   cmd[7] = data & 0xff;
   value_xfer2.tx_buf = cmd + 6;
   spi_message_add_tail(&value_xfer2, &msg);

   return spi_sync(pspi, &msg);
}
#endif

static u8 flag = 0;
static u8 gamma_flag[2] = {0x43, 0x34};

//lsb : panel gamma control register number
static u8 gamma_register[21] = 
{
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66
};

//lsb : gamma value according to brightness level
//      this panel support 9 level from 0 to 8
static u8 brightness[11][21] = 
{
#if 0
	/* brightness 50 */
	{
         0x00, 0x3F, 0x3C, 0x2C, 0x2D, 0x27, 0x24,
         0x00, 0x00, 0x00, 0x22, 0x2A, 0x27, 0x23,
         0x00, 0x3F, 0x3B, 0x2C, 0x2B, 0x24, 0x31
	},
#endif	 
	/* brightness 70 */	
	{
         0x00, 0x3F, 0x35, 0x2C, 0x2B, 0x26, 0x29,
         0x00, 0x00, 0x00, 0x25, 0x29, 0x26, 0x28,
         0x00, 0x3F, 0x34, 0x2B, 0x2A, 0x23, 0x37
	},
	
	/* brightness 90 */
	{
         0x00, 0x3F, 0x31, 0x2B, 0x2B, 0x25, 0x2D,
         0x00, 0x00, 0x00, 0x25, 0x29, 0x25, 0x2C,
         0x00, 0x3F, 0x30, 0x2A, 0x29, 0x22, 0x3D
	},

	/* brightness 110 */
	{
         0x00, 0x3F, 0x2E, 0x2B, 0x2A, 0x24, 0x31,
         0x00, 0x00, 0x04, 0x25, 0x28, 0x24, 0x30,
         0x00, 0x3F, 0x2E, 0x29, 0x28, 0x21, 0x41
	},

	/* brightness 130 */
	{
         0x00, 0x3F, 0x2E, 0x29, 0x2A, 0x23, 0x34,
         0x00, 0x00, 0x0A, 0x25, 0x28, 0x23, 0x33,
         0x00, 0x3F, 0x2D, 0x28, 0x27, 0x20, 0x46
	},

	/* brightness 150 */
	{
         0x00, 0x3F, 0x2D, 0x29, 0x28, 0x23, 0x37,
         0x00, 0x00, 0x0B, 0x25, 0x28, 0x22, 0x36,
         0x00, 0x3F, 0x2B, 0x28, 0x26, 0x1F, 0x4A
	},

      /* brightness 170 */
	{
         0x00, 0x3F, 0x29, 0x2A, 0x27, 0x22, 0x3A,
         0x00, 0x00, 0x0C, 0x26, 0x26, 0x22, 0x39,
         0x00, 0x3F, 0x28, 0x28, 0x24, 0x1F, 0x4D
	},

	/* brightness 190 */
	{
         0x00, 0x3F, 0x29, 0x29, 0x27, 0x22, 0x3C,
         0x00, 0x00, 0x10, 0x26, 0x26, 0x22, 0x3B,
         0x00, 0x3F, 0x28, 0x28, 0x24, 0x1F, 0x50
	},
#if 1
	/* brightness 210 */
	{
         0x00, 0x3F, 0x29, 0x28, 0x28, 0x20, 0x3F,
         0x00, 0x00, 0x10, 0x25, 0x27, 0x20, 0x3E,
         0x00, 0x3F, 0x27, 0x27, 0x25, 0x1C, 0x55
	},
	
      /* brightness 230 */
      {
         0x00, 0x3F, 0x2A, 0x28, 0x27, 0x20, 0x41,
         0x00, 0x00, 0x14, 0x25, 0x26, 0x20, 0x40,
         0x00, 0x3F, 0x29, 0x26, 0x24, 0x1D, 0x57
	},

	/* brightness 250 */
	{
         0x00, 0x3F, 0x2A, 0x27, 0x27, 0x1F, 0x44,
         0x00, 0x00, 0x17, 0x24, 0x26, 0x1F, 0x43,
         0x00, 0x3F, 0x2A, 0x25, 0x24, 0x1B, 0x5C
	},

      /* brightness 275 */
      {
        0x00, 0x3F, 0x27, 0x28, 0x25, 0x1F, 0x47, 
        0x00, 0x00, 0x15, 0x25, 0x25, 0x1F, 0x45, 
        0x00, 0x3F, 0x28, 0x25, 0x23, 0x1B, 0x5F
	}
#else         
	/* brightness 250 */
	{
         0x00, 0x3F, 0x2A, 0x27, 0x27, 0x1F, 0x44,
         0x00, 0x00, 0x17, 0x24, 0x26, 0x1F, 0x43,
         0x00, 0x3F, 0x2A, 0x25, 0x24, 0x1B, 0x5C
	},
	
      /* brightness 275 */
      {
        0x00, 0x3F, 0x27, 0x28, 0x25, 0x1F, 0x47, 
        0x00, 0x00, 0x15, 0x25, 0x25, 0x1F, 0x45, 
        0x00, 0x3F, 0x28, 0x25, 0x23, 0x1B, 0x5F
	},

	/* brightness 300 */
	{
        0x00, 0x3F, 0x25, 0x27, 0x26, 0x1E, 0x49, 
        0x00, 0x00, 0x16, 0x24, 0x26, 0x1D, 0x48, 
        0x00, 0x3F, 0x25, 0x25, 0x23, 0x1A, 0x62
	}
#endif

};

//20100809 jjangu_device
#if (BOARD_VER < JMASAI_PT20)
static void change_brightness (u8 *brightness, int size)
{
	int i = 0;

#ifdef CONFIG_PANTECH_ERROR_LOG
	if(atomic_read(&is_gpio_spi) == 1) {
		send_spi_command_gpio(0x39, gamma_flag[flag]);
		mdelay(20);

		for (i = 0; i < size; i++)
		{
			send_spi_command_gpio(gamma_register[i], brightness[i]);
		}

		flag = (flag + 1) % 2;
		send_spi_command_gpio(0x39, gamma_flag[flag]);
		mdelay(20);

	}
	else {
#endif
		send_spi_command (0x39, gamma_flag[flag]);
		//printk (KERN_ERR "[1]\n");
		//mdelay(30);	
		msleep(20);
		// do change gamma value
		for (i = 0; i < size; i++)
		{
			send_spi_command (gamma_register[i], brightness[i]);
			//printk (KERN_ERR "[%d]\n", i + 2);
		}

		flag = (flag + 1) % 2;
		send_spi_command (0x39, gamma_flag[flag]);
		//printk (KERN_ERR "[last]\n");
		//mdelay (30);
		msleep(20);

#ifdef CONFIG_PANTECH_ERROR_LOG
	}
#endif
}

#else
static void change_brightness (u8 brightness, int size)
{
    int idx;
    int count = 0;
    unsigned long frags;
    int rc = 0;
    
    ENTER_FUNC();

    // 6 7 8 9 10 11 13 16 17 21 (LEVEL 10)
    set_value = MAX8848Y_BLU_MAX - brightness;
    //printk(KERN_ERR "[IRMAVEP] %s : brightness : %d set_value : %d max_current_level : %d \n", __FUNCTION__, brightness,set_value,max_current_level);

    //if(set_value >=25 && set_value < 26)
    //  set_value = 25;
    
    
    if (!initialized)
    {
		
      gpio_set_value(LCD_DIM_CON,GPIO_LOW_VALUE);
      mdelay(T_SHDN);
      initialized = TRUE;
    }  
    if(set_value >= MAX8848Y_BLU_OFF) // BACKLIGHT_OFF
    {
      gpio_set_value(LCD_DIM_CON,GPIO_LOW_VALUE);
      mdelay(T_SHDN);
      first_set_start = FALSE;
      circle_count = 0;
    }    
    else if (set_value == max_current_level)
    {
      //Do Nothing    
    }
    else
    {   
      if (set_value < max_current_level)
      {
        if(first_set_start == FALSE){
          //SHUTDOWN 
          gpio_set_value(LCD_DIM_CON,GPIO_LOW_VALUE);
          mdelay(T_SHDN);
          
          //INIT
          gpio_set_value(LCD_DIM_CON,GPIO_HIGH_VALUE);
          udelay(T_INT);
		  
          if(set_value == 6)
          {
            count = 1;
          }
          else if(set_value == 7)
          {
            count = 1+1;
          }
          else if(set_value == 8)
          {
            count = 1+1+1;
          }
          else if(set_value == 9)
          {
            count = 1+1+1+3;
          }

          //RESET 에 의한 count 재조정...max 30 -> 26 -> 10 chg 
          //count = set_value;
          count += set_value+5;  //start is 6

          //count = set_value+3;
          first_set_start = TRUE;
        
        }else
        {
          //printk(KERN_ERR "[IRMAVEP] UP!!!!!!!!!!! \n", __FUNCTION__);
        
          if(set_value == 9)
          {
            count = 1+1+1+3; //21
          }
          else if(set_value == 8)
          {
            count = 1+1+1; //17 
          }
          else if(set_value == 7)
          {
            if(max_current_level == 8)
            {
              count = 3+1+1; //15
            }
            else if(max_current_level >= 9)
            {
              count = 1+1; //15
            }
          }
          else if(set_value == 6)
          {
            if(max_current_level == 7)
            {
              count = 1+3+1; //15
            }
            else if(max_current_level == 8)
            {
              count = 3+1; //15
            }
            else if(max_current_level >= 9)
            {
              count = 1; //15
            }
          }
          else if(set_value == 5)
          {
            if(max_current_level == 6)
            {
              count = 1+1+3; //15
            }
            else if(max_current_level == 7)
            {
              count = 1+3; //15
            }
            else if(max_current_level == 8)
            {
              count = 3; //15
            }
            else if(max_current_level == 9)
            {
              count = 0; //15
            }
          }
          else
          {
            if(max_current_level == 5)
            {
              count = 1+1+1+3; //15
            }
            else if(max_current_level == 6)
            {
              count = 1+1+3; //15
            }
            else if(max_current_level == 7)
            {
              count = 1+3; //15
            }
            else if(max_current_level == 8)
            {
              count = 3; //15
            }
            else if(max_current_level >= 9)
            {
              count = 0;
            } 
            else
            {
              count = 1+1+1+3; //15
            }
          }

          count +=  (MAX8848Y_BLU_MAX - max_current_level)+ set_value;
        }    
      }
      else
      {
          //printk(KERN_ERR "[IRMAVEP] DOWN!!!!!!!!!!! \n", __FUNCTION__);
		  
        if(set_value == 5)
        {
          count = 0;  //12
        }
        else if(set_value == 6)
        {
          count = 1;  //12
        }
        else if(set_value == 7)
        {
          if(max_current_level == 6)
          {
            count = 1;
          }
	   else
	   {
	     count = 1+1;
	   }
        }
        else if(set_value == 8)
        {
          if(max_current_level == 7)
          {
            count = 1;
          }
          else if(max_current_level == 6)		  
          {
            count = 1+1;
          }
	   else
	   {
	     count = 1+1+1;
	   }
        }
        else if(set_value == 9)
        {
          if(max_current_level == 8)
          {
            count = 3;
          }
          else if(max_current_level == 7)		  
          {
            count = 1+3;
          }
          else if(max_current_level == 6)		  
          {
            count = 1+1+3;
          }
	   else
	   {
	     count = 1+1+1+3;
	   }
        }		
		
        count += set_value - max_current_level;
      }
      
      
      circle_count+= count;
      if(circle_count >= 21)
      {
        count+=15;
        //circle_count-=26;
        //circle_count -= (MAX8848Y_BLU_MAX);
        switch(set_value)
        {
          case 0:
	     circle_count = 5;
            break;
          case 1:
	     circle_count = 6;
	     break;
          case 2:
	     circle_count = 7;
            break;
          case 3:
	     circle_count = 8;
            break;
          case 4:
	     circle_count = 9;
            break;
          case 5:
	     circle_count = 10;
            break;
          case 6:
	     circle_count = 12;
            break;
          case 7:
	     circle_count = 14;
            break;
          case 8:
	     circle_count = 16;
            break;
          case 9:
	     circle_count = 20;
            break;
          default:
            break;
        }
		
      }
      
      //printk(KERN_ERR "[IRMAVEP] %s : circle_count : %d count : %d \n", __FUNCTION__, circle_count,count);

      local_save_flags(frags);
      local_irq_disable();
      for(idx = 0;idx < count; idx++)
      {
        gpio_set_value(LCD_DIM_CON,GPIO_LOW_VALUE);
        udelay(T_LO);
        gpio_set_value(LCD_DIM_CON,GPIO_HIGH_VALUE);
        udelay(T_HI);
      }
      local_irq_restore(frags);
    
    }  
    max_current_level = set_value;   
    
    EXIT_FUNC();
	
}
#endif

int old_brightness = 0;

#ifdef CONFIG_SW_RESET
extern int sky_sys_rst_SetLcdBLStatus(uint32_t eBrightness);
#endif

static void lcdc_set_backlight (struct msm_fb_data_type *mfd)
{
	int blevel = mfd->bl_level;

#ifdef LCD_MSG
	//printk (KERN_ERR "[IRMAVEP] %s : brightness : %d\n", __FUNCTION__, blevel);
#endif

//20100809 jjangu_device
#if (BOARD_VER < JMASAI_PT20)
	change_brightness(brightness[blevel], 21);
#else
	if (panel_state.disp_initialized == FALSE)
		return;

#if defined(FEATURE_PANTECH_PMIC_CHG)
  if( !isOfflineChargingMode() )
  {
    change_brightness(blevel, 10);
  }
#else
       change_brightness(blevel, 10);
#endif
#endif
#if 0 //2009.12.09 delete
	if (blevel==0)
	{
		send_spi_command(0x14, 0x00);
		mdelay(200); //wait for several frame :: guide
		send_spi_command (0x1D, 0xA1);
		mdelay (200);
	}
#endif

#ifdef CONFIG_SW_RESET
	sky_sys_rst_SetLcdBLStatus((uint32_t)blevel);
#endif
}

int tl2796_screen_on(void)
{
    ENTER_FUNC();
//20100809 jjangu_device
#if (BOARD_VER < JMASAI_PT20)

    send_spi_command (0x1D, 0xA0);
    mdelay(200);
    //internal power on
    mdelay(250);
    send_spi_command(0x14, 0x03);

#else
    send_spi_command(0x11, 0, parameter_list);
    mdelay(120);

#endif

    return 0;
}

int tl2796_screen_off(void)
{
//20100809 jjangu_device
#if (BOARD_VER < JMASAI_PT20)

    send_spi_command(0x14, 0x00);
    mdelay(200); //wait for several frame :: guide

    send_spi_command (0x1D, 0xA1);
    mdelay (200);
	
#else
    send_spi_command(0x10, 0, parameter_list);
    mdelay(120);
#endif

    EXIT_FUNC();
    return 0;
}

#if defined(CONFIG_EF12_BOARD) && defined(CONFIG_PANTECH_ERROR_LOG)
static inline void spi_gpio_one_byte(unsigned char val)
{
    unsigned char i;
    for (i = 0; i < 8; i++) {
        gpio_set_value(SPI_SCLK, GPIO_LOW_VALUE);
        NOP();

        if (val & bit_shift[i])
            gpio_set_value(SPI_SDO, GPIO_HIGH_VALUE);
        else
            gpio_set_value(SPI_SDO, GPIO_LOW_VALUE);

        NOP();
        gpio_set_value(SPI_SCLK, GPIO_HIGH_VALUE);
        NOP();NOP();
    }
    /* Idle state of SDO (MOSI) is Low */
    gpio_set_value(SPI_SDO, GPIO_LOW_VALUE);
}


static void spi_gpio_sync(unsigned char start, unsigned char param)
{
    /* Enable the Chip Select */
    gpio_set_value(SPI_CS, GPIO_LOW_VALUE);
    NOP(); NOP();
    /* 1-Byte send */
    spi_gpio_one_byte(start);
    /* 1-Byte send */
    spi_gpio_one_byte(param);
    /* Now Disable the Chip Select */
    NOP(); NOP();
    gpio_set_value(SPI_CS, GPIO_HIGH_VALUE);
}

#if 0
static void spi_gpio_sync_read(u8 start)
{
	unsigned char i;
	int read = 0, tmp;
	/* Enable the Chip Select */
	gpio_set_value(SPI_CS, GPIO_LOW_VALUE);
	//udelay(100);
	udelay(33);

	/* 1-Byte send */
	spi_gpio_one_byte(start);

	for (i = 0; i < 8; i++) {
		udelay(1);
		gpio_set_value(SPI_SCLK, GPIO_LOW_VALUE);
		udelay(1);
		tmp = gpio_get_value(SPI_SDO);
		read |= tmp << (7-i);
		//udelay(1);
		gpio_set_value(SPI_SCLK, GPIO_HIGH_VALUE);
		udelay(2);
	}
	printk(KERN_ERR"[LIVED_READ] read=0x%x\n", read);

	/* Now Disable the Chip Select */
	//udelay(100);
	udelay(33);
	gpio_set_value(SPI_CS, GPIO_HIGH_VALUE);
}
#endif

static void send_spi_command_gpio(unsigned char reg, unsigned short data)
{
    spi_gpio_sync(RGB_SPI_REGISTER_TAG, reg);
    NOP();
		
		if((data >> 8) != 0) // 3 SPI comm.
    { 
        spi_gpio_sync(RGB_SPI_DATA_TAG, (data >> 8));
        NOP();
    }
    spi_gpio_sync(RGB_SPI_DATA_TAG, (data & 0x00FF));
    NOP();
}

static uint32_t tl2796_gpio_table[] = {
	GPIO_CFG(17, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(18, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(20, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
};

static void config_tl2796_gpio_table(uint32_t *table, int len, unsigned enable)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n],
			enable ? GPIO_ENABLE : GPIO_DISABLE);
		if (rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

static void gpio_spi_start(bool clock_on)
{
  atomic_set(&is_gpio_spi, 1);

	if(clock_on) {
         config_tl2796_gpio_table(tl2796_gpio_table, ARRAY_SIZE(tl2796_gpio_table), 1);
	}
	
	gpio_set_value(17, GPIO_HIGH_VALUE);
	gpio_set_value(18, GPIO_LOW_VALUE);
	gpio_set_value(20, GPIO_HIGH_VALUE);
    
}

static void gpio_spi_end(void)
{
  atomic_set(&is_gpio_spi, 0);
}

#ifdef kernel_warn
static void gpio_change_brightness (char *brightness, int size)
{
  int i = 0;

  send_spi_command_gpio(0x39, 43);
  mdelay(20);


// do change gamma value 
  for (i = 0; i < size; i++)
  {
    send_spi_command_gpio(gamma_register[i], brightness[i]);
  }
  send_spi_command_gpio(0x39, 34);
  mdelay(20);
  
}
#endif

///////// LCD ON ///////////////////////////
#ifdef kernel_warn
static void gpio_lcd_on(struct msm_fb_data_type *mfd)
{

	if(is_from_wakeup) {
		send_spi_command_gpio(0x1d, 0xa0);

		mdelay(250);

		send_spi_command_gpio(0x14, 0x03);
	}

}
#endif

extern int first_pixel_start_x;
extern int first_pixel_start_y;

static void err_mdp_lcdc_on(struct platform_device *pdev, bool clock_on)
{
	int lcdc_width;
	int lcdc_height;
	int lcdc_bpp;
	int lcdc_border_clr;
	int lcdc_underflow_clr;
	int lcdc_hsync_skew;

	int hsync_period;
	int hsync_ctrl;
	int vsync_period;
	int display_hctl;
	int display_v_start;
	int display_v_end;
	int active_hctl;
	int active_h_start;
	int active_h_end;
	int active_v_start;
	int active_v_end;
	int ctrl_polarity;
	int h_back_porch;
	int h_front_porch;
	int v_back_porch;
	int v_front_porch;
	int hsync_pulse_width;
	int vsync_pulse_width;
	int hsync_polarity;
	int vsync_polarity;
	int data_en_polarity;
	int hsync_start_x;
	int hsync_end_x;
	uint8 *buf;
	int bpp;
	uint32 dma2_cfg_reg;
	struct fb_info *fbi;
	struct fb_var_screeninfo *var;
	struct msm_fb_data_type *mfd;

	mfd = (struct msm_fb_data_type *)platform_get_drvdata(pdev);

	if (!mfd)
		return ;

	if (mfd->key != MFD_KEY)
		return ;

	fbi = mfd->fbi;
	var = &fbi->var;

	/* MDP cmd block enable */
	if(clock_on) {
		mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_ON, FALSE);
	}

	bpp = fbi->var.bits_per_pixel / 8;
	buf = (uint8 *) fbi->fix.smem_start;
	buf +=
	    (fbi->var.xoffset + fbi->var.yoffset * fbi->var.xres_virtual) * bpp;

	dma2_cfg_reg = DMA_PACK_ALIGN_LSB | DMA_DITHER_EN | DMA_OUT_SEL_LCDC;

	if (mfd->fb_imgType == MDP_BGR_565)
		dma2_cfg_reg |= DMA_PACK_PATTERN_BGR;
	else
		dma2_cfg_reg |= DMA_PACK_PATTERN_RGB;

	if (bpp == 2)
		dma2_cfg_reg |= DMA_IBUF_FORMAT_RGB565;
	else if (bpp == 3)
		dma2_cfg_reg |= DMA_IBUF_FORMAT_RGB888;
	else
		dma2_cfg_reg |= DMA_IBUF_FORMAT_xRGB8888_OR_ARGB8888;

	switch (mfd->panel_info.bpp) {
	case 24:
		dma2_cfg_reg |= DMA_DSTC0G_8BITS |
		    DMA_DSTC1B_8BITS | DMA_DSTC2R_8BITS;
		break;

	case 18:
		dma2_cfg_reg |= DMA_DSTC0G_6BITS |
		    DMA_DSTC1B_6BITS | DMA_DSTC2R_6BITS;
		break;

	case 16:
		dma2_cfg_reg |= DMA_DSTC0G_6BITS |
		    DMA_DSTC1B_5BITS | DMA_DSTC2R_5BITS;
		break;

	default:
		printk(KERN_ERR "mdp lcdc can't support format %d bpp!\n",
		       mfd->panel_info.bpp);
		return ;
	}

	/* DMA register config */

	/* starting address */
	MDP_OUTP(MDP_BASE + 0x90008, (uint32) buf);
	/* active window width and height */
	MDP_OUTP(MDP_BASE + 0x90004, ((fbi->var.yres) << 16) | (fbi->var.xres));
	/* buffer ystride */
	MDP_OUTP(MDP_BASE + 0x9000c, fbi->var.xres_virtual * bpp);
	/* x/y coordinate = always 0 for lcdc */
	MDP_OUTP(MDP_BASE + 0x90010, 0);
	/* dma config */
	MDP_OUTP(MDP_BASE + 0x90000, dma2_cfg_reg);

	/*
	 * LCDC timing setting
	 */
	h_back_porch = var->left_margin;
	h_front_porch = var->right_margin;
	v_back_porch = var->upper_margin;
	v_front_porch = var->lower_margin;
	hsync_pulse_width = var->hsync_len;
	vsync_pulse_width = var->vsync_len;
	lcdc_border_clr = mfd->panel_info.lcdc.border_clr;
	lcdc_underflow_clr = mfd->panel_info.lcdc.underflow_clr;
	lcdc_hsync_skew = mfd->panel_info.lcdc.hsync_skew;

	lcdc_width = mfd->panel_info.xres;
	lcdc_height = mfd->panel_info.yres;
	lcdc_bpp = mfd->panel_info.bpp;

	hsync_period =
	    hsync_pulse_width + h_back_porch + lcdc_width + h_front_porch;
	hsync_ctrl = (hsync_period << 16) | hsync_pulse_width;
	hsync_start_x = hsync_pulse_width + h_back_porch;
	hsync_end_x = hsync_period - h_front_porch - 1;
	display_hctl = (hsync_end_x << 16) | hsync_start_x;

	vsync_period =
	    (vsync_pulse_width + v_back_porch + lcdc_height +
	     v_front_porch) * hsync_period;
	display_v_start =
	    (vsync_pulse_width + v_back_porch) * hsync_period + lcdc_hsync_skew;
	display_v_end =
	    vsync_period - (v_front_porch * hsync_period) + lcdc_hsync_skew - 1;

	if (lcdc_width != var->xres) {
		active_h_start = hsync_start_x + first_pixel_start_x;
		active_h_end = active_h_start + var->xres - 1;
		active_hctl =
		    ACTIVE_START_X_EN | (active_h_end << 16) | active_h_start;
	} else {
		active_hctl = 0;
	}

	if (lcdc_height != var->yres) {
		active_v_start =
		    display_v_start + first_pixel_start_y * hsync_period;
		active_v_end = active_v_start + (var->yres) * hsync_period - 1;
		active_v_start |= ACTIVE_START_Y_EN;
	} else {
		active_v_start = 0;
		active_v_end = 0;
	}


#ifdef CONFIG_FB_MSM_MDP40
	hsync_polarity = 1;
	vsync_polarity = 1;
	lcdc_underflow_clr |= 0x80000000;	/* enable recovery */
#else
	hsync_polarity = 0;
	vsync_polarity = 0;
#endif
#ifdef CONFIG_EF12_BOARD //20090518
	data_en_polarity = 0;
#else //CONFIG_EF12_BOARD //20090516
	data_en_polarity = 0;
#endif //CONFIG_EF12_BOARD //20090516	

	ctrl_polarity =
	    (data_en_polarity << 2) | (vsync_polarity << 1) | (hsync_polarity);

	MDP_OUTP(MDP_BASE + LCDC_BASE + 0x4, hsync_ctrl);
	MDP_OUTP(MDP_BASE + LCDC_BASE + 0x8, vsync_period);
	MDP_OUTP(MDP_BASE + LCDC_BASE + 0xc, vsync_pulse_width * hsync_period);
	MDP_OUTP(MDP_BASE + LCDC_BASE + 0x10, display_hctl);
	MDP_OUTP(MDP_BASE + LCDC_BASE + 0x14, display_v_start);
	MDP_OUTP(MDP_BASE + LCDC_BASE + 0x18, display_v_end);
	MDP_OUTP(MDP_BASE + LCDC_BASE + 0x28, lcdc_border_clr);
	MDP_OUTP(MDP_BASE + LCDC_BASE + 0x2c, lcdc_underflow_clr);
	MDP_OUTP(MDP_BASE + LCDC_BASE + 0x30, lcdc_hsync_skew);
	MDP_OUTP(MDP_BASE + LCDC_BASE + 0x38, ctrl_polarity);
	MDP_OUTP(MDP_BASE + LCDC_BASE + 0x1c, active_hctl);
	MDP_OUTP(MDP_BASE + LCDC_BASE + 0x20, active_v_start);
	MDP_OUTP(MDP_BASE + LCDC_BASE + 0x24, active_v_end);


	if(clock_on){
		panel_next_on(pdev);
	} else {
		lcdc_panel_on(pdev);
	}

	/* enable LCDC block */
	MDP_OUTP(MDP_BASE + LCDC_BASE, 1);
		
	if(clock_on) {
		mdp_pipe_ctrl(MDP_DMA2_BLOCK, MDP_BLOCK_POWER_ON, FALSE);
		mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_OFF, FALSE);
	}

}

void force_lcd_screen_on(bool clock_on)
{
	struct fb_info *info; 
  struct msm_fb_data_type *mfd; 

  info = registered_fb[0];
  mfd = (struct msm_fb_data_type *)info->par;

	if(is_from_wakeup) {
         gpio_spi_start(clock_on);
         err_mdp_lcdc_on(mfd->pdev, clock_on);	
         //gpio_change_brightness(brightness[4], 21); 
//20100809 jjangu_device
#if (BOARD_VER < JMASAI_PT20)
         change_brightness (brightness[4], 21);
#else
         change_brightness (5, 10);
#endif
         gpio_spi_end();
	} else {
         gpio_spi_start(clock_on);
         //gpio_change_brightness(brightness[4], 21); 
//20100809 jjangu_device
#if (BOARD_VER < JMASAI_PT20)
         change_brightness(brightness[4], 21); 
#else
         change_brightness(5, 10);
#endif
         gpio_spi_end();
	}

}
EXPORT_SYMBOL(force_lcd_screen_on);
#endif

//20100809 jjangu_device
#if (BOARD_VER >= JMASAI_PT20)
static void lcd_setup(void)
{

    ENTER_FUNC();
	
    parameter_list[0] = 0xFF;
    parameter_list[1] = 0x83;
    parameter_list[2] = 0x63;
    send_spi_command(0xB9, 3, parameter_list);

    parameter_list[0] = 0x81;
    parameter_list[1] = 0x24;
    parameter_list[2] = 0x04;
    parameter_list[3] = 0x02;
    parameter_list[4] = 0x02;
    parameter_list[5] = 0x03;
    parameter_list[6] = 0x10;
    parameter_list[7] = 0x10;
    parameter_list[8] = 0x34;
    parameter_list[9] = 0x3C;
    parameter_list[10] = 0x3F;
    parameter_list[11] = 0x3F;
    send_spi_command(0xB1, 12, parameter_list);

    send_spi_command(0x11, 0, parameter_list);
    mdelay(10);

#if 1 //don't touch
    send_spi_command(0x20, 0, parameter_list);
    //send_spi_command(0x21, 0, parameter_list);

    //parameter_list[0] = 0x00;
    parameter_list[0] = 0x03;
    send_spi_command(0x36, 1, parameter_list);
#endif

    parameter_list[0] = 0x70;
    send_spi_command(0x3A, 1, parameter_list);
    mdelay(150);

    parameter_list[0] = 0x78;
    parameter_list[1] = 0x24;
    parameter_list[2] = 0x04;
    parameter_list[3] = 0x02;
    parameter_list[4] = 0x02;
    parameter_list[5] = 0x03;
    parameter_list[6] = 0x10;
    parameter_list[7] = 0x10;
    parameter_list[8] = 0x34;
    parameter_list[9] = 0x3C;
    parameter_list[10] = 0x3F;
    parameter_list[11] = 0x3F;
    send_spi_command(0xB1, 12, parameter_list);

    parameter_list[0] = 0x01;
    send_spi_command(0xB3, 1, parameter_list);

    parameter_list[0] = 0x00;
    parameter_list[1] = 0x08;
    parameter_list[2] = 0x6E;
    parameter_list[3] = 0x07;
    parameter_list[4] = 0x01;
    parameter_list[5] = 0x01;
    parameter_list[6] = 0x62;
    parameter_list[7] = 0x01;
    parameter_list[8] = 0x57;
    send_spi_command(0xB4, 9, parameter_list);

#if 0
    parameter_list[0] = 0x01;
    parameter_list[1] = 0x00;
    parameter_list[2] = 0x02;
    parameter_list[3] = 0x0A;
    parameter_list[4] = 0x0F;
    parameter_list[5] = 0x14;
    parameter_list[6] = 0x19;
    parameter_list[7] = 0x21;
    parameter_list[8] = 0x29;
    parameter_list[9] = 0x2F;
    parameter_list[10] = 0x36;
    parameter_list[11] = 0x41;
    parameter_list[12] = 0x4B;
    parameter_list[13] = 0x53;
    parameter_list[14] = 0x5F;
    parameter_list[15] = 0x67;
    parameter_list[16] = 0x6F;
    parameter_list[17] = 0x78;
    parameter_list[18] = 0x82;
    parameter_list[19] = 0x89;
    parameter_list[20] = 0x92;
    parameter_list[21] = 0x9C;
    parameter_list[22] = 0xA5;
    parameter_list[23] = 0xAE;
    parameter_list[24] = 0xB7;
    parameter_list[25] = 0xBE;
    parameter_list[26] = 0xC6;
    parameter_list[27] = 0xCD;
    parameter_list[28] = 0xD6;
    parameter_list[29] = 0xDF;
    parameter_list[30] = 0xE9;
    parameter_list[31] = 0xF2;
    parameter_list[32] = 0xF7;
    parameter_list[33] = 0xFF;
    parameter_list[34] = 0x20;
    parameter_list[35] = 0x0F;
    parameter_list[36] = 0x74;
    parameter_list[37] = 0xA9;
    parameter_list[38] = 0xBD;
    parameter_list[39] = 0x41;
    parameter_list[40] = 0x70;
    parameter_list[41] = 0x85;
    parameter_list[42] = 0xC0;
    parameter_list[43] = 0x00;
    parameter_list[44] = 0x00;
    parameter_list[45] = 0x10;
    parameter_list[46] = 0x15;
    parameter_list[47] = 0x1A;
    parameter_list[48] = 0x21;
    parameter_list[49] = 0x26;
    parameter_list[50] = 0x2C;
    parameter_list[51] = 0x32;
    parameter_list[52] = 0x39;
    parameter_list[53] = 0x41;
    parameter_list[54] = 0x48;
    parameter_list[55] = 0x50;
    parameter_list[56] = 0x57;
    parameter_list[57] = 0x5F;
    parameter_list[58] = 0x66;
    parameter_list[59] = 0x6E;
    parameter_list[60] = 0x75;
    parameter_list[61] = 0x7D;
    parameter_list[62] = 0x84;
    parameter_list[63] = 0x8C;
    parameter_list[64] = 0x93;
    parameter_list[65] = 0x9B;
    parameter_list[66] = 0xA5;
    parameter_list[67] = 0xAF;
    parameter_list[68] = 0xB9;
    parameter_list[69] = 0xC3;
    parameter_list[70] = 0xCD;
    parameter_list[71] = 0xD7;
    parameter_list[72] = 0xE1;
    parameter_list[73] = 0xEB;
    parameter_list[74] = 0xF5;
    parameter_list[75] = 0xFF;
    parameter_list[76] = 0x06;
    parameter_list[77] = 0xCB;
    parameter_list[78] = 0x22;
    parameter_list[79] = 0x22;
    parameter_list[80] = 0x22;
    parameter_list[81] = 0x20;
    parameter_list[82] = 0x00;
    parameter_list[83] = 0x00;
    parameter_list[84] = 0xC0;
    parameter_list[85] = 0x00;
    parameter_list[86] = 0x0A;
    parameter_list[87] = 0x14;
    parameter_list[88] = 0x1E;
    parameter_list[89] = 0x28;
    parameter_list[90] = 0x28;
    parameter_list[91] = 0x31;
    parameter_list[92] = 0x37;
    parameter_list[93] = 0x3E;
    parameter_list[94] = 0x48;
    parameter_list[95] = 0x51;
    parameter_list[96] = 0x59;
    parameter_list[97] = 0x61;
    parameter_list[98] = 0x6A;
    parameter_list[99] = 0x6F;
    parameter_list[100] = 0x78;
    parameter_list[101] = 0x81;
    parameter_list[102] = 0x89;
    parameter_list[103] = 0x90;
    parameter_list[104] = 0x98;
    parameter_list[105] = 0xA0;
    parameter_list[106] = 0xA9;
    parameter_list[107] = 0xB3;
    parameter_list[108] = 0xB9;
    parameter_list[109] = 0xC0;
    parameter_list[110] = 0xC8;
    parameter_list[111] = 0xCD;
    parameter_list[112] = 0xD6;
    parameter_list[113] = 0xDF;
    parameter_list[114] = 0xE9;
    parameter_list[115] = 0xF2;
    parameter_list[116] = 0xF7;
    parameter_list[117] = 0xFF;
    parameter_list[118] = 0x00;
    parameter_list[119] = 0x32;
    parameter_list[120] = 0x0B;
    parameter_list[121] = 0xCE;
    parameter_list[122] = 0xCB;
    parameter_list[123] = 0xF1;
    parameter_list[124] = 0x68;
    parameter_list[125] = 0x85;
    parameter_list[126] = 0xC0;
    send_spi_command(0xC1, 127, parameter_list);
#endif

    parameter_list[0] = 0x0B;
    send_spi_command(0xCC, 1, parameter_list);

    parameter_list[0] = 0x03;
    parameter_list[1] = 0x49;
    parameter_list[2] = 0x4E;
    parameter_list[3] = 0x4C;
    parameter_list[4] = 0x57;
    parameter_list[5] = 0xF4;
    parameter_list[6] = 0x0B;
    parameter_list[7] = 0x4E;
    parameter_list[8] = 0x92;
    parameter_list[9] = 0x57;
    parameter_list[10] = 0x1A;
    parameter_list[11] = 0x99;
    parameter_list[12] = 0x96;
    parameter_list[13] = 0x0C;
    parameter_list[14] = 0x10;
    parameter_list[15] = 0x01;
    parameter_list[16] = 0x47;
    parameter_list[17] = 0x4D;
    parameter_list[18] = 0x57;
    parameter_list[19] = 0x62;
    parameter_list[20] = 0xFF;
    parameter_list[21] = 0x0A;
    parameter_list[22] = 0x4E;
    parameter_list[23] = 0xD1;
    parameter_list[24] = 0x16;
    parameter_list[25] = 0x19;
    parameter_list[26] = 0x98;
    parameter_list[27] = 0xD6;
    parameter_list[28] = 0x0E;
    parameter_list[29] = 0x11;
    send_spi_command(0xE0, 30, parameter_list);
    mdelay(10);

    send_spi_command(0x29, 0, parameter_list);

    EXIT_FUNC();

}
#endif

void spi_init()
{

    ENTER_FUNC();

    config_tl2796_gpio_table(tl2796_gpio_table, ARRAY_SIZE(tl2796_gpio_table), 1);

//20100809 jjangu_device
    gpio_set_value(SPI_SCLK, GPIO_LOW_VALUE);
    gpio_set_value(SPI_SDO, GPIO_LOW_VALUE);
    gpio_set_value(SPI_CS, GPIO_HIGH_VALUE);

    EXIT_FUNC();
    
}

#if 1
void front_test_lcd_init(void)
{
#ifdef LCD_MSG
	printk(KERN_ERR"[front_test_lcd_init....]\n");
#endif
	//gpio_request(LCD_RST, "lcd_rst");
	//gpio_tlmm_config(GPIO_CFG(LCD_RST, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
	gpio_set_value(LCD_RST, GPIO_LOW_VALUE);
	udelay(20);
	gpio_set_value(LCD_RST, GPIO_HIGH_VALUE);
	udelay(20);

//20100809 jjangu_device
#if (BOARD_VER < JMASAI_PT20)

	send_spi_command(0x31, 0x08);
	send_spi_command(0x32, 0x14);
	send_spi_command(0x30, 0x02);
	send_spi_command(0x27, 0x01);

	send_spi_command(0x12, 0x08);
	send_spi_command(0x13, 0x08);
	send_spi_command(0x15, 0x03); //0x00->0x02 dot falling
	send_spi_command(0x16, 0x00);
	send_spi_special_command(0xef, 0xd0e8);
	send_spi_command(0x39, 0x44);

#if 1
	//[[gamma
	//R
	send_spi_command(0x46, 0x44);
	send_spi_command(0x45, 0x1f);
	send_spi_command(0x44, 0x27);
	send_spi_command(0x43, 0x27);
	send_spi_command(0x42, 0x2a);
	send_spi_command(0x41, 0x3f);
	send_spi_command(0x40, 0x00);

	//G
	send_spi_command(0x56, 0x43);
	send_spi_command(0x55, 0x1f);
	send_spi_command(0x54, 0x26);
	send_spi_command(0x53, 0x24);
	send_spi_command(0x52, 0x17);
	send_spi_command(0x51, 0x00);
	send_spi_command(0x50, 0x00);

	//G
	send_spi_command(0x66, 0x5c);
	send_spi_command(0x65, 0x1b);
	send_spi_command(0x64, 0x24);
	send_spi_command(0x63, 0x25);
	send_spi_command(0x62, 0x2a);
	send_spi_command(0x61, 0x3f);
	send_spi_command(0x60, 0x00);
#endif

	//]]

	send_spi_command(0x17, 0x22);
	send_spi_command(0x18, 0x33);
	send_spi_command(0x19, 0x03);
	send_spi_command(0x1a, 0x01);
	send_spi_command(0x22, 0xa5);
	send_spi_command(0x23, 0x00);
	send_spi_command(0x26, 0xa0);

	send_spi_command(0x1d, 0xa0);

	mdelay(250);

	send_spi_command(0x14, 0x03); //0x03->0x13

#else
    parameter_list[0] = 0xFF;
    parameter_list[1] = 0x83;
    parameter_list[2] = 0x63;
    send_spi_command(0xB9, 3, parameter_list);

    parameter_list[0] = 0x81;
    parameter_list[1] = 0x24;
    parameter_list[2] = 0x04;
    parameter_list[3] = 0x02;
    parameter_list[4] = 0x02;
    parameter_list[5] = 0x03;
    parameter_list[6] = 0x10;
    parameter_list[7] = 0x10;
    parameter_list[8] = 0x34;
    parameter_list[9] = 0x3C;
    parameter_list[10] = 0x3F;
    parameter_list[11] = 0x3F;
    send_spi_command(0xB1, 12, parameter_list);

    send_spi_command(0x11, 0, parameter_list);
    mdelay(10);

#if 1 //don't touch
    send_spi_command(0x20, 0, parameter_list);
    //send_spi_command(0x21, 0, parameter_list);

    //parameter_list[0] = 0x00;
    parameter_list[0] = 0x03;
    send_spi_command(0x36, 1, parameter_list);
#endif

    parameter_list[0] = 0x70;
    send_spi_command(0x3A, 1, parameter_list);
    mdelay(150);

    parameter_list[0] = 0x78;
    parameter_list[1] = 0x24;
    parameter_list[2] = 0x04;
    parameter_list[3] = 0x02;
    parameter_list[4] = 0x02;
    parameter_list[5] = 0x03;
    parameter_list[6] = 0x10;
    parameter_list[7] = 0x10;
    parameter_list[8] = 0x34;
    parameter_list[9] = 0x3C;
    parameter_list[10] = 0x3F;
    parameter_list[11] = 0x3F;
    send_spi_command(0xB1, 12, parameter_list);

    parameter_list[0] = 0x01;
    send_spi_command(0xB3, 1, parameter_list);

    parameter_list[0] = 0x00;
    parameter_list[1] = 0x08;
    parameter_list[2] = 0x6E;
    parameter_list[3] = 0x07;
    parameter_list[4] = 0x01;
    parameter_list[5] = 0x01;
    parameter_list[6] = 0x62;
    parameter_list[7] = 0x01;
    parameter_list[8] = 0x57;
    send_spi_command(0xB4, 9, parameter_list);

#if 0
    parameter_list[0] = 0x01;
    parameter_list[1] = 0x00;
    parameter_list[2] = 0x02;
    parameter_list[3] = 0x0A;
    parameter_list[4] = 0x0F;
    parameter_list[5] = 0x14;
    parameter_list[6] = 0x19;
    parameter_list[7] = 0x21;
    parameter_list[8] = 0x29;
    parameter_list[9] = 0x2F;
    parameter_list[10] = 0x36;
    parameter_list[11] = 0x41;
    parameter_list[12] = 0x4B;
    parameter_list[13] = 0x53;
    parameter_list[14] = 0x5F;
    parameter_list[15] = 0x67;
    parameter_list[16] = 0x6F;
    parameter_list[17] = 0x78;
    parameter_list[18] = 0x82;
    parameter_list[19] = 0x89;
    parameter_list[20] = 0x92;
    parameter_list[21] = 0x9C;
    parameter_list[22] = 0xA5;
    parameter_list[23] = 0xAE;
    parameter_list[24] = 0xB7;
    parameter_list[25] = 0xBE;
    parameter_list[26] = 0xC6;
    parameter_list[27] = 0xCD;
    parameter_list[28] = 0xD6;
    parameter_list[29] = 0xDF;
    parameter_list[30] = 0xE9;
    parameter_list[31] = 0xF2;
    parameter_list[32] = 0xF7;
    parameter_list[33] = 0xFF;
    parameter_list[34] = 0x20;
    parameter_list[35] = 0x0F;
    parameter_list[36] = 0x74;
    parameter_list[37] = 0xA9;
    parameter_list[38] = 0xBD;
    parameter_list[39] = 0x41;
    parameter_list[40] = 0x70;
    parameter_list[41] = 0x85;
    parameter_list[42] = 0xC0;
    parameter_list[43] = 0x00;
    parameter_list[44] = 0x00;
    parameter_list[45] = 0x10;
    parameter_list[46] = 0x15;
    parameter_list[47] = 0x1A;
    parameter_list[48] = 0x21;
    parameter_list[49] = 0x26;
    parameter_list[50] = 0x2C;
    parameter_list[51] = 0x32;
    parameter_list[52] = 0x39;
    parameter_list[53] = 0x41;
    parameter_list[54] = 0x48;
    parameter_list[55] = 0x50;
    parameter_list[56] = 0x57;
    parameter_list[57] = 0x5F;
    parameter_list[58] = 0x66;
    parameter_list[59] = 0x6E;
    parameter_list[60] = 0x75;
    parameter_list[61] = 0x7D;
    parameter_list[62] = 0x84;
    parameter_list[63] = 0x8C;
    parameter_list[64] = 0x93;
    parameter_list[65] = 0x9B;
    parameter_list[66] = 0xA5;
    parameter_list[67] = 0xAF;
    parameter_list[68] = 0xB9;
    parameter_list[69] = 0xC3;
    parameter_list[70] = 0xCD;
    parameter_list[71] = 0xD7;
    parameter_list[72] = 0xE1;
    parameter_list[73] = 0xEB;
    parameter_list[74] = 0xF5;
    parameter_list[75] = 0xFF;
    parameter_list[76] = 0x06;
    parameter_list[77] = 0xCB;
    parameter_list[78] = 0x22;
    parameter_list[79] = 0x22;
    parameter_list[80] = 0x22;
    parameter_list[81] = 0x20;
    parameter_list[82] = 0x00;
    parameter_list[83] = 0x00;
    parameter_list[84] = 0xC0;
    parameter_list[85] = 0x00;
    parameter_list[86] = 0x0A;
    parameter_list[87] = 0x14;
    parameter_list[88] = 0x1E;
    parameter_list[89] = 0x28;
    parameter_list[90] = 0x28;
    parameter_list[91] = 0x31;
    parameter_list[92] = 0x37;
    parameter_list[93] = 0x3E;
    parameter_list[94] = 0x48;
    parameter_list[95] = 0x51;
    parameter_list[96] = 0x59;
    parameter_list[97] = 0x61;
    parameter_list[98] = 0x6A;
    parameter_list[99] = 0x6F;
    parameter_list[100] = 0x78;
    parameter_list[101] = 0x81;
    parameter_list[102] = 0x89;
    parameter_list[103] = 0x90;
    parameter_list[104] = 0x98;
    parameter_list[105] = 0xA0;
    parameter_list[106] = 0xA9;
    parameter_list[107] = 0xB3;
    parameter_list[108] = 0xB9;
    parameter_list[109] = 0xC0;
    parameter_list[110] = 0xC8;
    parameter_list[111] = 0xCD;
    parameter_list[112] = 0xD6;
    parameter_list[113] = 0xDF;
    parameter_list[114] = 0xE9;
    parameter_list[115] = 0xF2;
    parameter_list[116] = 0xF7;
    parameter_list[117] = 0xFF;
    parameter_list[118] = 0x00;
    parameter_list[119] = 0x32;
    parameter_list[120] = 0x0B;
    parameter_list[121] = 0xCE;
    parameter_list[122] = 0xCB;
    parameter_list[123] = 0xF1;
    parameter_list[124] = 0x68;
    parameter_list[125] = 0x85;
    parameter_list[126] = 0xC0;
    send_spi_command(0xC1, 127, parameter_list);
#endif

    parameter_list[0] = 0x0B;
    send_spi_command(0xCC, 1, parameter_list);

    parameter_list[0] = 0x03;
    parameter_list[1] = 0x49;
    parameter_list[2] = 0x4E;
    parameter_list[3] = 0x4C;
    parameter_list[4] = 0x57;
    parameter_list[5] = 0xF4;
    parameter_list[6] = 0x0B;
    parameter_list[7] = 0x4E;
    parameter_list[8] = 0x92;
    parameter_list[9] = 0x57;
    parameter_list[10] = 0x1A;
    parameter_list[11] = 0x99;
    parameter_list[12] = 0x96;
    parameter_list[13] = 0x0C;
    parameter_list[14] = 0x10;
    parameter_list[15] = 0x01;
    parameter_list[16] = 0x47;
    parameter_list[17] = 0x4D;
    parameter_list[18] = 0x57;
    parameter_list[19] = 0x62;
    parameter_list[20] = 0xFF;
    parameter_list[21] = 0x0A;
    parameter_list[22] = 0x4E;
    parameter_list[23] = 0xD1;
    parameter_list[24] = 0x16;
    parameter_list[25] = 0x19;
    parameter_list[26] = 0x98;
    parameter_list[27] = 0xD6;
    parameter_list[28] = 0x0E;
    parameter_list[29] = 0x11;
    send_spi_command(0xE0, 30, parameter_list);
    mdelay(10);

    send_spi_command(0x29, 0, parameter_list);

#endif

}
#endif
/////////////////////////////////////////////////////////////////////

//lsb 090716 : you may add error handling code ^^;
/* probe function */
static int __devinit lcd_spi_probe (struct spi_device *spi)
{
	ENTER_FUNC();
	pspi = spi;

	if (pspi == NULL)
		printk (KERN_ERR "[IRMAVEP] %s : spi device is null\n", __FUNCTION__);
	else
	{
		printk (KERN_ERR "[IRMAVEP] max_speed : %d\n", pspi->max_speed_hz);
		printk (KERN_ERR "[IRMAVEP] chip_select : %d\n", pspi->chip_select);
		printk (KERN_ERR "[IRMAVEP] mode : %d\n", pspi->mode);
	}	

	EXIT_FUNC();

	return 0;
}

/* module remove function */
static int __devexit lcd_spi_remove (struct spi_device *spi)
{
	ENTER_FUNC();
	EXIT_FUNC();

	return 0;
}

/* spi driver data */
static struct spi_driver lcd_spi_driver = {
	.driver = {
		.name = "tl2796_spi",
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
	},

	.probe = lcd_spi_probe,
	.remove = __devexit_p (lcd_spi_remove),
};


/* module init function. register spi driver data */
static int __init lcd_spi_init (void)
{
	int rc;

	ENTER_FUNC();
	rc = spi_register_driver (&lcd_spi_driver);

	EXIT_FUNC();
	return 0;
}

/* module exit function */
static void __exit lcd_spi_exit (void)
{
	spi_unregister_driver (&lcd_spi_driver);
}

module_init (lcd_spi_init); 
#endif /* CONFIG_EF10_BOARD */

module_init(lcdc_panel_init);
