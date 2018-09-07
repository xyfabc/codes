#include <asm/jzsoc.h> 

void cim_power_off(void)
{
#ifdef CONFIG_JZ_CIM_0
  	cpm_stop_clock(CGM_CIM0);
#endif
#ifdef CONFIG_JZ_CIM_1
  	cpm_stop_clock(CGM_CIM1);
#endif
}

void cim_power_on(void)
{
#ifdef CONFIG_JZ_CIM_0
       cpm_stop_clock(CGM_CIM0);
       cpm_set_clock(CGU_CIMCLK,24000000);
       cpm_start_clock(CGM_CIM0);
#endif
#ifdef CONFIG_JZ_CIM_1
       cpm_stop_clock(CGM_CIM1);
       cpm_set_clock(CGU_CIM1CLK,24000000);
       cpm_start_clock(CGM_CIM1);
#endif
}




