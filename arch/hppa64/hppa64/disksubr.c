/*	$OpenBSD: disksubr.c,v 1.60 2009/08/13 15:23:10 deraadt Exp $	*/

/*
 * Copyright (c) 1999 Michael Shalayeff
 * Copyright (c) 1997 Niklas Hallqvist
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

/*
 * This disksubr.c module started to take its present form on OpenBSD/alpha
 * but it was always thought it should be made completely MI and not need to
 * be in that alpha-specific tree at all.
 *
 * XXX HPUX disklabel is not understood yet.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/syslog.h>
#include <sys/disk.h>

int	readliflabel(struct buf *, void (*)(struct buf *),
	    struct disklabel *, int *, int);

/*
 * Attempt to read a disk label from a device
 * using the indicated strategy routine.
 * The label must be partly set up before this:
 * secpercyl, secsize and anything required for a block i/o read
 * operation in the driver's strategy/start routines
 * must be filled in before calling us.
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

	/* get a buffer and initialize it */
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;

	error = readliflabel(bp, strat, lp, NULL, spoofonly);
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

	/* If there was an error, still provide a decent fake one.  */
	if (msg)
		*lp = fallbacklabel;

	if (bp) {
		bp->b_flags |= B_INVAL;
		brelse(bp);
	}
	return (error);
}

int
readliflabel(struct buf *bp, void (*strat)(struct buf *),
    struct disklabel *lp, int *partoffp, int spoofonly)
{
	struct buf *dbp = NULL;
	struct lifdir *p;
	struct lifvol *lvp;
	int error = 0;
	int fsoff = 0, openbsdstart = MAXLIFSPACE, i;

	/* read LIF volume header */
	bp->b_blkno = btodb(LIF_VOLSTART);
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_BUSY | B_READ | B_RAW;
	(*strat)(bp);
	if (biowait(bp))
		return (bp->b_error);

	lvp = (struct lifvol *)bp->b_data;
	if (lvp->vol_id != LIF_VOL_ID) {
		error = EINVAL;		/* no LIF volume header */
		goto done;
	}

	/* record the OpenBSD partition's placement for the caller */
	if (partoffp)
		*partoffp = dospartoff;
	if (cylp)
		*cylp = cyl;

	/* read LIF directory */
	dbp->b_blkno = lifstodb(lvp->vol_addr);
	dbp->b_bcount = lp->d_secsize;
	dbp->b_flags = B_BUSY | B_READ | B_RAW;
	(*strat)(dbp);
	if (biowait(dbp)) {
		error = dbp->b_error;
		goto done;
	}

char *
readliflabel (bp, strat, lp, osdep, partoffp, cylp, spoofonly)
	struct buf *bp;
	void (*strat)(struct buf *);
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
	int *partoffp;
	int *cylp;
	int spoofonly;
{
	int fsoff;

	if (p->dir_type == LIF_DIR_FS) {
		fsoff = lifstodb(p->dir_addr);
		openbsdstart = 0;
		goto finished;
	}

	/* Only came here to find the offset... */
	if (partoffp)
		goto finished;

		dev = bp->b_dev;
		dbp = geteblk(LIF_DIRSIZE);
		dbp->b_dev = dev;

		/* read LIF directory */
		dbp->b_blkno = lifstodb(osdep->u._hppa.lifvol.vol_addr);
		dbp->b_bcount = lp->d_secsize;
		dbp->b_flags = B_BUSY | B_READ | B_RAW;
		(*strat)(dbp);

		if (biowait(dbp)) {
			error = dbp->b_error;
			goto done;
		}

		hl = (struct hpux_label *)dbp->b_data;
		if (hl->hl_magic1 != hl->hl_magic2 ||
		    hl->hl_magic != HPUX_MAGIC || hl->hl_version != 1) {
			error = EINVAL;	 /* HPUX label magic mismatch */
			goto done;
		}

		bcopy(dbp->b_data, osdep->u._hppa.lifdir, LIF_DIRSIZE);
		dbp->b_flags |= B_INVAL;
		brelse(dbp);

		/* scan for LIF_DIR_FS dir entry */
		for (fsoff = -1,  p = &osdep->u._hppa.lifdir[0];
		    fsoff < 0 && p < &osdep->u._hppa.lifdir[LIF_NUMDIR]; p++) {
			if (p->dir_type == LIF_DIR_FS ||
			    p->dir_type == LIF_DIR_HPLBL)
				break;
		}

		if (p->dir_type == LIF_DIR_FS)
			fsoff = lifstodb(p->dir_addr);
		else if (p->dir_type == LIF_DIR_HPLBL) {
			struct hpux_label *hl;
			struct partition *pp;
			u_int8_t fstype;
			int i;

			dev = bp->b_dev;
			dbp = geteblk(LIF_DIRSIZE);
			dbp->b_dev = dev;

			/* read LIF directory */
			dbp->b_blkno = lifstodb(p->dir_addr);
			dbp->b_bcount = lp->d_secsize;
			dbp->b_flags = B_BUSY | B_READ;
			dbp->b_cylinder = dbp->b_blkno / lp->d_secpercyl;
			(*strat)(dbp);

			if (biowait(dbp)) {
				if (partoffp)
					*partoffp = -1;

				dbp->b_flags |= B_INVAL;
				brelse(dbp);
				return ("HOUX label I/O error");
			}

			bcopy(dbp->b_data, &osdep->u._hppa.hplabel,
			    sizeof(osdep->u._hppa.hplabel));
			dbp->b_flags |= B_INVAL;
			brelse(dbp);

			hl = &osdep->u._hppa.hplabel;
			if (hl->hl_magic1 != hl->hl_magic2 ||
			    hl->hl_magic != HPUX_MAGIC ||
			    hl->hl_version != 1) {
				if (partoffp)
					*partoffp = -1;

				return "HPUX label magic mismatch";
			}

			lp->d_bbsize = 8192;
			lp->d_sbsize = 8192;
			for (i = 0; i < MAXPARTITIONS; i++) {
				lp->d_partitions[i].p_size = 0;
				lp->d_partitions[i].p_offset = 0;
				lp->d_partitions[i].p_fstype = 0;
			}

			for (i = 0; i < HPUX_MAXPART; i++) {
				if (!hl->hl_flags[i])
					continue;

				if (hl->hl_flags[i] == HPUX_PART_ROOT) {
					pp = &lp->d_partitions[0];
					fstype = FS_BSDFFS;
				} else if (hl->hl_flags[i] == HPUX_PART_SWAP) {
					pp = &lp->d_partitions[1];
					fstype = FS_SWAP;
				} else if (hl->hl_flags[i] == HPUX_PART_BOOT) {
					pp = &lp->d_partitions[RAW_PART + 1];
					fstype = FS_BSDFFS;
				} else
					continue;

				pp->p_size = hl->hl_parts[i].hlp_length * 2;
				pp->p_offset = hl->hl_parts[i].hlp_start * 2;
				pp->p_fstype = fstype;
			}

			lp->d_partitions[RAW_PART].p_size = lp->d_secperunit;
			lp->d_partitions[RAW_PART].p_offset = 0;
			lp->d_partitions[RAW_PART].p_fstype = FS_UNUSED;
			lp->d_npartitions = MAXPARTITIONS;
			lp->d_magic = DISKMAGIC;
			lp->d_magic2 = DISKMAGIC;
			lp->d_checksum = 0;
			lp->d_checksum = dkcksum(lp);

			return (NULL);
		}

		/* if no suitable lifdir entry found assume zero */
		if (fsoff < 0) {
			fsoff = 0;
		}
	}

finished:
	/* record the OpenBSD partition's placement for the caller */
	if (partoffp)
		*partoffp = fsoff;
	else {
		DL_SETBSTART(lp, openbsdstart);
		DL_SETBEND(lp, DL_GETDSIZE(lp));	/* XXX */
	}

