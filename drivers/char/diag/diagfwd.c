/* Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/diagchar.h>
#include <mach/usbdiag.h>
#include <mach/msm_smd.h>
#include "diagmem.h"
#include "diagchar.h"
#include "diagfwd.h"
#include "diagchar_hdlc.h"


#ifdef FEATURE_SKY_FACTORY_COMMAND
/*  Sub Command 	*/
enum
{
FACTORY_CS_AUTO_INFO_I          = 0x01, // 0x01-A/S 羨呪??鯵昔?�左 溌昔
FACTORY_CS_AUTO_LAYER1_I        = 0x02, // 0x02-??��?�娃, �?仙持 ?�娃, �??�鍵 ?�呪
FACTORY_CS_AUTO_LAYER2_I        = 0x03, // 0x03-Slide(Folder) 鯵二 ?�呪, 賎希 �?�� ?�呪
FACTORY_CS_AUTO_LAYER3_I        = 0x04, // 0x04-?�五??姥疑 ?�娃, LCD Backlight 繊去?�娃
FACTORY_CS_AUTO_MEMORY_I        = 0x05, // 0x05-五乞�?紫遂 ?�勲
FACTORY_CS_AUTO_CALL1_I         = 0x06, // 0x06-????�� ?�娃/?�呪, ?�雌 ??�� ?�娃
FACTORY_CS_AUTO_CALL2_I         = 0x07, // 0x07-SMS ???�重 ?�呪, WAP 羨紗 ?�呪 // SMS_AUTO_TAKEOVER
FACTORY_CS_AUTO_CALL3_I         = 0x08, // 0x08-巷識?�斗???�娃, NO SVC/Call Drop ?�持 ?�呪
FACTORY_CS_AUTO_RESET_I         = 0x09, // 0x09-s/w reset, 穿据 ON/OFF ?�呪, h/w reset
FACTORY_CS_AUTO_ERRLOG_I        = 0x0A, // 0x0A-??�� 稽益
FACTORY_CS_AUTO_CALL4_I         = 0x0B, // 0x0B-Call drop �??�重 ?�鳶
FACTORY_CS_AUTO_CALL5_I         = 0x0C, // 0x0C-?�重 �??�重 失因
FACTORY_MSECTOR_WRITE_I         = 0x14, // 0x14-MSector Write Flag
FACTORY_MSECTOR_READ_I          = 0x15, // 0x15-MSector Read Flag
FACTORY_CAL_FLAG_I              = 0x1E, // 0x1E-Cal ?�戟食採 溌昔
FACTORY_FIRMWARE_VER_I          = 0x27, // 0x27-FIRMWARE ?�穿 溌昔
FACTORY_M3A_NAND_VER_I          = 0x28, // 0x28-2nd NAND/TCC Version ?�穿 溌昔
FACTORY_FINAL_I                 = 0x30, // 0x30-窒馬竺舛淫恵 段奄??
FACTORY_FINAL_CHECK_I           = 0x31, // 0x31-窒馬竺舛淫恵 溌昔
FACTORY_INIT_ALL_I              = 0x37, // 0x37-穿端 段奄??
FACTORY_INIT_RESET_I            = 0x38, // 0x38-段奄???�戟 ??H/W 軒実
FACTORY_INIT_SYS_CHECK          = 0x3A, // 0x3A-?��?奴�?奄鉢 溌昔
FACTORY_RSSIBAR_READ_I          = 0x3C, // 0x3C-RSSI BAR Read
FACTORY_OPENING_DAY_READ_I      = 0x40, // 0x40-Opening Day Read
FACTORY_EVDO_SET_I              = 0x46, // 0x46-EVDO Mode 穿発
FACTORY_EVDO_RELEASE_I          = 0x47, // 0x47-EVDO Mode ?�薦
FACTORY_EVDO_CHECK_I            = 0x48, // 0x48-EVDO Mode 溌昔
FACTORY_DMB_STANDBY_I           = 0x50, // 0x50-DMB TEST 乞球 穿発
FACTORY_DMB_TEST_START_I        = 0x51, // 0x51-DMB BER TEST ?�拙
FACTORY_DMB_MEASURE_I           = 0x52, // 0x52-DMB BER ?�舛 Status
FACTORY_DMB_BER_READ_I          = 0x53, // 0x53-DMB BER Read
FACTORY_DMB_TEST_FINISH_I       = 0x54, // 0x54-DMB TEST ?�戟
FACTORY_DMB_CAS_ID_I            = 0x55, // 0x55-DMB CAS ID 溌昔
FACTORY_BD_ADDR_RD_I            = 0x5A, // 0x5A-Bluetooth Device Address Read
FACTORY_BD_ADDR_WR_I            = 0x5B, // 0x5B-Bluetooth Device Address Write
FACTORY_BT_TEST_SET_I           = 0x5C, // 0x5C-Bluetooth 乞球 竺舛
FACTORY_BT_TEST_RELEASE_I       = 0x5D, // 0x5D-Bluetooth 乞球 ?�薦
FACTORY_BT_TEST_CHECK_I         = 0x5E, // 0x5E-Bluetooth 乞球 溌昔
FACTORY_WIFI_MODESET_I          = 0x60, // 0x60 WiFi 乞球 竺舛
FACTORY_WIFI_TXCTRL_I           = 0x61, // 0x61 WiFi Tx 竺舛
FACTORY_WIFI_RXRPT_I            = 0x62, // 0x62 WiFi Rx 竺舛
FACTORY_WIFI_RELEASE_I          = 0x63, // 0x63 WiFi 乞球 ?�薦
FACTORY_BOOT_CHECK_I            = 0x64, // 0x64-?�特???�戟?�嬢�?TEST亜�? 乞球?�走 溌昔
FACTORY_KEY_PRESS_I             = 0x6E, // 0x6E-Command???�遂馬食 徹研 ?�拙?�鉄
FACTORY_KEY_SCAN_I              = 0x6F, // 0x6F-�?�� ?�献 徹研 硝焼??
FACTORY_LCD_I                   = 0x73, // 0x73-LCD??On/Off
FACTORY_SPEAKER_I               = 0x78, // 0x78-Speaker???�遂??製据??窒径
FACTORY_IRDA_I                  = 0x7D, // 0x7D-IRDA Port�?汽戚???�呪??Test
FACTORY_BATTERY_BAR_I           = 0x87, // 0x87-Battery bar 鯵呪??硝焼??
FACTORY_BATTERY_ADC_I           = 0x88, // 0x88-Battery ADC Value
FACTORY_CUTOFF_SWITCH_I         = 0x89, // 0x89-Battery Cutoff ADC �?�� ?�醗失鉢
FACTORY_SLEEP_MODE_I            = 0x8C, // 0x8C-Sleep Mode Enable/Disable
FACTORY_TEMP_I                  = 0x91, // 0x91-紳�? ??Read
FACTORY_GPS_APP_MAP_CHECK_I     = 0x9A, // 0x9A-GPS App, Map ?��? 溌昔
FACTORY_CONTENTS_CHECK_I        = 0x9B, // 0x9B-Contents ?�雌 ?�巷 溌昔
FACTORY_AGPS_TEST_SET_I         = 0x9C, // 156-A-GPS C/No Test 乞球 竺舛
FACTORY_AGPS_TEST_CHECK_I       = 0x9D, // 157-A-GPS C/No Test 乞球 溌昔
FACTORY_AGPS_MEASURE_I          = 0x9E, // 158-A-GPS C/No?�舛??溌昔
FACTORY_AGPS_TEST_RELEASE_I     = 0x9F, // 159-A-GPS C/No Test 乞球 ?�薦
FACTORY_CHECKSUM_READ_I         = 0xA0, // 0xA0-SECTION �?CHECKSUM ?�至
FACTORY_USIM_MODE_SET_I         = 0xB0, // 0xB0-USIM Mode竺舛
FACTORY_USIM_MODE_CHECK_I       = 0xB1, // 0xB1-USIM Mode 溌昔
FACTORY_USIM_CARD_CHECK_I       = 0xB2, // 0xB2-USIM Card 溌昔
#ifdef FEATURE_PS_UIM_FTM
FACTORY_USIM_VALID_CHECK_I      = 0xB3,
#else
FACTORY_USIM_PERSO_CONTROL_I    = 0xB3, // 0xB3_ME Personalization????��
#endif
FACTORY_SDCARD_CHECK_I          = 0xB4, // 0xB4_Micro SD Card�??�拭 �?��/尻衣?�嬢 赤澗�?溌昔
FACTORY_SDMB_MODE_START_I       = 0xC0, // 0xC0-SDMB Test 乞球 竺舛
FACTORY_SDMB_PILOT_CHECK_I      = 0xC1, // 0xC1-SDMB Pilot Table 姥失溌昔
FACTORY_SDMB_MEASURE_I          = 0xC2, // 0xC2-SDMB ?�舛 Status 溌昔
FACTORY_SDMB_INFO_READ_I        = 0xC3, // 0xC3-SDMB ?�舛 ?�左 Read
FACTORY_SDMB_MODE_FINISH_I      = 0xC4, // 0xC4-SDMB Test 乞球 ?�薦
FACTORY_BAUDRATE_CHANGE_I       = 0xC8, // 0xC8-Baudrate change
FACTORY_VOLUME_MAX_I            = 0xC9, // 0xC9-製勲 段奄??
FACTORY_SERIAL_NUMBER_I         = 0xCA, // 0xCA-?�沙??乞�? ??��?�硲 Read/Write Command
FACTORY_MODEL_NAME_I            = 0xCB, // 0xCB-乞�?�?Read Command
FACTORY_PMIC_INIT_I             = 0xCC, // 0xCC-PMIC 段奄??誤敬.
FACTORY_PIN_CODE_READ_I         = 0xD2, // 0xD2-?�沙??PIN Code Read 誤敬
FACTORY_WCDMA_MIN_READ_I        = 0xD3, // 0xD3-?�呪??WCDMA乞�?�?穿鉢?�硲???�嬢神澗 誤敬
FACTORY_SHORTCUT_DIAL_I        = 0xD8, //0xD8- ���� Call connect command //FEATURE_CC_PANTECH_PCDT_SHORT_CALL
// Enum 240 ~ 254 ?�走�??�肖�?鯵降?�遂?�稽 ?�雁
FACTORY_WIFI_MAC_RD_I	        = 0xE0,	// WiFi Mac Read Command
FACTORY_WIFI_MAC_WR_I	        = 0xE1,	// WiFi Mac Write Command
FACTORY_MONITORING_ONOFF_I      = 0xF0,  /* 0xF0-?�沙??Usb�?serial diag??��???�鎧?�旋?�稽 乞艦?�元 汽戚?�亜 ?�持馬澗???�走馬澗 誤敬*/
FACTORY_ACTIVATION_DATE_READ_I  = 241,  // 0xF1-Helio??Activation Date Read Command
FACTORY_FUNCTION_FLAG_I         = 242,  // FUNCTION TEST??衣引�?PASS/FAIL???�鯉??企背
FACTORY_GS_COMPENSATION_I       = 243,  // GlideSensor Compensation Command
FACTORY_SYSTEM_PARAMETER_I      = 244, 	// System Parameter 溌昔 Command
FACTORY_MICP_CONTROL_I		      = 245,  // 1st, 2nd MIC Path Control(Enable/Disable)
FACTORY_PREF_MODE_I             = 247,  // 0xF7-Prefer Mode Command
FACTORY_BOOT_MODE_I             = 248,
FACTORY_BOOT_MODE_CHECK_I       = 249,
FACTORY_TWND_RD_I			          = 250,	// Touch Window Cal Value Read Command
FACTORY_TWND_WR_I			          = 251,	// Touch Window Cal Value Write Command
FACTORY_FREEZONE_CONTROL_I      = 0xFE, // //20090212 seobest | freezone test by AT cmd
	FACTORY_CAMERA_SN_READ_I        = 0xFF,  // 0xFF-CAMERA SN Read
	FACTORY_COMMAND_SIZE_I
};
/*  Sub Command - MAINTENANCE SECTOR */
enum
{
  FACTORY_MSECTOR_SETID_I,       // 00 - 0x00
  FACTORY_MSECTOR_PCBA_I,        // 01 - 0x01
  FACTORY_MSECTOR_ASSY_I,        // 02 - 0x02
  FACTORY_MSECTOR_TEST_I,        // 03 - 0x03
  FACTORY_MSECTOR_BSN_I,         // 04 - 0x04
  FACTORY_MSECTOR_MINPOWER_I,    // 05 - 0x05
  FACTORY_MSECTOR_MAXPOWER_I,    // 06 - 0x06
  FACTORY_MSECTOR_COMP1_I,       // 07 - 0x07
  FACTORY_MSECTOR_COMP2_I,       // 08 - 0x08
  FACTORY_MSECTOR_COMP3_I,       // 09 - 0x09
  FACTORY_MSECTOR_REPAIR1_I,     // 10 - 0x0A
  FACTORY_MSECTOR_REPAIR2_I,     // 11 - 0x0B
  FACTORY_MSECTOR_REPAIR3_I,     // 12 - 0x0C
  FACTORY_MSECTOR_AS1_I,         // 13 - 0x0D
  FACTORY_MSECTOR_AS2_I,         // 14 - 0x0E
  FACTORY_MSECTOR_AS3_I,         // 15 - 0x0F
  FACTORY_MSECTOR_MODEL_I,       // 16 - 0x10
  FACTORY_MSECTOR_PROCESS_FLAG_I,// 17 - 0x11
  
};

