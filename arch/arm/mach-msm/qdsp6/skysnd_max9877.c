/************************************************************************************************
**
**    AUDIO EXTERNAL AMP
**
**    FILE
**        SndExtAmp.c
**
**    DESCRIPTION
**        This file contains audio external amp api
**          
**          void snd_extamp_api_Init()
**          void snd_extamp_api_SetPath()
**          void snd_extamp_api_SetVolume()
**          void snd_extamp_api_Sleep()
**
**    Copyright (c) 2009 by PANTECH Incorporated.  All Rights Reserved.
*************************************************************************************************/


/************************************************************************************************
** Includes
*************************************************************************************************/
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include "dal.h"
#include "dal_audio.h"
#include "skysnd_max9877.h"

/************************************************************************************************
** Definition
*************************************************************************************************/
/* Default Register Value */ 
#define DEFAULT_INMODE_CTL_REG_VAL  (ZCD_EN | INA_CFG_MONO | INB_CFG_MONO | PGAINA_0DB | PGAINB_0DB) 
//#define DEFAULT_VOLUME_CTL_REG_VAL  (VOLTBL(0))
#define DEFAULT_HPH_VOLUME_CTL_REG_VAL  0 //24 //-7dB
#define DEFAULT_SPK_VOLUME_CTL_REG_VAL  0 //26 //-5dB
#define DEFAULT_OUTMODE_CTL_REG_VAL (SHDN_SHUTDOWN | BYPASS_EN | OSC_1176_588 | OUTMODE_INA_SPK)

/************************************************************************************************
** Variables
*************************************************************************************************/
static extamp_info_t tExtampInfo;
static unsigned char bHeadphonePath = 0;
static struct i2c_client *max9877_i2c_client = NULL;
static uint32_t CurrDeviceId = 0xffffffff;

/************************************************************************************************
** Declaration
*************************************************************************************************/
static int snd_extamp_i2c_write(u8 reg, u8 data);
/*static int snd_extamp_i2c_read(u8 reg, u8 *data);*/
static int max9877_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int __exit max9877_remove(struct i2c_client *client);
static void max9877_shutdown(struct i2c_client *client);

static void snd_extamp_setpath(extamp_inmode_e inpath, extamp_outmode_e outpath, extamp_outfmt_e outfmt);
static void snd_extamp_setvolume(extamp_outmode_e outpath, uint32_t volume);
static void snd_extamp_setpreampgain(extamp_inmode_e inpath, u8 val);
static void snd_extamp_sleep(u8 enable);

/*==========================================================================
** mt9d112_probe
**=========================================================================*/
static int max9877_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int rc = 0;
    int status = 0;
    /*u8 inregval, outregval, volregval;*/

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        max9877_i2c_client = NULL;
        rc = -1;
    }
    else {
        max9877_i2c_client = client;

        tExtampInfo.inm_ctl_reg_val = DEFAULT_INMODE_CTL_REG_VAL;
        tExtampInfo.spk_vol_reg_val = DEFAULT_SPK_VOLUME_CTL_REG_VAL;//DEFAULT_VOLUME_CTL_REG_VAL;
        tExtampInfo.hpl_vol_reg_val = DEFAULT_HPH_VOLUME_CTL_REG_VAL;//DEFAULT_VOLUME_CTL_REG_VAL;
        tExtampInfo.hpr_vol_reg_val = DEFAULT_HPH_VOLUME_CTL_REG_VAL;//DEFAULT_VOLUME_CTL_REG_VAL;
        tExtampInfo.outm_ctl_reg_val = DEFAULT_OUTMODE_CTL_REG_VAL;

        /* Init MAX9877 register */
        status |= snd_extamp_i2c_write(OUTMODE_CTL_REG_ADDR, tExtampInfo.outm_ctl_reg_val);
        status |= snd_extamp_i2c_write(INMODE_CTL_REG_ADDR, tExtampInfo.inm_ctl_reg_val);
        status |= snd_extamp_i2c_write(SPKVOL_CTL_REG_ADDR, tExtampInfo.spk_vol_reg_val);
        status |= snd_extamp_i2c_write(HPHVOL_L_CTL_REG_ADDR, tExtampInfo.hpl_vol_reg_val);
        status |= snd_extamp_i2c_write(HPHVOL_R_CTL_REG_ADDR, tExtampInfo.hpr_vol_reg_val);     

        /* For Test */
        /*snd_extamp_i2c_read(INMODE_CTL_REG_ADDR, &inregval);
        snd_extamp_i2c_read(OUTMODE_CTL_REG_ADDR, &outregval);
        snd_extamp_i2c_read(SPKVOL_CTL_REG_ADDR, &volregval);*/

        if(status){
            rc = -1;
        }
    }

    return rc;
}

