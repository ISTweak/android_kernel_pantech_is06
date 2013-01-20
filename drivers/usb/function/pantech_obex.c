#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/poll.h>

#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/list.h>

#include <asm/atomic.h>
#include <asm/uaccess.h>

#include "usb_function.h"

#define TXN_MAX 4096

/* number of rx and tx requests to allocate */
#define RX_REQ_MAX 4
#define TX_REQ_MAX 4

#define OBEX_FUNCTION_NAME "obex"

struct obex_context
{
	int online;
	int error;

	atomic_t read_excl;
	atomic_t write_excl;
	atomic_t open_excl;
	atomic_t enable_excl;
	spinlock_t lock;

	struct usb_endpoint *out;
	struct usb_endpoint *in;

	struct list_head tx_idle;
	struct list_head rx_idle;
	struct list_head rx_done;

	wait_queue_head_t read_wq;
	wait_queue_head_t write_wq;

	/* the request we're currently reading from */
	struct usb_request *read_req;
	unsigned char *read_buf;
	unsigned read_count;
	unsigned bound;
};

static struct obex_context _context;

static struct usb_interface_descriptor obex_cdc_intf_desc = {
	.bLength =		sizeof obex_cdc_intf_desc,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bNumEndpoints =	0,
	.bInterfaceClass =	0x02,
	.bInterfaceSubClass =	0x0B,
	.bInterfaceProtocol =	0x00,
};

static struct usb_interface_descriptor obex_data_intf_desc = {
	.bLength =		sizeof obex_data_intf_desc,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bNumEndpoints =	2,
	.bInterfaceClass =	0x0A,
	.bInterfaceSubClass =	0x00,
	.bInterfaceProtocol =	0x00,
};

static struct usb_endpoint_descriptor hs_bulk_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(512),
	.bInterval =		0,
};
static struct usb_endpoint_descriptor fs_bulk_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(64),
	.bInterval =		0,
};

static struct usb_endpoint_descriptor hs_bulk_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(512),
	.bInterval =		0,
};

static struct usb_endpoint_descriptor fs_bulk_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(64),
	.bInterval =		0,
};

static struct usb_function usb_func_obex;

static inline int _lock(atomic_t *excl)
{
	if (atomic_inc_return(excl) == 1) {
		return 0;
	} else {
		atomic_dec(excl);
		return -1;
	}
}

static inline void _unlock(atomic_t *excl)
{
	atomic_dec(excl);
}

/* add a request to the tail of a list */
static void req_put(struct obex_context *ctxt, struct list_head *head, struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&ctxt->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&ctxt->lock, flags);
}

/* remove a request from the head of a list */
static struct usb_request *req_get(struct obex_context *ctxt, struct list_head *head)
{
	unsigned long flags;
	struct usb_request *req;

