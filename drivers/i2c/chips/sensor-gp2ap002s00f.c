/* Copyright (c) 2009-2010, Code PANTECH. All rights reserved.
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
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <mach/io.h>
#include <mach/system.h>
#include <mach/gpio.h>
#include <mach/oem_rapi_client.h>
#include <mach/vreg.h>
#include <linux/miscdevice.h>
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <mach/mpp.h>
#include <linux/syscalls.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/pm.h> /*yjw 20100914*/

MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.2");
MODULE_DESCRIPTION("gp2ap002s00f driver");

/* -------------------------------------------------------------------- */
/* debug option */
/* -------------------------------------------------------------------- */
//#define SENSOR_GP2AP002S00F_DBG_ENABLE
#ifdef SENSOR_GP2AP002S00F_DBG_ENABLE
#define sensor_dbg(fmt, args...)   printk("[SENSOR_GP2AP002S00F] " fmt, ##args)
#else
#define sensor_dbg(fmt, args...)
#endif
#define sensor_dbg_func_in()       sensor_dbg("[FUNC_IN] %s\n", __func__)
#define sensor_dbg_func_out()      sensor_dbg("[FUNC_OUT] %s\n", __func__)
#define sensor_dbg_line()          sensor_dbg("[LINE] %d(%s)\n", __LINE__, __func__)
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* MODEL FEATURE */
/* -------------------------------------------------------------------- */
#define GP2AP002S00F_PS_INT_N				34		// interrupt gpio
#define GP2AP002S00F_PS_IRQ					gpio_to_irq(GP2AP002S00F_PS_INT_N)	// interrupt irq
#define GP2AP002S00F_SLAVE_ADDR			0x44

#define SENSOR_I2C_SCL		29
#define SENSOR_I2C_SDA		83
	
#define I2C_SLEEP()	//				usleep(1)

#define GP2AP002S00F_IOCTL_GET_LS			0x11
#define GP2AP002S00F_IOCTL_GET_PS			0x12
#define GP2AP002S00F_IOCTL_SET_CONFIG		0x21	
#define GP2AP002S00F_IOCTL_GET_CONFIG		0x22
#define GP2AP002S00F_IOCTL_SET_CALLSTATUS	0x91
#define GP2AP002S00F_IOCTL_GET_CALLSTATUS	0x92
#define GP2AP002S00F_DEV_MASK		1
#define GP2AP002S00F_IOCTL_GET_ALIVE_STATUS			0x78

#define BMA150_SLAVE_ADDR     0x38
#define BMA150_IOCTL_SET_INIT			0x31
#define BMA150_IOCTL_GET_ACCEL			0x32
#define BMA150_IOCTL_GET_ALIVE_STATUS			0x79
#define BMA150_DEV_MASK		2

#define PS_INT_CLR 0x80
#define HYSTERESIS_PS_BUF_SIZE		7
#define HYSTERESIS_LS_BUF_SIZE		1

#define YAS529_SLAVE_ADDR	0x2E
#define YAS529_IOCTL_WRITE_GET_ROUGHOFFSET		0x41
#define YAS529_IOCTL_READ_GET_ROUGHOFFSET		0x42
#define YAS529_IOCTL_WRITE_SET_INIT				0x43
#define YAS529_IOCTL_READ_SET_INIT				0x44
#define YAS529_IOCTL_SET_COILINIT				0x45
#define YAS529_IOCTL_SET_PREPARE				0x46
#define YAS529_IOCTL_WRITE_GET_MAGFIELD			0x47
#define YAS529_IOCTL_READ_GET_MAGFIELD			0x48
#define YAS529_IOCTL_SET_GPO					0x49
#define YAS529_DEV_MASK		4
#define YAS529_IOCTL_GET_ALIVE_STATUS			0x7A
/* -------------------------------------------------------------------- */
#define SLEEP_REG                       0x0A

#define MODE_SLEEP                      2

extern int call_sleep;

/* -------------------------------------------------------------------- */
/* DEVICE Environment */
/* -------------------------------------------------------------------- */
#define GP2AP002S00F_INPUT_NAME             "sensor-gp2ap002s00f-input"
#define GP2AP002S00F_INPUT_DEVICE           "/dev/sensor-gp2ap002s00f-input"
#define GP2AP002S00F_MISC_NAME				"sensor-gp2ap002s00f"
/* -------------------------------------------------------------------- */


/* -------------------------------------------------------------------- */
/* STRUCT TYPE */
/* -------------------------------------------------------------------- */
struct gp2ap002s00f_ps_type {
	u8		buf[HYSTERESIS_PS_BUF_SIZE];		// hysteresis 용 이전 데이터 저장 버퍼
	u8		val;								// 이전 근접 데이터
	bool	is_called;							// current is calling
	bool	is_suspended;						// current is suspended
	bool	is_near;							// current is proximity
	bool	is_irqenabled;						// current is interrupte enable status
	//struct timeval timestamp;
};

struct gp2ap002s00f_ls_type {
	u8		buf[HYSTERESIS_LS_BUF_SIZE];		// hysteresis 용 이전 데이터 저장할 버퍼
	u8		val;								// 이전 근접 데이터
};

struct gp2ap002s00f_data_type {
//	struct	input_dev *inputdev;				// proximity interrupt를 KEY로 변환해서 report 하는 device
	struct	miscdevice *miscdev;				// driver의 open,close,ioctl 를 사용하기 위한 device
	struct	gp2ap002s00f_ps_type ps;				// proximity sensor
	u32		gpio;								// interrupt 에 사용되는 gpio
	struct	i2c_client *clientp;				// i2c client structure
};

struct bma150_acc_type {
	short x;
	short y;
	short z;
};

