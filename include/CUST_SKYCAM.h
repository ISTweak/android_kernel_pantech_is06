#ifndef __CUST_SKYCAM_H__
#define __CUST_SKYCAM_H__


/*

(1)  �ϵ���� ����

EF10S ������ ������ ISP (MtekVision MV9337) �� ���忡 �����ϰ�, Sony BAYER ���� 
(imx034) �� ��� Ÿ������ FPCB �� ���� SMIA �� �����Ͽ� ����Ѵ�.
�׷���, SW ���鿡���� ISP �� ������ �и��Ǿ��� �� ���յ� SOC ī�޶�� �ٸ� �� 
�����Ƿ�, ���� ���ݿ� ���� ���� FEATURE �𵨵鿡�� SOC ī�޶� �̿��Ͽ� ������ 
������� �����ϰ� �����ϰ�, ������ ������� �����Ѵ�.

Sony IMX034 (BAYER Sensor) + MtekVision MV9337 (ISP) + LGInnotek (Packaging)
	MCLK = 24MHz, PCLK = 96MHz (PREVIEW/SNAPSHOT), SMIA = 648MHz


(2)  ī�޶� ���� ��� kernel/userspace/android �α׸� ����

kernel/arch/arm/config/qsd8650-perf_defconfig �� �����Ѵ�.

	# CONFIG_MSM_CAMERA_DEBUG is not set (default)

CUST_SKYCAM.h ���� F_SKYCAM_LOG_PRINTK �� #undef �Ѵ�. 

	#define F_SKYCAM_LOG_PRINTK (default)

��� �ҽ� ���Ͽ��� F_SKYCAM_LOG_OEM �� ã�� �ּ� ó���Ѵ�. 
	���� �� ���, �ش� ���Ͽ� ���� SKYCDBG/SKYCERR ��ũ�θ� �̿��� 
	�޽������� Ȱ��ȭ ��Ų��. (user-space only)

��� �ҽ� ���Ͽ��� F_SKYCAM_LOG_CDBG �� ã�� �ּ� ó���Ѵ�. 
	���� �� ���, �ش� ���Ͽ� ���� CDBG ��ũ�θ� �̿��� �޽������� 
	Ȱ��ȭ ��Ų��. (user-space only)

��� �ҽ� ���Ͽ��� F_SKYCAM_LOG_VERBOSE �� ã�� �ּ� ó���Ѵ�.
	���� �� ���, �ش� ���Ͽ� ���� LOGV/LOGD/LOGI ��ũ�θ� �̿��� 
	�޽������� Ȱ��ȭ ��Ų��. (user-space only)


(3)  MV9337 ���� ���� ȯ�� (kernel/user-space)

kernel/arch/arm/config/qsd8650-perf_defconfig �� �����Ѵ�. (kernel)

	CONFIG_MV9337=y (default)	����
	# CONFIG_MV9337 is not set	������

	qsd8650-perf_defconfig �� Kconfig (default y) ���� �켱 ������ �����Ƿ� 
	�� �κ� �� �����ϸ� �ȴ�.

vendor/qcom-proprietary/mm-camera/Android.mk,camera.mk �� �����Ѵ�. (user-space)

	SKYCAM_MV9337=yes (default)	����
	SKYCAM_MV9337=no		������


(4)  �ȸ��ν� ���� ��� ���� ȯ��

vendor/qcom/android-open/libcamera2/Android.mk �� �����Ѵ�.
	3rd PARTY �ַ�� ��� ���θ� �����Ѵ�.

	SKYCAM_FD_ENGINE := 0		������
	SKYCAM_FD_ENGINE := 1		�ö���� �ַ�� ���
	SKYCAM_FD_ENGINE := 2		��Ÿ �ַ�� ���

CUST_SKYCAM.h ���� F_SKYCAM_ADD_CFG_FACE_FILTER �� #undef �Ѵ�.
	�ȸ��ν� ��� ���� �������̽� ���� ���θ� �����Ѵ�.

libOlaEngine.so �� ���� libcamera.so �� ��ũ�ϹǷ� ���� ��� libcamera.so ��
ũ�Ⱑ �����Ͽ� ��ũ ������ �߻� �����ϸ�, �� ��� �Ʒ� ���ϵ鿡�� 
liboemcamera.so �� ������ �ٿ� libcamera.so �� ������ Ȯ���� �� �ִ�.

build/core/prelink-linux-arm-2G.map (for 2G/2G)
build/core/prelink-linux-arm.map (for 1G/3G)


(5)  ���� �߰��� ���� ���

kernel\drivers\media\video\msm\mv9337.c
kernel\drivers\media\video\msm\mv9337.h
kernel\drivers\media\video\msm\mv9337_reg.c
kernel\drivers\media\video\msm\mv9337_imx034_**_**.h

vendor\qcom\android-open\libcamera2\libOlaEngine.so
vendor\qcom\android-open\libcamera2\Ola*.*

vendor\qcom-proprietary\mm-camera\targets\tgtcommon\sensor\mv9337
vendor\qcom-proprietary\mm-camera\targets\tgtcommon\sensor\mv9337\mv9337.c
vendor\qcom-proprietary\mm-camera\targets\tgtcommon\sensor\mv9337\mv9337.h

*/


/* EF10S ��� ���� EF12S ���� EF10S ��� ����/�߰��� ������� FEATURE �� 
 * �����Ѵ�. 
 *
 * EF12S ���� ��� ����/�߰��� ���� ����� ��Ÿ FEATURE ���� ������ ����.
 * - CONFIG_SKYCAM_MV9335
 * - SKYCAM_MV9335
 * - F_SKYCAM_SENSOR_MV9335 
 *
 * �߰��� ���� ���
 * vendor\qcom-proprietary\mm-camera\targets\tgtcommon\sensor\mv9335
 * vendor\qcom-proprietary\mm-camera\targets\tgtcommon\sensor\mv9335\mv9335.c
 * vendor\qcom-proprietary\mm-camera\targets\tgtcommon\sensor\mv9335\mv9335.h
 * kernel\drivers\media\video\msm\mv9335.c
 * kernel\drivers\media\video\msm\mv9335.h
 * kernel\drivers\media\video\msm\mv9335_reg.c
 * kernel\drivers\media\video\msm\mv9335_mt9p013_**_**.h */
#define F_SKYCAM_MODEL_EF12S
#define F_SKYCAM_SENSOR_MV9335

/*----------------------------------------------------------------------------*/
/*  MODEL-COMMON                                                              */
/*  �ȵ���̵� ��� ��� �𵨿� ���� ����Ǿ�� �� FEATURE ���               */
/*----------------------------------------------------------------------------*/


