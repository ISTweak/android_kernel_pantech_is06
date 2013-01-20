#ifndef _SIL9024_DEV_H_
#define _SIL9024_DEV_H_


#ifdef UNDEF_FB2_HDMI //modify.. fb2

typedef enum
{
  FUNC_HDMI_INIT,
  FUNC_HDMI_POWER_ON,
  FUNC_HDMI_POWER_OFF,
  FUNC_HDMI_TRANS_START,
  FUNC_HDMI_TRANS_STOP,
  FUNC_HDMI_I2C_TEST,

  /* 이 위에 추가 해야 함.. */
  FUNC_HDMI_MAXNR
} sky_hdmi_func_name_type;

#define IOCTL_HDMI_INIT                  _IO(IOCTL_HDMI_MAGIC, FUNC_HDMI_INIT)
#define IOCTL_HDMI_POWER_ON             _IO(IOCTL_HDMI_MAGIC, FUNC_HDMI_POWER_ON)
#define IOCTL_HDMI_POWER_OFF            _IO(IOCTL_HDMI_MAGIC, FUNC_HDMI_POWER_OFF)
#define IOCTL_HDMI_TRANS_START         _IO(IOCTL_HDMI_MAGIC, FUNC_HDMI_TRANS_START)
#define IOCTL_HDMI_TRANS_STOP         _IO(IOCTL_HDMI_MAGIC, FUNC_HDMI_TRANS_STOP)
#define IOCTL_HDMI_I2C_TEST               _IO(IOCTL_HDMI_MAGIC, FUNC_HDMI_I2C_TEST)

#define IOCTL_HDMI_MAXNR  FUNC_HDMI_MAXNR


#endif //UNDEF_FB2_HDMI#define IOCTL_HDMI_MAGIC 't'

///////////////////////////////////////////////
//                                 TPI_Reg 
///////////////////////////////////////////////

// TPI Video Mode Data
//====================

#define	TPI_PIX_CLK_LSB			0x00
#define	TPI_PIX_CLK_MSB			0x01

#define	TPI_VERT_FREQ_LSB		0x02
#define	TPI_VERT_FREQ_MSB		0x03

#define	TPI_TOTAL_PIX_LSB		0x04
#define	TPI_TOTAL_PIX_MSB		0x05

#define	TPI_TOTAL_LINES_LSB		0x06
#define	TPI_TOTAL_LINES_MSB		0x07

// Pixel Repetition Data
//======================

#define	TPI_PIX_REPETITION		0x08

// TPI AVI Input and Output Format Data
//=====================================

#define	TPI_INPUT_FORMAT		0x09
#define	TPI_OUTPUT_FORMAT		0x0A

#define	TPI_RESERVED1			0x0B

// TPI AVI InfoFrame Data
//======================= 

#define	TPI_AVI_BYTE_0			0x0C
#define	TPI_AVI_BYTE_1			0x0D
#define	TPI_AVI_BYTE_2			0x0E
#define	TPI_AVI_BYTE_3			0x0F
#define	TPI_AVI_BYTE_4			0x10
#define	TPI_AVI_BYTE_5			0x11

#define TPI_AUDIO_BYTE_0		0xBF

#define	TPI_END_TOP_BAR_LSB		0x12
#define	TPI_END_TOP_BAR_MSB		0x13

#define	TPI_START_BTM_BAR_LSB	0x14
#define	TPI_START_BTM_BAR_MSB	0x15

#define	TPI_END_LEFT_BAR_LSB	0x16
#define	TPI_END_LEFT_BAR_MSB	0x17

#define	TPI_END_RIGHT_BAR_LSB	0x18
#define	TPI_END_RIGHT_BAR_MSB	0x19

// TPI System Control Data
//========================

#define	TPI_SYSTEM_CONTROL		0x1A

// TPI Identification Registers
//=============================

#define	TPI_DEVICE_ID			0x1B
#define	TPI_DEVICE_REV_ID		0x1C

#define	TPI_RESERVED2			0x1D

// TPI Device Power State Control Data
//====================================