/*===========================================================================
GP2AP002S00F PROXIMITY SENSOR  Internal resister map 
Address       SYMBOL                     DATA                                                    Function
0x00           PROX           X      X      X      X      X      X      X     VO          proximity output register ( 0 : no detection, 1: detection)
0x01           GAIN           X      X      X      X    LED     X      X      X           LED drive current in 2 levels�� 0 : small, 1 : large �� 
0x02           HYS         HYSD HYSC HYSC  X   HYSF HYSF HYSF HYSF        HYSC : receiver sensitivity change 
0x03           CYCLE         X      X  CYCL CYCL CYCL OSC    X      X          CYCL : Determine the detection cycle 
0x04           OPMOD        X      X      X   ADS     X      X   VCON  SSD       ASD : Analog circuit shutdown register��0��operating mode��1��sleep mode��
															VCON :VCON switch��0��normal output mode, 1��interrupt output mode��
															SSD: software shutdown��0��shutdown mode, 1��operating mode �� 
0x06           CON            X      X      X   OCON OCON X      X       X         Select switches for enabling VOUT terminal (00:enable 11:disable)
===========================================================================*/

typedef enum 
{    
	PROX_REG               =    0x00,
	GAIN_REG               =    0x01,
	HYS_REG                 =    0x02,
	CYCLE_REG             =    0x03,
	OPMOD_REG            =    0x04,
	CON_REG                 =    0x06
} proximitysensor_reg_type;

typedef enum
{       
	HS_PS_PWR_DOWN,	
	HS_PS_PWR_ON	
}proximitysensor_operating_type;

typedef enum
{
	HS_PS_STATE_NEAR,
	HS_PS_STATE_FAR,
	HS_PS_STATE_NULL
}proximitysensor_state_type;

typedef enum  // [20090813_mirinae]
{
	HS_PS_STATE_OFF,
	HS_PS_STATE_ON
};

/* -------------------------------------------------------------------- */

static struct wake_lock prox_wake_lock;/*yjw 20100919*/
static int has_prox_wake_lock = false; /*yjw 20100919*/
/* -------------------------------------------------------------------- */
/* FUNCTION PROTO TYPE */
/* -------------------------------------------------------------------- */
#ifndef PREVENT_SUSPEND_ENABLE
static void	gp2ap002s00f_interrupt_enable(struct gp2ap002s00f_data_type *dd);
static void	gp2ap002s00f_interrupt_disable(struct gp2ap002s00f_data_type *dd);
#endif
static int	gp2ap002s00f_i2c_command(struct i2c_client *client, u8 cmd);
static int	gp2ap002s00f_i2c_write(struct i2c_client *client, int reg, u8 data);
static int	gp2ap002s00f_i2c_read(struct i2c_client *client, int reg, u8 *buf, int count);

static int	gp2ap002s00f_probe(struct i2c_client *client, const struct i2c_device_id *id);
static void	gp2ap002s00f_work_f(struct work_struct *work);
static irqreturn_t	gp2ap002s00f_interrupt(int irq, void *dev_id);
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* INTERNAL VARIABLE */
/* -------------------------------------------------------------------- */
static struct gp2ap002s00f_data_type *gp2ap002s00f_data;
static	DECLARE_COMPLETION(calibration_complete);
static	DECLARE_WORK(gp2ap002s00f_work, gp2ap002s00f_work_f);
struct bma150_acc_type bma150_acc;
static char devmask = 0;
static int prox_value = 0;


static void bma150_set_mode(int mode)
{
        unsigned char buf[2];

        if(gp2ap002s00f_data != NULL)
        {
                buf[0] = SLEEP_REG;
                if(mode == MODE_SLEEP) buf[1] = 1;
                else buf[1] = 0;
                gp2ap002s00f_data->clientp->addr = BMA150_SLAVE_ADDR;
                i2c_master_send(gp2ap002s00f_data->clientp, buf,2);  I2C_SLEEP();
        }
}

static int bma150_set_init()
{
	// write range_2G (reg : 0x14, val : 0<<3)
	// write bandwidth_25Hz (reg : 0x14, val : 0)
	unsigned char buf[2];
	int rc;
	
	if(gp2ap002s00f_data == NULL) return -1;
	
	buf[0] = 0x14;
	buf[1] = 0;
	gp2ap002s00f_data->clientp->addr = BMA150_SLAVE_ADDR;
	rc = i2c_master_send(gp2ap002s00f_data->clientp, buf,2);  I2C_SLEEP();
	if(rc<0)
	{
		printk("Error Func: %s\n", __FUNCTION__);
		return -1;
	}
	
	return 0;
}

static int bma150_get_accel(struct bma150_acc_type* val)
{
	unsigned char reg, buf[6];
	int rc;

	sensor_dbg_func_in();

	if(gp2ap002s00f_data->clientp == NULL) return -1;

	gp2ap002s00f_data->clientp->addr = BMA150_SLAVE_ADDR;

	reg = 0x02;
	rc = i2c_master_send(gp2ap002s00f_data->clientp, &reg, 1);  I2C_SLEEP();
	if(rc<0)
	{
		printk("Error Func: %s\n", __FUNCTION__);
		return -1;
	}

	rc = i2c_master_recv(gp2ap002s00f_data->clientp, buf, 6); I2C_SLEEP();
	if(rc<0)
	{
		printk("Error Func: %s\n", __FUNCTION__);
		return -1;
	}

	bma150_acc.x = (((unsigned char)buf[0]&0xC0)>>6) | (((unsigned char)buf[1]&0xFF)<<2);
	bma150_acc.x = bma150_acc.x<<6;
	bma150_acc.x = bma150_acc.x>>6;

	bma150_acc.y = (((unsigned char)buf[2]&0xC0)>>6) | (((unsigned char)buf[3]&0xFF)<<2);
	bma150_acc.y = bma150_acc.y<<6;
	bma150_acc.y = bma150_acc.y>>6;

	bma150_acc.z = (((unsigned char)buf[4]&0xC0)>>6) | (((unsigned char)buf[5]&0xFF)<<2);
	bma150_acc.z = bma150_acc.z<<6;
	bma150_acc.z = bma150_acc.z>>6;

//	sensor_dbg("x=%d y=%d z=%d\n", bma150_acc.x, bma150_acc.y, bma150_acc.z);

	memcpy(val, &bma150_acc, sizeof(struct bma150_acc_type));
	/*
	if(x < -127) return ACC_XY_180;
	else if(x > 127) return ACC_XY_0;
	else if(y < -127) return ACC_XY_90;
	else if(y > 127) return ACC_XY_270;
	*/

	return 6;
}

