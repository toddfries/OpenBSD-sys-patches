/*	$OpenBSD: autoconf.c,v 1.46 2010/11/18 21:13:19 miod Exp $ */

/*
 * Copyright (c) 1995 Theo de Raadt
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 * 	The Regents of the University of California. All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: autoconf.c 1.36 92/12/20$
 * 
 *	@(#)autoconf.c  8.2 (Berkeley) 1/12/94
 */

/*
 * Setup the system to run on the current machine.
 *
 * cpu_configure() is called at boot time.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/disklabel.h>

#include <machine/vmparam.h>
#include <machine/autoconf.h>
#include <machine/disklabel.h>
#include <machine/cpu.h>
#include <machine/pte.h>

#include <uvm/uvm_extern.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

void	setroot(void);
int	mainbus_print(void *, const char *);
int	mainbus_scan(struct device *, void *, void *);
int	findblkmajor(struct device *);
struct	device *getdisk(char *, int, int, dev_t *);
struct	device *parsedisk(char *, int, int, dev_t *);

extern void init_intrs(void);
extern void dumpconf(void);

int	get_target(int *, int *, int *);

/* boot device information */
paddr_t	bootaddr;
int	bootctrllun, bootdevlun;
int	bootpart, bootbus;
struct	device *bootdv;

struct	extent *extio;
extern	vaddr_t extiobase;

/*
 * Determine mass storage and memory configuration for a machine.
 */
void
cpu_configure()
{
	bootdv = NULL; /* set by device drivers (if found) */

	init_intrs();

	extio = extent_create("extio",
	    (u_long)extiobase, (u_long)extiobase + ptoa(EIOMAPSIZE),
	    M_DEVBUF, NULL, 0, EX_NOWAIT);

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("autoconfig failed, no root");

	setroot();
	dumpconf();
	cold = 0;
}

/*
 * Allocate/deallocate a cache-inhibited range of kernel virtual address
 * space mapping the indicated physical address range [pa - pa+size)
 */
vaddr_t
mapiodev(pa, size)
	paddr_t pa;
	int size;
{
	int error;
	paddr_t base;
	vaddr_t va, iova;

	if (size <= 0)
		return NULL;

	base = pa & PAGE_MASK;
	pa = trunc_page(pa);
	size = round_page(base + size);

	error = extent_alloc(extio, size, EX_NOALIGN, 0, EX_NOBOUNDARY,
	    EX_NOWAIT | EX_MALLOCOK, &iova);

	if (error != 0)
	        return NULL;

	va = iova;
	while (size != 0) {
		pmap_kenter_cache(va, pa, PG_RW | PG_CI);
		size -= PAGE_SIZE;
		va += PAGE_SIZE;
		pa += PAGE_SIZE;
	}
	pmap_update(pmap_kernel());
	return (iova + base);
}

void
unmapiodev(kva, size)
	vaddr_t kva;
	int size;
{
	int error;
	vaddr_t va;

#ifdef DEBUG
	if (kva < extiobase || kva + size >= extiobase + ctob(EIOMAPSIZE))
	        panic("unmapiodev: bad address");
#endif

	va = trunc_page(kva);
	size = round_page(kva + size) - va;
	pmap_kremove(va, size);
	pmap_update(pmap_kernel());

	error = extent_free(extio, va, size, EX_NOWAIT);
#ifdef DIAGNOSTIC
	if (error != 0)
		printf("unmapiodev: extent_free failed\n");
#endif
}

/*
 * the rest of this file was adapted from Theo de Raadt's code in the 
 * sparc port to nuke the "options GENERIC" stuff.
 */

struct nam2blk {
	char *name;
	int maj;
} nam2blk[] = {
	{ "sd",		4 },
	{ "st",		7 },
	{ "rd",		9 },
};

int
findblkmajor(dv)
	struct device *dv;
{
	char *name = dv->dv_xname;
	register int i;

	for (i = 0; i < sizeof(nam2blk)/sizeof(nam2blk[0]); ++i)
		if (strncmp(name, nam2blk[i].name, strlen(nam2blk[0].name)) == 0)
			return (nam2blk[i].maj);
	return (-1);
}

struct device *
getdisk(str, len, defpart, devp)
	char *str;
	int len, defpart;
	dev_t *devp;
{
	register struct device *dv;

	if ((dv = parsedisk(str, len, defpart, devp)) == NULL) {
		printf("use one of:");
		TAILQ_FOREACH(dv, &alldevs, dv_list) {
			if (dv->dv_class == DV_DISK)
				printf(" %s[a-p]", dv->dv_xname);
#ifdef NFSCLIENT
			if (dv->dv_class == DV_IFNET)
				printf(" %s", dv->dv_xname);
#endif
		}
		printf("\n");
	}
	return (dv);
}

