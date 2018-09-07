/*
 * linux/arch/mips/jz4780/cpm.c
 *
 * jz4780 on-chip modules.
 *
 * Copyright (C) 2010 Ingenic Semiconductor Co., Ltd.
 *
 * Author: <whxu@ingenic.cn>
 */

#include <asm/jzsoc.h>
#include <linux/module.h>

/*
 * Get the xPLL clock
 */
unsigned int cpm_get_xpllout(src_clk sclk_name)
{
        unsigned int exclk = __cpm_get_extalclk();
        unsigned int xpll_ctrl = 0, pllout;
        unsigned int m, n, od;

	switch (sclk_name) {
	case SCLK_APLL:
		xpll_ctrl = INREG32(CPM_CPAPCR);	
		break;
	case SCLK_MPLL:
		xpll_ctrl = INREG32(CPM_CPMPCR);	
		break;
	case SCLK_EPLL:
		xpll_ctrl = INREG32(CPM_CPEPCR);	
		break;
	case SCLK_VPLL:
		xpll_ctrl = INREG32(CPM_CPVPCR);	
		break;
	default:
		printk("WARNING: can NOT get the %dpll's clock\n", sclk_name);
		break;
	}

	if (xpll_ctrl & CPXPCR_XPLLBP) {
		pllout = exclk;
		return pllout;
	}

	if (!(xpll_ctrl & CPXPCR_XPLLEN)) {
		pllout = 0;
		return 0;
	}


	m = ((xpll_ctrl & CPXPCR_XPLLM_MASK) >> CPXPCR_XPLLM_LSB) + 1;
	n = ((xpll_ctrl & CPXPCR_XPLLN_MASK) >> CPXPCR_XPLLN_LSB) + 1;
	od = ((xpll_ctrl & CPXPCR_XPLLOD_MASK) >> CPXPCR_XPLLOD_LSB) + 1;

	pllout = exclk * m / (n * od);

	return pllout;
}
EXPORT_SYMBOL(cpm_get_xpllout);

/*
 * Start the module clock
 */
void cpm_start_clock(clock_gate_module module_name)
{
	unsigned int cgr_index, module_index;

	if (module_name == CGM_ALL_MODULE) {
		OUTREG32(CPM_CLKGR0, 0x0);
		OUTREG32(CPM_CLKGR1, 0x0);
		return;
	}

	cgr_index = module_name / 32;
	module_index = module_name % 32;
	switch (cgr_index) {
		case 0:
			CLRREG32(CPM_CLKGR0, 1 << module_index);
			switch(module_index) {
				case 3:
				case 11:
				case 12:
					//SETREG32(CPM_MSC0CDR, MSCCDR_MPCS_APLL);
					SETREG32(CPM_MSC0CDR, MSCCDR_MPCS_MPLL);
					break;
				default:
					;
			}
			break;
		case 1:
			CLRREG32(CPM_CLKGR1, 1 << module_index);
			break;
		default:
			printk("WARNING: can NOT start the %d's clock\n",
					module_name);
			break;
	}
}
EXPORT_SYMBOL(cpm_start_clock);

/*
 * Stop the module clock
 */
void cpm_stop_clock(clock_gate_module module_name)
{
	unsigned int cgr_index, module_index;

	if (module_name == CGM_ALL_MODULE) {
		OUTREG32(CPM_CLKGR0, 0xffffffff);
		OUTREG32(CPM_CLKGR1, 0xffff);
		return;
	}

	cgr_index = module_name / 32;
	module_index = module_name % 32;
	switch (cgr_index) {
		case 0:
			SETREG32(CPM_CLKGR0, 1 << module_index);
			break;

		case 1:
			SETREG32(CPM_CLKGR1, 1 << module_index);
			break;

		default:
			printk("WARNING: can NOT stop the %d's clock\n",
					module_name);
			break;
	}
}
EXPORT_SYMBOL(cpm_stop_clock);

/*
 * Get the clock, assigned by the clock_name, and the return value unit is Hz
 */
