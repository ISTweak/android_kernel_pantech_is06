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
#include <linux/pm.h> /*yjw 20100914*/

/* -------------------------------------------------------------------- */
/* debug option */
/* -------------------------------------------------------------------- */
//#define SENSOR_YAS529_DBG_ENABLE
#ifdef SENSOR_YAS529_DBG_ENABLE
#define dbg(fmt, args...)   printk("[SENSOR_YAS529] " fmt, ##args)
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
#define SENSOR_YAS529_NAME		"sensor-yas529"
#define YAS529_SLAVE_ADDR			0x2E

#define SET_BITSLICE(regvar, bitname, val) (regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK)

/* -------------------------------------------------------------------- */
/* internal variable */
/* -------------------------------------------------------------------- */
struct yas529_data_type
{
	struct i2c_client *pClient;
};
struct yas529_data_type *yas529_data;
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* internal function */
/* -------------------------------------------------------------------- */


/* -------------------------------------------------------------------- */


/* -------------------------------------------------------------------- */
/* Misc Driver */
/* -------------------------------------------------------------------- */
static int yas529_misc_open(struct inode *inode, struct file *file) 
{
	dbg_func_in();
	
	if(yas529_data->pClient == NULL)
	{
		printk(KERN_ERR "yas529 : misc_open failed. No i2c-client.\n");
		return -ENODEV;
	}
	
	if (!try_module_get(THIS_MODULE))
	{
		printk(KERN_ERR "yas529 : misc_open failed. can't get module\n");
		return -ENODEV;
	}
	
	dbg_func_out();
	return 0;
}

static ssize_t yas529_misc_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	int rc;
	int i;
		
	dbg_func_in();
	
	#ifdef SENSOR_YAS529_DBG_ENABLE
	for(i=0; i<count; i++)
	{
		dbg("i2c_master_send[%d] = %d\n", i, buf[i]);	
	}
	#endif
	rc = i2c_master_send(yas529_data->pClient, buf, count);
	dbg("i2c_master_send : rc=%d\n", rc);
	
	dbg_func_out();
	return rc;
}

static ssize_t yas529_misc_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	int rc;
	int i;
	
	dbg_func_in();
	
	rc = i2c_master_recv(yas529_data->pClient, buf, count);
	#ifdef SENSOR_YAS529_DBG_ENABLE
	for(i=0; i<count; i++)
	{
		dbg("i2c_master_recv[%d] = %d\n", i, buf[i]);
	}
	#endif
	dbg("i2c_master_recv : rc=%d\n", rc);
	
	dbg_func_out();
	return rc;
}

static int yas529_misc_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	unsigned long reg;
	int rc = 0;

	dbg_func_in();

	dbg("ioctl cmd = %d / arg = %d\n", cmd, arg);

	switch (cmd)
	{
		case 0x706:
			yas529_data->pClient->addr = arg;
			dbg_func_out();
			return 0;
	}

	dbg_func_out();
	return 0;
}

static int yas529_misc_release(struct inode *inode, struct file *file) 
{
	dbg_func_in();
	file->private_data = (void*)NULL;
	module_put(THIS_MODULE);
	dbg_func_out();
	return 0;
}

static struct file_operations yas529_misc_fops = {
	.owner =	THIS_MODULE,
	.open =		yas529_misc_open,
	.read =		yas529_misc_read,
	.write =	yas529_misc_write,
	.ioctl =	yas529_misc_ioctl,
	.release =	yas529_misc_release,
};

static struct miscdevice yas529_misc_device = {
	.minor =	MISC_DYNAMIC_MINOR,
	.name =		SENSOR_YAS529_NAME,
	.fops =		&yas529_misc_fops,
};

/* -------------------------------------------------------------------- */



/* -------------------------------------------------------------------- */
/* i2c Driver */
/* -------------------------------------------------------------------- */

static int __devinit yas529_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct yas529_data_type *dd;
	int rc;

	dbg_func_in();

	if(yas529_data)
	{
		dev_err(&client->dev, "yas529_i2c_probe error\n");
		rc = -EPERM;
		goto probe_exit;
	}

	dd = kzalloc (sizeof(struct yas529_data_type),GFP_KERNEL);
	if(!dd)
	{
		rc = -ENOMEM;
		goto probe_exit;
	}
	yas529_data = dd;
	i2c_set_clientdata(client, dd);
	dd->pClient = client;
	
	rc = misc_register(&yas529_misc_device);
	if (rc) 
	{
		printk(KERN_ERR "yas529 : misc_register failed.\n");
		goto probe_free_exit;
	}
	
	yas529_data->pClient->addr = YAS529_SLAVE_ADDR;
	
	dbg_func_out();
	return 0;
	
probe_free_exit:
	yas529_data = NULL;
	i2c_set_clientdata(client, NULL);
probe_exit:
	dbg_func_out();
	return rc;
}

static int yas529_i2c_remove(struct i2c_client *client)
{
	struct yas529_data_type *dd;

	dbg_func_in();
	
	misc_deregister(&yas529_misc_device);
	dd = i2c_get_clientdata(client);
	kfree(dd);
	i2c_set_clientdata(client, NULL);
	
	dbg_func_out();

	return 0;
}

static int yas529_i2c_suspend(struct i2c_client *client, pm_message_t state)
{
	dbg_func_in();
	dbg_func_out();
	return 0;
}

static int yas529_i2c_resume(struct i2c_client *client, pm_message_t state)
{
	dbg_func_in();
	dbg_func_out();
	return 0;
}

static const struct i2c_device_id yas529_i2c_id[] = {
	    { SENSOR_YAS529_NAME, 0 },
	    { }
};

/*yjw 20100914*/
static struct dev_pm_ops yas529_pm_ops = {
	.suspend = yas529_i2c_suspend,
	.resume = yas529_i2c_resume,
};

static struct i2c_driver yas529_i2c_driver = {
	.driver = {
		.name = SENSOR_YAS529_NAME,
		.owner = THIS_MODULE,
		.pm = &yas529_pm_ops, /*yjw 20100914*/
	},
	.probe		= yas529_i2c_probe,
	.remove		= yas529_i2c_remove,
	//.suspend	= yas529_i2c_suspend,
	//.resume	= yas529_i2c_resume,
	.id_table	= yas529_i2c_id,
};
/* -------------------------------------------------------------------- */


/* -------------------------------------------------------------------- */
/* Module Driver */
/* -------------------------------------------------------------------- */

static int __init yas529_driver_init(void)
{
	int rc;   /* initialized below */

	dbg_func_in();
	
	rc = i2c_add_driver(&yas529_i2c_driver);
	if (rc) 
	{
		printk(KERN_ERR "yas529 : i2c_add_driver failed(rc=%d).\n", rc);
		return rc;
	}

	dbg_func_out();
	
	return rc;
}

static void __exit yas529_driver_exit(void)
{
	dbg_func_in();
	return i2c_del_driver(&yas529_i2c_driver);
	dbg_func_out();
}

module_init(yas529_driver_init);
module_exit(yas529_driver_exit);

/* -------------------------------------------------------------------- */
