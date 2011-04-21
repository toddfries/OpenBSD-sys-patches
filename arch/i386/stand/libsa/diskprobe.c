/*	$OpenBSD: diskprobe.c,v 1.32 2010/04/23 15:25:20 jsing Exp $	*/

/*
 * Copyright (c) 1997 Tobias Weingartner
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* We want the disk type names from disklabel.h */
#undef DKTYPENAMES

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/reboot.h>
#include <sys/disklabel.h>
#include <dev/biovar.h>
#include <dev/softraidvar.h>
#include <stand/boot/bootarg.h>
#include <machine/biosvar.h>
#include <lib/libz/zlib.h>
#include "disk.h"
#include "biosdev.h"
#include "libsa.h"

#define MAX_CKSUMLEN MAXBSIZE / DEV_BSIZE	/* Max # of blks to cksum */

/* Local Prototypes */
static int disksum(int);

/* List of disk devices we found/probed */
struct disklist_lh disklist;

/* List of softraid volumes and metadata. */
struct sr_boot_volume_head bvh;
struct sr_metadata_list_head mlh;
struct sr_metadata_list_head kdh;

/* Pointer to boot device */
struct diskinfo *bootdev_dip;

extern int debug;
extern int bios_bootdev;
extern int bios_cddev;

/* Probe for all BIOS floppies */
static void
floppyprobe(void)
{
	struct diskinfo *dip;
	int i;

	/* Floppies */
	for (i = 0; i < 4; i++) {
		dip = alloc(sizeof(struct diskinfo));
		bzero(dip, sizeof(*dip));

		if (bios_getdiskinfo(i, &dip->bios_info)) {
#ifdef BIOS_DEBUG
			if (debug)
				printf(" <!fd%u>", i);
#endif
			free(dip, 0);
			break;
		}

		printf(" fd%u", i);

		/* Fill out best we can - (fd?) */
		dip->bios_info.bsd_dev = MAKEBOOTDEV(2, 0, 0, i, RAW_PART);

		/*
		 * Delay reading the disklabel until we're sure we want
		 * to boot from the floppy. Doing this avoids a delay
		 * (sometimes very long) when trying to read the label
		 * and the drive is unplugged.
		 */
		dip->bios_info.flags |= BDI_BADLABEL;

		/* Add to queue of disks */
		TAILQ_INSERT_TAIL(&disklist, dip, list);
	}
}


/* Probe for all BIOS hard disks */
static void
hardprobe(void)
{
	struct diskinfo *dip;
	int i;
	u_int bsdunit, type;
	u_int scsi = 0, ide = 0;
	const char *dc = (const char *)((0x40 << 4) + 0x75);

	/* Hard disks */
	for (i = 0x80; i < (0x80 + *dc); i++) {
		dip = alloc(sizeof(struct diskinfo));
		bzero(dip, sizeof(*dip));

		if (bios_getdiskinfo(i, &dip->bios_info)) {
#ifdef BIOS_DEBUG
			if (debug)
				printf(" <!hd%u>", i&0x7f);
#endif
			free(dip, 0);
			break;
		}

		printf(" hd%u%s", i&0x7f, (dip->bios_info.bios_edd > 0?"+":""));

		/* Try to find the label, to figure out device type */
		if ((bios_getdisklabel(&dip->bios_info, &dip->disklabel)) ) {
			printf("*");
			bsdunit = ide++;
			type = 0;	/* XXX let it be IDE */
		} else {
			/* Best guess */
			switch (dip->disklabel.d_type) {
			case DTYPE_SCSI:
				type = 4;
				bsdunit = scsi++;
				dip->bios_info.flags |= BDI_GOODLABEL;
				break;

			case DTYPE_ESDI:
			case DTYPE_ST506:
				type = 0;
				bsdunit = ide++;
				dip->bios_info.flags |= BDI_GOODLABEL;
				break;

			default:
				dip->bios_info.flags |= BDI_BADLABEL;
				type = 0;	/* XXX Suggest IDE */
				bsdunit = ide++;
			}
		}

		dip->bios_info.checksum = 0; /* just in case */
		/* Fill out best we can */
		dip->bios_info.bsd_dev =
		    MAKEBOOTDEV(type, 0, 0, bsdunit, RAW_PART);

		/* Add to queue of disks */
		TAILQ_INSERT_TAIL(&disklist, dip, list);
	}
}


