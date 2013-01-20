/*
//
// lived 2009.11.05
// FEATURE define
//
*/

#ifndef SKY_FRAMEWORK_FEATURE
#define SKY_FRAMEWORK_FEATURE

/*
// Debug Msg
//#if TARGET_BUILD_VARIANT == eng
*/
#if 0
#define TEST_MSG(...)   ((void)LOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define SKY_LOG_DEBUG
#else
#define TEST_MSG(...)
#endif

#define SKY_LCD_LOADBAR_ANDROID //leecy.. 
#define SKY_EF12S_LCD
/*20101002 jjangu_device*/
#define SKY_STATE_LCD

#ifndef PANTECH_BOOT_SOUND  //20100803 jhsong : boot sound
#define PANTECH_BOOT_SOUND
#endif

/*
//#define SKY_USE_HDMI    // SKY_OUTPUT_SWITCH
#define SKY_FB_RGB_888
*/
/*20100717 jjangu_device Froyo use MDP31*/
#define SKY_FB_RGB_888

#define SKY_FRAMEBUFFER_32

#define SKY_SET_BACKLIGHT_BEFORE_BOOTANIM
/*
//#define SKY_SCREEN_CAPTURE_API
*/

/*20100907 jjangu_device H/W RENDERER »ç¿ë*/
/*20101018 jjangu_device we can use "debug.sf.hw=1" in the system.prop for LCD performance so not use DIM_WITH_TEXTURE anymore*/
/*20101026 jjangu_device gray color issue occure when camera preview so we use hw=0*/
#define DIM_WITH_TEXTURE

#define SKY_FPCB_TEST_API
/*20100907 jjangu_device*/
#undef SKY_LCD_ROTATE_180
#define SKY_FIX_REFRESH_RATE
#define SKY_ANDROID_RESET_REBOOT

#ifdef SKY_USE_HDMI /*SKY_OUTPUT_SWITCH*/
#define HDMI_STATE      "/sys/devices/platform/sky_hdmi.0/connection_state"
#define HDMI_PMEM_SIZE  1280*720*2
#ifdef SKY_FB_RGB_888
#define SKY_FB_HDMI_16BPP_RGB_565
#endif
/*
//#define SKY_USE_HDMI_USING_THREAD
*/
#endif

#ifdef SKY_SCREEN_CAPTURE_API
#define SNAPSHOT_FILE   "/data/.snapshot.raw"
#define CAPTURE_SIZE    480*800*2
#endif

#ifdef SKY_FB_RGB_888
#define SKY_FB_HDMI_RGB_565
#endif

#ifdef SKY_USE_HDMI /*SKY_OUTPUT_SWITCH*/
enum {  /* Frame Buffer Number */
    LCD_OUT,
    HDMI_OUT_720P,
    HDMI_OUT_480P,
    OUT_TYPE_LIMIT
};
#endif

#endif  /*SKY_FRAMEWORK_FEATURE*/