/* -------------------------------------------------------------------- */


static int yas529_write_get_roughoffset(void)
{
	unsigned char reg;
	int rc;

	sensor_dbg_func_in();
	if(gp2ap002s00f_data->clientp == NULL) return -1;

	gp2ap002s00f_data->clientp->addr = YAS529_SLAVE_ADDR;

	reg = 0xC0;	// MS3CDRV_RDSEL_MEASURE
	rc = i2c_master_send(gp2ap002s00f_data->clientp, &reg, 1);  I2C_SLEEP();

	reg = 0x01;	// MS3CDRV_CMD_MEASURE_ROUGHOFFSET
	rc = i2c_master_send(gp2ap002s00f_data->clientp, &reg, 1);  I2C_SLEEP();
	if(rc<0) return -1;

	return 0;
}

static int yas529_read_get_roughoffset(unsigned char* val)
{
	unsigned char reg, buf[2];
	int rc;

	sensor_dbg_func_in();
	if(gp2ap002s00f_data->clientp == NULL) return -1;

	gp2ap002s00f_data->clientp->addr = YAS529_SLAVE_ADDR;

	rc = i2c_master_recv(gp2ap002s00f_data->clientp, val, 6);  I2C_SLEEP();
	if(rc<0) return -1;

	return 0;
}

static int yas529_write_set_init(void)
{
	unsigned char reg;
	int rc;

	sensor_dbg_func_in();
	if(gp2ap002s00f_data->clientp == NULL) return -1;

	gp2ap002s00f_data->clientp->addr = YAS529_SLAVE_ADDR;

	reg = 0x01;	// MS3CDRV_CMD_MEASURE_ROUGHOFFSET
	rc = i2c_master_send(gp2ap002s00f_data->clientp, &reg, 1);  I2C_SLEEP();
	if(rc<0) return -1;
	
	return 0;
}

static int yas529_read_set_init(unsigned char* buf)
{
	
	unsigned char reg, dumy[9];
	int rc;

	sensor_dbg_func_in();
	if(gp2ap002s00f_data->clientp == NULL) return -1;

	gp2ap002s00f_data->clientp->addr = YAS529_SLAVE_ADDR;

	reg = 0xC8;	// MS3CDRV_RDSEL_CALREGISTER
	rc = i2c_master_send(gp2ap002s00f_data->clientp, &reg, 1);  I2C_SLEEP();
	if(rc<0) return -1;

	rc = i2c_master_recv(gp2ap002s00f_data->clientp, dumy, 9);  I2C_SLEEP();
	if(rc<0) return -1;

	rc = i2c_master_recv(gp2ap002s00f_data->clientp, buf, 9);  I2C_SLEEP();
	if(rc<0) return -1;

	return 0;
}

static int yas529_set_coilinit(void)
{
	int i;
	unsigned char buf[16] = { 0x90, 0x81, 0x91, 0x82, 0x92, 0x83, 0x93, 0x84, 0x94, 0x85, 0x95, 0x86, 0x96, 0x87, 0x97, 0x80 };
	int rc;

	sensor_dbg_func_in();
	if(gp2ap002s00f_data->clientp == NULL) return -1;

	gp2ap002s00f_data->clientp->addr = YAS529_SLAVE_ADDR;

	for(i=0; i<16; i++)
	{
		// The operation time(ON time) of each initialization coil should b 1 ms or less.
		rc = i2c_master_send(gp2ap002s00f_data->clientp, &buf[i], 1);  I2C_SLEEP();
		if(rc<0) return -1;
	}

	return 0;
}

static int yas529_set_prepare(void)
{
	int rc;
	unsigned char reg = 0xC0;	// MS3CDRV_RDSEL_MEASURE

	sensor_dbg_func_in();
	if(gp2ap002s00f_data->clientp == NULL) return -1;

	gp2ap002s00f_data->clientp->addr = YAS529_SLAVE_ADDR;

	rc = i2c_master_send(gp2ap002s00f_data->clientp, &reg, 1);  I2C_SLEEP();
	if(rc<0) return -1;

	return 0;
}

static int yas529_set_gpo(unsigned char val)
{
	int rc;
	unsigned char reg = (val) ? 0xF5 : 0xF4;

	sensor_dbg_func_in();
	if(gp2ap002s00f_data->clientp == NULL) return -1;

	gp2ap002s00f_data->clientp->addr = YAS529_SLAVE_ADDR;

	rc = i2c_master_send(gp2ap002s00f_data->clientp, &reg, 1);  I2C_SLEEP();
	if(rc<0) return -1;

	return 0;
}

static int yas529_write_get_magfield(void)
{
	char reg;
	int rc;

	sensor_dbg_func_in();
	if(gp2ap002s00f_data->clientp == NULL) return -1;

	gp2ap002s00f_data->clientp->addr = YAS529_SLAVE_ADDR;

	reg = 0xC0;	// MS3CDRV_RDSEL_MEASURE
	rc = i2c_master_send(gp2ap002s00f_data->clientp, &reg, 1);  I2C_SLEEP();

	reg = 0x02;	// MS3CDRV_CMD_MEASURE_XY1Y2T
	rc = i2c_master_send(gp2ap002s00f_data->clientp, &reg, 1);  I2C_SLEEP();
	if(rc<0) return -1;
	
	return 0;
}

static int yas529_read_get_magfield(unsigned char* buf)
{
	int rc;

	sensor_dbg_func_in();
	if(gp2ap002s00f_data->clientp == NULL) return -1;

	gp2ap002s00f_data->clientp->addr = YAS529_SLAVE_ADDR;

	rc = i2c_master_recv(gp2ap002s00f_data->clientp, buf, 8);  I2C_SLEEP();
	if(rc<0) return -1;
	
	return 0;
}

