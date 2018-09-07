/*
 *
 * BRIEF MODULE DESCRIPTION
 *    PROM library initialisation code, supports YAMON and U-Boot.
 *
 * Copyright 2000, 2001, 2006 MontaVista Software Inc.
 * Author: MontaVista Software, Inc.
 *         	ppopov@mvista.com or source@mvista.com
 *
 * This file was derived from Carsten Langgaard's
 * arch/mips/mips-boards/xx files.
 *
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>

#include <asm/bootinfo.h>
#include <asm/jzsoc.h>

/* #define DEBUG_CMDLINE */

int prom_argc;
char **prom_argv, **prom_envp;

char * prom_getcmdline(void)
{
	return &(arcs_cmdline[0]);
}

void  prom_init_cmdline(void)
{
	char *cp;
	int actr;

	actr = 1; /* Always ignore argv[0] */

	cp = &(arcs_cmdline[0]);
	while(actr < prom_argc) {
	        strcpy(cp, prom_argv[actr]);
		cp += strlen(prom_argv[actr]);
		*cp++ = ' ';
		actr++;
	}
	if (cp != &(arcs_cmdline[0])) /* get rid of trailing space */
		--cp;
	if (prom_argc > 1)
		*cp = '\0';

}


char *prom_getenv(char *envname)
{
#if 0
	/*
	 * Return a pointer to the given environment variable.
	 * YAMON uses "name", "value" pairs, while U-Boot uses "name=value".
	 */

	char **env = prom_envp;
	int i = strlen(envname);
	int yamon = (*env && strchr(*env, '=') == NULL);

	while (*env) {
		if (yamon) {
			if (strcmp(envname, *env++) == 0)
				return *env;
		} else {
			if (strncmp(envname, *env, i) == 0 && (*env)[i] == '=')
				return *env + i + 1;
		}
		env++;
	}
#endif
	return NULL;
}

void serial_put_hex(void)
{
	register unsigned char c __asm__("$8");  // t0
	register int i __asm__("$9");   // t1
	register volatile unsigned int *reg_addrs __asm__("$10"); // t2 
	register volatile unsigned int data __asm__("$11");
	register volatile u8 *uart_lsr __asm__("$12") = (volatile u8 *)(UART3_BASE + OFF_LSR);
	register volatile u8 *uart_tdr __asm__("$13") = (volatile u8 *)(UART3_BASE + OFF_TDR);
	register volatile unsigned int addr __asm__("$14");
	register volatile int r __asm__("$15");

	reg_addrs = (volatile unsigned int *)0x12345678;

	for (r = 0; r < 15; r++) {
		addr = reg_addrs[r];
		data = REG32(addr);
		for(i = 0; i < 8; i++) 
		{       
			c = (data >> ((7 - i) * 4)) & 0xf;
			if(c < 10)
				c += 0x30;
			else    
				c += (0x41 - 10);


			while ( !((*uart_lsr & (UART_LSR_TDRQ | UART_LSR_TEMT)) == 0x60) );
			*uart_tdr = c;
		}       

		while ( !((*uart_lsr & (UART_LSR_TDRQ | UART_LSR_TEMT)) == 0x60) );
		*uart_tdr = '\r'; 
		while ( !((*uart_lsr & (UART_LSR_TDRQ | UART_LSR_TEMT)) == 0x60) );
		*uart_tdr = '\n';
	}
}

void __init prom_free_prom_memory(void)
{
}

void __init prom_init(void)
{
	unsigned char *memsize_str;
	unsigned long memsize;

	prom_argc = (int) fw_arg0;
	prom_argv = (char **) fw_arg1;
	prom_envp = (char **) fw_arg2;

	mips_machtype = MACH_INGENIC_JZ4775;

	prom_init_cmdline();
	memsize_str = prom_getenv("memsize");
	if (!memsize_str) {
		memsize = 0x04000000;
	} else {
		memsize = simple_strtol(memsize_str, NULL, 0);
	}
	add_memory_region(0, memsize, BOOT_MEM_RAM);
}


/* used by early printk */
void prom_putchar(char c)
{
	volatile u8 *uart_lsr = (volatile u8 *)(JZ_EARLY_UART_BASE + OFF_LSR);
	volatile u8 *uart_tdr = (volatile u8 *)(JZ_EARLY_UART_BASE + OFF_TDR);

	/* Wait for fifo to shift out some bytes */
	while ( !((*uart_lsr & (UARTLSR_TDRQ | UARTLSR_TEMT)) == 0x60) );

	*uart_tdr = (u8)c;
}

const char *get_system_type(void)
{
	return "JZ4775";
}

EXPORT_SYMBOL(prom_getcmdline);
