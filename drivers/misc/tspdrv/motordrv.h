// shpark 091229 add
// -----------------

#ifndef _MOTOR_DRV_H_
#define _MOTOR_DRV_H_

#if (BOARD_VER == EF12_WS10) 
#define GPIO_MOTOR_EN		109 
#endif

#define GP_M_VAL_MIN		1
#define GP_N_VAL_MIN		1
#define GP_M_VAL_MAX		511
#define GP_N_VAL_MAX		8191
#define GP_PREV_DIV			1
#define GP_CLK_SRC			19200000	// (Hz) src TCXO
#define GP_CLK_DEST(_clk)	(_clk*128)	// (Hz) ouput frequency
#define GP_CLK_DEST_DEFAULT	174

#define HWIO_GP_MD_REG_ADDR	*(volatile unsigned long *)(MSM_CLK_CTL_BASE+0x54)
#define HWIO_GP_NS_REG_ADDR	*(volatile unsigned long *)(MSM_CLK_CTL_BASE+0x58) 

#define PWM_COEF		90	// shpark EF10?ì„œ??90%ë¡??¤ì • (PP?Œì˜ ê²°ê³¼) 100410

#define abssub(a,b)		(a-b>0) ? (a-b) : (b-a)
#if (PWM_COEF == 100)
#define pwmmul(m,n)		(((((n-(2*m))*99)/100) * PWM_COEF ) / 100)
#else
#define pwmmul(m,n)		(((n-(2*m)) * PWM_COEF ) / 100)
#endif

// ioctl command
#define MOTOR_IOCTL_CMD_INIT		1
#define MOTOR_IOCTL_CMD_DISABLE		2
#define MOTOR_IOCTL_CMD_ENABLE		3
#define MOTOR_IOCTL_CMD_SET_PWM		4
#define MOTOR_IOCTL_CMD_GEN_PWM		5
#define MOTOR_IOCTL_CMD_READ_MDR	6
#define MOTOR_IOCTL_CMD_READ_NSR	7
#define MOTOR_IOCTL_CMD_SET_RATE	8

// motor rate
#define MOTOR_DEFAULT_INTENSITY		100
#define MOTOR_CALLSTATE_INTENSITY	32

typedef struct
{
	uint32_t rate;
	uint32_t valM;
	uint32_t valN;
	uint32_t valD;
	uint32_t multiplier;
	uint32_t mdr;
	uint32_t nsr;
} _pwm_type;

void motor_disable(void);
void motor_enable(void);
int  motor_init(void);
void motor_set_pwm(signed char nForce);
void motor_gen_pwm(uint32_t destClk);
void motor_set_rate(uint32_t rate);
uint32_t motor_get_rate(void);

#ifdef APDS9801_POWER_PROCESS_ENABLE
void apds9801_powerOn();
void apds9801_powerOff();
#endif

#endif // _MOTOR_H_