	spin_lock_irqsave(&ctxt->lock, flags);
	if (list_empty(head)) {
		req = 0;
	} else {
		req = list_first_entry(head, struct usb_request, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&ctxt->lock, flags);
	return req;
}

static void obex_complete_in(struct usb_endpoint *ept, struct usb_request *req)
{
	struct obex_context *ctxt = req->context;

	if (req->status != 0)
		ctxt->error = 1;

	req_put(ctxt, &ctxt->tx_idle, req);

	wake_up(&ctxt->write_wq);
}

static void obex_complete_out(struct usb_endpoint *ept, struct usb_request *req)
{
	struct obex_context *ctxt = req->context;

#if defined(CONFIG_HSUSB_PANTECH_USB_TEST)
	if((pantech_usb_test_get_run() == 1) 
	  &&	(pantech_usb_test_get_open() == 1) 
	  && (pantech_usb_test_get_port_type() == 2))
	{
		//printk("obex_loopback_read size[%d] = count[%d]\n", req->length, req->actual);

		if(req->status == 0) { 
			if(req->actual == 0) { 
			  pantech_usb_test_send_loopback((void*)ctxt, ctxt->in);
			} else if(req->actual < req->length) {
				pantech_usb_test_copy_buffer(req->buf, req->actual);
			  pantech_usb_test_send_loopback((void*)ctxt, ctxt->in);
			} else {
				pantech_usb_test_copy_buffer(req->buf, req->actual);
			}
		}
		usb_ept_queue_xfer(ctxt->out, req);

		return;	
	}
#endif

	if (req->status != 0) {
		ctxt->error = 1;
		req_put(ctxt, &ctxt->rx_idle, req);
	} else {
		req_put(ctxt, &ctxt->rx_done, req);
	}

	wake_up(&ctxt->read_wq);
}

static ssize_t obex_read(struct file *fp, char __user *buf,
			size_t count, loff_t *pos)
{
	struct obex_context *ctxt = &_context;
	struct usb_request *req;
	int r = count, xfer;
	int ret;
	int sended_count;

	DBG("obex_read(%d)\n", count);

	if (_lock(&ctxt->read_excl))
		return -EBUSY;

	/* we will block until we're online */
	while (!(ctxt->online || ctxt->error)) {
		DBG("obex_read: waiting for online state\n");
		ret = wait_event_interruptible(ctxt->read_wq, (ctxt->online || ctxt->error));
		if (ret < 0) {
			_unlock(&ctxt->read_excl);
			return ret;
		}
	}

	sended_count = 0;

	while (count > 0) {
		if (ctxt->error) {
			r = -EIO;
			break;
		}

		/* if we have idle read requests, get them queued */
		while ((req = req_get(ctxt, &ctxt->rx_idle))) {
			req->length = TXN_MAX;
			ret = usb_ept_queue_xfer(ctxt->out, req);
			if (ret < 0) {
				DBG("obex_read: failed to queue req %p (%d)\n", req, ret);
				r = -EIO;
				ctxt->error = 1;
				req_put(ctxt, &ctxt->rx_idle, req);
				goto fail;
			} else {
				DBG("rx %p queue\n", req);
			}
		}


		/* if we have data pending, give it to userspace */
		if (ctxt->read_count > 0) {
			xfer = (ctxt->read_count < count) ? ctxt->read_count : count;

			if (copy_to_user(buf, ctxt->read_buf, xfer)) {
				r = -EFAULT;
				break;
			}
			ctxt->read_buf += xfer;
			ctxt->read_count -= xfer;
			buf += xfer;
			count -= xfer;

			sended_count += xfer;

			/* if we've emptied the buffer, release the request */
			if (ctxt->read_count == 0) {
				req_put(ctxt, &ctxt->rx_idle, ctxt->read_req);
				ctxt->read_req = 0;
			}
			continue;
		}

		/* wait for a request to complete */
		req = 0;
		ret = wait_event_interruptible(ctxt->read_wq,
					       ((req = req_get(ctxt, &ctxt->rx_done)) || ctxt->error || (fp->f_flags & O_NONBLOCK) ));

		if ((req == 0) && (fp->f_flags & O_NONBLOCK))
		{
			r = (sended_count > 0)? sended_count : -EAGAIN;
			break;
		}

		if (req != 0) {
			/* if we got a 0-len one we need to put it back into
			** service.  if we made it the current read req we'd
			** be return to userspace
			*/
			if (req->actual == 0){ //End Of Packet
				req->length = TXN_MAX;
				ret = usb_ept_queue_xfer(ctxt->out, req);
				if (ret < 0) {
					DBG("obex_read: failed to queue req %p (%d)\n", req, ret);
					r = -EIO;
					ctxt->error = 1;
					req_put(ctxt, &ctxt->rx_idle, req);
					goto fail;
				} else {
					DBG("rx %p queue\n", req);
					r = (sended_count > 0)? sended_count : 0;
					break;
				}
			}

			ctxt->read_req = req;
			ctxt->read_count = req->actual;
			ctxt->read_buf = req->buf;
			DBG("rx %p %d\n", req, req->actual);
		}

		if (ret < 0) {
			r = ret;
			break;
		}
	}

fail:
	_unlock(&ctxt->read_excl);
	return r;
}

static ssize_t obex_write(struct file *fp, const char __user *buf,
			 size_t count, loff_t *pos)
{
	struct obex_context *ctxt = &_context;
	struct usb_request *req = 0;
	int r = count, xfer;
	int all_count = count;
	int ret;

	DBG("obex_write(%d)\n", count);

	if (_lock(&ctxt->write_excl))
		return -EBUSY;

	while (count > 0) {
		if (ctxt->error) {
			r = -EIO;
			break;
		}

		/* get an idle tx request to use */
		req = 0;
		ret = wait_event_interruptible(ctxt->write_wq,
					       ((req = req_get(ctxt, &ctxt->tx_idle)) || ctxt->error));

		if (ret < 0) {
			r = ret;
			break;
		}

		if (req != 0) {
			xfer = count > TXN_MAX ? TXN_MAX : count;
			if (copy_from_user(req->buf, buf, xfer)) {
				r = -EFAULT;
				break;
			}

			req->length = xfer;
			ret = usb_ept_queue_xfer(ctxt->in, req);
			if (ret < 0) {
				DBG("obex_write: xfer error %d\n", ret);
				ctxt->error = 1;
				r = -EIO;
				break;
			}

			buf += xfer;
			count -= xfer;

			/* zero this so we don't try to free it on error exit */
			req = 0;
		}
	}


	if (req)
		req_put(ctxt, &ctxt->tx_idle, req);

	/* send 0-length packet */
	if(all_count > ctxt->in->max_pkt && (all_count % ctxt->in->max_pkt) == 0) {
		req = 0;
		ret = wait_event_interruptible(ctxt->write_wq,
					       ((req = req_get(ctxt, &ctxt->tx_idle)) || ctxt->error));

		if (ret >= 0) {
			if (req != 0) {
				req->length = 0;
				usb_ept_queue_xfer(ctxt->in, req);
				req = 0;
			}
		}
	}

	_unlock(&ctxt->write_excl);
	return r;
}

static int obex_poll(struct file *fp, poll_table *wait)
{
	struct obex_context *ctxt = &_context;
	struct usb_request *req;
	int ret;

	DBG("obex_poll\n");

	/* read_exec is shared with poll&read */

	if (_lock(&ctxt->read_excl))
		return -EBUSY;

	if(!(ctxt->online || ctxt->error)) {
		DBG("obex_poll: It's not online state\n");
		_unlock(&ctxt->read_excl);
		return POLLERR | POLLHUP;
	}


	if(ctxt->read_count > 0) {
		_unlock(&ctxt->read_excl);
		return POLLIN | POLLRDNORM;
	}

	if(!list_empty(&ctxt->rx_idle) && (req = req_get(ctxt, &ctxt->rx_idle))) 
	{
		req->length = TXN_MAX;
		ret = usb_ept_queue_xfer(ctxt->out, req);
		if (ret < 0) {
			DBG("obex_poll:xfer error %d\n", ret);
			ctxt->error = 1;
			req_put(ctxt, &ctxt->rx_idle, req);
			_unlock(&ctxt->read_excl);
		  return -EIO;
		}
	}

	poll_wait(fp, &ctxt->read_wq, wait);

	if(!list_empty(&ctxt->rx_done)) {
		_unlock(&ctxt->read_excl);
		return POLLIN | POLLRDNORM;
	}

	_unlock(&ctxt->read_excl);
	return 0;
}

static int obex_open(struct inode *ip, struct file *fp)
{
	struct obex_context *ctxt = &_context;
	struct usb_request *req;

	DBG("obex_open\n");

	if (_lock(&ctxt->open_excl))
		return -EBUSY;

	/* clear the error latch */
	ctxt->error = 0;

	/* if we have a stale request being read, recycle it */
	ctxt->read_buf = 0;
	ctxt->read_count = 0;
	if (ctxt->read_req) {
		req_put(ctxt, &ctxt->rx_idle, ctxt->read_req);
		ctxt->read_req = 0;
	}

	/* retire any completed rx requests from previous session */
	while ((req = req_get(ctxt, &ctxt->rx_done)))
		req_put(ctxt, &ctxt->rx_idle, req);

	return 0;
}

static int obex_release(struct inode *ip, struct file *fp)
{
	struct obex_context *ctxt = &_context;
	struct usb_request *req;

	DBG("obex_release\n");

	/* if we have a stale request being read, recycle it */
	ctxt->read_buf = 0;
	ctxt->read_count = 0;
	if (ctxt->read_req) {
		req_put(ctxt, &ctxt->rx_idle, ctxt->read_req);
		ctxt->read_req = 0;
	}

	/* retire any completed rx requests from previous session */
	while ((req = req_get(ctxt, &ctxt->rx_done)))
		req_put(ctxt, &ctxt->rx_idle, req);
	
	ctxt->error = 1;
	wake_up(&ctxt->read_wq);

	_unlock(&ctxt->open_excl);
	return 0;
}

static struct file_operations obex_fops = {
	.owner =   THIS_MODULE,
	.read =    obex_read,
	.write =   obex_write,
	.poll =    obex_poll,
	.open =    obex_open,
	.release = obex_release,
};

static struct miscdevice obex_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "obex",
	.fops = &obex_fops,
};

