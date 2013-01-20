#ifndef __CUST_SKYCAM_H__
#define __CUST_SKYCAM_H__


/*

(1)  하드웨어 구성

EF10S 에서는 별도의 ISP (MtekVision MV9337) 를 보드에 실장하고, Sony BAYER 센서 
(imx034) 를 모듈 타입으로 FPCB 를 통해 SMIA 로 연결하여 사용한다.
그러나, SW 측면에서는 ISP 와 센서가 분리되었을 뿐 통합된 SOC 카메라와 다를 바 
없으므로, 구현 전반에 걸쳐 기존 FEATURE 모델들에서 SOC 카메라를 이용하여 구현한 
방식으로 동일하게 구현하고, 동일한 방식으로 설명한다.

Sony IMX034 (BAYER Sensor) + MtekVision MV9337 (ISP) + LGInnotek (Packaging)
	MCLK = 24MHz, PCLK = 96MHz (PREVIEW/SNAPSHOT), SMIA = 648MHz


(2)  카메라 관련 모든 kernel/userspace/android 로그를 제거

kernel/arch/arm/config/qsd8650-perf_defconfig 를 수정한다.

	# CONFIG_MSM_CAMERA_DEBUG is not set (default)

CUST_SKYCAM.h 에서 F_SKYCAM_LOG_PRINTK 을 #undef 한다. 

	#define F_SKYCAM_LOG_PRINTK (default)

모든 소스 파일에서 F_SKYCAM_LOG_OEM 을 찾아 주석 처리한다. 
	선언 된 경우, 해당 파일에 사용된 SKYCDBG/SKYCERR 매크로를 이용한 
	메시지들을 활성화 시킨다. (user-space only)

모든 소스 파일에서 F_SKYCAM_LOG_CDBG 를 찾아 주석 처리한다. 
	선언 된 경우, 해당 파일에 사용된 CDBG 매크로를 이용한 메시지들을 
	활성화 시킨다. (user-space only)

모든 소스 파일에서 F_SKYCAM_LOG_VERBOSE 를 찾아 주석 처리한다.
	선언 된 경우, 해당 파일에 사용된 LOGV/LOGD/LOGI 매크로를 이용한 
	메시지들을 활성화 시킨다. (user-space only)


(3)  MV9337 관련 빌드 환경 (kernel/user-space)

kernel/arch/arm/config/qsd8650-perf_defconfig 을 수정한다. (kernel)

	CONFIG_MV9337=y (default)	포함
	# CONFIG_MV9337 is not set	미포함

	qsd8650-perf_defconfig 가 Kconfig (default y) 보다 우선 순위가 높으므로 
	이 부분 만 수정하면 된다.

vendor/qcom-proprietary/mm-camera/Android.mk,camera.mk 를 수정한다. (user-space)

	SKYCAM_MV9337=yes (default)	포함
	SKYCAM_MV9337=no		미포함


(4)  안면인식 관련 기능 빌드 환경

vendor/qcom/android-open/libcamera2/Android.mk 를 수정한다.
	3rd PARTY 솔루션 사용 여부를 결정한다.

	SKYCAM_FD_ENGINE := 0		미포함
	SKYCAM_FD_ENGINE := 1		올라웍스 솔루션 사용
	SKYCAM_FD_ENGINE := 2		기타 솔루션 사용

CUST_SKYCAM.h 에서 F_SKYCAM_ADD_CFG_FACE_FILTER 를 #undef 한다.
	안면인식 기능 관련 인터페이스 포함 여부를 결정한다.

libOlaEngine.so 를 기존 libcamera.so 에 링크하므로 기존 대비 libcamera.so 의
크기가 증가하여 링크 오류가 발생 가능하며, 이 경우 아래 파일들에서 
liboemcamera.so 의 영역을 줄여 libcamera.so 의 영역을 확보할 수 있다.

build/core/prelink-linux-arm-2G.map (for 2G/2G)
build/core/prelink-linux-arm.map (for 1G/3G)


(5)  별도 추가된 파일 목록

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


/* EF10S 기반 모델인 EF12S 에서 EF10S 대비 수정/추가한 내용들을 FEATURE 로 
 * 관리한다. 
 *
 * EF12S 에서 기능 수정/추가를 위해 사용한 기타 FEATURE 들은 다음과 같다.
 * - CONFIG_SKYCAM_MV9335
 * - SKYCAM_MV9335
 * - F_SKYCAM_SENSOR_MV9335 
 *
 * 추가된 파일 목록
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
/*  안드로이드 기반 모든 모델에 공통 적용되어야 할 FEATURE 목록               */
/*----------------------------------------------------------------------------*/


/* 내수 CS 부서에서는 소비자 시료 분석을 위해 별도 PC 프로그램을 사용하여 
 * 카메라 구동 시간 정보를 PC 로 추출한다. 
 *
 * 구현 방법은 공정 커맨드 사양서에 명시되어 있으므로 관련 코드들은 공정 커맨드 
 * 관련 모듈에 포함되어 있으나, 공정 커맨드 용 PC 프로그램을 사용하지 않고 별도
 * 프로그램을 사용하여, 시료의 DIAG 포트로부터 구동 시간 정보를 확인할 수 있다.
 *
 * 공정 커맨드 사양서 v10.35 기반 구현
 * PhoneInfoDisplay v4.0 프로그램으로 확인
 * 사양서와 프로그램은 DS2팀 박경호 선임에게 문의 */
#define F_SKYCAM_FACTORY_PROC_CMD