#define	TPI_DEVICE_PWR_STATE	0x1E

// Configuration of I2S Interface
//===============================

#define	TPI_I2S_EN				0x1F
#define	TPI_I2S_IN_CFG			0x20

// Available only when TPI 0x26[7:6]=10 to select I2S input
//=========================================================
#define	TPI_I2S_CHST_0			0x21
#define	TPI_I2S_CHST_1			0x22
#define	TPI_I2S_CHST_2			0x23
#define	TPI_I2S_CHST_3			0x24
#define	TPI_I2S_CHST_4			0x25


// Available only when 0x26[7:6]=01
//=================================
#define	TPI_SPDIF_HEADER		0x24
#define	TPI_AUDIO_HANDLING		0x25

#define	TPI_AUDIO_INTERFACE		0x26
#define	TPI_AUDIO_SAMPLE_CTRL	0x27
#define	TPI_SPEAKER_CFG			0x28

#define	TPI_HDCP_QUERY_DATA		0x29
#define	TPI_HDCP_CTRL			0x2A

#define	TPI_BKSV_1				0x2B
#define	TPI_BKSV_2				0x2C
#define	TPI_BKSV_3				0x2D
#define	TPI_BKSV_4				0x2E
#define	TPI_BKSV_5				0x2F

#define	TPI_HDCP_REV			0x30
#define	TPI_V_PRIME_SELECTOR	0x31

// V' Value Readback Registers
//============================

#define	TPI_V_PRIME_7_0			0x32
#define	TPI_V_PRIME_15_9		0x33
#define	TPI_V_PRIME_23_16		0x34
#define	TPI_V_PRIME_31_24		0x35


// Aksv Value Readback Registers
//==============================

#define	TPI_AKSV_1				0x36
#define	TPI_AKSV_2				0x37
#define	TPI_AKSV_3				0x38
#define	TPI_AKSV_4				0x39
#define	TPI_AKSV_5				0x3A

#define	TPI_RESERVED3			0x3B

// Interrupt Service Registers
//============================
#define	TPI_INTERRUPT_EN		0x3C
#define	TPI_INTERRUPT_STATUS	0x3D

// Sync Register Configuration and Sync Monitoring Registers
//==========================================================

#define	TPI_SYNC_GEN_CTRL		0x60
#define	TPI_SYNC_POLAR_DETECT	0x61

// Explicit Sync DE Generator Registers (TPI 0x60[7]=0)
//=====================================================

#define	TPI_DE_DLY				0x62
#define	TPI_DE_CTRL				0x63
#define	TPI_DE_TOP				0x64

#define	TPI_RESERVED4			0x65

#define	TPI_DE_CNT_7_0			0x66
#define	TPI_DE_CNT_11_8			0x67

#define	TPI_DE_LIN_7_0			0x68
#define	TPI_DE_LIN_10_8			0x69

#define	TPI_DE_H_RES_7_0		0x6A
#define	TPI_DE_H_RES_10_8		0x6B

#define	TPI_DE_V_RES_7_0		0x6C
#define	TPI_DE_V_RES_10_8		0x6D

// Embedded Sync Register Set (TPI 0x60[7]=1)
//===========================================

#define	TPI_HBIT_TO_HSYNC_7_0	0x62
#define	TPI_HBIT_TO_HSYNC_9_8	0x63
#define	TPI_FIELD_2_OFFSET_7_0	0x64
#define	TPI_FIELD_2_OFFSET_11_8	0x65
#define	TPI_HWIDTH_7_0			0x66
#define	TPI_HWIDTH_8_9			0x67
#define	TPI_VBIT_TO_VSYNC    	0x68
#define	TPI_VBIT_TO_VSYNC    	0x68
#define	TPI_VWIDTH    			0x69

// TPI Enable Register
//====================

#define	TPI_ENABLE    			0xC7


#define  START_1ST_TPI_REG_BLOCK	0x00
#define  START_2ND_TPI_REG_BLOCK	0x60
#define  START_3RD_TPI_REG_BLOCK 0xBF

