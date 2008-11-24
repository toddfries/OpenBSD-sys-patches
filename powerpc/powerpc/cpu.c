/*-
 * Copyright (c) 2001 Matt Thomas.
 * Copyright (c) 2001 Tsubai Masanari.
 * Copyright (c) 1998, 1999, 2001 Internet Research Institute, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by
 *	Internet Research Institute, Inc.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Copyright (C) 2003 Benno Rice.
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
 * THIS SOFTWARE IS PROVIDED BY Benno Rice ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * from $NetBSD: cpu_subr.c,v 1.1 2003/02/03 17:10:09 matt Exp $
 * $FreeBSD: src/sys/powerpc/powerpc/cpu.c,v 1.14 2008/09/28 15:12:43 nwhitehorn Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>

#include <machine/bus.h>
#include <machine/hid.h>
#include <machine/md_var.h>
#include <machine/spr.h>

int powerpc_pow_enabled;

struct cputab {
	const char	*name;
	uint16_t	version;
	uint16_t	revfmt;
};
#define	REVFMT_MAJMIN	1	/* %u.%u */
#define	REVFMT_HEX	2	/* 0x%04x */
#define	REVFMT_DEC	3	/* %u */
static const struct cputab models[] = {
        { "Motorola PowerPC 601",	MPC601,		REVFMT_DEC },
        { "Motorola PowerPC 602",	MPC602,		REVFMT_DEC },
        { "Motorola PowerPC 603",	MPC603,		REVFMT_MAJMIN },
        { "Motorola PowerPC 603e",	MPC603e,	REVFMT_MAJMIN },
        { "Motorola PowerPC 603ev",	MPC603ev,	REVFMT_MAJMIN },
        { "Motorola PowerPC 604",	MPC604,		REVFMT_MAJMIN },
        { "Motorola PowerPC 604ev",	MPC604ev,	REVFMT_MAJMIN },
        { "Motorola PowerPC 620",	MPC620,		REVFMT_HEX },
        { "Motorola PowerPC 750",	MPC750,		REVFMT_MAJMIN },
        { "IBM PowerPC 750FX",		IBM750FX,	REVFMT_MAJMIN },
        { "Motorola PowerPC 7400",	MPC7400,	REVFMT_MAJMIN },
        { "Motorola PowerPC 7410",	MPC7410,	REVFMT_MAJMIN },
        { "Motorola PowerPC 7450",	MPC7450,	REVFMT_MAJMIN },
        { "Motorola PowerPC 7455",	MPC7455,	REVFMT_MAJMIN },
        { "Motorola PowerPC 7457",	MPC7457,	REVFMT_MAJMIN },
        { "Motorola PowerPC 7447A",	MPC7447A,	REVFMT_MAJMIN },
        { "Motorola PowerPC 7448",	MPC7448,	REVFMT_MAJMIN },
        { "Motorola PowerPC 8240",	MPC8240,	REVFMT_MAJMIN },
        { "Freescale e500v1 core",	FSL_E500v1,	REVFMT_MAJMIN },
        { "Freescale e500v2 core",	FSL_E500v2,	REVFMT_MAJMIN },
        { "Unknown PowerPC CPU",	0,		REVFMT_HEX }
};

static char model[64];
SYSCTL_STRING(_hw, HW_MODEL, model, CTLFLAG_RD, model, 0, "");

register_t	l2cr_config = 0;
register_t	l3cr_config = 0;

static void	cpu_print_speed(void);
static void	cpu_print_cacheinfo(u_int, uint16_t);