/* 카메라 장치 파일 OPEN 에 실패한 경우 (단순 I2C 버스 R/W 오류, 카메라 미장착) 
 * 오류 처리를 위해 수정한다. 
 *
 * 장치 파일을 OPEN 하는 과정에서 VFE 초기화 이후 카메라 HW 초기화가 이루어 
 * 지는데, HW 초기화에 실패할 경우 VFE 는 초기화 된 상태로 남게되고 다음
 * OPEN 시도 시 HW 초기화에 성공한다 하더라도 이미 VFE 가 초기화된 상태이므로 
 * VFE 초기화 시 오류가 발생한다.
 * 
 * 호출순서 : vfefn.vfe_init() -> sctrl.s_init()
 *
 * HW 초기화에 실패할 경우, 이미 초기화된 VFE 를 RELEASE (vfe_release) 시켜 
 * 다음 열기 시도 시 정상 동작하도록 수정한다. 
 *
 * ECLAIR 버전에서는 위와 같은 에러 처리에도 불구하고 센서가 연결되어 있지
 * 않거나 센서 하드웨어에 이상이 발생한 경우 카메라 응용이 ANR 오류로 인해 
 * 비정상 종료되고 이후 재부팅 전까지는 지속하여 재진입이 불가능하다.
 *
 * 센서가 비 정상인 경우, ISP 초기화 시 ISP 와 센서를 프리뷰 모드로 설정하는 
 * 과정에서 3초 간 POLLING 수행하며, 이로 인해 타임아웃 발생하고 ANR 오류로 
 * 이어진다. 비 정상 종료 이후 카메라 재진입 시 센서가 정상이라 하더라도 ANR 
 * 오류 이후 응용이 비 정상적으로 종료되었으므로 FRAMEWORK 내부는 비 정상인 
 * 상태로 유지되고, 이로 인해 재부팅 전까지는 카메라 응용 진입 시 "Unable to 
 * connect camera device" 팝업과 함께 무조건 진입에 실패한다.
 *
 * ISP 초기화 시 프리뷰 모드 설정 이전에, ISP 를 통해 센서의 특정 레지스터를 
 * 1회 READ 하고 비 정상일 경우, 이를 FRAMWORK 을 통해 응용으로 전달하여 
 * 정상적으로 종료되도록 수정한다. 
 *
 * 또한 ISP 자체에 이상이 발생한 경우에도, PROBE 시에 오류 발생하여 해당 
 * 디바이스 파일들을 생성할 수 없으므로 FRAMWORK 내부에서 함께 처리 가능하다. 
 *
 * P11185@100609, F_SKYCAM_MODEL_EF12S
 * EF10S 의 경우, BAYER 센서만 커넥터로 연결되어 있고, MV9337 은 ON-BOARD
 * 되어 있으므로, BAYER 센서가 연결되어 있지 않아도, MV9337 만 정상이라면,
 * PROBE 시 정상 동작하였으나, EF12S 의 경우, 카메라 모듈에 MV9335 가 함께
 * 인스톨되어 있어, 커넥터에 모듈이 연결되지 않으면 PROBE 시 I2C R/W 실패가
 * 지속 발생, RETRY 수행하면서 부팅 시간이 10초 이상 지연되고, 이로 인해
 * 다른 모듈들의 초기화에 직접적인 영향을 미친다. */
#define F_SKYCAM_INVALIDATE_CAMERA_CLIENT


/* 단말에서 촬영된 사진의 EXIF TAG 정보 중 제조사 관련 정보를 수정한다. */
#define F_SKYCAM_OEM_EXIF_TAG


/*----------------------------------------------------------------------------*/
/*  MODEL-SPECIFIC                                                            */
/*  EF10S 에만 적용되는 또는 EF10S 에서만 검증된 FEATURE 목록                 */
/*----------------------------------------------------------------------------*/


/* EF10S 에서 지원 가능한 촬영 해상도 테이블을 수정한다. 
 *
 * HAL 에서는 유효한 촬영 해상도를 테이블 형태로 관리하고 테이블에 포함된 
 * 해상도 이외의 설정 요청은 오류로 처리한다. */
#define F_SKYCAM_CUST_PICTURE_SIZES


/* EF10S 에서 지원 가능한 프리뷰 해상도 테이블을 수정한다. 
 * HAL 에서는 유효한 프리뷰 해상도를 테이블 형태로 관리하고 테이블에 포함된 
 * 해상도 이외의 설정 요청은 오류로 처리한다. */
#define F_SKYCAM_CUST_PREVIEW_SIZES


/* EF10S 에서 사용되는 카메라 정보 테이블 (최대 출력 가능 해상도, 최대 스냅샷 
 * 해상도, AF 지원 여부) 을 수정한다. */
#define F_SKYCAM_CUST_SENSOR_TYPE


/* 카메라 IOCTL 커맨드 MSM_CAM_IOCTL_SENSOR_IO_CFG 를 확장한다. 
 *
 * EF10S 에서 추가한 기능 및 SOC 카메라를 감안하지 않고 미구현된 부분들을 
 * 수정 및 추가 구현한다. */
#define F_SKYCAM_CUST_MSM_CAMERA_CFG


/* HW I2C 버스의 클럭을 수정한다. 
 *
 * MV9337 의 64KB NAND 플래쉬에 바이너리 (시스템 + 튜닝 데이터) 를 WRITE 하는데 
 * 소요되는 시간을 최소로 줄이기 위해, 또한 카메라 진입/종료/동작 중 체감 
 * 반응 속도 향상을 위해 최대한 높은 클럭을 사용한다. 특정 값 이상으로 
 * 설정할 경우, WRITE 시 오류 발생한다.
 *
 * WS 당시 HW I2C 버스에 등록된 각 디바이스 담당자들에게 정상 동작 여부를 
 * 확인 후 변경하였다. */
#define F_SKYCAM_CUST_I2C_CLK_FREQ


/* 리눅스 테스트 응용 (/system/bin/mm-qcamera-test) 중 필요 부분을 수정한다. 
 *
 * 수정이 완벽하지 않으므로 일부 기능은 정상 동작을 보장하지 못한다.
 * 안드로이드 플랫폼이 초기화되기 직전에 adb shell stop 명령으로 시스템을 
 * 중지시키고, adb shell 을 통해 접근 후 실행한다. */