/* ���� CS �μ������� �Һ��� �÷� �м��� ���� ���� PC ���α׷��� ����Ͽ� 
 * ī�޶� ���� �ð� ������ PC �� �����Ѵ�. 
 *
 * ���� ����� ���� Ŀ�ǵ� ��缭�� ��õǾ� �����Ƿ� ���� �ڵ���� ���� Ŀ�ǵ� 
 * ���� ��⿡ ���ԵǾ� ������, ���� Ŀ�ǵ� �� PC ���α׷��� ������� �ʰ� ����
 * ���α׷��� ����Ͽ�, �÷��� DIAG ��Ʈ�κ��� ���� �ð� ������ Ȯ���� �� �ִ�.
 *
 * ���� Ŀ�ǵ� ��缭 v10.35 ��� ����
 * PhoneInfoDisplay v4.0 ���α׷����� Ȯ��
 * ��缭�� ���α׷��� DS2�� �ڰ�ȣ ���ӿ��� ���� */
#define F_SKYCAM_FACTORY_PROC_CMD


/* ī�޶� ��ġ ���� OPEN �� ������ ��� (�ܼ� I2C ���� R/W ����, ī�޶� ������) 
 * ���� ó���� ���� �����Ѵ�. 
 *
 * ��ġ ������ OPEN �ϴ� �������� VFE �ʱ�ȭ ���� ī�޶� HW �ʱ�ȭ�� �̷�� 
 * ���µ�, HW �ʱ�ȭ�� ������ ��� VFE �� �ʱ�ȭ �� ���·� ���Եǰ� ����
 * OPEN �õ� �� HW �ʱ�ȭ�� �����Ѵ� �ϴ��� �̹� VFE �� �ʱ�ȭ�� �����̹Ƿ� 
 * VFE �ʱ�ȭ �� ������ �߻��Ѵ�.
 * 
 * ȣ����� : vfefn.vfe_init() -> sctrl.s_init()
 *
 * HW �ʱ�ȭ�� ������ ���, �̹� �ʱ�ȭ�� VFE �� RELEASE (vfe_release) ���� 
 * ���� ���� �õ� �� ���� �����ϵ��� �����Ѵ�. 
 *
 * ECLAIR ���������� ���� ���� ���� ó������ �ұ��ϰ� ������ ����Ǿ� ����
 * �ʰų� ���� �ϵ��� �̻��� �߻��� ��� ī�޶� ������ ANR ������ ���� 
 * ������ ����ǰ� ���� ����� �������� �����Ͽ� �������� �Ұ����ϴ�.
 *
 * ������ �� ������ ���, ISP �ʱ�ȭ �� ISP �� ������ ������ ���� �����ϴ� 
 * �������� 3�� �� POLLING �����ϸ�, �̷� ���� Ÿ�Ӿƿ� �߻��ϰ� ANR ������ 
 * �̾�����. �� ���� ���� ���� ī�޶� ������ �� ������ �����̶� �ϴ��� ANR 
 * ���� ���� ������ �� ���������� ����Ǿ����Ƿ� FRAMEWORK ���δ� �� ������ 
 * ���·� �����ǰ�, �̷� ���� ����� �������� ī�޶� ���� ���� �� "Unable to 
 * connect camera device" �˾��� �Բ� ������ ���Կ� �����Ѵ�.
 *
 * ISP �ʱ�ȭ �� ������ ��� ���� ������, ISP �� ���� ������ Ư�� �������͸� 
 * 1ȸ READ �ϰ� �� ������ ���, �̸� FRAMWORK �� ���� �������� �����Ͽ� 
 * ���������� ����ǵ��� �����Ѵ�. 
 *
 * ���� ISP ��ü�� �̻��� �߻��� ��쿡��, PROBE �ÿ� ���� �߻��Ͽ� �ش� 
 * ����̽� ���ϵ��� ������ �� �����Ƿ� FRAMWORK ���ο��� �Բ� ó�� �����ϴ�. 
 *
 * P11185@100609, F_SKYCAM_MODEL_EF12S
 * EF10S �� ���, BAYER ������ Ŀ���ͷ� ����Ǿ� �ְ�, MV9337 �� ON-BOARD
 * �Ǿ� �����Ƿ�, BAYER ������ ����Ǿ� ���� �ʾƵ�, MV9337 �� �����̶��,
 * PROBE �� ���� �����Ͽ�����, EF12S �� ���, ī�޶� ��⿡ MV9335 �� �Բ�
 * �ν���Ǿ� �־�, Ŀ���Ϳ� ����� ������� ������ PROBE �� I2C R/W ���а�
 * ���� �߻�, RETRY �����ϸ鼭 ���� �ð��� 10�� �̻� �����ǰ�, �̷� ����
 * �ٸ� ������ �ʱ�ȭ�� �������� ������ ��ģ��. */
#define F_SKYCAM_INVALIDATE_CAMERA_CLIENT


/* �ܸ����� �Կ��� ������ EXIF TAG ���� �� ������ ���� ������ �����Ѵ�. */
#define F_SKYCAM_OEM_EXIF_TAG


/*----------------------------------------------------------------------------*/
/*  MODEL-SPECIFIC                                                            */
/*  EF10S ���� ����Ǵ� �Ǵ� EF10S ������ ������ FEATURE ���                 */
/*----------------------------------------------------------------------------*/


/* EF10S ���� ���� ������ �Կ� �ػ� ���̺��� �����Ѵ�. 
 *
 * HAL ������ ��ȿ�� �Կ� �ػ󵵸� ���̺� ���·� �����ϰ� ���̺� ���Ե� 
 * �ػ� �̿��� ���� ��û�� ������ ó���Ѵ�. */
#define F_SKYCAM_CUST_PICTURE_SIZES


/* EF10S ���� ���� ������ ������ �ػ� ���̺��� �����Ѵ�. 
 * HAL ������ ��ȿ�� ������ �ػ󵵸� ���̺� ���·� �����ϰ� ���̺� ���Ե� 
 * �ػ� �̿��� ���� ��û�� ������ ó���Ѵ�. */
#define F_SKYCAM_CUST_PREVIEW_SIZES


/* EF10S ���� ���Ǵ� ī�޶� ���� ���̺� (�ִ� ��� ���� �ػ�, �ִ� ������ 
 * �ػ�, AF ���� ����) �� �����Ѵ�. */
#define F_SKYCAM_CUST_SENSOR_TYPE