#define	SIZE_OF_1ST_TPI_BLOCK	0x3E
#define	SIZE_OF_2ND_TPI_BLOCK	0x0E
#define	SIZE_OF_3RD_TPI_BLOCK	0x20

// Generic Masks
//==============
#define HDMI_LOW_BYTE				0x00FF

#define HDMI_LOW_NIBBLE				0x0F
#define HDMI_HI_NIBBLE				0xF0

#define HDMI_MSBIT					0x80
#define HDMI_LSBIT					0x01

#define HDMI_BIT_0					0x01
#define HDMI_BIT_1					0x02
#define HDMI_BIT_2					0x04
#define HDMI_BIT_3					0x08
#define HDMI_BIT_4					0x10
#define HDMI_BIT_5					0x20
#define HDMI_BIT_6					0x40
#define HDMI_BIT_7					0x80
#define HDMI_TWO_LSBITS				0x03
#define HDMI_THREE_LSBITS			0x07
#define HDMI_FOUR_LSBITS				0x0F
#define HDMI_FIVE_LSBITS				0x1F
#define HDMI_SEVEN_LSBITS			0x7F
#define HDMI_TWO_MSBITS				0xC0
#define HDMI_EIGHT_BITS				0xFF
#define HDMI_BYTE_SIZE				0x08 

// Device ID
//==========
#define SiI_9022_9024_TX		0xb0

// Authentication States
//======================
#define NO_HDCP					0x00
#define AUTH_REQ				0x01
#define WAIT_FOR_RX				0x02
#define AUTHENTICATED			0x04

// Output States
//==============
#define DISCONNECTED			0x00
#define CHECK_EDID				0x01
#define CONNECTED				0x00

// Time Constants 
//===============
#define T_ONE_MILLISECOND	1000


enum HdmiErrCode{
    HDMI_SUCCESS_E=0,
    HDMI_INIT_FAIL_E=100,
    HDMI_I2C_FAIL_E=101    
};



// Error Codes
//============
enum ErrorMessages {
	INIT_SYSTEM_SUCCESSFUL,						// 0

	BLACK_BOX_OPEN_FAILURE,	
	BLACK_BOX_OPENED_SUCCESSFULLY,
	HW_RESET_FAILURE,		
	TPI_ENABLE_FAILURE,		
	INTERRUPT_EN_FAILURE,
	INTERRUPT_POLLING_FAILED,	

	NO_SINK_CONNECTED,		
	DDC_BUS_REQ_FAILURE,		
	HDCP_FAILURE,			
	HDCP_OK, 									// 10
	RX_AUTHENTICATED,			
	SINK_DOES_NOT_SUPPORT_HDCP,
	TX_DOES_NOT_SUPPORT_HDCP,

	ILLEGAL_AKSV,
	SET_PROTECTION_FAILURE,
	REVOKED_KEYS_FOUND,
	REPEATER_AUTHENTICATED,
	INT_STATUS_READ_FAILURE,

	PROTECTION_OFF_FAILED, 
	PROTECTION_ON_FAILED,						// 20
	INTERRUPT_POLLING_OK,

	EDID_PARSING_FAILURE,	
	VIDEO_SETUP_FAILURE,		
	TPI_READ_FAILURE,		
	TPI_WRITE_FAILURE,		

	INIT_VIDEO_FAILURE,		
	DE_CANNOT_BE_SET_WITH_EMBEDDED_SYNC,			
	SET_EMBEDDED_SYC_FAILURE,	
	V_MODE_NOT_SUPPORTED,	

	AUD_MODE_NOT_SUPPORTED,						// 30
	I2S_NOT_SET,				

	EDID_READ_FAILURE,		
//	EDID_CHECKSUM_ERROR,		
	INCORRECT_EDID_HEADER,	
//	EDID_EXT_TAG_ERROR,		

//	EDID_REV_ADDR_ERROR,		
//	EDID_V_DESCR_OVERFLOW,	
	INCORRECT_EDID_FILE,		
	UNKNOWN_EDID_TAG_CODE,	
	NO_DETAILED_DESCRIPTORS_IN_THIS_EDID_BLOCK,	// 40
	CONFIG_DATA_VALID,		
	CONFIG_DATA_INVALID,		