#define F_SKYCAM_CUST_LINUX_APP


/* "ro.product.device" 설정 값 변경으로 인한 코드 오동작을 막기 위해 카메라
 * 관련 코드들에서는 이 값을 읽지 않거나 'TARGET_QSD8250' 으로 고정한다. 
 * 시스템 설정 "ro.product.device" 는 릴리즈 버전의 경우 "qsd8250_surf" 이고,
 * EF10S 의 경우 "ef10s" 으로 설정되어 있다. */
#define F_SKYCAM_CUST_TARGET_TYPE_8X50


/* SKAF 의 경우, 국내 제조사들에 공통 적용되는 내용이므로 QUALCOMM JPEG 성능 
 * 향상 기능을 사용하지 않도록 수정한다. (DS7팀 요청사항)
 *
 * SKAF 의 경우, SKIA 라이브러리의 표준 라이브러리 헤더 'jpeglib.h' 를
 * 수정 없이 사용하고, QUALCOMM 은 JPEG 성능 향상을 위해 'jpeglib.h' 파일의 
 * 'jpeg_decompress_struct' 구조체에 'struct jpeg_qc_routines * qcroutines'
 * 멤버를 추가하였다. 이로 인해 'jpeg_CreateDecompress()' 실행 시 구조체 크기 
 * 비교 부분에서 오류가 발생하고 해당 디코드 세션은 실패한다.
 *
 * QUALCOMM JPEG 성능 향상은 SNAPDRAGON 코어에서만 지원되고, 2MP 이상의 
 * JPEG 파일 디코드 시 18~32% 의 성능이 향상된다. 
 * (android/external/jpeg/Android.mk, R4070 release note 'D.5 LIBJPEG' 참고) 
 * F_SKYCAM_TODO, QUALCOMM 은 왜 공용 구조체를 수정했는가? 별도 구조체로 
 * 분리하여 구현했다면, 이런 문제는 피할 수 있을텐데... */
#define F_SKYCAM_CUST_QC_JPG_OPTIMIZATION


/* MV9337 에서 사용하는 전원들을 설정한다.
 *
 * VREG_GP6  : 2.8V_CAM_D
 * VREG_RFTX : 1.8V_CAM
 * VREG_GP1  : 2.6V_CAM (I/O)
 * VREG_GP5  : 2.8V_CAM_A, 1.0V_CAM 
 * 1.0V_CAM 의 경우, VREG_GP5 를 ON 시키는 시점에 PMIC(TPS65023) 를 통해 출력
 */
#define F_SKYCAM_HW_POWER


/* 카메라 GPIO 설정을 변경한다. 
 *
 * EF10S 에서는 총 12 라인의 데이터 버스에서 상위 8비트만 사용한다. 
 * CAM_DATA0(GPIO#0) ~ CAM_DATA11(GPIO#11), CAM_PCLK(#12), CAM_HSYNC_IN(#13), 
 * CAM_VSYNC_IN(#14), CAM_MCLK(#15) 의 경우 P4 패드이고 전원은 2.6V_CAM 을 
 * 사용한다. I2C_SDA, I2C_SCL, CAM_RESET_N 의 경우 P3 패드이고 전원은 
 * 2.6V_MSMP 를 사용한다.
 * 하위 4비트는 다른 용도로 사용 가능하다. */
#define F_SKYCAM_HW_GPIO


#ifdef F_SKYCAM_MODEL_EF12S
/* 원본 코드의 경우, 센서 및 VFE 설정 직후 CALLBACK(mmcamera_shutter_callback)을
 * 통해 카메라 서비스에서 촬영음을 스냅샷 시점보다 일찍 재생한다.
 * EF12S 의 경우, 실제 스냅샷 시점이 촬영음이 이미 재생된 이후 이므로,
 * 촬영음 재생 시점을 실제 스냅샷 이미지를 수신한 시점으로 수정한다. */
#define F_SKYCAM_DELAY_SHUTTER_SOUND
#endif


/*----------------------------------------------------------------------------*/
/*  SENSOR CONFIGURATION                                                      */
/*  모델 별 사용 센서(ISP)를 위해 수정/추가된 FEATURE 목록                    */
/*----------------------------------------------------------------------------*/


/* 카메라의 개수에 상관 없이 오직 SOC 카메라(들)만 사용할 경우 선언한다.
 *
 * 영상통화를 위해 두 개의 카메라를 사용하고, 하나는 SOC 타입, 다른 하나는
 * BAYER 타입인 경우에는 선언하지 않는다. 선언할 경우, BAYER 카메라를 위한
 * 일부 코드들이 실행되지 않는다.
 *
 * EF10S 에서는 하나의 SOC 카메라만 사용하므로, BAYER 관련 코드들을 검증하지 
 * 않았고, 일부는 아래 FEATURE 들을 사용하여 주석 처리하였다. */
#define F_SKYCAM_YUV_SENSOR_ONLY


#ifdef F_SKYCAM_YUV_SENSOR_ONLY


/* 안드로이드 응용을 통해 MV9337 의 NAND 플래쉬를 업데이트하기 위한 
 * 인터페이스를 추가한다. EF10S 에서는 사용하지 않으며, 참고용으로 코드들은 
 * 남겨둔다. 
 *
 * 리눅스 테스트 응용 (/system/bin/mm-qcamera-test) 의 LED 제어 기능을 사용하여
 * 테스트 가능하다. EF10S 에서는 WS/DONUT 당시 커널 부팅 이후에 I2C BURST WRITE 
 * 가 자주 실패하여 사용하지 않았으며, 추후 ECLAIR 에서 별도 확인하지 않았다. 
 * 튜닝 작업 편의를 위해 추후 구현 필요하다. 
 *
 * P11185@100609, F_SKYCAM_MODEL_EF12S
 * ECLAIR 릴리즈 이후에 I2C 가 전체적으로 안정화되었고, EF10S 출시 시점에
 * 최종 테스트 결과, ISP 업데이트 구현에 별다른 문제가 없으므로 EF12S 튜닝을
 * 위해 관련 코드들을 활성화하고 별도 안드로이드 응용을 통해 업데이트가
 * 가능하도록 수정한다. */
