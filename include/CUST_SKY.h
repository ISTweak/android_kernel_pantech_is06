#ifndef _CUST_SKY_FILE_
#define _CUST_SKY_FILE_

/* MODEL_ID  */
#define MODEL_EF10	0x3001

#define MODEL_EF12	0x3002

/*20100717 jjangu_device*/
#define MODEL_JMASAI  0x3003

/* BOARD_VER */
#define EF10_EV	         (MODEL_EF10<<8)+0x00
#define EF10_PT10        (MODEL_EF10<<8)+0x01
#define EF10_WS10        (MODEL_EF10<<8)+0x02
#define EF10_WS20        (MODEL_EF10<<8)+0x03
#define EF10_ES10        (MODEL_EF10<<8)+0x04
#define EF10_ES20        (MODEL_EF10<<8)+0x05

#define EF10_TP10        (MODEL_EF10<<8)+0x10
#define EF10_TP20        (MODEL_EF10<<8)+0x11


#define EF12_WS10           (MODEL_EF12<<8)+0x00
#define EF12_WS20           (MODEL_EF12<<8)+0x01

#define EF12_TP10           (MODEL_EF12<<8)+0x02
#define EF12_TP20           (MODEL_EF12<<8)+0x03

/*20100717 jjangu_device*/
#define JMASAI_PT10           (MODEL_JMASAI<<8)+0x00
#define JMASAI_PT20           (MODEL_JMASAI<<8)+0x01

#define JMASAI_WS10          (MODEL_JMASAI<<8)+0x02
#define JMASAI_WS20          (MODEL_JMASAI<<8)+0x03

#define JMASAI_TP10           (MODEL_JMASAI<<8)+0x04
#define JMASAI_TP20           (MODEL_JMASAI<<8)+0x05

/* cust features for android c&cpp modules. */

/* 20100920 YGP for Pantech USB Driver porting ========>*/
#define CONFIG_ANDROID_PANTECH_HSUSB
/* 20100920 YGP for Pantech USB Driver porting ========<*/

#if defined(CONFIG_HSUSB_PANTECH_OBEX)
#define CONFIG_HSUSB_PANTECH_USB_TEST
#endif

#undef FEATURE_PANTECH_MDS_MTC  /* MTC */ // block to define kim.eungbong
#if defined(FEATURE_PANTECH_MDS_MTC)
#define FEATURE_PANTECH_MAT      /* MAT */
#endif

#if defined(FEATURE_PANTECH_MDS_MTC)
#define FEATURE_DIAG_LARGE_PACKET
#endif

#if defined(FEATURE_AARM_RELEASE_MODE)
#define FEATURE_SKY_DM_MSG_VIEW_DISABLE
#define FEATURE_SW_RESET_RELEASE_MODE // use in release mode
#endif

#define FEATURE_PANTECH_VOLUME_CTL

#if (BOARD_VER >= JMASAI_PT10)
#define FEATURE_PANTECH_UMIC_HEADSET  //20100811 jhsong
#define FEATURE_PANTECH_ASR_NEW_DEVICE  //20100823 jhsong
#endif

/* Global features for SKY camera framework. */
#include "CUST_SKYCAM.h"
/* [DS3] Kang Seong-Goo framework features about SurfaceFlinger */
#include "cust_sky_surfaceflinger.h"

#ifndef FEATURE_SKY_FACTORY_COMMAND
#define FEATURE_SKY_FACTORY_COMMAND 
#endif

/* 20101008 hun_gps added for temp min */
#ifdef FEATURE_SKY_FACTORY_COMMAND
#define FEATURE_SKY_TEMP_MIN_BACKUP
#endif

/* stlee 20100903 */
#define FEATURE_PANTECH_VOLUME_CTL

/* [ES9 APP5 MP] `10.09.10 CBW  */
#define FEATURE_PANTECH_MEDIAFRAMEWORK
#undef FEATURE_PANTECH_MEDIAFRAMEWORK_DEBUG

/* J-MASAI,20101006,twnam.. added for JCDMA Debug Screen as Req of »ý±â */
#define FEATURE_JCDMA_DEBUG_SCREEN_SUPPORT

/*YGP 20101105*/
#define FEATURE_PANTECH_MSG_ONOFF

#endif/*_CUST_SKY_FILE_*/