static int skyfwddiagcmd[] =
{ 
	FACTORY_MSECTOR_WRITE_I,
	FACTORY_MSECTOR_READ_I,
	FACTORY_CHECKSUM_READ_I,
	FACTORY_CAL_FLAG_I,
	FACTORY_BATTERY_ADC_I,
	FACTORY_CUTOFF_SWITCH_I,
	FACTORY_SLEEP_MODE_I,
	FACTORY_TEMP_I,
	FACTORY_AGPS_TEST_SET_I,
	FACTORY_AGPS_TEST_CHECK_I,
	FACTORY_AGPS_MEASURE_I,
	FACTORY_AGPS_TEST_RELEASE_I,
	FACTORY_USIM_CARD_CHECK_I,
	FACTORY_PMIC_INIT_I,
	FACTORY_FUNCTION_FLAG_I,
	FACTORY_SYSTEM_PARAMETER_I,
 	FACTORY_TWND_RD_I,
	FACTORY_TWND_WR_I,	

//--> [ds1_cp_factory_diag_protocol]
	FACTORY_FINAL_I,
	FACTORY_FINAL_CHECK_I,
	FACTORY_OPENING_DAY_READ_I,
	FACTORY_USIM_MODE_SET_I,
	FACTORY_USIM_MODE_CHECK_I,
#ifdef FEATURE_PS_UIM_FTM
  FACTORY_USIM_VALID_CHECK_I,
#endif /* FEATURE_PS_UIM_FTM */
	FACTORY_WCDMA_MIN_READ_I,
	FACTORY_USIM_PERSO_CONTROL_I,
	FACTORY_RSSIBAR_READ_I,
	FACTORY_CS_AUTO_CALL3_I,
	FACTORY_CS_AUTO_CALL4_I,
	FACTORY_CS_AUTO_CALL5_I,
	FACTORY_PREF_MODE_I,	
	FACTORY_SHORTCUT_DIAL_I,  //FEATURE_CC_PANTECH_PCDT_SHORT_CALL
//<-- [ds1_cp_factory_diag_protocol]

	0x00
};
#endif