	/* don't read the on-disk label if we are in spoofed-only mode */
	if (spoofonly)
		goto done;

	bp->b_blkno = fsoff + LABELSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_BUSY | B_READ | B_RAW;
	(*strat)(bp);
	if (biowait(bp)) {
		error = bp->b_error;
		goto done;
	}

	error = checkdisklabel(bp->b_data + LABELOFFSET, lp, openbsdstart,
	    DL_GETDSIZE(lp));	/* XXX */

done:
	if (dbp) {
		dbp->b_flags |= B_INVAL;
		brelse(dbp);
	}
	return (error);
}


/*
 * Write disk label back to device after modification.
 */
int
writedisklabel(dev, strat, lp, osdep)
	dev_t dev;
	void (*strat)(struct buf *);
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
	char *msg = "no disk label";
	struct buf *bp;
	struct disklabel dl;
	struct cpu_disklabel cdl;
	int labeloffset, error, partoff = 0, cyl = 0;

	/* get a buffer and initialize it */
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;

	/*
	 * I once played with the thought of using osdep->label{tag,sector}
	 * as a cache for knowing where (and what) to write.  However, now I
	 * think it might be useful to reprobe if someone has written
	 * a newer disklabel of another type with disklabel(8) and -r.
	 */
	dl = *lp;
	msg = readliflabel(bp, strat, &dl, &cdl, &partoff, &cyl, 0);
	labeloffset = HPPA_LABELOFFSET;
	if (msg) {	
		dl = *lp;
		msg = readdoslabel(bp, strat, &dl, &cdl, &partoff, &cyl, 0);
		labeloffset = I386_LABELOFFSET;
	}
	if (msg) {
		if (partoff == -1)
			return EIO;

		/* Write it in the regular place with native byte order. */
		labeloffset = LABELOFFSET;
		bp->b_blkno = partoff + LABELSECTOR;
		bp->b_cylinder = cyl;
		bp->b_bcount = lp->d_secsize;
	}

	/* Read it in, slap the new label in, and write it back out */
	bp->b_blkno = partoff + LABELSECTOR;
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
bounds_check_with_label(bp, lp, osdep, wlabel)
	struct buf *bp;
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
	int wlabel;
{
#define blockpersec(count, lp) ((count) * (((lp)->d_secsize) / DEV_BSIZE))
	struct partition *p = lp->d_partitions + DISKPART(bp->b_dev);
	int labelsector = blockpersec(lp->d_partitions[RAW_PART].p_offset,
	    lp) + osdep->labelsector;
	int sz = howmany(bp->b_bcount, DEV_BSIZE);

	/* avoid division by zero */
	if (lp->d_secpercyl == 0) {
		bp->b_error = EINVAL;
		goto bad;
	}

	/* beyond partition? */
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
	    bp->b_blkno + blockpersec(p->p_offset, lp) + sz > labelsector &&
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