	GPIO_ACCESS_FAILED, 
	GPIO_CONFIG_ERROR,

	HP_EVENT_GOING_TO_SERVICE_LOOP,
	EDID_PARSED_OK,
	VIDEO_MODE_SET_OK,
	AUDIO_MODE_SET_OK,
	
	I2S_MAPPING_SUCCESSFUL,
	I2S_INPUT_CONFIG_SUCCESSFUL,				// 50
	I2S_HEADER_SET_SUCCESSFUL, 
	INTERRUPT_POLLING_SUCCESSFUL,
	
	HPD_LOOP_EXITED_SUCCESSFULY,
	HPD_LOOP_FAILED,
	SINK_CONNECTED,
	HP_EVENT_RETURNING_FROM_SERVICE_LOOP,
	AVI_INFOFRAMES_SETTING_FAILED,
	
	TMDS_ENABLING_FAILED, 
	DE_SET_OK,
	DE_SET_FAILED, 
	NO_STATUS_DATA,
	NO_861_EXTENSIONS
};

enum EDID_ErrorCodes {
        EDID_OK,
        EDID_INCORRECT_HEADER,
        EDID_CHECKSUM_ERROR,
        EDID_NO_861_EXTENSIONS,
        EDID_SHORT_DESCRIPTORS_OK,
        EDID_LONG_DESCRIPTORS_OK,
        EDID_EXT_TAG_ERROR,
        EDID_REV_ADDR_ERROR,
        EDID_V_DESCR_OVERFLOW,
        EDID_UNKNOWN_TAG_CODE,
        EDID_NO_DETAILED_DESCRIPTORS,
        EDID_DDC_BUS_REQ_FAILURE,
        EDID_DDC_BUS_RELEASE_FAILURE
};

typedef enum
{
  SKY_HDMI_720P_VID_MODE,
  SKY_HDMI_480P_VID_MODE    
} sky_hdmi_vid_mod_type;

// Interrupt Masks
//================
#define   HOT_PLUG_EVENT			0x01 
#define   RX_SENSE_EVENT			0x02
#define	HOT_PLUG_STATE			0x04
#define	RX_SENSE_STATE			0x08

#define	AUDIO_ERROR_EVENT		0x10
#define	SECURITY_CHANGE_EVENT	0x20
#define	V_READY_EVENT			0x40
#define	HDCP_CHANGE_EVENT		0x80

// TPI Control Masks
// =================
#define BIT_OUTPUT_MODE		0x01
#define BIT_DDC_BUS_GRANT	0x02
#define BIT_DDC_BUS_REQ		0x04
#define BIT_TMDS_OUTPUT		0x10

#define BITS_RELEASE_DDC	0x06

// TPI Power Control Masks
//========================
#define PWR_D0				0x00
#define PWR_D1				0x01
#define PWR_D2				0x02

// HDCP Control Masks
//===================
#define BIT_PROTECT_LEVEL	0x01
#define BIT_PROTECT_TYPE	0x02
#define BIT_REPEATER		0x08
#define BIT_LOCAL_PROTECT	0x40
#define BIT_EXT_PROTECT		0x80

// Pixel Repetition Masks
//=======================
#define BIT_BUS_24			0x20
#define BIT_EDGE_RISE		0x10

//Audio Maps
//==========
#define BIT_AUDIO_MUTE		0x10

// Input/Output Format Masks
//==========================

#define BITS_IN_RGB			0x00
#define BITS_IN_YCBCR444	0x01
#define BITS_IN_YCBCR422	0x02

#define BITS_IN_AUTO_RANGE	0x00
#define BITS_IN_FULL_RANGE	0x04
#define BITS_IN_LTD_RANGE	0x08

#define BIT_EN_DITHER_10_8	0x40
#define BIT_EXTENDED_MODE	0x80