MODULE_DESCRIPTION("Diag Char Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");

int diag_debug_buf_idx;
unsigned char diag_debug_buf[1024];

/* Number of maximum USB requests that the USB layer should handle at
   one time. */
#define MAX_DIAG_USB_REQUESTS 12
static unsigned int buf_tbl_size = 8; /*Number of entries in table of buffers */

#define CHK_OVERFLOW(bufStart, start, end, length) \
((bufStart <= start) && (end - start >= length)) ? 1 : 0

void __diag_smd_send_req(void)
{
	void *buf;

	if (driver->ch && (!driver->in_busy)) {
		int r = smd_read_avail(driver->ch);

	if (r > USB_MAX_IN_BUF) {
		if (r < MAX_BUF_SIZE) {
				printk(KERN_ALERT "\n diag: SMD sending in "
					   "packets upto %d bytes", r);
				driver->usb_buf_in = krealloc(
					driver->usb_buf_in, r, GFP_KERNEL);
		} else {
			printk(KERN_ALERT "\n diag: SMD sending in "
				 "packets more than %d bytes", MAX_BUF_SIZE);
			return;
		}
	}
		if (r > 0) {

			buf = driver->usb_buf_in;
			if (!buf) {
				printk(KERN_INFO "Out of diagmem for a9\n");
			} else {
				APPEND_DEBUG('i');
					smd_read(driver->ch, buf, r);
				APPEND_DEBUG('j');
				driver->usb_write_ptr->length = r;
				driver->in_busy = 1;
				diag_device_write(buf, MODEM_DATA);
			}
		}
	}
}