/*==========================================================================
** mt9d112_remove
**=========================================================================*/
static int __exit max9877_remove(struct i2c_client *client)
{
    int rc = 0;

    max9877_i2c_client = NULL;
    /*rc = i2c_detach_client(client);*/

    return rc;
}

static int max9877_suspend(struct i2c_client *client, pm_message_t mesg)
{
    unsigned char SuspendOk = 0;
    
    SuspendOk = (tExtampInfo.spk_vol_reg_val == 0 && tExtampInfo.hpl_vol_reg_val == 0 && tExtampInfo.hpr_vol_reg_val == 0);

    tExtampInfo.outm_ctl_reg_val &= ~SHDN_OPERATION;
    tExtampInfo.outm_ctl_reg_val = SuspendOk ? tExtampInfo.outm_ctl_reg_val | SHDN_SHUTDOWN : tExtampInfo.outm_ctl_reg_val | SHDN_OPERATION;
    
    snd_extamp_i2c_write(OUTMODE_CTL_REG_ADDR, tExtampInfo.outm_ctl_reg_val);
    return 0;
}

static int max9877_resume(struct i2c_client *client)
{
    snd_extamp_sleep(1);
    return 0;
}

static void max9877_shutdown(struct i2c_client *client)
{
    tExtampInfo.outm_ctl_reg_val &= ~SHDN_OPERATION;
    tExtampInfo.outm_ctl_reg_val = tExtampInfo.outm_ctl_reg_val | SHDN_SHUTDOWN;
    
    snd_extamp_i2c_write(OUTMODE_CTL_REG_ADDR, tExtampInfo.outm_ctl_reg_val);
}

static const struct i2c_device_id max9877_id[] = {
    { "max9877-amp", 0},
};

static struct i2c_driver max9877_driver = {
    .id_table = max9877_id,
    .probe  = max9877_probe,
    .remove = __exit_p(max9877_remove),
    .suspend = max9877_suspend,
    .resume = max9877_resume,
    .shutdown = max9877_shutdown,
    .driver = {
        .name = "max9877-amp",
    },
};

/*==========================================================================
** snd_extamp_api_Init
**=========================================================================*/
void snd_extamp_api_Init(void)
{
    int result = 0;

    result = i2c_add_driver(&max9877_driver);
    if(result){
        /* error */
    }
}

/*==========================================================================
** snd_extamp_api_DeInit
**=========================================================================*/
void snd_extamp_api_DeInit(void)
{
    i2c_del_driver(&max9877_driver);
}