/* ī�޶� IOCTL Ŀ�ǵ� MSM_CAM_IOCTL_SENSOR_IO_CFG �� Ȯ���Ѵ�. 
 *
 * EF10S ���� �߰��� ��� �� SOC ī�޶� �������� �ʰ� �̱����� �κе��� 
 * ���� �� �߰� �����Ѵ�. */
#define F_SKYCAM_CUST_MSM_CAMERA_CFG


/* HW I2C ������ Ŭ���� �����Ѵ�. 
 *
 * MV9337 �� 64KB NAND �÷����� ���̳ʸ� (�ý��� + Ʃ�� ������) �� WRITE �ϴµ� 
 * �ҿ�Ǵ� �ð��� �ּҷ� ���̱� ����, ���� ī�޶� ����/����/���� �� ü�� 
 * ���� �ӵ� ����� ���� �ִ��� ���� Ŭ���� ����Ѵ�. Ư�� �� �̻����� 
 * ������ ���, WRITE �� ���� �߻��Ѵ�.
 *
 * WS ��� HW I2C ������ ��ϵ� �� ����̽� ����ڵ鿡�� ���� ���� ���θ� 
 * Ȯ�� �� �����Ͽ���. */
#define F_SKYCAM_CUST_I2C_CLK_FREQ


/* ������ �׽�Ʈ ���� (/system/bin/mm-qcamera-test) �� �ʿ� �κ��� �����Ѵ�. 
 *
 * ������ �Ϻ����� �����Ƿ� �Ϻ� ����� ���� ������ �������� ���Ѵ�.
 * �ȵ���̵� �÷����� �ʱ�ȭ�Ǳ� ������ adb shell stop ������� �ý����� 
 * ������Ű��, adb shell �� ���� ���� �� �����Ѵ�. */
#define F_SKYCAM_CUST_LINUX_APP


/* "ro.product.device" ���� �� �������� ���� �ڵ� �������� ���� ���� ī�޶�
 * ���� �ڵ�鿡���� �� ���� ���� �ʰų� 'TARGET_QSD8250' ���� �����Ѵ�. 
 * �ý��� ���� "ro.product.device" �� ������ ������ ��� "qsd8250_surf" �̰�,
 * EF10S �� ��� "ef10s" ���� �����Ǿ� �ִ�. */
#define F_SKYCAM_CUST_TARGET_TYPE_8X50


/* SKAF �� ���, ���� ������鿡 ���� ����Ǵ� �����̹Ƿ� QUALCOMM JPEG ���� 
 * ��� ����� ������� �ʵ��� �����Ѵ�. (DS7�� ��û����)
 *
 * SKAF �� ���, SKIA ���̺귯���� ǥ�� ���̺귯�� ��� 'jpeglib.h' ��
 * ���� ���� ����ϰ�, QUALCOMM �� JPEG ���� ����� ���� 'jpeglib.h' ������ 
 * 'jpeg_decompress_struct' ����ü�� 'struct jpeg_qc_routines * qcroutines'
 * ����� �߰��Ͽ���. �̷� ���� 'jpeg_CreateDecompress()' ���� �� ����ü ũ�� 
 * �� �κп��� ������ �߻��ϰ� �ش� ���ڵ� ������ �����Ѵ�.
 *
 * QUALCOMM JPEG ���� ����� SNAPDRAGON �ھ���� �����ǰ�, 2MP �̻��� 
 * JPEG ���� ���ڵ� �� 18~32% �� ������ ���ȴ�. 
 * (android/external/jpeg/Android.mk, R4070 release note 'D.5 LIBJPEG' ����) 
 * F_SKYCAM_TODO, QUALCOMM �� �� ���� ����ü�� �����ߴ°�? ���� ����ü�� 
 * �и��Ͽ� �����ߴٸ�, �̷� ������ ���� �� �����ٵ�... */
#define F_SKYCAM_CUST_QC_JPG_OPTIMIZATION


/* MV9337 ���� ����ϴ� �������� �����Ѵ�.
 *
 * VREG_GP6  : 2.8V_CAM_D
 * VREG_RFTX : 1.8V_CAM
 * VREG_GP1  : 2.6V_CAM (I/O)
 * VREG_GP5  : 2.8V_CAM_A, 1.0V_CAM 
 * 1.0V_CAM �� ���, VREG_GP5 �� ON ��Ű�� ������ PMIC(TPS65023) �� ���� ���
 */
#define F_SKYCAM_HW_POWER


/* ī�޶� GPIO ������ �����Ѵ�. 
 *
 * EF10S ������ �� 12 ������ ������ �������� ���� 8��Ʈ�� ����Ѵ�. 
 * CAM_DATA0(GPIO#0) ~ CAM_DATA11(GPIO#11), CAM_PCLK(#12), CAM_HSYNC_IN(#13), 
 * CAM_VSYNC_IN(#14), CAM_MCLK(#15) �� ��� P4 �е��̰� ������ 2.6V_CAM �� 
 * ����Ѵ�. I2C_SDA, I2C_SCL, CAM_RESET_N �� ��� P3 �е��̰� ������ 
 * 2.6V_MSMP �� ����Ѵ�.
 * ���� 4��Ʈ�� �ٸ� �뵵�� ��� �����ϴ�. */
#define F_SKYCAM_HW_GPIO


#ifdef F_SKYCAM_MODEL_EF12S
/* ���� �ڵ��� ���, ���� �� VFE ���� ���� CALLBACK(mmcamera_shutter_callback)��
 * ���� ī�޶� ���񽺿��� �Կ����� ������ �������� ���� ����Ѵ�.
 * EF12S �� ���, ���� ������ ������ �Կ����� �̹� ����� ���� �̹Ƿ�,
 * �Կ��� ��� ������ ���� ������ �̹����� ������ �������� �����Ѵ�. */
#define F_SKYCAM_DELAY_SHUTTER_SOUND
#endif


/*----------------------------------------------------------------------------*/
/*  SENSOR CONFIGURATION                                                      */
/*  �� �� ��� ����(ISP)�� ���� ����/�߰��� FEATURE ���                    */
/*----------------------------------------------------------------------------*/


/* ī�޶��� ������ ��� ���� ���� SOC ī�޶�(��)�� ����� ��� �����Ѵ�.
 *
 * ������ȭ�� ���� �� ���� ī�޶� ����ϰ�, �ϳ��� SOC Ÿ��, �ٸ� �ϳ���
 * BAYER Ÿ���� ��쿡�� �������� �ʴ´�. ������ ���, BAYER ī�޶� ����
 * �Ϻ� �ڵ���� ������� �ʴ´�.
 *
 * EF10S ������ �ϳ��� SOC ī�޶� ����ϹǷ�, BAYER ���� �ڵ���� �������� 
 * �ʾҰ�, �Ϻδ� �Ʒ� FEATURE ���� ����Ͽ� �ּ� ó���Ͽ���. */