static void obex_unbind(void *_ctxt)
{
	struct obex_context *ctxt = _ctxt;
	struct usb_request *req;

	if (!ctxt->bound)
		return;

	while ((req = req_get(ctxt, &ctxt->rx_idle))) {
		usb_ept_free_req(ctxt->out, req);
	}
	while ((req = req_get(ctxt, &ctxt->tx_idle))) {
		usb_ept_free_req(ctxt->in, req);
	}
	if (ctxt->in) {
		usb_ept_fifo_flush(ctxt->in);
		usb_ept_enable(ctxt->in,  0);
		usb_free_endpoint(ctxt->in);
	}
	if (ctxt->out) {
		usb_ept_fifo_flush(ctxt->out);
		usb_ept_enable(ctxt->out,  0);
		usb_free_endpoint(ctxt->out);
	}

	ctxt->online = 0;
	ctxt->error = 1;

	/* readers may be blocked waiting for us to go online */
	wake_up(&ctxt->read_wq);
	ctxt->bound = 0;
}

static void obex_bind(void *_ctxt)
{
	struct obex_context *ctxt = _ctxt;
	struct usb_request *req;
	int n;

	obex_cdc_intf_desc.bInterfaceNumber =
		usb_msm_get_next_ifc_number(&usb_func_obex);

	obex_data_intf_desc.bInterfaceNumber =
		usb_msm_get_next_ifc_number(&usb_func_obex);

	ctxt->in = usb_alloc_endpoint(USB_DIR_IN);
	if (ctxt->in) {
		hs_bulk_in_desc.bEndpointAddress = USB_DIR_IN | ctxt->in->num;
		fs_bulk_in_desc.bEndpointAddress = USB_DIR_IN | ctxt->in->num;
	}

	ctxt->out = usb_alloc_endpoint(USB_DIR_OUT);
	if (ctxt->out) {
		hs_bulk_out_desc.bEndpointAddress = USB_DIR_OUT|ctxt->out->num;
		fs_bulk_out_desc.bEndpointAddress = USB_DIR_OUT|ctxt->out->num;
	}

	for (n = 0; n < RX_REQ_MAX; n++) {
		req = usb_ept_alloc_req(ctxt->out, 4096);
		if (req == 0) {
			ctxt->bound = 1;
			goto fail;
		}
		req->context = ctxt;
		req->complete = obex_complete_out;
		req_put(ctxt, &ctxt->rx_idle, req);
	}

	for (n = 0; n < TX_REQ_MAX; n++) {
		req = usb_ept_alloc_req(ctxt->in, 4096);
		if (req == 0) {
			ctxt->bound = 1;
			goto fail;
		}
		req->context = ctxt;
		req->complete = obex_complete_in;
		req_put(ctxt, &ctxt->tx_idle, req);
	}
	ctxt->bound = 1;
	return;

fail:
	printk(KERN_ERR "obex_bind() could not allocate requests\n");
	obex_unbind(ctxt);
}

