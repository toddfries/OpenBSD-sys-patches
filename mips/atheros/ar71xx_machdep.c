/*-
 * Copyright (c) 2009 Oleksandr Tymoshenko
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/mips/atheros/ar71xx_machdep.c,v 1.3 2010/01/25 00:44:05 gonzo Exp $");

#include <sys/param.h>
#include <machine/cpuregs.h>

#include <mips/sentry5/s5reg.h>

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cons.h>
#include <sys/kdb.h>
#include <sys/reboot.h>

#include <vm/vm.h>
#include <vm/vm_page.h>

#include <net/ethernet.h>

#include <machine/clock.h>
#include <machine/cpu.h>
#include <machine/hwfunc.h>
#include <machine/md_var.h>
#include <machine/trap.h>
#include <machine/vmparam.h>

#include <mips/atheros/ar71xxreg.h>

extern char edata[], end[];

uint32_t ar711_base_mac[ETHER_ADDR_LEN];
/* 4KB static data aread to keep a copy of the bootload env until
   the dynamic kenv is setup */
char boot1_env[4096];

/*
 * We get a string in from Redboot with the all the arguments together,
 * "foo=bar bar=baz". Split them up and save in kenv.
 */
static void
parse_argv(char *str)
{
	char *n, *v;

	while ((v = strsep(&str, " ")) != NULL) {
		if (*v == '\0')
			continue;
		if (*v == '-') {
			while (*v != '\0') {
				v++;
				switch (*v) {
				case 'a': boothowto |= RB_ASKNAME; break;
				case 'd': boothowto |= RB_KDB; break;
				case 'g': boothowto |= RB_GDB; break;
				case 's': boothowto |= RB_SINGLE; break;
				case 'v': boothowto |= RB_VERBOSE; break;
				}
			}
		} else {
			n = strsep(&v, "=");
			if (v == NULL)
				setenv(n, "1");
			else
				setenv(n, v);
		}
	}
}

void
platform_cpu_init()
{
	/* Nothing special */
}

void
platform_halt(void)
{

}

void
platform_identify(void)
{

}

void
platform_reset(void)
{
	uint32_t reg = ATH_READ_REG(AR71XX_RST_RESET);

	ATH_WRITE_REG(AR71XX_RST_RESET, reg | RST_RESET_FULL_CHIP);
	/* Wait for reset */
	while(1)
		;
}

void
platform_trap_enter(void)
{

}

void
platform_trap_exit(void)
{

}

void
platform_start(__register_t a0 __unused, __register_t a1 __unused, 
    __register_t a2 __unused, __register_t a3 __unused)
{
	uint64_t platform_counter_freq;
	uint32_t reg;
	int argc, i, count = 0;
	char **argv, **envp;
	vm_offset_t kernend;

	/* 
	 * clear the BSS and SBSS segments, this should be first call in
	 * the function
	 */
	kernend = (vm_offset_t)&end;
	memset(&edata, 0, kernend - (vm_offset_t)(&edata));

	mips_postboot_fixup();

	/* Initialize pcpu stuff */
	mips_pcpu0_init();

	argc = a0;
	argv = (char**)a1;
	envp = (char**)a2;
	/* 
	 * Protect ourselves from garbage in registers 
	 */
	if (MIPS_IS_VALID_PTR(envp)) {
		for (i = 0; envp[i]; i += 2)
		{
			if (strcmp(envp[i], "memsize") == 0)
				realmem = btoc(strtoul(envp[i+1], NULL, 16));
			else if (strcmp(envp[i], "ethaddr") == 0) {
				count = sscanf(envp[i+1], "%x.%x.%x.%x.%x.%x", 
				    &ar711_base_mac[0], &ar711_base_mac[1],
				    &ar711_base_mac[2], &ar711_base_mac[3],
				    &ar711_base_mac[4], &ar711_base_mac[5]);
				if (count < 6)
					memset(ar711_base_mac, 0,
					    sizeof(ar711_base_mac));
			}
		}
	}

	/*
	 * Just wild guess. RedBoot let us down and didn't reported 
	 * memory size
	 */
	if (realmem == 0)
		realmem = btoc(32*1024*1024);

	/* phys_avail regions are in bytes */
	phys_avail[0] = MIPS_KSEG0_TO_PHYS(kernel_kseg0_end);
	phys_avail[1] = ctob(realmem);

	physmem = realmem;

	/*
	 * ns8250 uart code uses DELAY so ticker should be inititalized 
	 * before cninit. And tick_init_params refers to hz, so * init_param1 
	 * should be called first.
	 */
	init_param1();
	platform_counter_freq = ar71xx_cpu_freq();
	mips_timer_init_params(platform_counter_freq, 1);
	cninit();
	init_static_kenv(boot1_env, sizeof(boot1_env));

	printf("platform frequency: %lld\n", platform_counter_freq);
	printf("arguments: \n");
	printf("  a0 = %08x\n", a0);
	printf("  a1 = %08x\n", a1);
	printf("  a2 = %08x\n", a2);
	printf("  a3 = %08x\n", a3);

	printf("Cmd line:");
	if (MIPS_IS_VALID_PTR(argv)) {
		for (i = 0; i < argc; i++) {
			printf(" %s", argv[i]);
			parse_argv(argv[i]);
		}
	}
	else
		printf ("argv is invalid");
	printf("\n");

	printf("Environment:\n");
	if (MIPS_IS_VALID_PTR(envp)) {
		for (i = 0; envp[i]; i+=2) {
			printf("  %s = %s\n", envp[i], envp[i+1]);
			setenv(envp[i], envp[i+1]);
		}
	}
	else 
		printf ("envp is invalid\n");

	init_param2(physmem);
	mips_cpu_init();
	pmap_bootstrap();
	mips_proc0_init();
	mutex_init();

	/*
	 * Reset USB devices 
	 */
	reg = ATH_READ_REG(AR71XX_RST_RESET);
	reg |= 
	    RST_RESET_USB_OHCI_DLL | RST_RESET_USB_HOST | RST_RESET_USB_PHY;
	ATH_WRITE_REG(AR71XX_RST_RESET, reg);
	DELAY(1000);
	reg &= 
	    ~(RST_RESET_USB_OHCI_DLL | RST_RESET_USB_HOST | RST_RESET_USB_PHY);
	ATH_WRITE_REG(AR71XX_RST_RESET, reg);
	
	ATH_WRITE_REG(AR71XX_USB_CTRL_CONFIG,
	    USB_CTRL_CONFIG_OHCI_DES_SWAP | USB_CTRL_CONFIG_OHCI_BUF_SWAP |
	    USB_CTRL_CONFIG_EHCI_DES_SWAP | USB_CTRL_CONFIG_EHCI_BUF_SWAP);

	ATH_WRITE_REG(AR71XX_USB_CTRL_FLADJ, 
	    (32 << USB_CTRL_FLADJ_HOST_SHIFT) | (3 << USB_CTRL_FLADJ_A5_SHIFT));
	DELAY(1000);

	kdb_init();
#ifdef KDB
	if (boothowto & RB_KDB)
		kdb_enter(KDB_WHY_BOOTFLAGS, "Boot flags requested debugger");
#endif
}