#define F_SKYCAM_YUV_SENSOR_ONLY


#ifdef F_SKYCAM_YUV_SENSOR_ONLY


/* �ȵ���̵� ������ ���� MV9337 �� NAND �÷����� ������Ʈ�ϱ� ���� 
 * �������̽��� �߰��Ѵ�. EF10S ������ ������� ������, ��������� �ڵ���� 
 * ���ܵд�. 
 *
 * ������ �׽�Ʈ ���� (/system/bin/mm-qcamera-test) �� LED ���� ����� ����Ͽ�
 * �׽�Ʈ �����ϴ�. EF10S ������ WS/DONUT ��� Ŀ�� ���� ���Ŀ� I2C BURST WRITE 
 * �� ���� �����Ͽ� ������� �ʾ�����, ���� ECLAIR ���� ���� Ȯ������ �ʾҴ�. 
 * Ʃ�� �۾� ���Ǹ� ���� ���� ���� �ʿ��ϴ�. 
 *
 * P11185@100609, F_SKYCAM_MODEL_EF12S
 * ECLAIR ������ ���Ŀ� I2C �� ��ü������ ����ȭ�Ǿ���, EF10S ��� ������
 * ���� �׽�Ʈ ���, ISP ������Ʈ ������ ���ٸ� ������ �����Ƿ� EF12S Ʃ����
 * ���� ���� �ڵ���� Ȱ��ȭ�ϰ� ���� �ȵ���̵� ������ ���� ������Ʈ��
 * �����ϵ��� �����Ѵ�. */
#define F_SKYCAM_ADD_CFG_UPDATE_ISP


/* MV9337 ��ü���� ���� ���� ZOOM �� �����ϱ� ���� �������̽��� �߰��Ѵ�. 
 * EF10S ������ QUALCOMM ZOOM �� ����ϸ�, ��������� �ڵ���� ���ܵд�.
 *
 * MV9337 ��ü ZOOM �� ���, ������/������ ��忡�� �̹� ZOOM �� ����� �̹����� 
 * ��µǸ� �� ���� ��带 �����Ѵ�.
 *
 * 1) DIGITAL (SUBSAMPLE & RESIZE)
 *     ������/������ �ػ󵵺��� ������ ������ �����Ѵ�. MV9337 ���ο��� 
 *     �̹����� SUBSAMPLE �Ͽ� RESIZE �� ����ϸ�, �̷� ���� ZOOM ������
 *     0 �� �ƴ� ������ ������ ��� ������ FPS �� 1/2 �� ���ҵȴ�.
 * 2) SUBSAMPLE ONLY
 *     ������/������ �ػ󵵺��� ������ ������ �����Ѵ�. MV9337 ���ο��� 
 *     SUBSAMPLE �� �����ϹǷ� ���� �ػ󵵿����� ���� ������ �����ϰ� �ִ� 
 *     �ػ󵵿����� ZOOM ��ü�� �Ұ����ϴ�. ������ FPS �� ���ҵ��� �ʴ´�.
 *
 * QUALCOMM ZOOM ���� ��, ī�޶��� ��� ��� �ػ󵵿��� ���� ���� ZOOM �� 
 * �����ϹǷ� �̸� ����ϸ�, ���� ���� ���� �ش� �ڵ���� ���ܵд�. 
 *
 * ���� FEATURE : F_SKYCAM_FIX_CFG_ZOOM, F_SKYCAM_ADD_CFG_DIMENSION */
/* #define F_SKYCAM_ADD_CFG_SZOOM */


/* MV9337 ���� �����Ǵ� �ն��� ���� ��� (Digital Image Stabilization) �� ����
 * �������̽��� �߰��Ѵ�. 
 *
 * ����/�¿� ���� �������� ��鸱 ��츸 ���� �����ϴ�. 
 * ��� ��带 OFF �̿��� ���� ������ ���, ���� �ն��� ���� ������ 
 * ���õȴ�. */
#define F_SKYCAM_ADD_CFG_ANTISHAKE


/* AF WINDOW ������ ���� �������̽��� �����Ѵ�. SPOT FOCUS ��� ���� �� 
 * ����Ѵ�.
 *
 * MV9337 ������ ������ ��� ��� �ػ󵵸� �������� ����/���θ� ���� 16 ���� 
 * �������� �� 256 �� ������ ������ �� ������ AF WINDOW ������ �����ϴ�. 
 * ���뿡���� ������ �ػ󵵸� �������� ����ڰ� ��ġ�� ������ ��ǥ�� HAL �� 
 * �����ϰ�, HAL ������ �̸� �� ��ǥ�� ��ȯ�Ͽ� MV9337 �� �����Ѵ�. 
 * ���� AF ���� �� �� WINDOW �� ���Ե� �̹��������� FOCUS VALUE �� �����Ͽ�
 * ������ ��ġ�� �����Ѵ�.
 *
 * MV9337 �� ����� ������ ���¿��� QUALCOMM ZOOM �� ����Ͽ� SUBSAMPLE/RESIZE
 * �ϱ� ������ ZOOM �� 0 ���� �̻����� ������ ���, HAL ���� ��ǥ-to-���
 * ��ȯ���� ����������, Ư�� ZOOM ���� �̻��� ��� �� ���� ��� �ȿ� ��ü
 * ������ ������ ���ԵǾ� �����Ƿ� ���� ��ü�� �ǹ̰� ����.
 * �׷��Ƿ�, ������ SPOT FOCUS ��� ��� �ÿ��� ZOOM ����� ����� �� ������ 
 * ó�� �ؾ��Ѵ�. */
#define F_SKYCAM_FIX_CFG_FOCUS_RECT


/* QUALCOMM BAYER �ַ�� ����� ȭ��Ʈ�뷱�� ���� �������̽��� �����Ѵ�. 
 *
 * ��� ��带 OFF �̿��� ���� ������ ���, ���� ȭ��Ʈ�뷱�� ������ 
 * ���õȴ�. */
#define F_SKYCAM_FIX_CFG_WB


/* QUALCOMM BAYER �ַ�� ����� ���� ���� �������̽��� �����Ѵ�. 
 *
 * ��� ��带 OFF �̿��� ���� ������ ���, ���� ���� ������ ���õȴ�. */
#define F_SKYCAM_FIX_CFG_EXPOSURE