#define F_SKYCAM_ADD_CFG_UPDATE_ISP


/* MV9337 자체에서 지원 센서 ZOOM 을 설정하기 위한 인터페이스를 추가한다. 
 * EF10S 에서는 QUALCOMM ZOOM 을 사용하며, 참고용으로 코드들은 남겨둔다.
 *
 * MV9337 자체 ZOOM 의 경우, 프리뷰/스냅샷 모드에서 이미 ZOOM 이 적용된 이미지가 
 * 출력되며 두 가지 모드를 지원한다.
 *
 * 1) DIGITAL (SUBSAMPLE & RESIZE)
 *     프리뷰/스냅샷 해상도별로 동일한 배율을 지원한다. MV9337 내부에서 
 *     이미지를 SUBSAMPLE 하여 RESIZE 후 출력하며, 이로 인해 ZOOM 레벨이
 *     0 이 아닌 값으로 설정된 경우 프리뷰 FPS 가 1/2 로 감소된다.
 * 2) SUBSAMPLE ONLY
 *     프리뷰/스냅샷 해상도별로 상이한 배율을 지원한다. MV9337 내부에서 
 *     SUBSAMPLE 만 적용하므로 낮은 해상도에서는 높은 배율을 지원하고 최대 
 *     해상도에서는 ZOOM 자체가 불가능하다. 프리뷰 FPS 는 감소되지 않는다.
 *
 * QUALCOMM ZOOM 적용 시, 카메라의 경우 모든 해상도에서 동일 배율 ZOOM 이 
 * 가능하므로 이를 사용하며, 추후 참고를 위해 해당 코드들은 남겨둔다. 
 *
 * 관련 FEATURE : F_SKYCAM_FIX_CFG_ZOOM, F_SKYCAM_ADD_CFG_DIMENSION */
/* #define F_SKYCAM_ADD_CFG_SZOOM */


/* MV9337 에서 지원되는 손떨림 보정 기능 (Digital Image Stabilization) 을 위한
 * 인터페이스를 추가한다. 
 *
 * 상하/좌우 일정 패턴으로 흔들릴 경우만 보정 가능하다. 
 * 장면 모드를 OFF 이외의 모드로 설정할 경우, 기존 손떨림 보정 설정은 
 * 무시된다. */
#define F_SKYCAM_ADD_CFG_ANTISHAKE


/* AF WINDOW 설정을 위한 인터페이스를 수정한다. SPOT FOCUS 기능 구현 시 
 * 사용한다.
 *
 * MV9337 에서는 프리뷰 모드 출력 해상도를 기준으로 가로/세로를 각각 16 개의 
 * 구간으로 총 256 개 블럭으로 나누어 블럭 단위로 AF WINDOW 설정이 가능하다. 
 * 응용에서는 프리뷰 해상도를 기준으로 사용자가 터치한 지점의 좌표를 HAL 로 
 * 전달하고, HAL 에서는 이를 블럭 좌표로 변환하여 MV9337 에 설정한다. 
 * 이후 AF 수행 시 이 WINDOW 에 포함된 이미지에서만 FOCUS VALUE 를 측정하여
 * 렌즈의 위치를 결정한다.
 *
 * MV9337 의 출력은 고정된 상태에서 QUALCOMM ZOOM 을 사용하여 SUBSAMPLE/RESIZE
 * 하기 때문에 ZOOM 이 0 레벨 이상으로 설정된 경우, HAL 에서 좌표-to-블록
 * 변환식이 복잡해지고, 특정 ZOOM 레벨 이상일 경우 몇 개의 블록 안에 전체
 * 프리뷰 영역이 포함되어 버리므로 설정 자체가 의미가 없다.
 * 그러므로, 응용은 SPOT FOCUS 기능 사용 시에는 ZOOM 기능을 사용할 수 없도록 
 * 처리 해야한다. */
#define F_SKYCAM_FIX_CFG_FOCUS_RECT


/* QUALCOMM BAYER 솔루션 기반의 화이트밸런스 설정 인터페이스를 수정한다. 
 *
 * 장면 모드를 OFF 이외의 모드로 설정할 경우, 기존 화이트밸런스 설정은 
 * 무시된다. */
#define F_SKYCAM_FIX_CFG_WB


/* QUALCOMM BAYER 솔루션 기반의 노출 설정 인터페이스를 수정한다. 
 *
 * 장면 모드를 OFF 이외의 모드로 설정할 경우, 기존 노출 설정은 무시된다. */
#define F_SKYCAM_FIX_CFG_EXPOSURE


/* 장면 모드 설정을 위한 인터페이스를 추가한다. 
 *
 * 장면 모드를 OFF 이외의 값으로 설정할 경우 기존 화이트밸런스/측광/손떨림보정/
 * ISO 설정은 무시된다. 응용에서 장면 모드를 다시 OFF 로 초기화 하는 경우, 
 * 화이트밸런스/측광/손떨림보정/ISO 는 HAL 에서 기존 설정대로 자동 복구되므로,
 * 응용은 복구할 필요 없다. (HW 제약사항이므로, HAL 에서 제어한다.) */
#define F_SKYCAM_FIX_CFG_SCENE_MODE


