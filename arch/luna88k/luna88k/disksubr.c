/* $OpenBSD: disksubr.c,v 1.46 2010/09/29 13:39:03 miod Exp $ */
/* $NetBSD: disksubr.c,v 1.12 2002/02/19 17:09:44 wiz Exp $ */

/*
 * Copyright (c) 1994, 1995 Gordon W. Ross
 * Copyright (c) 1994 Theo de Raadt
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * Credits:
 * This file was based mostly on the i386/disksubr.c file:
 *  	@(#)ufs_disksubr.c	7.16 (Berkeley) 5/4/91
 * The functions: disklabel_sun_to_bsd, disklabel_bsd_to_sun
 * were originally taken from arch/sparc/scsi/sun_disklabel.c
 * (which was written by Theo de Raadt) and then substantially
 * rewritten by Gordon W. Ross.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>

#include <dev/sun/disklabel.h>

/*
 * UniOS disklabel (== ISI disklabel) is very similar to SunOS.
 *	SunOS				UniOS/ISI
 *	text		128			128
 *	(pad)		292			294
 *	rpm		2		-
 *	pcyl		2		badchk	2
 *	sparecyl	2		maxblk	4
 *	(pad)		4		dtype	2
 *	interleave	2		ndisk	2
 *	ncyl		2			2
 *	acyl		2			2
 *	ntrack		2			2
 *	nsect		2			2
 *	(pad)		4		bhead	2
 *	-				ppart	2
 *	dkpart[8]	64			64
 *	magic		2			2
 *	cksum		2			2
 *
 * Magic number value and checksum calculation are identical.  Subtle
 * difference is partition start address; UniOS/ISI maintains sector
 * numbers while SunOS label has cylinder number.
 *
 * It is found that LUNA Mach2.5 has BSD label embedded at offset 64
 * retaining UniOS/ISI label at the end of label block.  LUNA Mach
 * manipulates BSD disklabel in the same manner as 4.4BSD.  It's
 * uncertain LUNA Mach can create a disklabel on fresh disks since
 * Mach writedisklabel logic seems to fail when no BSD label is found.
 *
 * Kernel handles disklabel in this way;
 *	- searchs BSD label at offset 64
 *	- if not found, searchs UniOS/ISI label at the end of block
 *	- kernel can distinguish whether it was SunOS label or UniOS/ISI
 *	  label and understand both
 *	- kernel writes UniOS/ISI label combined with BSD label to update
 *	  the label block
 */

#if LABELSECTOR != 0
#error	"Default value of LABELSECTOR no longer zero?"
#endif

int disklabel_om_to_bsd(struct sun_disklabel *, struct disklabel *);
int disklabel_bsd_to_om(struct disklabel *, struct sun_disklabel *);

/*
 * Attempt to read a disk label from a device
 * using the indicated strategy routine.
 * The label must be partly set up before this:
 * secpercyl, secsize and anything required for a block i/o read
 * operation in the driver's strategy/start routines
 * must be filled in before calling us.
 *
 * Return buffer for use in signalling errors if requested.
 *
 * Returns null on success and an error string on failure.
 */
int
readdisklabel(dev_t dev, void (*strat)(struct buf *),
    struct disklabel *lp, int spoofonly)
{
	struct buf *bp = NULL;
	int error;

	if ((error = initdisklabel(lp)))
		goto done;
	lp->d_flags |= D_VENDOR;

	/* obtain buffer to probe drive with */
	bp = geteblk((int)lp->d_secsize);

	/* next, dig out disk label */
	bp->b_dev = dev;
	bp->b_blkno = LABELSECTOR;
	bp->b_cylinder = 0;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_BUSY | B_READ | B_RAW;
	(*strat)(bp);
	if (biowait(bp)) {
		error = bp->b_error;
		goto done;
	}

	error = disklabel_om_to_bsd((struct sun_disklabel *)bp->b_data, lp);
	if (error == 0)
		goto done;

	error = checkdisklabel(bp->b_data + LABELOFFSET, lp, 0,
	    DL_GETDSIZE(lp));
	if (error == 0)
		goto done;

#if defined(CD9660)
	error = iso_disklabelspoof(dev, strat, lp);
	if (error == 0)
		goto done;
#endif
#if defined(UDF)
	error = udf_disklabelspoof(dev, strat, lp);
	if (error == 0)
		goto done;
#endif

	/* Check for a BSD disk label first. */
	dlp = (struct disklabel *)(clp->cd_block + LABELOFFSET);
	if (dlp->d_magic == DISKMAGIC && dlp->d_magic2 == DISKMAGIC) {
		if (dkcksum(dlp) == 0) {
			*lp = *dlp; 	/* struct assignment */
			return (NULL);
		}
		printf("BSD disk label corrupted");
	}