/************************************************************************/


/* -------------------------------------------------------------------- */
/* gp2ap002s00f REGISTER READING via I2C
 *    reg : 읽을 register address
 *    buf : 읽은 데이터 저장 buffer
 *    count : 읽을 데이터 bytes */
/* -------------------------------------------------------------------- */
static int gp2ap002s00f_i2c_read(struct i2c_client *client, int reg, u8 *buf, int count)
{
	int rc;
	int ret = 0;

	buf[0] = reg;
	gp2ap002s00f_data->clientp->addr = GP2AP002S00F_SLAVE_ADDR;
	rc = i2c_master_send(client, buf, 1);  I2C_SLEEP();
	if (rc != 1) {
		dev_err(&client->dev, "gp2ap002s00f_i2c_read FAILED: read of register %d\n", reg);
		ret = -1;
		goto gp2ap002s00f_i2c_read_exit;
	}
	gp2ap002s00f_data->clientp->addr = GP2AP002S00F_SLAVE_ADDR;
	rc = i2c_master_recv(client, buf, count);  I2C_SLEEP();
	if (rc != count) {
		dev_err(&client->dev, "gp2ap002s00f_i2c_read FAILED: read %d bytes from reg %d\n", count, reg);
		ret = -1;
	}

gp2ap002s00f_i2c_read_exit:
	return ret;
}
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* gp2ap002s00f COMMAND via I2C
 *    cmd : 명령 */
/* -------------------------------------------------------------------- */
static int gp2ap002s00f_i2c_command(struct i2c_client *client, u8 cmd)
{
	u8            buf[2];
	int           rc;
	int           ret = 0;

	buf[0] = cmd;
	gp2ap002s00f_data->clientp->addr = GP2AP002S00F_SLAVE_ADDR;
	rc = i2c_master_send(client, buf, 1);  I2C_SLEEP();
	if (rc != 1) {
		dev_err(&client->dev, "gp2ap002s00f_i2c_command FAILED: writing to command %d\n", cmd);
		ret = -1;
	}
	return ret;
}
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* gp2ap002s00f REGISTER WRITING via I2C
 *    reg : 쓸 register address
 *    data : 쓸 데이터 */
/* -------------------------------------------------------------------- */
static int gp2ap002s00f_i2c_write(struct i2c_client *client, int reg, u8 data)
{
	u8            buf[2];
	int           rc;
	int           ret = 0;


	buf[0] = (u8)reg;
	buf[1] = data;
	gp2ap002s00f_data->clientp->addr = GP2AP002S00F_SLAVE_ADDR;

	rc= i2c_master_send(client, buf, 2);  I2C_SLEEP();

	if (rc != 2) {
		dev_err(&client->dev, "gp2ap002s00f_i2c_write FAILED: writing to reg %d, rc: %d, addr: %02x\n", 
			reg, rc, client->addr);
		ret = -1;
	}
	return ret;
}

/* -------------------------------------------------------------------- */


#ifndef PREVENT_SUSPEND_ENABLE
/* -------------------------------------------------------------------- */
/* INTERRUPT ENABLE */
/* -------------------------------------------------------------------- */
static void gp2ap002s00f_interrupt_enable(struct gp2ap002s00f_data_type *dd)
{

}
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* INTERRUPT DISABLE */
/* -------------------------------------------------------------------- */
static void gp2ap002s00f_interrupt_disable(struct gp2ap002s00f_data_type *dd)
{

}
/* -------------------------------------------------------------------- */
#endif

/* -------------------------------------------------------------------- */
/* INTERRUPT SERVICE */
/* -------------------------------------------------------------------- */
static void gp2ap002s00f_work_f(struct work_struct *work)
{
	struct gp2ap002s00f_data_type   *dd = gp2ap002s00f_data;
	u8	psint[2];
	int	rc, write_data;

	disable_irq(gpio_to_irq(GP2AP002S00F_PS_INT_N));/*yjw 20100925*/    
	rc = gp2ap002s00f_i2c_read(dd->clientp,  (PROX_REG | PS_INT_CLR), psint, 2);

//	printk("----------- INTERRUPT (%02x),(%02x) \n", psint[0], psint[1]);

	if(psint[1] == 0xff)	// near
	{
		rc = gp2ap002s00f_i2c_write(dd->clientp, HYS_REG, 0x20);		
		prox_value = HS_PS_STATE_NEAR;	

		if(call_sleep == true)/*yjw 20100919*/
		{
		    if(has_prox_wake_lock == true)
		    {
		        printk(KERN_ERR "%s: ----------- wake_unlock(&prox_wake_lock)\n",__func__);
		        wake_unlock(&prox_wake_lock);
			 has_prox_wake_lock =false;	
		    }
		}
	}
	else if(psint[1] == 0xfe)	// far
	{
		rc = gp2ap002s00f_i2c_write(dd->clientp, HYS_REG, 0x40);			
		prox_value = HS_PS_STATE_FAR;
		
		if(call_sleep == true)/*yjw 20100919*/
		{
		    printk(KERN_ERR "%s: ----------- wake_lock(&prox_wake_lock)\n",__func__);
		    wake_lock(&prox_wake_lock);
		    has_prox_wake_lock = true;	
		}
	}

#if 0	
	rc = gp2ap002s00f_i2c_write(dd->clientp, CON_REG, 0x40);
#else /*yjw 20100925*/
       rc = gp2ap002s00f_i2c_write(dd->clientp, CON_REG, 0x18);
#endif
       enable_irq(gpio_to_irq(GP2AP002S00F_PS_INT_N));/*yjw 20100925*/
	rc = gp2ap002s00f_i2c_write(dd->clientp, CON_REG, 0x00);	

	return;
}