int diag_device_write(void *buf, int proc_num)
{
	int i, err = 0;

	if (driver->logging_mode == USB_MODE) {
		if (proc_num == APPS_DATA) {
			driver->usb_write_ptr_svc = (struct diag_request *)
			(diagmem_alloc(driver, sizeof(struct diag_request),
				 POOL_TYPE_USB_STRUCT));
			driver->usb_write_ptr_svc->length = driver->used;
			driver->usb_write_ptr_svc->buf = buf;
			err = diag_write(driver->usb_write_ptr_svc);
		} else if (proc_num == MODEM_DATA) {
				driver->usb_write_ptr->buf = buf;
#ifdef DIAG_DEBUG
				printk(KERN_INFO "writing data to USB,"
						 " pkt length %d \n",
				       driver->usb_write_ptr->length);
				print_hex_dump(KERN_DEBUG, "Written Packet Data"
					       " to USB: ", 16, 1, DUMP_PREFIX_
					       ADDRESS, buf, driver->
					       usb_write_ptr->length, 1);
#endif
			err = diag_write(driver->usb_write_ptr);
		} else if (proc_num == QDSP_DATA) {
			driver->usb_write_ptr_qdsp->buf = buf;
			err = diag_write(driver->usb_write_ptr_qdsp);
		}
		APPEND_DEBUG('k');
	} else if (driver->logging_mode == MEMORY_DEVICE_MODE) {
		if (proc_num == APPS_DATA) {
			for (i = 0; i < driver->poolsize_usb_struct; i++)
				if (driver->buf_tbl[i].length == 0) {
					driver->buf_tbl[i].buf = buf;
					driver->buf_tbl[i].length =
								 driver->used;
#ifdef DIAG_DEBUG
					printk(KERN_INFO "\n ENQUEUE buf ptr"
						   " and length is %x , %d\n",
						   (unsigned int)(driver->buf_
				tbl[i].buf), driver->buf_tbl[i].length);
#endif
					break;
				}
		}
		for (i = 0; i < driver->num_clients; i++)
			if (driver->client_map[i] == driver->logging_process_id)
				break;
		if (i < driver->num_clients) {
			driver->data_ready[i] |= MEMORY_DEVICE_LOG_TYPE;
			wake_up_interruptible(&driver->wait_q);
		} else
			return -EINVAL;
	} else if (driver->logging_mode == NO_LOGGING_MODE) {
		if (proc_num == MODEM_DATA) {
			driver->in_busy = 0;
			queue_work(driver->diag_wq, &(driver->
							diag_read_smd_work));
		} else if (proc_num == QDSP_DATA) {
			driver->in_busy_qdsp = 0;
			queue_work(driver->diag_wq, &(driver->
						diag_read_smd_qdsp_work));
		}
		err = -1;
	}
    return err;
}

void __diag_smd_qdsp_send_req(void)
{
	void *buf;

	if (driver->chqdsp && (!driver->in_busy_qdsp)) {
		int r = smd_read_avail(driver->chqdsp);

		if (r > USB_MAX_IN_BUF) {
			printk(KERN_INFO "diag dropped num bytes = %d\n", r);
			return;
		}
		if (r > 0) {
			buf = driver->usb_buf_in_qdsp;
			if (!buf) {
				printk(KERN_INFO "Out of diagmem for q6\n");
			} else {
				APPEND_DEBUG('l');
					smd_read(driver->chqdsp, buf, r);
				APPEND_DEBUG('m');
				driver->usb_write_ptr_qdsp->length = r;
				driver->in_busy_qdsp = 1;
				diag_device_write(buf, QDSP_DATA);
			}
		}

	}
}