/* ��� ��� ������ ���� �������̽��� �߰��Ѵ�. 
 *
 * ��� ��带 OFF �̿��� ������ ������ ��� ���� ȭ��Ʈ�뷱��/����/�ն�������/
 * ISO ������ ���õȴ�. ���뿡�� ��� ��带 �ٽ� OFF �� �ʱ�ȭ �ϴ� ���, 
 * ȭ��Ʈ�뷱��/����/�ն�������/ISO �� HAL ���� ���� ������� �ڵ� �����ǹǷ�,
 * ������ ������ �ʿ� ����. (HW ��������̹Ƿ�, HAL ���� �����Ѵ�.) */
#define F_SKYCAM_FIX_CFG_SCENE_MODE


/* �ø�Ŀ ������ ���� �������̽��� �����Ѵ�.
 *
 * 2.1 SDK ���� �� �� ���� ��� (OFF/50Hz/60Hz/AUTO) �� ��õǾ� ������, 
 * MV9337 �� ��� OFF/AUTO �� �������� �ʴ´�. �׷��Ƿ�, ������ OFF �� ���� 
 * �ÿ��� Ŀ�� ����̹����� 60Hz �� �����ϰ�, AUTO �� ������ ��� HAL ���� 
 * �ý��� ���� �� �� ���� �ڵ� ("gsm.operator.numeric", �� 3�ڸ� ����) �� �а�, 
 * ������ Hz ������ ��ȯ�Ͽ� �ش� ������ �����Ѵ�.
 *
 * ��ȹ�� ���� ���, �ø�Ŀ�� �Ϲ����� ����� �ƴϹǷ�, ���� �ڵ带 �ν��Ͽ� 
 * �ڵ����� ������ �� �ֵ��� �ϰ�, ���� ���� �޴��� ���� ó���Ѵ�. */
#define F_SKYCAM_FIX_CFG_ANTIBANDING


/* �÷��� LED ������ ���� �������̽��� �����Ѵ�.
 *
 * QUALCOMM ������ ������ IOCTL (MSM_CAM_IOCTL_FLASH_LED_CFG) Ŀ�ǵ带 
 * ����Ͽ� �����Ǿ� ������, PMIC ������ ����ϴ� LED ����̹��� �����Ѵ�.
 * EF10S ������ �̸� ������� ������, MSM_CAM_IOCTL_SENSOR_IO_CFG �� Ȯ���Ͽ�
 * �����Ѵ�.
 *
 * AUTO ���� ������ ���, ������ �� ��쿡�� AF ���� �� AF/AE �� ����
 * ��� ON �ǰ�, ���� ������ �������� �� �� �� ON �ȴ�. */
#define F_SKYCAM_FIX_CFG_LED_MODE


/* ISO ������ ���� �������̽��� �����Ѵ�.
 *
 * ��ȹ�� ���� ���, AUTO ��忡���� ȭ���� ū �̻��� �����Ƿ� ��������
 * ISO �� ������ �� �ִ� �޴��� ���� ó���Ѵ�.
 * ��� ��带 OFF �̿��� ���� ������ ���, ���� ISO ������ ���õȴ�. */
#define F_SKYCAM_FIX_CFG_ISO


/* Ư��ȿ�� ������ ���� �������̽��� �����Ѵ�.
 *
 * SDK 2.1 �� ��õ� ȿ���� �� �Ϻθ� �����Ѵ�. MV9337 �� ��� SDK �� ��õ���
 * ���� ȿ���鵵 ���������� EF10S ���� ������� �����Ƿ� �߰����� �ʴ´�. */
#define F_SKYCAM_FIX_CFG_EFFECT


/* MANUAL FOCUS ������ ���� �������̽��� �����Ѵ�. 
 *
 * MV9337 �� MANUAL FOCUS ���� ������ ���, �� ������ ���Ƿ� ������ ��ġ��
 * �̵���ų ���, ������ ������ AF �� ������� �ʴ� ���� �����̸�, �̷� ���� 
 * MV9337 ���� ������ �����Ͽ� ������ �� �ִ� �ð��� �����Ƿ� �÷����� ON ��
 * ���, ������/������ �̹����� ������ �� �ִ�. �׷��Ƿ� ������ MANUAL FOCUS 
 * ��� ���� �� �ݵ�� �÷��� ��带 OFF �� �����ؾ� �ϰ�, AUTO FOCUS �Ǵ� 
 * SPOT FOCUS ��� ���� �� ���� ���� �������Ѿ� �Ѵ�. */
#define F_SKYCAM_FIX_CFG_FOCUS_STEP


/* ��� ������ ���� �������̽��� �����Ѵ�. */
#define F_SKYCAM_FIX_CFG_BRIGHTNESS


/* LENS SHADE ������ ���� �������̽��� �����Ѵ�. 
 *
 * MV9337 �� ��� ������ LENS SHADE ����� �������� �ʰ�, ������/������ ��
 * �׻� LENS SHADE �� ����� �̹����� ����Ѵ�. */
#define F_SKYCAM_FIX_CFG_LENSSHADE


/* ������ ȸ���� ������ ���� �������̽��� �����Ѵ�.
 *
 * ������ ���� JPEG ���ڵ� �ÿ� �ȸ��ν� ���� (������) ���� �� ī�޶��� 
 * ȸ�� ���¸� �Է��Ͽ��� �Ѵ�. ������ OrientationListener ��� �� �Ʒ��� ����
 * ������ HAL �� ȸ���� ���� ���� ���־�� �Ѵ�.
 * 
 * JPEG ���ڵ�
 *     ���ڵ� ������ ����
 * ������ ���
 *     ���� �� �Ź� ����
 *
 * F_SKYCAM_TODO, �ö���� ����ũ/Ư��ȿ������ ���� ROTATION �� �ڵ� �νĵǾ� 
 * ����Ǵµ� ���� �������� ��츸 �̸� �Է����־�� �Ѵ�? ��? */
#define F_SKYCAM_FIX_CFG_ROTATION


/* AF ������ ���� �������̽��� �����Ѵ�. 
 *
 * QUICK SEARCH �˰����� ����ϸ�, ���� �̵� �ø��� FOCUS VALUE �� �����ϰ�,
 * �����ؼ� FOCUS VALUE �� ������ ���, ���� �ֱٿ� �ְ��� FOCUS VALUE ����
 * ���� ��ġ�� ��� �̵���Ų��.
 * EF10S �� ���, ���ѰŸ��� �ǻ�ü�� �Կ��� ��� ��� ��ũ�� ������
 * �̵���Ű�� �����Ƿ� �ؼ����� ��� AF �ӵ��� ����ϴ�. */
#define F_SKYCAM_FIX_CFG_AF