/* -------------------------------------------------------------------- */
/* GET PROXIMITY SENSOR DATA */
/* -------------------------------------------------------------------- */
static int gp2ap002s00f_get_ps(struct gp2ap002s00f_data_type *dd)
{
#if 0
	u8 val = 0;
	int i, rc;
	int retryCount = 100;
	int checkSum = 0;
	bool loop = 1;
	u8 rbuf[2];

	if(dd->clientp == NULL ) return 0;

	while(loop)
	{
		checkSum = 0;

		// (1) read adc
		rc = gp2ap002s00f_i2c_read(dd->clientp, (PROX_REG | PS_INT_CLR), &rbuf, 2);
		if(rc < 0) return 0;

		// (2) push buf
		for(i=0; i<HYSTERESIS_PS_BUF_SIZE-1; i++) dd->ps.buf[i] = dd->ps.buf[i+1];
		dd->ps.buf[HYSTERESIS_PS_BUF_SIZE-1] = rbuf;

		// (3) compare
		for(i=0; i<HYSTERESIS_PS_BUF_SIZE-1; i++)
		{
			if( dd->ps.buf[HYSTERESIS_PS_BUF_SIZE-1] == dd->ps.buf[i] )
				checkSum++;
		}
		if(checkSum < HYSTERESIS_PS_BUF_SIZE-1)
		{
			retryCount--;
			if(retryCount < 0) loop = 0;
			else msleep(10);
		}
		else
		{
			dd->ps.val = dd->ps.buf[HYSTERESIS_PS_BUF_SIZE-1];
			loop = 0;
		}
	};

	val = dd->ps.val;
	return val;
#endif

       if(has_prox_wake_lock == true) /*yjw 20100920*/
       {
           printk(KERN_ERR "%s: ----------- wake_unlock(&prox_wake_lock)\n",__func__);
           wake_unlock(&prox_wake_lock);
   	    has_prox_wake_lock =false;	
       }
 	
	printk("PROX value: %d\n", prox_value);

	return prox_value;
}
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* MISC DEVICE IOCTL */
/* -------------------------------------------------------------------- */
static int gp2ap002s00f_miscdev_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	struct gp2ap002s00f_data_type *dd = gp2ap002s00f_data;
	void __user *argp = (void __user *)arg;
	unsigned long reg, val;
	struct bma150_acc_type acc_val;
	unsigned char roughoffset_val[6];
	unsigned char magfield_val[8];
	unsigned char init_val[9];
	int rc = 0;
//sensor_dbg("--- ioctl : 0x%02X", cmd);
	if(dd == NULL) return -EFAULT;

	sensor_dbg("gp2ap002s00f_miscdev_ioctl cmd: %d\n", cmd);

	switch (cmd)
	{
		case GP2AP002S00F_IOCTL_GET_PS:
			val = (unsigned long)gp2ap002s00f_get_ps(dd);
			if(copy_to_user(argp, &val, sizeof(unsigned long))) rc = -EFAULT;
			break;
/*
		case GP2AP002S00F_IOCTL_SET_CONFIG:
			reg = (arg>>16) & 0xFFFF;
			val = arg & 0xFFFF;
			gp2ap002s00f_i2c_write(dd->clientp, reg, val);
			break;

		case GP2AP002S00F_IOCTL_GET_CONFIG:
			gp2ap002s00f_i2c_read(dd->clientp, (int)arg, &val, 1);
			if(copy_to_user(argp, &val, sizeof(unsigned long))) rc = -EFAULT;
			break;
*/
		case GP2AP002S00F_IOCTL_SET_CALLSTATUS:
			break;

		case GP2AP002S00F_IOCTL_GET_CALLSTATUS:
			val = (unsigned long)dd->ps.is_called;
			if(copy_to_user(argp, &val, sizeof(unsigned long))) rc = -EFAULT;
			break;

		case 0x706:
			gp2ap002s00f_data->clientp->addr = arg;
			break;

		case BMA150_IOCTL_SET_INIT:
			rc = bma150_set_init();
			break;

		case BMA150_IOCTL_GET_ACCEL:
			if( bma150_get_accel(&acc_val) == 6)
			{
				if(copy_to_user(argp, &acc_val, 6)) rc = -EFAULT;
			}
			break;

		case YAS529_IOCTL_WRITE_GET_ROUGHOFFSET:
			rc = yas529_write_get_roughoffset();
			break;

		case YAS529_IOCTL_READ_GET_ROUGHOFFSET:
			if ( yas529_read_get_roughoffset(roughoffset_val) == 0 )
			{
				if(copy_to_user(argp, &roughoffset_val, 6)) rc = -EFAULT;
			}
			break;

		case YAS529_IOCTL_WRITE_SET_INIT:
			rc = yas529_write_set_init();
			break;

		case YAS529_IOCTL_READ_SET_INIT:
			rc = yas529_read_set_init(init_val);
			break;

		case YAS529_IOCTL_SET_COILINIT:
			rc = yas529_set_coilinit();
			break;

		case YAS529_IOCTL_SET_PREPARE:
			rc = yas529_set_prepare();
			break;

		case YAS529_IOCTL_WRITE_GET_MAGFIELD:
			rc = yas529_write_get_magfield();
			break;

		case YAS529_IOCTL_READ_GET_MAGFIELD:
			if ( yas529_read_get_magfield(magfield_val) == 0 )
			{
				if(copy_to_user(argp, magfield_val, 8)) rc = -EFAULT;
			}
			break;

		case YAS529_IOCTL_SET_GPO:
			rc = yas529_set_gpo((unsigned char)arg);
			break;

		case BMA150_IOCTL_GET_ALIVE_STATUS:
			rc = (devmask & BMA150_DEV_MASK) ? 0 : -EFAULT; 
			break;

		case YAS529_IOCTL_GET_ALIVE_STATUS:
			rc = (devmask & YAS529_DEV_MASK) ? 0 : -EFAULT;
			break;
	}

	return rc;
}
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* MISC DEVICE OPEN */
/* -------------------------------------------------------------------- */
static int gp2ap002s00f_miscdev_open(struct inode *inode, struct file *file) 
{
       int rc = 0;/*yjw 20101122*/
	struct gp2ap002s00f_data_type *dd = gp2ap002s00f_data; 

       /*yjw 20101122  for ESD test   start---------*/
	disable_irq(gpio_to_irq(GP2AP002S00F_PS_INT_N));
       rc  = gp2ap002s00f_i2c_write(dd->clientp, OPMOD_REG, 0x02);	
       rc  = gp2ap002s00f_i2c_write(dd->clientp, CON_REG, 0x18);	
       rc  = gp2ap002s00f_i2c_write(dd->clientp, HYS_REG, 0x40);	
       rc  = gp2ap002s00f_i2c_write(dd->clientp, OPMOD_REG, 0x03);	
       enable_irq(gpio_to_irq(GP2AP002S00F_PS_INT_N));
       rc  = gp2ap002s00f_i2c_write(dd->clientp, CON_REG, 0x00);			      
       prox_value = HS_PS_STATE_FAR;	 
	sensor_dbg("gp2ap002s00f_miscdev_open: The prox sensor is reset when initialize() is called\n");   
       /*yjw 20101122  end---------*/
	   
	if(dd == NULL)
	{
		sensor_dbg(KERN_ERR "gp2ap002s00f : misc_open failed. No gp2ap002s00f_data.\n");
		return -ENODEV;
	}
	
	if (!try_module_get(THIS_MODULE))
	{
		sensor_dbg(KERN_ERR "gp2ap002s00f : misc_open failed. can't get module\n");
		return -ENODEV;
	}
	
	return 0;
}
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* MISC DEVICE WRITE */
/* -------------------------------------------------------------------- */
static ssize_t gp2ap002s00f_miscdev_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	struct gp2ap002s00f_data_type *dd = gp2ap002s00f_data;
	int rc;

	if(dd == NULL)
	{
		sensor_dbg(KERN_ERR "gp2ap002s00f : misc_write failed. No gp2ap002s00f_data.\n");
		return -ENODEV;
	}
		
	rc = i2c_master_send(dd->clientp, buf, count);  I2C_SLEEP();
	return rc;
}
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* MISC DEVICE READ */
/* -------------------------------------------------------------------- */
static ssize_t gp2ap002s00f_miscdev_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	struct gp2ap002s00f_data_type *dd = gp2ap002s00f_data; 
	int rc;

	if(dd == NULL)
	{
		sensor_dbg(KERN_ERR "gp2ap002s00f : misc_read failed. No gp2ap002s00f_data.\n");
		return -ENODEV;
	}
	
	rc = i2c_master_recv(dd->clientp, buf, count);  I2C_SLEEP();
	return rc;
}
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* MISC DEVICE RELEASE */
/* -------------------------------------------------------------------- */
static int gp2ap002s00f_miscdev_release(struct inode *inode, struct file *file) 
{
	file->private_data = (void*)NULL;
	module_put(THIS_MODULE);
	return 0;
}
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* MISC DEVICE FILE OPERATION & PROPERTY*/
/* -------------------------------------------------------------------- */
static struct file_operations gp2ap002s00f_miscdev_fops = {
	.owner =	THIS_MODULE,
	.open =		gp2ap002s00f_miscdev_open,
	.read =		gp2ap002s00f_miscdev_read,
	.write =	gp2ap002s00f_miscdev_write,
	.ioctl =	gp2ap002s00f_miscdev_ioctl,
	.release =	gp2ap002s00f_miscdev_release,
};