/* 플리커 설정을 위한 인터페이스를 수정한다.
 *
 * 2.1 SDK 에는 총 네 가지 모드 (OFF/50Hz/60Hz/AUTO) 가 명시되어 있으나, 
 * MV9337 의 경우 OFF/AUTO 가 지원되지 않는다. 그러므로, 응용이 OFF 로 설정 
 * 시에는 커널 드라이버에서 60Hz 로 설정하고, AUTO 로 설정할 경우 HAL 에서 
 * 시스템 설정 값 중 국가 코드 ("gsm.operator.numeric", 앞 3자리 숫자) 를 읽고, 
 * 국가별 Hz 값으로 변환하여 해당 값으로 설정한다.
 *
 * 기획팀 문의 결과, 플리커는 일반적인 기능이 아니므로, 국가 코드를 인식하여 
 * 자동으로 설정할 수 있도록 하고, 수동 설정 메뉴는 삭제 처리한다. */
#define F_SKYCAM_FIX_CFG_ANTIBANDING


/* 플래쉬 LED 설정을 위한 인터페이스를 수정한다.
 *
 * QUALCOMM 에서는 별도의 IOCTL (MSM_CAM_IOCTL_FLASH_LED_CFG) 커맨드를 
 * 사용하여 구현되어 있으며, PMIC 전원을 사용하는 LED 드라이버를 제공한다.
 * EF10S 에서는 이를 사용하지 않으며, MSM_CAM_IOCTL_SENSOR_IO_CFG 을 확장하여
 * 구현한다.
 *
 * AUTO 모드로 설정할 경우, 저조도 일 경우에만 AF 수행 중 AF/AE 를 위해
 * 잠시 ON 되고, 실제 스냅샷 시점에서 한 번 더 ON 된다. */
#define F_SKYCAM_FIX_CFG_LED_MODE


/* ISO 설정을 위한 인터페이스를 수정한다.
 *
 * 기획팀 문의 결과, AUTO 모드에서의 화질에 큰 이상이 없으므로 수동으로
 * ISO 를 변경할 수 있는 메뉴는 삭제 처리한다.
 * 장면 모드를 OFF 이외의 모드로 설정할 경우, 기존 ISO 설정은 무시된다. */
#define F_SKYCAM_FIX_CFG_ISO


/* 특수효과 설정을 위한 인터페이스를 수정한다.
 *
 * SDK 2.1 에 명시된 효과들 중 일부만 지원한다. MV9337 의 경우 SDK 에 명시되지
 * 않은 효과들도 지원하지만 EF10S 에서 사용하지 않으므로 추가하지 않는다. */
#define F_SKYCAM_FIX_CFG_EFFECT


/* MANUAL FOCUS 설정을 위한 인터페이스를 수정한다. 
 *
 * MV9337 을 MANUAL FOCUS 모드로 설정할 경우, 즉 응용이 임의로 렌즈의 위치를
 * 이동시킬 경우, 스냅샷 직전에 AF 가 수행되지 않는 것이 정상이며, 이로 인해 
 * MV9337 에서 조도를 측정하여 보정할 수 있는 시간이 없으므로 플래쉬가 ON 될
 * 경우, 프리뷰/스냅샷 이미지가 상이할 수 있다. 그러므로 응용은 MANUAL FOCUS 
 * 모드 진입 시 반드시 플래쉬 모드를 OFF 로 설정해야 하고, AUTO FOCUS 또는 
 * SPOT FOCUS 모드 진입 시 원래 모드로 복구시켜야 한다. */
#define F_SKYCAM_FIX_CFG_FOCUS_STEP


/* 밝기 설정을 위한 인터페이스를 수정한다. */
#define F_SKYCAM_FIX_CFG_BRIGHTNESS


/* LENS SHADE 설정을 위한 인터페이스를 수정한다. 
 *
 * MV9337 의 경우 별도의 LENS SHADE 기능을 지원하지 않고, 프리뷰/스냅샷 시
 * 항상 LENS SHADE 가 적용된 이미지를 출력한다. */
#define F_SKYCAM_FIX_CFG_LENSSHADE


/* 프리뷰 회전각 설정을 위한 인터페이스를 수정한다.
 *
 * 스냅샷 이후 JPEG 인코드 시와 안면인식 필터 (셀프샷) 적용 시 카메라의 
 * 회전 상태를 입력하여야 한다. 응용은 OrientationListener 등록 후 아래와 같은
 * 시점에 HAL 에 회전각 값을 설정 해주어야 한다.
 * 
 * JPEG 인코드
 *     인코드 직전에 설정
 * 셀프샷 모드
 *     변경 시 매번 설정
 *
 * F_SKYCAM_TODO, 올라웍스 마스크/특수효과들의 경우는 ROTATION 이 자동 인식되어 
 * 적용되는데 반해 셀프샷의 경우만 이를 입력해주어야 한다? 왜? */
#define F_SKYCAM_FIX_CFG_ROTATION


/* AF 동작을 위한 인터페이스를 수정한다. 
 *
 * QUICK SEARCH 알고리즘을 사용하며, 렌즈 이동 시마다 FOCUS VALUE 를 측정하고,
 * 연속해서 FOCUS VALUE 가 감소할 경우, 가장 최근에 최고의 FOCUS VALUE 값을
 * 갖는 위치로 렌즈를 이동시킨다.
 * EF10S 의 경우, 무한거리의 피사체를 촬영할 경우 렌즈를 매크로 모드까지
 * 이동시키지 않으므로 넥서스원 대비 AF 속도가 우수하다. */
#define F_SKYCAM_FIX_CFG_AF


/* AF 동작 모드 설정을 위한 인터페이스를 수정한다.
 *
 * EF10S 에서는 별도의 모드 설정이 없이도, 피사체의 거리에 따라 AF 수행 시간만 
 * 다를 뿐 동작에는 이상이 없으므로 기획팀 문의 후 메뉴에서는 삭제 처리한다. */
#define F_SKYCAM_FIX_CFG_FOCUS_MODE


