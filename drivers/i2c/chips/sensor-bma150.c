#ifndef __KERNEL__
#define __KERNEL__
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <linux/time.h>
#include <linux/syscalls.h>
#include <mach/mpp.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/i2c/sensor-bma150.h>
#include <linux/pm.h> /*yjw 20100914*/

/* -------------------------------------------------------------------- */
/* debug option */
/* -------------------------------------------------------------------- */
//#define SENSOR_BMA150_DBG_ENABLE
#ifdef SENSOR_BMA150_DBG_ENABLE
#define dbg(fmt, args...)   printk("[SENSOR_BMA150] " fmt, ##args)
#else
#define dbg(fmt, args...)
#endif
#define dbg_func_in()       dbg("[FUNC_IN] %s\n", __func__)
#define dbg_func_out()      dbg("[FUNC_OUT] %s\n", __func__)
#define dbg_line()          dbg("[LINE] %d(%s)\n", __LINE__, __func__)
/* -------------------------------------------------------------------- */



/* -------------------------------------------------------------------- */
/* Definition */
/* -------------------------------------------------------------------- */
#define SENSOR_BMA150_NAME		"sensor-bma150"

/* -------------------------------------------------------------------- */
/* internal variable */
/* -------------------------------------------------------------------- */
struct bma150_data_type
{
	struct i2c_client *pClient;
};
struct bma150_data_type *bma150_data = NULL;
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* internal function */
/* -------------------------------------------------------------------- */
#define BMA150_SLAVE_ADDR	0x38
#define WAKEUP_REG		0x15
#define WAKEUP_MSK		0x01
#define WAKEUP_POS		0x00
#define SLEEP_REG		0x0A
#define SLEEP_MSK		0x01
#define SLEEP_POS		0x00
#define MODE_SLEEP		2
#define MODE_WAKEUP	3

static void bma150_set_mode(int mode)
{
	unsigned char buf[2];
	int           rc;
	
	if(bma150_data != NULL)
	{
		buf[0] = SLEEP_REG;
		if(mode == MODE_SLEEP) buf[1] = 1;
		else buf[1] = 0;
		rc = i2c_master_send(bma150_data->pClient, buf,2);
	}
}

/*
int get_accel(void)
{
	char buf[6];
	int rc;
	short x=0, y=0, z=0;

	if(bma150_data->pClient == NULL) return ACC_XY_ERR;

	buf[0] = 2;
	rc = i2c_master_send(bma150_data->pClient, buf, 1);
	if(rc<0) return ACC_XY_ERR;

	rc = i2c_master_recv(bma150_data->pClient, buf, 6);
	if(rc<0) return ACC_XY_ERR;

	x = (((unsigned char)buf[0]&0xC0)>>6) | (((unsigned char)buf[1]&0xFF)<<2);
	x = x<<6;
	x = x>>6;

	y = (((unsigned char)buf[2]&0xC0)>>6) | (((unsigned char)buf[3]&0xFF)<<2);
	y = y<<6;
	y = y>>6;

	z = (((unsigned char)buf[4]&0xC0)>>6) | (((unsigned char)buf[5]&0xFF)<<2);
	z = z<<6;
	z = z>>6;

	if(x < -127) return ACC_XY_180;
	else if(x > 127) return ACC_XY_0;
	else if(y < -127) return ACC_XY_90;
	else if(y > 127) return ACC_XY_270;

	return ACC_XY_NONE;
}
*/

/* -------------------------------------------------------------------- */


/* -------------------------------------------------------------------- */
/* Misc Driver */
/* -------------------------------------------------------------------- */
static int bma150_misc_open(struct inode *inode, struct file *file) 
{
	dbg_func_in();
	
	if(bma150_data->pClient == NULL)
	{
		printk(KERN_ERR "bma150 : misc_open failed. No i2c-client.\n");
		return -ENODEV;
	}
	
	if (!try_module_get(THIS_MODULE))
	{
		printk(KERN_ERR "bma150 : misc_open failed. can't get module\n");
		return -ENODEV;
	}
	
	dbg_func_out();
	return 0;
}

static ssize_t bma150_misc_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	int rc;
	int i;
		
	dbg_func_in();
	
	#ifdef SENSOR_BMA150_DBG_ENABLE
	for(i=0; i<count; i++)
	{
		dbg("i2c_master_send[%d] = %d\n", i, buf[i]);	
	}
	#endif
	rc = i2c_master_send(bma150_data->pClient, buf, count);
	dbg("i2c_master_send : rc=%d\n", rc);
	
	dbg_func_out();
	return rc;
}

static ssize_t bma150_misc_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	int rc;
	int i;
	
	dbg_func_in();
	
	rc = i2c_master_recv(bma150_data->pClient, buf, count);
	#ifdef SENSOR_BMA150_DBG_ENABLE
	for(i=0; i<count; i+=2)
	{
		dbg("i2c_master_recv[%d] = %d\n", i, buf[i]);
	}
	dbg("i2c_master_recv : rc=%d\n", rc);
	#endif

	dbg_func_out();
	return rc;
}