/*==========================================================================
** snd_extamp_api_SetDevice
**=========================================================================*/
void snd_extamp_api_SetDevice(uint32_t cad_device)
{
    if(CurrDeviceId == cad_device) return;
    CurrDeviceId = cad_device;

    switch(cad_device){
    case ADSP_AUDIO_DEVICE_ID_HANDSET_SPKR:
        snd_extamp_setpath(EXTAMP_IN_RXIN, EXTAMP_OUT_BYPASS, EXTAMP_MONO);
        break;
    case ADSP_AUDIO_DEVICE_ID_HEADSET_SPKR_MONO:
#if 1 //20100910 jhsong : audio cal
        snd_extamp_setpath(EXTAMP_IN_INA, EXTAMP_OUT_SPK, EXTAMP_STEREO);
#else
        snd_extamp_setpath(EXTAMP_IN_INA, EXTAMP_OUT_SPK, EXTAMP_MONO);
#endif
        break;
    case ADSP_AUDIO_DEVICE_ID_HEADSET_SPKR_STEREO:
        snd_extamp_setpath(EXTAMP_IN_INA, EXTAMP_OUT_HPH_LR, EXTAMP_STEREO);
        break;
    case ADSP_AUDIO_DEVICE_ID_SPKR_PHONE_MONO:
        snd_extamp_setpath(EXTAMP_IN_INB, EXTAMP_OUT_SPK, EXTAMP_MONO);
        break;
    case ADSP_AUDIO_DEVICE_ID_SPKR_PHONE_STEREO:
        snd_extamp_setpath(EXTAMP_IN_INB, EXTAMP_OUT_SPK, EXTAMP_MONO);
        break;
    case ADSP_AUDIO_DEVICE_ID_SPKR_PHONE_MONO_W_MONO_HEADSET:
    case ADSP_AUDIO_DEVICE_ID_SPKR_PHONE_STEREO_W_MONO_HEADSET:
    case ADSP_AUDIO_DEVICE_ID_SPKR_PHONE_MONO_W_STEREO_HEADSET:
    case ADSP_AUDIO_DEVICE_ID_SPKR_PHONE_STEREO_W_STEREO_HEADSET:
        snd_extamp_setpath(EXTAMP_IN_INB, EXTAMP_OUT_SPK_HPH, EXTAMP_MONO);
        break;
    case ADSP_AUDIO_DEVICE_ID_BT_SCO_SPKR:
    case ADSP_AUDIO_DEVICE_ID_BT_A2DP_SPKR:
    case ADSP_AUDIO_DEVICE_ID_TTY_HEADSET_SPKR:
    case ADSP_AUDIO_DEVICE_ID_I2S_SPKR:
        break;
    default:
        break;
    }

    printk(KERN_ERR "(skyaudio) kernel : set sub-amp path(cad dev : 0x%x)\n", cad_device);
}

/*==========================================================================
** snd_extamp_api_SetVolume
**=========================================================================*/
void snd_extamp_api_SetVolume(uint32_t master_volume)
{
    extamp_outmode_e outpath;

    switch(CurrDeviceId){
    case ADSP_AUDIO_DEVICE_ID_HEADSET_SPKR_STEREO:
    //case ADSP_AUDIO_DEVICE_ID_HEADSET_SPKR_MONO:
        outpath = EXTAMP_OUT_HPH_LR;
        break;
    case ADSP_AUDIO_DEVICE_ID_HEADSET_SPKR_MONO:
    case ADSP_AUDIO_DEVICE_ID_SPKR_PHONE_MONO:
    case ADSP_AUDIO_DEVICE_ID_SPKR_PHONE_STEREO:
        outpath = EXTAMP_OUT_SPK;
        break;
    case ADSP_AUDIO_DEVICE_ID_SPKR_PHONE_MONO_W_MONO_HEADSET:
    case ADSP_AUDIO_DEVICE_ID_SPKR_PHONE_STEREO_W_MONO_HEADSET:
    case ADSP_AUDIO_DEVICE_ID_SPKR_PHONE_MONO_W_STEREO_HEADSET:
    case ADSP_AUDIO_DEVICE_ID_SPKR_PHONE_STEREO_W_STEREO_HEADSET:
        outpath = EXTAMP_OUT_SPK_HPH;
        break;
    default:
        outpath = EXTAMP_OUT_BYPASS;
        break;
    }
    snd_extamp_setvolume(outpath, master_volume);
}

/*==========================================================================
** snd_extamp_api_HeadsetConnected
**=========================================================================*/
void snd_extamp_api_HeadsetConnected(uint32_t connect)
{
    bHeadphonePath = connect;
    snd_extamp_sleep(1);
}

/*==========================================================================
** snd_extamp_api_SetPreAmpGain
**=========================================================================*/
void snd_extamp_api_SetPreAmpGain(uint32_t gain)
{
    /**0 -> 0dB, 1-> 9dB, 2->24dB*/
    switch(CurrDeviceId){
    case ADSP_AUDIO_DEVICE_ID_HEADSET_SPKR_MONO:
    case ADSP_AUDIO_DEVICE_ID_HEADSET_SPKR_STEREO:
        snd_extamp_setpreampgain(EXTAMP_IN_INA, gain);
        snd_extamp_setpreampgain(EXTAMP_IN_INB, 0);
        break;
    case ADSP_AUDIO_DEVICE_ID_SPKR_PHONE_MONO:
    case ADSP_AUDIO_DEVICE_ID_SPKR_PHONE_STEREO:
        snd_extamp_setpreampgain(EXTAMP_IN_INB, gain);
        snd_extamp_setpreampgain(EXTAMP_IN_INA, 0);
        break;
    case ADSP_AUDIO_DEVICE_ID_SPKR_PHONE_MONO_W_MONO_HEADSET:
    case ADSP_AUDIO_DEVICE_ID_SPKR_PHONE_STEREO_W_MONO_HEADSET:
    case ADSP_AUDIO_DEVICE_ID_SPKR_PHONE_MONO_W_STEREO_HEADSET:
        snd_extamp_setpreampgain(EXTAMP_IN_INA, gain);
        snd_extamp_setpreampgain(EXTAMP_IN_INB, gain);
        break;
    default:
        snd_extamp_setpreampgain(EXTAMP_IN_INA, 0/*0dB*/);
        snd_extamp_setpreampgain(EXTAMP_IN_INB, 0/*0dB*/);
        break;
    }
}

