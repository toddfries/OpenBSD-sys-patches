/*	$OpenBSD: disksubr.c,v 1.68 2009/10/05 22:20:35 dms Exp $	*/
/*	$NetBSD: disksubr.c,v 1.21 1996/05/03 19:42:03 christos Exp $	*/

/*
 * Copyright (c) 1996 Theo de Raadt
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
 *	@(#)ufs_disksubr.c	7.16 (Berkeley) 5/4/91
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/syslog.h>
#include <sys/disk.h>

int	readdpmelabel(struct buf *, void (*)(struct buf *),
	    struct disklabel *, int *, int);

/*
 * Attempt to read a disk label from a device
 * using the indicated strategy routine.
 * The label must be partly set up before this:
 * secpercyl, secsize and anything required for a block i/o read
 * operation in the driver's strategy/start routines
 * must be filled in before calling us.
 *
 * If dos partition table requested, attempt to load it and
 * find disklabel inside a DOS partition. Return buffer
 * for use in signalling errors if requested.
 *
 * We would like to check if each MBR has a valid BOOT_MAGIC, but
 * we cannot because it doesn't always exist. So.. we assume the
 * MBR is valid.
 *
 * Returns null on success and an error string on failure.
 */
int
readdisklabel(dev_t dev, void (*strat)(struct buf *),
    struct disklabel *lp, struct cpu_disklabel *osdep, int spoofonly)
{
	struct buf *bp = NULL;
	int error;

	if ((error = initdisklabel(lp)))
		goto done;

	/* get a buffer and initialize it */
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;

	error = readdpmelabel(bp, strat, lp, NULL, spoofonly);
	if (error == 0)
		goto done;

	error = readdoslabel(bp, strat, lp, NULL, spoofonly);
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

done:
	if (bp) {
		bp->b_flags |= B_INVAL;
		brelse(bp);
	}
	return (error);
}

int
readdpmelabel(struct buf *bp, void (*strat)(struct buf *),
    struct disklabel *lp, int *partoffp, int spoofonly)
{
	int i, part_cnt, n, hfspartoff = -1;
	u_int64_t hfspartend = DL_GETDSIZE(lp);
	struct part_map_entry *part;

	/* First check for a DPME (HFS) disklabel */
	bp->b_blkno = 1;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_BUSY | B_READ | B_RAW;
	(*strat)(bp);
	if (biowait(bp))
		return (bp->b_error);

	/* if successful, wander through DPME partition table */
	if (biowait(bp)) {
		msg = "DPME partition I/O error";
		goto hfs_done;
	}

	part = (struct part_map_entry *)bp->b_data;
	/* if first partition is not valid, assume not HFS/DPME partitioned */
	if (part->pmSig != PART_ENTRY_MAGIC)
		return (EINVAL);	/* not a DPME partition */
	part_cnt = part->pmMapBlkCnt;
	n = 8;
	for (i = 0; i < part_cnt; i++) {
		struct partition *pp;
		char *s;

		bp->b_blkno = 1+i;
		bp->b_bcount = lp->d_secsize;
		bp->b_flags = B_BUSY | B_READ | B_RAW;
		(*strat)(bp);
		if (biowait(bp))
			return (bp->b_error);

		if (biowait(bp)) {
			msg = "DPME partition I/O error";
			goto hfs_done;
		}
		part = (struct part_map_entry *)bp->b_data;
		/* toupper the string, in case caps are different... */
		for (s = part->pmPartType; *s; s++)
			if ((*s >= 'a') && (*s <= 'z'))
				*s = (*s - 'a' + 'A');

		if (strcmp(part->pmPartType, PART_TYPE_OPENBSD) == 0) {
			hfspartoff = part->pmPyPartStart - LABELSECTOR;
			hfspartend = hfspartoff + part->pmPartBlkCnt;
			if (partoffp) {
				*partoffp = hfspartoff;
				return (0);
			} else {
				DL_SETBSTART(lp, hfspartoff);
				DL_SETBEND(lp,
				    hfspartend < DL_GETDSIZE(lp) ? hfspartend :
				    DL_GETDSIZE(lp));
			}
			continue;
		}

		if (n >= MAXPARTITIONS || partoffp)
			continue;

		/* Currently we spoof HFS partitions only. */
		if (strcmp(part->pmPartType, PART_TYPE_MAC) == 0) {
			pp = &lp->d_partitions[n];
			DL_SETPOFFSET(pp, part->pmPyPartStart);
			DL_SETPSIZE(pp, part->pmPartBlkCnt);
			pp->p_fstype = FS_HFS;
			n++;
		}

	}

	if (hfspartoff == -1)
		return (EINVAL);	/* no OpenBSD partition inside DPME label */

	/* don't read the on-disk label if we are in spoofed-only mode */
	if (spoofonly)
		return (0);

	/* next, dig out disk label */
	bp->b_blkno = dospartoff + LABELSECTOR;
	bp->b_cylinder = cyl;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_BUSY | B_READ | B_RAW;
	(*strat)(bp);
	if (biowait(bp))
		return(bp->b_error);

	return checkdisklabel(bp->b_data + LABELOFFSET, lp, hfspartoff,
	    hfspartend);
}

/*
 * Check new disk label for sensibility
 * before setting it.
 */
int
setdisklabel(struct disklabel *olp, struct disklabel *nlp, u_long openmask,
    struct cpu_disklabel *osdep)
{
	int i;
	struct partition *opp, *npp;