	/* Check for a UniOS/ISI disk label. */
	slp = (struct sun_disklabel *)clp->cd_block;
	if (slp->sl_magic == SUN_DKMAGIC) {
		return (disklabel_om_to_bsd(clp->cd_block, lp));
	}
	return (error);
}


/*
 * Write disk label back to device after modification.
 * Current label is already in clp->cd_block[]
 */
int
writedisklabel(dev, strat, lp, clp)
	dev_t dev;
	void (*strat)(struct buf *);
	struct disklabel *lp;
	struct cpu_disklabel *clp;
{
	struct buf *bp;
	struct disklabel *dlp;
	int error;

	/* implant OpenBSD disklabel at LABELOFFSET. */
	dlp = (struct disklabel *)(clp->cd_block + LABELOFFSET);
	*dlp = *lp; 	/* struct assignment */

	error = disklabel_bsd_to_om(lp, clp->cd_block);
	if (error)
		return (error);

	/* Get a buffer and copy the new label into it. */
	bp = geteblk((int)lp->d_secsize);
	bcopy(clp->cd_block, bp->b_data, sizeof(clp->cd_block));

	/* Write out the updated label. */
	bp->b_dev = dev;
	bp->b_blkno = LABELSECTOR;
	bp->b_cylinder = 0;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_BUSY | B_READ | B_RAW;

	(*strat)(bp);
	error = biowait(bp);
	brelse(bp);

	return (error);
}

	bp->b_flags = B_BUSY | B_WRITE | B_RAW;
	(*strat)(bp);
	error = biowait(bp);

	/* overwriting disk label ? */
	/* XXX this assumes everything <=LABELSECTOR is label! */
	/*     But since LABELSECTOR is 0, that's ok for now. */
	if (bp->b_blkno + blockpersec(p->p_offset, lp) <= LABELSECTOR &&
	    (bp->b_flags & B_READ) == 0 && wlabel == 0) {
		bp->b_error = EROFS;
		goto bad;
	}

	/* beyond partition? */
	if (bp->b_blkno + sz > blockpersec(p->p_size, lp)) {
		sz = blockpersec(p->p_size, lp) - bp->b_blkno;
		if (sz == 0) {
			/* if exactly at end of disk, return an EOF */
			bp->b_resid = bp->b_bcount;
			return (0);
		}
		if (sz < 0) {
			bp->b_error = EINVAL;
			goto bad;
		}
		/* or truncate if part of it fits */
		bp->b_bcount = sz << DEV_BSHIFT;
	}

	/* calculate cylinder for disksort to order transfers with */
	bp->b_cylinder = (bp->b_blkno + blockpersec(p->p_offset, lp)) /
	    lp->d_secpercyl;
	return (1);

bad:
	bp->b_flags |= B_ERROR;
	return (-1);
}

/************************************************************************
 *
 * The rest of this was taken from arch/sparc/scsi/sun_disklabel.c
 * and then substantially rewritten by Gordon W. Ross
 *
 ************************************************************************/

/* What partition types to assume for Sun disklabels: */
static const u_char
sun_fstypes[8] = {
	FS_BSDFFS,	/* a */
	FS_SWAP,	/* b */
	FS_OTHER,	/* c - whole disk */
	FS_BSDFFS,	/* d */
	FS_BSDFFS,	/* e */
	FS_BSDFFS,	/* f */
	FS_BSDFFS,	/* g */
	FS_BSDFFS,	/* h */
};

/*
 * Given a UniOS/ISI disk label, set lp to a BSD disk label.
 * Returns NULL on success, else an error string.
 *
 * The BSD label is cleared out before this is called.
 */
