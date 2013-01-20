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


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <mach/gpio.h>

#include <mach/board.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/android_pmem.h>
#include <linux/poll.h>

#include <linux/sched.h>
#include <linux/time.h>
#include <linux/workqueue.h>

#include <sil9024_type.h>
#include <sil9024_dev.h>
#include <sil9024_i2c.h>
#include <sil9024_ddc.h>

#include <mach/vreg.h>

#include <mach/mpp.h>


#ifdef UNDEF_FB2_HDMI //modify.. fb2
#define MSM_HDMI_DEV_NAME    "sky_hdmi"
#define SKY_HDMI_DDC_DEV_NAME    "sky_hdmi_ddc"
#endif //UNDEF_FB2_HDMI

//#define I2S_AUDIO //

#define SKY_HDMI_AUDIO_I2S //2009.11.24 leecy add.. 

#ifdef SKY_HDMI_AUDIO_I2S
#define SCK_EDGE_RISING 0x80
#define FIFO_ENABLE     HDMI_BIT_7

#define FIFO_I2S_0      (0x00 << 4)
#define FIFO_I2S_1		(0x01 << 4)
#define FIFO_I2S_2		(0x02 << 4)
#define FIFO_I2S_3		(0x03 << 4)

#define AT_DWN_SAMPLE	HDMI_BIT_3
#define LR_SWAP         HDMI_BIT_2

#define SELECT_SD_CH0	0x00
#define SELECT_SD_CH1	0x01
#define SELECT_SD_CH2	0x02
#define SELECT_SD_CH3	0x03
#endif


#define TRUE 1
#define FALSE 0

#define OFF						0
#define ON						1

#define DUMMY					0xFF
#define ALL						0xFF


#ifndef MSM_HDMI_DBG_MSG
#define MSM_HDMI_DBG_MSG
#endif


#ifdef MSM_HDMI_DBG_MSG
#define HDMI_MSG(fmt, args...) printk("HDMI DEV : " fmt, ##args)
#else
#define HDMI_MSG(fmt, args...)
#endif

#define DISABLE 0x00
#define ENABLE  0xFF

#define CONF__TPI_EDID_PRINT    (DISABLE)


#define MSM_HDMI_RESET_GPIO  31
#define T_HDMI_RESET_DELAY 1 //ms....


// Time Constants Used in File TPI_Access.c Only
//==============================================
#define T_DDC_ACCESS	40		// About 2 seconds. May be tweaked as needed


// Constants used in file TPI.c only
//==================================

#define T_EN_TPI		10			// May be tweaked if needed
#define T_HW_RESET		 1			// May be tweaked if needed
#define T_HPD_DELAY	   100			// May be tweaked if needed


// Time Constants used in file HDCP.c only
//========================================
#define T_HDCP_AVAIL		10	// May be tweaked if needed
#define T_HDCP_ACTIVATION	10	// May be tweaked if needed
#define T_HDCP_DEACTIVATION	 5	// May be tweaked if needed

#define MAX_V_DESCRIPTORS			20
#define MAX_A_DESCRIPTORS			10
#define MAX_SPEAKER_CONFIGURATIONS	 4
#define MAX_COMMAND_ARGUMENTS		10
#define MAX_NUBER_OF_ERRORS			0x20		// size of ErrorMessageIndex queue

#define AUDIO_DESCR_SIZE			 3



#define HDMI_AFTER_INIT				1

#define DEV_SUPPORT_EDID

// Constants Specific to EDID.C Only
//==================================
#define ESTABLISHED_TIMING_INDEX		35		// Offset of Established Timing in EDID block
#define NUM_OF_STANDARD_TIMINGS			 8
#define STANDARD_TIMING_OFFSET			38
#define LONG_DESCR_LEN					18
#define NUM_OF_DETAILED_DESCRIPTORS	 	 4

#define DETAILED_TIMING_OFFSET		  0x36


// Offsets within a Long Descriptors Block
//========================================
#define HDMI_PIX_CLK_OFFSET					 0
#define HDMI_H_ACTIVE_OFFSET					 2
#define HDMI_H_BLANKING_OFFSET				 3
#define HDMI_V_ACTIVE_OFFSET				 	 5
#define HDMI_V_BLANKING_OFFSET				 6
#define HDMI_H_SYNC_OFFSET					 8
#define HDMI_H_SYNC_PW_OFFSET			 	 9
#define HDMI_V_SYNC_OFFSET				 	10
#define HDMI_V_SYNC_PW_OFFSET			 	10
#define HDMI_H_IMAGE_SIZE_OFFSET				12
#define HDMI_V_IMAGE_SIZE_OFFSET				13
#define HDMI_H_BORDER_OFFSET					15
#define HDMI_V_BORDER_OFFSET					16
#define HDMI_FLAGS_OFFSET					17

#define HDMI_AR16_10						 	 0
#define HDMI_AR4_3							 1
#define HDMI_AR5_4							 2
#define HDMI_AR16_9							 3


#define READ_DDC_A0   //test feature



#define ClearInterrupt(x)                   msm_hdmi_WriteByteTPI(TPI_INTERRUPT_STATUS, x)                   // write "1" to clear interrupt bit
#define MuteAudio()                         msm_hdmi_ReadSetWriteTPI(TPI_AUDIO_INTERFACE, BIT_AUDIO_MUTE)
#define UnMuteAudio()                       msm_hdmi_ReadClearWriteTPI(TPI_AUDIO_INTERFACE, BIT_AUDIO_MUTE)
//#define ReleaseDDC()				msm_hdmi_ReadClearWriteTPI(TPI_SYSTEM_CONTROL, BITS_RELEASE_DDC)


#define byte  unsigned char
#define word unsigned short
#define dword unsigned long


#ifndef HDMI_OLD_VER
///////////////////////////////////
//   
//             new..
//
/////////////////////////////////

#define DEV_SUPPORT_HDCP
//#define USE_DE_GENERATOR
#undef  USE_DE_GENERATOR

////////////////
// Globals.c

byte vid_mode;

#define TPI_INTERNAL_PAGE_REG   0xBC
#define TPI_REGISTER_OFFSET_REG 0xBD
#define TPI_REGISTER_VALUE_REG  0xBE


#define SiI_DEVICE_ID           0xB0

// FPLL Multipliers:
//==================
#define X1                      0x01

#define TPI_DEVICE_POWER_STATE_CTRL_REG		(0x1E)

#define CTRL_PIN_CONTROL_MASK				(HDMI_BIT_4)
#define CTRL_PIN_TRISTATE					(0x00)
#define CTRL_PIN_DRIVEN_TX_BRIDGE			(0x10)


#define TX_POWER_STATE_MASK			    (HDMI_BIT_1 | HDMI_BIT_0)
#define TX_POWER_STATE_D0					(0x00)
#define TX_POWER_STATE_D1					(0x01)
#define TX_POWER_STATE_D2					(0x02)
#define TX_POWER_STATE_D3					(0x03)

#define BYTE_SIZE               0x08
#define BITS_1_0                0x03
#define BITS_2_1                0x06
#define BITS_2_1_0              0x07
#define BITS_3_2                0x0C
#define BITS_5_4                0x30
#define BITS_5_4_3				0x38
#define BITS_6_5                0x60
#define BITS_6_5_4              0x70
#define BITS_7_6                0xC0


// Audio Modes
//============
#define AUD_PASS_BASIC      0x00
#define AUD_PASS_ALL        0x01
#define AUD_DOWN_SAMPLE     0x02
#define AUD_DO_NOT_CHECK    0x03

#define REFER_TO_STREAM_HDR     0x00
#define TWO_CHANNELS            0x00
#define EIGHT_CHANNELS          0x01
#define AUD_IF_SPDIF            0x40
#define AUD_IF_I2S              0x80
#define AUD_IF_DSD				0xC0
#define AUD_IF_HBR				0x04

#define TWO_CHANNEL_LAYOUT      0x00
#define EIGHT_CHANNEL_LAYOUT    0x20


// ===================================================== //

#define TPI_SYSTEM_CONTROL_DATA_REG			(0x1A)

#define LINK_INTEGRITY_MODE_MASK			(HDMI_BIT_6)
#define LINK_INTEGRITY_STATIC				(0x00)
#define LINK_INTEGRITY_DYNAMIC				(0x40)

#define TMDS_OUTPUT_CONTROL_MASK			(HDMI_BIT_4)
#define TMDS_OUTPUT_CONTROL_ACTIVE			(0x00)
#define TMDS_OUTPUT_CONTROL_POWER_DOWN		(0x10)

#define AV_MUTE_MASK						(HDMI_BIT_3)
#define AV_MUTE_NORMAL						(0x00)
#define AV_MUTE_MUTED						(0x08)

#define DDC_BUS_REQUEST_MASK				(HDMI_BIT_2)
#define DDC_BUS_REQUEST_NOT_USING			(0x00)
#define DDC_BUS_REQUEST_REQUESTED			(0x04)

#define DDC_BUS_GRANT_MASK					(HDMI_BIT_1)
#define DDC_BUS_GRANT_NOT_AVAILABLE			(0x00)
#define DDC_BUS_GRANT_GRANTED				(0x02)

#define OUTPUT_MODE_MASK					(HDMI_BIT_0)
#define OUTPUT_MODE_DVI						(0x00)
#define OUTPUT_MODE_HDMI					(0x01)


#define MISC_INFO_FRAMES_CTRL	0xBF
#define ENABLE_AND_REPEAT	0xC0


// ===================================================== //

#define TPI_HDCP_QUERY_DATA_REG				(0x29)

#define EXTENDED_LINK_PROTECTION_MASK		(HDMI_BIT_7)
#define EXTENDED_LINK_PROTECTION_NONE		(0x00)
#define EXTENDED_LINK_PROTECTION_SECURE		(0x80)

#define LOCAL_LINK_PROTECTION_MASK			(HDMI_BIT_6)
#define LOCAL_LINK_PROTECTION_NONE			(0x00)
#define LOCAL_LINK_PROTECTION_SECURE		(0x40)

#define LINK_STATUS_MASK					(HDMI_BIT_5 | HDMI_BIT_4)
#define LINK_STATUS_NORMAL					(0x00)
#define LINK_STATUS_LINK_LOST				(0x10)
#define LINK_STATUS_RENEGOTIATION_REQ		(0x20)
#define LINK_STATUS_LINK_SUSPENDED			(0x30)

#define HDCP_REPEATER_MASK					(HDMI_BIT_3)
#define HDCP_REPEATER_NO					(0x00)
#define HDCP_REPEATER_YES					(0x08)

#define CONNECTOR_TYPE_MASK					(HDMI_BIT_2 | HDMI_BIT_0)
#define CONNECTOR_TYPE_DVI					(0x00)
#define CONNECTOR_TYPE_RSVD					(0x01)
#define CONNECTOR_TYPE_HDMI					(0x04)
#define CONNECTOR_TYPE_FUTURE				(0x05)

#define PROTECTION_TYPE_MASK				(HDMI_BIT_1)
#define PROTECTION_TYPE_NONE				(0x00)
#define PROTECTION_TYPE_HDCP				(0x02)

// ===================================================== //

// ===================================================== //

#define TPI_HDCP_REVISION_DATA_REG			(0x30)

#define HDCP_MAJOR_REVISION_MASK			(HDMI_BIT_7 | HDMI_BIT_6 | HDMI_BIT_5 | HDMI_BIT_4)
#define HDCP_MAJOR_REVISION_VALUE			(0x10)

#define HDCP_MINOR_REVISION_MASK			(HDMI_BIT_3 | HDMI_BIT_2 | HDMI_BIT_1 | HDMI_BIT_0)
#define HDCP_MINOR_REVISION_VALUE			(0x02)

// ===================================================== //

// ===================================================== //

#define TPI_HDCP_CONTROL_DATA_REG			(0x2A)

#define PROTECTION_LEVEL_MASK				(HDMI_BIT_0)
#define PROTECTION_LEVEL_MIN				(0x00)
#define PROTECTION_LEVEL_MAX				(0x01)

// ===================================================== //



#ifdef DEV_SUPPORT_HDCP

bool HDCP_TxSupports = FALSE;
bool HDCP_Started;
byte HDCP_LinkProtectionLevel;

#ifdef READBKSV
static bool GetBKSV(void);
#endif
#endif



#endif //#ifdef HDMI_OLD_VER
///]]]]]]]]]]]]]]]]]



typedef struct {				// for storing EDID parsed data
	byte VideoDescriptor[MAX_V_DESCRIPTORS];	// maximum number of video descriptors
	byte AudioDescriptor[MAX_A_DESCRIPTORS][3];	// maximum number of audio descriptors
	byte SpkrAlloc[MAX_SPEAKER_CONFIGURATIONS];	// maximum number of speaker configurations
	bool UnderScan;								// "1" if DTV monitor underscans IT video formats by default
	bool BasicAudio;							// Sink supports Basic Audio
	bool YCbCr_4_4_4;							// Sink supports YCbCr 4:4:4
	bool YCbCr_4_2_2;							// Sink supports YCbCr 4:2:2
	bool HDMI_Sink;								// "1" if HDMI signature found
	byte CEC_A_B;								// CEC Physical address. See HDMI 1.3 Table 8-6
	byte CEC_C_D;
	byte ColorimetrySupportFlags;				// IEC 61966-2-4 colorimetry support: 1 - xvYCC601; 2 - xvYCC709 
	byte MetadataProfile;

} Type_EDID_Descriptors;


// Video Mode Table Data Structures
//=================================
typedef struct {
	byte Mode_C1;
	byte Mode_C2;
	byte SubMode;
} ModeIdType;

typedef struct {
	word Pixels;
	word Lines;
} PxlLnTotalType;

typedef struct {
	word H;
	word V;
} HVPositionType;

typedef struct {
	word H;
	word V;
} HVResolutionType;

typedef struct{
   byte RefrTypeVHPol;
   word VFreq;
   PxlLnTotalType Total;
} TagType;

typedef struct {
	byte IntAdjMode;
	word HLength;
	byte VLength;
	word Top;
	word Dly;
	word HBit2HSync;
	byte VBit2VSync;
	word Field2Offset;
}  _656Type;

typedef struct {
	ModeIdType ModeId;
	word PixClk;
	TagType Tag;
	HVPositionType Pos;
	HVResolutionType Res;
	byte AspectRatio;
	_656Type _656;
	byte PixRep;
} VModeInfoType;


// API Interface Data Structures
//==============================
typedef struct {
	byte	OpCode;
	byte	CommandLength;
	byte	Arg[MAX_COMMAND_ARGUMENTS];
} API_Cmd;



struct msm_hdmi_dev {
  struct cdev cdev;
  struct device *dev;
  int reset;
  int irq;
};

struct sky_hdmi_ddc_dev {
  struct cdev cdev;
  struct device *dev;
  int reset;
};


//static struct msm_hdmi_dev *msm_hdmi_device;
//static struct class *msm_hdmi_class;
//static dev_t msm_hdmi_devno;


//static struct sky_hdmi_ddc_dev *sky_hdmi_ddc_dev;
//static struct class *sky_hdmi_ddc_class;
//static dev_t sky_hdmi_ddc_devno;


#ifdef HDMI_OLD_VER //sky feature..
static byte hdmi_AuthState;				// Authentication state variable
static byte hdmi_OutputState;			// Output state variable
static byte hdmi_LinkProtectionLevel; 	// Hold the most recently set link protection level from 0x29[7:6]
static byte BKSV_Array[BKSV_ARRAY_SIZE];		// Holds KSV values read from the DDC KSV FIFO.
#endif

Type_EDID_Descriptors EDID_Data;		// holds parsed EDID data needed by the FW




#ifndef HDMI_OLD_VER
///////////////////////////////////
//   
//             new..
//
/////////////////////////////////
byte IDX_InChar;
byte NumOfArgs;
bool F_SBUF_DataReady;
bool F_CollectingData;

#ifndef DEBUG_EDID
bool F_IgnoreEDID;                  // Allow setting of any video input format regardless of Sink's EDID (for debuggin puroses)
#endif

#ifdef MHL_CABLE_HPD
bool mhlCableConnected;
#endif
bool hdmiCableConnected;
bool dsRxPoweredUp;
bool tmdsPoweredUp;


bool edidDataValid;

static byte sky_hdmi_vid_mode_f;

#endif //#ifdef HDMI_OLD_VER
///]]]]]]]]]]]]]]]]]


byte EDID_TempData[EDID_BLOCK_SIZE + 3];



#define IsHDMI_Sink()				EDID_Data.HDMI_Sink




#define TxPowerUp()					msm_hdmi_WriteByteTPI(TPI_DEVICE_PWR_STATE, PWR_D0)			// 0x1E[1:0] = 00
#define EnableTMDS()				msm_hdmi_ReadClearWriteTPI(TPI_SYSTEM_CONTROL, BIT_TMDS_OUTPUT)	// 0x1A[4] = 0
#define DisableTMDS()				msm_hdmi_ReadSetWriteTPI(TPI_SYSTEM_CONTROL, BIT_TMDS_OUTPUT)	// 0x1A[4] = 1

#define ReadBlockEDID(a,b,c)       msm_hdmi_ReadBlockTPI(EDID_ROM_ADDR, a, c, b)
#define ReadSegmentBlockEDID(a,b,c,d)   msm_hdmi_ReadSegmentBlockTPI(EDID_ROM_ADDR, a, b, d, c)
#define ReadBlockEDIDseg(a,b,c,d)       msm_hdmi_ReadSegBlockTPI(EDID_ROM_ADDR, a, b, d, c)

const byte edid_sample_data[] = {
#if 0
  /*samsung sample*/
  0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x4C,0x2D,0x67,0x02,0x33,0x34,0x38,0x4E,
  0x1C,0x11,0x01,0x03,0x80,0x10,0x09,0x78,0x0A,0xDB,0x71,0xA4,0x55,0x49,0x99,0x27,
  0x13,0x50,0x54,0x20,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x1D,0x80,0x18,0x71,0x1C,0x16,0x20,0x58,0x2C,
  0x25,0x00,0xA0,0x5A,0x00,0x00,0x00,0x9E,0x01,0x1D,0x00,0x72,0x51,0xD0,0x1E,0x20,
  0x6E,0x28,0x55,0x00,0xA0,0x5A,0x00,0x00,0x00,0x1E,0x00,0x00,0x00,0xFD,0x00,0x37,
  0x41,0x1E,0x44,0x10,0x00,0x0A,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xFC,
  0x00,0x53,0x41,0x4D,0x53,0x55,0x4E,0x47,0x20,0x54,0x56,0x0A,0x20,0x20,0x01,0x2A,
  0x02,0x03,0x17,0x71,0x44,0x85,0x04,0x03,0x10,0x23,0x09,0x07,0x07,0x83,0x01,0x00,
  0x00,0x65,0x03,0x0C,0x00,0x10,0x00,0x8C,0x0A,0xD0,0x8A,0x20,0xE0,0x2D,0x10,0x10,
  0x3E,0x96,0x00,0xA0,0x5A,0x00,0x00,0x00,0x18,0x02,0x3A,0x80,0x18,0x71,0x38,0x2D,
  0x40,0x58,0x2C,0x45,0x00,0xA0,0x5A,0x00,0x00,0x00,0x1E,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x63
#else
  /*lg sample*/
  0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1e, 0x6d, 0xc4, 0x56, 0x01, 0x01, 0x01, 0x01, 
  0x08, 0x13, 0x01, 0x03, 0x80, 0x33, 0x1c, 0x78, 0x0a, 0xc9, 0xa5, 0xa4, 0x56, 0x4b, 0x9d, 0x25, 
  0x12, 0x50, 0x54, 0xa5, 0x6f, 0x00, 0x81, 0x80, 0x81, 0x8f, 0x71, 0x40, 0xb3, 0x00, 0x81, 0x4f, 
  0xd1, 0xc0, 0x01, 0x01, 0x01, 0x01, 0x1a, 0x36, 0x80, 0xa0, 0x70, 0x38, 0x1f, 0x40, 0x30, 0x20, 
  0x35, 0x00, 0xfd, 0x1e, 0x11, 0x00, 0x00, 0x1a, 0x21, 0x39, 0x90, 0x30, 0x62, 0x1a, 0x27, 0x40, 
  0x68, 0xb0, 0x36, 0x00, 0xfd, 0x1e, 0x11, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x38, 
  0x4b, 0x1e, 0x53, 0x11, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfc, 
  0x00, 0x4d, 0x32, 0x33, 0x39, 0x34, 0x44, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x09,
  0x02, 0x03, 0x2b, 0x72, 0x55, 0x90, 0x84, 0x03, 0x02, 0x0e, 0x0f, 0x07, 0x23, 0x24, 0x05, 0x94, 
  0x13, 0x12, 0x11, 0x1d, 0x1e, 0x20, 0x21, 0x22, 0x01, 0x1f, 0x23, 0x09, 0x07, 0x07, 0x83, 0x01, 
  0x00, 0x00, 0x68, 0x03, 0x0c, 0x00, 0x10, 0x00, 0xb8, 0x2d, 0x00, 0x8c, 0x0a, 0xd0, 0x8a, 0x20, 
  0xe0, 0x2d, 0x10, 0x10, 0x3e, 0x96, 0x00, 0xc4, 0x8e, 0x21, 0x00, 0x00, 0x18, 0x8c, 0x0a, 0xd0, 
  0x90, 0x20, 0x40, 0x31, 0x20, 0x0c, 0x40, 0x55, 0x00, 0xc4, 0x8e, 0x21, 0x00, 0x00, 0x18, 0x01, 
  0x1d, 0x80, 0x18, 0x71, 0x1c, 0x16, 0x20, 0x58, 0x2c, 0x25, 0x00, 0xc4, 0x8e, 0x21, 0x00, 0x00, 
  0x9e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c  
#endif
};