	/* sanity clause */
	if (nlp->d_secpercyl == 0 || nlp->d_secsize == 0 ||
	    (nlp->d_secsize % DEV_BSIZE) != 0)
		return(EINVAL);

	/* special case to allow disklabel to be invalidated */
	if (nlp->d_magic == 0xffffffff) {
		*olp = *nlp;
		return (0);
	}

	if (nlp->d_magic != DISKMAGIC || nlp->d_magic2 != DISKMAGIC ||
	    dkcksum(nlp) != 0)
		return (EINVAL);

	/* XXX missing check if other dos partitions will be overwritten */

	while (openmask != 0) {
		i = ffs(openmask) - 1;
		openmask &= ~(1 << i);
		if (nlp->d_npartitions <= i)
			return (EBUSY);
		opp = &olp->d_partitions[i];
		npp = &nlp->d_partitions[i];
		if (npp->p_offset != opp->p_offset || npp->p_size < opp->p_size)
			return (EBUSY);
		/*
		 * Copy internally-set partition information
		 * if new label doesn't include it.		XXX
		 */
		if (npp->p_fstype == FS_UNUSED && opp->p_fstype != FS_UNUSED) {
			npp->p_fstype = opp->p_fstype;
			npp->p_fsize = opp->p_fsize;
			npp->p_frag = opp->p_frag;
			npp->p_cpg = opp->p_cpg;
		}
	}
	nlp->d_checksum = 0;
	nlp->d_checksum = dkcksum(nlp);
	*olp = *nlp;
	return (0);
}


/*
 * Write disk label back to device after modification.
 * XXX cannot handle OpenBSD partitions in extended partitions!
 */
int
writedisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
    struct cpu_disklabel *osdep)
{
	struct dos_partition dp[NDOSPART], *dp2;
	struct disklabel *dlp;
	struct buf *bp;
	int error, dospartoff, cyl, i;
	int ourpart = -1;

	/* get a buffer and initialize it */
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;

	/* try DPME partition */
	if (osdep->macparts[0].pmSig == PART_ENTRY_MAGIC) {
		/* only write if a valid "OpenBSD" partition type exists */
		if (osdep->macparts[1].pmSig == PART_ENTRY_MAGIC) {
			bp->b_blkno = osdep->macparts[1].pmPyPartStart;
			bp->b_cylinder = bp->b_blkno/lp->d_secpercyl;
			bp->b_bcount = lp->d_secsize;
			bp->b_flags = B_BUSY | B_WRITE;
			*(struct disklabel *)bp->b_data = *lp;
			(*strat)(bp);
			error = biowait(bp);
			goto done;
		}

		/* SHOULD FAIL TO WRITE LABEL IF VALID HFS partition exists
		 * and no OpenBSD partition exists
		 */
		error = 1; /* EPERM? */
		goto done;
	}

	/* do dos partitions in the process of getting disklabel? */
	dospartoff = 0;
	cyl = LABELSECTOR / lp->d_secpercyl;
	/* read master boot record */
	bp->b_blkno = DOSBBSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_BUSY | B_READ | B_RAW;
	(*strat)(bp);

	if ((error = biowait(bp)) != 0)
		goto done;

	dlp = (struct disklabel *)(bp->b_data + LABELOFFSET);
	*dlp = *lp;
	bp->b_flags = B_BUSY | B_WRITE | B_RAW;
	(*strat)(bp);
	error = biowait(bp);

done:
	bp->b_flags |= B_INVAL;
	brelse(bp);
	return (error);
}

/*
 * Determine the size of the transfer, and make sure it is
 * within the boundaries of the partition. Adjust transfer
 * if needed, and signal errors or early completion.
 */
int
bounds_check_with_label(struct buf *bp, struct disklabel *lp,
    struct cpu_disklabel *osdep, int wlabel)
{
#define blockpersec(count, lp) ((count) * (((lp)->d_secsize) / DEV_BSIZE))
	struct partition *p = lp->d_partitions + DISKPART(bp->b_dev);
	int labelsector = blockpersec(lp->d_partitions[RAW_PART].p_offset, lp) +
	    LABELSECTOR;
	int sz = howmany(bp->b_bcount, DEV_BSIZE);

	/* avoid division by zero */
	if (lp->d_secpercyl == 0) {
		bp->b_error = EINVAL;
		goto bad;
	}

	if (bp->b_blkno + sz > blockpersec(p->p_size, lp)) {
		sz = blockpersec(p->p_size, lp) - bp->b_blkno;
		if (sz == 0) {
			/* If exactly at end of disk, return EOF. */
			bp->b_resid = bp->b_bcount;
			goto done;
		}
		if (sz < 0) {
			/* If past end of disk, return EINVAL. */
			bp->b_error = EINVAL;
			goto bad;
		}
		/* Otherwise, truncate request. */
		bp->b_bcount = sz << DEV_BSHIFT;
	}

	/* Overwriting disk label? */
	if (bp->b_blkno + blockpersec(p->p_offset, lp) <= labelsector &&
#if LABELSECTOR != 0
	    bp->b_blkno + blockpersec(p->p_offset, lp) + sz > labelsector &&
#endif
	    (bp->b_flags & B_READ) == 0 && !wlabel) {
		bp->b_error = EROFS;
		goto bad;
	}

	/* calculate cylinder for disksort to order transfers with */
	bp->b_cylinder = (bp->b_blkno + blockpersec(p->p_offset, lp)) /
	    lp->d_secpercyl;
	return (1);

bad:
	bp->b_flags |= B_ERROR;
done:
	return (0);
}
