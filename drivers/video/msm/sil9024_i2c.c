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
#define HDMI_MSG_I2C(fmt, args...) printk("HDMI_I2C: " fmt, ##args)
#else
#define HDMI_MSG_I2C(fmt, args...)
#endif

#define HDMI_I2C_DEV_NAME    "sky_hdmi_i2c"

static struct i2c_client *sil9024a_client = NULL;

static int msm_hdmi_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);

static int msm_hdmi_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
  int rc = 0;

    printk("%s :: start ", __func__);


  if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
  {
    sil9024a_client = NULL;
    rc = -1;
    HDMI_MSG_I2C(" i2c_probe failed!!!\n");
  }
  else {
  sil9024a_client = client;
  }

  HDMI_MSG_I2C("ID(%x) i2c_probe successed!!!\n", ((client->addr) << 1));

  return rc;
}


static int msm_hdmi_i2c_remove(struct i2c_client *client)
{
  int rc;

  sil9024a_client = NULL;
#if 1
  rc = i2c_detach_client(client);

  return rc;
#else
  HDMI_MSG_I2C("i2c_probe removed!!!\n");
  return 0;
#endif
}


static const struct i2c_device_id msm_hdmi_i2c_id[] = {
  {HDMI_I2C_DEV_NAME, 0},
};

static struct i2c_driver msm_hdmi_i2c_driver = {
  .id_table = msm_hdmi_i2c_id,
  .probe  = msm_hdmi_i2c_probe,
  .remove = msm_hdmi_i2c_remove,
  .driver = {
    .name = HDMI_I2C_DEV_NAME,
  },
};

#if 1 //차후 삭제...
void sky_hdmi_i2c_drv_register(void)
{
  int result = 0;

  result = i2c_add_driver(&msm_hdmi_i2c_driver);
  if(result){
    HDMI_MSG_I2C("Init error !!\n");
  }
}

void msm_hdmi_i2c_api_DeInit(void)
{
  i2c_del_driver(&msm_hdmi_i2c_driver);
}
#endif

/* module init function. register spi driver data */
static int __init sky_hdmi_i2c_init (void)
{
    int result = 0;

    printk("%s :: start ", __func__);

    result = i2c_add_driver(&msm_hdmi_i2c_driver);
    if(result){
        HDMI_MSG_I2C("Init error !!\n");
    }
		return result;
}

/* module exit function */
static void __exit sky_hdmi_i2c_exit (void)
{
      i2c_del_driver(&msm_hdmi_i2c_driver);
}

module_init (sky_hdmi_i2c_init); 


///////////////////[[
static int sil9024a_i2c_rxdata(unsigned short saddr, byte *raddr, unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
	{
		.addr   = saddr,
		.flags = 0,
		.len   = 1,
		.buf   = raddr,
	},
	{
		.addr  = saddr,
		.flags = I2C_M_RD,
		.len   = length,
		.buf   = rxdata,
	},
	};

	if (i2c_transfer(sil9024a_client->adapter, msgs, 2) < 0) {
		HDMI_MSG_I2C("ID(0x%2.2x)i2c_rxdata failed!\n", sil9024a_client->addr);
		return -EIO;
	}

	return 0;
}

static int sil9024a_i2c_seg_rxdata(unsigned short saddr, byte *segment, byte *offset, unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
	{
		.addr   = 0x60>>1,
		.flags = 0,
		.len   = 1,
		.buf   = segment,
	},
	{
		.addr   = saddr,
		.flags = 0,
		.len   = 1,
		.buf   = offset,
	},
	{
		.addr  = saddr,
		.flags = I2C_M_RD,
		.len   = length,
		.buf   = rxdata,
	},
	};

	if (i2c_transfer(sil9024a_client->adapter, msgs, 3) < 0) {
		HDMI_MSG_I2C("ID(%d)sil9024a_i2c_seg_rxdata failed!\n", sil9024a_client->addr);
		return -EIO;
	}

	return 0;
}