/* AF ���� ��� ������ ���� �������̽��� �����Ѵ�.
 *
 * EF10S ������ ������ ��� ������ ���̵�, �ǻ�ü�� �Ÿ��� ���� AF ���� �ð��� 
 * �ٸ� �� ���ۿ��� �̻��� �����Ƿ� ��ȹ�� ���� �� �޴������� ���� ó���Ѵ�. */
#define F_SKYCAM_FIX_CFG_FOCUS_MODE


/* ZOOM ������ ���� �������̽��� �����Ѵ�.
 *
 * QUALCOMM ZOOM �� ���, �ִ� ZOOM ������ HAL ���Ͽ��� ������/������ �ػ󵵿� 
 * ���� �����Ǹ�, ������ �� �� ("max-zoom") �� �о� �ּ�/�ִ� ZOOM ������
 * �����ϰ� �� ���� ���� ����θ� �����ؾ� �Ѵ�.
 * ���� �ػ�/�𵨿� ������� ������ ZOOM ���� ������ �����ؾ� �� ���, HAL
 * ������ "max-zoom" ���� ���� ZOOM ���� ������ ������ ������ �� �־�� �Ѵ�.
 *
 * ���� FEATURE : F_SKYCAM_ADD_CFG_SZOOM, F_SKYCAM_ADD_CFG_DIMENSION */
/* #define F_SKYCAM_FIX_CFG_ZOOM */


/* SDK 2.1 ���� ��õǾ� ���� �ʰ�, QUALCOMM ���� �߰��� �޴��̸� ������ ��ȯ
 * �� ��� �⺻ ķ�ڴ� ������ �������ϹǷ� ������ ������ ��ȯ�ϵ��� �����Ѵ�. */
#define F_SKYCAM_FIX_CFG_SHARPNESS
#define F_SKYCAM_FIX_CFG_CONTRAST
#define F_SKYCAM_FIX_CFG_SATURATION


#endif /* F_SKYCAM_YUV_SENSOR_ONLY */


/* �ȸ��ν� ��� �̹��� ���� ������ ���� �������̽��� �߰��Ѵ�.
 *
 * EF10S ������ �ö���� �ַ���� ����ϸ�, ������/������ �̹������� ��
 * ��ġ�� �����Ͽ� �� ������ ����ũ�� Ư��ȿ���� ������ �� �ִ�. 
 *
 * vendor/qcom/android-open/libcamera2/Android.mk ���� �ö���� ���̺귯����
 * ��ũ���Ѿ߸� �����Ѵ�. SKYCAM_FD_ENGINE �� 1 �� ������ ��� �ö���� 
 * �ַ���� ����ϰ�, 0 ���� ������ ��� ī�޶� �Ķ���͸� �߰��� ���·� ���� 
 * ��ɵ��� �������� �ʴ´�. �ٸ� �ַ���� ����� ��� �� ���� Ȯ���Ͽ� 
 * ����Ѵ�. */
#define F_SKYCAM_ADD_CFG_FACE_FILTER


/* MV9337 �� ��� �ػ󵵸� ������ �� �ִ� �������̽��� �߰��Ѵ�.
 *
 * QUALCOMM ZOOM ����� VFE/CAMIF SUBSAMPLE �� MDP RESIZE �� ����ϸ�, ���
 * ������/������ �ػ󵵿� ���� ������ ZOOM ������ �����Ѵ�. �̸� ���� MV9337 ��
 * ��� �ػ󵵸� ������/������ ��忡�� ���� ������ ������ �����ϰ� 
 * VFE/CAMIF/MDP ���� ��ó�� (SUBSAMPLE/RESIZE) �ϴ� �����̴�.
 *
 * �׷���, MV9337 �� ZOOM ����� ��� ��ü�� ZOOM �� ����Ǿ� ��µǹǷ�, 
 * MV9337 ��ü ����� ���뿡�� �䱸�ϴ� �ػ󵵷� �����ϰ� VFE/CAMIF/MDP 
 * ��ɵ��� ��� DISABLE ���Ѿ� �Ѵ�.
 *
 * EF10S ������ ������� ������ ��������� �ڵ���� ���ܵд�.
 * ���� FEATURE : F_SKYCAM_FIX_CFG_ZOOM, F_SKYCAM_ADD_CFG_SZOOM */
/* #define F_SKYCAM_ADD_CFG_DIMENSION */


/* ���� �ν��� ���� ������ ���� �������̽��� �����Ѵ�.
 *
 * EF10S �� ��� ���뿡�� UI �� ���θ��� �����Ͽ� ����ϸ�, HAL ���� �����Ͽ� 
 * ó���ؾ� �� �κ��� ���� ��� �� ���� ����ؾ� �Ѵ�.
 * EF10S ������ ���� ���Ǵ� �κ��� ������ ��������� �ڵ���� ���ܵд�. */
#define F_SKYCAM_FIX_CFG_ORIENTATION


/* ������ FPS ������ ���� �������̽��� �����Ѵ�. 
 *
 * 1 ~ 30 ���� ���� �����ϸ� �ǹ̴� ������ ����.
 *
 * 5 ~ 29 : fixed fps (������ ���� ���� ����) ==> ķ�ڴ� ������ �� ���
 * 30 : 8 ~ 30 variable fps (������ ���� �ڵ� ����) ==> ī�޶� ������ �� ���
 *
 * MV9337 �� ������ ��忡�� ���� 1 ~ 30 �����Ӱ� ���� 8 ~ 30 �������� 
 * �����ϸ�, EF10S ������ ������ ��ȭ �� 24fps (QVGA MMS �� ��� 15fps) ����
 * �����ϰ�, ī�޶� ������ �ÿ��� ���� 8 ~ 30fps ���� �����Ѵ�. */
#define F_SKYCAM_FIX_CFG_PREVIEW_FPS


/* ��Ƽ�� ������ ���� �������̽��� �߰��Ѵ�.
 * 
 * �����Կ�, �����Կ� 4��/9�� ��忡�� �� �Կ� �ø��� ���� ��带 �����ϴ� 
 * ���� �����Ѵ�. ù ��° �Կ����� ������ ������ ���� �����ϰ�, VFE/CAMIF
 * �������� �Ϸ�� �����̹Ƿ�, �� ��° �Կ����ʹ� ������/�����/JPEG ���۸�
 * �ʱ�ȭ�ϰ� VFE/CAMIF �� ������ ��ɸ� �۽��Ѵ�.
 *
 * ������� : ���������� �÷��� ��带 �ڵ����� �����ϰ� �����Կ�, �����Կ� 
 * 4��/9�� ���� ���� �� �Կ� ��, ù ��° �Կ������� �÷����� ON �ǰ�, 
 * �� ��°���ʹ� ON ���� �����Ƿ� �����Կ�, �����Կ� 4��/9�� ��忡����
 * ������ �ݵ�� �÷����� OFF �ؾ��ϸ�, �̿� ���� ���� �� ���� ������
 * ������ �������Ѿ� �Ѵ�. */