static struct miscdevice gp2ap002s00f_misc_device = {
	.minor =	MISC_DYNAMIC_MINOR,
	.name =		GP2AP002S00F_MISC_NAME,
	.fops =		&gp2ap002s00f_miscdev_fops,
};
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* INPUT DEVICE OPEN */
/* -------------------------------------------------------------------- */
static int gp2ap002s00f_inputdev_open(struct input_dev *dev)
{
	int rc = 0;
	struct gp2ap002s00f_data_type *dd = gp2ap002s00f_data; 

	if (!dd->clientp) {
		sensor_dbg("gp2ap002s00f_inputdev_open: no i2c adapter present\n");
		rc = -ENODEV;
	}

	return rc;
}
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* INPUT DEVICE CLOSE */
/* -------------------------------------------------------------------- */
static void gp2ap002s00f_inputdev_close(struct input_dev *dev)
{
	struct gp2ap002s00f_data_type *dd = gp2ap002s00f_data;
//	if(dd->ps.is_irqenabled == GP2AP002S00F_STATUS_IRQ_ENABLED) 
		free_irq(GP2AP002S00F_PS_IRQ, NULL);
}
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* INTERRUPT HANDLER */
/* -------------------------------------------------------------------- */
static irqreturn_t gp2ap002s00f_interrupt(int irq, void *dev_id)
{
//	printk("get gp2ap002s00f_interrupt interrupt!!!\n");
	schedule_work(&gp2ap002s00f_work);
	return IRQ_HANDLED;
}
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* I2C DRIVER REMOVE */
/* -------------------------------------------------------------------- */
static int __exit gp2ap002s00f_remove(struct i2c_client *client)
{
	struct gp2ap002s00f_data_type			*dd;

	dd = i2c_get_clientdata(client);

	wake_unlock(&prox_wake_lock);/*yjw 20100919*/

	kfree(dd);
	i2c_set_clientdata(client, NULL);
	gp2ap002s00f_data = NULL;

	return 0;
}
/* -------------------------------------------------------------------- */