static void obex_configure(int configured, void *_ctxt)
{
	struct obex_context *ctxt = _ctxt;
	struct usb_request *req;

	if (configured) {
		ctxt->online = 1;

		if (usb_msm_get_speed() == USB_SPEED_HIGH) {
			usb_configure_endpoint(ctxt->in, &hs_bulk_in_desc);
			usb_configure_endpoint(ctxt->out, &hs_bulk_out_desc);
		} else {
			usb_configure_endpoint(ctxt->in, &fs_bulk_in_desc);
			usb_configure_endpoint(ctxt->out, &fs_bulk_out_desc);
		}
		usb_ept_enable(ctxt->in,  1);
		usb_ept_enable(ctxt->out, 1);

		/* if we have a stale request being read, recycle it */
		ctxt->read_buf = 0;
		ctxt->read_count = 0;
		if (ctxt->read_req) {
			req_put(ctxt, &ctxt->rx_idle, ctxt->read_req);
			ctxt->read_req = 0;
		}

		/* retire any completed rx requests from previous session */
		while ((req = req_get(ctxt, &ctxt->rx_done)))
			req_put(ctxt, &ctxt->rx_idle, req);

	} else {
		ctxt->online = 0;
		ctxt->error = 1;
	}

	/* readers may be blocked waiting for us to go online */
	wake_up(&ctxt->read_wq);
}