int32 sil9024a_i2c_read(unsigned short raddr, unsigned char *rdata)
{
	int32 rc = 0;
	unsigned char buf[2];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));
	buf[0] = (raddr & 0xFF);

	rc = sil9024a_i2c_rxdata(sil9024a_client->addr, buf, rdata, 1);
	if (rc < 0){
		HDMI_MSG_I2C("sil9024a_i2c_read_w failed!\n");
	}
	return rc;
}

int32 sil9024a_i2c_read_len(byte deviceID, word raddr, byte *rdata, word length)
{
    int32 rc = 0;
    unsigned char buf[2];

    if (!rdata)
        return -EIO;
    
    sil9024a_client->addr = deviceID >> 1;
    memset(buf, 0, sizeof(buf));
    buf[0] = (raddr & 0xFF);

    rc = sil9024a_i2c_rxdata(sil9024a_client->addr, buf, rdata/*buf*/, length);
    if (rc < 0){
        HDMI_MSG_I2C("sil9024a_i2c_read_len failed! ID(0x%x), addr 0x%2.2x length %d\n", sil9024a_client->addr, raddr, length);
    }
    else{
        HDMI_MSG_I2C("sil9024a_i2c_read_len (ID 0x%x, regAddr 0x%2.2x, len %x, data 0x%x) success!\n", sil9024a_client->addr, raddr, length, rdata[0]);
    }
    return rc;
}


int32 sil9024a_i2c_read_segment(byte deviceID, byte segment, byte raddr, byte *rdata, word length)
{
    int32 rc = 0;
    unsigned char buf[2];
    unsigned char dummy_buf;

    if (!rdata)
        return -EIO;

    sil9024a_client->addr = deviceID >> 1;

    memset(buf, 0, sizeof(buf));
    buf[0] = (raddr & 0xFF);
    dummy_buf = segment;

    rc = sil9024a_i2c_seg_rxdata(sil9024a_client->addr, &dummy_buf, buf, rdata, length);
    if (rc < 0){
        HDMI_MSG_I2C("sil9024a_i2c_read_len failed! ID(0x%x), addr 0x%x length %d\n", sil9024a_client->addr, raddr, length);
    }
    else{
        HDMI_MSG_I2C("sil9024a_i2c_read_len (ID 0x%x, regAddr %x, len %d, data 0x%x) success!\n", sil9024a_client->addr, raddr, length, rdata[0]);
    }
    return rc;
}



static int32 sil9024a_i2c_txdata(unsigned short saddr, unsigned char *txdata, int length)
{
    int ret_val;

    struct i2c_msg msg[] = {
        {
        .addr = saddr,
        .flags = 0,
        .len = length,
        .buf = txdata,
        },
    };


    ret_val = i2c_transfer(sil9024a_client->adapter, msg, 1);
    if (ret_val < 0) {
        HDMI_MSG_I2C("sil9024a_i2c_txdata faild error code %d\n", ret_val);
        return -EIO;
    }

    return 0;
}

int32 sil9024a_i2c_write(byte deviceID, byte raddr, byte wdata)
{
    int32 rc = -EFAULT;
    unsigned char buf[2];

    sil9024a_client->addr = deviceID >> 1;

    memset(buf, 0, sizeof(buf));

    buf[0] = raddr;
    buf[1] = wdata;
    rc = sil9024a_i2c_txdata(sil9024a_client->addr, buf, 2);

    if (rc < 0){
        HDMI_MSG_I2C("sil9024a_i2c_write_b failed, Daddr=0x%x, raddr = 0x%2.2x, val = 0x%2.2x!\n", 
        sil9024a_client->addr, raddr, wdata);
    }
    else{
        HDMI_MSG_I2C("sil9024a_i2c_write (ID 0x%x, regAddr 0x%2.2x, 0x%2.2x) success!\n", sil9024a_client->addr, raddr, wdata);
    }
    return rc;
}