#define F_SKYCAM_ADD_CFG_MULTISHOT


/*----------------------------------------------------------------------------*/
/*  MISCELLANEOUS / QUALCOMM BUG                                              */
/*  �� ��� ����/������ Ư���� ���谡 ���� FEATURE ��� QUALCOMM ���׿�     */
/*  ���� �ӽ� ����, SBA ���� � ���� FEATURE ���                           */
/*----------------------------------------------------------------------------*/


/* Ŀ�� ���� �ڵ忡�� SKYCDBG/SKYCERR ��ũ�θ� ����Ͽ� ��µǴ� �޽�������
 * Ȱ��ȭ ��Ų��. 
 * kernel/arch/arm/mach-msm/include/mach/camera.h */
#define F_SKYCAM_LOG_PRINTK


/* ���� ���� ���̰ų� ���� ���䰡 �ʿ��� ����, �Ǵ� Ÿ �𵨿��� ���䰡 �ʿ���
 * ������� ��ŷ�ϱ� ���� ����Ѵ�. */
#define F_SKYCAM_TODO


/* QUALCOMM ������ �ڵ忡 ����� �� �޽����� �߰��Ѵ�. */
#define F_SKYCAM_LOG_DEBUG


/* CS ���� CTS �׽�Ʈ ���� �׸� ���� �����Ѵ�. (R4075 known issue #7)
 *
 * - CtsHardwareTestCases.testSetOneShotPreviewCallback
 * - CtsHardwareTestCases.testSetPreviewDisplay 
 * - cts/tests/tests/hardware/src/android/hardware/cts/CameraTest.java
 *
 * CameraTest.java ������ android.hardware.Camera �ν��Ͻ��� �����Ͽ� ī�޶�
 * open() �� ���Ķ��, ������ �߰� ���� (setParameters()) ���̵� �����䰡 
 * ������ ���¶�� �����Ѵ�. 
 * �׷��� QualcommCameraHardware::initDefaultParameters() ���� ���� �ϵ���
 * �ʱ�ȭ�ϴ� �κ��� R4075 ���� ���ŵǸ鼭 �߻��� side effect �̴�. 
 *
 * F_SKYCAM_TODO, QUALCOMM ���� Ȯ�� �� �����ؾ� �Ѵ�. (SR#307473) */
#define F_SKYCAM_QBUG_CTS_FAILURE


/* ��ȭ ����/���� ��, ȿ������ �ش� ������ ���Ͽ� ��ȭ�Ǵ� ������ �ӽ� �����Ѵ�.
 *
 * ��ȭ ���� �ÿ��� ȿ���� ��� �Ϸ� �ñ��� POLLING �� ��ȭ�� �����ϰ�,
 * �Ϸ� �ÿ��� ��ȭ �Ϸ� ���� 800ms ���� DELAY ���� �� ȿ������ ����Ѵ�.
 *
 * R4070 ������ ��ȭ ���� �� ����� Ŭ���� �ʱ�ȭ�Ǵ� �ð��� ���� �ɷ�
 * �ټ��� �������� DROP �Ǹ鼭 ��ȭ �������� ���� �� ���ɼ��� ���������,
 * R4075 ���� �� �̽��� �����Ǹ鼭 ��ȭ ���� �� ȿ������ 100% Ȯ���� �����ǰ�, 
 * ���� �� 80% �̻� Ȯ���� �����ȴ�.
 *
 * F_SKYCAM_TODO, QUALCOMM ���� Ȯ�� �� �����ؾ� �Ѵ�. (SR#307114) */
#define F_SKYCAM_QBUG_REC_BEEP_IS_RECORDED


/* 1600x1200, 1600x960 �ػ󵵿��� "zoom" �Ķ���͸� 21 �� ���� �� ������ ��
 * ����� YUV �̹����� CROP �ϴ� �������� �߻��ϴ� BUFFER OVERRUN ������ 
 * �ӽ� �����Ѵ�. 
 *
 * QualcommCameraHardware::receiveRawPicture() ���� crop_yuv420(thumbnail) 
 * ȣ�� �� �Ķ���ʹ� width=512, height=384, cropped_width=504, 
 * cropped_height=380 �̸� memcpy ���߿� SOURCE �ּҺ��� DESTINATION �ּҰ� 
 * �� Ŀ���� ��Ȳ�� �߻��Ѵ�.
 *
 * F_SKYCAM_TODO, QUALCOMM ���� Ȯ�� �� �����ؾ� �Ѵ�. (SR#308616) */
#define F_SKYCAM_QBUG_ZOOM_CAUSE_BUFFER_OVERRUN


/* ��� �ػ󵵿��� ZOOM �� Ư�� ���� �̻����� ������ ���, 
 * EXIFTAGID_EXIF_PIXEL_X_DIMENSION, EXIFTAGID_EXIF_PIXEL_Y_DIMENSION �±�
 * ������ �߸��� ���� ����Ǵ� ������ �ӽ� �����Ѵ�.
 *
 * F_SKYCAM_TODO, QUALCOMM ���� Ȯ�� �� �����ؾ� �Ѵ�. (SR#307343) */
#define F_SKYCAM_QBUG_EXIF_IMAGE_WIDTH_HEIGHT


