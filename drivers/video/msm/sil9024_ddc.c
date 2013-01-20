//=============================================================================
// File       : sil9024_i2c.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2009/08/14      leecy          Draft
//=============================================================================

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include "sil9024_type.h"
#include "sil9024_i2c.h"
#include "sil9024_ddc.h"


#ifndef MSM_HDMI_DBG_MSG
#define MSM_HDMI_DBG_MSG
#endif

#ifdef MSM_HDMI_DBG_MSG
#define HDMI_MSG_DDC(fmt, args...) printk(KERN_INFO "HDMI_DDC: " fmt, ##args)
#else
#define HDMI_MSG_DDC(fmt, args...)
#endif

#define HDMI_DDC_DEV_NAME    "sky_hdmi_ddc"

static struct i2c_client *sil9024a_ddc_client = NULL;

static int msm_hdmi_ddc_probe(struct i2c_client *client, const struct i2c_device_id *id);


static int msm_hdmi_ddc_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
  int rc = 0;

  if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
  {
    sil9024a_ddc_client = NULL;
    rc = -1;
    HDMI_MSG_DDC(" i2c_probe failed!!!\n");
  }
  else {
  sil9024a_ddc_client = client;
  }

  HDMI_MSG_DDC("ID(%x) i2c_probe successed!!!\n", ((client->addr) << 1));

  return rc;
}

static int msm_hdmi_ddc_remove(struct i2c_client *client)
{
  int rc;

  sil9024a_ddc_client = NULL;
#if 1
  rc = i2c_detach_client(client);

  return rc;
#else
  HDMI_MSG_DDC("i2c_probe removed!!!\n");
  return 0;
#endif
}

static const struct i2c_device_id msm_hdmi_ddc_id[] = {
  {HDMI_DDC_DEV_NAME, 0},
};

static struct i2c_driver msm_hdmi_ddc_driver = {
  .id_table = msm_hdmi_ddc_id,
  .probe  = msm_hdmi_ddc_probe,
  .remove = __exit_p(msm_hdmi_ddc_remove),
  .driver = {
    .name = HDMI_DDC_DEV_NAME,
  },
};

#if 1 //차후 삭제..
void sky_hdmi_ddc_api_init()
{
    int result;
    
    result = i2c_add_driver(&msm_hdmi_ddc_driver);
    if(result){
      HDMI_MSG_DDC("Init error !!\n");
    }
}

void sky_hdmi_ddc_api_deinit()
{
#if 1
    i2c_del_driver(&msm_hdmi_ddc_driver);
#endif
}
#endif

/* module init function. register spi driver data */
static int __init sky_hdmi_ddc_init (void)
{
    int result;
    
    result = i2c_add_driver(&msm_hdmi_ddc_driver);
    if(result){
      HDMI_MSG_DDC("Init error !!\n");
    }
		return result;
}

/* module exit function */
static void __exit sky_hdmi_ddc_exit (void)
{
    i2c_del_driver(&msm_hdmi_ddc_driver);
}

module_init (sky_hdmi_ddc_init); 


///////////////////[[
static int sil9024a_ddc_rxdata(unsigned short saddr,
	unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
	{
		.addr   = saddr,
		.flags = 0,
		.len   = 1,
		.buf   = rxdata,
	},
	{
		.addr  = saddr,
		.flags = I2C_M_RD,
		.len   = length,
		.buf   = rxdata,
	},
	};

	if (i2c_transfer(sil9024a_ddc_client->adapter, msgs, 2) < 0) {
		HDMI_MSG_DDC("ID(%x)ddc_rxdata failed!\n", sil9024a_ddc_client->addr);
		return -EIO;
	}

	return 0;
}

int32 sil9024a_ddc_read(unsigned short raddr, unsigned char *rdata)
{
	int32 rc = 0;
	unsigned char buf[128];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));
	buf[0] = (raddr & 0xFF);

	rc = sil9024a_ddc_rxdata(sil9024a_ddc_client->addr, buf, 1);
	if (rc < 0){
		HDMI_MSG_DDC("sil9024a_ddc_read_w failed!\n");
	}
  
	*rdata = buf[0];

	return rc;
}

int32 sil9024a_ddc_read_len(word raddr, byte *rdata, word length)
{
	int32 rc = 0;
	unsigned char buf[256];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));
	rdata[0] = buf[0] = (raddr & 0xFF);

	rc = sil9024a_ddc_rxdata(sil9024a_ddc_client->addr, rdata/*buf*/, length);
	if (rc < 0){
		HDMI_MSG_DDC("sil9024a_ddc_read_len failed!\n");
	}
    else{
      HDMI_MSG_DDC("sil9024a_ddc_read_len (rAddr %x, %x) success!!!\n", raddr, buf[0]);
    }
  
	//*rdata = buf[0];

	return rc;
}

static int32 sil9024a_ddc_txdata(unsigned short saddr,
	unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
	{
		.addr = saddr,
		.flags = 0,
		.len = length,
		.buf = txdata,
	},
	};

	if (i2c_transfer(sil9024a_ddc_client->adapter, msg, 1) < 0) {
		HDMI_MSG_DDC("sil9024a_i2c_txdata faild\n");
		return -EIO;
	}

	return 0;
}

int32 sil9024a_ddc_write(unsigned short raddr, unsigned short wdata)
{
	int32 rc = -EFAULT;
	unsigned char buf[2];

	memset(buf, 0, sizeof(buf));
  
	buf[0] = raddr;
	buf[1] = wdata;
	rc = sil9024a_ddc_txdata(sil9024a_ddc_client->addr, buf, 2);

	if (rc < 0){
		HDMI_MSG_DDC("sil9024a_ddc_write failed, Daddr=0x%x, raddr = 0x%x, val = 0x%x!\n", 
		sil9024a_ddc_client->addr, raddr, wdata);
	}
    else{
        HDMI_MSG_DDC("sil9024a_ddc_write (rAddr %x, %x) success!!!\n", raddr, wdata);
    }

	return rc;
}