static struct usb_function usb_func_obex = {
	.bind = obex_bind,
	.unbind = obex_unbind,
	.configure = obex_configure,

	.name = OBEX_FUNCTION_NAME,
	.context = &_context,

};

#if defined(CONFIG_HSUSB_PANTECH_USB_TEST)
void obex_loopback_open(void)
{
	struct obex_context *ctxt = &_context;
	struct usb_request *req;
	int ret;

	if (atomic_read(&ctxt->open_excl) != 0)
		return ;

	/* clear the error latch */
	ctxt->error = 0;

	/* if we have a stale request being read, recycle it */
	ctxt->read_buf = 0;
	ctxt->read_count = 0;
	if (ctxt->read_req) {
		req_put(ctxt, &ctxt->rx_idle, ctxt->read_req);
		ctxt->read_req = 0;
	}

	/* retire any completed rx requests from previous session */
	while ((req = req_get(ctxt, &ctxt->rx_done)))
		req_put(ctxt, &ctxt->rx_idle, req);

	while ((req = req_get(ctxt, &ctxt->rx_idle))) {
		req->length = TXN_MAX;
		ret = usb_ept_queue_xfer(ctxt->out, req);
		if(ret < 0) {
			req_put(ctxt, &ctxt->rx_idle, req);
		}
	}

}

void obex_loopback_close(void)
{
	struct obex_context *ctxt = &_context;
	struct usb_request *req;

	/* if we have a stale request being read, recycle it */
	ctxt->read_buf = 0;
	ctxt->read_count = 0;
	if (ctxt->read_req) {
		req_put(ctxt, &ctxt->rx_idle, ctxt->read_req);
		ctxt->read_req = 0;
	}

	/* retire any completed rx requests from previous session */
	while ((req = req_get(ctxt, &ctxt->rx_done)))
		req_put(ctxt, &ctxt->rx_idle, req);
	
	ctxt->error = 1;
}
#endif

struct usb_descriptor_header *obex_hs_descriptors[5];
struct usb_descriptor_header *obex_fs_descriptors[5];
static int __init obex_init(void)
{
	int ret = 0;
	struct obex_context *ctxt = &_context;
	DBG("obex_init()\n");

	init_waitqueue_head(&ctxt->read_wq);
	init_waitqueue_head(&ctxt->write_wq);

	atomic_set(&ctxt->open_excl, 0);
	atomic_set(&ctxt->read_excl, 0);
	atomic_set(&ctxt->write_excl, 0);
	atomic_set(&ctxt->enable_excl, 0);

	spin_lock_init(&ctxt->lock);

	INIT_LIST_HEAD(&ctxt->rx_idle);
	INIT_LIST_HEAD(&ctxt->rx_done);
	INIT_LIST_HEAD(&ctxt->tx_idle);

	obex_hs_descriptors[0] = (struct usb_descriptor_header *)&obex_cdc_intf_desc;
	obex_hs_descriptors[1] = (struct usb_descriptor_header *)&obex_data_intf_desc;
	obex_hs_descriptors[2] =
		(struct usb_descriptor_header *)&hs_bulk_in_desc;
	obex_hs_descriptors[3] =
		(struct usb_descriptor_header *)&hs_bulk_out_desc;
	obex_hs_descriptors[4] = NULL;

	obex_fs_descriptors[0] = (struct usb_descriptor_header *)&obex_cdc_intf_desc;
	obex_fs_descriptors[1] = (struct usb_descriptor_header *)&obex_data_intf_desc;
	obex_fs_descriptors[2] =
		(struct usb_descriptor_header *)&fs_bulk_in_desc;
	obex_fs_descriptors[3] =
		(struct usb_descriptor_header *)&fs_bulk_out_desc;
	obex_fs_descriptors[4] = NULL;

	usb_func_obex.hs_descriptors = obex_hs_descriptors;
	usb_func_obex.fs_descriptors = obex_fs_descriptors;

	ret = misc_register(&obex_device);
	if (ret) {
		printk(KERN_ERR " Can't register misc device  %d \n",
						MISC_DYNAMIC_MINOR);
		return ret;
	}

	ret = usb_function_register(&usb_func_obex);
	if (ret) {
		misc_deregister(&obex_device);
	}
	return ret;
}

module_init(obex_init);

static void __exit obex_exit(void)
{
	misc_deregister(&obex_device);

	usb_function_unregister(&usb_func_obex);
}
module_exit(obex_exit);