void gp2ap002s00f_normal_oper_mode(struct gp2ap002s00f_data_type *dd, 
											proximitysensor_operating_type mode_val )
{
	switch(mode_val)
    	{
       	case HS_PS_PWR_ON:
                 	gp2ap002s00f_i2c_write(dd->clientp, GAIN_REG, 0x08);  //LED drive current in 2 levels�� 0 : small, 4 : large ��     
			gp2ap002s00f_i2c_write(dd->clientp, HYS_REG, 0xC2);       //receiver sensitivity            
			gp2ap002s00f_i2c_write(dd->clientp, CYCLE_REG, 0x04); 		
			gp2ap002s00f_i2c_write(dd->clientp, OPMOD_REG, 0x01); 		
        		break;
        
        	case HS_PS_PWR_DOWN:
			gp2ap002s00f_i2c_write(dd->clientp, OPMOD_REG, 0x00); 					
        		break;
        
        	default:
        		break;
	}

}

/* -------------------------------------------------------------------- */
/* SUSPEND Function */
/* -------------------------------------------------------------------- */
#ifdef CONFIG_PM
static int gp2ap002s00f_suspend(struct i2c_client *client, pm_message_t state)
{
       /*yjw 20100901*/
       int result;
	//unsigned mpp_20; /*yjw 20100925*/   
	struct gp2ap002s00f_data_type   *dd = gp2ap002s00f_data;
	bma150_set_mode(MODE_SLEEP);

   
       if(call_sleep == true)
       {
               printk(KERN_ERR "%s: set_irq_wake(gpio_to_irq(GP2AP002S00F_PS_INT_N), 1)\n",  __func__);    		 	   
               set_irq_wake(gpio_to_irq(GP2AP002S00F_PS_INT_N), 1);		 	   
       }
	else /*yjw 20100919*/
	{
#if 0	
	       //VDD_SEN_EN low  ---> 2.8V_SEN off
      	      mpp_20 = 19;
             result = mpp_config_digital_out(mpp_20, MPP_CFG(MPP_DLOGIC_LVL_MSME, MPP_DLOGIC_OUT_CTRL_LOW));  /*yjw 20100914*/
             printk(KERN_ERR"%s: VDD_SEN_EN = LOW (result: %d)\n",  __func__, result);
#else /*yjw 20100925*/
             disable_irq(gpio_to_irq(GP2AP002S00F_PS_INT_N));
             result = gp2ap002s00f_i2c_write(dd->clientp, OPMOD_REG, 0x02);	
             result = gp2ap002s00f_i2c_write(dd->clientp, CON_REG, 0x18);	
	      result = gp2ap002s00f_i2c_write(dd->clientp, HYS_REG, 0x40);	
	      printk(KERN_ERR"%s: OPMOD_REG(0x02), CON_REG(0x18),HYS_REG(0x40)\n",  __func__, result);	  
#endif
	}

	return 0;
}
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* RESUME Function */
/* -------------------------------------------------------------------- */
static int gp2ap002s00f_resume(struct i2c_client *client)
{
       /*yjw 20100901*/
       int result;
	//unsigned mpp_20; /*yjw 20100925*/  
	struct gp2ap002s00f_data_type   *dd = gp2ap002s00f_data;

	
//	printk("gp2ap002s00f_resume!!!!\n");
        if(call_sleep == true)
        { 
              printk(KERN_ERR "%s: set_irq_wake(gpio_to_irq(GP2AP002S00F_PS_INT_N), 0)\n",  __func__); 
		if(has_prox_wake_lock == true)/*yjw 20100919*/
		{
		    printk(KERN_ERR "%s: ----------- wake_unlock(&prox_wake_lock)\n",__func__);
		    wake_unlock(&prox_wake_lock);
		    has_prox_wake_lock = false;	
		}
		set_irq_wake(gpio_to_irq(GP2AP002S00F_PS_INT_N), 0);
        }
	 else /*yjw 20100919*/
	 {
#if 0	 
	       // VDD_SEN_EN high ---> 2.8V_SEN on
     	       mpp_20 = 19;
              result = mpp_config_digital_out(mpp_20, MPP_CFG(MPP_DLOGIC_LVL_MSME, MPP_DLOGIC_OUT_CTRL_HIGH));  /*yjw 20100914*/
              printk(KERN_ERR "%s: VDD_SEN_EN = HIGH (result: %d)\n",  __func__, result);
#else /*yjw 20100925*/
              result = gp2ap002s00f_i2c_write(dd->clientp, OPMOD_REG, 0x03);	
              enable_irq(gpio_to_irq(GP2AP002S00F_PS_INT_N));
              result = gp2ap002s00f_i2c_write(dd->clientp, CON_REG, 0x00);	
		printk(KERN_ERR "%s: OPMOD_REG(0x03), CON_REG(0x00)\n",  __func__, result);	  
#endif
              prox_value = HS_PS_STATE_FAR;/*yjw 20101021*/
	 }

	return 0;
}
#endif
/* -------------------------------------------------------------------- */

static void device_check(void)
{
	int rc;
	unsigned char buf;
	devmask = 0;
	
	gp2ap002s00f_data->clientp->addr = BMA150_SLAVE_ADDR;
	buf = 0x00;
	if( i2c_master_send(gp2ap002s00f_data->clientp, &buf, 1) >= 0)
	{
		rc = i2c_master_recv(gp2ap002s00f_data->clientp, &buf, 1);
		if( (rc>=0) && (buf&0x02) ) devmask |= BMA150_DEV_MASK;
	}

	gp2ap002s00f_data->clientp->addr = YAS529_SLAVE_ADDR;
	buf = 0x00;
	if( i2c_master_send(gp2ap002s00f_data->clientp, &buf, 1) >= 0)
	{
		devmask |= YAS529_DEV_MASK;
	}
		
}