/* ������ ��ȭ ���� ��� ���� ��, ������� �� 300ms ������ ����ǰ� �ð��� 
 * ����Ҽ��� ������� �� �� �̻� �� ��������.
 *
 * OPENCORE ����� �⺻ VIDEO �������� ��� �ÿ��� �ʹݿ� SYNC �� �´� ���·� 
 * ��� ���� ������, �ð��� ����Ҽ��� ������� �� ��������. 
 * ���÷��̾�/��Ÿ��/�������÷��̾�� ��� �ÿ��� ������� ���� ���¿��� ��� 
 * ���۵ǰ� �ð��� ����Ҽ��� ������� �� ��������.
 *
 * ������ ���, �� ������ ���� �������� �ý��� �ð��� ���� TIMESTAMP �� �ٿ�
 * ���ڴ��� ���� ������, ������� ��� ���ڵ� �Ϸ�� ����� ��Ʈ���� ������ 
 * �����ٿ� ���ڵ� BITRATE �� ����Ͽ� ���� TIMESTAMP �� ���̰�, ���ڵ� �Ϸ��
 * ���� ��Ʈ���� �Բ� ���Ϸ� ����ȴ�.
 *
 * ����� ���ڴ��� BITRATE �� ����Ͽ� ���� TIMESTAMP �� �ý��� �ð��� ����
 * ���, ��ȭ ���� �� �ð��� ����Ҽ��� �ý��� �ð��� ����� TIMESTAMP �� ���̰�
 * �����ϸ�, �̸� �����ϱ� ���� ����� TIMESTAMP �� �ý��� �ð��� �̿��ϵ��� 
 * �����Ѵ�.
 *
 * ��ȭ ���� 1 ~ 2�� �� ���ʹ� �� 800ms ������ ���ڵ� �� ����� ��Ʈ���� 
 * AUTHOR �� ���޵Ǹ�, ���� �� ����� �������� TIMESTAMP ���� ���̰� �� ����
 * �۰� ������ ���, ���÷��̾�/�������÷��̾�� ��� �� ������� MUTE �Ǵ� 
 * ��Ȳ�� �߻��Ѵ�. �׷��Ƿ� �ݵ�� �� ����� �������� TIMESTAMP �� BITRATE �� 
 * ����Ͽ� ���� TIMESTAMP ���ٴ� Ŀ���Ѵ�.
 * 
 * F_SKYCAM_TODO, QUALCOMM ���� Ȯ�� �� �����ؾ� �Ѵ�. (SR#308616) */
#define F_SKYCAM_QBUG_REC_AV_SYNC


/* ������ ��ȭ ����/���Ḧ ������ �ݺ��ϰų�, �̾����� ������ ���¿��� �����Կ�
 * ���� �Կ��� ���, MediaPlayer �� �������ϸ鼭 HALT �߻��Ѵ�.
 *
 * MediaPlayer �� ���, ������ ������ ��� �߿� �� �ٽ� ��� �õ��� ��� 100%
 * �������ϹǷ� ���� ������ �����Ͽ� ����ؾ� �� ���, �ݵ�� ���� ����� �Ϸ�
 * �Ǿ����� ���θ� Ȯ�� �� ����ؾ� �Ѵ�. */
#define F_SKYCAM_QBUG_SKIP_CAMERA_SOUND


/* ���ǰ �� 1% �̻󿡼� ���� ���� ī�޶� 2ȸ° ���� �� 10�� �̻� ��� �� 
 * �����䰡 ���۵ǰų� �� ���� �� RESET �߻��Ͽ� SW WORKAROUND �� �����Ѵ�.
 *
 * �ҷ� �������δ� �� ȸ ���� ���� �� ������ �߻��ϴ� ��쵵 �ְ�, ���� �� ����
 * ī�޶� ���԰� ���ÿ� RESET �߻��ϴ� ��쵵 �ִ�.
 *
 * ��κ� �ҷ�ǰ���� ���, 2ȸ° ī�޶� ���� �� CAMIF PAD RESET �� ���ÿ�
 * VFE RESET ACK ���ͷ�Ʈ�� BURST �ϰ� �߻��ϹǷ�, VFE �� ���������� RESET �Ǳ�
 * ������ �߻��ϴ� ��� ���ͷ�Ʈ���� �����ϵ��� �����Ѵ�.
 *
 * ���� ���� ���� RMA (RMA5009140, RMA5009149) ����� Ȯ�εǾ�߸� QSD �ϵ���� 
 * ��ü ��������, SW �������� ���θ� Ȯ���� �� �ִ�.
 *
 * TP2�� (1/89ea) �߻� ��� SR#293365 ����
 * F_SKYCAM_TODO, QUALCOMM ���� Ȯ�� �� �����ؾ� �Ѵ�. (SR#312913) */
#define F_SKYCAM_QBUG_VFE_BURST_INTERRUPTS


/*----------------------------------------------------------------------------*/
/*  MODEL-SPECIFIC CONSTANTS                                                  */
/*  �� ���� ��� ����                                                       */
/*----------------------------------------------------------------------------*/


#if defined(F_SKYCAM_MODEL_EF12S) && defined(F_SKYCAM_ADD_CFG_FACE_FILTER)
#define F_SKYCAM_ADD_CFG_FACE_FILTER_RECT
#endif


/* ī�޶� ���� �ð��� �����ϱ� ���� ������ ���ϸ��̴�. */
#ifdef F_SKYCAM_FACTORY_PROC_CMD
#define C_SKYCAM_FNAME_FACTORY_PROC_CMD	"/data/.factorycamera.dat"
#endif


/* ���� ������ �ּ�/�ִ� ��Ŀ�� �����̴�. */
#ifdef F_SKYCAM_FIX_CFG_FOCUS_STEP
#define C_SKYCAM_MIN_FOCUS_STEP 0	/* ���Ѵ� (default) */
#define C_SKYCAM_MAX_FOCUS_STEP 9	/* ��ũ�� */
#endif


/* ���� ������ �ּ�/�ִ� ��� �ܰ��̴�. */
#ifdef F_SKYCAM_FIX_CFG_BRIGHTNESS
#define C_SKYCAM_MIN_BRIGHTNESS 0	/* -4 */
#define C_SKYCAM_MAX_BRIGHTNESS 8	/* +4 */
#endif


/* ���� ������ �ּ�/�ִ� ZOOM �ܰ��̴�. ���뿡���� ��� ���Ǽ��� ���� �ּ�/�ִ�
 * �ܰ踦 ������ ��� ����Ѵ�.
 * EF10S ������ ������� ������ ��������� �ڵ���� ���ܵд�. */
#ifdef F_SKYCAM_FIX_CFG_ZOOM
#define C_SKYCAM_MIN_ZOOM 0
#define C_SKYCAM_MAX_ZOOM 8
#endif


/* ī�޶� ���뿡�� �Կ� �� ������ EXIF �±� ������ ������ ���� ������ 
 * �����Ѵ�. */
#ifdef F_SKYCAM_OEM_EXIF_TAG
#define C_SKYCAM_EXIF_MAKE		"KDDI-PT"
#define C_SKYCAM_EXIF_MAKE_LEN		8		/* including NULL */
#endif


/* ������ ������ ����� ���, ��� ������ FPS �� �ּ�/�ִ� ���̴�. */
#ifdef F_SKYCAM_FIX_CFG_PREVIEW_FPS
#define C_SKYCAM_MIN_PREVIEW_FPS	5
#define C_SKYCAM_MAX_PREVIEW_FPS	30
#endif


#endif /* CUST_SKYCAM.h */