//////////////////////////////////////////////////////////////////////////////
//
// PROJECT	: 9022/4 - Xavier
//
// CONTAINS	: Table of video modes supported by the 9022/4.
//
//  CONDITIONAL COMPILATION FLAGS (defined in file cpldefs.h):
//
//////////////////////////////////////////////////////////////////////////////
const VModeInfoType VModesTable[] = {
	// M.Id SubM-	PixClk  RefTVPolHPol		RefrRt {HTot, VTot} {HStrt,VStrt}{HRes,VRes}  -AR- 656 - PixRep
	{{ 1,  0, NSM},	 2517, {ProgrVNegHNeg,		 6000, { 800,  525}}, {144, 35}, { 640, 480}, _4,		{0,  96, 2, 33,  48,  16, 10,    0}, 0}, // 1. 640 x 480p @ 60 VGA
	{{ 2,  3, NSM},	 2700, {ProgrVNegHNeg,		 6000, { 858,  525}}, {122, 36}, { 720, 480}, _4or16,	{0,  62, 6, 30,  60,  19,  9,    0}, 0}, // 2,3 720 x 480p
	{{ 4,  0, NSM},	 7425, {ProgrVPosHPos,		 6000, {1650,  750}}, {260, 25}, {1280, 720}, _16,		{0,  40, 5, 20, 220, 110,  5,    0}, 0}, // 4   1280 x 720p
	{{ 5,  0, NSM},	 7425, {InterlaceVPosHPos,	 6000, {2200,  562}}, {192, 20}, {1920,1080}, _16,		{0,  44, 5, 15, 148,  88,  2, 1100}, 0}, // 5 1920 x 1080i
	{{ 6,  7, NSM},	 2700, {InterlaceVNegHNeg,	 6000, {1716,  264}}, {119, 18}, { 720, 480}, _4or16,	{3,  62, 3, 15, 114,  17,  5,  429}, 1}, // 6,7 1440 x 480i,pix repl
	{{ 8,  9,   1},	 2700, {ProgrVNegHNeg,		 6000, {1716,  262}}, {119, 18}, {1440, 240}, _4or16,	{0, 124, 3, 15, 114,  38,  4,    0}, 1}, // 8,9(1) 1440 x 240p
	{{ 8,  9,   2},	 2700, {ProgrVNegHNeg,		 6000, {1716,  263}}, {119, 18}, {1440, 240}, _4or16,	{0, 124, 3, 15, 114,  38,  4,    0}, 1}, // 8,9(2) 1440 x 240p
	{{10, 11, NSM},	 5400, {InterlaceVNegHNeg,	 6000, {3432,  525}}, {238, 18}, {2880, 480}, _4or16,	{0, 248, 3, 15, 228,  76,  4, 1716}, 0}, // 10,11 2880 x 480p
	{{12, 13,   1},	 5400, {ProgrVNegHNeg,		 6000, {3432,  262}}, {238, 18}, {2880, 240}, _4or16,	{0, 248, 3, 15, 228,  76,  4,    0}, 1}, // 12,13(1) 2280 x 240p
	{{12, 13,   2},	 5400, {ProgrVNegHNeg,		 6000, {3432,  263}}, {238, 18}, {2880, 240}, _4or16,	{0, 248, 3, 15, 228,  76,  4,    0}, 1}, // 12,13(2) 2280 x 240p
	{{14, 15, NSM},	 5400, {ProgrVNegHNeg,		 6000, {1716,  525}}, {244, 36}, {1440, 480}, _4or16,	{0, 124, 6, 30, 120,  32,  9,    0}, 0}, // 14,15 1140 x 480p
	{{16,  0, NSM},	14835, {ProgrVPosHPos,		 6000, {2200, 1125}}, {192, 41}, {1920,1080}, _16,		{0,  44, 5, 36, 148,  88,  4,    0}, 0}, // 16 1920 x 1080p
	{{17, 18, NSM},	 2700, {ProgrVNegHNeg,		 5000, { 864,  625}}, {132, 44}, { 720, 576}, _4or16,	{0,  64, 5, 39,  68,  12,  5,    0}, 0}, // 17,18 720 x 576p
	{{19,  0, NSM},	 7425, {ProgrVPosHPos,		 5000, {1980,  750}}, {260, 25}, {1280, 720}, _16,		{0,  40, 5, 20, 220, 440,  5,    0}, 0}, // 19 1280 x 720p
	{{20,  0, NSM},	 7425, {InterlaceVPosHPos,	 5000, {2640, 1125}}, {192, 20}, {1920,1080}, _16,		{0,  44, 5, 15, 148, 528,  2, 1320}, 0}, // 20 1920 x 1280p
	{{21, 22, NSM},	 2700, {InterlaceVNegHNeg,	 5000, {1728,  625}}, {132, 22}, { 720, 576}, _4,		{3,  63, 3, 19, 138,  24,  2,  432}, 1}, // 21,22 1440 x 576i
	{{23, 24,   1},	 2700, {ProgrVNegHNeg,		 5000, {1728,  312}}, {132, 22}, {1440, 288}, _4or16,	{0, 126, 3, 19, 138,  24,  2,    0}, 1}, // 23,24(1) 1440 x 288p
	{{23, 24,   2},	 2700, {ProgrVNegHNeg,		 5000, {1728,  313}}, {132, 22}, {1440, 288}, _4or16,	{0, 126, 3, 19, 138,  24,  2,    0}, 1}, // 23,24(2) 1440 x 288p
	{{23, 24,   3},	 2700, {ProgrVNegHNeg,		 5000, {1728,  314}}, {132, 22}, {1440, 288}, _4or16,	{0, 126, 3, 19, 138,  24,  2,    0}, 1}, // 23,24(3) 1440 x 288p
	{{25, 26, NSM},	 5400, {InterlaceVNegHNeg,	 5000, {3456,  625}}, {264, 22}, {2880, 576}, _4or16,	{0, 252, 3, 19, 276,  48,  2, 1728}, 1}, // 25,26 2880 x 576p
	{{27, 28,   1},	 5400, {ProgrVNegHNeg,		 5000, {3456,  312}}, {264, 22}, {2880, 288}, _4or16,	{0, 252, 3, 19, 276,  48,  2,    0}, 1}, // 27,28(1) 2880 x 288p
	{{27, 28,   2},	 5400, {ProgrVNegHNeg,		 5000, {3456,  313}}, {264, 22}, {2880, 288}, _4or16,	{0, 252, 3, 19, 276,  48,  3,    0}, 1}, // 27,28(2) 2880 x 288p
	{{27, 28,   3},	 5400, {ProgrVNegHNeg,		 5000, {3456,  314}}, {264, 22}, {2880, 288}, _4or16,	{0, 252, 3, 19, 276,  48,  4,    0}, 1}, // 27,28(3) 2880 x 288p
	{{29, 30, NSM},	 5400, {ProgrVPosHNeg,		 5000, {1728,  625}}, {264, 44}, {1440, 576}, _4or16,	{0, 128, 5, 39, 136,  24,  5,    0}, 0}, // 29,30 1440 x 576p
	{{31,  0, NSM},	14850, {ProgrVPosHPos,		 5000, {2640, 1125}}, {192, 41}, {1920,1080}, _16,		{0,  44, 5, 36, 148, 528,  4,    0}, 0}, // 31(1) 1920 x 1080p
	{{32,  0, NSM},	 7417, {ProgrVPosHPos,		 2400, {2750, 1125}}, {192, 41}, {1920,1080}, _16,		{0,  44, 5, 36, 148, 638,  4,    0}, 0}, // 32(2) 1920 x 1080p
	{{33,  0, NSM},	 7425, {ProgrVPosHPos,		 2500, {2640, 1125}}, {192, 41}, {1920,1080}, _16,		{0,  44, 5, 36, 148, 528,  4,    0}, 0}, // 33(3) 1920 x 1080p
	{{34,  0, NSM},	 7417, {ProgrVPosHPos,		 3000, {2200, 1125}}, {192, 41}, {1920,1080}, _16,		{0,  44, 5, 36, 148, 528,  4,    0}, 0}, // 34(4) 1920 x 1080p
	{{35, 36, NSM},	10800, {ProgrVNegHNeg,		 5994, {3432,  525}}, {488, 36}, {2880, 480}, _4or16,	{0, 248, 6, 30, 240,  64, 10,    0}, 0}, // 35, 36 2880 x 480p@59.94/60Hz
	{{37, 38, NSM},	10800, {ProgrVNegHNeg,		 5000, {3456,  625}}, {272, 39}, {2880, 576}, _4or16,	{0, 256, 5, 40, 272,  48,  5,    0}, 0}, // 37, 38 2880 x 576p@50Hz
	{{39,  0, NSM},	 7200, {InterlaceVNegHNeg,	 5000, {2304, 1250}}, {352, 62}, {1920,1080}, _16,		{0, 168, 5, 87, 184,  32, 24,    0}, 0}, // 39 1920 x 1080i@50Hz
	{{40,  0, NSM},	14850, {InterlaceVPosHPos,	10000, {2640, 1125}}, {192, 20}, {1920,1080}, _16,		{0,  44, 5, 15, 148, 528,  2, 1320}, 0}, // 40 1920 x 1080i@100Hz
	{{41,  0, NSM},	14850, {InterlaceVPosHPos,	10000, {1980,  750}}, {260, 25}, {1280, 720}, _16,		{0,  40, 5, 20, 220, 400,  5,    0}, 0}, // 41 1280 x 720p@100Hz
	{{42, 43, NSM},	 5400, {ProgrVNegHNeg,		10000, { 864,  144}}, {132, 44}, { 720, 576}, _4or16,	{0,  64, 5, 39,  68,  12,  5,    0}, 0}, // 42, 43, 720p x 576p@100Hz
	{{44, 45, NSM},	 5400, {InterlaceVNegHNeg,	10000, { 864,  625}}, {132, 22}, { 720, 576}, _4or16,	{0,  63, 3, 19,  69,  12,  2,  432}, 1}, // 44, 45, 720p x 576i@100Hz, pix repl
	{{46,  0, NSM},	14835, {InterlaceVPosHPos,	11988, {2200, 1125}}, {192, 20}, {1920,1080}, _16,		{0,  44, 5, 15, 149,  88,  2, 1100}, 0}, // 46, 1920 x 1080i@119.88/120Hz
	{{47,  0, NSM},	14835, {ProgrVPosHPos,		11988, {1650,  750}}, {260, 25}, {1280, 720}, _16,		{0,  40, 5, 20, 220, 110,  5, 1100}, 0}, // 47, 1280 x 720p@119.88/120Hz
	{{48, 49, NSM},	 5400, {ProgrVNegHNeg,		11988, { 858,  525}}, {122, 36}, { 720, 480}, _4or16,	{0,  62, 6, 30,  60,  16, 10,    0}, 0}, // 48, 49 720 x 480p@119.88/120Hz
	{{50, 51, NSM},	 5400, {InterlaceVNegHNeg,	11988, { 858,  525}}, {119, 18}, { 720, 480}, _4or16,	{0,  62, 3, 15,  57,  19,  4,  429}, 1}, // 50, 51 720 x 480i@119.88/120Hz
	{{52, 53, NSM},	10800, {ProgrVNegHNeg,		20000, { 864,  625}}, {132, 44}, { 720, 576}, _4or16,	{0,  64, 5, 39,  68,  12,  5,    0}, 0}, // 52, 53, 720 x 576p@200Hz
	{{54, 55, NSM},	10800, {InterlaceVNegHNeg,	20000, { 864,  625}}, {132, 22}, { 720, 576}, _4or16,	{0,  63, 3, 19,  69,  12,  2,  432}, 1}, // 54, 55, 1440 x 720i @200Hz, pix repl
	{{56, 57, NSM},	10800, {ProgrVNegHNeg,		24000, { 858,  525}}, {122, 42}, { 720, 480}, _4or16,	{0,  62, 6, 30,  60,  16,  9,    0}, 0}, // 56, 57, 720 x 480p @239.76/240Hz
	{{58, 59, NSM},	10800, {InterlaceVNegHNeg,	24000, { 858,  525}}, {119, 18}, { 720, 480}, _4or16,	{0,  62, 3, 15,  57,  19,  4,  429}, 1}, // 58, 59, 1440 x 480i @239.76/240Hz, pix repl
/*
// NOTE: DO NOT ATTEMPT INPUT RESOLUTIONS THAT REQUIRE PIXEL CLOCK FREQUENCIES HIGHER THAN THOSE SUPPOTED BY THE TRANSMITTER CHIP

                             1         2                 3       4    5         6 7          8   9      10    11, 13, 15 */                             
	{{PC_BASE, 0,NSM},      3150,   {ProgrVNegHPos,     8508,   {832, 445}},    {160,63},   {640,350},   _16,  {0,64,3,60,96,32,32, 0}, 0}, // 640x350@85.08
	{{PC_BASE+1, 0,NSM},    3150,   {ProgrVPosHNeg,     8508,   {832, 445}},    {160,44},   {640,400},   _16,  {0,64,3,41,96,32,1, 0},  0}, // 640x400@85.08
	{{PC_BASE+2, 0,NSM},    2700,   {ProgrVPosHNeg,     7008,   {900, 449}},    {0,0},      {720,400},   _16,  {0,0,0,0,0,0,0, 0},      0}, // 720x400@70.08
	{{PC_BASE+3, 0,NSM},    3500,   {ProgrVPosHNeg,     8504,   {936, 446}},    {20,45},    {720,400},   _16,  {0,72,3,42,108,36,1, 0}, 0}, // 720x400@85.04
	{{PC_BASE+4, 0,NSM},    2517,   {ProgrVNegHNeg,     5994,   {800, 525}},    {144,35},   {640,480},   _4,  {0,96,2,33,48,16,10,0},  0}, // 640x480@59.94
	{{PC_BASE+5, 0,NSM},    3150,   {ProgrVNegHNeg,     7281,   {832, 520}},    {144,31},   {640,480},   _4,  {0,40,3,28,128,128,9,0}, 0}, // 640x480@72.80
	{{PC_BASE+6, 0,NSM},    3150,   {ProgrVNegHNeg,     7500,   {840, 500}},    {21,19},    {640,480},   _4,  {0,64,3,28,128,24,9, 0}, 0}, // 640x480@75.00
	{{PC_BASE+7,0,NSM},     3600,   {ProgrVNegHNeg,     8500,   {832, 509}},    {168,28},   {640,480},   _4,  {0,56,3,25,128,24,9, 0}, 0}, // 640x480@85.00
	{{PC_BASE+8,0,NSM},     3600,   {ProgrVPosHPos,     5625,   {1024, 625}},   {200,24},   {800,600},   _4,  {0,72,2,22,128,24,1, 0}, 0}, // 800x600@56.25
	{{PC_BASE+9,0,NSM},     4000,   {ProgrVPosHPos,     6032,   {1056, 628}},   {216,27},   {800,600},   _4,  {0,128,4,23,88,40,1, 0}, 0}, // 800x600@60.317
	{{PC_BASE+10,0,NSM},    5000,   {ProgrVPosHPos,     7219,   {1040, 666}},   {184,29},   {800,600},   _4,  {0,120,6,23,64,56,37,0}, 0}, // 800x600@72.19
	{{PC_BASE+11,0,NSM},    4950,   {ProgrVPosHPos,     7500,   {1056, 625}},   {240,24},   {800,600},   _4,  {0,80,3,21,160,16,1,0},  0}, // 800x600@75
	{{PC_BASE+12,0,NSM},    5625,   {ProgrVPosHPos,     8506,   {1048, 631}},   {216,30},   {800,600},   _4,  {0,64,3,27,152,32,1,0},  0}, // 800x600@85.06
	{{PC_BASE+13,0,NSM},    6500,   {ProgrVNegHNeg,     6000,   {1344, 806}},   {296,35},   {1024,768},  _4,  {0,136,6,29,160,24,3,0}, 0}, // 1024x768@60
	{{PC_BASE+14,0,NSM},    7500,   {ProgrVNegHNeg,     7007,   {1328, 806}},   {280,35},   {1024,768},  _4,  {0,136,6,19,144,24,3,0}, 0}, // 1024x768@70.07
	{{PC_BASE+15,0,NSM},    7875,   {ProgrVPosHPos,     7503,   {1312, 800}},   {272,31},   {1024,768},  _4,  {0,96,3,28,176,16,1,0},  0}, // 1024x768@75.03
	{{PC_BASE+16,0,NSM},    9450,   {ProgrVPosHPos,     8500,   {1376, 808}},   {304,39},   {1024,768},  _4,  {0,96,3,36,208,48,1,0},  0}, // 1024x768@85
	{{PC_BASE+17,0,NSM},   10800,   {ProgrVPosHPos,     7500,   {1600, 900}},   {384,35},   {1152,864},  _4,  {0,128,3,32,256,64,1,0}, 0}, // 1152x864@75
	{{PC_BASE+18,0,NSM},   16200,   {ProgrVPosHPos,     6000,   {2160, 1250}},  {496,49},   {1600,1200}, _4,  {0,304,3,46,304,64,1,0}, 0}, // 1600x1200@60
	{{PC_BASE+19,0,NSM},    6825,   {ProgrVNegHPos,     6000,   {1440, 790}},   {112,19},   {1280,768},  _16,  {0,32,7,12,80,48,3,0},   0}, // 1280x768@59.95
	{{PC_BASE+20,0,NSM},    7950,   {ProgrVPosHNeg,     5987,   {1664, 798}},   {320,27},   {1280,768},  _16,  {0,128,7,20,192,64,3,0}, 0}, // 1280x768@59.87
	{{PC_BASE+21,0,NSM},   10220,   {ProgrVPosHNeg,     6029,   {1696, 805}},   {320,27},   {1280,768},  _16,  {0,128,7,27,208,80,3,0}, 0}, // 1280x768@74.89
	{{PC_BASE+22,0,NSM},   11750,   {ProgrVPosHNeg,     8484,   {1712, 809}},   {352,38},   {1280,768},  _16,  {0,136,7,31,216,80,3,0}, 0}, // 1280x768@85
	{{PC_BASE+23,0,NSM},   10800,   {ProgrVPosHPos,     6000,   {1800, 1000}},  {424,39},   {1280,960},  _4,  {0,112,3,36,312,96,1,0}, 0}, // 1280x960@60
	{{PC_BASE+24,0,NSM},   14850,   {ProgrVPosHPos,     8500,   {1728, 1011}},  {384,50},   {1280,960},  _4,  {0,160,3,47,224,64,1,0}, 0}, // 1280x960@85
	{{PC_BASE+25,0,NSM},   10800,   {ProgrVPosHPos,     6002,   {1688, 1066}},  {360,41},   {1280,1024}, _4,  {0,112,3,38,248,48,1,0}, 0}, // 1280x1024@60
	{{PC_BASE+26,0,NSM},   13500,   {ProgrVPosHPos,     7502,   {1688, 1066}},  {392,41},   {1280,1024}, _4,  {0,144,3,38,248,16,1, 0},0}, // 1280x1024@75
	{{PC_BASE+27,0,NSM},   15750,   {ProgrVPosHPos,     8502,   {1728, 1072}},  {384,47},   {1280,1024}, _4,  {0,160,3,4,224,64,1, 0}, 0}, // 1280x1024@85
	{{PC_BASE+28,0,NSM},    8550,   {ProgrVPosHPos,     6002,   {1792, 795}},   {368,24},   {1360,768},  _16,  {0,112,6,18,256,64,3,0}, 0}, // 1360x768@60
	{{PC_BASE+29,0,NSM},   10100,   {ProgrVNegHPos,     5995,   {1560, 1080}},  {112,27},   {1400,1050}, _4,  {0,32,4,23,80,48,3,0},   0}, // 1400x105@59.95
	{{PC_BASE+30,0,NSM},   12175,   {ProgrVPosHNeg,     5998,   {1864, 1089}},  {376,36},   {1400,1050}, _4,  {0,144,4,32,232,88,3,0}, 0}, // 1400x105@59.98
	{{PC_BASE+31,0,NSM},   15600,   {ProgrVPosHNeg,     7487,   {1896, 1099}},  {392,46},   {1400,1050}, _4,  {0,144,4,22,248,104,3,0},0}, // 1400x105@74.87
	{{PC_BASE+32,0,NSM},   17950,   {ProgrVPosHNeg,     8496,   {1912, 1105}},  {408,52},   {1400,1050}, _4,  {0,152,4,48,256,104,3,0},0}, // 1400x105@84.96
	{{PC_BASE+33,0,NSM},   17550,   {ProgrVPosHPos,     6500,   {2160, 1250}},  {496,49},   {1600,1200}, _4,  {0,192,3,46,304,64,1,0}, 0}, // 1600x1200@65
	{{PC_BASE+34,0,NSM},   18900,   {ProgrVPosHPos,     7000,   {2160, 1250}},  {496,49},   {1600,1200}, _4,  {0,192,3,46,304,64,1,0}, 0}, // 1600x1200@70
	{{PC_BASE+35,0,NSM},   20250,   {ProgrVPosHPos,     7500,   {2160, 1250}},  {496,49},   {1600,1200}, _4,  {0,192,3,46,304,64,1,0}, 0}, // 1600x1200@75
	{{PC_BASE+36,0,NSM},   22950,   {ProgrVPosHPos,     8500,   {2160, 1250}},  {496,49},   {1600,1200}, _4,  {0,192,3,46,304,64,1,0}, 0}, // 1600x1200@85
	{{PC_BASE+37,0,NSM},   20475,   {ProgrVPosHNeg,     6000,   {2448, 1394}},  {528,49},   {1792,1344}, _4,  {0,200,3,46,328,128,1,0},0}, // 1792x1344@60
	{{PC_BASE+38,0,NSM},   26100,   {ProgrVPosHNeg,     7500,   {2456, 1417}},  {568,72},   {1792,1344}, _4,  {0,216,3,69,352,96,1,0}, 0}, // 1792x1344@74.997
	{{PC_BASE+39,0,NSM},   21825,   {ProgrVPosHNeg,     6000,   {2528, 1439}},  {576,46},   {1856,1392}, _4,  {0,224,3,43,352,96,1,0}, 0}, // 1856x1392@60
	{{PC_BASE+40,0,NSM},   28800,   {ProgrVPosHNeg,     7500,   {2560, 1500}},  {576,107},  {1856,1392}, _4,  {0,224,3,104,352,128,1,0},0}, // 1856x1392@75
	{{PC_BASE+41,0,NSM},   15400,   {ProgrVNegHPos,     5995,   {2080, 1235}},  {112,32},   {1920,1200}, _16,  {0,32,6,26,80,48,3,0},    0}, // 1920x1200@59.95
	{{PC_BASE+42,0,NSM},   19325,   {ProgrVPosHNeg,     5988,   {2592, 1245}},  {536,42},   {1920,1200}, _16,  {0,200,6,36,336,136,3,0}, 0}, // 1920x1200@59.88
	{{PC_BASE+43,0,NSM},   24525,   {ProgrVPosHNeg,     7493,   {2608, 1255}},  {552,52},   {1920,1200}, _16,  {0,208,6,46,344,136,3,0}, 0}, // 1920x1200@74.93
	{{PC_BASE+44,0,NSM},   28125,   {ProgrVPosHNeg,     8493,   {2624, 1262}},  {560,59},   {1920,1200}, _16,  {0,208,6,53,352,144,3,0}, 0}, // 1920x1200@84.93
	{{PC_BASE+45,0,NSM},   23400,   {ProgrVPosHNeg,     6000,   {2600, 1500}},  {552,59},   {1920,1440}, _4,  {0,208,3,56,344,128,1,0}, 0}, // 1920x1440@60
	{{PC_BASE+46,0,NSM},   29700,   {ProgrVPosHNeg,     7500,   {2640, 1500}},  {576,59},   {1920,1440}, _4,  {0,224,3,56,352,144,1,0}, 0}, // 1920x1440@75
	{{PC_BASE+47,0,NSM},   29700,   {ProgrVPosHNeg,     7500,   {2640, 1500}},  {576,59},   {1920,1440}, _4,  {0,224,3,56,352,144,1,0}, 0}, // 1920x1440@75
	};