static void
srprobe(void)
{
	struct sr_metadata_list	*mle, *mlenext, *mle1, *mle2;
	struct sr_boot_volume *vol, *vp1, *vp2;
	struct sr_metadata *md;
	struct diskinfo *dip;
	struct partition *pp;
	int i, error, volno;
	daddr_t off;

	/* Probe for softraid volumes. */
	SLIST_INIT(&mlh);
	md = alloc(SR_META_SIZE * 512);

	TAILQ_FOREACH(dip, &disklist, list) {

		/* Only check hard disks, skip those with I/O errors. */
		if ((dip->bios_info.bios_number & 0x80) == 0 ||
		    (dip->bios_info.flags & BDI_INVALID))
			continue;

		/* Make sure disklabel has been read. */
		if ((dip->bios_info.flags & (BDI_BADLABEL|BDI_GOODLABEL)) == 0)
			continue;

		for (i = 0; i < MAXPARTITIONS; i++) {
			pp = &dip->disklabel.d_partitions[i];
			if (pp->p_fstype != FS_RAID || pp->p_size == 0)
				continue;

			/* Read softraid metadata. */
			bzero(md, SR_META_SIZE * 512);
			off = DL_GETPOFFSET(pp) + SR_META_OFFSET;
			error = biosd_io(F_READ, &dip->bios_info, off,
			    SR_META_SIZE, md);
			if (error)
				continue;
		
			/* XXX - validate checksum */
	
			/* Is this valid softraid metadata? */
			if (md->ssdi.ssd_magic != SR_MAGIC)
				continue;

			mle = alloc(sizeof(struct sr_metadata_list));
			if (mle == NULL)
				continue;
			bcopy(md, &mle->sml_metadata,
			    sizeof(struct sr_metadata));
			mle->sml_diskinfo = dip;
			mle->sml_disk = dip->bios_info.bios_number;
			mle->sml_part = 'a' + i;
			SLIST_INSERT_HEAD(&mlh, mle, sml_link);
		}
	}

	/*
	 * Create a list of volumes and associate chunks with each volume.
	 */

	SLIST_INIT(&bvh);
	SLIST_INIT(&kdh);

	for (mle = SLIST_FIRST(&mlh); mle != SLIST_END(&mlh); mle = mlenext) {

		mlenext = SLIST_NEXT(mle, sml_link);
		SLIST_REMOVE(&mlh, mle, sr_metadata_list, sml_link);

		md = (struct sr_metadata *)&mle->sml_metadata;
		mle->sml_chunk_id = md->ssdi.ssd_chunk_id;

		/* Handle key disks separately. */
		if (md->ssdi.ssd_level == SR_KEYDISK_LEVEL) {
			SLIST_INSERT_HEAD(&kdh, mle, sml_link);
			continue;
		}

		SLIST_FOREACH(vol, &bvh, sbv_link) {
			if (bcmp(&md->ssdi.ssd_uuid, &vol->sbv_uuid,
			    sizeof(md->ssdi.ssd_uuid)) == 0)
				break;
		}

		if (vol == NULL) {
			vol = alloc(sizeof(struct sr_boot_volume));
			vol->sbv_level = md->ssdi.ssd_level;
			vol->sbv_volid = md->ssdi.ssd_volid;
			vol->sbv_chunk_no = md->ssdi.ssd_chunk_no;
			vol->sbv_flags = md->ssdi.ssd_vol_flags;
			bcopy(&md->ssdi.ssd_uuid, &vol->sbv_uuid,
			    sizeof(md->ssdi.ssd_uuid));
			SLIST_INIT(&vol->sml);

			/* Maintain volume order. */
			vp2 = NULL;
			SLIST_FOREACH(vp1, &bvh, sbv_link) {
				if (vp1->sbv_volid > vol->sbv_volid)
					break;
				vp2 = vp1;
			}
			if (vp2 == NULL)
				SLIST_INSERT_HEAD(&bvh, vol, sbv_link);
			else
				SLIST_INSERT_AFTER(vp2, vol, sbv_link);
		}

		/* Maintain chunk order. */
		mle2 = NULL;
		SLIST_FOREACH(mle1, &vol->sml, sml_link) {
			if (mle1->sml_chunk_id > mle->sml_chunk_id)
				break;
			mle2 = mle1;
		}
		if (mle2 == NULL)
			SLIST_INSERT_HEAD(&vol->sml, mle, sml_link);
		else
			SLIST_INSERT_AFTER(mle2, mle, sml_link);

		vol->sbv_dev_no++;
	}

	/*
	 * Assemble RAID volumes.
	 */
	volno = 0;
	SLIST_FOREACH(vol, &bvh, sbv_link) {

		/* Check if this is a hotspare "volume". */
		if (vol->sbv_level == SR_HOTSPARE_LEVEL &&
		    vol->sbv_chunk_no == 1)
			continue;

		if (vol->sbv_chunk_no != vol->sbv_dev_no) {
		}

		/*
		 * XXX - Validate that volume has sufficient chunks for
		 * readonly mode.
		 */

		vol->sbv_unit = volno++;
		printf(" sr%d%s", vol->sbv_unit,
		    vol->sbv_flags & BIOC_SCBOOTABLE ? "*" : "");
	}

	if (md)
		free(md, 0);
}