/*==========================================================================
** snd_extamp_api_SetPath
**=========================================================================*/
static void snd_extamp_setpath(extamp_inmode_e inpath, extamp_outmode_e outpath, extamp_outfmt_e outfmt)
{
    u8 outregval = 0;
    u8 inregval = 0;
/* kernel_warn: u8 volume; */

    switch(outfmt){
    case EXTAMP_MONO:
        inregval = INA_CFG_MONO | INB_CFG_MONO;
        break;
    case EXTAMP_STEREO:
        inregval = INA_CFG_STEREO | INB_CFG_STEREO;
        break;
    default:
        break;
    }

    switch(inpath){
    case EXTAMP_IN_INA:
        switch(outpath){
        case EXTAMP_OUT_SPK:
            outregval = OUTMODE_INA_SPK | BYPASS_DIS;
            break;
        case EXTAMP_OUT_HPH_L:
        case EXTAMP_OUT_HPH_R:
        case EXTAMP_OUT_HPH_LR:
            outregval = OUTMODE_INA_HPH | BYPASS_DIS;
            break;
        case EXTAMP_OUT_SPK_HPH:
            outregval = OUTMODE_INA_SPKHPH | BYPASS_DIS;
            bHeadphonePath = 1;
            break;
        default:
            return;
        }
        break;
    case EXTAMP_IN_INB:
        switch(outpath){
        case EXTAMP_OUT_SPK:
            outregval = OUTMODE_INB_SPK | BYPASS_DIS;
            break;
        case EXTAMP_OUT_HPH_L:
        case EXTAMP_OUT_HPH_R:
        case EXTAMP_OUT_HPH_LR:
            outregval = OUTMODE_INB_HPH | BYPASS_DIS;
            break;
        case EXTAMP_OUT_SPK_HPH:
            outregval = OUTMODE_INB_SPKHPH | BYPASS_DIS;
            break;
        default:
            return;
        }
        break;
    case EXTAMP_IN_INAB:
        switch(outpath){
        case EXTAMP_OUT_SPK:
            outregval = OUTMODE_INAB_SPK | BYPASS_DIS;
            break;
        case EXTAMP_OUT_HPH_L:
        case EXTAMP_OUT_HPH_R:
        case EXTAMP_OUT_HPH_LR:
            outregval = OUTMODE_INAB_HPH | BYPASS_DIS;
            break;
        case EXTAMP_OUT_SPK_HPH:
            outregval = OUTMODE_INAB_SPKHPH | BYPASS_DIS;
            break;
        default:
            return;
            }
        break;
    case EXTAMP_IN_RXIN:
        outregval = BYPASS_EN;
        inregval = tExtampInfo.inm_ctl_reg_val & 0x30;
        /*outpath = EXTAMP_OUT_BYPASS;*/
        break;
    default:
        return;
    }

    /*volume = (bHeadphonePath) ? 0 : tExtampInfo.spk_vol_reg_val;
    volume = (inpath == EXTAMP_IN_RXIN) ? 0 : volume;
    snd_extamp_api_SetVolume(outpath, volume);*/

    tExtampInfo.outm_ctl_reg_val  = (tExtampInfo.outm_ctl_reg_val & 0xB0) | outregval;
    tExtampInfo.inm_ctl_reg_val = (tExtampInfo.inm_ctl_reg_val & 0xCF) | inregval;
    snd_extamp_i2c_write(OUTMODE_CTL_REG_ADDR, tExtampInfo.outm_ctl_reg_val);
    snd_extamp_i2c_write(INMODE_CTL_REG_ADDR, tExtampInfo.inm_ctl_reg_val);
}


