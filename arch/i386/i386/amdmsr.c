/*      $OpenBSD: amdmsr.c,v 1.2 2008/06/15 01:18:22 deraadt Exp $	*/

/*
 * Copyright (c) 2008 Marc Balmer <mbalmer@openbsd.org>
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Enable MSR access AMD Processors
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/sysctl.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <machine/amdmsr.h>

#include <machine/bus.h>
#include <machine/cpufunc.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#ifdef APERTURE
static int amdmsr_open_cnt;
extern int allowaperture;
#endif

struct amdmsr_softc {
	struct device		sc_dev;
};

struct cfdriver amdmsr_cd = {
	NULL, "amdmsr", DV_DULL
};

int	amdmsr_match(struct device *, void *, void *);
void	amdmsr_attach(struct device *, struct device *, void *);

struct cfattach amdmsr_ca = {
	sizeof(struct amdmsr_softc), amdmsr_match, amdmsr_attach
};

int
amdmsr_match(struct device *parent, void *match, void *aux)
{
	int family, model, step;

	family = (cpu_id >> 8) & 0xf;
	model  = (cpu_id >> 4) & 0xf;
	step   = (cpu_id >> 0) & 0xf;

	if (strcmp(cpu_vendor, "AuthenticAMD") == 0 && family == 0x5 &&
	    (model > 0x8 || (model == 0x8 && step > 0x7)))
		return 1;

	return 0;
}

void
amdmsr_attach(struct device *parent, struct device *self, void *aux)
{
	printf("\n");
}

int
amdmsropen(dev_t dev, int flags, int devtype, struct proc *p)
{
	if (suser(p, 0) != 0)
		return EPERM;
#ifdef APERTURE
	if (!allowaperture)
		return EPERM;
#endif
	/* allow only one simultaneous open() */
	if (amdmsr_open_cnt > 0)
		return EPERM;
	amdmsr_open_cnt++;
	return 0;
}

int
amdmsrclose(dev_t dev, int flags, int devtype, struct proc *p)
{
	amdmsr_open_cnt--;
	return 0;
}

int
amdmsrioctl(dev_t dev, u_long cmd, caddr_t data, int flags, struct proc *p)
{
	struct amdmsr_req *req = (struct amdmsr_req *)data;

	switch (cmd) {
	case RDMSR:
		req->val = rdmsr(req->addr);
		break;
	case WRMSR:
		wrmsr(req->addr, req->val);
		break;
	default:
		return EINVAL;
	}
	return 0;
}