static void diag_print_mask_table(void)
{
/* Enable this to print mask table when updated */
#ifdef MASK_DEBUG
	int first;
	int last;
	uint8_t *ptr = driver->msg_masks;
	int i = 0;

	while (*(uint32_t *)(ptr + 4)) {
		first = *(uint32_t *)ptr;
		ptr += 4;
		last = *(uint32_t *)ptr;
		ptr += 4;
		printk(KERN_INFO "SSID %d - %d\n", first, last);
		for (i = 0 ; i <= last - first ; i++)
			printk(KERN_INFO "MASK:%x\n", *((uint32_t *)ptr + i));
		ptr += ((last - first) + 1)*4;

	}
#endif
}

static void diag_update_msg_mask(int start, int end , uint8_t *buf)
{
	int found = 0;
	int first;
	int last;
	uint8_t *ptr = driver->msg_masks;
	uint8_t *ptr_buffer_start = &(*(driver->msg_masks));
	uint8_t *ptr_buffer_end = &(*(driver->msg_masks)) + MSG_MASK_SIZE;

	mutex_lock(&driver->diagchar_mutex);
	/* First SSID can be zero : So check that last is non-zero */

	while (*(uint32_t *)(ptr + 4)) {
		first = *(uint32_t *)ptr;
		ptr += 4;
		last = *(uint32_t *)ptr;
		ptr += 4;
		if (start >= first && start <= last) {
			ptr += (start - first)*4;
			if (end <= last)
				if (CHK_OVERFLOW(ptr_buffer_start, ptr,
						  ptr_buffer_end,
						  (((end - start)+1)*4)))
					memcpy(ptr, buf , ((end - start)+1)*4);
				else
					printk(KERN_CRIT "Not enough"
							 " buffer space for"
							 " MSG_MASK\n");
			else
				printk(KERN_INFO "Unable to copy"
						 " mask change\n");

			found = 1;
			break;
		} else {
			ptr += ((last - first) + 1)*4;
		}
	}
	/* Entry was not found - add new table */
	if (!found) {
		if (CHK_OVERFLOW(ptr_buffer_start, ptr, ptr_buffer_end,
				  8 + ((end - start) + 1)*4)) {
			memcpy(ptr, &(start) , 4);
			ptr += 4;
			memcpy(ptr, &(end), 4);
			ptr += 4;
			memcpy(ptr, buf , ((end - start) + 1)*4);
		} else
			printk(KERN_CRIT " Not enough buffer"
					 " space for MSG_MASK\n");
	}
	mutex_unlock(&driver->diagchar_mutex);
	diag_print_mask_table();

}

static void diag_update_event_mask(uint8_t *buf, int toggle, int num_bits)
{
	uint8_t *ptr = driver->event_masks;
	uint8_t *temp = buf + 2;

	mutex_lock(&driver->diagchar_mutex);
	if (!toggle)
		memset(ptr, 0 , EVENT_MASK_SIZE);
	else
		if (CHK_OVERFLOW(ptr, ptr,
				 ptr+EVENT_MASK_SIZE,
				  num_bits/8 + 1))
			memcpy(ptr, temp , num_bits/8 + 1);
		else
			printk(KERN_CRIT "Not enough buffer space "
					 "for EVENT_MASK\n");
	mutex_unlock(&driver->diagchar_mutex);
}

static void diag_update_log_mask(uint8_t *buf, int num_items)
{
	uint8_t *ptr = driver->log_masks;
	uint8_t *temp = buf;

	mutex_lock(&driver->diagchar_mutex);
	if (CHK_OVERFLOW(ptr, ptr, ptr + LOG_MASK_SIZE,
				  (num_items+7)/8))
		memcpy(ptr, temp , (num_items+7)/8);
	else
		printk(KERN_CRIT " Not enough buffer space for LOG_MASK\n");
	mutex_unlock(&driver->diagchar_mutex);
}

static void diag_update_pkt_buffer(unsigned char *buf)
{
	unsigned char *ptr = driver->pkt_buf;
	unsigned char *temp = buf;

	mutex_lock(&driver->diagchar_mutex);
	if (CHK_OVERFLOW(ptr, ptr, ptr + PKT_SIZE, driver->pkt_length))
		memcpy(ptr, temp , driver->pkt_length);
	else
		printk(KERN_CRIT " Not enough buffer space for PKT_RESP\n");
	mutex_unlock(&driver->diagchar_mutex);
}

void diag_update_userspace_clients(unsigned int type)
{
	int i;

	mutex_lock(&driver->diagchar_mutex);
	for (i = 0; i < driver->num_clients; i++)
		if (driver->client_map[i] != 0)
			driver->data_ready[i] |= type;
	wake_up_interruptible(&driver->wait_q);
	mutex_unlock(&driver->diagchar_mutex);
}

void diag_update_sleeping_process(int process_id)
{
	int i;

	mutex_lock(&driver->diagchar_mutex);
	for (i = 0; i < driver->num_clients; i++)
		if (driver->client_map[i] == process_id) {
			driver->data_ready[i] |= PKT_TYPE;
			break;
		}
	wake_up_interruptible(&driver->wait_q);
	mutex_unlock(&driver->diagchar_mutex);
}

