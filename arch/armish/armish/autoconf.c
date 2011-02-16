/*	$OpenBSD: autoconf.c,v 1.11 2010/11/28 20:44:20 miod Exp $	*/
/*	$NetBSD: autoconf.c,v 1.2 2001/09/05 16:17:36 matt Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
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
 *	This product includes software developed by Mark Brinicombe for
 *      the NetBSD project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * autoconf.c
 *
 * Autoconfiguration functions
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <machine/bootconfig.h>
#include <machine/intr.h>

#include <dev/cons.h>

struct device *bootdv = NULL;

int findblkmajor(struct device *dv);
char *findblkname(int maj);

void rootconf(void);
void diskconf(void);

static struct device * getdisk(char *str, int len, int defpart, dev_t *devp);
struct  device *parsedisk(char *, int, int, dev_t *);
extern char *boot_file;

#include "wd.h"
#if NWD > 0  
extern  struct cfdriver wd_cd;
#endif
#include "sd.h"
#if NSD > 0
extern  struct cfdriver sd_cd;
#endif
#include "cd.h"
#if NCD > 0
extern  struct cfdriver cd_cd;
#endif
#if NRD > 0
extern  struct cfdriver rd_cd;
#endif
#include "raid.h"
#if NRAID > 0
extern  struct cfdriver raid_cd;
#endif

struct  genericconf {
	char *gc_name;
	dev_t gc_major;
} genericconf[] = {
#if NWD > 0
	{ "wd",  16 },
#endif
#if NSD > 0
	{ "sd",  24 },
#endif
#if NCD > 0
	{ "cd",  26 },
#endif
#if NRD > 0
	{ "rd",  18 },
#endif
#if NRAID > 0
	{ "raid",  71 },
#endif
	{ 0 }
};

int
findblkmajor(dv)
	struct device *dv;
{
	char *name = dv->dv_xname;
	int i;

	for (i = 0; i < sizeof(genericconf)/sizeof(genericconf[0]); ++i)
		if (strncmp(name, genericconf[i].gc_name,
		    strlen(genericconf[i].gc_name)) == 0)
			return (genericconf[i].gc_major);
	return (-1);
}

char *
findblkname(maj)
	int maj;
{
	int i;

	for (i = 0; i < sizeof(genericconf)/sizeof(genericconf[0]); ++i)
		if (maj == genericconf[i].gc_major)
			return (genericconf[i].gc_name);
	return (NULL);
}

static struct device *
getdisk(char *str, int len, int defpart, dev_t *devp)
{
	struct device *dv;

	if ((dv = parsedisk(str, len, defpart, devp)) == NULL) {
		printf("use one of: reboot");
		for (dv = alldevs.tqh_first; dv != NULL;
		    dv = dv->dv_list.tqe_next) {
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
parsedisk(char *str, int len, int defpart, dev_t *devp)
{
	struct device *dv;
	char *cp, c;
	int majdev, part;

	if (len == 0)
		return (NULL);
	cp = str + len - 1;
	c = *cp;
	if (c >= 'a' && (c - 'a') < MAXPARTITIONS) {
		part = c - 'a';
		*cp = '\0';
	} else
		part = defpart;

	for (dv = alldevs.tqh_first; dv != NULL; dv = dv->dv_list.tqe_next) {
		if (dv->dv_class == DV_DISK &&
		    strcmp(str, dv->dv_xname) == 0) {
			majdev = findblkmajor(dv);
			if (majdev < 0)
				panic("parsedisk");
			*devp = MAKEDISKDEV(majdev, dv->dv_unit, part);
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
 * Now that we are fully operational, we can checksum the
 * disks, and using some heuristics, hopefully are able to
 * always determine the correct root disk.
 */
void
diskconf()
{
	/*
	 * Configure root, swap, and dump area.  This is
	 * currently done by running the same checksum
	 * algorithm over all known disks, as was done in
	 * /boot.  Then we basically fixup the *dev vars
	 * from the info we gleaned from this.
	 */
#if 0
	dkcsumattach();

#endif
	rootconf();
#if 0
	dumpconf();
#endif
}


/*
 * void cpu_configure()
 *
 * Configure all the root devices
 * The root devices are expected to configure their own children
 */
void
cpu_configure(void)
{
	softintr_init();

	/*
	 * Since various PCI interrupts could be routed via the ICU
	 * (for PCI devices in the bridge) we need to set up the ICU
	 * now so that these interrupts can be established correctly
	 * i.e. This is a hack.
	 */

	config_rootfound("mainbus", NULL);

	/*
	 * We can not know which is our root disk, defer
	 * until we can checksum blocks to figure it out.
	 */
	md_diskconf = diskconf;
	cold = 0;

	/* Time to start taking interrupts so lets open the flood gates .... */
	(void)spl0();

}

/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 */
void
rootconf()
{
	int  majdev, mindev, unit, part, len;
	dev_t temp;
	struct swdevt *swp;
	struct device *dv;
	dev_t nrootdev, nswapdev = NODEV;
	char buf[128];
	int s;

#if defined(NFSCLIENT)
	extern char *nfsbootdevname;
#endif

	if (boothowto & RB_DFLTROOT)
		return;		/* Boot compiled in */

	/*
	 * (raid) device auto-configuration could have returned
	 * the root device's id in rootdev.  Check this case.
	 */
	if (rootdev != NODEV) {
		majdev = major(rootdev);
		unit = DISKUNIT(rootdev);
		part = DISKPART(rootdev);

		len = snprintf(buf, sizeof buf, "%s%d", findblkname(majdev),
			unit);
		if (len == -1 || len >= sizeof(buf))
			panic("rootconf: device name too long");

		bootdv = getdisk(buf, len, part, &rootdev);
	}

	/* Lookup boot device from boot if not set by configuration */
	if (bootdv == NULL) {
		int len;
		char *p;

		/* boot_file is of the form wd0a:/bsd, we want 'wd0a' */
		if ((p = strchr(boot_file, ':')) != NULL)
			len = p - boot_file;
		else
			len = strlen(boot_file);
		
		bootdv = parsedisk(boot_file, len, 0, &temp);
	}
	if (bootdv == NULL) {
		printf("boot device: lookup '%s' failed.\n", boot_file);
		boothowto |= RB_ASKNAME; /* Don't Panic :-) */
		/* boothowto |= RB_SINGLE; */
	} else
		printf("boot device: %s.\n", bootdv->dv_xname);

struct nam2blk nam2blk[] = {
	{ "wd",		16 },
	{ "sd",		24 },
	{ "cd",		26 },
	{ "rd",		18 },
	{ "raid",	71 },
	{ "vnd",	19 },
	{ NULL,		-1 }
};