/*==========================================================================
** snd_extamp_setvolume
**=========================================================================*/
static void snd_extamp_setvolume(extamp_outmode_e outpath, uint32_t volume)
{
    unsigned int spkvol = 0;
    unsigned int hphvol = 0;
    const u8 VOL_STEP = 10;
    const u8 VOL_STEP_DN = 10;
    const u8 DEALY = 5;
    const u8 DEALY_DN = 15;
    
    switch(outpath){
    case EXTAMP_OUT_SPK:
        spkvol = (volume&0x001f0000)>>16;
        break;
    case EXTAMP_OUT_HPH_L:
    case EXTAMP_OUT_HPH_R:
    case EXTAMP_OUT_HPH_LR:
        hphvol = (volume&0x0000001f);
        break;
    case EXTAMP_OUT_SPK_HPH:
        spkvol = (volume&0x001f0000)>>16;
        hphvol = (volume&0x0000001f);
        break;
    case EXTAMP_OUT_BYPASS:
        spkvol = 0;
        hphvol = 0;
        break;
    default:
        return;
    }
    
    /* Wakeup AMP */
    if(spkvol > 0 || hphvol > 0){
        snd_extamp_sleep(0);
    }

    /*if (tExtampInfo.spk_vol_reg_val != (u8)spkvol) {
        tExtampInfo.spk_vol_reg_val = (u8)spkvol;
        snd_extamp_i2c_write(SPKVOL_CTL_REG_ADDR, tExtampInfo.spk_vol_reg_val);
        }
    if (tExtampInfo.hpl_vol_reg_val != (u8)hphvol) {
        tExtampInfo.hpl_vol_reg_val = tExtampInfo.hpr_vol_reg_val = (u8)hphvol;
        snd_extamp_i2c_write(HPHVOL_L_CTL_REG_ADDR, tExtampInfo.hpl_vol_reg_val);
        snd_extamp_i2c_write(HPHVOL_R_CTL_REG_ADDR, tExtampInfo.hpr_vol_reg_val);
    }
    msleep(20);*/
    
    if(spkvol > tExtampInfo.spk_vol_reg_val){
        while(tExtampInfo.spk_vol_reg_val < spkvol){
            if(tExtampInfo.spk_vol_reg_val + VOL_STEP > spkvol)
                tExtampInfo.spk_vol_reg_val = (u8)spkvol;
            else
                tExtampInfo.spk_vol_reg_val += VOL_STEP;

            snd_extamp_i2c_write(SPKVOL_CTL_REG_ADDR, tExtampInfo.spk_vol_reg_val);
            msleep(DEALY);
        }
    }
    else if(spkvol < tExtampInfo.spk_vol_reg_val){
        while(tExtampInfo.spk_vol_reg_val > spkvol){
            if(tExtampInfo.spk_vol_reg_val < spkvol + VOL_STEP_DN)
                tExtampInfo.spk_vol_reg_val = (u8)spkvol;
            else
                tExtampInfo.spk_vol_reg_val -= VOL_STEP_DN;

            snd_extamp_i2c_write(SPKVOL_CTL_REG_ADDR, tExtampInfo.spk_vol_reg_val);
            msleep(DEALY_DN);
        }
    }

    if(hphvol > tExtampInfo.hpl_vol_reg_val){
        while(tExtampInfo.hpl_vol_reg_val < hphvol){
            if(tExtampInfo.hpl_vol_reg_val + VOL_STEP > hphvol)
                tExtampInfo.hpl_vol_reg_val = (u8)hphvol;
            else
                tExtampInfo.hpl_vol_reg_val += VOL_STEP;

            snd_extamp_i2c_write(HPHVOL_L_CTL_REG_ADDR, tExtampInfo.hpl_vol_reg_val);
            snd_extamp_i2c_write(HPHVOL_R_CTL_REG_ADDR, tExtampInfo.hpl_vol_reg_val);
            msleep(DEALY);
        }
    }
    else if(hphvol < tExtampInfo.hpl_vol_reg_val){
        while(tExtampInfo.hpl_vol_reg_val > hphvol){
            if(tExtampInfo.hpl_vol_reg_val < hphvol + VOL_STEP_DN)
                tExtampInfo.hpl_vol_reg_val = (u8)hphvol;
            else
                tExtampInfo.hpl_vol_reg_val -= VOL_STEP_DN;

            snd_extamp_i2c_write(HPHVOL_L_CTL_REG_ADDR, tExtampInfo.hpl_vol_reg_val);
            snd_extamp_i2c_write(HPHVOL_R_CTL_REG_ADDR, tExtampInfo.hpl_vol_reg_val);
            msleep(DEALY_DN);
        }
    }
    tExtampInfo.hpr_vol_reg_val = tExtampInfo.hpl_vol_reg_val;    
    printk(KERN_ERR "(skyaudio) kernel : set sub-amp volume(spk:%d, hph:%d)\n", spkvol, hphvol);
    
    /* Sleep AMP */
    if(spkvol == 0 && hphvol == 0){ 
        snd_extamp_sleep(1);
    }
    return;
}