struct device *
parsedisk(str, len, defpart, devp)
	char *str;
	int len, defpart;
	dev_t *devp;
{
	register struct device *dv;
	register char *cp, c;
	int majdev, unit, part;

	if (len == 0)
		return (NULL);
	cp = str + len - 1;
	c = *cp;
	if (c >= 'a' && (c - 'a') < MAXPARTITIONS) {
		part = c - 'a';
		*cp = '\0';
	} else
		part = defpart;

	TAILQ_FOREACH(dv, &alldevs, dv_list) {
		if (dv->dv_class == DV_DISK &&
		    strcmp(str, dv->dv_xname) == 0) {
			majdev = findblkmajor(dv);
			unit = dv->dv_unit;
			if (majdev < 0)
				panic("parsedisk");
			*devp = MAKEDISKDEV(majdev, unit, part);
			break;
		}
#ifdef NFSCLIENT
		if (dv->dv_class == DV_IFNET &&
		    strcmp(str, dv->dv_xname) == 0) {
			*devp = NODEV;
			break;
		}
#endif
	}

	*cp = c;
	return (dv);
}

/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 *
 * XXX Actually, swap and root must be on the same type of device,
 * (ie. DV_DISK or DV_IFNET) because of how (*mountroot) is written.
 * That should be fixed.
 */
void
setroot()
{
	register struct swdevt *swp;
	register struct device *dv;
	register int len, majdev, unit;
	dev_t nrootdev, nswapdev = NODEV;
	char buf[128];
	dev_t temp;
#if defined(NFSCLIENT)
	extern char *nfsbootdevname;
#endif

	printf("boot device: %s\n",
	    (bootdv) ? bootdv->dv_xname : "<unknown>");

	/*
	 * If 'swap generic' and we could not determine the boot device,
	 * ask the user.
	 */
	if (mountroot == NULL && bootdv == NULL)
		boothowto |= RB_ASKNAME;

	if (boothowto & RB_ASKNAME) {
		for (;;) {
			printf("root device ");
			if (bootdv != NULL)
				printf("(default %s%c)",
				    bootdv->dv_xname,
				    bootdv->dv_class == DV_DISK ? 'a' : ' ');
			printf(": ");
			len = getsn(buf, sizeof(buf));
			if (len == 0 && bootdv != NULL) {
				strlcpy(buf, bootdv->dv_xname, sizeof buf);
				len = strlen(buf);
			}
			if (len > 0 && buf[len - 1] == '*') {
				buf[--len] = '\0';
				dv = getdisk(buf, len, 1, &nrootdev);
				if (dv) {
					bootdv = dv;
					nswapdev = nrootdev;
					goto gotswap;
				}
			}
			dv = getdisk(buf, len, 0, &nrootdev);
			if (dv) {
				bootdv = dv;
				break;
			}
		}

		/*
		 * because swap must be on same device as root, for
		 * network devices this is easy.
		 */
		if (bootdv->dv_class == DV_IFNET) {
			goto gotswap;
		}
		for (;;) {
			printf("swap device ");
			if (bootdv != NULL)
				printf("(default %s%c)",
				    bootdv->dv_xname,
				    bootdv->dv_class == DV_DISK ? 'b' : ' ');
			printf(": ");
			len = getsn(buf, sizeof(buf));
			if (len == 0 && bootdv != NULL) {
				switch (bootdv->dv_class) {
				case DV_IFNET:
					nswapdev = NODEV;
					break;
				case DV_DISK:
					nswapdev = MAKEDISKDEV(major(nrootdev),
					    DISKUNIT(nrootdev), 1);
					break;
				case DV_TAPE:
				case DV_TTY:
				case DV_DULL:
				case DV_CPU:
					break;
				}
				break;
			}
			dv = getdisk(buf, len, 1, &nswapdev);
			if (dv) {
				if (dv->dv_class == DV_IFNET)
					nswapdev = NODEV;
				break;
			}
		}
gotswap:
		rootdev = nrootdev;
		dumpdev = nswapdev;
		swdevt[0].sw_dev = nswapdev;
		swdevt[1].sw_dev = NODEV;
	} else if (mountroot == NULL) {
		/*
		 * `swap generic': Use the device the ROM told us to use.
		 */
		if (bootdv == NULL)
			panic("boot device not known");

		majdev = findblkmajor(bootdv);
		if (majdev >= 0) {
			/*
			 * Root and swap are on a disk.
			 * val[2] of the boot device is the partition number.
			 * Assume swap is on partition b.
			 */
			unit = bootdv->dv_unit;
			rootdev = MAKEDISKDEV(majdev, unit, bootpart);
			nswapdev = dumpdev = MAKEDISKDEV(major(rootdev),
			    DISKUNIT(rootdev), 1);
		} else {
			/*
			 * Root and swap are on a net.
			 */
			nswapdev = dumpdev = NODEV;
		}
		swdevt[0].sw_dev = nswapdev;
		swdevt[1].sw_dev = NODEV;
	} else {
		/*
		 * `root DEV swap DEV': honour rootdev/swdevt.
		 * rootdev/swdevt/mountroot already properly set.
		 */
		return;
	}

	switch (bootdv->dv_class) {
#if defined(NFSCLIENT)
	case DV_IFNET:
		mountroot = nfs_mountroot;
		nfsbootdevname = bootdv->dv_xname;
		return;
#endif
#if defined(FFS)
	case DV_DISK:
		mountroot = dk_mountroot;
		majdev = major(rootdev);
		unit = DISKUNIT(rootdev);
		printf("root on %s%c\n", bootdv->dv_xname,
		    DISKPART(rootdev) + 'a');
		break;
#endif
	default:
		printf("can't figure root, hope your kernel is right\n");
		return;
	}

	/*
	 * Make the swap partition on the root drive the primary swap.
	 */
	temp = NODEV;
	for (swp = swdevt; swp->sw_dev != NODEV; swp++) {
		if (majdev == major(swp->sw_dev) &&
		    unit == DISKUNIT(swp->sw_dev)) {
			temp = swdevt[0].sw_dev;
			swdevt[0].sw_dev = swp->sw_dev;
			swp->sw_dev = temp;
			break;
		}
	}
	if (swp->sw_dev == NODEV)
		return;

	/*
	 * If dumpdev was the same as the old primary swap device, move
	 * it to the new primary swap device.
	 */
	if (temp == dumpdev)
		dumpdev = swdevt[0].sw_dev;
}