/* Probe for all BIOS supported disks */
u_int32_t bios_cksumlen;
void
diskprobe(void)
{
	struct diskinfo *dip;
	int i;

	/* These get passed to kernel */
	bios_diskinfo_t *bios_diskinfo;

	/* Init stuff */
	TAILQ_INIT(&disklist);

	/* Do probes */
	floppyprobe();
#ifdef BIOS_DEBUG
	if (debug)
		printf(";");
#endif
	hardprobe();

	srprobe();

	/* Checksumming of hard disks */
	for (i = 0; disksum(i++) && i < MAX_CKSUMLEN; )
		;
	bios_cksumlen = i;

	/* Get space for passing bios_diskinfo stuff to kernel */
	for (i = 0, dip = TAILQ_FIRST(&disklist); dip;
	    dip = TAILQ_NEXT(dip, list))
		i++;
	bios_diskinfo = alloc(++i * sizeof(bios_diskinfo_t));

	/* Copy out the bios_diskinfo stuff */
	for (i = 0, dip = TAILQ_FIRST(&disklist); dip;
	    dip = TAILQ_NEXT(dip, list))
		bios_diskinfo[i++] = dip->bios_info;

	bios_diskinfo[i++].bios_number = -1;
	/* Register for kernel use */
	addbootarg(BOOTARG_CKSUMLEN, sizeof(u_int32_t), &bios_cksumlen);
	addbootarg(BOOTARG_DISKINFO, i * sizeof(bios_diskinfo_t),
	    bios_diskinfo);
}

void
cdprobe(void)
{
	struct diskinfo *dip;
	int cddev = bios_cddev & 0xff;

	/* Another BIOS boot device... */

	if (bios_cddev == -1)			/* Not been set, so don't use */
		return;

	dip = alloc(sizeof(struct diskinfo));
	bzero(dip, sizeof(*dip));

#if 0
	if (bios_getdiskinfo(cddev, &dip->bios_info)) {
		printf(" <!cd0>");	/* XXX */
		free(dip, 0);
		return;
	}
#endif

	printf(" cd0");

	dip->bios_info.bios_number = cddev;
	dip->bios_info.bios_edd = 1;		/* Use the LBA calls */
	dip->bios_info.flags |= BDI_GOODLABEL | BDI_EL_TORITO;
	dip->bios_info.checksum = 0;		 /* just in case */
	dip->bios_info.bsd_dev =
	    MAKEBOOTDEV(6, 0, 0, 0, RAW_PART);

	/* Create an imaginary disk label */
	dip->disklabel.d_secsize = 2048;
	dip->disklabel.d_ntracks = 1;
	dip->disklabel.d_nsectors = 100;
	dip->disklabel.d_ncylinders = 1;
	dip->disklabel.d_secpercyl = dip->disklabel.d_ntracks *
	    dip->disklabel.d_nsectors;
	if (dip->disklabel.d_secpercyl == 0) {
		dip->disklabel.d_secpercyl = 100;
		/* as long as it's not 0, since readdisklabel divides by it */
	}

	strncpy(dip->disklabel.d_typename, "ATAPI CD-ROM",
	    sizeof(dip->disklabel.d_typename));
	dip->disklabel.d_type = DTYPE_ATAPI;

	strncpy(dip->disklabel.d_packname, "fictitious",
	    sizeof(dip->disklabel.d_packname));
	dip->disklabel.d_secperunit = 100;

	dip->disklabel.d_bbsize = 2048;
	dip->disklabel.d_sbsize = 2048;

	/* 'a' partition covering the "whole" disk */
	dip->disklabel.d_partitions[0].p_offset = 0;
	dip->disklabel.d_partitions[0].p_size = 100;
	dip->disklabel.d_partitions[0].p_fstype = FS_UNUSED;

	/* The raw partition is special */
	dip->disklabel.d_partitions[RAW_PART].p_offset = 0;
	dip->disklabel.d_partitions[RAW_PART].p_size = 100;
	dip->disklabel.d_partitions[RAW_PART].p_fstype = FS_UNUSED;

	dip->disklabel.d_npartitions = MAXPARTITIONS;

	dip->disklabel.d_magic = DISKMAGIC;
	dip->disklabel.d_magic2 = DISKMAGIC;
	dip->disklabel.d_checksum = dkcksum(&dip->disklabel);

	/* Add to queue of disks */
	TAILQ_INSERT_TAIL(&disklist, dip, list);
}