/* ZOOM 설정을 위한 인터페이스를 수정한다.
 *
 * QUALCOMM ZOOM 의 경우, 최대 ZOOM 레벨은 HAL 이하에서 프리뷰/스냅샷 해상도에 
 * 따라 결정되며, 응용은 이 값 ("max-zoom") 을 읽어 최소/최대 ZOOM 레벨을
 * 결정하고 그 범위 안의 값들로만 설정해야 한다.
 * 만약 해상도/모델에 관계없이 일정한 ZOOM 레벨 범위를 제공해야 할 경우, HAL
 * 에서는 "max-zoom" 값을 일정 ZOOM 레벨 범위로 나누어 설정할 수 있어야 한다.
 *
 * 관련 FEATURE : F_SKYCAM_ADD_CFG_SZOOM, F_SKYCAM_ADD_CFG_DIMENSION */
/* #define F_SKYCAM_FIX_CFG_ZOOM */


/* SDK 2.1 에는 명시되어 있지 않고, QUALCOMM 에서 추가한 메뉴이며 오류를 반환
 * 할 경우 기본 캠코더 응용이 오동작하므로 무조건 성공을 반환하도록 수정한다. */
#define F_SKYCAM_FIX_CFG_SHARPNESS
#define F_SKYCAM_FIX_CFG_CONTRAST
#define F_SKYCAM_FIX_CFG_SATURATION


#endif /* F_SKYCAM_YUV_SENSOR_ONLY */


/* 안면인식 기반 이미지 필터 설정을 위한 인터페이스를 추가한다.
 *
 * EF10S 에서는 올라웍스 솔루션을 사용하며, 프리뷰/스냅샷 이미지에서 얼굴
 * 위치를 검출하여 얼굴 영역에 마스크나 특수효과를 적용할 수 있다. 
 *
 * vendor/qcom/android-open/libcamera2/Android.mk 에서 올라웍스 라이브러리를
 * 링크시켜야만 동작한다. SKYCAM_FD_ENGINE 를 1 로 설정할 경우 올라웍스 
 * 솔루션을 사용하고, 0 으로 설정할 경우 카메라 파라미터만 추가된 상태로 관련 
 * 기능들은 동작하지 않는다. 다른 솔루션을 사용할 경우 이 값을 확장하여 
 * 사용한다. */
#define F_SKYCAM_ADD_CFG_FACE_FILTER


/* MV9337 의 출력 해상도를 설정할 수 있는 인터페이스를 추가한다.
 *
 * QUALCOMM ZOOM 기능은 VFE/CAMIF SUBSAMPLE 과 MDP RESIZE 를 사용하며, 모든
 * 프리뷰/스냅샷 해상도에 대해 동일한 ZOOM 배율을 지원한다. 이를 위해 MV9337 의
 * 출력 해상도를 프리뷰/스냅샷 모드에서 각각 별도의 값으로 고정하고 
 * VFE/CAMIF/MDP 에서 후처리 (SUBSAMPLE/RESIZE) 하는 구조이다.
 *
 * 그러나, MV9337 의 ZOOM 기능은 출력 자체가 ZOOM 이 적용되어 출력되므로, 
 * MV9337 자체 출력을 응용에서 요구하는 해상도로 설정하고 VFE/CAMIF/MDP 
 * 기능들은 모두 DISABLE 시켜야 한다.
 *
 * EF10S 에서는 사용하지 않으며 참고용으로 코드들은 남겨둔다.
 * 관련 FEATURE : F_SKYCAM_FIX_CFG_ZOOM, F_SKYCAM_ADD_CFG_SZOOM */
/* #define F_SKYCAM_ADD_CFG_DIMENSION */


/* 센서 인스톨 방향 설정을 위한 인터페이스를 수정한다.
 *
 * EF10S 의 경우 응용에서 UI 를 가로모드로 고정하여 사용하며, HAL 에서 연동하여 
 * 처리해야 할 부분이 있을 경우 이 값을 사용해야 한다.
 * EF10S 에서는 실제 사용되는 부분은 없으나 참고용으로 코드들은 남겨둔다. */
#define F_SKYCAM_FIX_CFG_ORIENTATION


/* 프리뷰 FPS 설정을 위한 인터페이스를 변경한다. 
 *
 * 1 ~ 30 까지 설정 가능하며 의미는 다음과 같다.
 *
 * 5 ~ 29 : fixed fps (조도에 관계 없이 고정) ==> 캠코더 프리뷰 시 사용
 * 30 : 8 ~ 30 variable fps (조도에 따라 자동 조절) ==> 카메라 프리뷰 시 사용
 *
 * MV9337 은 프리뷰 모드에서 고정 1 ~ 30 프레임과 변동 8 ~ 30 프레임을 
 * 지원하며, EF10S 에서는 동영상 녹화 시 24fps (QVGA MMS 의 경우 15fps) 으로
 * 설정하고, 카메라 프리뷰 시에는 가변 8 ~ 30fps 으로 설정한다. */
#define F_SKYCAM_FIX_CFG_PREVIEW_FPS


/* 멀티샷 설정을 위한 인터페이스를 추가한다.
 * 
 * 연속촬영, 분할촬영 4컷/9컷 모드에서 매 촬영 시마다 센서 모드를 변경하는 
 * 것을 방지한다. 첫 번째 촬영에서 센서를 스냅샷 모드로 변경하고, VFE/CAMIF
 * 설정까지 완료된 상태이므로, 두 번째 촬영부터는 스냅샷/썸네일/JPEG 버퍼만
 * 초기화하고 VFE/CAMIF 에 스냅샷 명령만 송신한다.
 *
 * 제약사항 : 저조도에서 플래쉬 모드를 자동으로 설정하고 연속촬영, 분할촬영 
 * 4컷/9컷 모드로 설정 후 촬영 시, 첫 번째 촬영에서만 플래쉬가 ON 되고, 
 * 두 번째부터는 ON 되지 않으므로 연속촬영, 분할촬영 4컷/9컷 모드에서는
 * 응용이 반드시 플래쉬를 OFF 해야하며, 이외 모드로 설정 시 이전 설정을
 * 스스로 복구시켜야 한다. */