/* -------------------------------------------------------------------- */
/* I2C DRIVER PROPERTY */
/* -------------------------------------------------------------------- */
static const struct i2c_device_id gp2ap002s00f_id[] = {
	{ "sensor-gp2ap002s00f", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, gp2ap002s00f_id);

/*yjw 20100914*/
static struct dev_pm_ops gp2ap002s00f_pm_ops = {
	.suspend = gp2ap002s00f_suspend,
	.resume = gp2ap002s00f_resume,
};

static struct i2c_driver gp2ap002s00f_driver = {
	.driver = {
		.name   = "sensor-gp2ap002s00f",
		.owner  = THIS_MODULE,
		.pm = &gp2ap002s00f_pm_ops, /*yjw 20100914*/
	},
	.probe    = gp2ap002s00f_probe,
	.remove   =  __exit_p(gp2ap002s00f_remove),
	//#ifdef CONFIG_PM
	//.suspend  = gp2ap002s00f_suspend,
	//.resume   = gp2ap002s00f_resume,
	//#endif
	.id_table = gp2ap002s00f_id,
};
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* I2C DRIVER PROBE */
/* -------------------------------------------------------------------- */
static int gp2ap002s00f_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int			                rc;
	struct gp2ap002s00f_data_type	*dd;

	sensor_dbg_func_in();

	// (1) 2.8V_SEN turn ON
	mpp_config_digital_out(19/*mpp20*/, MPP_CFG(MPP_DLOGIC_LVL_MSME, MPP_DLOGIC_OUT_CTRL_HIGH));
	
       wake_lock_init(&prox_wake_lock, WAKE_LOCK_SUSPEND, "prox-sensor");/*yjw 20100919*/

	if (gp2ap002s00f_data) {
		dev_err(&client->dev,
			"gp2ap002s00f_probe: only a single gp2ap002s00f supported\n");
		rc = -EPERM;
		goto probe_exit;
	}

	dd = kzalloc(sizeof *dd, GFP_KERNEL);
	if (!dd) {
		rc = -ENOMEM;
		goto probe_exit;
	}
	gp2ap002s00f_data = dd;
	i2c_set_clientdata(client, dd);
	dd->clientp = client;
	client->driver = &gp2ap002s00f_driver;

	dd->gpio = GP2AP002S00F_PS_INT_N;

	GPIO_CFG(GP2AP002S00F_PS_INT_N, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA);
	
/*
	dd->inputdev = input_allocate_device();
	if (!dd->inputdev) {
		rc = -ENOMEM;
		goto probe_free_exit;
	}
	input_set_drvdata(dd->inputdev, dd);
	dd->inputdev->open       = gp2ap002s00f_inputdev_open;
	dd->inputdev->close      = gp2ap002s00f_inputdev_close;
	dd->inputdev->name       = GP2AP002S00F_INPUT_NAME;
	dd->inputdev->id.bustype = BUS_I2C;
	dd->inputdev->id.product = 1;
	dd->inputdev->id.version = 2;
	set_bit(KEY_PROXIMITY, dd->inputdev->keybit);
	set_bit(EV_KEY, dd->inputdev->evbit);

	rc = input_register_device(dd->inputdev);
	if (rc) {
		dev_err(&client->dev, "gp2ap002s00f_probe: input_register_device rc=%d\n", rc);
		goto probe_fail_reg_inputdev;
	}
*/
	device_check();

	dd->miscdev = &gp2ap002s00f_misc_device;
	rc = misc_register(dd->miscdev);
	if (rc) 
	{
		dev_err(&dd->clientp->dev, "gp2ap002s00f_probe: misc_register rc=%d\n", rc);
		goto probe_fail_reg_miscdev;
	}

	//LED drive current in 2 levels�� 0 : small, 4 : large ��
	rc = gp2ap002s00f_i2c_write(dd->clientp, GAIN_REG, 0x08);
//	if(rc) goto gp2ap002s00f_init_exit;

	//receiver sensitivity 
	rc = gp2ap002s00f_i2c_write(dd->clientp, HYS_REG, 0x40);
//	if(rc) goto gp2ap002s00f_init_exit;

	rc = gp2ap002s00f_i2c_write(dd->clientp, CYCLE_REG, 0x04);
//	if(rc) goto gp2ap002s00f_init_exit;
	
	rc = gp2ap002s00f_i2c_write(dd->clientp, OPMOD_REG, 0x03);
//	if(rc) goto gp2ap002s00f_init_exit;

//	gp2ap002s00f_normal_oper_mode(dd, HS_PS_PWR_ON);
	dd->gpio = GP2AP002S00F_PS_INT_N;

        gpio_tlmm_config(GPIO_CFG(GP2AP002S00F_PS_INT_N, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA), 0);

	rc = request_irq(gpio_to_irq(dd->gpio),  gp2ap002s00f_interrupt, IRQF_TRIGGER_FALLING,
				 "proximity_sensor", dd);

	if (rc) 
	{
		dev_err(&dd->clientp->dev, "gp2ap002s00f_probe: request irq fail rc=%d\n", rc);
		goto probe_fail_reg_miscdev;
	}

	prox_value = HS_PS_STATE_FAR;

	return 0;

gp2ap002s00f_init_exit:
	sensor_dbg("\n\n***********[ERROR] gp2ap002s00f write fail!!!***********\n\n");

probe_fail_reg_miscdev:
	dd->miscdev = NULL;

//probe_fail_reg_inputdev:
//	input_free_device(dd->inputdev);

probe_free_exit:
	kfree(dd);
	gp2ap002s00f_data = NULL;
	i2c_set_clientdata(client, NULL);
probe_exit:
	return rc;
}
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* THIS DRIVER INIT */
/* -------------------------------------------------------------------- */
static int __init gp2ap002s00f_init(void)
{
	int rc;

	sensor_dbg_func_in();

	rc = i2c_add_driver(&gp2ap002s00f_driver);
	if (rc) {
		printk(KERN_ERR "gp2ap002s00f_init FAILED: i2c_add_driver rc=%d\n",
		       rc);
		goto init_exit;
	}

	return 0;

init_exit:
	return rc;
}
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* THIS DRIVER EXIT */
/* -------------------------------------------------------------------- */

static void __exit gp2ap002s00f_exit(void)
{
	sensor_dbg_func_in();
	i2c_del_driver(&gp2ap002s00f_driver);
}
/* -------------------------------------------------------------------- */

module_init(gp2ap002s00f_init);
module_exit(gp2ap002s00f_exit);