/* Find info on given BIOS disk */
struct diskinfo *
dklookup(int dev)
{
	struct diskinfo *dip;

	for (dip = TAILQ_FIRST(&disklist); dip; dip = TAILQ_NEXT(dip, list))
		if (dip->bios_info.bios_number == dev)
			return dip;

	return NULL;
}

void
dump_diskinfo(void)
{
	struct diskinfo *dip;

	printf("Disk\tBIOS#\tType\tCyls\tHeads\tSecs\tFlags\tChecksum\n");
	for (dip = TAILQ_FIRST(&disklist); dip; dip = TAILQ_NEXT(dip, list)) {
		bios_diskinfo_t *bdi = &dip->bios_info;
		int d = bdi->bios_number;
		int u = d & 0x7f;
		char c;

		if (bdi->flags & BDI_EL_TORITO) {
			c = 'c';
			u = 0;
		} else {
		    	c = (d & 0x80) ? 'h' : 'f';
		}

		printf("%cd%d\t0x%x\t%s\t%d\t%d\t%d\t0x%x\t0x%x\n",
		    c, u, d,
		    (bdi->flags & BDI_BADLABEL)?"*none*":"label",
		    bdi->bios_cylinders, bdi->bios_heads, bdi->bios_sectors,
		    bdi->flags, bdi->checksum);
	}
}

/* Find BIOS portion on given BIOS disk
 * XXX - Use dklookup() instead.
 */
bios_diskinfo_t *
bios_dklookup(int dev)
{
	struct diskinfo *dip;

	dip = dklookup(dev);
	if (dip)
		return &dip->bios_info;

	return NULL;
}

/*
 * Checksum one more block on all harddrives
 *
 * Use the adler32() function from libz,
 * as it is quick, small, and available.
 */
int
disksum(int blk)
{
	struct diskinfo *dip, *dip2;
	int st, reprobe = 0;
	char *buf;

	buf = alloca(DEV_BSIZE);
	for (dip = TAILQ_FIRST(&disklist); dip; dip = TAILQ_NEXT(dip, list)) {
		bios_diskinfo_t *bdi = &dip->bios_info;

		/* Skip this disk if it is not a HD or has had an I/O error */
		if (!(bdi->bios_number & 0x80) || bdi->flags & BDI_INVALID)
			continue;

		/* Adler32 checksum */
		st = biosd_io(F_READ, bdi, blk, 1, buf);
		if (st) {
			bdi->flags |= BDI_INVALID;
			continue;
		}
		bdi->checksum = adler32(bdi->checksum, buf, DEV_BSIZE);

		for (dip2 = TAILQ_FIRST(&disklist); dip2 != dip;
				dip2 = TAILQ_NEXT(dip2, list)) {
			bios_diskinfo_t *bd = &dip2->bios_info;
			if ((bd->bios_number & 0x80) &&
			    !(bd->flags & BDI_INVALID) &&
			    bdi->checksum == bd->checksum)
				reprobe = 1;
		}
	}

	return reprobe;
}
