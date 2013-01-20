/* drivers/video/msm/logo.c
 *
 * Show Logo in RLE 565 format
 *
 * Copyright (C) 2008 Google Incorporated
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fb.h>
#include <linux/vt_kern.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>

#include <linux/irq.h>
#include <asm/system.h>

#define fb_width(fb)	((fb)->var.xres)
#define fb_height(fb)	((fb)->var.yres)
//#define fb_size(fb)	((fb)->var.xres * (fb)->var.yres * 2)

#ifdef SKY_FRAMEBUFFER_32
typedef unsigned int IBUF_TYPE;
#if 0
//#ifdef SKY_FB_RGB_888
static void memset24(void *_ptr, unsigned short val, unsigned count)
{
    unsigned char *ptr = _ptr;
    unsigned char r, g, b;

    r = (unsigned char)((val & 0xf800) >> 8);
    g = (unsigned char)((val & 0x07e0) >> 3);
    b = (unsigned char)((val & 0x001f) << 3);

    count >>= 1;
    while (count--)
    {
        *ptr++ = b;
        *ptr++ = g;
        *ptr++ = r;
        *ptr++ = 0; // 32bpp
    }
}
#else
static void memset32(void *_ptr, unsigned int val, unsigned count)
{
	unsigned int *ptr = _ptr;
	count >>= 2;
	while (count--)
		*ptr++ = val;
}
#endif

#else
typedef unsigned short IBUF_TYPE;

static void memset16(void *_ptr, unsigned short val, unsigned count)
{
	unsigned short *ptr = _ptr;
	count >>= 1;
	while (count--)
		*ptr++ = val;
}
#endif  // SKY_FRAMEBUFFER_32

/* 565RLE image format: [count(2 bytes), rle(2 bytes)] */
int load_565rle_image(char *filename)
{
	struct fb_info *info;
	int fd, err = 0;
	unsigned count, max;
#ifdef SKY_FRAMEBUFFER_32
    IBUF_TYPE *data, *bits, *ptr;

//#ifdef SKY_FB_RGB_888
//	unsigned char *data, *bits;
//      unsigned short *ptr;
#else
	unsigned short *data, *bits, *ptr;
#endif

	info = registered_fb[0];
	if (!info) {
		printk(KERN_WARNING "%s: Can not access framebuffer\n",
			__func__);
		return -ENODEV;
	}

	fd = sys_open(filename, O_RDONLY, 0);
	if (fd < 0) {
		printk(KERN_WARNING "%s: Can not open %s\n",
			__func__, filename);
		return -ENOENT;
	}
	count = (unsigned)sys_lseek(fd, (off_t)0, 2);
	if (count == 0) {
		sys_close(fd);
		err = -EIO;
		printk(KERN_ERR "%s: Can not lseek data\n", __func__);
		goto err_logo_close_file;
	}
	sys_lseek(fd, (off_t)0, 0);
	data = kmalloc(count, GFP_KERNEL);
	if (!data) {
		printk(KERN_WARNING "%s: Can not alloc data\n", __func__);
		err = -ENOMEM;
		goto err_logo_close_file;
	}
	if ((unsigned)sys_read(fd, (char *)data, count) != count) {
		err = -EIO;
		printk(KERN_ERR "%s: Can not read data\n", __func__);
		goto err_logo_free_data;
	}

	max = fb_width(info) * fb_height(info);
	ptr = data;
#ifdef SKY_FRAMEBUFFER_32
	bits = (IBUF_TYPE *)(info->screen_base);
//#ifdef SKY_FB_RGB_888
//	bits = (unsigned char *)(info->screen_base);
#else
	bits = (unsigned short *)(info->screen_base);
#endif
	while (count > 3) {
		unsigned n = ptr[0];
		if (n > max)
			break;
#ifdef SKY_FRAMEBUFFER_32
		memset32((unsigned int *)bits, ptr[1], n << 2);
//#ifdef SKY_FB_RGB_888
//		memset24(bits, ptr[1], n << 1);
//		//bits += (n*3);
//		bits += (n<<2);
#else
		memset16(bits, ptr[1], n << 1);
#endif
		bits += n;
		max -= n;
		ptr += 2;
		count -= 4;
	}

err_logo_free_data:
	kfree(data);
err_logo_close_file:
	sys_close(fd);
	return err;
}

#ifdef CONFIG_SKY_LCD_MODIFY
#if defined(FEATURE_SW_RESET_RELEASE_MODE)
extern bool sky_sys_rst_is_silent_boot_mode(void);
#endif

#define PIXEL_TO_32(RGB)  ((((RGB)&0xF800)<<8) | (((RGB)&0x07E0)<<5) | (((RGB)&0x001F)<<3))
static IBUF_TYPE loading_progress_t[8] = 
{
#ifdef SKY_FRAMEBUFFER_32
    PIXEL_TO_32(0x9c13),PIXEL_TO_32(0xd61a),PIXEL_TO_32(0xc598),PIXEL_TO_32(0xa454),
    PIXEL_TO_32(0x8310),PIXEL_TO_32(0x9c13),PIXEL_TO_32(0xc598),PIXEL_TO_32(0x9c13)
#else
    0x9c13,0xd61a,0xc598,0xa454,0x8310,0x9c13,0xc598,0x9c13
#endif
};

int sky_lcdc_display_loadingbar(int ratio)
{

    struct fb_info *info;
    int err = 0;
   IBUF_TYPE *load_bar;
   unsigned int i;
   IBUF_TYPE BAR_COLOR = 0xFFFF;

#ifdef SKY_LCD_ROTATE_180
    const unsigned short ST_X = 20, ST_Y = 49;
#else
    const unsigned short ST_X = 19, ST_Y = 743;
#endif
    const unsigned short SCREEN_WIDTH = 480;
    const unsigned short BAR_WIDTH = 441;
    const unsigned short BAR_HEIGHT = 8;

    int cr=0;

#if defined(FEATURE_SW_RESET_RELEASE_MODE)
    if (sky_sys_rst_is_silent_boot_mode())
        return 0;
#endif

    if (ratio>100) ratio=100;
    cr = (int)(ratio*BAR_WIDTH/100);


    info = registered_fb[0];
    if (!info) {
        printk(KERN_WARNING "%s: Can not access framebuffer\n",
                __func__);
        return -ENODEV;
    }

   load_bar = (IBUF_TYPE *)(info->screen_base) + SCREEN_WIDTH*ST_Y+ST_X;
   for (i = 0; i < BAR_HEIGHT; i++) {
        BAR_COLOR = loading_progress_t[i];
#ifdef SKY_FRAMEBUFFER_32
      memset32((unsigned int *)
#ifdef SKY_LCD_ROTATE_180
              load_bar+BAR_WIDTH-cr,
#else
              load_bar,
#endif
              BAR_COLOR, cr<<2);
#else   // ! SKY_FRAMEBUFFER_32
        memset16(load_bar, BAR_COLOR, cr<<1);
#endif  // end of SKY_FRAMEBUFFER_32
        load_bar = load_bar + SCREEN_WIDTH;
   }

   return err;
}

EXPORT_SYMBOL(sky_lcdc_display_loadingbar);
#endif //#ifdef CONFIG_SKY_LCD_MODIFY

EXPORT_SYMBOL(load_565rle_image);