static int bma150_misc_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	unsigned long reg;
	int rc = 0;

	dbg_func_in();

	dbg("ioctl cmd = %d / arg = %d\n", cmd, arg);

	switch (cmd)
	{
		case 0x706:
			bma150_data->pClient->addr = arg;
			dbg_func_out();
			return 0;
	}

	dbg_func_out();
	return 0;
}

static int bma150_misc_release(struct inode *inode, struct file *file) 
{
	dbg_func_in();
	file->private_data = (void*)NULL;
	module_put(THIS_MODULE);
	dbg_func_out();
	return 0;
}

static struct file_operations bma150_misc_fops = {
	.owner =	THIS_MODULE,
	.open =		bma150_misc_open,
	.read =		bma150_misc_read,
	.write =	bma150_misc_write,
	.ioctl =	bma150_misc_ioctl,
	.release =	bma150_misc_release,
};

static struct miscdevice bma150_misc_device = {
	.minor =	MISC_DYNAMIC_MINOR,
	.name =		SENSOR_BMA150_NAME,
	.fops =		&bma150_misc_fops,
};

/* -------------------------------------------------------------------- */


/* -------------------------------------------------------------------- */
/* i2c Driver */
/* -------------------------------------------------------------------- */

static int __devinit bma150_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct bma150_data_type *dd;
	int rc;

	dbg_func_in();

	if(bma150_data)
	{
		dev_err(&client->dev, "bma150_i2c_probe error\n");
		rc = -EPERM;
		goto probe_exit;
	}

	dd = kzalloc (sizeof(struct bma150_data_type),GFP_KERNEL);
	if(!dd)
	{
		rc = -ENOMEM;
		goto probe_exit;
	}
	bma150_data = dd;
	i2c_set_clientdata(client, dd);
	dd->pClient = client;
	
	rc = misc_register(&bma150_misc_device);
	if (rc) 
	{
		printk(KERN_ERR "bma150 : misc_register failed.\n");
		goto probe_free_exit;
	}
	
	bma150_data->pClient->addr = BMA150_SLAVE_ADDR;
	bma150_set_mode(MODE_WAKEUP);

	dbg_func_out();
	return 0;
	
probe_free_exit:
	bma150_data = NULL;
	i2c_set_clientdata(client, NULL);
probe_exit:
	dbg_func_out();
	return rc;
}

static int bma150_i2c_remove(struct i2c_client *client)
{
	struct bma150_data_type *dd;

	dbg_func_in();
	
	bma150_set_mode(MODE_SLEEP);
	
	misc_deregister(&bma150_misc_device);
	dd = i2c_get_clientdata(client);
	kfree(dd);
	i2c_set_clientdata(client, NULL);
	
	dbg_func_out();

	return 0;
}

static int bma150_i2c_suspend(struct i2c_client *client, pm_message_t state)
{
	dbg_func_in();
	bma150_set_mode(MODE_SLEEP);
	dbg_func_out();
	return 0;
}

static int bma150_i2c_resume(struct i2c_client *client, pm_message_t state)
{
	dbg_func_in();
	bma150_set_mode(MODE_WAKEUP);
	dbg_func_out();
	return 0;
}

static const struct i2c_device_id bma150_i2c_id[] = {
	    { SENSOR_BMA150_NAME, 0 },
	    { }
};

/*yjw 20100914*/
static struct dev_pm_ops bma150_pm_ops = {
	.suspend = bma150_i2c_suspend,
	.resume = bma150_i2c_resume,
};

static struct i2c_driver bma150_i2c_driver = {
	.driver = {
		.name = SENSOR_BMA150_NAME,
		.owner = THIS_MODULE,
		.pm = &bma150_pm_ops, /*yjw 20100914*/
	},
	.probe		= bma150_i2c_probe,
	.remove		= bma150_i2c_remove,
	//.suspend	= bma150_i2c_suspend,
	//.resume	= bma150_i2c_resume,
	.id_table	= bma150_i2c_id,
};
/* -------------------------------------------------------------------- */


/* -------------------------------------------------------------------- */
/* Module Driver */
/* -------------------------------------------------------------------- */

static int __init bma150_driver_init(void)
{
	int rc;   /* initialized below */

	dbg_func_in();
	
	rc = i2c_add_driver(&bma150_i2c_driver);
	if (rc) 
	{
		printk(KERN_ERR "bma150 : i2c_add_driver failed(rc=%d).\n", rc);
		return rc;
	}

	dbg_func_out();
	
	return rc;
}

static void __exit bma150_driver_exit(void)
{
	dbg_func_in();
	return i2c_del_driver(&bma150_i2c_driver);
	dbg_func_out();
}

module_init(bma150_driver_init);
module_exit(bma150_driver_exit);

/* -------------------------------------------------------------------- */