static int diag_process_apps_pkt(unsigned char *buf, int len)
{
	uint16_t start;
	uint16_t end, subsys_cmd_code;
	int i, cmd_code, subsys_id;
	int packet_type = 1;
	unsigned char *temp = buf;

	/* event mask */
	if ((*buf == 0x60) && (*(++buf) == 0x0)) {
		diag_update_event_mask(buf, 0, 0);
		diag_update_userspace_clients(EVENT_MASKS_TYPE);
	}
	/* check for set event mask */
	else if (*buf == 0x82) {
		buf += 4;
		diag_update_event_mask(buf, 1, *(uint16_t *)buf);
		diag_update_userspace_clients(
		EVENT_MASKS_TYPE);
	}
	/* log mask */
	else if (*buf == 0x73) {
		buf += 4;
		if (*(int *)buf == 3) {
			buf += 8;
			diag_update_log_mask(buf+4, *(int *)buf);
			diag_update_userspace_clients(LOG_MASKS_TYPE);
		}
	}
	/* Check for set message mask  */
	else if ((*buf == 0x7d) && (*(++buf) == 0x4)) {
		buf++;
		start = *(uint16_t *)buf;
		buf += 2;
		end = *(uint16_t *)buf;
		buf += 4;
		diag_update_msg_mask((uint32_t)start, (uint32_t)end , buf);
		diag_update_userspace_clients(MSG_MASKS_TYPE);
	}
	/* Set all run-time masks
	if ((*buf == 0x7d) && (*(++buf) == 0x5)) {
		TO DO
	} */

	/* Check for registered clients and forward packet to user-space */
	else{
		cmd_code = (int)(*(char *)buf);
		temp++;
		subsys_id = (int)(*(char *)temp);
		temp++;
		subsys_cmd_code = *(uint16_t *)temp;
		temp += 2;

#ifdef FEATURE_SKY_FACTORY_COMMAND
		i=0;

		/* Added DS2 kbkim */
		if(cmd_code == 0xFA) { /* bug_fix: DIAG_FACTORY_REQ_F */
			while(skyfwddiagcmd[i] != 0)
			{	
				if(subsys_id == skyfwddiagcmd[i++])
				{
					//printk("mArm Fwd id: %d\n", subsys_id );
					return packet_type;;
				}
			}
		}
#endif

		for (i = 0; i < diag_max_registration; i++) {
			if (driver->table[i].process_id != 0) {
				if (driver->table[i].cmd_code ==
				     cmd_code && driver->table[i].subsys_id ==
				     subsys_id &&
				    driver->table[i].cmd_code_lo <=
				     subsys_cmd_code &&
					  driver->table[i].cmd_code_hi >=
				     subsys_cmd_code){
					driver->pkt_length = len;
					diag_update_pkt_buffer(buf);
					diag_update_sleeping_process(
						driver->table[i].process_id);
						return 0;
				    } /* end of if */
				else if (driver->table[i].cmd_code == 255
					  && cmd_code == 75) {
					if (driver->table[i].subsys_id ==
					    subsys_id &&
					   driver->table[i].cmd_code_lo <=
					    subsys_cmd_code &&
					     driver->table[i].cmd_code_hi >=
					    subsys_cmd_code){
						driver->pkt_length = len;
						diag_update_pkt_buffer(buf);
						diag_update_sleeping_process(
							driver->table[i].
							process_id);
						return 0;
					}
				} /* end of else-if */
				else if (driver->table[i].cmd_code == 255 &&
					  driver->table[i].subsys_id == 255) {
					if (driver->table[i].cmd_code_lo <=
							 cmd_code &&
						     driver->table[i].
						    cmd_code_hi >= cmd_code){
						driver->pkt_length = len;
						diag_update_pkt_buffer(buf);
						diag_update_sleeping_process
							(driver->table[i].
							 process_id);
						return 0;
					}
				} /* end of else-if */
			} /* if(driver->table[i].process_id != 0) */
		}  /* for (i = 0; i < diag_max_registration; i++) */
	} /* else */
		return packet_type;
}

void diag_process_hdlc(void *data, unsigned len)
{
	struct diag_hdlc_decode_type hdlc;
	int ret, type = 0;
#ifdef DIAG_DEBUG
	int i;
	printk(KERN_INFO "\n HDLC decode function, len of data  %d\n", len);
#endif
	hdlc.dest_ptr = driver->hdlc_buf;
	hdlc.dest_size = USB_MAX_OUT_BUF;
	hdlc.src_ptr = data;
	hdlc.src_size = len;
	hdlc.src_idx = 0;
	hdlc.dest_idx = 0;
	hdlc.escaping = 0;

	ret = diag_hdlc_decode(&hdlc);

	if (ret)
		type = diag_process_apps_pkt(driver->hdlc_buf,
							  hdlc.dest_idx - 3);
	else if (driver->debug_flag) {
		printk(KERN_ERR "Packet dropped due to bad HDLC coding/CRC"
				" errors or partial packet received, packet"
				" length = %d\n", len);
		print_hex_dump(KERN_DEBUG, "Dropped Packet Data: ", 16, 1,
					   DUMP_PREFIX_ADDRESS, data, len, 1);
		driver->debug_flag = 0;
	}
#ifdef DIAG_DEBUG
	printk(KERN_INFO "\n hdlc.dest_idx = %d \n", hdlc.dest_idx);
	for (i = 0; i < hdlc.dest_idx; i++)
		printk(KERN_DEBUG "\t%x", *(((unsigned char *)
							driver->hdlc_buf)+i));
#endif
	/* ignore 2 bytes for CRC, one for 7E and send */
	if ((driver->ch) && (ret) && (type) && (hdlc.dest_idx > 3)) {
		APPEND_DEBUG('g');
		smd_write(driver->ch, driver->hdlc_buf, hdlc.dest_idx - 3);
		APPEND_DEBUG('h');
#ifdef DIAG_DEBUG
		printk(KERN_INFO "writing data to SMD, pkt length %d \n", len);
		print_hex_dump(KERN_DEBUG, "Written Packet Data to SMD: ", 16,
			       1, DUMP_PREFIX_ADDRESS, data, len, 1);
#endif
	}

}

