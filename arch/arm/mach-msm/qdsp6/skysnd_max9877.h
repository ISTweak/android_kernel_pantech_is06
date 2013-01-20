#ifndef SNDEXTAMP_H
#define SNDEXTAMP_H
/************************************************************************************************
**
**    AUDIO EXTERNAL AMP
**
**    FILE
**        SndExtAmp.h
**
**    DESCRIPTION
**        This file contains audio external amp api
**
**    Copyright (c) 2009 by PANTECH Incorporated.  All Rights Reserved.
*************************************************************************************************/


/************************************************************************************************
** Includes
*************************************************************************************************/
#include <linux/kernel.h>
/************************************************************************************************
** Definition
*************************************************************************************************/
#define MAX9877_SLAVE_ADDR	0x4D	//b'1001101
//#define MAX9877_SLAVE_ADDR_W    0x9A	//write 10011010 
//#define MAX9877_SLAVE_ADDR_R    0x9B	//read 10011011

#define INMODE_CTL_REG_ADDR     0x00
#define SPKVOL_CTL_REG_ADDR     0x01	
#define HPHVOL_L_CTL_REG_ADDR   0x02	
#define HPHVOL_R_CTL_REG_ADDR   0x03
#define OUTMODE_CTL_REG_ADDR    0x04

/* Input Mode Control Register */
#define ZCD_EN  0x40
#define ZCD_DIS 0x00
#define INA_CFG_MONO    0x20
#define INA_CFG_STEREO  0x00
#define INB_CFG_MONO    0x10
#define INB_CFG_STEREO  0x00
#define PGAINA_0DB  0x00
#define PGAINA_9DB  0x04
#define PGAINA_20DB 0x08
#define PGAINB_0DB  0x00
#define PGAINB_9DB  0x01
#define PGAINB_20DB 0x02

/* Volume Control Registers */
#define OUTVOL_M	    0x1F
#define OUTVOL_MAX  OUTVOL_M
#define OUTVOL_MIN  0x00
#define VOLTBL(x) ((OUTVOL_MAX+x)&OUTVOL_M)

/* Ouput Mode Control Register */
#define SHDN_OPERATION  0x80
#define SHDN_SHUTDOWN   0x00
#define BYPASS_EN   0x40
#define BYPASS_DIS  0x00
#define OSC_1176_588    0x00
#define OSC_1100_550    0x10
#define OSC_700_350     0x20
#define OUTMODE_INA_SPK 0x01
#define OUTMODE_INA_HPH 0x02
#define OUTMODE_INA_SPKHPH    0x03
#define OUTMODE_INB_SPK 0x04
#define OUTMODE_INB_HPH 0x05
#define OUTMODE_INB_SPKHPH    0x06
#define OUTMODE_INAB_SPK 0x07
#define OUTMODE_INAB_HPH 0x08
#define OUTMODE_INAB_SPKHPH    0x09


typedef enum 
{
  EXTAMP_OUT_SPK,
  EXTAMP_OUT_HPH_L,
  EXTAMP_OUT_HPH_R,
  EXTAMP_OUT_HPH_LR,
  EXTAMP_OUT_SPK_HPH,
  EXTAMP_OUT_BYPASS
}extamp_outmode_e;

typedef enum
{
  EXTAMP_IN_INA,
  EXTAMP_IN_INB,
  EXTAMP_IN_INAB,
  EXTAMP_IN_RXIN
}extamp_inmode_e;

typedef enum
{
	EXTAMP_MONO,
	EXTAMP_STEREO
}extamp_outfmt_e;

typedef struct
{
  u8	inm_ctl_reg_val;
  u8	spk_vol_reg_val;
  u8	hpl_vol_reg_val;
  u8	hpr_vol_reg_val;
  u8	outm_ctl_reg_val;
}extamp_info_t;

/************************************************************************************************
** Variables
*************************************************************************************************/

/************************************************************************************************
** Declaration
*************************************************************************************************/
extern void snd_extamp_api_Init(void);
extern void snd_extamp_api_DeInit(void);
extern void snd_extamp_api_SetDevice(uint32_t cad_device);
extern void snd_extamp_api_SetVolume(uint32_t master_volume);
extern void snd_extamp_api_SetPreAmpGain(uint32_t gain);
extern void snd_extamp_api_HeadsetConnected(uint32_t connect);
#endif /*SNDEXTAMP_H*/