#define F_SKYCAM_ADD_CFG_MULTISHOT


/*----------------------------------------------------------------------------*/
/*  MISCELLANEOUS / QUALCOMM BUG                                              */
/*  모델 기능 구현/수정과 특별히 관계가 없는 FEATURE 들과 QUALCOMM 버그에     */
/*  대한 임시 수정, SBA 적용 등에 대한 FEATURE 목록                           */
/*----------------------------------------------------------------------------*/


/* 커널 영역 코드에서 SKYCDBG/SKYCERR 매크로를 사용하여 출력되는 메시지들을
 * 활성화 시킨다. 
 * kernel/arch/arm/mach-msm/include/mach/camera.h */
#define F_SKYCAM_LOG_PRINTK


/* 현재 개발 중이거나 추후 검토가 필요한 내용, 또는 타 모델에서 검토가 필요한
 * 내용들을 마킹하기 위해 사용한다. */
#define F_SKYCAM_TODO


/* QUALCOMM 릴리즈 코드에 디버그 용 메시지를 추가한다. */
#define F_SKYCAM_LOG_DEBUG


/* CS 버전 CTS 테스트 실패 항목에 대해 수정한다. (R4075 known issue #7)
 *
 * - CtsHardwareTestCases.testSetOneShotPreviewCallback
 * - CtsHardwareTestCases.testSetPreviewDisplay 
 * - cts/tests/tests/hardware/src/android/hardware/cts/CameraTest.java
 *
 * CameraTest.java 에서는 android.hardware.Camera 인스턴스를 생성하여 카메라를
 * open() 한 이후라면, 별도의 추가 설정 (setParameters()) 없이도 프리뷰가 
 * 가능한 상태라고 간주한다. 
 * 그러나 QualcommCameraHardware::initDefaultParameters() 에서 실제 하드웨어를
 * 초기화하는 부분이 R4075 에서 제거되면서 발생한 side effect 이다. 
 *
 * F_SKYCAM_TODO, QUALCOMM 수정 확인 후 삭제해야 한다. (SR#307473) */
#define F_SKYCAM_QBUG_CTS_FAILURE


/* 녹화 시작/종료 시, 효과음이 해당 동영상 파일에 녹화되는 문제를 임시 수정한다.
 *
 * 녹화 시작 시에는 효과음 재생 완료 시까지 POLLING 후 녹화를 시작하고,
 * 완료 시에는 녹화 완료 이후 800ms 고정 DELAY 적용 후 효과음을 재생한다.
 *
 * R4070 까지는 녹화 시작 시 오디오 클럭이 초기화되는 시간이 오래 걸려
 * 다수의 프레임이 DROP 되면서 녹화 시작음이 녹음 될 가능성이 희박했으나,
 * R4075 에서 이 이슈가 수정되면서 녹화 시작 시 효과음이 100% 확률로 녹음되고, 
 * 종료 시 80% 이상 확률로 녹음된다.
 *
 * F_SKYCAM_TODO, QUALCOMM 수정 확인 후 삭제해야 한다. (SR#307114) */
#define F_SKYCAM_QBUG_REC_BEEP_IS_RECORDED


/* 1600x1200, 1600x960 해상도에서 "zoom" 파라미터를 21 로 설정 후 스냅샷 시
 * 썸네일 YUV 이미지를 CROP 하는 과정에서 발생하는 BUFFER OVERRUN 문제를 
 * 임시 수정한다. 
 *
 * QualcommCameraHardware::receiveRawPicture() 에서 crop_yuv420(thumbnail) 
 * 호출 시 파라미터는 width=512, height=384, cropped_width=504, 
 * cropped_height=380 이며 memcpy 도중에 SOURCE 주소보다 DESTINATION 주소가 
 * 더 커지는 상황이 발생한다.
 *
 * F_SKYCAM_TODO, QUALCOMM 수정 확인 후 삭제해야 한다. (SR#308616) */
#define F_SKYCAM_QBUG_ZOOM_CAUSE_BUFFER_OVERRUN


/* 모든 해상도에서 ZOOM 을 특정 레벨 이상으로 설정할 경우, 
 * EXIFTAGID_EXIF_PIXEL_X_DIMENSION, EXIFTAGID_EXIF_PIXEL_Y_DIMENSION 태그
 * 정보에 잘못된 값이 저장되는 문제를 임시 수정한다.
 *
 * F_SKYCAM_TODO, QUALCOMM 수정 확인 후 삭제해야 한다. (SR#307343) */
#define F_SKYCAM_QBUG_EXIF_IMAGE_WIDTH_HEIGHT