int32 sil9024a_i2c_writeAddr(byte deviceID, byte raddr, byte dummy_data)
{
    int32 rc = -EFAULT;
    byte buf[2];

    sil9024a_client->addr = deviceID >> 1;

    memset(buf, 0, sizeof(buf));

    buf[0] = raddr;
    buf[1] = dummy_data;    
    rc = sil9024a_i2c_txdata(sil9024a_client->addr, buf, 2);

    if (rc < 0){
        HDMI_MSG_I2C("sil9024a_i2c_write_b failed, Daddr=0x%x, raddr = 0x%x!\n", 
        sil9024a_client->addr, raddr);
    }
    else{
        HDMI_MSG_I2C("sil9024a_i2c_write (ID 0x%x, regAddr 0x%x) success!\n", sil9024a_client->addr, raddr);
    }
    return rc;
}


#if 0
int32 sil9022a_i2c_write(byte deviceID, unsigned short saddr, byte *data, unsigned short len)
{
    unsigned int i = 0;
    int rc = 0;

    struct i2c_msg msg[] = {
        {
        .addr = saddr,
        .flags = 0,
        .len = len,
        .buf = data,
        },
    };

    sil9024a_client->addr = deviceID >> 1;

    rc = i2c_transfer(mv9337_client->adapter, msg, 1);
    if (rc < 0) {
	HDMI_MSG_I2C("sil9024a_i2c_write_b failed, Daddr=0x%x, raddr = 0x%x, val = 0x%x!\n", 
	sil9024a_client->addr, saddr, *data);
    }

    return 0;    
}
#endif

/////////////]]

static int32 sil9024a_i2c_write_w(unsigned short saddr,
	unsigned short waddr, unsigned short wdata)
{
	int32 rc = -EFAULT;
	unsigned char buf[4];

	memset(buf, 0, sizeof(buf));
	buf[0] = (waddr & 0xFF00)>>8;
	buf[1] = (waddr & 0x00FF);
	buf[2] = (wdata & 0xFF00)>>8;
	buf[3] = (wdata & 0x00FF);

	rc = sil9024a_i2c_txdata(saddr, buf, 4);

	if (rc < 0)
		HDMI_MSG_I2C("i2c_write_w failed, addr = 0x%x, val = 0x%x!\n",
		waddr, wdata);

	return rc;
}


unsigned char msm_hdmi_i2c_write_word(unsigned short reg, unsigned short data)
{
  static int ret = 0;
  unsigned char buf[4];
  struct i2c_msg msg[2];
#ifdef FEATURE_I2C_WRITE_CHECK
  uint16 rData;
#endif /* FEATURE_I2C_WRITE_CHECK */

  if(!sil9024a_client)
  {
    return -1;
  }

  buf[0] = (reg & 0xFF00) >> 8;
  buf[1] = (reg & 0x00FF);
  buf[2] = (data & 0xFF00) >> 8;
  buf[3] = (data & 0x00FF);

  msg[0].addr = sil9024a_client->addr;
  msg[0].flags = 0;
  msg[0].len = 4;
  msg[0].buf = buf;

#ifdef FEATURE_I2C_DBG_MSG
    HDMI_MSG_I2C("[msm_hdmi_i2c_write_word] ID[0x%02x] reg[0x%04x] data[0x%04x]\n", sil9024a_client->addr, reg, data);
#endif /* FEATURE_I2C_DBG_MSG */

  ret = i2c_transfer(sil9024a_client->adapter, msg, 1);
  if (ret < 0)
  {
    HDMI_MSG_I2C("[msm_hdmi_i2c_write_word] write error!!!\n");
    return FALSE;
  }
  else
  {
#ifdef FEATURE_I2C_DBG_MSG
    HDMI_MSG_I2C("[msm_hdmi_i2c_write_word] write OK!!!\n");
#endif /* FEATURE_I2C_DBG_MSG */

#ifdef FEATURE_I2C_WRITE_CHECK
    rData = msm_hdmi_i2c_read_word(reg);
    if (rData != data)
    {
      HDMI_MSG_I2C("[msm_hdmi_i2c_write_word:error] reg[0x%04x], data[0x%04x]\n", reg, rData);
    }
#endif /* FEATURE_I2C_WRITE_CHECK */

    return TRUE;
  }
}