void
cpu_setup(u_int cpuid)
{
	u_int		pvr, maj, min, hid0;
	uint16_t	vers, rev, revfmt;
	const struct	cputab *cp;
	const char	*name;
	char		*bitmask;

	pvr = mfpvr();
	vers = pvr >> 16;
	rev = pvr;
	switch (vers) {
		case MPC7410:
			min = (pvr >> 0) & 0xff;
			maj = min <= 4 ? 1 : 2;
			break;
		case FSL_E500v1:
		case FSL_E500v2:
			maj = (pvr >>  4) & 0xf;
			min = (pvr >>  0) & 0xf;
			break;
		default:
			maj = (pvr >>  8) & 0xf;
			min = (pvr >>  0) & 0xf;
	}

	for (cp = models; cp->version != 0; cp++) {
		if (cp->version == vers)
			break;
	}

	revfmt = cp->revfmt;
	name = cp->name;
	if (rev == MPC750 && pvr == 15) {
		name = "Motorola MPC755";
		revfmt = REVFMT_HEX;
	}
	strncpy(model, name, sizeof(model) - 1);

	printf("cpu%d: %s revision ", cpuid, name);

	switch (revfmt) {
		case REVFMT_MAJMIN:
			printf("%u.%u", maj, min);
			break;
		case REVFMT_HEX:
			printf("0x%04x", rev);
			break;
		case REVFMT_DEC:
			printf("%u", rev);
			break;
	}

	hid0 = mfspr(SPR_HID0);

	/*
	 * Configure power-saving mode.
	 */
	switch (vers) {
		case MPC603:
		case MPC603e:
		case MPC603ev:
		case MPC604ev:
		case MPC750:
		case IBM750FX:
		case MPC7400:
		case MPC7410:
		case MPC8240:
		case MPC8245:
			/* Select DOZE mode. */
			hid0 &= ~(HID0_DOZE | HID0_NAP | HID0_SLEEP);
			hid0 |= HID0_DOZE | HID0_DPM;
			powerpc_pow_enabled = 1;
			break;

		case MPC7448:
		case MPC7447A:
		case MPC7457:
		case MPC7455:
		case MPC7450:
			/* Enable the 7450 branch caches */
			hid0 |= HID0_SGE | HID0_BTIC;
			hid0 |= HID0_LRSTK | HID0_FOLD | HID0_BHT;
			/* Disable BTIC on 7450 Rev 2.0 or earlier and on 7457 */
			if (((pvr >> 16) == MPC7450 && (pvr & 0xFFFF) <= 0x0200)
					|| (pvr >> 16) == MPC7457)
				hid0 &= ~HID0_BTIC;
			/* Select NAP mode. */
			hid0 &= ~(HID0_DOZE | HID0_NAP | HID0_SLEEP);
			hid0 |= HID0_NAP | HID0_DPM;
			powerpc_pow_enabled = 1;
			break;

		default:
			/* No power-saving mode is available. */ ;
	}

	switch (vers) {
		case IBM750FX:
		case MPC750:
			hid0 &= ~HID0_DBP;		/* XXX correct? */
			hid0 |= HID0_EMCP | HID0_BTIC | HID0_SGE | HID0_BHT;
			break;

		case MPC7400:
		case MPC7410:
			hid0 &= ~HID0_SPD;
			hid0 |= HID0_EMCP | HID0_BTIC | HID0_SGE | HID0_BHT;
			hid0 |= HID0_EIEC;
			break;

		case FSL_E500v1:
		case FSL_E500v2:
			hid0 |= HID0_EMCP;
			break;
	}

	mtspr(SPR_HID0, hid0);

	switch (vers) {
		case MPC7447A:
		case MPC7448:
		case MPC7450:
		case MPC7455:
		case MPC7457:
			bitmask = HID0_7450_BITMASK;
			break;
		case FSL_E500v1:
		case FSL_E500v2:
			bitmask = HID0_E500_BITMASK;
			break;
		default:
			bitmask = HID0_BITMASK;
			break;
	}

	switch (vers) {
		case MPC7450:
		case MPC7455:
		case MPC7457:
			/* Only MPC745x CPUs have an L3 cache. */

			l3cr_config = mfspr(SPR_L3CR);

			/* Fallthrough */
		case MPC750:
		case IBM750FX:
		case MPC7400:
		case MPC7410:
		case MPC7447A:
		case MPC7448:
			cpu_print_speed();
			printf("\n");

			l2cr_config = mfspr(SPR_L2CR);

			if (bootverbose)
				cpu_print_cacheinfo(cpuid, vers);
			break;
		default:
			printf("\n");
			break;
	}

	printf("cpu%d: HID0 %b\n", cpuid, hid0, bitmask);
}

void
cpu_print_speed(void)
{
	uint64_t	cps;

	mtspr(SPR_MMCR0, SPR_MMCR0_FC);
	mtspr(SPR_PMC1, 0);
	mtspr(SPR_MMCR0, SPR_MMCR0_PMC1SEL(PMCN_CYCLES));
	DELAY(100000);
	cps = (mfspr(SPR_PMC1) * 10) + 4999;
	printf(", %lld.%02lld MHz", cps / 1000000, (cps / 10000) % 100);
}

void
cpu_print_cacheinfo(u_int cpuid, uint16_t vers)
{
	uint32_t hid;


	hid = mfspr(SPR_HID0);
	printf("cpu%u: ", cpuid);
	printf("L1 I-cache %sabled, ", (hid & HID0_ICE) ? "en" : "dis");
	printf("L1 D-cache %sabled\n", (hid & HID0_DCE) ? "en" : "dis");

	printf("cpu%u: ", cpuid);
  	if (l2cr_config & L2CR_L2E) {
		switch (vers) {
		case MPC7450:
		case MPC7455:
		case MPC7457:
			printf("256KB L2 cache, ");
			if (l3cr_config & L3CR_L3E)
				printf("%cMB L3 backside cache",
				    l3cr_config & L3CR_L3SIZ ? '2' : '1');
			else
				printf("L3 cache disabled");
			printf("\n");
			break;
		case IBM750FX:
			printf("512KB L2 cache\n");
			break; 
		default:
			switch (l2cr_config & L2CR_L2SIZ) {
			case L2SIZ_256K:
				printf("256KB ");
				break;
			case L2SIZ_512K:
				printf("512KB ");
				break;
			case L2SIZ_1M:
				printf("1MB ");
				break;
			}
			printf("write-%s", (l2cr_config & L2CR_L2WT)
			    ? "through" : "back");
			if (l2cr_config & L2CR_L2PE)
				printf(", with parity");
			printf(" backside cache\n");
			break;
		}
	} else
		printf("L2 cache disabled\n");
}
