#include <asm/jzsoc.h> 

static void cim_power_off(void)
{
  	cpm_stop_clock(CGM_CIM1);
}

static void cim_power_on(void)
{
       cpm_stop_clock(CGM_CIM1);
       cpm_set_clock(CGU_CIM1CLK,24000000);
       cpm_start_clock(CGM_CIM1);
}