int diagfwd_connect(void)
{
	int err;

	printk(KERN_DEBUG "diag: USB connected\n");
	err = diag_open(driver->poolsize + 3); /* 2 for A9 ; 1 for q6*/
	if (err)
		printk(KERN_ERR "diag: USB port open failed");
	driver->usb_connected = 1;
	driver->in_busy = 0;
	driver->in_busy_qdsp = 0;

	/* Poll SMD channels to check for data*/
	queue_work(driver->diag_wq, &(driver->diag_read_smd_work));
	queue_work(driver->diag_wq, &(driver->diag_read_smd_qdsp_work));

	driver->usb_read_ptr->buf = driver->usb_buf_out;
	driver->usb_read_ptr->length = USB_MAX_OUT_BUF;
	APPEND_DEBUG('a');
	diag_read(driver->usb_read_ptr);
	APPEND_DEBUG('b');
	return 0;
}

int diagfwd_disconnect(void)
{
	printk(KERN_DEBUG "diag: USB disconnected\n");
	driver->usb_connected = 0;
	driver->in_busy = 1;
	driver->in_busy_qdsp = 1;
	driver->debug_flag = 1;
	diag_close();
	/* TBD - notify and flow control SMD */
	return 0;
}

int diagfwd_write_complete(struct diag_request *diag_write_ptr)
{
	unsigned char *buf = diag_write_ptr->buf;
	/*Determine if the write complete is for data from arm9/apps/q6 */
	/* Need a context variable here instead */
	if (buf == (void *)driver->usb_buf_in) {
		driver->in_busy = 0;
		APPEND_DEBUG('o');
		queue_work(driver->diag_wq, &(driver->diag_read_smd_work));
	} else if (buf == (void *)driver->usb_buf_in_qdsp) {
		driver->in_busy_qdsp = 0;
		APPEND_DEBUG('p');
		queue_work(driver->diag_wq, &(driver->diag_read_smd_qdsp_work));
	} else {
		diagmem_free(driver, (unsigned char *)buf, POOL_TYPE_HDLC);
		diagmem_free(driver, (unsigned char *)diag_write_ptr,
							 POOL_TYPE_USB_STRUCT);
		APPEND_DEBUG('q');
	}
	return 0;
}

int diagfwd_read_complete(struct diag_request *diag_read_ptr)
{
	int len = diag_read_ptr->actual;

	APPEND_DEBUG('c');
#ifdef DIAG_DEBUG
	printk(KERN_INFO "read data from USB, pkt length %d \n",
		    diag_read_ptr->actual);
	print_hex_dump(KERN_DEBUG, "Read Packet Data from USB: ", 16, 1,
		       DUMP_PREFIX_ADDRESS, diag_read_ptr->buf,
		       diag_read_ptr->actual, 1);
#endif
	driver->read_len = len;
	if (driver->logging_mode == USB_MODE)
		queue_work(driver->diag_wq , &(driver->diag_read_work));
	return 0;
}

static struct diag_operations diagfwdops = {
	.diag_connect = diagfwd_connect,
	.diag_disconnect = diagfwd_disconnect,
	.diag_char_write_complete = diagfwd_write_complete,
	.diag_char_read_complete = diagfwd_read_complete
};

static void diag_smd_notify(void *ctxt, unsigned event)
{
	queue_work(driver->diag_wq, &(driver->diag_read_smd_work));
}

#if defined(CONFIG_MSM_N_WAY_SMD)
static void diag_smd_qdsp_notify(void *ctxt, unsigned event)
{
	queue_work(driver->diag_wq, &(driver->diag_read_smd_qdsp_work));
}
#endif

static int diag_smd_probe(struct platform_device *pdev)
{
	int r = 0;

	if (pdev->id == 0) {
		if (driver->usb_buf_in == NULL &&
			(driver->usb_buf_in =
			kzalloc(USB_MAX_IN_BUF, GFP_KERNEL)) == NULL)

			goto err;
		else

		r = smd_open("DIAG", &driver->ch, driver, diag_smd_notify);
	}
#if defined(CONFIG_MSM_N_WAY_SMD)
	if (pdev->id == 1) {
		if (driver->usb_buf_in_qdsp == NULL &&
			(driver->usb_buf_in_qdsp =
			kzalloc(USB_MAX_IN_BUF, GFP_KERNEL)) == NULL)

			goto err;
		else

		r = smd_named_open_on_edge("DIAG", SMD_APPS_QDSP,
			&driver->chqdsp, driver, diag_smd_qdsp_notify);

	}
#endif
	printk(KERN_INFO "diag opened SMD port ; r = %d\n", r);

err:
	return 0;
}