#define BITS_OUT_RGB		0x00
#define BITS_OUT_YCBCR444	0x01
#define BITS_OUT_YCBCR422	0x02

#define BITS_OUT_AUTO_RANGE	0x00
#define BITS_OUT_FULL_RANGE	0x04
#define BITS_OUT_LTD_RANGE	0x08

#define BIT_BT_709			0x10


// DE Generator Masks
//===================
#define BIT_EN_DE_GEN		0x40

// Embedded Sync Masks
//====================
#define BIT_EN_SYNC_EXTRACT 0x40


// HDCP Revision
//==============
#define HDCP_REVISION		0x12

// Protection Levels
//==================
#define NO_PROTECTION		0x00
#define LOCAL_PROTECTION	0x01
#define EXTENDED_PROTECTION	0x03

// Audio Modes
//============
#define	AUD_PASS_BASIC		0x00
#define	AUD_PASS_ALL		0x01
#define	AUD_DOWN_SAMPLE		0x02
#define	AUD_DO_NOT_CHECK	0x03

#define	AUD_IF_SPDIF		0x40
#define	AUD_IF_I2S			0x80
	   
// Configuration File Constants
//=============================
#define CONFIG_DATA_LEN		0x10		
#define IDX_VIDEO_MODE		0x00		// Offset inf Config. file where video mode is stored

// TPI Bus Address
//================
#define TPI_BASE_ADDR		0x72		

// DDC Bus Addresses
//==================
#define HDCP_SLAVE_ADDR		0x74
#define EDID_ROM_ADDR		0xA0
#define DDC_BKSV_ADDR		0x00	// 5 bytes, Read only (RO), receiver KSV
#define DDC_Ri_ADDR			0x08	// Ack from receiver RO

#define DDC_AKSV_ADDR       0x10    // 5 bytes WO, transmitter KSV
#define DDC_AN_ADDR         0x18    
#define DDC_V_ADDR          0x20    // 20 bytes RO
#define DDC_BCAPS_ADDR      0x40    // Capabilities Status byte RO

#define DDC_BSTATUS_ADDR_L	0x41
#define DDC_BSTATUS_ADDR_H	0x42
#define DDC_KSV_FIFO_ADDR   0x43
#define BKSV_ARRAY_SIZE		 128

#define AKSV_SIZE			   5
#define NUM_OF_ONES_IN_KSV	  20	

// DDC Bus Bit Masks 
//==================
#define BIT_DDC_HDMI		0x80
#define BIT_DDC_REPEATER	0x40
#define BIT_DDC_FIFO_RDY	0x20
#define DEVICE_COUNT_MASK	0x7F

// EDID Constants
//===============
#define EDID_BLOCK_0_OFFSET 0x00
#define EDID_BLOCK_1_OFFSET 0x80
#define EDID_BLOCK_SIZE		 128
#define EDID_HDR_NO_OF_FF	0x06
#define NUM_OF_EXTEN_ADDR	0x7E

#define EDID_TAG_ADDR		0x00
#define EDID_REV_ADDR		0x01
#define EDID_TAG_IDX		0x02
#define LONG_DESCR_PTR_IDX	0x02
#define MISC_SUPPORT_IDX	0x03

#define AUDIO_D_BLOCK		0x01
#define VIDEO_D_BLOCK		0x02
#define VENDOR_SPEC_D_BLOCK	0x03
#define SPKR_ALLOC_D_BLOCK 	0x04
#define USE_EXTENDED_TAG    0x07

// Extended Data Block Tag Codes
//==============================
#define COLORIMETRY_D_BLOCK 0x05

#define HDMI_SIGNATURE_LEN	0x03

#define CEC_PHYS_ADDR_LEN	0x02
#define EDID_EXTENSION_TAG	0x02
#define EDID_REV_THREE		0x03
#define EDID_DATA_START		0x04

#define EDID_BLOCK_0		0x00
#define EDID_BLOCK_2_3      0x01

#define VIDEO_CAPABILITY_D_BLOCK 0x00


// KSV Buffer Size
//================
#define DEVICE_COUNT		 128	// May be tweaked as needed	

