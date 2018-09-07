#include <asm/jzsoc.h> 

//#define GPIO_CIM_VCC_EN		(32*1+27)	/* GPB27, for Jz4780 T760 Board */

void cim_power_off(void)
{
  	cpm_stop_clock(CGM_CIM);
}

void cim_power_on(void)
{
	cpm_stop_clock(CGM_CIM);
	//cpm_set_clock(CGU_CIMCLK, 24000000);	//use 24MHz oscillator
	cpm_start_clock(CGM_CIM);
}