unsigned short msm_hdmi_i2c_read_word(unsigned short reg)
{
  static int ret = 0;
  unsigned char wbuf[2];
  unsigned char rbuf[2] = {0,0};
  struct i2c_msg msgs[2];
  unsigned short uiData;

  if(!sil9024a_client)
  {
    return -1;
  }

  wbuf[0] = (reg & 0xFF00) >> 8;
  wbuf[1] = (reg & 0x00FF);

  msgs[0].addr = sil9024a_client->addr;
  msgs[0].flags = 0;
  msgs[0].len = 2;
  msgs[0].buf = wbuf;

  msgs[1].addr = sil9024a_client->addr;
  msgs[1].flags = I2C_M_RD;
  msgs[1].len = 2;
  msgs[1].buf = rbuf;

#ifdef FEATURE_I2C_DBG_MSG
  HDMI_MSG_I2C("[msm_hdmi_i2c_read_word] ID[0x%02x] reg[0x%04x]\n", sil9024a_client->addr, reg);
#endif

#if 1 // android 3145 버전에서 msgs 2개 한번에 보내면 에러리턴됨
  ret = i2c_transfer(sil9024a_client->adapter, msgs, 1);
  if (ret < 0)
  {
    HDMI_MSG_I2C("[msm_hdmi_i2c_read_word] write error!!!\n");
    return FALSE;
  }
  ret = i2c_transfer(sil9024a_client->adapter, &msgs[1], 1);
#else
  ret = i2c_transfer(sil9024a_client->adapter, msgs, 2);
#endif
  if (ret < 0)
  {
    HDMI_MSG_I2C("[msm_hdmi_i2c_read_word] read error!!!\n");
    return FALSE;
  }
  else
  {
    uiData = (rbuf[0] << 8) | rbuf[1];
#ifdef FEATURE_I2C_DBG_MSG
    HDMI_MSG_I2C("[msm_hdmi_i2c_read_word] read OK!!!\n");
    HDMI_MSG_I2C("[msm_hdmi_i2c_read_word] reg[0x%04x], data[0x%04x]\n", reg, uiData);
#endif
    return uiData;
  }
}

void msm_hdmi_i2c_read_len(unsigned int uiAddr, unsigned char ucRecvBuf[], unsigned short ucCount)
{
  static int ret = 0;
  unsigned char buf[2];
  struct i2c_msg msgs[2];
#ifdef FEATURE_I2C_DBG_MSG
  uint16 uiData;
#endif /* FEATURE_I2C_DBG_MSG */


  if(!sil9024a_client)
  {
    return;
  }

  buf[0] = (uiAddr & 0xFF00) >> 8;
  buf[1] = (uiAddr & 0x00FF);

  msgs[0].addr = sil9024a_client->addr;
  msgs[0].flags = 0;
  msgs[0].len = 2;
  msgs[0].buf = buf;

  msgs[1].addr = sil9024a_client->addr;
  msgs[1].flags = I2C_M_RD;
  msgs[1].len = ucCount;
  msgs[1].buf = ucRecvBuf;

#ifdef FEATURE_I2C_DBG_MSG
    HDMI_MSG_I2C("[msm_hdmi_i2c_read_len] ID[0x%02x] reg[0x%04x] cnt[%d]\n", sil9024a_client->addr, uiAddr, ucCount);
#endif

#if 1
  ret = i2c_transfer(sil9024a_client->adapter, msgs, 1);
  if ( ret < 0)
  {
    HDMI_MSG_I2C("[msm_hdmi_i2c_read_len] read error!!!\n");
  }
  ret = i2c_transfer(sil9024a_client->adapter, &msgs[1], 1);
#else
  ret = i2c_transfer(sil9024a_client->adapter, msgs, 2);
#endif
  if ( ret < 0)
  {
    HDMI_MSG_I2C("[msm_hdmi_i2c_read_len] read error!!!\n");
  }
  else
  {
#ifdef FEATURE_I2C_DBG_MSG
    uiData = (ucRecvBuf[0] << 8) | ucRecvBuf[1];
    HDMI_MSG_I2C("[msm_hdmi_i2c_read_len] read OK!!!\n");
    HDMI_MSG_I2C("[msm_hdmi_i2c_read_len] reg[0x%04x], data[0x%04x]\n", uiAddr, uiData);
#endif
  }
  return;
}