static struct platform_driver msm_smd_ch1_driver = {

	.probe = diag_smd_probe,
	.driver = {
		   .name = "DIAG",
		   .owner = THIS_MODULE,
		   },
};

void diag_read_work_fn(struct work_struct *work)
{
	APPEND_DEBUG('d');
	diag_process_hdlc(driver->usb_buf_out, driver->read_len);
	driver->usb_read_ptr->buf = driver->usb_buf_out;
	driver->usb_read_ptr->length = USB_MAX_OUT_BUF;
	APPEND_DEBUG('e');
	diag_read(driver->usb_read_ptr);
	APPEND_DEBUG('f');
}

void diagfwd_init(void)
{
	diag_debug_buf_idx = 0;
	if (driver->usb_buf_out  == NULL &&
	     (driver->usb_buf_out = kzalloc(USB_MAX_OUT_BUF,
					 GFP_KERNEL)) == NULL)
		goto err;
	if (driver->hdlc_buf == NULL
	    && (driver->hdlc_buf = kzalloc(HDLC_MAX, GFP_KERNEL)) == NULL)
		goto err;
	if (driver->msg_masks == NULL
	    && (driver->msg_masks = kzalloc(MSG_MASK_SIZE,
					     GFP_KERNEL)) == NULL)
		goto err;
	if (driver->log_masks == NULL &&
	    (driver->log_masks = kzalloc(LOG_MASK_SIZE, GFP_KERNEL)) == NULL)
		goto err;
	if (driver->event_masks == NULL &&
	    (driver->event_masks = kzalloc(EVENT_MASK_SIZE,
					    GFP_KERNEL)) == NULL)
		goto err;
	if (driver->client_map == NULL &&
	    (driver->client_map = kzalloc
	     ((driver->num_clients) * 4, GFP_KERNEL)) == NULL)
		goto err;
	if (driver->buf_tbl == NULL)
			driver->buf_tbl = kzalloc(buf_tbl_size *
			  sizeof(struct diag_write_device), GFP_KERNEL);
	if (driver->buf_tbl == NULL)
		goto err;
	if (driver->data_ready == NULL &&
	     (driver->data_ready = kzalloc(driver->num_clients * 4,
					    GFP_KERNEL)) == NULL)
		goto err;
	if (driver->table == NULL &&
	     (driver->table = kzalloc(diag_max_registration*
				      sizeof(struct diag_master_table),
				       GFP_KERNEL)) == NULL)
		goto err;
	if (driver->usb_write_ptr == NULL)
			driver->usb_write_ptr = kzalloc(
				sizeof(struct diag_request), GFP_KERNEL);
			if (driver->usb_write_ptr == NULL)
					goto err;
	if (driver->usb_write_ptr_qdsp == NULL)
			driver->usb_write_ptr_qdsp = kzalloc(
				sizeof(struct diag_request), GFP_KERNEL);
			if (driver->usb_write_ptr_qdsp == NULL)
					goto err;
	if (driver->usb_read_ptr == NULL)
			driver->usb_read_ptr = kzalloc(
				sizeof(struct diag_request), GFP_KERNEL);
			if (driver->usb_read_ptr == NULL)
				goto err;
	if (driver->pkt_buf == NULL &&
	     (driver->pkt_buf = kzalloc(PKT_SIZE,
					 GFP_KERNEL)) == NULL)
		goto err;

	driver->diag_wq = create_singlethread_workqueue("diag_wq");
	INIT_WORK(&(driver->diag_read_work), diag_read_work_fn);

	diag_usb_register(&diagfwdops);

	platform_driver_register(&msm_smd_ch1_driver);

	return;
err:
		printk(KERN_INFO "\n Could not initialize diag buffers\n");
		kfree(driver->usb_buf_out);
		kfree(driver->hdlc_buf);
		kfree(driver->msg_masks);
		kfree(driver->log_masks);
		kfree(driver->event_masks);
		kfree(driver->client_map);
		kfree(driver->buf_tbl);
		kfree(driver->data_ready);
		kfree(driver->table);
		kfree(driver->pkt_buf);
		kfree(driver->usb_write_ptr);
		kfree(driver->usb_write_ptr_qdsp);
		kfree(driver->usb_read_ptr);
}

void diagfwd_exit(void)
{
	smd_close(driver->ch);
	smd_close(driver->chqdsp);
	driver->ch = 0;		/*SMD can make this NULL */
	driver->chqdsp = 0;

	if (driver->usb_connected)
		diag_close();

	platform_driver_unregister(&msm_smd_ch1_driver);

	diag_usb_unregister();

	kfree(driver->usb_buf_in);
	kfree(driver->usb_buf_in_qdsp);
	kfree(driver->usb_buf_out);
	kfree(driver->hdlc_buf);
	kfree(driver->msg_masks);
	kfree(driver->log_masks);
	kfree(driver->event_masks);
	kfree(driver->client_map);
	kfree(driver->buf_tbl);
	kfree(driver->data_ready);
	kfree(driver->table);
	kfree(driver->pkt_buf);
	kfree(driver->usb_write_ptr);
	kfree(driver->usb_write_ptr_qdsp);
	kfree(driver->usb_read_ptr);
}