// Black Box Definitions
//======================
#define HW_RESET_GPIO		0x40

// Array Sizes
//============
#define MODE_TABLE_SIZE		  92

// Video Mode Table Constants
//===========================
#define NSM						0	// No Sub-Mode

#define ProgrVNegHNeg           0x00
#define ProgrVNegHPos           0x01
#define ProgrVPosHNeg 			0x02
#define ProgrVPosHPos           0x03

#define InterlaceVNegHNeg       0x04
#define InterlaceVPosHNeg       0x05
#define InterlaceVNgeHPos       0x06
#define InterlaceVPosHPos       0x07

#define PC_BASE					60

// Aspect ratio
//=============
#define _4						0	// 4:3
#define _4or16					1	// 4:3 or 16:9
#define _16						2	// 16:9

// InfoFrames
//===========
#define SIZE_AVI_INFOFRAME		14  	// including checksum byte
#define BITS_OUT_FORMAT			0x60	// Y1Y0 field

#define _4_To_3					0x10	// Aspect ratio - 4:3  in InfoFrame DByte 1
#define _16_To_9				0x20	// Aspect ratio - 16:9 in InfoFrame DByte 1
#define SAME_AS_AR				0x08	// R3R2R1R0 - in AVI InfoFrame DByte 2

#define BT_601					0x40
#define BT_709					0x80

#define	SIZE_AUDIO_INFOFRAME	0x0F

#define	EN_AUDIO_INFOFRAMES			0xC2
#define	TYPE_AUDIO_INFOFRAMES		0x84
#define	AUDIO_INFOFRAMES_VERSION	0x01
#define	AUDIO_INFOFRAMES_LENGTH		0x0A




// API Command Opcodes
//====================

#define SET_VIDEO_MODE			0x01
#define SET_AUDIO_MODE			0x02
#define MAP_I2S					0x03
#define CONFIG_I2S_INPUT		0x04
#define SET_I2S_STREAM_HEADER	0x05

#ifdef SiIMon				// For SiIMon API Testing

#define SET_VIDEO_ID			0x01
#define PIX_REP					0x02
#define IN_FORMAT				0x03
#define OUT_FORMAT				0x04
#define VBIT_TO_VSYNC			0x05
#define EN_DE_GEN				0x06

#define SET_AUDIO_CODING		0x07
#define AUDIO_CHANNEL_COUNT		0x08
#define AUDIO_SPKR_CONFIG		0x09
#define AUDIO_HANDLING			0x0A

#define I2S_SD0_MAPPING			0x0B
#define I2S_SD1_MAPPING			0x0C
#define I2S_SD2_MAPPING			0x0D
#define I2S_SD3_MAPPING			0x0E

#define CONFIG_I2S_IN			0x0F

#define SET_CHST_BYTE0			0x10
#define SET_CHST_BYTE1			0x11
#define SET_CHST_BYTE2			0x12
#define FS_CLK_ACCURACY			0x13
#define AUDIO_SAMPLE_LENGTH		0x14

#define GO						0xFF

#endif

// API Command Parameters
//=======================
#define VIDEO_SETUP_CMD_LEN		0x06
#define AUDIO_SETUP_CMD_LEN		0x04
#define I2S_MAPPING_CMD_LEN		0x04
#define CFG_I2S_INPUT_CMD_LEN	0x01
#define I2S_STREAM_HDR_CMD_LEN	0x05

#define SPDIF_INTERFACE			0x40
#define I2S_INTERFACE			0x80

// API Error Codes
//================
#define	ILLEGAL_COMMAND			0xFF

//Target def
#define HDMI_INT  110

///////////////////////////////////////////////////////
extern int sky_hdmi_drv_init(void);
extern int sky_hdmi_switch(int onoff);

extern int sky_hdmi_test_start(void);
extern void set_connection_cable_flag(int connect);

extern int sky_hdmi_fasync(int fd, struct file *file, int on);
//////////////////////////////////////////////////////

#endif //_SIL9024_DEV_H_