/*==========================================================================
** snd_extamp_setpreampgain
**=========================================================================*/
static void snd_extamp_setpreampgain(extamp_inmode_e inpath, u8 val)
{
    int status = 0;
    
    val = (u8)(val & 0x03);
    
    switch(inpath)      
    {
    case EXTAMP_IN_INA:
        tExtampInfo.inm_ctl_reg_val = (tExtampInfo.inm_ctl_reg_val & 0xF3) | (val << 2);
        break;
    case EXTAMP_IN_INB:
        tExtampInfo.inm_ctl_reg_val = (tExtampInfo.inm_ctl_reg_val & 0xFC) | (val);
        break;
    case EXTAMP_IN_INAB:
        tExtampInfo.inm_ctl_reg_val = (tExtampInfo.inm_ctl_reg_val & 0xF0) | (val << 2) | (val);
        break;
    case EXTAMP_IN_RXIN:
    default:
        return;
    }
    
    status = snd_extamp_i2c_write(INMODE_CTL_REG_ADDR, tExtampInfo.inm_ctl_reg_val);
    
    if(status)
    {
        //error
    }
}

/*==========================================================================
** snd_extamp_sleep
**=========================================================================*/
static void snd_extamp_sleep(u8 enable)
{
    unsigned char SleepOk = 0;
    
    SleepOk = (enable && !bHeadphonePath && tExtampInfo.spk_vol_reg_val == 0 && tExtampInfo.hpl_vol_reg_val == 0 && tExtampInfo.hpr_vol_reg_val == 0);

    tExtampInfo.outm_ctl_reg_val &= ~SHDN_OPERATION;
    tExtampInfo.outm_ctl_reg_val = SleepOk ? tExtampInfo.outm_ctl_reg_val | SHDN_SHUTDOWN : tExtampInfo.outm_ctl_reg_val | SHDN_OPERATION;
    
    snd_extamp_i2c_write(OUTMODE_CTL_REG_ADDR, tExtampInfo.outm_ctl_reg_val);
    //printk(KERN_ERR "(skyaudio) kernel : set sub-amp sleep(sleepok: %d)\n", SleepOk);
}

/*==========================================================================
** snd_extamp_i2c_write
**=========================================================================*/
static int snd_extamp_i2c_write(u8 reg, u8 data)
{
    static int ret = 0;
    unsigned char buf[2];
    struct i2c_msg msg[1];

    if(!max9877_i2c_client){
        return -1;
    }

    buf[0] = (unsigned char)reg;
    buf[1] = (unsigned char)data;

    msg[0].addr = max9877_i2c_client->addr;
    msg[0].flags = 0;
    msg[0].len = 2;
    msg[0].buf = buf;
    
    ret = i2c_transfer(max9877_i2c_client->adapter, msg, 1);
    if (ret < 0) {
        return -1;
    }

    return 0;
}

/*==========================================================================
** snd_extamp_i2c_read
**=========================================================================*/
/*static int snd_extamp_i2c_read(u8 reg, u8 *data)
{
    static int ret = 0;
    unsigned char buf[1];
    struct i2c_msg msgs[2];

    if(!max9877_i2c_client){
        return -1;
    }

    buf[0] = reg;

    msgs[0].addr = max9877_i2c_client->addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = buf;

    msgs[1].addr = max9877_i2c_client->addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = 1;
    msgs[1].buf = buf;

    ret = i2c_transfer(max9877_i2c_client->adapter, msgs, 2);
    if ( ret < 0) {
        return -1;
    }

    *data = (u8)buf[0];
    return 0;
}*/