unsigned int cpm_get_clock(cgu_clock clock_name)
{
	unsigned int exclk = __cpm_get_extalclk();
	//unsigned int apllclk = cpm_get_pllout();
	unsigned int apllclk = cpm_get_pllout1();
	unsigned int mpllclk = cpm_get_pllout1();
	unsigned int epllclk = cpm_get_xpllout(SCLK_EPLL);
	unsigned int vpllclk = cpm_get_xpllout(SCLK_VPLL);
	unsigned int clock_ctrl = INREG32(CPM_CPCCR);
	unsigned int clock_hz = 0;
	unsigned int val, tmp, div,clksrc;

	switch (clock_name) {
		case CGU_CCLK:
			val = get_bf_value(clock_ctrl, CPCCR_CDIV_LSB, CPCCR_CDIV_MASK);
			div = val + 1;
			clock_hz = apllclk / div;

			break;

		case CGU_L2CLK:
			val = get_bf_value(clock_ctrl, CPCCR_L2DIV_LSB, CPCCR_L2DIV_MASK);
			div = val + 1;
			clock_hz = apllclk / div;

			break;

		case CGU_HCLK:
			val = get_bf_value(clock_ctrl, CPCCR_H0DIV_LSB, CPCCR_H0DIV_MASK);
			div = val + 1;
			clock_hz = apllclk / div;

			break;

		case CGU_H2CLK:
			val = get_bf_value(clock_ctrl, CPCCR_H2DIV_LSB, CPCCR_H2DIV_MASK);
			div = val + 1;
			clock_hz = apllclk / div;

			break;

		case CGU_PCLK:
			val = get_bf_value(clock_ctrl, CPCCR_PDIV_LSB, CPCCR_PDIV_MASK);
			div = val + 1;
			clock_hz = apllclk / div;

			break;

		case CGU_MCLK:
			tmp = INREG32(CPM_DDCDR);
			val = get_bf_value(tmp, DDCDR_DDRDIV_LSB, DDCDR_DDRDIV_MASK);
			div = val + 1;
			clock_hz = mpllclk / div;

			break;

		case CGU_MSC0CLK:
			tmp = INREG32(CPM_MSC0CDR);
			clksrc = INREG32(CPM_MSC0CDR) & MSCCDR_MPCS_MASK;
			val = get_bf_value(tmp, MSCCDR_MSCDIV_LSB, MSCCDR_MSCDIV_MASK);
			div = val + 1;
			if (tmp & MSCCDR_MSC_STOP) {
				clock_hz = 0;
			} else {
				if (clksrc == MSCCDR_MPCS_APLL) 
					clock_hz = apllclk / div;
				else if (clksrc == MSCCDR_MPCS_MPLL)
					clock_hz = mpllclk / div;
				else 
					clock_hz = 0;
			}

			break;

		case CGU_MSC1CLK:
			tmp = INREG32(CPM_MSC1CDR);
			clksrc = INREG32(CPM_MSC0CDR) & MSCCDR_MPCS_MASK;
			val = get_bf_value(tmp, MSCCDR_MSCDIV_LSB, MSCCDR_MSCDIV_MASK);
			div = val + 1;
			if (tmp & MSCCDR_MSC_STOP) {
				clock_hz = 0;
			} else {
				if (clksrc == MSCCDR_MPCS_APLL) 
					clock_hz = apllclk / div;
				else if (clksrc == MSCCDR_MPCS_MPLL)
					clock_hz = mpllclk / div;
				else 
					clock_hz = 0;
			}

			break;

		case CGU_MSC2CLK:
			tmp = INREG32(CPM_MSC2CDR);
			clksrc = INREG32(CPM_MSC0CDR) & MSCCDR_MPCS_MASK;
			val = get_bf_value(tmp, MSCCDR_MSCDIV_LSB, MSCCDR_MSCDIV_MASK);
			div = val + 1;
			if (tmp & MSCCDR_MSC_STOP) {
				clock_hz = 0;
			} else {
				if (clksrc == MSCCDR_MPCS_APLL) 
					clock_hz = apllclk / div;
				else if (clksrc == MSCCDR_MPCS_MPLL)
					clock_hz = mpllclk / div;
				else 
					clock_hz = 0;
			}

			break;

		case CGU_CIMCLK:
			tmp = INREG32(CPM_CIMCDR);
			val = get_bf_value(tmp, CIMCDR_CIMDIV_LSB, CIMCDR_CIMDIV_MASK);
			div = val + 1;
			if (tmp & CIMCDR_CIM_STOP) {
				clock_hz = 0;
			} else {
				if (tmp & CIMCDR_CIMPCS_MPLL)
					clock_hz = mpllclk / div;
				else
					clock_hz = apllclk / div;
			}

			break;

		case CGU_SSICLK:
			tmp = INREG32(CPM_SSICDR);
			val = get_bf_value(tmp, SSICDR_SSIDIV_LSB, SSICDR_SSIDIV_MASK);
			div = val + 1;
			if (tmp & SSICDR_SSI_STOP) {
				clock_hz = 0;
			} else {
				if (tmp & SSICDR_SCS) { 
					if (tmp & SSICDR_SPCS) 
						clock_hz = mpllclk / div;
					else 
						clock_hz = apllclk / div;
				} else {
					clock_hz = exclk;
				}
			}

			break;

		case CGU_OTGCLK:
			tmp = INREG32(CPM_USBCDR);
			val = get_bf_value(tmp, USBCDR_OTGDIV_LSB, USBCDR_OTGDIV_MASK);
			div = val + 1;
			if (tmp & USBCDR_USB_STOP) {
				clock_hz = 0;
			} else {
				if (tmp & USBCDR_UCS) {
					if (tmp & USBCDR_UPCS)
						clock_hz = mpllclk / div;
					else
						clock_hz = apllclk / div;
				} else {
					clock_hz = exclk;
				}
			}

			break;

		case CGU_LPCLK0:
			tmp = INREG32(CPM_LPCDR0);
			clksrc = tmp & LPCDR_LPCS_MASK;
			val = get_bf_value(tmp, LPCDR_PIXDIV_LSB, LPCDR_PIXDIV_MASK);
			div = val + 1;
			if (tmp & LPCDR_LCD_STOP) {
				clock_hz = 0;
			} else {
				if (clksrc == LPCDR_LPCS_APLL)
					clock_hz = apllclk / div;
				else if (clksrc == LPCDR_LPCS_MPLL)
					clock_hz = mpllclk / div;
				else if (clksrc == LPCDR_LPCS_VPLL)
					clock_hz = vpllclk / div;
				else
					clock_hz = 0;
			}

			break;

		case CGU_LPCLK1:
			tmp = INREG32(CPM_LPCDR1);
			clksrc = tmp & LPCDR_LPCS_MASK;
			val = get_bf_value(tmp, LPCDR_PIXDIV_LSB, LPCDR_PIXDIV_MASK);
			div = val + 1;
			if (tmp & LPCDR_LCD_STOP) {
				clock_hz = 0;
			} else {
				if (clksrc == LPCDR_LPCS_APLL)
					clock_hz = apllclk / div;
				else if (clksrc == LPCDR_LPCS_MPLL)
					clock_hz = mpllclk / div;
				else if (clksrc == LPCDR_LPCS_VPLL)
					clock_hz = vpllclk / div;
				else
					clock_hz = 0;
			}

			break;

		case CGU_HDMICLK:
			tmp = INREG32(CPM_HDMICDR);
			clksrc = tmp & HDMICDR_HPCS_MASK;
			val = get_bf_value(tmp, HDMICDR_HDMIDIV_LSB, HDMICDR_HDMIDIV_MASK);
			div = val + 1;
			if (tmp & HDMICDR_HDMI_STOP) {
				clock_hz = 0;
			} else {
				if (clksrc == HDMICDR_HPCS_APLL)
					clock_hz = apllclk / div;
				else if (clksrc == HDMICDR_HPCS_MPLL)
					clock_hz = mpllclk / div;
				else if (clksrc == HDMICDR_HPCS_VPLL)
					clock_hz = vpllclk / div;
				else
					clock_hz = 0;
			}

			break;

		case CGU_VPUCLK:
			tmp = INREG32(CPM_VPUCDR);
			clksrc = tmp & VPUCDR_VCS_MASK;
			val = get_bf_value(tmp, VPUCDR_VPUDIV_LSB, VPUCDR_VPUDIV_MASK);
			div = val + 1;
			if (tmp & VPUCDR_VPU_STOP) {
				clock_hz = 0;
			} else {
				if (clksrc == VPUCDR_VPU_STOP)  
					clock_hz = 0;
				else if (clksrc == VPUCDR_VCS_APLL)
					clock_hz = apllclk / div;
				else if (clksrc == VPUCDR_VCS_MPLL)
					clock_hz = mpllclk / div;
				else if (clksrc == VPUCDR_VCS_EPLL)
					clock_hz = epllclk / div;
			}

			break;

		case CGU_GPUCLK:
			tmp = INREG32(CPM_GPUCDR);
			clksrc = tmp & GPUCDR_GPCS_MASK;
			val = get_bf_value(tmp, GPUCDR_GPUDIV_LSB, GPUCDR_GPUDIV_MASK);
			div = val + 1;
			if (tmp & GPUCDR_GPU_STOP) {
				clock_hz = 0;
			} else {
				if (clksrc == GPUCDR_GPCS_STOP)  
					clock_hz = 0;
				else if (clksrc == GPUCDR_GPCS_APLL)
					clock_hz = apllclk / div;
				else if (clksrc == GPUCDR_GPCS_MPLL)
					clock_hz = mpllclk / div;
				else if (clksrc == GPUCDR_GPCS_EPLL)
					clock_hz = epllclk / div;
			}

			break;

		case CGU_I2SCLK:
			tmp = INREG32(CPM_I2SCDR);
			val = get_bf_value(tmp, I2SCDR_I2SDIV_LSB, I2SCDR_I2SDIV_MASK);
			div = val + 1;
			if (tmp & I2SCDR_I2S_STOP) {
				clock_hz = 0;
			} else {
				if (tmp & I2SCDR_I2CS) {
					if (tmp & I2SCDR_I2PCS)
						clock_hz = epllclk / div;
					else
						clock_hz = apllclk / div;
				} else {
					clock_hz = exclk;
				}
			}

			break;

		case CGU_PCMCLK:
			tmp = INREG32(CPM_PCMCDR);
			val = get_bf_value(tmp, PCMCDR_PCMDIV_LSB, PCMCDR_PCMDIV_MASK);
			div = val + 1;
			if (tmp & PCMCDR_PCM_STOP) {
				clock_hz = 0;
			} else {
				if (tmp & PCMCDR_PCMS) {
					clksrc = tmp & PCMCDR_PCMPCS_MASK;
					if (tmp == PCMCDR_PCMPCS_APLL)
						clock_hz = apllclk / div;
					else if (tmp == PCMCDR_PCMPCS_MPLL)
						clock_hz = mpllclk / div;
					else if (tmp == PCMCDR_PCMPCS_EPLL)
						clock_hz = epllclk / div;
					else if (tmp == PCMCDR_PCMPCS_VPLL)
						clock_hz = vpllclk / div;
				} else {
					clock_hz = exclk;
				}
			}

			break;

		case CGU_UHCCLK:
			tmp = INREG32(CPM_UHCCDR);
			clksrc = tmp & UHCCDR_UHCS_MASK;
			val = get_bf_value(tmp, UHCCDR_UHCDIV_LSB, UHCCDR_UHCDIV_MASK);
			div = val + 1;
			if (tmp & UHCCDR_UHC_STOP) {
				clock_hz = 0;
			} else {
				if (clksrc == UHCCDR_UHCS_APLL)
					clock_hz = apllclk / div;
				else if (clksrc == UHCCDR_UHCS_MPLL)
					clock_hz = mpllclk / div;
				else if (clksrc == UHCCDR_UHCS_EPLL)
					clock_hz = epllclk / div;
				else if (clksrc == UHCCDR_UHCS_OTG)
					clock_hz = cpm_get_clock(CGU_OTGCLK);
			}

			break;

		case CGU_UARTCLK:
		case CGU_SADCCLK:
		case CGU_TCUCLK:
			clock_hz = exclk;

			break;

#if 0
		case CGU_GPSCLK:
			tmp = INREG32(CPM_GPSCDR);
			val = get_bf_value(tmp, GPSCDR_GPSDIV_LSB, GSPCDR_GPSDIV_MASK);
			div = val + 1;
			if (tmp & GPSCDR_GPCS)
				clock_hz = mpllclk / div;
			else
				clock_hz = apllclk / div;

			break;
#endif
		default:
			printk("WARNING: can NOT get clock %d!\n", clock_name);
			clock_hz = exclk;
			break;
	}

	return clock_hz;
}
EXPORT_SYMBOL(cpm_get_clock);