/* 동영상 녹화 파일 재생 시작 시, 오디오가 약 300ms 빠르게 재생되고 시간이 
 * 경과할수록 오디오가 수 초 이상 더 빨라진다.
 *
 * OPENCORE 기반의 기본 VIDEO 응용으로 재생 시에는 초반에 SYNC 가 맞는 상태로 
 * 재생 시작 되지만, 시간이 경과할수록 오디오가 더 빨라진다. 
 * 곰플레이어/퀵타임/다음팟플레이어로 재생 시에는 오디오가 빠른 상태에서 재생 
 * 시작되고 시간이 경과할수록 오디오가 더 빨라진다.
 *
 * 비디오의 경우, 매 프레임 수신 시점에서 시스템 시간을 얻어와 TIMESTAMP 를 붙여
 * 인코더로 전송 하지만, 오디오의 경우 인코드 완료된 오디오 스트림을 수신할 
 * 때마다에 인코드 BITRATE 에 기반하여 계산된 TIMESTAMP 를 붙이고, 인코드 완료된
 * 비디오 스트림과 함께 파일로 저장된다.
 *
 * 오디오 인코더의 BITRATE 에 기반하여 계산된 TIMESTAMP 와 시스템 시간을 비교한
 * 결과, 녹화 시작 후 시간이 경과할수록 시스템 시간과 오디오 TIMESTAMP 의 차이가
 * 증가하며, 이를 수정하기 위해 오디오 TIMESTAMP 도 시스템 시간을 이용하도록 
 * 수정한다.
 *
 * 녹화 시작 1 ~ 2초 후 부터는 약 800ms 단위로 인코드 된 오디오 스트림이 
 * AUTHOR 로 전달되며, 만약 각 오디오 프레임의 TIMESTAMP 값의 차이가 이 보다
 * 작게 설정될 경우, 곰플레이어/다음팟플레이어에서 재생 시 오디오가 MUTE 되는 
 * 상황이 발생한다. 그러므로 반드시 각 오디오 프레임의 TIMESTAMP 는 BITRATE 를 
 * 기반하여 계산된 TIMESTAMP 보다는 커야한다.
 * 
 * F_SKYCAM_TODO, QUALCOMM 수정 확인 후 삭제해야 한다. (SR#308616) */
#define F_SKYCAM_QBUG_REC_AV_SYNC


/* 동영상 녹화 시작/종료를 빠르게 반복하거나, 이어잭을 장착한 상태에서 연속촬영
 * 모드로 촬영할 경우, MediaPlayer 가 오동작하면서 HALT 발생한다.
 *
 * MediaPlayer 의 경우, 동일한 음원을 재생 중에 또 다시 재생 시도할 경우 100%
 * 오동작하므로 동일 음원을 연속하여 재생해야 할 경우, 반드시 이전 재생이 완료
 * 되었는지 여부를 확인 후 재생해야 한다. */
#define F_SKYCAM_QBUG_SKIP_CAMERA_SOUND


/* 양산품 중 1% 이상에서 부팅 이후 카메라 2회째 진입 시 10초 이상 경과 후 
 * 프리뷰가 시작되거나 수 십초 후 RESET 발생하여 SW WORKAROUND 를 적용한다.
 *
 * 불량 증상으로는 수 회 정상 진입 후 오류가 발생하는 경우도 있고, 부팅 후 최초
 * 카메라 진입과 동시에 RESET 발생하는 경우도 있다.
 *
 * 대부분 불량품들의 경우, 2회째 카메라 진입 시 CAMIF PAD RESET 과 동시에
 * VFE RESET ACK 인터럽트가 BURST 하게 발생하므로, VFE 가 정상적으로 RESET 되기
 * 전까지 발생하는 모든 인터럽트들은 무시하도록 수정한다.
 *
 * 현재 진행 중인 RMA (RMA5009140, RMA5009149) 결과가 확인되어야만 QSD 하드웨어 
 * 자체 결함인지, SW 버그인지 여부를 확인할 수 있다.
 *
 * TP2차 (1/89ea) 발생 당시 SR#293365 참고
 * F_SKYCAM_TODO, QUALCOMM 수정 확인 후 삭제해야 한다. (SR#312913) */
#define F_SKYCAM_QBUG_VFE_BURST_INTERRUPTS


/*----------------------------------------------------------------------------*/
/*  MODEL-SPECIFIC CONSTANTS                                                  */
/*  모델 관련 상수 선언                                                       */
/*----------------------------------------------------------------------------*/


#if defined(F_SKYCAM_MODEL_EF12S) && defined(F_SKYCAM_ADD_CFG_FACE_FILTER)
#define F_SKYCAM_ADD_CFG_FACE_FILTER_RECT
#endif


/* 카메라 동작 시간을 저장하기 위한 데이터 파일명이다. */
#ifdef F_SKYCAM_FACTORY_PROC_CMD
#define C_SKYCAM_FNAME_FACTORY_PROC_CMD	"/data/.factorycamera.dat"
#endif


/* 설정 가능한 최소/최대 포커스 레벨이다. */
#ifdef F_SKYCAM_FIX_CFG_FOCUS_STEP
#define C_SKYCAM_MIN_FOCUS_STEP 0	/* 무한대 (default) */
#define C_SKYCAM_MAX_FOCUS_STEP 9	/* 매크로 */
#endif


/* 설정 가능한 최소/최대 밝기 단계이다. */
#ifdef F_SKYCAM_FIX_CFG_BRIGHTNESS
#define C_SKYCAM_MIN_BRIGHTNESS 0	/* -4 */
#define C_SKYCAM_MAX_BRIGHTNESS 8	/* +4 */
#endif


/* 설정 가능한 최소/최대 ZOOM 단계이다. 응용에서의 사용 편의성을 위해 최소/최대
 * 단계를 고정할 경우 사용한다.
 * EF10S 에서는 사용하지 않으며 참고용으로 코드들은 남겨둔다. */
#ifdef F_SKYCAM_FIX_CFG_ZOOM
#define C_SKYCAM_MIN_ZOOM 0
#define C_SKYCAM_MAX_ZOOM 8
#endif


/* 카메라 응용에서 촬영 시 삽입할 EXIF 태그 정보의 제조사 관련 정보를 
 * 수정한다. */
#ifdef F_SKYCAM_OEM_EXIF_TAG
#define C_SKYCAM_EXIF_MAKE		"KDDI-PT"
#define C_SKYCAM_EXIF_MAKE_LEN		8		/* including NULL */
#endif


/* 센서가 프리뷰 모드일 경우, 출력 가능한 FPS 의 최소/최대 값이다. */
#ifdef F_SKYCAM_FIX_CFG_PREVIEW_FPS
#define C_SKYCAM_MIN_PREVIEW_FPS	5
#define C_SKYCAM_MAX_PREVIEW_FPS	30
#endif


#endif /* CUST_SKYCAM.h */