void
device_register(struct device *dev, void *aux)
{
	if (bootpart == -1) /* ignore flag from controller driver? */
		return;

	/*
	 * scsi: sd,cd
	 */
	if (strcmp("sd", dev->dv_cfdata->cf_driver->cd_name) == 0 ||
	    strcmp("cd", dev->dv_cfdata->cf_driver->cd_name) == 0) {
		struct scsi_attach_args *sa = aux;
		int target, bus, lun;

#if defined(MVME141) || defined(MVME147)
		/*
		 * Both 141 and 147 do not use the controller number to
		 * identify the controller itself, but expect the
		 * operating system to match it with its physical address
		 * (bootaddr), which is indeed what we are doing.
		 * Then the SCSI device id may be found in the controller
		 * number, and the device number is zero (except on MVME141
		 * when booting from MVME319/320/321/322, which we
		 * do not support anyway).
		 */
		if (cputyp == CPU_141 || cputyp == CPU_147) {
			target = bootctrllun;
			bus = lun = 0;
		} else
#endif
		{
			if (get_target(&target, &bus, &lun) != 0)
				return;

			/* make sure we are on the expected scsibus */
			if (bootbus != bus)
				return;
		}
    		
		if (sa->sa_sc_link->target == target &&
		    sa->sa_sc_link->lun == lun) {
			bootdv = dev;
			return;
		}
	}

	/*
	 * ethernet: ie,le
	 */
	else if (strcmp("ie", dev->dv_cfdata->cf_driver->cd_name) == 0 ||
	    strcmp("le", dev->dv_cfdata->cf_driver->cd_name) == 0) {
		struct confargs *ca = aux;

		if (ca->ca_paddr == bootaddr) {
			bootdv = dev;
			return;
		}
	}
}

/*
 * Returns the ID of the SCSI disk based on Motorola's CLUN/DLUN stuff
 * This handles SBC SCSI and MVME32[78].
 */
int
get_target(int *target, int *bus, int *lun)
{
	switch (bootctrllun) {
	/* built-in controller */
	case 0x00:
	/* MVME327 */
	case 0x02:
	case 0x03:
		*bus = 0;
		*target = (bootdevlun & 0x70) >> 4;
		*lun = (bootdevlun & 0x07);
		return (0);
	/* MVME328 */
	case 0x06:
	case 0x07:
	case 0x16:
	case 0x17:
	case 0x18:
	case 0x19:
		*bus = (bootdevlun & 0x40) >> 6;
		*target = (bootdevlun & 0x38) >> 3;
		*lun = (bootdevlun & 0x07);
		return (0);
	default:
		return (ENODEV);
	}
}

struct nam2blk nam2blk[] = {
	{ "sd",		4 },
	{ "st",		7 },
	{ "rd",		9 },
	{ "vnd",	6 },
	{ NULL,		-1 }
};