int
disklabel_om_to_bsd(struct sun_disklabel *sl, struct disklabel *lp)
{
	struct sun_disklabel *sl;
	struct partition *npp;
	struct sun_dkpart *spp;
	int i, secpercyl;
	u_short cksum, *sp1, *sp2;

	sl = (struct sun_disklabel *)cp;

	if (sl->sl_magic != SUN_DKMAGIC)
		return (EINVAL);

	/* Verify the XOR check. */
	sp1 = (u_short *)sl;
	sp2 = (u_short *)(sl + 1);
	cksum = 0;
	while (sp1 < sp2)
		cksum ^= *sp1++;
	if (cksum != 0)
		return (EINVAL);	/* UniOS disk label, bad checksum */

	memset((caddr_t)lp, 0, sizeof(struct disklabel));
	/* Format conversion. */
	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	memcpy(lp->d_packname, sl->sl_text, sizeof(lp->d_packname));

	lp->d_secsize = DEV_BSIZE;
	lp->d_nsectors = sl->sl_nsectors;
	lp->d_ntracks = sl->sl_ntracks;
	lp->d_ncylinders = sl->sl_ncylinders;

	secpercyl = sl->sl_nsectors * sl->sl_ntracks;
	lp->d_secpercyl  = secpercyl;
	lp->d_secperunit = secpercyl * sl->sl_ncylinders;

	memcpy(&lp->d_uid, &sl->sl_uid, sizeof(sl->sl_uid));

	lp->d_acylinders = sl->sl_acylinders;

	if (sl->sl_rpm == 0) {
		/* UniOS label has blkoffset, not cyloffset */
		secpercyl = 1;
	}

	lp->d_npartitions = MAXPARTITIONS;
	/* These are as defined in <ufs/ffs/fs.h> */
	lp->d_bbsize = 8192;				/* XXX */
	lp->d_sbsize = 8192;				/* XXX */
	for (i = 0; i < 8; i++) {
		spp = &sl->sl_part[i];
		npp = &lp->d_partitions[i];
		npp->p_offset = spp->sdkp_cyloffset * secpercyl;
		npp->p_size = spp->sdkp_nsectors;
		if (npp->p_size == 0)
			npp->p_fstype = FS_UNUSED;
		else {
			/* Partition has non-zero size.  Set type, etc. */
			npp->p_fstype = sun_fstypes[i];

			/*
			 * The sun label does not store the FFS fields,
			 * so just set them with default values here.
			 * XXX: This keeps newfs from trying to rewrite
			 * XXX: the disk label in the most common case.
			 * XXX: (Should remove that code from newfs...)
			 */
			if (npp->p_fstype == FS_BSDFFS) {
				npp->p_fsize = 1024;
				npp->p_frag = 8;
				npp->p_cpg = 16;
			}
		}
	}

	/*
	 * XXX BandAid XXX
	 * UniOS rootfs sits on part c which don't begin at sect 0,
	 * and impossible to mount.  Thus, make it usable as part b.
	 */
	if (sl->sl_rpm == 0 && lp->d_partitions[2].p_offset != 0) {
		lp->d_partitions[1] = lp->d_partitions[2];
		lp->d_partitions[1].p_fstype = FS_BSDFFS;
	}

	lp->d_checksum = dkcksum(lp);
	return (checkdisklabel(lp, lp, 0, DL_GETDSIZE(lp)));
}

/*
 * Given a BSD disk label, update the UniOS disklabel
 * pointed to by cp with the new info.  Note that the
 * UniOS disklabel may have other info we need to keep.
 * Returns zero or error code.
 */
int
disklabel_bsd_to_om(lp, cp)
	struct disklabel *lp;
	char *cp;
{
	struct sun_disklabel *sl;
	struct partition *npp;
	struct sun_dkpart *spp;
	int i;
	u_short cksum, *sp1, *sp2;

	if (lp->d_secsize != DEV_BSIZE)
		return (EINVAL);

	sl = (struct sun_disklabel *)cp;

	/* Format conversion. */
	bzero(lp, sizeof(*lp));
	memcpy(sl->sl_text, lp->d_packname, sizeof(lp->d_packname));
	sl->sl_rpm = 0;					/* UniOS */
#if 0 /* leave as was */
	sl->sl_pcyl = lp->d_ncylinders + lp->d_acylinders;	/* XXX */
#endif
	sl->sl_interleave = 1;
	sl->sl_ncylinders = lp->d_ncylinders;
	sl->sl_acylinders = lp->d_acylinders;
	sl->sl_ntracks = lp->d_ntracks;
	sl->sl_nsectors = lp->d_nsectors;

	memcpy(&sl->sl_uid, &lp->d_uid, sizeof(lp->d_uid));

	for (i = 0; i < 8; i++) {
		spp = &sl->sl_part[i];
		npp = &lp->d_partitions[i];

		spp->sdkp_cyloffset = npp->p_offset;	/* UniOS */
		spp->sdkp_nsectors = npp->p_size;
	}
	sl->sl_magic = SUN_DKMAGIC;

	/* Correct the XOR check. */
	sp1 = (u_short *)sl;
	sp2 = (u_short *)(sl + 1);
	sl->sl_cksum = cksum = 0;
	while (sp1 < sp2)
		cksum ^= *sp1++;
	sl->sl_cksum = cksum;

	return (0);
}