/////////////////////////////////////////////////////////////////////////////////////
//
// Aspect Ratio table defines the aspect ratio as function of VIC. This table 
// should be used in conjunction with the 861-D part of VModeInfoType VModesTable[]
// (formats 0 - 59) because some formats that differ only in their AR are grouped 
// together (e.g., formats 2 and 3). 

/////////////////////////////////////////////////////////////////////////////////////
const byte AspectRatioTable[] = 
	{
		_4, _4, _16, _16, _16, _4, _16, _4, _16, _4,
		_16, _4, _16, _4, _16, _16, _4, _16, _16, _16, 
		_4, _16, _4, _16, _4, _16, _4, _16, _4, _16, 
		_16, _16, _16, _16, _4, _16, _4, _16, _16, _16, 
		_16, _4, _16, _4, _16, _16, _16, _4, _16, _4, 
		_16, _4, _16, _4, _16, _4, _16, _4, _16
	};

static int de_switch_to_hdmi(void);
static int de_switch_to_lcdc(void);
static int sky_hdmi_set_path(void);
static int sky_hdmi_unset_path(void);
static int msm_hdmi_power_off(void);
static int sky_hdmi_gpio_reset(void);
//static int msm_hdmi_InitSystem(void);
byte msm_hdmi_ParseEDID(void);
bool msm_hdmi_IsHDCP_Available(void);
int msm_hdmi_drv_start(void);
#if 0
void sky_hdmi_interrupt(void);
#else
static irqreturn_t sky_hdmi_interrupt(int irq, void *dev_id);
#endif
void sky_hdmi_set_isr(int on_off);

#ifdef UNDEF_FB2_HDMI //modify.. fb2
static int msm_hdmi_ioctl(struct inode *inodep, struct file *filp, unsigned int cmd, unsigned long arg);
#endif
//static void msm_hdmi_dev_Init(void);


void ReadModifyWriteTPI(byte Offset, byte Mask, byte Value);
bool msm_hdmi_ReadByteTPI(byte RegOffset, byte *Data);
bool msm_hdmi_WriteByteTPI(byte RegOffset, byte Data);
int sky_hdmi_dev_init(void);


#ifdef DEV_SUPPORT_HDCP

extern bool HDCP_TxSupports;			// True if the TX supports HDCP and has legal AKSVs... False otherwise.
extern bool HDCP_Started;				// True if the HDCP enable bit has been set... False otherwise.

extern byte HDCP_LinkProtectionLevel;	// The most recently set link protection level from 0x29[7:6]

void HDCP_Init (void);

void HDCP_CheckStatus (byte InterruptStatusImage);

bool IsHDCP_Supported(void);

void HDCP_On(void);
void HDCP_Off(void);

#endif



#ifndef HDMI_OLD_VER
///////////////////////////////////
//   
//             new..
//
/////////////////////////////////
#ifdef READBKSV
static bool GetBKSV(void);
#endif

static bool AreAKSV_OK(void);
static bool CheckRevocationList(void);

static void RestartHDCP (void);
#endif //#ifdef HDMI_OLD_VER
///]]]]]]]]]]]]]]]]]

static struct fasync_struct *sky_hdmi_async_queue;

#if 1
static void sky_hdmi_work_f(struct work_struct *work);

static DECLARE_WORK(sky_hdmi_work, sky_hdmi_work_f);

int sky_hdmi_fasync(int fd, struct file *file, int on)
{
  int err;

  printk("%s::\n", __func__);

  err = fasync_helper(fd, file, on, &sky_hdmi_async_queue);
  if (err < 0)
  {
    return err;
      printk("%s:: >>>>>>>> FAIL !!!>>>>>>>>>>>\n", __func__);      
  }
  return 0;
}

static void sky_hdmi_work_f(struct work_struct *work)
{
    byte status;

    msm_hdmi_ReadByteTPI(TPI_INTERRUPT_STATUS, &status);

    printk("%s ,, status 0x%x \n", __FUNCTION__, status);

    //if(((status & HOT_PLUG_STATE) >> 2) == 0x00){
    if( ((status & HOT_PLUG_STATE) == 0x00) || ((status & RX_SENSE_STATE) == 0x00) ){
        kill_fasync(&sky_hdmi_async_queue, SIGIO, POLL_IN);
        set_connection_cable_flag(OFF); //ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½..
        printk("%s , stop hdmi. status 0x%x \n", __FUNCTION__, status);        
    }
    else{
        printk("%s , continue. status 0x%x \n", __FUNCTION__, status);
        ClearInterrupt(status);
    }

}
#endif

byte msm_hdmi_WriteBlockTPI(byte deviceID, byte regAddr, byte* Data, word length )
{  //lengthï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ loop ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½Ñ´ï¿½... 
#if 0
    int ret;  
    
    ret = sil9024a_i2c_write(regAddr, Data[0]);
    if(ret < 0)
    {
        HDMI_MSG("write fail!!!\n");
        return (byte)ret;
    }
    
    return 0;
#else
    int ret;  
    int i;

    for(i=0; i<length; i++, regAddr++){
        ret = sil9024a_i2c_write(deviceID, regAddr, Data[i]);
        if(ret < 0)
        {
            HDMI_MSG("write fail!!!\n");
            return (byte)ret;
        }
    }

    return 0;

#endif
}

word msm_hdmi_ReadDDCWord(byte deviceID, word regAddr)
{
   
    return 0;
}

byte msm_hdmi_ReadBlockTPI(byte deviceID, word regAddr, byte* pData, word length )
{ //lengthï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ loop ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½Ñ´ï¿½...

    int ret;  
    ret = sil9024a_i2c_read_len(deviceID, regAddr, pData, length);
    if(ret < 0)
    {
        HDMI_MSG("read fail!!!\n");
        return (byte)ret;
    } 
    return 0;
}

byte msm_hdmi_ReadSegBlockTPI(byte deviceID, byte segment, byte regAddr, byte* pData, word length )
{ //lengthï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ loop ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½Ñ´ï¿½...

    int ret;  
    ret = sil9024a_i2c_read_segment(deviceID, segment, regAddr, pData, length);
    if(ret < 0)
    {
        HDMI_MSG("read fail!!!\n");
        return (byte)ret;
    } 
    return 0;
}


byte msm_hdmi_ReadSegmentBlockTPI(byte deviceID, byte segment, byte offset, byte *buffer, word length)
{
    int ret;  
    byte dummy_data=1;
    ret = sil9024a_i2c_writeAddr(0x60, (unsigned short)segment, dummy_data);
    HDMI_MSG("%s :: ret %d\n", __func__, ret);
    
    ret = sil9024a_i2c_read_len(deviceID, offset, buffer, length);
    if(ret < 0)
    {
        HDMI_MSG("read fail!!!\n");
        return (byte)ret;
    } 
    return 0;
}