/*
 * Check div value whether valid, if invalid, return the max valid value
 */
static unsigned int __check_div(unsigned int div, unsigned int lsb, unsigned int mask)
{
	if ((div << lsb) > mask) {
		printk("WARNING: Invalid div %d larger than %d\n", div, mask >> lsb);
		return mask >> lsb;
	} else
		return div;
}

/*
 * Set the clock, assigned by the clock_name, and the return value unit is Hz,
 * which means the actual clock
 */
#define ceil(v,div) ({ unsigned int val = 0; if(v % div ) val = v /div + 1; else val = v /div; val;})
#define nearbyint(v,div) ({unsigned int val = 0; if((v % div) * 2 >= div)  val = v / div + 1;else val = v / div; val;})
unsigned int cpm_set_clock(cgu_clock clock_name, unsigned int clock_hz)
{
	unsigned int actual_clock = 0;
	unsigned int exclk = __cpm_get_extalclk();
	//unsigned int apllclk = cpm_get_pllout();
	unsigned int apllclk = cpm_get_pllout1();
	unsigned int mpllclk = cpm_get_pllout1();
	unsigned int epllclk = cpm_get_xpllout(SCLK_EPLL);
	unsigned int vpllclk = cpm_get_xpllout(SCLK_VPLL);
	unsigned int div;

	if (!clock_hz)
		return actual_clock;

	switch (clock_name) {
		case CGU_VPUCLK:
			div = ceil(apllclk , clock_hz) - 1;
			div = __check_div(div, VPUCDR_VPUDIV_LSB, VPUCDR_VPUDIV_MASK);
			/* Select apll clock as input */
			//OUTREG32(CPM_VPUCDR, VPUCDR_VCS_APLL | VPUCDR_CE_VPU | div);
			OUTREG32(CPM_VPUCDR, VPUCDR_VCS_MPLL | VPUCDR_CE_VPU | div);
			while (INREG32(CPM_VPUCDR) & VPUCDR_VPU_BUSY) ;
			break;

		case CGU_MSC0CLK:
			div = ceil(apllclk , clock_hz) - 1;
			div = __check_div(div, MSCCDR_MSCDIV_LSB, MSCCDR_MSCDIV_MASK);
			/* Select apll clock as input */
			//OUTREG32(CPM_MSC0CDR, MSCCDR_MPCS_APLL | MSCCDR_CE_MSC | div);
			OUTREG32(CPM_MSC0CDR, MSCCDR_MPCS_MPLL | MSCCDR_CE_MSC | div);
			while (INREG32(CPM_MSC0CDR) & MSCCDR_MSC_BUSY) ;

			break;

		case CGU_MSC1CLK:
			/* Select apll clock as input */
			//CMSREG32(CPM_MSC0CDR, MSCCDR_MPCS_APLL | MSCCDR_CE_MSC, 0xffffff00);
			CMSREG32(CPM_MSC0CDR, MSCCDR_MPCS_MPLL | MSCCDR_CE_MSC, 0xffffff00);
			while (INREG32(CPM_MSC0CDR) | MSCCDR_MSC_BUSY) ;
			div = ceil(apllclk , clock_hz) - 1;
			div = __check_div(div, MSCCDR_MSCDIV_LSB, MSCCDR_MSCDIV_MASK);
			OUTREG32(CPM_MSC1CDR, MSCCDR_CE_MSC | div);
			while (INREG32(CPM_MSC1CDR) & MSCCDR_MSC_BUSY) ;

			break;

		case CGU_MSC2CLK:
			/* Select apll clock as input */
			//CMSREG32(CPM_MSC0CDR, MSCCDR_MPCS_APLL | MSCCDR_CE_MSC, 0xffffff00);
			CMSREG32(CPM_MSC0CDR, MSCCDR_MPCS_MPLL | MSCCDR_CE_MSC, 0xffffff00);
			while (INREG32(CPM_MSC0CDR) | MSCCDR_MSC_BUSY) ;
			div = ceil(apllclk , clock_hz) - 1;
			div = __check_div(div, MSCCDR_MSCDIV_LSB, MSCCDR_MSCDIV_MASK);
			OUTREG32(CPM_MSC2CDR, MSCCDR_CE_MSC | div);
			while (INREG32(CPM_MSC2CDR) & MSCCDR_MSC_BUSY) ;

			break;

		case CGU_LPCLK0:
			div = nearbyint(mpllclk , clock_hz) - 1;
			div = __check_div(div, LPCDR_PIXDIV_LSB, LPCDR_PIXDIV_MASK);
			OUTREG32(CPM_LPCDR0, div | LPCDR_LPCS_MPLL | LPCDR_CE_LCD);
			while (INREG32(CPM_LPCDR0) & LPCDR_LCD_BUSY) ;
			break;

		case CGU_LPCLK1:
			div = nearbyint(mpllclk , clock_hz) - 1;
			div = __check_div(div, LPCDR_PIXDIV_LSB, LPCDR_PIXDIV_MASK);
			OUTREG32(CPM_LPCDR1, div | LPCDR_LPCS_MPLL | LPCDR_CE_LCD);
			while (INREG32(CPM_LPCDR1) & LPCDR_LCD_BUSY) ;
			break;

		case CGU_HDMICLK:
			div = nearbyint(mpllclk , clock_hz) - 1;
			div = __check_div(div, HDMICDR_HPCS_LSB, HDMICDR_HPCS_MASK);
			OUTREG32(CPM_HDMICDR, div | HDMICDR_HPCS_MPLL | HDMICDR_HPCS_CE);
			while (INREG32(CPM_HDMICDR) & HDMICDR_HPCS_BUSY) ;
			break;

		case CGU_I2SCLK:
			if (clock_hz == exclk) {
				OUTREG32(CPM_I2SCDR, I2SCDR_CE_I2S);
			} else {
				div = nearbyint(apllclk , clock_hz) - 1;
				div = __check_div(div, I2SCDR_I2SDIV_LSB, I2SCDR_I2SDIV_MASK);
				OUTREG32(CPM_I2SCDR, I2SCDR_I2CS | I2SCDR_CE_I2S | div);
			}
			while (INREG32(CPM_I2SCDR) & I2SCDR_I2S_BUSY) ;
			break;

		case CGU_PCMCLK:
			if (clock_hz == exclk) {
				/* Select external clock as input*/
				CLRREG32(CPM_PCMCDR, PCMCDR_PCMS | PCMCDR_CE_PCM);
			} else {
				div = ceil(apllclk , clock_hz) - 1;
				div = __check_div(div, PCMCDR_PCMDIV_LSB, PCMCDR_PCMDIV_MASK);
				OUTREG32(CPM_PCMCDR, PCMCDR_PCMS | PCMCDR_CE_PCM | div);
			}
			while (INREG32(CPM_PCMCDR) & PCMCDR_PCM_BUSY) ;

			break;

		case CGU_OTGCLK:
			if (clock_hz == exclk) {
				OUTREG32(CPM_USBCDR, USBCDR_CE_USB);
			} else {
				div = nearbyint(apllclk , clock_hz) - 1;
				div = __check_div(div, USBCDR_OTGDIV_LSB, USBCDR_OTGDIV_MASK);
				OUTREG32(CPM_USBCDR, USBCDR_UCS | USBCDR_CE_USB | div);
			}
			while (INREG32(CPM_USBCDR) & USBCDR_USB_BUSY) ;

			break;

		case CGU_RTCCLK:
			SETREG32(CPM_OPCR, OPCR_ERCS);
			break;

		case CGU_CIMCLK:
			/* Select apll clock as input */
			//CMSREG32(CPM_CIMCDR, CIMCDR_CIMPCS_APLL | CIMCDR_CE_CIM, 0xffffff00);
			CMSREG32(CPM_CIMCDR, CIMCDR_CIMPCS_MPLL | CIMCDR_CE_CIM, 0xffffff00);
			while (INREG32(CPM_CIMCDR) & CIMCDR_CIM_BUSY) ;

			div = ceil(apllclk , clock_hz) - 1;
			div = __check_div(div, CIMCDR_CIMDIV_LSB, CIMCDR_CIMDIV_MASK);
			OUTREG32(CPM_CIMCDR, CIMCDR_CIM_BUSY | div);
			while (INREG32(CPM_CIMCDR) & CIMCDR_CIM_BUSY) ;

			printk("==>%s L%d: CIMCDR=0x%08x\n", __func__, __LINE__, REG_CPM_CIMCDR);
			break;

		case CGU_UHCCLK:
			div = nearbyint(mpllclk , clock_hz) - 1;
			div = __check_div(div, UHCCDR_UHCDIV_LSB, UHCCDR_UHCDIV_MASK);
			OUTREG32(CPM_UHCCDR, div | UHCCDR_CE_UHC | UHCCDR_UHCS_MPLL);
			while (INREG32(CPM_UHCCDR) & UHCCDR_UHC_BUSY) ;

			break;

		default:
			printk("WARNING: can NOT set clock %d!\n", clock_name);
			break;
	}

	/* Get the actual clock */
	actual_clock = cpm_get_clock(clock_name);

	return actual_clock;
}
EXPORT_SYMBOL(cpm_set_clock);

/*
 * Control UHC phy, if en is NON-ZERO, enable the UHC phy, otherwise disable
 */
void cpm_uhc_phy(unsigned int en)
{
	if (en) {
		SETREG32(CPM_OPCR, OPCR_OTGPHY1_ENABLE);
	} else {
		CLRREG32(CPM_OPCR, OPCR_OTGPHY1_ENABLE);
	}
}

