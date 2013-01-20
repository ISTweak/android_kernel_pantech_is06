/* drivers/usb/function/whcm.c
 *
 * SKT OEM(DEVGURU) Function Driver
 * 2009.08.14
 * kim.sanghoun@pantech.com
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>

#include "usb_function.h"

struct whcm_cch_func_descriptor {
	__u8 	bLength;
	__u8	bDescriptorType;
	__u8	bDescriptorSubType;
	__u16	bcdCDC;
} __attribute__ ((packed));

struct whcm_func_descriptor {
	__u8 	bLength;
	__u8	bDescriptorType;
	__u8	bDescriptorSubType;
	__u16	bcdVersion;
} __attribute__ ((packed));

struct whcm_ccu_func_descriptor {
	__u8 	bLength;
	__u8	bDescriptorType;
	__u8	bDescriptorSubType;
	__u8	bControlInterface;
	__u8	bSubordinateInterface0;
	__u8	bSubordinateInterface1;
	__u8	bSubordinateInterface2;
} __attribute__ ((packed));

static struct usb_interface_descriptor whcm_intf_desc = {
	.bLength            =	0x09,
	.bDescriptorType    =	USB_DT_INTERFACE,
	.bInterfaceNumber		= 0x00,
	.bAlternateSetting	= 0x00,
	.bNumEndpoints      =	0,
	.bInterfaceClass    =	0x02,
	.bInterfaceSubClass =	0x08,
	.bInterfaceProtocol =	0x00,
	.iInterface					= 0x00,//0x04
};


static struct whcm_cch_func_descriptor whcm_cch_func_desc = {
	.bLength 						= 0x05,
	.bDescriptorType 		= 0x24,
	.bDescriptorSubType	= 0x00,
	.bcdCDC							= 0x0110,
}; 

static struct whcm_func_descriptor whcm_func_desc = {
	.bLength						= 0x05,
	.bDescriptorType		= 0x24,
	.bDescriptorSubType	= 0x11,
	.bcdVersion					= 0x0110,
};

static struct whcm_ccu_func_descriptor whcm_ccu_func_desc = {
	.bLength								= 0x07,
	.bDescriptorType				= 0x24,
	.bDescriptorSubType			= 0x06,
	.bControlInterface			= 0x00,
	.bSubordinateInterface0	= 0x00,
	.bSubordinateInterface1	= 0x02,
	.bSubordinateInterface2	= 0x08,
};

struct whcm_context {
	unsigned configured;
	unsigned bound;
};

static struct usb_function usb_func_whcm;
static struct whcm_context _context;

static void whcm_unbind(void *context)
{
	struct whcm_context *ctxt = context;

	if (!ctxt)
		return;
	if (!ctxt->bound)
		return;
	ctxt->bound = 0;
}

static void whcm_bind(void *context)
{
	struct whcm_context *ctxt = context;

	if (!ctxt)
		return;

	whcm_intf_desc.bInterfaceNumber =
		usb_msm_get_next_ifc_number(&usb_func_whcm);

	ctxt->bound = 1;
}

static void whcm_configure(int configured, void *_ctxt)
{
	struct whcm_context *ctxt = _ctxt;

	if (!ctxt)
		return;
	if (configured) {
		ctxt->configured = 1;
	} else {
		ctxt->configured = 0;
	}

}

static struct usb_function usb_func_whcm = {
	.bind = whcm_bind,
	.configure = whcm_configure,
	.unbind = whcm_unbind,
	.name = "whcm",
	.context = &_context,
};

struct usb_descriptor_header *whcm_hs_descriptors[5];
struct usb_descriptor_header *whcm_fs_descriptors[5];

static int __init whcm_init(void)
{
	int r;
	struct whcm_context *ctxt = &_context;

	whcm_hs_descriptors[0] = (struct usb_descriptor_header *)&whcm_intf_desc;
	whcm_hs_descriptors[1] =
		(struct usb_descriptor_header *)&whcm_cch_func_desc;
	whcm_hs_descriptors[2] =
		(struct usb_descriptor_header *)&whcm_func_desc;
	whcm_hs_descriptors[3] =
		(struct usb_descriptor_header *)&whcm_ccu_func_desc;
	whcm_hs_descriptors[4] = NULL;

	whcm_fs_descriptors[0] = (struct usb_descriptor_header *)&whcm_intf_desc;
	whcm_fs_descriptors[1] =
		(struct usb_descriptor_header *)&whcm_cch_func_desc;
	whcm_fs_descriptors[2] =
		(struct usb_descriptor_header *)&whcm_func_desc;
	whcm_fs_descriptors[3] =
		(struct usb_descriptor_header *)&whcm_ccu_func_desc;
	whcm_fs_descriptors[4] = NULL;

	usb_func_whcm.hs_descriptors = whcm_hs_descriptors;
	usb_func_whcm.fs_descriptors = whcm_fs_descriptors;
	r = usb_function_register(&usb_func_whcm);
	return r;
}
module_init(whcm_init);

static void __exit whcm_exit(void)
{
	struct whcm_context *ctxt = &_context;
	if (!ctxt)
		return;
	if (!ctxt)
		BUG_ON(1);

	usb_function_unregister(&usb_func_whcm);
}
module_exit(whcm_exit);

MODULE_LICENSE("GPL v2");