byte msm_hdmi_ReadBlockDDC(byte deviceID, word regAddr, byte* pData, word length )
{
  int ret;  

  ret = sil9024a_ddc_read_len(regAddr, pData, length);
  if(ret < 0)
  {
      HDMI_MSG("read fail!!!\n");
      return (byte)ret;
  } 
  return (byte)ret;    
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :	msm_hdmi_ReadByteTPI ()
//
// PURPOSE       :	Read one byte from a given offset of the TPI interface.
//
// INPUT PARAMS  :	Offset of TPI register to be read; A pointer to the variable
//					where the data read will be stored
//
// OUTPUT PARAMS :	Data - Contains the value read from the register value 
//					specified as the first input parameter
//
// GLOBALS USED  :	None
//
// RETURNS       :	TRUE if read successful. FALSE if failed.
//
//////////////////////////////////////////////////////////////////////////////
bool msm_hdmi_ReadByteTPI(byte RegOffset, byte *Data)
{
	byte Result;

	Result = msm_hdmi_ReadBlockTPI(TPI_BASE_ADDR, RegOffset, Data, 1); 
	
	if (Result)
		return FALSE;
	
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :	msm_hdmi_WriteByteTPI ()
//
// PURPOSE       :	Write one byte to a given offset of the TPI interface.
//
// INPUT PARAMS  :	Offset of TPI register to write to; value to be written to 
//					that TPI retister
//
// OUTPUT PARAMS : 
//
// GLOBALS USED  :	None
//
// RETURNS       :	void
//
//////////////////////////////////////////////////////////////////////////////
bool msm_hdmi_WriteByteTPI(byte RegOffset, byte Data)
{
	byte Result;
	Result =  msm_hdmi_WriteBlockTPI(TPI_BASE_ADDR, RegOffset, &Data, 1);	
	if (Result)
		return FALSE;		// writer failed

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :	msm_hdmi_ReadSetWriteTPI(byte Offset, byte Pattern)
//
// PURPOSE       :	Write "1" to all bits in TPI offset "Offset" that are set 
//					to "1" in "Pattern"; Leave all other bits in "Offset" 
//					unchanged.
//
// INPUT PARAMS  :	Offset	:	TPI register offset
//					Pattern	:	GPIO bits that need to be set
//
// OUTPUT PARAMS :	None
//
// GLOBALS USED  :	None
//
// RETURNS       :	void
//
//////////////////////////////////////////////////////////////////////////////

bool msm_hdmi_ReadSetWriteTPI(byte Offset, byte Pattern)
{

	byte Tmp;
	bool Result;

	Result = msm_hdmi_ReadByteTPI(Offset, &Tmp);
	if (!Result)
		return FALSE;				// read failed
	
	Tmp |= Pattern;
	Result = msm_hdmi_WriteByteTPI(Offset, Tmp);

	return Result;
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :	msm_hdmi_ReadClearWriteTPI(byte Offset, byte Pattern)
//
// PURPOSE       :	Write "0" to all bits in TPI offset "Offset" that are set 
//					to "1" in "Pattern"; Leave all other bits in "Offset" 
//					unchanged.
//
// INPUT PARAMS  :	Offset	:	TPI register offset
//					Pattern	:	"1" for each TPI register bit that needs to be 
//								cleared
//
// OUTPUT PARAMS :	None
//
// GLOBALS USED  :	None
//
// RETURNS       :	void
//
//////////////////////////////////////////////////////////////////////////////

bool msm_hdmi_ReadClearWriteTPI(byte Offset, byte Pattern)
{
	byte Tmp;
	bool Result;

	Result = msm_hdmi_ReadByteTPI(Offset, &Tmp);
	if (!Result)
		return FALSE;		// read failed

	Tmp &= ~Pattern;
	Result = msm_hdmi_WriteByteTPI(Offset, Tmp);

	if (!Result)
		return FALSE;

	return TRUE;
}




//********************************************************************
//
//
//                   NEW  Ver.  [[[[  
//
//
//
//*********************************************************************

byte sky_hdmi_I2C_ReadByte(byte deviceID, byte offset)
{
	byte Result;
    byte Data;

	Result = msm_hdmi_ReadBlockTPI(deviceID, offset, &Data, 1); 
	
	if (Result){
       HDMI_MSG("sky_hdmi_I2C_ReadByte fail!!!\n");
		return FALSE;
	}
	
	return Data;

}

bool sky_hdmi_I2C_WriteByte(byte deviceID, byte RegOffset, byte Data)
{
	byte Result;
	Result =  msm_hdmi_WriteBlockTPI(deviceID, RegOffset, &Data, 1);	
	if (Result)
		return FALSE;		// writer failed

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   sky_hdmi_ReadByteTPI ()
//
// PURPOSE      :   Read one byte from a given offset of the TPI interface.
//
// INPUT PARAMS :   Offset of TPI register to be read; A pointer to the variable
//                  where the data read will be stored
//
// OUTPUT PARAMS:   Data - Contains the value read from the register value
//                  specified as the first input parameter
//
// GLOBALS USED :   None
//
// RETURNS      :   TRUE
//
// NOTE         :   sky_hdmi_ReadByteTPI() is ported from the PC based FW to the uC
//                  version while retaining the same function interface. This
//                  will save the need to modify higher level I/O functions
//                  such as msm_hdmi_ReadSetWriteTPI(), msm_hdmi_ReadClearWriteTPI() etc.
//                  A dummy returned value (always TRUE) is provided for
//                  the same reason
//
//////////////////////////////////////////////////////////////////////////////
byte sky_hdmi_ReadByteTPI(byte RegOffset)
{
	byte Result;
    byte Data;

	Result = msm_hdmi_ReadBlockTPI(TPI_BASE_ADDR, RegOffset, &Data, 1); 
	
	if (Result){
       HDMI_MSG("sky_hdmi_ReadByteTPI fail!!!\n");
		return FALSE;
	}
	
	return Data;

}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION         :   ReadBlockTPI ()
//
// PURPOSE          :   Read NBytes from offset Addr of the TPI slave address
//                      into a byte Buffer pointed to by Data
//
// INPUT PARAMETERS :   TPI register offset, number of bytes to read and a
//                      pointer to the data buffer where the data read will be
//                      saved
//
// OUTPUT PARAMETERS:   pData - pointer to the buffer that will store the TPI
//                      block to be read
//
// RETURNED VALUE   :   VOID
//
// GLOBALS USED     :   None
//
////////////////////////////////////////////////////////////////////////////////
void sky_hdmi_ReadBlockTPI(byte TPI_Offset, word NBytes, byte * pData)
{
    msm_hdmi_ReadBlockTPI(TPI_BASE_ADDR, TPI_Offset, pData, NBytes);
}

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION         :   msm_hdmi_WriteBlockTPI ()
//
// PURPOSE          :   Write NBytes from a byte Buffer pointed to by Data to
//                      the TPI I2C slave starting at offset Addr
//
// INPUT PARAMETERS :   TPI register offset to start writing at, number of bytes
//                      to write and a pointer to the data buffer that contains
//                      the data to write
//
// OUTPUT PARAMETERS:   None.
//
// RETURNED VALUES  :   void
//
// GLOBALS USED     :   None
//
////////////////////////////////////////////////////////////////////////////////
void sky_hdmi_WriteBlockTPI(byte TPI_Offset, word NBytes, byte * pData)
{
    msm_hdmi_WriteBlockTPI(TPI_BASE_ADDR, TPI_Offset, pData, NBytes);
}

void sky_hdmi_set_vid_mode(sky_hdmi_vid_mod_type mode)
{
    switch (mode){
        case SKY_HDMI_480P_VID_MODE:
            sky_hdmi_vid_mode_f = 2;
            break;
        case SKY_HDMI_720P_VID_MODE:
            sky_hdmi_vid_mode_f = 4;
            break;
        default:
            printk("%s :: vid mode fail", __func__);
            break;
    }
            
}

byte sky_hdmi_get_vid_mode(void)
{
    return sky_hdmi_vid_mode_f;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      : HW_Reset()
//
// PURPOSE       : Send a
//
// INPUT PARAMS  : None
//
// OUTPUT PARAMS : void
//
// GLOBALS USED  : None
//
// RETURNS       : Void
//
//////////////////////////////////////////////////////////////////////////////

static void TxHW_Reset(void)
{

}


void TXHAL_InitPreReset (void)
{
    EDID_Data.HDMI_Sink = TRUE;
}


void TXHAL_InitPostReset (void)
{
    HDMI_MSG("TXHAL_InitPostReset \n");
    sky_hdmi_I2C_WriteByte(0x72, 0x82, 0x25);					// Terminations
    sky_hdmi_I2C_WriteByte(0x72, 0x08, 0x36);
}

byte ReadBackDoorRegister(byte PageNum, byte RegOffset) {
  
    HDMI_MSG("ReadBackDoorRegister \n");
    
    msm_hdmi_WriteByteTPI(TPI_INTERNAL_PAGE_REG, PageNum);		// Internal page
    msm_hdmi_WriteByteTPI(TPI_REGISTER_OFFSET_REG, RegOffset);	// Indexed register
    return sky_hdmi_ReadByteTPI(TPI_REGISTER_VALUE_REG); 		// Return read value
	}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      : StartTPI()
//
// PURPOSE       : Start HW TPI mode by writing 0x00 to TPI address 0xC7. This
//                 will take the Tx out of power down mode.
//
// INPUT PARAMS  : None
//
// OUTPUT PARAMS : void
//
// GLOBALS USED  : None
//
// RETURNS       : TRUE if HW TPI started successfully. FALSE if failed to.
//
//////////////////////////////////////////////////////////////////////////////
static bool StartTPI(void)
{
    byte devID = 0x00;
    word wID = 0x0000;

    //TPI_TRACE_PRINT((">>StartTPI()\n"));

    HDMI_MSG("StartTPI \n");

    msm_hdmi_WriteByteTPI(TPI_ENABLE, 0x00);            // Write "0" to 72:C7 to start HW TPI mode
    msleep(100);

    devID = ReadBackDoorRegister(0x00, 0x03);
    wID = devID;
    wID <<= 8;
    devID = ReadBackDoorRegister(0x00, 0x02);
    wID |= devID;

    devID = sky_hdmi_ReadByteTPI(TPI_DEVICE_ID);

    //printf ("0x%04X\n", (int) wID);

    if (devID == SiI_DEVICE_ID) {
        HDMI_MSG("SiI_DEVICE_ID %x \n", devID);
        return TRUE;
    }

    //printf ("Unsupported TX\n");
    return FALSE;
}


#ifdef DEV_SUPPORT_HDCP

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   HDCP_Init()
//
// PURPOSE      :   Tests Tx and Rx support of HDCP. If found, checks if
//                  and attempts to set the security level accordingly.
//
// INPUT PARAMS :   None
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :	HDCP_TxSupports - initialized to FALSE, set to TRUE if supported by this device
//					HDCP_Started - initialized to FALSE;
//					HDCP_LinkProtectionLevel - initialized to (EXTENDED_LINK_PROTECTION_NONE | LOCAL_LINK_PROTECTION_NONE);
//
// RETURNS      :   None
//
//////////////////////////////////////////////////////////////////////////////

void HDCP_Init (void) {

	//TPI_TRACE_PRINT((">>HDCP_Init()\n"));
   HDMI_MSG("HDCP_Init \n");


	HDCP_TxSupports = FALSE;
	HDCP_Started = FALSE;
	HDCP_LinkProtectionLevel = EXTENDED_LINK_PROTECTION_NONE | LOCAL_LINK_PROTECTION_NONE;

	// This is TX-related... need only be done once.
    if (!IsHDCP_Supported())
    {
		HDMI_MSG("HDCP -> TX does not support HDCP\n");
		return;
	}

	// This is TX-related... need only be done once.
    if (!AreAKSV_OK())
    {
        HDMI_MSG("HDCP -> Illegal AKSV\n");
        return;
    }

	// Both conditions above must pass or authentication will never be attempted. 
	//HDMI_MSG(("HDCP -> Supported by TX\n"));
	HDCP_TxSupports = TRUE;
}


void HDCP_CheckStatus (byte InterruptStatusImage) {

	byte QueryData;
	byte LinkStatus;
	byte RegImage;
	byte NewLinkProtectionLevel;

	if ((HDCP_TxSupports == TRUE) && (hdmiCableConnected == TRUE) && (dsRxPoweredUp == TRUE))
	{
		if ((HDCP_LinkProtectionLevel == (EXTENDED_LINK_PROTECTION_NONE | LOCAL_LINK_PROTECTION_NONE)) && (HDCP_Started == FALSE))
		{
			QueryData = sky_hdmi_ReadByteTPI(TPI_HDCP_QUERY_DATA_REG);

			if (QueryData & PROTECTION_TYPE_MASK)
			{
				HDCP_On();
			}
		}

		// Check if Link Status has changed:
		if (InterruptStatusImage & SECURITY_CHANGE_EVENT)
		{
			//HDMI_MSG (("HDCP -> "));

			LinkStatus = sky_hdmi_ReadByteTPI(TPI_HDCP_QUERY_DATA_REG);
			LinkStatus &= LINK_STATUS_MASK;

			ClearInterrupt(SECURITY_CHANGE_EVENT);

			switch (LinkStatus)
			{
				case LINK_STATUS_NORMAL:
					//HDMI_MSG (("Link = Normal\n"));
					break;

				case LINK_STATUS_LINK_LOST:
					//HDMI_MSG (("Link = Lost\n"));
					RestartHDCP();
					break;

				case LINK_STATUS_RENEGOTIATION_REQ:
					//HDMI_MSG (("Link = Renegotiation Required\n"));
					HDCP_Off();
					HDCP_On();
					break;

				case LINK_STATUS_LINK_SUSPENDED:
					//HDMI_MSG (("Link = Suspended\n"));
					HDCP_On();
					break;
			}
		}

		// Check if HDCP state has changed:
		if (InterruptStatusImage & HDCP_CHANGE_EVENT)
		{
			//HDMI_MSG (("HDCP -> "));

			RegImage = sky_hdmi_ReadByteTPI(TPI_HDCP_QUERY_DATA_REG);

			NewLinkProtectionLevel = RegImage & (EXTENDED_LINK_PROTECTION_MASK | LOCAL_LINK_PROTECTION_MASK);
			if (NewLinkProtectionLevel != HDCP_LinkProtectionLevel)
			{
				HDCP_LinkProtectionLevel = NewLinkProtectionLevel;
			}

			ClearInterrupt(HDCP_CHANGE_EVENT);

			switch (HDCP_LinkProtectionLevel)
			{
				case (EXTENDED_LINK_PROTECTION_NONE | LOCAL_LINK_PROTECTION_NONE):
					//HDMI_MSG (("Protection = None\n"));
					RestartHDCP();
					break;
				case LOCAL_LINK_PROTECTION_SECURE:
					ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, AV_MUTE_MASK, AV_MUTE_NORMAL);
					//HDMI_MSG (("Protection = Local, Video Unmuted\n"));
					break;
				case (EXTENDED_LINK_PROTECTION_SECURE | LOCAL_LINK_PROTECTION_SECURE):
					//HDMI_MSG (("Protection = Extended\n"));
					break;
				default:
					//HDMI_MSG (("Protection = Extended but not Local?\n"));
					RestartHDCP();
					break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   IsHDCP_Supported()
//
// PURPOSE      :   Check Tx revision number to find if this Tx supports HDCP
//                  by reading the HDCP revision number from TPI register 0x30.
//
// INPUT PARAMS :   None
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   TRUE if Tx supports HDCP. FALSE if not.
//
//////////////////////////////////////////////////////////////////////////////

bool IsHDCP_Supported(void)
{
    byte HDCP_Rev;
	bool HDCP_Supported;

	//TPI_TRACE_PRINT((">>IsHDCP_Supported()\n"));

	HDCP_Supported = TRUE;

	// Check Device ID
    HDCP_Rev = sky_hdmi_ReadByteTPI(TPI_HDCP_REVISION_DATA_REG);

    if (HDCP_Rev != (HDCP_MAJOR_REVISION_VALUE | HDCP_MINOR_REVISION_VALUE))
	{
    	HDCP_Supported = FALSE;
	}

#ifdef SiI_9022AYBT_DEVICEID_CHECK
	// Even if HDCP is supported check for incorrect Device ID
	HDCP_Rev = sky_hdmi_ReadByteTPI(TPI_AKSV_1);
	if (HDCP_Rev == 0x90)
	{
		HDCP_Rev = sky_hdmi_ReadByteTPI(TPI_AKSV_2);
		if (HDCP_Rev == 0x22)
		{
			HDCP_Rev = sky_hdmi_ReadByteTPI(TPI_AKSV_3);
			if (HDCP_Rev == 0xA0)
			{
				HDCP_Rev = sky_hdmi_ReadByteTPI(TPI_AKSV_4);
				if (HDCP_Rev == 0x00)
				{
					HDCP_Rev = sky_hdmi_ReadByteTPI(TPI_AKSV_5);
					if (HDCP_Rev == 0x00)
					{
						HDCP_Supported = FALSE;
					}
				}
			}
		}
	}
#endif
	return HDCP_Supported;;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   HDCP_On()
//
// PURPOSE      :   Switch hdcp on.
//
// INPUT PARAMS :   None
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   HDCP_Started set to TRUE
//
// RETURNS      :   None
//
//////////////////////////////////////////////////////////////////////////////

void HDCP_On(void)
{
	//TPI_TRACE_PRINT((">>HDCP_On()\n"));
	//HDMI_MSG(("HDCP Started\n"));

	msm_hdmi_WriteByteTPI(TPI_HDCP_CONTROL_DATA_REG, PROTECTION_LEVEL_MAX);

	HDCP_Started = TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   HDCP_Off()
//
// PURPOSE      :   Switch hdcp off.
//
// INPUT PARAMS :   None
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   HDCP_Started set to FALSE
//
// RETURNS      :   None
//
//////////////////////////////////////////////////////////////////////////////

void HDCP_Off(void)
{
	//TPI_TRACE_PRINT((">>HDCP_Off()\n"));
	//HDMI_MSG(("HDCP Stopped\n"));

	//ReadModifyWriteTPI(TPI_SYSTEM_CONTROL, 0x08, 0x08); // AV MUTE enable
	msm_hdmi_WriteByteTPI(TPI_HDCP_CONTROL_DATA_REG, PROTECTION_LEVEL_MIN);

	HDCP_Started = FALSE;
}


void RestartHDCP (void)
{
	//HDMI_MSG (("HDCP -> Restart\n"));

	DisableTMDS();
	mdelay(10);
	EnableTMDS();

	HDCP_Off();
}



//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   GetBKSV()
//
// PURPOSE      :   Collect all downstrean KSV for verification
//
// INPUT PARAMS :   None
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   BKSV_Array[]
//
// RETURNS      :   TRUE if BKSVs collected successfully. False if not.
//
//////////////////////////////////////////////////////////////////////////////
#ifdef READBKSV
static bool GetBKSV(void)
{
    byte KeyCount;
    byte BKSV_Array[BKSV_ARRAY_SIZE];

	//TPI_TRACE_PRINT((">>GetBKSV()\n"));
    ReadBlockHDCP(DDC_BSTATUS_ADDR_L, 2, &KeyCount);    // Read the number of BKSV's from HDCP port
    KeyCount &= DEVICE_COUNT_MASK;
	if (KeyCount != 0)
	{
		KeyCount *= 5; // 1 KSV is 5 bytes
		ReadBlockHDCP(DDC_BKSV_ADDR, KeyCount, BKSV_Array);
	}

    return TRUE;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  AreAKSV_OK()
//
// PURPOSE       :  Check if AKSVs contain 20 '0' and 20 '1'
//
// INPUT PARAMS  :  None
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  TBD
//
// RETURNS       :  TRUE if 20 zeros and 20 ones found in AKSV. FALSE OTHERWISE
//
//////////////////////////////////////////////////////////////////////////////
static bool AreAKSV_OK(void)
{
    byte B_Data[AKSV_SIZE];
    byte NumOfOnes = 0;

    byte i;
    byte j;

	//TPI_TRACE_PRINT((">>AreAKSV_OK()\n"));

    sky_hdmi_ReadBlockTPI(TPI_AKSV_1, AKSV_SIZE, B_Data);

    for (i=0; i < AKSV_SIZE; i++)
    {
        for (j=0; j < BYTE_SIZE; j++)
        {
            if (B_Data[i] & 0x01)
            {
                NumOfOnes++;
            }
            B_Data[i] >>= 1;
        }
     }
     if (NumOfOnes != NUM_OF_ONES_IN_KSV)
        return FALSE;

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  CheckRevocationList()
//
// PURPOSE       :  Compare KSVs to those included in a revocation list
//
// INPUT PARAMS  :  None
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  TBD
//
// RETURNS       :  TRUE if no illegal KSV found in BKSV list
//
// NOTE			 :	Currently this function is implemented as a place holder only
//
//////////////////////////////////////////////////////////////////////////////

static bool CheckRevocationList(void)
{
	//TPI_TRACE_PRINT((">>CheckRevocationList()\n"));
    return TRUE;            // A DUMMY FUNCTION BODY. TO BE FIXED WHEN A REVOCATION LIST IS DEFINED.
}

#endif // #ifdef DEV_SUPPORT_HDCP


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      : TPI_Init()
//
// PURPOSE       : TPI initialization: HW Reset, Interrupt enable.
//
// INPUT PARAMS  : None
//
// OUTPUT PARAMS : void
//
// GLOBALS USED  : None
//
// RETURNS      :   TRUE
//
//////////////////////////////////////////////////////////////////////////////
int TPI_Init(void)
{
    HDMI_MSG("TPI_Init \n");
#ifdef DEV_SUPPORT_EDID
    edidDataValid = FALSE;					// Move this into EDID_Init();
#endif
    EDID_Data.HDMI_Sink = FALSE;
    tmdsPoweredUp = FALSE;
    IDX_InChar = 0;
    NumOfArgs = 0;
    F_SBUF_DataReady = FALSE;
    F_CollectingData = FALSE;
    hdmiCableConnected = 0;
    dsRxPoweredUp = 0;

    TXHAL_InitPostReset();
    sky_hdmi_I2C_WriteByte(0x72, 0x7C, 0x14);		// HW debounce to 64ms (0x14)

    if (StartTPI()){     // Enable HW TPI mode, check device ID
        return HDMI_SUCCESS_E;
    }

    return HDMI_INIT_FAIL_E;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  EnableInterrupts()
//
// PURPOSE       :  Enable the interrupts specified in the input parameter
//
// INPUT PARAMS  :  A bit pattern with "1" for each interrupt that needs to be
//                  set in the Interrupt Enable Register (TPI offset 0x3C)
//
// OUTPUT PARAMS :  void
//
// GLOBALS USED  :  None
//
// RETURNS       :  TRUE
//
//////////////////////////////////////////////////////////////////////////////
bool EnableInterrupts(byte Interrupt_Pattern)
{
	//TPI_TRACE_PRINT((">>EnableInterrupts()\n"));
    msm_hdmi_ReadSetWriteTPI(TPI_INTERRUPT_EN, Interrupt_Pattern);

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  DisableInterrupts()
//
// PURPOSE       :  Enable the interrupts specified in the input parameter
//
// INPUT PARAMS  :  A bit pattern with "1" for each interrupt that needs to be
//                  cleared in the Interrupt Enable Register (TPI offset 0x3C)
//
// OUTPUT PARAMS :  void
//
// GLOBALS USED  :  None
//
// RETURNS       :  TRUE
//
//////////////////////////////////////////////////////////////////////////////
static bool DisableInterrupts(byte Interrupt_Pattern)
{
	//TPI_TRACE_PRINT((">>DisableInterrupts()\n"));
    msm_hdmi_ReadClearWriteTPI(TPI_INTERRUPT_EN, Interrupt_Pattern);

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      : ConvertVIC_To_VM_Index()
//
// PURPOSE       :  Convert Video Identification Code to the corresponding
//                  index of VModesTable[]. This is needed since most of
//                  VModesTable[] 861-D compatible entries cover more than
//                  one code.
//
// INPUT PARAMS  : VIC to be converted
//
// OUTPUT PARAMS : None
//
// GLOBALS USED  : VModesTable[]
//
// RETURNS       : Index into VModesTable[] corrsponding to VIC
//
// Note         : Conversion is needed for 861-D formats only.
//
//////////////////////////////////////////////////////////////////////////////
byte ConvertVIC_To_VM_Index(byte VIC)
{
    byte ConvertedVIC;

    const byte VIC2Index[] = {
                                0,  0,  1,  1,  2,  3,  4,  4,  5,  5,
                                7,  7,  8,  8, 10, 10, 11, 12, 12, 13,
                               14, 15, 15, 16, 16, 19, 19, 20, 20, 23,
                               23, 24, 25, 26, 27, 28, 28, 29, 29, 30,
                               31, 32, 33, 33, 34, 34, 35, 36, 37, 37,
                               38, 38, 39, 39, 40, 40, 41, 41, 42, 42
                            };
    VIC &= HDMI_SEVEN_LSBITS;                // MSBit indicates if "native" (CEA-861-D, Table 33)

    if (VIC < PC_BASE)                  // 861-D mode - return VModesTable[] index
        ConvertedVIC = VIC2Index[VIC];
    else
        ConvertedVIC = VIC;

    return ConvertedVIC;
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   SetDE(V_Mode)
//
// PURPOSE      :   Set the 9022/4 internal DE generator parameters
//
// INPUT PARAMS :   Index of video mode to set
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   DE_SET_OK
//
// NOTE         :   0x60[7] must be set to "0" for the follwing settings to
//                  take effect
//
//////////////////////////////////////////////////////////////////////////////
byte SetDE(byte V_Mode)
{
    byte RegValue;

    word H_StartPos;
    word V_StartPos;
    word H_Res;
    word V_Res;

    byte Polarity;
    byte B_Data[8];

    HDMI_MSG(">>SetDE()\n");

    // Make sure that External Sync method is set before enableing the DE Generator:
    RegValue = sky_hdmi_ReadByteTPI(TPI_SYNC_GEN_CTRL);
//MessageIndex     if (!Result)
//MessageIndex         EnqueueMessageIndex(TPI_READ_FAILURE);

    if (RegValue & HDMI_BIT_7)
    {
//MessageIndex         EnqueueMessageIndex(DE_CANNOT_BE_SET_WITH_EMBEDDED_SYNC);
        return DE_CANNOT_BE_SET_WITH_EMBEDDED_SYNC;
    }

    V_Mode = ConvertVIC_To_VM_Index(V_Mode);                // convert 861-D VIC into VModesTable[] index

    H_StartPos = VModesTable[V_Mode].Pos.H;
    V_StartPos = VModesTable[V_Mode].Pos.V;

    Polarity = (~VModesTable[V_Mode].Tag.RefrTypeVHPol) & HDMI_TWO_LSBITS;

    H_Res = VModesTable[V_Mode].Res.H;

	if ((VModesTable[V_Mode].Tag.RefrTypeVHPol & 0x04))
	{
    	V_Res = (VModesTable[V_Mode].Res.V) >> 1;
	}
	else
	{
		V_Res = (VModesTable[V_Mode].Res.V);
	}

    B_Data[0] = H_StartPos & HDMI_LOW_BYTE;              // 8 LSB of DE DLY in 0x62

    B_Data[1] = (H_StartPos >> 8) & HDMI_TWO_LSBITS;     // 2 MSBits of DE DLY to 0x63
    B_Data[1] |= (Polarity << 4);                   // V and H polarity
    B_Data[1] |= BIT_EN_DE_GEN;                     // enable DE generator

    B_Data[2] = V_StartPos & HDMI_SEVEN_LSBITS;      // DE_TOP in 0x64
    B_Data[3] = 0x00;                           // 0x65 is reserved
    B_Data[4] = H_Res & HDMI_LOW_BYTE;               // 8 LSBits of DE_CNT in 0x66
    B_Data[5] = (H_Res >> 8) & HDMI_LOW_NIBBLE;      // 4 MSBits of DE_CNT in 0x67
    B_Data[6] = V_Res & HDMI_LOW_BYTE;               // 8 LSBits of DE_LIN in 0x68
    B_Data[7] = (V_Res >> 8) & HDMI_THREE_LSBITS;    // 3 MSBits of DE_LIN in 0x69

    sky_hdmi_WriteBlockTPI(TPI_DE_DLY, 8, &B_Data[0]);

//MessageIndex     EnqueueMessageIndex(DE_SET_OK);                   // For GetStatus()
    return DE_SET_OK;                               // Write completed successfully
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  sky_hdmi_InitVideo()
//
// PURPOSE       :  Set the 9022/4 to the video mode determined by GetVideoMode()
//
// INPUT PARAMS  :  Index of video mode to set; Flag that distinguishes between
//                  calling this function after power up and after input
//                  resolution change
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  VModesTable, VideoCommandImage
//
// RETURNS       :  TRUE
//
//////////////////////////////////////////////////////////////////////////////
bool sky_hdmi_InitVideo(byte Mode, byte TclkSel, bool Init)
{
    byte V_Mode;
#ifdef DEEP_COLOR
        byte temp;
#endif
    //word W_Data[4];
    byte B_Data[8];
#ifdef USE_DE_GENERATOR
    byte Status;
#endif

#ifndef PLL_BACKDOOR                                    // Use BackDoor for 9022/24 for video clock multiplier
    byte Pattern;
#endif

    HDMI_MSG("sky_hdmi_InitVideo : mode : %x()\n", Mode);

    V_Mode = ConvertVIC_To_VM_Index(Mode);              // convert 861-D VIC into VModesTable[] index

#ifdef PLL_BACKDOOR                                   // Use BackDoor for 9022/24 for video clock multiplier
    SetPLL(TclkSel);
#else
    Pattern = (TclkSel << 6) & HDMI_TWO_MSBITS;              // Use TPI 0x08[7:6] for 9022A/24A video clock multiplier
    msm_hdmi_ReadSetWriteTPI(TPI_PIX_REPETITION, Pattern);
#endif

    // Take values from VModesTable[]:
    //W_Data[0] = VModesTable[V_Mode].PixClk;             // write Pixel clock to TPI registers 0x00, 0x01
    //W_Data[1] = VModesTable[V_Mode].Tag.VFreq;          // write Vertical Frequency to TPI registers 0x02, 0x03
    //W_Data[2] = VModesTable[V_Mode].Tag.Total.Pixels;   // write total number of pixels to TPI registers 0x04, 0x05
    //W_Data[3] = VModesTable[V_Mode].Tag.Total.Lines;    // write total number of lines to TPI registers 0x06, 0x07

    B_Data[0] = VModesTable[V_Mode].PixClk & 0x00FF;             // write Pixel clock to TPI registers 0x00, 0x01
    B_Data[1] = (VModesTable[V_Mode].PixClk >> 8) & 0xFF;

    B_Data[2] = VModesTable[V_Mode].Tag.VFreq & 0x00FF;          // write Vertical Frequency to TPI registers 0x02, 0x03
    B_Data[3] = (VModesTable[V_Mode].Tag.VFreq >> 8) & 0xFF;

    B_Data[4] = VModesTable[V_Mode].Tag.Total.Pixels & 0x00FF;   // write total number of pixels to TPI registers 0x04, 0x05
    B_Data[5] = (VModesTable[V_Mode].Tag.Total.Pixels >> 8) & 0xFF;

    B_Data[6] = VModesTable[V_Mode].Tag.Total.Lines & 0x00FF;    // write total number of lines to TPI registers 0x06, 0x07
    B_Data[7] = (VModesTable[V_Mode].Tag.Total.Lines >> 8) & 0xFF;

    //Result=TxTPI_WriteBlock(TPI_PIX_CLK_LSB, 8, (byte *)(&W_Data[0]));  // Write TPI Mode data in one burst (8 bytes);
    sky_hdmi_WriteBlockTPI(TPI_PIX_CLK_LSB, 8, B_Data);  // Write TPI Mode data in one burst (8 bytes);

#ifdef USE_DE_GENERATOR
    msm_hdmi_ReadClearWriteTPI(TPI_SYNC_GEN_CTRL, HDMI_MSBIT);       // set 0x60[7] = 0 for External Sync
    Status = SetDE(Mode);                              // Call SetDE() with Video Mode as a parameter
    //if (Status != DE_SET_OK)
    //{
    //  return Status;
    //}
        msm_hdmi_ReadSetWriteTPI(TPI_DE_CTRL, HDMI_BIT_6);    // set 0x63[6] = 1 for DE
#endif

#ifdef DEEP_COLOR
        switch (video_information.color_depth)
        {
                case 0: temp = 0x00; break;
                case 1: temp = 0x80; break;
                case 2: temp = 0xC0; break;
                case 3: temp = 0x40; break;
                default: temp = 0x00; break;
        }
        B_Data[0] = sky_hdmi_ReadByteTPI(0x09);
        B_Data[0] = ((B_Data[0] & 0x3F) | temp);
//      //printf("\nDEEP COLOR = %d  TPI 0x09 = %d\n", (int) video_information.color_depth, (int) B_Data[0]);
#endif
    B_Data[1] = (BITS_OUT_RGB | BITS_OUT_AUTO_RANGE) & ~BIT_BT_709;
#ifdef DEEP_COLOR
        B_Data[1] = ((B_Data[1] & 0x3F) | temp);
#endif

#ifdef DEV_SUPPORT_EDID
	// Set YCbCr color space depending on EDID
	if (EDID_Data.YCbCr_4_4_4)
	{
		B_Data[1] = ((B_Data[1] & 0xFC) | 0x01); 
	}
	else
	{
		if (EDID_Data.YCbCr_4_2_2)
		{
			B_Data[1] = ((B_Data[1] & 0xFC) | 0x02);
		}
	}
#else
	B_Data[1] = 0x00;
#endif

    sky_hdmi_WriteBlockTPI(TPI_INPUT_FORMAT, 2, B_Data);

#if 1 //for test... 

    if (Init)
    {
        B_Data[0] = (VModesTable[V_Mode].PixRep) & HDMI_LOW_BYTE;        // Set pixel replication field of 0x08
        B_Data[0] |= BIT_BUS_24;                                    // Set 24 bit bus

        #ifndef PLL_BACKDOOR
        B_Data[0] |= (TclkSel << 6) & HDMI_TWO_MSBITS;
        #endif

#ifdef CLOCK_EDGE_FALLING
        B_Data[0] &= ~BIT_EDGE_RISE;                                // Set to falling edge
#endif

#ifdef CLOCK_EDGE_RISING
                B_Data[0] |= BIT_EDGE_RISE;                                                                     // Set to rising edge
#endif

        msm_hdmi_WriteByteTPI(TPI_PIX_REPETITION, B_Data[0]);       // 0x08

        // default to full range RGB at the input:
#ifndef USE_DE_GENERATOR
        B_Data[0] = (((BITS_IN_RGB | BITS_IN_AUTO_RANGE) & ~BIT_EN_DITHER_10_8) & ~BIT_EXTENDED_MODE);  // 0x09
#else
        B_Data[0] = (((BITS_IN_YCBCR422 | BITS_IN_AUTO_RANGE) & ~BIT_EN_DITHER_10_8) & ~BIT_EXTENDED_MODE);  // 0x09
#endif

#ifdef DEEP_COLOR
                switch (video_information.color_depth)
                {
                        case 0: temp = 0x00; break;
                        case 1: temp = 0x80; break;
                        case 2: temp = 0xC0; break;
                        case 3: temp = 0x40; break;
                        default: temp = 0x00; break;
                }
                B_Data[0] = ((B_Data[0] & 0x3F) | temp);
//              //printf("\nInit DEEP COLOR = %d  TPI 0x09 = %d\n", (int) video_information.color_depth, (int) B_Data[0]);
#endif
        B_Data[1] = (BITS_OUT_RGB | BITS_OUT_AUTO_RANGE) & ~BIT_BT_709;
#ifdef DEEP_COLOR
                B_Data[1] = ((B_Data[1] & 0x3F) | temp);
#endif

#ifdef DEV_SUPPORT_EDID
	// Set YCbCr color space depending on EDID
	if (EDID_Data.YCbCr_4_4_4)
	{
		B_Data[1] = ((B_Data[1] & 0xFC) | 0x01); 
	}
	else
	{
		if (EDID_Data.YCbCr_4_2_2)
		{
			B_Data[1] = ((B_Data[1] & 0xFC) | 0x02);
		}
	}
#else
	B_Data[1] = 0x00;
#endif

        sky_hdmi_WriteBlockTPI(TPI_INPUT_FORMAT, 2, B_Data);

        // Set external sync mode and disable DE generator
        // Otherwise, they will be set by the calling function.
        
		// Number HSync pulses from VSync active edge to Video Data Period should be 20 (VS_TO_VIDEO)
		msm_hdmi_WriteByteTPI(TPI_SYNC_GEN_CTRL, 0x00);     // Default to External Sync mode + disable VSync adjustment

#ifndef USE_DE_GENERATOR
        msm_hdmi_WriteByteTPI(TPI_DE_CTRL, 0x00);           // Disable DE generator by default
#endif
    }
#endif

#ifdef SOURCE_TERMINATION_ON
        V_Mode = ReadBackDoorRegister(INTERNAL_PAGE_1, TMDS_CONT_REG);
        V_Mode = (V_Mode & 0x3F) | 0x25;
        WriteBackDoorRegister(INTERNAL_PAGE_1, TMDS_CONT_REG, V_Mode);
#endif

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  ReadModifyWriteTPI(byte Offset, byte Mask, byte Value)
//
// PURPOSE       :  Write "Value" to all bits in TPI offset "Offset" that are set
//                  to "1" in "Mask"; Leave all other bits in "Offset"
//                  unchanged.
//
// INPUT PARAMS  :  Offset  :   TPI register offset
//                  Mask    :   "1" for each TPI register bit that needs to be
//                              modified
//					Value   :   The desired value for the register bits in their
//								proper positions
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  void
//
//////////////////////////////////////////////////////////////////////////////
void ReadModifyWriteTPI(byte Offset, byte Mask, byte Value)
{
    byte Tmp;

    Tmp = sky_hdmi_ReadByteTPI(Offset);
    Tmp &= ~Mask;
	Tmp |= (Value & Mask);
    msm_hdmi_WriteByteTPI(Offset, Tmp);
}


static void TxPowerState(byte powerState) 
{
   HDMI_MSG("TX Power State D%d\n", (int)powerState);
	ReadModifyWriteTPI(TPI_DEVICE_POWER_STATE_CTRL_REG, TX_POWER_STATE_MASK, powerState & TX_POWER_STATE_MASK);
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  SetAVI_InfoFrames()
//
// PURPOSE       :  Load AVI InfoFrame data into registers and send to sink
//
// INPUT PARAMS  :  An API_Cmd parameter that holds the data to be sent
//                  in the InfoFrames
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  TRUE
//
//////////////////////////////////////////////////////////////////////////////
bool SetAVI_InfoFrames(API_Cmd Command)
{
    byte B_Data[SIZE_AVI_INFOFRAME];
    byte VideoMode;                     // pointer to VModesTable[]
    byte i;
    byte TmpVal;

        
    HDMI_MSG(">>SetAVI_InfoFrames()\n");

    for (i = 0; i < SIZE_AVI_INFOFRAME; i++)
        B_Data[i] = 0;

    if (Command.CommandLength > VIDEO_SETUP_CMD_LEN)    // Command length > 7. AVI InfoFrame is set by the host
    {
        for (i = 1; i < Command.CommandLength - VIDEO_SETUP_CMD_LEN; i++)
            B_Data[i] = Command.Arg[VIDEO_SETUP_CMD_LEN + i - 1];
    }
    else                                                // Command length == 7. AVI InfoFrame is set by the FW
    {
        if ((Command.Arg[3] & HDMI_TWO_LSBITS) == 1)         // AVI InfoFrame DByte1
            TmpVal = 2;
        else if ((Command.Arg[3] & HDMI_TWO_LSBITS) == 2)
            TmpVal = 1;
        else
            TmpVal = 0;

            B_Data[1] = (TmpVal << 5) & BITS_OUT_FORMAT;                    // AVI Byte1: Y1Y0 (output format)

            if (((Command.Arg[6] >> 2) & HDMI_TWO_LSBITS) == 3)                  // Extended colorimetry - xvYCC
            {
                B_Data[2] = 0xC0;                                           // Extended colorimetry info (B_Data[3] valid (CEA-861D, Table 11)

                if (((Command.Arg[6] >> 4) & HDMI_THREE_LSBITS) == 0)            // xvYCC601
                    B_Data[3] &= ~BITS_6_5_4;

                else if (((Command.Arg[6] >> 4) & HDMI_THREE_LSBITS) == 1)       // xvYCC709
                    B_Data[3] = (B_Data[3] & ~BITS_6_5_4) | HDMI_BIT_4;
            }

            else if (((Command.Arg[6] >> 2) & HDMI_TWO_LSBITS) == 2)             // BT.709
                B_Data[2] = 0x80;                                           // AVI Byte2: C1C0

            else if (((Command.Arg[6] >> 2) & HDMI_TWO_LSBITS) == 1)             // BT.601
                B_Data[2] = 0x40;                                           // AVI Byte2: C1C0

            VideoMode = ConvertVIC_To_VM_Index(Command.Arg[0]);

            // Obtain the aspect ratio from the RX side
#if 0   //SKY Block.. this is for RX Test Board..         
            i = sky_hdmi_I2C_ReadByte(0x68, 0x45);
            i = (i & 0x30) >> 4;
#else
            i = 0;
#endif
            //if (AspectRatioTable[VideoMode] == _4)
#if 0            
            if (i == 0)
                B_Data[2] |= _4_To_3;
            else                                                            // AVI Byte2: M1M0
                B_Data[2] |= _16_To_9;
#else
            if(2 == sky_hdmi_get_vid_mode())
                B_Data[2] |= _4_To_3;
            else                                                            // AVI Byte2: M1M0
                B_Data[2] |= _16_To_9;

#endif
            B_Data[2] |= SAME_AS_AR;                                        // AVI Byte2: R3..R1

            B_Data[4] = Command.Arg[0] & HDMI_SEVEN_LSBITS;                      // AVI Byte4: VIC
            B_Data[5] = VModesTable[VideoMode].PixRep;                      // AVI Byte5: Pixel Replication - PR3..PR0
    }

    B_Data[0] = 0x82 + 0x02 +0x0D;                                          // AVI InfoFrame ChecKsum

    for (i = 1; i < SIZE_AVI_INFOFRAME; i++)
        B_Data[0] += B_Data[i];

    B_Data[0] = 0x100 - B_Data[0];

    sky_hdmi_WriteBlockTPI(TPI_AVI_BYTE_0, SIZE_AVI_INFOFRAME, B_Data);

    return TRUE;
}



//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      : SetPreliminaryInfoFrames()
//
// PURPOSE       : Set InfoFrames for default (initial) setup only
//
// INPUT PARAMS  : VIC, output mode,
//
// OUTPUT PARAMS : void
//
// GLOBALS USED  : None
//
// RETURNS       : TRUE
//
//////////////////////////////////////////////////////////////////////////////
static bool SetPreliminaryInfoFrames(byte V_Mode)
{
    byte i;
    API_Cmd Command;        // to allow using function SetAVI_InfoFrames()

	HDMI_MSG(">>SetPreliminaryInfoFrames()\n");

    for (i = 0; i < MAX_COMMAND_ARGUMENTS; i++)
        Command.Arg[i] = 0;

	Command.CommandLength = 0;	// fixes SetAVI_InfoFrames() from going into an infinite loop

    Command.Arg[0] = V_Mode;

#ifdef DEV_SUPPORT_EDID
//#if 0
	if (EDID_Data.YCbCr_4_4_4)
	{
		Command.Arg[3] = 0x01;
	}
	else
	{
		if (EDID_Data.YCbCr_4_2_2)
		{
			Command.Arg[3] = 0x02;
		}
	}
#else
	Command.Arg[3] = 0x00;
#endif

    SetAVI_InfoFrames(Command);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   SetChannelLayout()
//
// PURPOSE      :   Set up the Channel layout field of internal register 0x2F
//                  (0x2F[1])
//
// INPUT PARAMS :   Number of audio channels: "0 for 2-Channels ."1" for 8.
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   TRUE 
//
//////////////////////////////////////////////////////////////////////////////
bool SetChannelLayout(byte Count)
{
    // Use backdoor to set 0x7A:0x2F[1]:
    msm_hdmi_WriteByteTPI(TPI_INTERNAL_PAGE_REG, 0x02); // Internal page 2
    msm_hdmi_WriteByteTPI(TPI_REGISTER_OFFSET_REG, 0x2F);

    Count &= HDMI_THREE_LSBITS;

    if (Count == TWO_CHANNEL_LAYOUT)
    {
        // Clear 0x2F:
        msm_hdmi_ReadClearWriteTPI(TPI_REGISTER_VALUE_REG, HDMI_BIT_1);
    }

    else if (Count == EIGHT_CHANNEL_LAYOUT)
    {
        // Set 0x2F[0]:
        msm_hdmi_ReadSetWriteTPI(TPI_REGISTER_VALUE_REG, HDMI_BIT_1);
    }

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  SetAudioInfoFrames()
//
// PURPOSE       :  Load Audio InfoFrame data into registers and send to sink
//
// INPUT PARAMS  :  (1) Channel count (2) speaker configuration per CEA-861D
//                  Tables 19, 20 (3) Coding type: 0x09 for DSD Audio. 0 (refer
//                                      to stream header) for all the rest (4) Sample Frequency. Non
//                                      zero for HBR only (5) Audio Sample Length. Non zero for HBR
//                                      only.
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  TRUE
//
//////////////////////////////////////////////////////////////////////////////
bool SetAudioInfoFrames(byte ChannelCount, byte SpeakerConfig, byte CodingType, byte Fs, byte SS)
{
    byte B_Data[SIZE_AUDIO_INFOFRAME];  // 14
    byte i;

    //TPI_TRACE_PRINT((">>SetAudioInfoFrames()\n"));

    for (i = 0; i < SIZE_AUDIO_INFOFRAME +1; i++)
        B_Data[i] = 0;

    B_Data[0] = EN_AUDIO_INFOFRAMES;        // 0xC2
    B_Data[1] = TYPE_AUDIO_INFOFRAMES;      // 0x84
    B_Data[2] = AUDIO_INFOFRAMES_VERSION;   // 0x01
    B_Data[3] = AUDIO_INFOFRAMES_LENGTH;    // 0x0A

    B_Data[5] = ChannelCount;               // 0 for "Refer to Stream Header" or for 2 Channels. 0x07 for 8 Channels
        B_Data[5] |= (CodingType << 4);                 // 0xC7[7:4] == 0b1001 for DSD Audio
    B_Data[4] = 0x84 + 0x01 + 0x0A;         // Calculate checksum

    if (Fs)                                                                 // HBR
    	B_Data[6] = Fs | SS;

    B_Data[8] = SpeakerConfig;

    for (i = 5; i < SIZE_AUDIO_INFOFRAME; i++)
        B_Data[4] += B_Data[i];

    B_Data[4] = 0x100 - B_Data[4];

    sky_hdmi_WriteBlockTPI(TPI_AUDIO_BYTE_0, SIZE_AUDIO_INFOFRAME, B_Data);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      : SetBasicAudio()
//
// PURPOSE       : Set the 9022/4 audio interface to basic audio.
//
// INPUT PARAMS  : None
//
// OUTPUT PARAMS : None
//
// GLOBALS USED  : None
//
// RETURNS       : void.
//
//////////////////////////////////////////////////////////////////////////////
void SetBasicAudio(void)
{

    HDMI_MSG(">>SetBasicAudio()\n");

#ifdef I2S_AUDIO
    msm_hdmi_WriteByteTPI(TPI_AUDIO_INTERFACE,  AUD_IF_I2S);                             // 0x26
    msm_hdmi_WriteByteTPI(TPI_AUDIO_HANDLING, 0x08 | AUD_DO_NOT_CHECK);          // 0x25
#else
    msm_hdmi_WriteByteTPI(TPI_AUDIO_INTERFACE, AUD_IF_SPDIF);                    // 0x26 = 0x40
    msm_hdmi_WriteByteTPI(TPI_AUDIO_HANDLING, AUD_PASS_BASIC);                   // 0x25 = 0x00
#endif

#ifndef F_9022A_9334                                    // Use BackDoor for 9022/24 for Channel layout
            SetChannelLayout(TWO_CHANNELS);             // Always 2 channesl in S/PDIF
#else
            msm_hdmi_ReadClearWriteTPI(TPI_AUDIO_INTERFACE, HDMI_BIT_5); // Use TPI 0x26[5] for 9022A/24A and 9334 channel layout
#endif

#ifdef I2S_AUDIO
        // I2S - Map channels - replace with call to API MAPI2S
        msm_hdmi_WriteByteTPI(TPI_I2S_EN, 0x80); // 0x1F
        msm_hdmi_WriteByteTPI(TPI_I2S_EN, 0x91);
        msm_hdmi_WriteByteTPI(TPI_I2S_EN, 0xA2);
        msm_hdmi_WriteByteTPI(TPI_I2S_EN, 0xB3);

        // I2S - Stream Header Settings - replace with call to API SetI2S_StreamHeader
        msm_hdmi_WriteByteTPI(TPI_I2S_CHST_0, 0x00); // 0x21
        msm_hdmi_WriteByteTPI(TPI_I2S_CHST_1, 0x00);
        msm_hdmi_WriteByteTPI(TPI_I2S_CHST_2, 0x00);
        msm_hdmi_WriteByteTPI(TPI_I2S_CHST_3, 0x02);

        // I2S - Input Configuration - replace with call to API ConfigI2SInput
        msm_hdmi_WriteByteTPI(TPI_I2S_IN_CFG, 0x10); // 0x20
#endif

    msm_hdmi_WriteByteTPI(TPI_AUDIO_SAMPLE_CTRL, TWO_CHANNELS);  // 0x27 = 0x01
    SetAudioInfoFrames(TWO_CHANNELS, 0x00, 0x00, 0x00, 0x00);
}

#ifdef SKY_HDMI_AUDIO_I2S

byte Set_I2S_AudioMode(void) //brown 090930_00 I2S function
{
	bool Result;
	
	byte AudioInterface = 0x00;
	byte AudioSampleCtrl = 0x00;
	byte SpeakerCfg = 0x00;
	
//Set I2S Audio mode.

	//[7:6]Audio Interface Selection; 0b10 I2S
	//[4]Audio Mute: 0b1 Mute.
	//[3:0]Audio Coding Type: 0b0000 Refer to Stream header (Auto)
	AudioInterface = (AUD_IF_I2S | BIT_AUDIO_MUTE | 0x0000 ); 

	//[7:6]Audio Sample Size: 0b00 Refer to Stream header (Auto)
	//[5:3]Audio Sample Frequency: 0b000 Refer to Stream header (Auto)
	//[2]HBR Support: 0b0 Layer0 2 channels
	//[1:0]Audio Channel Count: 0b001 2Chnannels.
	AudioSampleCtrl = 0x01;

	//[7:0]Speaker Config : Reserved if two speakers.
	SpeakerCfg = 0x00;
	
	Result = msm_hdmi_WriteByteTPI(TPI_AUDIO_INTERFACE, AudioInterface);	// 0x26 - set to I2S interface
	if (!Result)return TPI_WRITE_FAILURE;
	
	Result = msm_hdmi_WriteByteTPI(TPI_AUDIO_SAMPLE_CTRL, AudioSampleCtrl);	// 0x27 - Channel Count; Sample Frequency; Sample Size
	if (!Result) return TPI_WRITE_FAILURE;

	Result = msm_hdmi_WriteByteTPI(TPI_SPEAKER_CFG, SpeakerCfg);			// 0x28 - Speaker Configuration
	if (!Result) return TPI_WRITE_FAILURE;

	return I2S_MAPPING_SUCCESSFUL;
}


byte Set_I2S_Map(void)
{
	bool Result;
	byte MAP_Arg[4] = {FIFO_ENABLE|FIFO_I2S_0|SELECT_SD_CH0, FIFO_I2S_1|SELECT_SD_CH1, 
						FIFO_I2S_2|SELECT_SD_CH2, FIFO_I2S_3|SELECT_SD_CH3};
	byte B_Data;
	byte i;

	Result = msm_hdmi_ReadByteTPI(TPI_AUDIO_INTERFACE, &B_Data);
	if (!Result)
		return TPI_READ_FAILURE;

	if ((B_Data & HDMI_TWO_MSBITS) != AUD_IF_I2S)	// 0x26 not set to I2S interface
		return I2S_NOT_SET;

	for (i = 0; i < 4; i++)
	{
		Result = msm_hdmi_WriteByteTPI(TPI_I2S_EN, MAP_Arg[i]);
		if (!Result)
			return TPI_WRITE_FAILURE;
	}

	return I2S_MAPPING_SUCCESSFUL;
}

byte Set_I2S_ConfigInput( void )
{
	bool Result;
	byte Config_Arg = SCK_EDGE_RISING;
	byte B_Data;

	Result = msm_hdmi_ReadByteTPI(TPI_AUDIO_INTERFACE, &B_Data);
	if (!Result)
		return TPI_READ_FAILURE;

	if ((B_Data & HDMI_TWO_MSBITS) != AUD_IF_I2S)	// 0x26 not set to I2S interface
		return I2S_NOT_SET;

	Result = msm_hdmi_WriteByteTPI(TPI_I2S_IN_CFG, Config_Arg);
	if (!Result)
			return TPI_WRITE_FAILURE;

	return I2S_INPUT_CONFIG_SUCCESSFUL;
}

byte Set_I2S_StreamHeader( void )
{
	bool Result;
	//byte StreamHeader_Arg[] = {0x00, 0x00, 0x00, 0x00, 0x02}; //0x25 == 0x0B-24bit(default)
	byte StreamHeader_Arg[] = {0x00, 0x00, 0x00, 0x02, 0x02}; //0x25 == 0x0B-24bit(default)
	byte B_Data;
	int i;

	Result = msm_hdmi_ReadByteTPI(TPI_AUDIO_INTERFACE, &B_Data);
	if (!Result)
		return TPI_READ_FAILURE;

	if ((B_Data & HDMI_TWO_MSBITS) != AUD_IF_I2S)	// 0x26 not set to I2S interface
		return I2S_NOT_SET;

	for (i = 0; i < 5; i++)
	{
		Result = msm_hdmi_WriteByteTPI(TPI_I2S_CHST_0 + i, StreamHeader_Arg[i]);
		if (!Result)
			return TPI_WRITE_FAILURE;
	}

	return I2S_HEADER_SET_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      : 	hdmi_set_i2s_audio()
//
// PURPOSE       : 	Changes the 9022/4 audio mode to I2S mode.
//
// INPUT PARAMS  : 	void
//
// OUTPUT PARAMS : 	void
//
// GLOBALS USED  : 	None
//
// RETURNS       : 	SUCCESSFUL if Audio mode changed to I2S successfully. Error 
//					Code if Audi mode change failed
//
//////////////////////////////////////////////////////////////////////////////
byte hdmi_set_i2s_audio(void)
{
	byte result;

	result = Set_I2S_AudioMode();
	if (!result) return TPI_WRITE_FAILURE;

	result = Set_I2S_Map();
	if (!result) return TPI_WRITE_FAILURE;

	result = Set_I2S_ConfigInput();
	if (!result) return TPI_WRITE_FAILURE;

	result = Set_I2S_StreamHeader();
	if (!result) return TPI_WRITE_FAILURE;

	return I2S_MAPPING_SUCCESSFUL;
}


#endif //SKY_HDMI_AUDIO_I2S


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   HotPlugServiceLoop()
//
// PURPOSE      :   Implement Hot Plug Service Loop activities
//
// INPUT PARAMS :   None
//
// OUTPUT PARAMS:   void
//
// GLOBALS USED :   LinkProtectionLevel
//
// RETURNS      :   An error code that indicates success or cause of failure
//
//////////////////////////////////////////////////////////////////////////////
int sky_hdmi_HotPlugServiceLoop(void)
{
    byte regDump[256];
    int i, j;
    int ret_val=HDMI_SUCCESS_E;

	HDMI_MSG("HotPlugServiceLoop()\n");

	DisableInterrupts(0xFF);

#ifndef HDMI_TRANSCODE
#ifndef RX_ONBOARD
//	vid_mode = 4; // 480p 2, 720p 4;   //EDID_Data.VideoDescriptor[0];	// use 1st mode supported by sink
	vid_mode = sky_hdmi_get_vid_mode(); // 480p 2, 720p 4;   //EDID_Data.VideoDescriptor[0];	// use 1st mode supported by sink
#endif
  
	sky_hdmi_InitVideo(vid_mode, X1, HDMI_AFTER_INIT);		// Set PLL Multiplier to x1 upon power up
#endif

	TxPowerState(TX_POWER_STATE_D0);

#ifndef HDMI_TRANSCODE
	if (IsHDMI_Sink())							// Set InfoFrames only if HDMI output mode
	{
		SetPreliminaryInfoFrames(vid_mode);
	}

#ifdef mod_0417

	// This check needs to be changed to if HDCP is required by the content... once support has been added by RX-side library.
	if (HDCP_TxSupports == TRUE)
	{
		HDMI_MSG("TMDS -> Enabled (Video Muted), HDCP_TxSupports\n");
		ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, TMDS_OUTPUT_CONTROL_MASK | AV_MUTE_MASK, TMDS_OUTPUT_CONTROL_ACTIVE | AV_MUTE_MUTED);
		tmdsPoweredUp = TRUE;
	}
	else
	{
		HDMI_MSG("TMDS -> Enabled, HDCP_Tx not Supports\n");
		ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, TMDS_OUTPUT_CONTROL_MASK | AV_MUTE_MASK, TMDS_OUTPUT_CONTROL_ACTIVE | AV_MUTE_NORMAL);
		tmdsPoweredUp = TRUE;
	}
#endif


    if (IsHDMI_Sink())							// Set audio only if sink is HDMI compatible
    {
#ifdef SKY_HDMI_AUDIO_I2S

//          msm_hdmi_ReadSetWriteTPI(MISC_INFO_FRAMES_CTRL, ENABLE_AND_REPEAT);  // Enable and Repeat MPEG InfoFrames

          msm_hdmi_WriteByteTPI(0xbc, 0x02);                    
          msm_hdmi_WriteByteTPI(0xbd, 0x22);                    
          ReadModifyWriteTPI(0xbe, 0x0f, 0x02);                    
          msm_hdmi_WriteByteTPI(0xbc, 0x02);                    
          msm_hdmi_WriteByteTPI(0xbd, 0x24);                    
          ReadModifyWriteTPI(0xbe, 0x0f, 0x02);                    
          
          hdmi_set_i2s_audio();
        //ReadModifyWriteTPI(TPI_AUDIO_INTERFACE, HDMI_BIT_4, 0x00);
//0x26 = 0x81 //I2S Audio, PCM
//0x27 = 0x51 //16bit 44.1KHz 2Ch
//0x20 = 0x80 //SCK Rasing Edge
//0x1F = 0x80 //SD Channel Input Enable
//.1a = 0x01 
          msm_hdmi_WriteByteTPI(0x26, 0x81);
//          msm_hdmi_WriteByteTPI(0x27, 0x49); //0x51->0x49
          //msm_hdmi_WriteByteTPI(0x27, 0x51); //0x51->0x49
          msm_hdmi_WriteByteTPI(0x27, 0x59); //0x51->0x59
          msm_hdmi_WriteByteTPI(0x20, 0x84); //0x84->0x80
          msm_hdmi_WriteByteTPI(0x1f, 0x80);
#ifdef mod_0417
          msm_hdmi_WriteByteTPI(0x1a, 0x01);
          SetAudioInfoFrames(TWO_CHANNELS, 0x00, 0x00, 0x00, 0x00); 
#else
          SetAudioInfoFrames(TWO_CHANNELS, 0x00, 0x00, 0x00, 0x00); 
          msm_hdmi_WriteByteTPI(0x1a, 0x01);
#endif

          
#else
        SetBasicAudio();						// set audio interface to basic audio (an external command is needed to set to any other mode
#endif
    }
    else
    {
        MuteAudio();
	HDMI_MSG("\n.......................... MuteAudio...............................\n");
        
	}

#ifdef DEV_SUPPORT_EHDMI
	EHDMI_ARC_Common_Enable();
#endif

#else
	EnableTMDS();
	tmdsPoweredUp = TRUE;
#endif

#if 1 //test... 
           HDMI_MSG("Sil9024A Register VAL -----------------\n ");
           msm_hdmi_ReadBlockTPI(TPI_BASE_ADDR, EDID_BLOCK_0_OFFSET, regDump, 0xff); // read first 128 bytes of EDID ROM
  
           for(i=0, j=0;i<0x100;i++)
          {
                printk("%2.2x, ", regDump[i]);
                j++;
         
                if (j == 0x10)
          {
                    printk("\n\n");
                    j = 0;
           } 
           } 
           printk("----------------------------------\n");
#endif
//	EnableInterrupts(HOT_PLUG_EVENT | RX_SENSE_EVENT | AUDIO_ERROR_EVENT | SECURITY_CHANGE_EVENT | V_READY_EVENT | HDCP_CHANGE_EVENT);
//	EnableInterrupts(HOT_PLUG_EVENT |  AUDIO_ERROR_EVENT);
	EnableInterrupts(HOT_PLUG_EVENT | RX_SENSE_EVENT | AUDIO_ERROR_EVENT | SECURITY_CHANGE_EVENT);
//	EnableInterrupts(HOT_PLUG_EVENT | RX_SENSE_EVENT | AUDIO_ERROR_EVENT); 
#if 1 //for test device id 0x74
//       ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, AV_MUTE_MASK, 0x00); //for test....
#endif
    HDMI_MSG("END HotPlugServiceLoop()\n");

    return ret_val;

}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  GetDDC_Access(void)
//
// PURPOSE       :  Request access to DDC bus from the receiver
//
// INPUT PARAMS  :  None
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  TRUE if bus obtained successfully. FALSE if failed.
//
//////////////////////////////////////////////////////////////////////////////

bool GetDDC_Access (byte* SysCtrlRegVal)
{
	byte sysCtrl;
	byte DDCReqTimeout = T_DDC_ACCESS;
	byte TPI_ControlImage;

	HDMI_MSG("\n\n>>GetDDC_Access()\n");

	msm_hdmi_ReadByteTPI (TPI_SYSTEM_CONTROL_DATA_REG, &sysCtrl);				// Read and store original value. Will be passed into ReleaseDDC()
	*SysCtrlRegVal = sysCtrl;

	HDMI_MSG("Reg 1a Val %x\n", sysCtrl);

	sysCtrl |= BIT_DDC_BUS_REQ;
	msm_hdmi_WriteByteTPI (TPI_SYSTEM_CONTROL_DATA_REG, sysCtrl);

	while (DDCReqTimeout--)									// Loop till 0x1A[1] reads "1"
	{
		msm_hdmi_ReadByteTPI(TPI_SYSTEM_CONTROL_DATA_REG, &TPI_ControlImage);

		if (TPI_ControlImage & BIT_DDC_BUS_GRANT)			// When 0x1A[1] reads "1"
		{
			sysCtrl |= BIT_DDC_BUS_GRANT;
			msm_hdmi_WriteByteTPI(TPI_SYSTEM_CONTROL_DATA_REG, sysCtrl);		// lock host DDC bus access (0x1A[2:1] = 11)
                     HDMI_MSG("DDCReqTimeout  %x\n", DDCReqTimeout);
                     msleep(5);
			return TRUE;
		}
		msm_hdmi_WriteByteTPI(TPI_SYSTEM_CONTROL_DATA_REG, sysCtrl);			// 0x1A[2] = "1" - Requst the DDC bus
		msleep(200);
	}

	msm_hdmi_WriteByteTPI(TPI_SYSTEM_CONTROL_DATA_REG, sysCtrl);				// Failure... restore original value.

	HDMI_MSG("DDCReqTimeout  %x\n", DDCReqTimeout);
    
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   byte Parse861ShortDescriptors()
//
// PURPOSE      :   Parse CEA-861 extension short descriptors of the EDID block
//                  passed as a parameter and save them in global structure
//                  EDID_Data.
//
// INPUT PARAMS :   A pointer to the EDID 861 Extension block being parsed.
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   EDID_Data
//
// RETURNS      :   EDID_PARSED_OK if EDID parsed correctly. Error code if failed.
//
// NOTE         :   Fields that are not supported by the 9022/4 (such as deep
//                  color) are not parsed.
//
//////////////////////////////////////////////////////////////////////////////

static byte Parse861ShortDescriptors(byte *Data)
{
    byte LongDescriptorOffset;
    byte DataBlockLength;
    byte DataIndex;
    byte ExtendedTagCode;

//    static byte V_DescriptorIndex = 0;  // static to support more than one extension
//    static byte A_DescriptorIndex = 0;  // static to support more than one extension
    byte V_DescriptorIndex = 0;  // static to support more than one extension
    byte A_DescriptorIndex = 0;  // static to support more than one extension 


    byte TagCode;

    byte i;
    byte j;

    if (Data[EDID_TAG_ADDR] != EDID_EXTENSION_TAG)
    {
        HDMI_MSG("EDID -> Extension Tag Error\n");
        return EDID_EXT_TAG_ERROR;
    }

    if (Data[EDID_REV_ADDR] != EDID_REV_THREE)
    {
        HDMI_MSG("EDID -> Revision Error\n");
        return EDID_REV_ADDR_ERROR;
    }

    LongDescriptorOffset = Data[LONG_DESCR_PTR_IDX];    // block offset where long descriptors start

    EDID_Data.UnderScan = ((Data[MISC_SUPPORT_IDX]) >> 7) & HDMI_LSBIT;  // byte #3 of CEA extension version 3
    EDID_Data.BasicAudio = ((Data[MISC_SUPPORT_IDX]) >> 6) & HDMI_LSBIT;
    EDID_Data.YCbCr_4_4_4 = ((Data[MISC_SUPPORT_IDX]) >> 5) & HDMI_LSBIT;
    EDID_Data.YCbCr_4_2_2 = ((Data[MISC_SUPPORT_IDX]) >> 4) & HDMI_LSBIT;

    DataIndex = EDID_DATA_START;            // 4

    while (DataIndex < LongDescriptorOffset)
    {
        TagCode = (Data[DataIndex] >> 5) & HDMI_THREE_LSBITS;
        DataBlockLength = Data[DataIndex++] & HDMI_FIVE_LSBITS;
        if ((DataIndex + DataBlockLength) > LongDescriptorOffset)
        {
            HDMI_MSG("EDID -> V Descriptor Overflow\n");
            return EDID_V_DESCR_OVERFLOW;
        }

        i = 0;                                  // num of short video descriptors in current data block

        switch (TagCode)
        {
            case VIDEO_D_BLOCK:
                while ((i < DataBlockLength) && (i < MAX_V_DESCRIPTORS))        // each SVD is 1 byte long
                {
                    EDID_Data.VideoDescriptor[V_DescriptorIndex++] = Data[DataIndex++];
                    i++;
                }
                DataIndex += DataBlockLength - i;   // if there are more STDs than MAX_V_DESCRIPTORS, skip the last ones. Update DataIndex

                HDMI_MSG("EDID -> Short Descriptor Video Block\n");
                break;

            case AUDIO_D_BLOCK:
                while (i < DataBlockLength/3)       // each SAD is 3 bytes long
                {
                    j = 0;
                    while (j < AUDIO_DESCR_SIZE)    // 3
                    {
                        EDID_Data.AudioDescriptor[A_DescriptorIndex][j++] = Data[DataIndex++];
                    }
                    A_DescriptorIndex++;
                    i++;
                }
                HDMI_MSG("EDID -> Short Descriptor Audio Block\n");
                break;

            case  SPKR_ALLOC_D_BLOCK:
                EDID_Data.SpkrAlloc[i++] = Data[DataIndex++];       // although 3 bytes are assigned to Speaker Allocation, only
                DataIndex += 2;                                     // the first one carries information, so the next two are ignored by this code.
                HDMI_MSG("EDID -> Short Descriptor Speaker Allocation Block\n");
                break;

            case USE_EXTENDED_TAG:
                ExtendedTagCode = Data[DataIndex++];

                switch (ExtendedTagCode)
                {
                    case VIDEO_CAPABILITY_D_BLOCK:

		                HDMI_MSG("EDID -> Short Descriptor Video Capability Block\n");
		
						// TO BE ADDED HERE: Save "video capability" parameters in EDID_Data data structure
						// Need to modify that structure definition
						// In the meantime: just increment DataIndex by 1

						DataIndex += 1;	   // replace with reading and saving the proper data per CEA-861 sec. 7.5.6 while incrementing DataIndex 

						break;
					

                    case COLORIMETRY_D_BLOCK:
                        EDID_Data.ColorimetrySupportFlags = Data[DataIndex++] & BITS_1_0;
                        EDID_Data.MetadataProfile = Data[DataIndex++] & BITS_2_1_0;
						
		                HDMI_MSG("EDID -> Short Descriptor Colorimetry Block\n");
		
						break;
                }
				break;

            case VENDOR_SPEC_D_BLOCK:
                if ((Data[DataIndex++] == 0x03) &&    // check if sink is HDMI compatible
                    (Data[DataIndex++] == 0x0C) &&
                    (Data[DataIndex++] == 0x00))

                    EDID_Data.HDMI_Sink = TRUE;
                else
                    EDID_Data.HDMI_Sink = FALSE;

                EDID_Data.CEC_A_B = Data[DataIndex++];  // CEC Physical address
                EDID_Data.CEC_C_D = Data[DataIndex++];

                DataIndex += DataBlockLength - HDMI_SIGNATURE_LEN - CEC_PHYS_ADDR_LEN; // Point to start of next block
                HDMI_MSG("EDID -> Short Descriptor Vendor Block\n");
 				HDMI_MSG("\n");
                break;

            default:
                HDMI_MSG("EDID -> Unknown Tag Code\n");
                return EDID_UNKNOWN_TAG_CODE;

        }                   // End, Switch statement
    }                       // End, while (DataIndex < LongDescriptorOffset) statement

    return EDID_SHORT_DESCRIPTORS_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   byte Parse861LongDescriptors()
//
// PURPOSE      :   Parse CEA-861 extension long descriptors of the EDID block
//                  passed as a parameter and printf() them to the screen.
//                  EDID_Data.
//
// INPUT PARAMS :   A pointer to the EDID block being parsed; The parsed block
//                  number.
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   An error code if no long descriptors found; EDID_PARSED_OK
//                  if descriptors found.
//
//////////////////////////////////////////////////////////////////////////////


static byte Parse861LongDescriptors(byte *Data, byte BlockNum)
{
    byte LongDescriptorsOffset;
    byte DescriptorNum = 1;

    LongDescriptorsOffset = Data[LONG_DESCR_PTR_IDX];   // EDID block offset 2 holds the offset

    if (!LongDescriptorsOffset)                         // per CEA-861-D, table 27
    {
        HDMI_MSG("EDID -> No Detailed Descriptors\n");
        return EDID_NO_DETAILED_DESCRIPTORS;
    }

    // of the 1st 18-byte descriptor
    while (LongDescriptorsOffset + LONG_DESCR_LEN < EDID_BLOCK_SIZE)
    {
        HDMI_MSG("Parse Results - CEA-861 Extension Block #%d, Long Descriptor #%d:\n", (int) BlockNum, (int) DescriptorNum);
        HDMI_MSG("===============================================================\n");

#if (CONF__TPI_EDID_PRINT == ENABLE)
        if (!ParseDetailedTiming(Data, LongDescriptorsOffset, EDID_BLOCK_2_3))
			break;
#endif
        LongDescriptorsOffset +=  LONG_DESCR_LEN;
        DescriptorNum++;
    }

    return EDID_LONG_DESCRIPTORS_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   byte Parse861Extensions()
//
// PURPOSE      :   Parse CEA-861 extensions from EDID ROM (EDID blocks beyond
//                  block #0). Save short descriptors in global structure
//                  EDID_Data. printf() long descriptors to the screen.
//
// INPUT PARAMS :   The number of extensions in the EDID being parsed
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   EDID_PARSED_OK if EDID parsed correctly. Error code if failed.
//
// NOTE         :   Fields that are not supported by the 9022/4 (such as deep
//                  color) were not parsed.
//
//////////////////////////////////////////////////////////////////////////////

static byte Parse861Extensions(byte NumOfExtensions)
{
    byte i,j,k;

    byte ErrCode;

#ifdef kernel_warn
//    byte V_DescriptorIndex = 0;
//    byte A_DescriptorIndex = 0;
#endif

    byte Segment = 0;
    byte Block = 0;
    byte Offset = 0;

	EDID_Data.HDMI_Sink = FALSE;

	do
    {
		Block++;

        Offset = 0;
        if ((Block % 2) > 0)
        {
            Offset = EDID_BLOCK_SIZE;
        }

        Segment = (byte) (Block / 2);

		if (Block == 1)
		{
			#ifdef CBUS_EDID
			CyclopsReadEdid01(EDID_BLOCK_1_OFFSET, EDID_BLOCK_SIZE);
			#else
   			ReadBlockEDID(EDID_BLOCK_1_OFFSET, EDID_BLOCK_SIZE, EDID_TempData);    // read first 128 bytes of EDID ROM
			#endif
		}
		else
		{
			#ifdef CBUS_EDID
			CyclopsReadEdid23(Offset, EDID_BLOCK_SIZE);
			#else
                     ReadBlockEDIDseg(Segment, Offset, EDID_BLOCK_SIZE, EDID_TempData);    // read first 128 bytes of EDID ROM
			//ReadSegmentBlockEDID(Segment, Offset, EDID_BLOCK_SIZE, EDID_TempData);     // read next 128 bytes of EDID ROM
			#endif
		}
	
		HDMI_MSG("\n");
        HDMI_MSG("%s :: EDID DATA (Segment = %d Block = %d Offset = %d):\n", __func__,  (int) Segment, (int) Block, (int) Offset);
        for (j=0, i=0; j<128; j++)
        {
            k = EDID_TempData[j];
            printk("%02x ", (int) k);
            i++;

            if (i == 0x10)
            {
                printk("\n");
                i = 0;
            }
        }
		printk("\n");
		
		if ((NumOfExtensions > 1) && (Block == 1))
		{
			continue;
		}

        ErrCode = Parse861ShortDescriptors(EDID_TempData);
            if (ErrCode != EDID_SHORT_DESCRIPTORS_OK)
            {
                return ErrCode;
            }

        ErrCode = Parse861LongDescriptors(EDID_TempData, Block);
        if (ErrCode != EDID_LONG_DESCRIPTORS_OK)
        {
            return ErrCode;
        }
	} while (Block < NumOfExtensions);

    return EDID_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   ParseStandardTiming()
//
// PURPOSE      :   Parse the standard timing section of EDID Block 0 and
//                  print their decoded meaning to the screen.
//
// INPUT PARAMS :   Pointer to the array where the data read from EDID
//                  Block0 is stored.
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   Void
//
//////////////////////////////////////////////////////////////////////////////
static void ParseStandardTiming(byte *Data)
{
    byte i;
    byte AR_Code;

    HDMI_MSG("Parsing Standard Timing:\n");
    HDMI_MSG("========================\n");

    for (i = 0; i < NUM_OF_STANDARD_TIMINGS; i += 2)
    {
        if ((Data[STANDARD_TIMING_OFFSET + i] == 0x01) && ((Data[STANDARD_TIMING_OFFSET + i +1]) == 1))
        {
            HDMI_MSG("Standard Timing Undefined\n"); // per VESA EDID standard, Release A, Revision 1, February 9, 2000, Sec. 3.9
		}
        else
        {
            HDMI_MSG("Horizontal Active pixels: %i\n", (int)((Data[STANDARD_TIMING_OFFSET + i] + 31)*8));    // per VESA EDID standard, Release A, Revision 1, February 9, 2000, Table 3.15

            AR_Code = (Data[STANDARD_TIMING_OFFSET + i +1] & HDMI_TWO_MSBITS) >> 6;
            HDMI_MSG("Aspect Ratio: ");

            switch(AR_Code)
            {
                case HDMI_AR16_10:
                    HDMI_MSG("16:10\n");
                    break;

                case HDMI_AR4_3:
                    HDMI_MSG("4:3\n");
                    break;

                case HDMI_AR5_4:
                    HDMI_MSG("5:4\n");
                    break;
                case HDMI_AR16_9:
                    HDMI_MSG("16:9\n");
                    break;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   ParseDetailedTiming()
//
// PURPOSE      :   Parse the detailed timing section of EDID Block 0 and
//                  print their decoded meaning to the screen.
//
// INPUT PARAMS :   Pointer to the array where the data read from EDID
//                  Block0 is stored.
//
//                  Offset to the beginning of the Detailed Timing Descriptor
//                  data.
//
//					Block indicator to distinguish between block #0 and blocks
//					#2, #3
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   Void
//
//////////////////////////////////////////////////////////////////////////////
static bool ParseDetailedTiming(byte *Data, byte DetailedTimingOffset, byte Block)
{
        byte TmpByte;
        byte i;
        word TmpWord;

        TmpWord = Data[DetailedTimingOffset + HDMI_PIX_CLK_OFFSET] +
                    256 * Data[DetailedTimingOffset + HDMI_PIX_CLK_OFFSET + 1];

        if (TmpWord == 0x00)            // 18 byte partition is used as either for Monitor Name or for Monitor Range Limits or it is unused
        {
        	if (Block == EDID_BLOCK_0)	// if called from Block #0 and first 2 bytes are 0 => either Monitor Name or for Monitor Range Limits
            {
            	if (Data[DetailedTimingOffset + 3] == 0xFC) // these 13 bytes are ASCII coded monitor name
            	{
					HDMI_MSG("Monitor Name: ");

                	for (i = 0; i < 13; i++)
					{
                    	HDMI_MSG("%c", Data[DetailedTimingOffset + 5 + i]); // Display monitor name on SiIMon
					}
					HDMI_MSG("\n");
            	}

		        else if (Data[DetailedTimingOffset + 3] == 0xFD) // these 13 bytes contain Monitor Range limits, binary coded
        		{
                            HDMI_MSG("Monitor Range Limits:\n\n");

                            i = 0;
                            HDMI_MSG("Min Vertical Rate in Hz: %d\n", (int) Data[DetailedTimingOffset + 5 + i++]); //
                            HDMI_MSG("Max Vertical Rate in Hz: %d\n", (int) Data[DetailedTimingOffset + 5 + i++]); //
                            HDMI_MSG("Min Horizontal Rate in Hz: %d\n", (int) Data[DetailedTimingOffset + 5 + i++]); //
                            HDMI_MSG("Max Horizontal Rate in Hz: %d\n", (int) Data[DetailedTimingOffset + 5 + i++]); //
                            HDMI_MSG("Max Supported pixel clock rate in MHz/10: %d\n", (int) Data[DetailedTimingOffset + 5 + i++]); //
                            HDMI_MSG("Tag for secondary timing formula (00h=not used): %d\n", (int) Data[DetailedTimingOffset + 5 + i++]); //
                            HDMI_MSG("Min Vertical Rate in Hz %d\n", (int) Data[DetailedTimingOffset + 5 + i]); //
                            HDMI_MSG("\n");
        		}
			}

        	else if (Block == EDID_BLOCK_2_3)			   // if called from block #2 or #3 and first 2 bytes are 0x00 (padding) then this
        	{											   // descriptor partition is not used and parsing should be stopped
				HDMI_MSG("No More Detailed descriptors in this block\n");
				HDMI_MSG("\n");
				return FALSE;
			}
		}

		else						// first 2 bytes are not 0 => this is a detailed timing descriptor from either block
		{
			if((Block == EDID_BLOCK_0) && (DetailedTimingOffset == 0x36))
			{
	        	HDMI_MSG("\n\n\nParse Results, EDID Block #0, Detailed Descriptor Number 1:\n");
				HDMI_MSG("===========================================================\n\n");
			}
			else if((Block == EDID_BLOCK_0) && (DetailedTimingOffset == 0x48))
	        {
	        	HDMI_MSG("\n\n\nParse Results, EDID Block #0, Detailed Descriptor Number 2:\n");
				HDMI_MSG("===========================================================\n\n");
			}

            HDMI_MSG("Pixel Clock (MHz * 100): %d\n", (int)TmpWord);

            TmpWord = Data[DetailedTimingOffset + HDMI_H_ACTIVE_OFFSET] +
                    256 * ((Data[DetailedTimingOffset + HDMI_H_ACTIVE_OFFSET + 2] >> 4) & HDMI_FOUR_LSBITS);
            HDMI_MSG("Horizontal Active Pixels: %d\n", (int)TmpWord);

            TmpWord = Data[DetailedTimingOffset + HDMI_H_BLANKING_OFFSET] +
                    256 * (Data[DetailedTimingOffset + HDMI_H_BLANKING_OFFSET + 1] & HDMI_FOUR_LSBITS);
            HDMI_MSG("Horizontal Blanking (Pixels): %d\n", (int)TmpWord);

            TmpWord = (Data[DetailedTimingOffset + HDMI_V_ACTIVE_OFFSET] )+
                    256 * ((Data[DetailedTimingOffset + (HDMI_V_ACTIVE_OFFSET) + 2] >> 4) & HDMI_FOUR_LSBITS);
            HDMI_MSG("Vertical Active (Lines): %d\n", (int)TmpWord);

            TmpWord = Data[DetailedTimingOffset + HDMI_V_BLANKING_OFFSET] +
                    256 * (Data[DetailedTimingOffset + HDMI_V_BLANKING_OFFSET + 1] & HDMI_LOW_NIBBLE);
            HDMI_MSG("Vertical Blanking (Lines): %d\n", (int)TmpWord);

            TmpWord = Data[DetailedTimingOffset + HDMI_H_SYNC_OFFSET] +
                    256 * ((Data[DetailedTimingOffset + (HDMI_H_SYNC_OFFSET + 3)] >> 6) & HDMI_TWO_LSBITS);
            HDMI_MSG("Horizontal Sync Offset (Pixels): %d\n", (int)TmpWord);

            TmpWord = Data[DetailedTimingOffset + HDMI_H_SYNC_PW_OFFSET] +
                    256 * ((Data[DetailedTimingOffset + (HDMI_H_SYNC_PW_OFFSET + 2)] >> 4) & HDMI_TWO_LSBITS);
            HDMI_MSG("Horizontal Sync Pulse Width (Pixels): %d\n", (int)TmpWord);

            TmpWord = (Data[DetailedTimingOffset + HDMI_V_SYNC_OFFSET] >> 4) & HDMI_FOUR_LSBITS +
                    256 * ((Data[DetailedTimingOffset + (HDMI_V_SYNC_OFFSET + 1)] >> 2) & HDMI_TWO_LSBITS);
            HDMI_MSG("Vertical Sync Offset (Lines): %d\n", (int)TmpWord);

            TmpWord = (Data[DetailedTimingOffset + HDMI_V_SYNC_PW_OFFSET]) & HDMI_FOUR_LSBITS +
                    256 * (Data[DetailedTimingOffset + (HDMI_V_SYNC_PW_OFFSET + 1)] & HDMI_TWO_LSBITS);
            HDMI_MSG("Vertical Sync Pulse Width (Lines): %d\n", (int)TmpWord);

            TmpWord = Data[DetailedTimingOffset + HDMI_H_IMAGE_SIZE_OFFSET] +
                    256 * (((Data[DetailedTimingOffset + (HDMI_H_IMAGE_SIZE_OFFSET + 2)]) >> 4) & HDMI_FOUR_LSBITS);
            HDMI_MSG("Horizontal Image Size (mm): %d\n", (int)TmpWord);

            TmpWord = Data[DetailedTimingOffset + HDMI_V_IMAGE_SIZE_OFFSET] +
                    256 * (Data[DetailedTimingOffset + (HDMI_V_IMAGE_SIZE_OFFSET + 1)] & HDMI_FOUR_LSBITS);
            HDMI_MSG("Vertical Image Size (mm): %d\n", (int)TmpWord);

            TmpByte = Data[DetailedTimingOffset + HDMI_H_BORDER_OFFSET];
            HDMI_MSG("Horizontal Border (Pixels): %d\n", (int)TmpByte);

            TmpByte = Data[DetailedTimingOffset + HDMI_V_BORDER_OFFSET];
            HDMI_MSG("Vertical Border (Lines): %d\n", (int)TmpByte);

            TmpByte = Data[DetailedTimingOffset + HDMI_FLAGS_OFFSET];
            if (TmpByte & HDMI_BIT_7)
                HDMI_MSG("Interlaced\n");
            else
                HDMI_MSG("Non-Interlaced\n");

            if (!(TmpByte & HDMI_BIT_5) && !(TmpByte & HDMI_BIT_6))
                HDMI_MSG("Normal Display, No Stereo\n");
            else
                HDMI_MSG("Refer to VESA E-EDID Release A, Revision 1, table 3.17\n");

            if (!(TmpByte & HDMI_BIT_3) && !(TmpByte & HDMI_BIT_4))
                HDMI_MSG("Analog Composite\n");
            if ((TmpByte & HDMI_BIT_3) && !(TmpByte & HDMI_BIT_4))
                HDMI_MSG("Bipolar Analog Composite\n");
            else if (!(TmpByte & HDMI_BIT_3) && (TmpByte & HDMI_BIT_4))
                HDMI_MSG("Digital Composite\n");

            else if ((TmpByte & HDMI_BIT_3) && (TmpByte & HDMI_BIT_4))
                HDMI_MSG("Digital Separate\n");

			HDMI_MSG("\n");
    	}
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   ParseEstablishedTiming()
//
// PURPOSE      :   Parse the established timing section of EDID Block 0 and
//                  print their decoded meaning to the screen.
//
// INPUT PARAMS :   Pointer to the array where the data read from EDID
//                  Block0 is stored.
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   Void
//
//////////////////////////////////////////////////////////////////////////////
static void ParseEstablishedTiming(byte *Data)
{
    HDMI_MSG("Parsing Established Timing:\n");
    HDMI_MSG("===========================\n");


    // Parse Established Timing Byte #0

    if(Data[ESTABLISHED_TIMING_INDEX] & HDMI_BIT_7)
        HDMI_MSG("720 x 400 @ 70Hz\n");
    if(Data[ESTABLISHED_TIMING_INDEX] & HDMI_BIT_6)
        HDMI_MSG("720 x 400 @ 88Hz\n");
    if(Data[ESTABLISHED_TIMING_INDEX] & HDMI_BIT_5)
        HDMI_MSG("640 x 480 @ 60Hz\n");
    if(Data[ESTABLISHED_TIMING_INDEX] & HDMI_BIT_4)
        HDMI_MSG("640 x 480 @ 67Hz\n");
    if(Data[ESTABLISHED_TIMING_INDEX] & HDMI_BIT_3)
        HDMI_MSG("640 x 480 @ 72Hz\n");
    if(Data[ESTABLISHED_TIMING_INDEX] & HDMI_BIT_2)
        HDMI_MSG("640 x 480 @ 75Hz\n");
    if(Data[ESTABLISHED_TIMING_INDEX] & HDMI_BIT_1)
        HDMI_MSG("800 x 600 @ 56Hz\n");
    if(Data[ESTABLISHED_TIMING_INDEX] & HDMI_BIT_0)
        HDMI_MSG("800 x 400 @ 60Hz\n");

    // Parse Established Timing Byte #1:

    if(Data[ESTABLISHED_TIMING_INDEX + 1]  & HDMI_BIT_7)
        HDMI_MSG("800 x 600 @ 72Hz\n");
    if(Data[ESTABLISHED_TIMING_INDEX + 1]  & HDMI_BIT_6)
        HDMI_MSG("800 x 600 @ 75Hz\n");
    if(Data[ESTABLISHED_TIMING_INDEX + 1]  & HDMI_BIT_5)
        HDMI_MSG("832 x 624 @ 75Hz\n");
    if(Data[ESTABLISHED_TIMING_INDEX + 1]  & HDMI_BIT_4)
        HDMI_MSG("1024 x 768 @ 87Hz\n");
    if(Data[ESTABLISHED_TIMING_INDEX + 1]  & HDMI_BIT_3)
        HDMI_MSG("1024 x 768 @ 60Hz\n");
    if(Data[ESTABLISHED_TIMING_INDEX + 1]  & HDMI_BIT_2)
        HDMI_MSG("1024 x 768 @ 70Hz\n");
    if(Data[ESTABLISHED_TIMING_INDEX + 1]  & HDMI_BIT_1)
        HDMI_MSG("1024 x 768 @ 75Hz\n");
    if(Data[ESTABLISHED_TIMING_INDEX + 1]  & HDMI_BIT_0)
        HDMI_MSG("1280 x 1024 @ 75Hz\n");

    // Parse Established Timing Byte #2:

    if(Data[ESTABLISHED_TIMING_INDEX + 2] & 0x80)
        HDMI_MSG("1152 x 870 @ 75Hz\n");

    if((!Data[0])&&(!Data[ESTABLISHED_TIMING_INDEX + 1]  )&&(!Data[2]))
        HDMI_MSG("No established video modes\n");
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   ParseBlock_0_TimingDescripors()
//
// PURPOSE      :   Parse EDID Block 0 timing descriptors per EEDID 1.3
//                  standard. printf() values to screen.
//
// INPUT PARAMS :   Pointer to the 128 byte array where the data read from EDID
//                  Block0 is stored.
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   Void
//
//////////////////////////////////////////////////////////////////////////////

static void ParseBlock_0_TimingDescripors(byte *Data)
{
#if (CONF__TPI_EDID_PRINT == ENABLE)
    byte i;
    byte Offset;

    ParseEstablishedTiming(Data);
    ParseStandardTiming(Data);


    for (i = 0; i < NUM_OF_DETAILED_DESCRIPTORS; i++)
    {
        Offset = DETAILED_TIMING_OFFSET + (LONG_DESCR_LEN * i);
        ParseDetailedTiming(Data, Offset, EDID_BLOCK_0);
    }
#else
	Data = Data; 	// Dummy usage to suppress warning
#endif
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   bool CheckEDID_Header()
//
// PURPOSE      :   Checks if EDID header is correct per VESA E-EDID standard
//
// INPUT PARAMS :   Pointer to 1st EDID block
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   TRUE if Header is correct. FALSE if not.
//
//////////////////////////////////////////////////////////////////////////////

static bool CheckEDID_Header(byte *Block)
{
    byte i = 0;

    if (Block[i])               // byte 0 must be 0
        return FALSE;

    for (i = 1; i < 1 + EDID_HDR_NO_OF_FF; i++)
    {
        if(Block[i] != 0xFF)    // bytes [1..6] must be 0xFF
            return FALSE;
    }

    if (Block[i])               // byte 7 must be 0
        return FALSE;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   bool DoEDID_Checksum()
//
// PURPOSE      :   Calculte checksum of the 128 byte block pointed to by the
//                  pointer passed as parameter
//
// INPUT PARAMS :   Pointer to a 128 byte block whose checksum needs to be
//                  calculated
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   TRUE if chcksum is 0. FALSE if not.
//
//////////////////////////////////////////////////////////////////////////////

bool DoEDID_Checksum(byte *Block)
{
    byte i;
    byte CheckSum = 0;

    for (i = 0; i < EDID_BLOCK_SIZE; i++)
        CheckSum += Block[i];

    if (CheckSum)
        return FALSE;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   byte ParseEDID()
//
// PURPOSE      :   Extract sink properties from its EDID file and save them in
//                  global structure EDID_Data.
//
// INPUT PARAMS :   None
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   EDID_Data
//
// RETURNS      :   A byte error code to indicates success or error type.
//
// NOTE         :   Fields that are not supported by the 9022/4 (such as deep
//                  color) were not parsed.
//
//////////////////////////////////////////////////////////////////////////////
byte ParseEDID(void)
{
    byte i,j,k;
    byte NumOfExtensions;

	#ifdef CBUS_EDID
	CyclopsReadEdid01(EDID_BLOCK_0_OFFSET, EDID_BLOCK_SIZE);
	#else
	ReadBlockEDID(EDID_BLOCK_0_OFFSET, EDID_BLOCK_SIZE, EDID_TempData);    // read first 128 bytes of EDID ROM
	#endif
	
	HDMI_MSG("\n");
	HDMI_MSG("%s ::  EDID DATA (Segment = 0 Block = 0 Offset = %d):\n", __func__, (int) EDID_BLOCK_0_OFFSET);
 	for (j=0, i=0; j<128; j++)
 	{
   		k = EDID_TempData[j];
       	printk("%2.2X ", (int) k);
       	i++;

       	if (i == 0x10)
       	{
           	printk("\n");
           	i = 0;
       	}
   	}
	HDMI_MSG("\n");

   	if (!DoEDID_Checksum(EDID_TempData))
   	{
       	HDMI_MSG("EDID -> Checksum Error\n");
       	return EDID_CHECKSUM_ERROR;									// non-zero EDID checksum
   	}

   	if (!CheckEDID_Header(EDID_TempData))									// first 8 bytes of EDID must be {0, FF, FF, FF, FF, FF, FF, 0}
   	{
       	HDMI_MSG("EDID -> Incorrect Header\n");
       	return EDID_INCORRECT_HEADER;
   	}

   	ParseBlock_0_TimingDescripors(EDID_TempData);							// Parse EDID Block #0 Desctiptors

   	NumOfExtensions = EDID_TempData[NUM_OF_EXTEN_ADDR];					// read # of extensions from offset 0x7E of block 0
   	HDMI_MSG("EDID -> No 861 Extensions = %d\n", (int) NumOfExtensions);

   	if (!NumOfExtensions)
   	{
       	return EDID_NO_861_EXTENSIONS;
   	}

	return Parse861Extensions(NumOfExtensions);						// Parse 861 Extensions (short and long descriptors);
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  ReleaseDDC(void)
//
// PURPOSE       :  Release DDC bus
//
// INPUT PARAMS  :  None
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  TRUE if bus released successfully. FALSE if failed.
//
//////////////////////////////////////////////////////////////////////////////

bool ReleaseDDC(byte SysCtrlRegVal)
{
	byte DDCReqTimeout = 3;//T_DDC_ACCESS;
	byte TPI_ControlImage;

	SysCtrlRegVal &= ~BITS_2_1;					// Just to be sure bits [2:1] are 0 before it is written

	HDMI_MSG(">>ReleaseDDC() , SysCtrlRegVal : 0x%x\n", SysCtrlRegVal);

	while (DDCReqTimeout--)						// Loop till 0x1A[1] reads "0"
	{
		// Cannot use msm_hdmi_ReadClearWriteTPI() here. A read of TPI_SYSTEM_CONTROL is invalid while DDC is granted.
		// Doing so will return 0xFF, and cause an invalid value to be written back.
		//msm_hdmi_ReadClearWriteTPI(TPI_SYSTEM_CONTROL,BITS_2_1); // 0x1A[2:1] = "0" - release the DDC bus

//		msm_hdmi_ReadByteTPI(0x26, &TPI_ControlImage); //ï¿½ï¿½ï¿½ï¿½ ï¿½ß°ï¿½.. 
		msm_hdmi_WriteByteTPI(TPI_SYSTEM_CONTROL_DATA_REG, SysCtrlRegVal);
              msleep(5);
		msm_hdmi_ReadByteTPI(TPI_SYSTEM_CONTROL_DATA_REG, &TPI_ControlImage);

		if (!(TPI_ControlImage & BITS_2_1))		// When 0x1A[2:1] read "0"
			return TRUE;
	}
	HDMI_MSG(">>ReleaseDDC() FAIL\n");

	return FALSE;								// Failed to release DDC bus control
}

int DoEdidRead (void)
{

    byte SysCtrlReg;
    byte Result;
    int ret_val=HDMI_SUCCESS_E;


	if (edidDataValid == FALSE)
	{
		if (GetDDC_Access(&SysCtrlReg))
		{
			Result = ParseEDID();
			if (Result != EDID_OK)
			{
				if (Result == EDID_NO_861_EXTENSIONS)
				{
					EDID_Data.HDMI_Sink = FALSE;
					HDMI_MSG("EDID -> No 861 Extensions. Handle as DVI Sink\n");
				}
				else
				{
					HDMI_MSG("EDID -> Parse FAILED\n");
				}
			}
			else
			{
				HDMI_MSG("EDID -> Parse OK\n");
			}

			if (!ReleaseDDC(SysCtrlReg))              // Host must release DDC bus once it is done reading EDID
			{
				HDMI_MSG("EDID -> DDC bus release failed\n");
				return EDID_DDC_BUS_RELEASE_FAILURE;
			}
		}
		else
		{
			HDMI_MSG("EDID -> DDC bus request failed\n");
			return EDID_DDC_BUS_REQ_FAILURE;
		}

		if (IsHDMI_Sink())              // select output mode (HDMI/DVI) according to sink capabilty
		{
			HDMI_MSG("EDID -> HDMI Sink detected\n");
			msm_hdmi_ReadSetWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, BIT_OUTPUT_MODE);       // Set output mode to HDMI
		}
		else
		{
			HDMI_MSG("EDID -> DVI Sink detected\n");
			msm_hdmi_ReadClearWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, BIT_OUTPUT_MODE);     // Set output mode to DVI
		}

	edidDataValid = TRUE;
	}
    return HDMI_SUCCESS_E;
}


static int OnHdmiCableConnected(void)
{
    int ret_val=HDMI_SUCCESS_E;
    
    HDMI_MSG("TP -> HDMI Cable Connected\n");
    hdmiCableConnected = TRUE;

#ifdef DEV_SUPPORT_MHL
    //Without this delay, the downstream BKSVs wil not be properly read, resulting in HDCP not being available.
    mdelay(100);
#else
    TPI_Init();
#endif

#ifdef DEV_SUPPORT_EDID
    ret_val=DoEdidRead();
    if(ret_val==EDID_DDC_BUS_RELEASE_FAILURE)
        return HDMI_I2C_FAIL_E;
#endif
    return HDMI_SUCCESS_E;
}

static void OnHdmiCableDisconnected(void) 
{
    HDMI_MSG("TP -> HDMI Cable Disconnected\n");
    hdmiCableConnected = FALSE;
    dsRxPoweredUp = FALSE;
#ifdef DEV_SUPPORT_EDID
    edidDataValid = FALSE;
#endif

#ifdef DEV_SUPPORT_HDCP
//    HDCP_Off();
#endif

    DisableTMDS();
    tmdsPoweredUp = FALSE;
}

static void OnDownstreamRxPoweredDown(void) {

	HDMI_MSG("TP -> Downstream RX Powered Down\n");
	dsRxPoweredUp = FALSE;

#ifdef DEV_SUPPORT_HDCP
//	HDCP_Off();
#endif

	//DisableTMDS();
}


static void OnDownstreamRxPoweredUp(void)
{
    HDMI_MSG("TP -> Downstream RX Powered Up\n");
    dsRxPoweredUp = TRUE;

    sky_hdmi_HotPlugServiceLoop();
    set_connection_cable_flag(ON); //20100410
    sky_hdmi_set_isr(ON); //20100410
}


int sky_hdmi_TPI_poll(void)
{
    byte InterruptStatusImage;
    int ret_val=HDMI_SUCCESS_E;
    HDMI_MSG("%s :: \n", __func__);

    msm_hdmi_ReadByteTPI(TPI_INTERRUPT_STATUS, &InterruptStatusImage);

    if ((InterruptStatusImage & HOT_PLUG_EVENT) || (InterruptStatusImage & HOT_PLUG_STATE))
    {
        HDMI_MSG("TP -> Hot Plug Event\n");

        msm_hdmi_ReadSetWriteTPI(TPI_INTERRUPT_EN, HOT_PLUG_EVENT);  // Enable HPD interrupt bit

        // Repeat this loop while cable is bouncing:
        do
        {
            msm_hdmi_WriteByteTPI(TPI_INTERRUPT_STATUS, HOT_PLUG_EVENT);
            msleep(T_HPD_DELAY); // Delay for metastability protection and to help filter out connection bouncing
            msm_hdmi_ReadByteTPI(TPI_INTERRUPT_STATUS, &InterruptStatusImage);    // Read Interrupt status register
        } while (InterruptStatusImage & HOT_PLUG_EVENT);              // loop as long as HP interrupts recur

        HDMI_MSG("InterruptStatusImage 0x%x, hdmiCableConnected 0x%x\n", InterruptStatusImage, hdmiCableConnected);

        //   sky_hdmi_set_isr(ON);

        if (((InterruptStatusImage & HOT_PLUG_STATE) >> 2) != hdmiCableConnected)
        {
            if (hdmiCableConnected == TRUE)
            {
                OnHdmiCableDisconnected();
            }
            else
            {
                ret_val = OnHdmiCableConnected();
                if(ret_val==HDMI_I2C_FAIL_E){
                    HDMI_MSG("\n\n\nOnHdmiCableConnected FAIL...... !!!!!!!!!!!!\n\n\n");
                    return HDMI_I2C_FAIL_E;
                }
            }
        }
    }

    HDMI_MSG("InterruptStatusImage 0x%x, dsRxPoweredUp 0x%x\n", InterruptStatusImage, dsRxPoweredUp);

#if 0
    // Check rx power
    if (((InterruptStatusImage & RX_SENSE_STATE) >> 3) != dsRxPoweredUp)
    {
        if (hdmiCableConnected == TRUE)
        {
            if (dsRxPoweredUp == TRUE)
            {
                OnDownstreamRxPoweredDown();
            }
            else
            {
                OnDownstreamRxPoweredUp();
            }
        }

        ClearInterrupt(RX_SENSE_EVENT);
    }
#else
    // Check rx power
    if(InterruptStatusImage & RX_SENSE_STATE)
    {
        OnDownstreamRxPoweredUp();
        ClearInterrupt(RX_SENSE_EVENT);
    }
#endif

    // Check if Audio Error event has occurred:
    if (InterruptStatusImage & AUDIO_ERROR_EVENT)
    {
        //HDMI_MSG (("TP -> Audio Error Event\n"));
        //  The hardware handles the event without need for host intervention (PR, p. 31)
        ClearInterrupt(AUDIO_ERROR_EVENT);
    }

#ifdef DEV_SUPPORT_HDCP
    //	HDCP_CheckStatus(InterruptStatusImage);
#endif

    return ret_val;
}


int sky_hdmi_init_cable_detect()
{
    byte InterruptStatusImage;

    printk("%s START \n", __func__);

    TPI_Init();
    EnableInterrupts(HOT_PLUG_EVENT | RX_SENSE_EVENT);
    msm_hdmi_ReadByteTPI(TPI_INTERRUPT_STATUS, &InterruptStatusImage);
    if((InterruptStatusImage & (HOT_PLUG_STATE|RX_SENSE_STATE)) == (HOT_PLUG_STATE|RX_SENSE_STATE)){
        set_connection_cable_flag(ON);
    }
    else{
        set_connection_cable_flag(OFF); //20100410
    }
    return InterruptStatusImage;
}

int sky_hdmi_dev_init()
{
    int ret_val=HDMI_SUCCESS_E;
    
    printk("%s START \n", __func__);

    ret_val = TPI_Init();
    if(ret_val)
        goto sky_hdmi_err;

    ret_val = sky_hdmi_HotPlugServiceLoop();
    if(ret_val) 
        goto sky_hdmi_err;

    ret_val = sky_hdmi_TPI_poll();
    if(ret_val) 
        goto sky_hdmi_err;

    printk("%s END \n", __func__);

    return HDMI_SUCCESS_E;

sky_hdmi_err:
    return ret_val;

}

void hdmi_power_statedown()
{
    DisableTMDS();
//    msm_hdmi_WriteByteTPI(TPI_SYSTEM_CONTROL_DATA_REG, TMDS_OUTPUT_CONTROL_POWER_DOWN);
//    msm_hdmi_WriteByteTPI(TPI_DEVICE_POWER_STATE_CTRL_REG, TX_POWER_STATE_D3);
}

static int32 hdmi_ddc_5v_power(int8 on)
{
	struct vreg *vreg = NULL;
	uint32 i = 0;
	int32 rc = 0;

    vreg = vreg_get(0, "boost");

    if (on){
        rc = vreg_set_level(vreg, 5000);  //5v  
        if (rc)
        goto power_fail;

        rc = vreg_enable(vreg);
        if (rc)
        goto power_fail;
    }else{
        rc = vreg_disable(vreg);
        if (rc)
        goto power_fail;
    }

	return 0;

power_fail:
	HDMI_MSG("%s failed! (%ld)(%d)(%ld)\n", __func__, rc, on, i);
	return rc;
}

static int32 hdmi_HD_12v_power(int8 on)
{
    unsigned mpp;
    struct vreg *vreg = NULL;
    int32 rc = 0;

    vreg = vreg_get(0, "msme2");

    if (on)
    {
        rc = vreg_set_level(vreg, 1200);  //1.2v
        if (rc)
            return FALSE;

        rc = vreg_enable(vreg);
        if (rc)
            return FALSE;

        HDMI_MSG("HD 1.2V set!!! \n");
    }

    mpp = 17;

    if (on)
    {
        rc = mpp_config_digital_out(mpp, MPP_CFG(MPP_DLOGIC_LVL_MSME, MPP_DLOGIC_OUT_CTRL_HIGH));
        HDMI_MSG("%s ret_val (%ld) \n", __FUNCTION__, rc);
    }
    else
    {// OFF
        rc = mpp_config_digital_out(mpp, MPP_CFG(MPP_DLOGIC_LVL_MSME, MPP_DLOGIC_OUT_CTRL_LOW));
        HDMI_MSG("%s ret_val (%ld) \n", __FUNCTION__, rc);
    }
    return TRUE;
}


static int32 hdmi_HD_33v_power(int8 on)
{
    unsigned mpp;
    int32 rc = 0;

    mpp = 12;

    if (on)
    {
        rc = mpp_config_digital_out(mpp, MPP_CFG(MPP_DLOGIC_LVL_MSME, MPP_DLOGIC_OUT_CTRL_HIGH));
        HDMI_MSG("%s ret_val (%ld) \n", __FUNCTION__, rc);
    }
    else
    {// OFF
        rc = mpp_config_digital_out(mpp, MPP_CFG(MPP_DLOGIC_LVL_MSME, MPP_DLOGIC_OUT_CTRL_LOW));
        HDMI_MSG("%s ret_val (%ld) \n", __FUNCTION__, rc);
    }
    return TRUE;
}



//de select.......
static int de_switch_to_hdmi()
{
    int rc = 0;

    rc = gpio_request(34, "sil9024");
    if (!rc) {
        //		rc = gpio_direction_output(34, 1);
        rc = gpio_direction_output(34, 0); //WS2ï¿½í¾¾í±¼ï¿½ ï¿½ï¿½ï¿½ï¿½..
        HDMI_MSG("HDMI :: DE switch Sil9024\n");
    }
    gpio_free(34);

    rc = gpio_request(57, "sil9024");
    if (!rc) {
        rc = gpio_direction_output(57, 1); //ES1ï¿½í¾¾í±¼ï¿½ ï¿½ï¿½ï¿½ï¿½..
	}
	gpio_free(57);
    
    return rc;
}

//de init....
static int de_switch_to_lcdc()
{
    int rc = 0;

    rc = gpio_request(34, "sil9024");
    if (!rc) {
        rc = gpio_direction_output(34, 1); //WS2ï¿½í¾¾í±¼ï¿½ ï¿½ï¿½ï¿½ï¿½..

          HDMI_MSG("HDMI :: DE switch LCDC\n");
	}
	gpio_free(34);

    rc = gpio_request(57, "sil9024");
    if (!rc) {
        rc = gpio_direction_output(57, 0); //ES1ï¿½í¾¾í±¼ï¿½ ï¿½ï¿½ï¿½ï¿½..
	}
	gpio_free(57);
    
	return rc;
}

static int msm_hdmi_power_off()
{
    int rc = 0;

    rc = gpio_request(31, "sil9024");
    if (!rc) {
        rc = gpio_direction_output(31, 0);
        HDMI_MSG("HDMI :: reset Low\n");
    }
    gpio_free(31);

    rc = gpio_request(34, "sil9024");
    if (!rc) {
        rc = gpio_direction_output(34, 0);
        HDMI_MSG("HDMI :: DE switch LCDC\n");
    }
    gpio_free(34);

    hdmi_ddc_5v_power(OFF);

    return rc;
}


static int sky_hdmi_gpio_reset()
{
   int rc = 0;

   HDMI_MSG("sky_hdmi_gpio_reset \n");

   rc = gpio_request(31, "sil9024");

   if (!rc) {
      rc = gpio_direction_output(31, 1);
      mdelay(T_HDMI_RESET_DELAY);
      rc = gpio_direction_output(31, 0);
      mdelay(T_HDMI_RESET_DELAY);
      rc = gpio_direction_output(31, 1);
   }

   gpio_free(31);
   return rc;
}

static int sky_hdmi_set_path()
{
	int rc = 0;

   HDMI_MSG("sky_hdmi_set_path \n");

//   sky_hdmi_set_isr(ON);
   hdmi_ddc_5v_power(ON);
   hdmi_HD_33v_power(ON);
   hdmi_HD_12v_power(ON);
   de_switch_to_hdmi();
   sky_hdmi_gpio_reset();
   return rc;
}

static int sky_hdmi_unset_path()
{
    int rc = 0;

    HDMI_MSG("sky_hdmi_unset_path \n");

    hdmi_power_statedown(); //0417 add...

    hdmi_ddc_5v_power(OFF);
    HDMI_MSG("sky_hdmi_unset_path: 3.3V,1.2V Power OFF \n");
    hdmi_HD_33v_power(OFF);
    hdmi_HD_12v_power(OFF);
    de_switch_to_lcdc();
    sky_hdmi_set_isr(OFF);
    set_connection_cable_flag(OFF); //ï¿½Ï´ï¿½ ï¿½ï¿½ï¿½â¼­ disconnectï¿½È°É·ï¿½.. 

    //sky_hdmi_gpio_reset(); //TODO :: reset pinï¿½ï¿½ default ï¿½ï¿½?? HIGH? LOW?? ï¿½ï¿½Æ° ï¿½ï¿½ï¿½ß¿ï¿½ ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½Æ¾ï¿½ ï¿½Ñ´ï¿½...
    return rc;
}



#if 0
void msm_hdmi_dev_Init()
{
    byte ErrCode = TRUE;

    HDMI_MSG("%s \n"__FUNCTION__);

//    ErrCode = msm_hdmi_InitSystem();
    sky_hdmi_dev_init();

#if 0     
    if(ErrCode == INIT_SYSTEM_SUCCESSFUL){
//        ErrCode = HotPlugServiceLoop();		// Loop until HPS loop successful
    }
    else{
      HDMI_MSG("init_system Err!!! \n");
    }
#endif    
}
#endif


#ifdef UNDEF_FB2_HDMI //modify.. fb2

static int msm_hdmi_ioctl(struct inode *inodep, struct file *filp, unsigned int cmd, unsigned long arg)
{
  //int i;
//  unsigned long flags;
  //unsigned long rc;
  unsigned int data_buffer_length = 0;
  //unsigned long data_buffer_length_adjusted = 0;
  void __user *argp = (void __user *)arg;
  //int err;
  //unsigned long size;

  HDMI_MSG("[HDMI_DEV] ioctl cmd [%x] enum[%d]\n", cmd, _IOC_NR(cmd));

  /* First copy down the buffer length */
  if (copy_from_user(&data_buffer_length, argp, sizeof(unsigned int)))
    return -EFAULT;

  if(_IOC_TYPE(cmd) != IOCTL_HDMI_MAGIC)
  {
    HDMI_MSG("[TDMB_DEV] invalid Magic Char [%c]\n", _IOC_TYPE(cmd));
    return -EINVAL;
  }
  if(_IOC_NR(cmd) >= IOCTL_HDMI_MAXNR)
  {
    return -EINVAL;
  }

  switch(cmd)
  {
    case FUNC_HDMI_INIT:      
//      msm_hdmi_dev_Init();
      break;

    case FUNC_HDMI_POWER_ON:
      sky_hdmi_set_path();
      break;

    case FUNC_HDMI_POWER_OFF:
      msm_hdmi_power_off();
      break;

    case FUNC_HDMI_TRANS_START:
      de_switch_to_hdmi();
      break;

    case FUNC_HDMI_TRANS_STOP:
      de_switch_to_lcdc();
      break;

    default:
      HDMI_MSG("[hdmi_ioctl] unknown command!!!\n");
      break;
  }

  HDMI_MSG("[hdmi_ioctl] end!!!\n");
  return 0;
}



static int msm_hdmi_release(struct inode *node, struct file *filep)
{
#if 0
	return msm_release_proc(filep, filep->private_data);
#else
    return 0;
#endif
}

static ssize_t msm_hdmi_read(struct file *filep, char __user *arg,
	size_t size, loff_t *loff)
{
	return 0;
}

static ssize_t msm_hdmi_write(struct file *filep, const char __user *arg,
	size_t size, loff_t *loff)
{
	return 0;
}

static int msm_hdmi_open(struct inode *inode, struct file *filep)
{
    return 0;
}

#endif //UNDEF_FB2_HDMI


#ifdef UNDEF_FB2_HDMI //modify.. fb2

static const struct file_operations msm_hdmi_fops = {
	.owner = THIS_MODULE,
	.open = msm_hdmi_open,
	.ioctl = msm_hdmi_ioctl,
	.release = msm_hdmi_release,
	.read = msm_hdmi_read,
	.write = msm_hdmi_write,
};

#endif //UNDEF_FB2_HDMI


int msm_hdmi_drv_start()
{
#ifdef UNDEF_FB2_HDMI //modify.. fb2
	int rc = -ENODEV;

	msm_hdmi_device = kzalloc(sizeof(struct msm_hdmi_dev), GFP_KERNEL);
	if (!msm_hdmi_device) {
		rc = -ENOMEM;
		goto start_done;
	}

	rc = alloc_chrdev_region(&msm_hdmi_devno, 0, 1, MSM_HDMI_DEV_NAME);
	if (rc < 0)
		goto start_region_fail;

	msm_hdmi_class = class_create(THIS_MODULE, MSM_HDMI_DEV_NAME);
	if (IS_ERR(msm_hdmi_class))
		goto start_class_fail;

    msm_hdmi_device->dev = device_create(msm_hdmi_class, NULL, 
                                                            msm_hdmi_devno, NULL, MSM_HDMI_DEV_NAME);
    if (!msm_hdmi_device->dev)
		goto start_io_fail;

    cdev_init(&msm_hdmi_device->cdev, &msm_hdmi_fops);
    msm_hdmi_device->cdev.owner = THIS_MODULE;

    rc = cdev_add(&msm_hdmi_device->cdev, msm_hdmi_devno, 1);
	if (rc < 0)
		goto start_cleanup_all;

    msm_hdmi_device->reset = MSM_HDMI_RESET_GPIO;

    sky_hdmi_i2c_drv_register();   

	HDMI_MSG("drive init \n");
	return 0;

start_cleanup_all:
	cdev_del(&msm_hdmi_device->cdev);
start_io_fail:
	class_destroy(msm_hdmi_class);
start_class_fail:
	unregister_chrdev_region(msm_hdmi_devno, 1);
start_region_fail:
	kfree(msm_hdmi_device);
	HDMI_MSG("FAIL: %s %s:%d\n", __FILE__, __func__, __LINE__);
start_done:
	HDMI_MSG("DONE: %s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;

#else

    sky_hdmi_i2c_drv_register();   
    return 0;
  
#endif //UNDEF_FB2_HDMI
}

#ifdef UNDEF_FB2_HDMI //modify.. fb2

int msm_hdmi_drv_remove(struct platform_device *dev)
{
    cdev_del(&(msm_hdmi_device->cdev));
    device_destroy(msm_hdmi_class, msm_hdmi_devno);  
    class_destroy(msm_hdmi_class);
    kfree(msm_hdmi_device);
    unregister_chrdev_region(msm_hdmi_devno, 1);
    return 0;
}

static int msm_hdmi_remove(struct platform_device *pdev)
{
	return msm_hdmi_drv_remove(pdev);
}


static int __init msm_hdmi_probe(struct platform_device *dev)
{
	 return 0;
}

static struct platform_driver msm_hdmi_driver = {
	.probe = msm_hdmi_probe,
	.remove	 = msm_hdmi_remove,
	.driver = {
		.name = MSM_HDMI_DEV_NAME,
		.owner = THIS_MODULE,
	},
};

#endif //UNDEF_FB2_HDMI


#ifdef UNDEF_FB2_HDMI //modify.. fb2

static int __init msm_hdmi_init(void)
{
    int rc;

    HDMI_MSG("module init :: msm_hdmi_init\n");

//    platform_driver_register(&msm_hdmi_driver);

    rc = msm_hdmi_drv_start();

    HDMI_MSG("module init end:: msm_hdmi_init\n");
  
    return rc;
}

#endif //UNDEF_FB2_HDMI


#if 0
static void __exit msm_hdmi_exit(void)
{
	platform_driver_unregister(&msm_hdmi_driver);
}
#endif



//////////////////////////////////////////////
//
//                APIs 
//
/////////////////////////////////////////////
int sky_hdmi_drv_init(void)
{
   int rc = 0;

   HDMI_MSG("%s\n", __FUNCTION__);
   sky_hdmi_i2c_drv_register();
   sky_hdmi_ddc_api_init();
   
   return rc;
}

int sky_hdmi_start(struct platform_device *dev)
{
   int rc = 0;

   HDMI_MSG("%s\n", __FUNCTION__);

   sky_hdmi_set_path();
   sky_hdmi_dev_init(); //test......
   return rc;
}


int sky_hdmi_test_start()
{
   int rc = 0;

   HDMI_MSG("%s\n", __FUNCTION__);

   sky_hdmi_set_path();
   sky_hdmi_dev_init(); //test......
   return rc;
}


int sky_hdmi_stop(struct platform_device *dev)
{
   int rc = 0;

   HDMI_MSG("%s\n", __FUNCTION__);
   sky_hdmi_unset_path();
   return rc;
}

// lived 20091119
int sky_hdmi_switch(int onoff)
{
    int ret_val=HDMI_SUCCESS_E;
    switch (onoff)
    {
    case 0: // off
        sky_hdmi_unset_path();
        break;
    case 1: // on 720p
        sky_hdmi_set_vid_mode(SKY_HDMI_720P_VID_MODE);
        sky_hdmi_set_path();
        ret_val=sky_hdmi_dev_init();
        if(ret_val)
            goto sky_hdmi_switch_err;
        
        break;
    case 2: // on 480p
        sky_hdmi_set_vid_mode(SKY_HDMI_480P_VID_MODE);
        sky_hdmi_set_path();
        ret_val=sky_hdmi_dev_init();
        if(ret_val)
            goto sky_hdmi_switch_err;
        break;
    case 10:    // only chip off
        hdmi_ddc_5v_power(OFF);
        HDMI_MSG("sky_hdmi_unset_path: 3.3V,1.2V Power OFF \n");
        hdmi_HD_33v_power(OFF);
        hdmi_HD_12v_power(OFF);
        sky_hdmi_set_isr(OFF);
        set_connection_cable_flag(OFF); //ï¿½Ï´ï¿½ ï¿½ï¿½ï¿½â¼­ disconnectï¿½È°É·ï¿½.. 
        break;
    case 11:    // only chip on
        hdmi_ddc_5v_power(ON);
        hdmi_HD_33v_power(ON);
        hdmi_HD_12v_power(ON);
        sky_hdmi_gpio_reset();
        sky_hdmi_init_cable_detect();        
        break;
    default:
        break;
    }
    return HDMI_SUCCESS_E;

sky_hdmi_switch_err:
    return ret_val;
    
}

void sky_hdmi_set_isr(int on_off)
{
  int irq;

   printk("%s \n", __FUNCTION__);

  if(on_off)
  {
    irq = gpio_to_irq(HDMI_INT);
    if (request_irq(irq, sky_hdmi_interrupt, IRQF_TRIGGER_FALLING, "sky_hdmi", NULL)) {
      printk("%s ISR ON \n", __FUNCTION__);
    }
  }
  else
  {
    free_irq(gpio_to_irq(HDMI_INT), NULL);
    printk("%s ISR Off \n", __FUNCTION__);
  }
}

#if 0    
void sky_hdmi_interrupt(void)
{

    byte status;

//    msm_hdmi_ReadByteTPI(TPI_INTERRUPT_STATUS, &status);

   printk("%s ,, status 0x%x \n", __FUNCTION__, status);

   if((status | HOT_PLUG_STATE) == 0x00){
   set_connection_cable_flag(OFF); 
}
   else{
       printk("%s ,, status 0x%x \n", __FUNCTION__, status);
   }
}
#else
static irqreturn_t sky_hdmi_interrupt(int irq, void *dev_id)
{ 
       printk("%s ............................... \n", __FUNCTION__);

	schedule_work(&sky_hdmi_work);
	return IRQ_HANDLED;
}
#endif    



///////////////////////////////////
//[[
#if 0

static int sky_hdmi_ddc_release(struct inode *node, struct file *filep)
{
    return 0;
}

static ssize_t sky_hdmi_ddc_read(struct file *filep, char __user *arg,
	size_t size, loff_t *loff)
{
	return 0;
}

static ssize_t sky_hdmi_ddc_write(struct file *filep, const char __user *arg,
	size_t size, loff_t *loff)
{
	return 0;
}

static int sky_hdmi_ddc_open(struct inode *inode, struct file *filep)
{
    return 0;
}

static const struct file_operations sky_hdmi_ddc_fops = {
	.owner = THIS_MODULE,
	.open = sky_hdmi_ddc_open,
	.release = sky_hdmi_ddc_release,
	.read = sky_hdmi_ddc_read,
	.write = sky_hdmi_ddc_write,
};

int sky_hdmi_ddc_drv_start(struct platform_device *dev)
{
	int rc = -ENODEV;

	if (!dev)
		return rc;

	sky_hdmi_ddc_dev = kzalloc(sizeof(struct sky_hdmi_ddc_dev), GFP_KERNEL);
	if (!sky_hdmi_ddc_dev) {
		rc = -ENOMEM;
		goto start_done;
	}

	rc = alloc_chrdev_region(&sky_hdmi_ddc_devno, 0, 1, SKY_HDMI_DDC_DEV_NAME);
	if (rc < 0)
		goto start_region_fail;

	sky_hdmi_ddc_class = class_create(THIS_MODULE, SKY_HDMI_DDC_DEV_NAME);
	if (IS_ERR(sky_hdmi_ddc_class))
		goto start_class_fail;

    sky_hdmi_ddc_dev->dev = device_create(sky_hdmi_ddc_class, NULL, 
                                                            sky_hdmi_ddc_devno, NULL, SKY_HDMI_DDC_DEV_NAME);
    if (!sky_hdmi_ddc_dev->dev)
		goto start_io_fail;

    cdev_init(&sky_hdmi_ddc_dev->cdev, &sky_hdmi_ddc_fops);
    sky_hdmi_ddc_dev->cdev.owner = THIS_MODULE;

    rc = cdev_add(&sky_hdmi_ddc_dev->cdev, sky_hdmi_ddc_devno, 1);
	if (rc < 0)
		goto start_cleanup_all;

    sky_hdmi_ddc_dev->reset = MSM_HDMI_RESET_GPIO;

    sky_hdmi_ddc_api_init();   

	HDMI_MSG("ddc drive init \n");
	return 0;

start_cleanup_all:
	cdev_del(&sky_hdmi_ddc_dev->cdev);
start_io_fail:
	class_destroy(sky_hdmi_ddc_class);
start_class_fail:
	unregister_chrdev_region(sky_hdmi_ddc_devno, 1);
start_region_fail:
	kfree(sky_hdmi_ddc_dev);
	HDMI_MSG("FAIL: %s %s:%d\n", __FILE__, __func__, __LINE__);
start_done:
	HDMI_MSG("DONE: %s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;
}

int sky_hdmi_ddc_drv_remove(struct platform_device *dev)
{
    cdev_del(&(sky_hdmi_ddc_dev->cdev));
    device_destroy(sky_hdmi_ddc_class, sky_hdmi_ddc_devno);  
    class_destroy(sky_hdmi_ddc_class);
    kfree(sky_hdmi_ddc_dev);
    unregister_chrdev_region(sky_hdmi_ddc_devno, 1);
    return 0;
}

static int sky_hdmi_ddc_remove(struct platform_device *pdev)
{
	return sky_hdmi_ddc_drv_remove(pdev);
}

static int __init sky_hdmi_ddc_probe(struct platform_device *dev)
{
	int rc;
	rc = sky_hdmi_ddc_drv_start(dev);

   HDMI_MSG("ddc probe :: msm_hdmi_ddc_probe\n");
  
	return rc;
}

static struct platform_driver sky_hdmi_ddc_driver = {
	.probe = sky_hdmi_ddc_probe,
	.remove	 = sky_hdmi_ddc_remove,
	.driver = {
		.name = SKY_HDMI_DDC_DEV_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init sky_hdmi_ddc_init(void)
{
   HDMI_MSG("module init :: msm_hdmi_ddc_init\n");
	return platform_driver_register(&sky_hdmi_ddc_driver);
}

static void __exit sky_hdmi_ddc_exit(void)
{
	platform_driver_unregister(&sky_hdmi_ddc_driver);
}
//]]

#endif // #if 0
//]]
////////////////////////////////////


//module_init(msm_hdmi_init);
//module_exit(msm_hdmi_exit);

