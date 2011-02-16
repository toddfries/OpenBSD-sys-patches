<<<<<<< HEAD
/*	$OpenBSD: vfs_bio.c,v 1.86 2006/10/19 12:04:31 mickey Exp $	*/
=======
/*	$OpenBSD: vfs_bio.c,v 1.127 2010/11/13 17:45:44 deraadt Exp $	*/
>>>>>>> origin/master
/*	$NetBSD: vfs_bio.c,v 1.44 1996/06/11 11:15:36 pk Exp $	*/

/*
 * Copyright (c) 1994 Christopher G. Demetriou
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)vfs_bio.c	8.6 (Berkeley) 1/11/94
 */

/*
 * Some references:
 *	Bach: The Design of the UNIX Operating System (Prentice Hall, 1986)
 *	Leffler, et al.: The Design and Implementation of the 4.3BSD
 *		UNIX Operating System (Addison Welley, 1989)
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/resourcevar.h>
#include <sys/conf.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <miscfs/specfs/specdev.h>

/*
 * Definitions for the buffer free lists.
 */
<<<<<<< HEAD
#define	BQUEUES		3		/* number of free buffer queues */

#define	BQ_CLEAN	0		/* LRU queue with clean buffers */
#define	BQ_DIRTY	1		/* LRU queue with dirty buffers */
#define	BQ_EMPTY	2		/* buffer headers with no memory */
=======
#define	BQUEUES		2		/* number of free buffer queues */

#define	BQ_DIRTY	0		/* LRU queue with dirty buffers */
#define	BQ_CLEAN	1		/* LRU queue with clean buffers */
>>>>>>> origin/master

TAILQ_HEAD(bqueues, buf) bufqueues[BQUEUES];
int needbuffer;
int nobuffers;
struct bio_ops bioops;

/*
 * Buffer pool for I/O buffers.
 */
struct pool bufpool;
<<<<<<< HEAD
=======
struct bufhead bufhead = LIST_HEAD_INITIALIZER(bufhead);
void buf_put(struct buf *);
>>>>>>> origin/master

/*
 * Insq/Remq for the buffer free lists.
 */
#define	binsheadfree(bp, dp)	TAILQ_INSERT_HEAD(dp, bp, b_freelist)
#define	binstailfree(bp, dp)	TAILQ_INSERT_TAIL(dp, bp, b_freelist)

<<<<<<< HEAD
static __inline struct buf *bio_doread(struct vnode *, daddr64_t, int, int);
struct buf *getnewbuf(int, int, int *);
=======
struct buf *bio_doread(struct vnode *, daddr64_t, int, int);
struct buf *buf_get(struct vnode *, daddr64_t, size_t);
void bread_cluster_callback(struct buf *);
>>>>>>> origin/master

/*
 * We keep a few counters to monitor the utilization of the buffer cache
 *
 *  numdirtypages - number of pages on BQ_DIRTY queue.
 *  lodirtypages  - low water mark for buffer cleaning daemon.
 *  hidirtypages  - high water mark for buffer cleaning daemon.
 *  numcleanpages - number of pages on BQ_CLEAN queue.
 *		    Used to track the need to speedup the cleaner and 
 *		    as a reserve for special processes like syncer.
 *  mincleanpages - the lowest byte count on BQ_CLEAN.
 *  numemptybufs  - number of buffers on BQ_EMPTY. unused.
 */
<<<<<<< HEAD
long numdirtypages;
=======

struct bcachestats bcstats;
>>>>>>> origin/master
long lodirtypages;
long hidirtypages;
long locleanpages;
<<<<<<< HEAD
int numemptybufs;
#ifdef DEBUG
long mincleanpages;
#endif
=======
long hicleanpages;
long maxcleanpages;
long backoffpages;	/* backoff counter for page allocations */
long buflowpages;	/* bufpages low water mark */
long bufhighpages; 	/* bufpages high water mark */
long bufbackpages; 	/* number of pages we back off when asked to shrink */
>>>>>>> origin/master

/* XXX - should be defined here. */
extern int bufcachepercent;

<<<<<<< HEAD
=======
vsize_t bufkvm;

struct proc *cleanerproc;
int bd_req;			/* Sleep point for cleaner daemon. */

>>>>>>> origin/master
void
bremfree(struct buf *bp)
{
	struct bqueues *dp = NULL;
<<<<<<< HEAD
=======

	splassert(IPL_BIO);
>>>>>>> origin/master

	/*
	 * We only calculate the head of the freelist when removing
	 * the last element of the list as that is the only time that
	 * it is needed (e.g. to reset the tail pointer).
	 *
	 * NB: This makes an assumption about how tailq's are implemented.
	 */
	if (TAILQ_NEXT(bp, b_freelist) == NULL) {
		for (dp = bufqueues; dp < &bufqueues[BQUEUES]; dp++)
			if (dp->tqh_last == &TAILQ_NEXT(bp, b_freelist))
				break;
		if (dp == &bufqueues[BQUEUES])
			panic("bremfree: lost tail");
	}
<<<<<<< HEAD
	if (bp->b_bufsize <= 0) {
		numemptybufs--;
	} else {
		numfreepages -= btoc(bp->b_bufsize);
		if (!ISSET(bp->b_flags, B_DELWRI)) {
			numcleanpages -= btoc(bp->b_bufsize);
#ifdef DEBUG
			if (mincleanpages > numcleanpages)
				mincleanpages = numcleanpages;
#endif
		} else {
			numdirtypages -= btoc(bp->b_bufsize);
		}
	}
	TAILQ_REMOVE(dp, bp, b_freelist);
=======
	if (!ISSET(bp->b_flags, B_DELWRI)) {
		bcstats.numcleanpages -= atop(bp->b_bufsize);
	} else {
		bcstats.numdirtypages -= atop(bp->b_bufsize);
	}
	TAILQ_REMOVE(dp, bp, b_freelist);
	bcstats.freebufs--;
}

void
buf_put(struct buf *bp)
{
	splassert(IPL_BIO);

#ifdef DIAGNOSTIC
	if (bp->b_pobj != NULL)
		KASSERT(bp->b_bufsize > 0);
	if (ISSET(bp->b_flags, B_DELWRI))
		panic("buf_put: releasing dirty buffer");
	if (bp->b_freelist.tqe_next != NOLIST &&
	    bp->b_freelist.tqe_next != (void *)-1)
		panic("buf_put: still on the free list");
	if (bp->b_vnbufs.le_next != NOLIST &&
	    bp->b_vnbufs.le_next != (void *)-1)
		panic("buf_put: still on the vnode list");
	if (!LIST_EMPTY(&bp->b_dep))
		panic("buf_put: b_dep is not empty");
#endif

	LIST_REMOVE(bp, b_list);
	bcstats.numbufs--;
	if (backoffpages) {
		backoffpages -= atop(bp->b_bufsize);
		if (backoffpages < 0)
			backoffpages = 0;
	}

	if (buf_dealloc_mem(bp) != 0)
		return;
	pool_put(&bufpool, bp);
>>>>>>> origin/master
}

/*
 * Initialize buffers and hash links for buffers.
 */
void
bufinit(void)
{
<<<<<<< HEAD
	struct buf *bp;
=======
>>>>>>> origin/master
	struct bqueues *dp;
	int i;
	int base, residual;

	/* XXX - for now */
	bufhighpages = buflowpages = bufpages = bufcachepercent = bufkvm = 0;

	/*
	 * If MD code doesn't say otherwise, use 10% of kvm for mappings and
	 * 10% physmem for pages.
	 */
	if (bufcachepercent == 0)
		bufcachepercent = 10;
	if (bufpages == 0)
		bufpages = physmem * bufcachepercent / 100;

	bufhighpages = bufpages;

	/*
	 * set the base backoff level for the buffer cache to bufpages.
	 * we will not allow uvm to steal back more than this number of
	 * pages
	 */
	buflowpages = physmem * 10 / 100;

	/*
	 * set bufbackpages to 100 pages, or 10 percent of the low water mark
	 * if we don't have that many pages.
	 */

	bufbackpages = buflowpages * 10 / 100;
	if (bufbackpages > 100)
		bufbackpages = 100;

	if (bufkvm == 0)
		bufkvm = (VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS) / 10;

	/*
	 * Don't use more than twice the amount of bufpages for mappings.
	 * It's twice since we map things sparsely.
	 */
	if (bufkvm > bufpages * PAGE_SIZE)
		bufkvm = bufpages * PAGE_SIZE;
	/*
	 * Round bufkvm to MAXPHYS because we allocate chunks of va space
	 * in MAXPHYS chunks.
	 */
	bufkvm &= ~(MAXPHYS - 1);

	pool_init(&bufpool, sizeof(struct buf), 0, 0, 0, "bufpl", NULL);
	for (dp = bufqueues; dp < &bufqueues[BQUEUES]; dp++)
		TAILQ_INIT(dp);
<<<<<<< HEAD
	bufhashtbl = hashinit(nbuf, M_CACHE, M_WAITOK, &bufhash);
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		bp = &buf[i];
		bzero((char *)bp, sizeof *bp);
		bp->b_dev = NODEV;
		bp->b_vnbufs.le_next = NOLIST;
		bp->b_data = buffers + i * MAXBSIZE;
		LIST_INIT(&bp->b_dep);
		if (i < residual)
			bp->b_bufsize = (base + 1) * PAGE_SIZE;
		else
			bp->b_bufsize = base * PAGE_SIZE;
		bp->b_flags = B_INVAL;
		if (bp->b_bufsize) {
			dp = &bufqueues[BQ_CLEAN];
			numfreepages += btoc(bp->b_bufsize);
			numcleanpages += btoc(bp->b_bufsize);
		} else {
			dp = &bufqueues[BQ_EMPTY];
			numemptybufs++;
		}
		binsheadfree(bp, dp);
		binshash(bp, &invalhash);
	}

	hidirtypages = bufpages / 4;
	lodirtypages = hidirtypages / 2;

	/*
	 * Reserve 5% of bufpages for syncer's needs,
	 * but not more than 25% and if possible
	 * not less than 2 * MAXBSIZE. locleanpages
	 * value must be not too small, but probably
	 * there is no reason to set it to more than 1-2 MB.
	 */
	locleanpages = bufpages / 20;
	if (locleanpages < btoc(2 * MAXBSIZE))
		locleanpages = btoc(2 * MAXBSIZE);
	if (locleanpages > bufpages / 4)
		locleanpages = bufpages / 4;
	if (locleanpages > btoc(2 * 1024 * 1024))
		locleanpages = btoc(2 * 1024 * 1024);

#ifdef DEBUG
	mincleanpages = locleanpages;
#endif
=======

	/*
	 * hmm - bufkvm is an argument because it's static, while
	 * bufpages is global because it can change while running.
 	 */
	buf_mem_init(bufkvm);

	hidirtypages = (bufpages / 4) * 3;
	lodirtypages = bufpages / 2;

	/*
	 * When we hit 95% of pages being clean, we bring them down to
	 * 90% to have some slack.
	 */
	hicleanpages = bufpages - (bufpages / 20);
	locleanpages = bufpages - (bufpages / 10);

	maxcleanpages = locleanpages;
}

/*
 * Change cachepct
 */
void
bufadjust(int newbufpages)
{
	/*
	 * XXX - note, bufkvm was allocated once, based on 10% of physmem
	 * see above.
	 */
	struct buf *bp;
	int s;

	s = splbio();
	bufpages = newbufpages;

	hidirtypages = (bufpages / 4) * 3;
	lodirtypages = bufpages / 2;

	/*
	 * When we hit 95% of pages being clean, we bring them down to
	 * 90% to have some slack.
	 */
	hicleanpages = bufpages - (bufpages / 20);
	locleanpages = bufpages - (bufpages / 10);

	maxcleanpages = locleanpages;

	/*
	 * If we we have more buffers allocated than bufpages,
	 * free them up to get back down. this may possibly consume
	 * all our clean pages...
	 */
	while ((bp = TAILQ_FIRST(&bufqueues[BQ_CLEAN])) &&
	    (bcstats.numbufpages > bufpages)) {
		bremfree(bp);
		if (bp->b_vp) {
			RB_REMOVE(buf_rb_bufs,
			    &bp->b_vp->v_bufs_tree, bp);
			brelvp(bp);
		}
		buf_put(bp);
	}

	/*
	 * Wake up cleaner if we're getting low on pages. We might
	 * now have too much dirty, or have fallen below our low
	 * water mark on clean pages so we need to free more stuff
	 * up.
	 */
	if (bcstats.numdirtypages >= hidirtypages ||
	    bcstats.numcleanpages <= locleanpages)
		wakeup(&bd_req);

	/*
	 * if immediate action has not freed up enough goo for us
	 * to proceed - we tsleep and wait for the cleaner above
	 * to do it's work and get us reduced down to sanity.
	 */
	while (bcstats.numbufpages > bufpages) {
		tsleep(&needbuffer, PRIBIO, "needbuffer", 0);
	}
	splx(s);
}

/*
 * Make the buffer cache back off from cachepct.
 */
int
bufbackoff()
{
	/*
	 * Back off the amount of buffer cache pages. Called by the page
	 * daemon to consume buffer cache pages rather than swapping.
	 *
	 * On success, it frees N pages from the buffer cache, and sets
	 * a flag so that the next N allocations from buf_get will recycle
	 * a buffer rather than allocate a new one. It then returns 0 to the
	 * caller. 
	 *
	 * on failure, it could free no pages from the buffer cache, does
	 * nothing and returns -1 to the caller. 
	 */
	long d;

	if (bufpages <= buflowpages) 
		return(-1);

	if (bufpages - bufbackpages >= buflowpages)
		d = bufbackpages;
	else
		d = bufpages - buflowpages;
	backoffpages = bufbackpages;
	bufadjust(bufpages - d);
	backoffpages = bufbackpages;
	return(0);
>>>>>>> origin/master
}

static __inline struct buf *
bio_doread(struct vnode *vp, daddr64_t blkno, int size, int async)
{
	struct buf *bp;
	struct proc *p = (curproc != NULL ? curproc : &proc0);	/* XXX */

	bp = getblk(vp, blkno, size, 0, 0);

	/*
	 * If buffer does not have valid data, start a read.
	 * Note that if buffer is B_INVAL, getblk() won't return it.
	 * Therefore, it's valid if its I/O has completed or been delayed.
	 */
	if (!ISSET(bp->b_flags, (B_DONE | B_DELWRI))) {
		SET(bp->b_flags, B_READ | async);
		bcstats.pendingreads++;
		bcstats.numreads++;
		VOP_STRATEGY(bp);
		/* Pay for the read. */
		p->p_stats->p_ru.ru_inblock++;		/* XXX */
	} else if (async) {
		brelse(bp);
	}

	return (bp);
}

/*
 * Read a disk block.
 * This algorithm described in Bach (p.54).
 */
int
bread(struct vnode *vp, daddr64_t blkno, int size, struct ucred *cred,
    struct buf **bpp)
{
	struct buf *bp;

	/* Get buffer for block. */
	bp = *bpp = bio_doread(vp, blkno, size, 0);

	/* Wait for the read to complete, and return result. */
	return (biowait(bp));
}

/*
 * Read-ahead multiple disk blocks. The first is sync, the rest async.
 * Trivial modification to the breada algorithm presented in Bach (p.55).
 */
int
breadn(struct vnode *vp, daddr64_t blkno, int size, daddr64_t rablks[],
    int rasizes[], int nrablks, struct ucred *cred, struct buf **bpp)
{
	struct buf *bp;
	int i;

	bp = *bpp = bio_doread(vp, blkno, size, 0);

	/*
	 * For each of the read-ahead blocks, start a read, if necessary.
	 */
	for (i = 0; i < nrablks; i++) {
		/* If it's in the cache, just go on to next one. */
		if (incore(vp, rablks[i]))
			continue;

		/* Get a buffer for the read-ahead block */
		(void) bio_doread(vp, rablks[i], rasizes[i], B_ASYNC);
	}

	/* Otherwise, we had to start a read for it; wait until it's valid. */
	return (biowait(bp));
}

/*
<<<<<<< HEAD
=======
 * Called from interrupt context.
 */
void
bread_cluster_callback(struct buf *bp)
{
	struct buf **xbpp = bp->b_saveaddr;
	int i;

	if (xbpp[1] != NULL) {
		size_t newsize = xbpp[1]->b_bufsize;

		/*
		 * Shrink this buffer to only cover its part of the total I/O.
		 */
		buf_shrink_mem(bp, newsize);
		bp->b_bcount = newsize;
	}

	for (i = 1; xbpp[i] != 0; i++) {
		if (ISSET(bp->b_flags, B_ERROR))
			SET(xbpp[i]->b_flags, B_INVAL | B_ERROR);
		biodone(xbpp[i]);
	}

	free(xbpp, M_TEMP);

	if (ISSET(bp->b_flags, B_ASYNC)) {
		brelse(bp);
	} else {
		CLR(bp->b_flags, B_WANTED);
		wakeup(bp);
	}
}

int
bread_cluster(struct vnode *vp, daddr64_t blkno, int size, struct buf **rbpp)
{
	struct buf *bp, **xbpp;
	int howmany, maxra, i, inc;
	daddr64_t sblkno;

	*rbpp = bio_doread(vp, blkno, size, 0);

	if (size != round_page(size))
		goto out;

	if (VOP_BMAP(vp, blkno + 1, NULL, &sblkno, &maxra))
		goto out;

	maxra++; 
	if (sblkno == -1 || maxra < 2)
		goto out;

	howmany = MAXPHYS / size;
	if (howmany > maxra)
		howmany = maxra;

	xbpp = malloc((howmany + 1) * sizeof(struct buf *), M_TEMP, M_NOWAIT);
	if (xbpp == NULL)
		goto out;

	for (i = howmany - 1; i >= 0; i--) {
		size_t sz;

		/*
		 * First buffer allocates big enough size to cover what
		 * all the other buffers need.
		 */
		sz = i == 0 ? howmany * size : 0;

		xbpp[i] = buf_get(vp, blkno + i + 1, sz);
		if (xbpp[i] == NULL) {
			for (++i; i < howmany; i++) {
				SET(xbpp[i]->b_flags, B_INVAL);
				brelse(xbpp[i]);
			}
			free(xbpp, M_TEMP);
			goto out;
		}
	}

	bp = xbpp[0];

	xbpp[howmany] = 0;

	inc = btodb(size);

	for (i = 1; i < howmany; i++) {
		bcstats.pendingreads++;
		bcstats.numreads++;
		SET(xbpp[i]->b_flags, B_READ | B_ASYNC);
		xbpp[i]->b_blkno = sblkno + (i * inc);
		xbpp[i]->b_bufsize = xbpp[i]->b_bcount = size;
		xbpp[i]->b_data = NULL;
		xbpp[i]->b_pobj = bp->b_pobj;
		xbpp[i]->b_poffs = bp->b_poffs + (i * size);
	}

	KASSERT(bp->b_lblkno == blkno + 1);
	KASSERT(bp->b_vp == vp);

	bp->b_blkno = sblkno;
	SET(bp->b_flags, B_READ | B_ASYNC | B_CALL);

	bp->b_saveaddr = (void *)xbpp;
	bp->b_iodone = bread_cluster_callback;

	bcstats.pendingreads++;
	bcstats.numreads++;
	VOP_STRATEGY(bp);
	curproc->p_stats->p_ru.ru_inblock++;

out:
	return (biowait(*rbpp));
}

/*
>>>>>>> origin/master
 * Block write.  Described in Bach (p.56)
 */
int
bwrite(struct buf *bp)
{
	int rv, async, wasdelayed, s;
	struct vnode *vp;
	struct mount *mp;
	struct proc *p = (curproc != NULL ? curproc : &proc0);	/* XXX */

	vp = bp->b_vp;
	if (vp != NULL)
		mp = vp->v_type == VBLK? vp->v_specmountpoint : vp->v_mount;
	else
		mp = NULL;

	/*
	 * Remember buffer type, to switch on it later.  If the write was
	 * synchronous, but the file system was mounted with MNT_ASYNC,
	 * convert it to a delayed write.
	 * XXX note that this relies on delayed tape writes being converted
	 * to async, not sync writes (which is safe, but ugly).
	 */
	async = ISSET(bp->b_flags, B_ASYNC);
	if (!async && mp && ISSET(mp->mnt_flag, MNT_ASYNC)) {
		bdwrite(bp);
		return (0);
	}

	/*
	 * Collect statistics on synchronous and asynchronous writes.
	 * Writes to block devices are charged to their associated
	 * filesystem (if any).
	 */
	if (mp != NULL) {
		if (async)
			mp->mnt_stat.f_asyncwrites++;
		else
			mp->mnt_stat.f_syncwrites++;
	}
	bcstats.pendingwrites++;
	bcstats.numwrites++;

	wasdelayed = ISSET(bp->b_flags, B_DELWRI);
	CLR(bp->b_flags, (B_READ | B_DONE | B_ERROR | B_DELWRI));

	s = splbio();

	/*
	 * If not synchronous, pay for the I/O operation and make
	 * sure the buf is on the correct vnode queue.  We have
	 * to do this now, because if we don't, the vnode may not
	 * be properly notified that its I/O has completed.
	 */
	if (wasdelayed) {
		reassignbuf(bp);
	} else
		p->p_stats->p_ru.ru_oublock++;
	

	/* Initiate disk write.  Make sure the appropriate party is charged. */
	bp->b_vp->v_numoutput++;
	splx(s);
	SET(bp->b_flags, B_WRITEINPROG);
	VOP_STRATEGY(bp);

	if (async)
		return (0);

	/*
	 * If I/O was synchronous, wait for it to complete.
	 */
	rv = biowait(bp);

	/* Release the buffer. */
	brelse(bp);

	return (rv);
}


/*
 * Delayed write.
 *
 * The buffer is marked dirty, but is not queued for I/O.
 * This routine should be used when the buffer is expected
 * to be modified again soon, typically a small write that
 * partially fills a buffer.
 *
 * NB: magnetic tapes cannot be delayed; they must be
 * written in the order that the writes are requested.
 *
 * Described in Leffler, et al. (pp. 208-213).
 */
void
bdwrite(struct buf *bp)
{
	int s;
	struct proc *p = (curproc != NULL ? curproc : &proc0);	/* XXX */

	/*
	 * If the block hasn't been seen before:
	 *	(1) Mark it as having been seen,
	 *	(2) Charge for the write.
	 *	(3) Make sure it's on its vnode's correct block list,
	 *	(4) If a buffer is rewritten, move it to end of dirty list
	 */
	if (!ISSET(bp->b_flags, B_DELWRI)) {
		SET(bp->b_flags, B_DELWRI);
<<<<<<< HEAD
		bp->b_synctime = time_second + 35;
		s = splbio();
		reassignbuf(bp);
		splx(s);
		p->p_stats->p_ru.ru_oublock++;	/* XXX */
	} else {
		/*
		 * see if this buffer has slacked through the syncer
		 * and enforce an async write upon it.
		 */
		if (bp->b_synctime < time_second) {
			bawrite(bp);
			return;
		}
=======
		s = splbio();
		reassignbuf(bp);
		splx(s);
		curproc->p_stats->p_ru.ru_oublock++;	/* XXX */
>>>>>>> origin/master
	}

	/* If this is a tape block, write the block now. */
	if (major(bp->b_dev) < nblkdev &&
	    bdevsw[major(bp->b_dev)].d_type == D_TAPE) {
		bawrite(bp);
		return;
	}

	/* Otherwise, the "write" is done, so mark and release the buffer. */
	CLR(bp->b_flags, B_NEEDCOMMIT|B_DONE);
	brelse(bp);
}

/*
 * Asynchronous block write; just an asynchronous bwrite().
 */
void
bawrite(struct buf *bp)
{

	SET(bp->b_flags, B_ASYNC);
	VOP_BWRITE(bp);
}

/*
 * Must be called at splbio()
 */
void
buf_dirty(struct buf *bp)
{
	splassert(IPL_BIO);

#ifdef DIAGNOSTIC
	if (!ISSET(bp->b_flags, B_BUSY))
		panic("Trying to dirty buffer on freelist!");
#endif

	if (ISSET(bp->b_flags, B_DELWRI) == 0) {
		SET(bp->b_flags, B_DELWRI);
<<<<<<< HEAD
		bp->b_synctime = time_second + 35;
=======
>>>>>>> origin/master
		reassignbuf(bp);
	}
}

/*
 * Must be called at splbio()
 */
void
buf_undirty(struct buf *bp)
{
	splassert(IPL_BIO);

#ifdef DIAGNOSTIC
	if (!ISSET(bp->b_flags, B_BUSY))
		panic("Trying to undirty buffer on freelist!");
#endif
	if (ISSET(bp->b_flags, B_DELWRI)) {
		CLR(bp->b_flags, B_DELWRI);
		reassignbuf(bp);
	}
}

/*
 * Release a buffer on to the free lists.
 * Described in Bach (p. 46).
 */
void
brelse(struct buf *bp)
{
	struct bqueues *bufq;
	int s;

<<<<<<< HEAD

	/* Wake up syncer and cleaner processes waiting for buffers */
	if (nobuffers) {
		wakeup(&nobuffers);
		nobuffers = 0;
	}

	/* Wake up any processes waiting for any buffer to become free. */
	if (needbuffer && (numcleanpages > locleanpages)) {
		needbuffer--;
		wakeup_one(&needbuffer);
	}

	/* Wake up any processes waiting for _this_ buffer to become free. */
	if (ISSET(bp->b_flags, B_WANTED)) {
		CLR(bp->b_flags, B_WANTED|B_AGE);
		wakeup(bp);
	}

	/* Block disk interrupts. */
=======
>>>>>>> origin/master
	s = splbio();

	/*
	 * Determine which queue the buffer should be on, then put it there.
	 */

	/* If it's not cacheable, or an error, mark it invalid. */
	if (ISSET(bp->b_flags, (B_NOCACHE|B_ERROR)))
		SET(bp->b_flags, B_INVAL);

<<<<<<< HEAD
	if ((bp->b_bufsize <= 0) || ISSET(bp->b_flags, B_INVAL)) {
=======
	if (ISSET(bp->b_flags, B_INVAL)) {
>>>>>>> origin/master
		/*
		 * If it's invalid or empty, dissociate it from its vnode
		 * and put on the head of the appropriate queue.
		 */
		if (LIST_FIRST(&bp->b_dep) != NULL)
			buf_deallocate(bp);

<<<<<<< HEAD
		if (bp->b_vp)
=======
		if (ISSET(bp->b_flags, B_DELWRI)) {
			CLR(bp->b_flags, B_DELWRI);
		}

		if (bp->b_vp) {
			RB_REMOVE(buf_rb_bufs, &bp->b_vp->v_bufs_tree,
			    bp);
>>>>>>> origin/master
			brelvp(bp);
		}
		bp->b_vp = NULL;

<<<<<<< HEAD
		CLR(bp->b_flags, B_DELWRI|B_DONE);
		if (bp->b_bufsize <= 0) {
			/* no data */
			bufq = &bufqueues[BQ_EMPTY];
			numemptybufs++;
		} else {
			/* invalid data */
			bufq = &bufqueues[BQ_CLEAN];
			numfreepages += btoc(bp->b_bufsize);
			numcleanpages += btoc(bp->b_bufsize);
		}
		binsheadfree(bp, bufq);
=======
		/*
		 * If the buffer has no associated data, place it back in the
		 * pool.
		 */
		if (bp->b_data == NULL && bp->b_pobj == NULL) {
			/*
			 * Wake up any processes waiting for _this_ buffer to
			 * become free. They are not allowed to grab it
			 * since it will be freed. But the only sleeper is
			 * getblk and it's restarting the operation after
			 * sleep.
			 */
			if (ISSET(bp->b_flags, B_WANTED)) {
				CLR(bp->b_flags, B_WANTED);
				wakeup(bp);
			}
			if (bp->b_vp != NULL)
				RB_REMOVE(buf_rb_bufs,
				    &bp->b_vp->v_bufs_tree, bp);
			buf_put(bp);
			splx(s);
			return;
		}

		bcstats.numcleanpages += atop(bp->b_bufsize);
		if (maxcleanpages < bcstats.numcleanpages)
			maxcleanpages = bcstats.numcleanpages;
		binsheadfree(bp, &bufqueues[BQ_CLEAN]);
>>>>>>> origin/master
	} else {
		/*
		 * It has valid data.  Put it on the end of the appropriate
		 * queue, so that it'll stick around for as long as possible.
		 */
<<<<<<< HEAD
		numfreepages += btoc(bp->b_bufsize);

		if (!ISSET(bp->b_flags, B_DELWRI)) {
			numcleanpages += btoc(bp->b_bufsize);
			bufq = &bufqueues[BQ_CLEAN];
		} else {
			numdirtypages += btoc(bp->b_bufsize);
=======

		if (!ISSET(bp->b_flags, B_DELWRI)) {
			bcstats.numcleanpages += atop(bp->b_bufsize);
			if (maxcleanpages < bcstats.numcleanpages)
				maxcleanpages = bcstats.numcleanpages;
			bufq = &bufqueues[BQ_CLEAN];
		} else {
			bcstats.numdirtypages += atop(bp->b_bufsize);
>>>>>>> origin/master
			bufq = &bufqueues[BQ_DIRTY];
		}
		if (ISSET(bp->b_flags, B_AGE))
			binsheadfree(bp, bufq);
		else
			binstailfree(bp, bufq);
	}

	/* Unlock the buffer. */
<<<<<<< HEAD
	CLR(bp->b_flags, (B_AGE | B_ASYNC | B_BUSY | B_NOCACHE | B_CACHE | B_DEFERRED));
=======
	bcstats.freebufs++;
	CLR(bp->b_flags, (B_AGE | B_ASYNC | B_NOCACHE | B_DEFERRED));
	buf_release(bp);

	/* Wake up any processes waiting for any buffer to become free. */
	if (needbuffer) {
		needbuffer--;
		wakeup(&needbuffer);
	}

	/* Wake up any processes waiting for _this_ buffer to become free. */
	if (ISSET(bp->b_flags, B_WANTED)) {
		CLR(bp->b_flags, B_WANTED);
		wakeup(bp);
	}
>>>>>>> origin/master

	splx(s);
}

/*
 * Determine if a block is in the cache. Just look on what would be its hash
 * chain. If it's there, return a pointer to it, unless it's marked invalid.
 */
struct buf *
incore(struct vnode *vp, daddr64_t blkno)
{
	struct buf *bp;
	struct buf b;
	int s;

	s = splbio();

	/* Search buf lookup tree */
	b.b_lblkno = blkno;
	bp = RB_FIND(buf_rb_bufs, &vp->v_bufs_tree, &b);
	if (bp != NULL && ISSET(bp->b_flags, B_INVAL))
		bp = NULL;

	splx(s);
	return (bp);
}

/*
 * Get a block of requested size that is associated with
 * a given vnode and block offset. If it is found in the
 * block cache, mark it as having been found, make it busy
 * and return it. Otherwise, return an empty block of the
 * correct size. It is up to the caller to ensure that the
 * cached blocks be of the correct size.
 */
struct buf *
getblk(struct vnode *vp, daddr64_t blkno, int size, int slpflag, int slptimeo)
{
<<<<<<< HEAD
	struct bufhashhdr *bh;
	struct buf *bp;
=======
	struct buf *bp;
	struct buf b;
>>>>>>> origin/master
	int s, error;

	/*
	 * XXX
	 * The following is an inlined version of 'incore()', but with
	 * the 'invalid' test moved to after the 'busy' test.  It's
	 * necessary because there are some cases in which the NFS
	 * code sets B_INVAL prior to writing data to the server, but
	 * in which the buffers actually contain valid data.  In this
	 * case, we can't allow the system to allocate a new buffer for
	 * the block until the write is finished.
	 */
start:
	s = splbio();
	b.b_lblkno = blkno;
	bp = RB_FIND(buf_rb_bufs, &vp->v_bufs_tree, &b);
	if (bp != NULL) {
		if (ISSET(bp->b_flags, B_BUSY)) {
			SET(bp->b_flags, B_WANTED);
			error = ltsleep(bp, slpflag | (PRIBIO + 1) | PNORELOCK,
				"getblk", slptimeo, &vp->v_interlock);
			splx(s);
			if (error)
				return (NULL);
			goto start;
		}

		if (!ISSET(bp->b_flags, B_INVAL)) {
<<<<<<< HEAD
#ifdef DIAGNOSTIC
			if (ISSET(bp->b_flags, B_DONE|B_DELWRI) &&
			    bp->b_bcount < size && vp->v_type != VBLK)
				panic("getblk: block size invariant failed");
#endif
			SET(bp->b_flags, B_BUSY);
=======
			bcstats.cachehits++;
			SET(bp->b_flags, B_CACHE);
>>>>>>> origin/master
			bremfree(bp);
			buf_acquire(bp);
			splx(s);
			return (bp);
		}
	}
<<<<<<< HEAD

	if (bp == NULL) {
		bp = getnewbuf(slpflag, slptimeo, &error);
		if (bp == NULL) {
			if (error == ERESTART || error == EINTR)
				return (NULL);
			goto start;
		}

		binshash(bp, bh);
		bp->b_blkno = bp->b_lblkno = blkno;
		s = splbio();
		bgetvp(vp, bp);
		splx(s);
	}
	allocbuf(bp, size);
=======
	splx(s);

	if ((bp = buf_get(vp, blkno, size)) == NULL)
		goto start;
>>>>>>> origin/master

	return (bp);
}

/*
 * Get an empty, disassociated buffer of given size.
 */
struct buf *
geteblk(int size)
{
	struct buf *bp;

<<<<<<< HEAD
	while ((bp = getnewbuf(0, 0, NULL)) == NULL)
		;
	SET(bp->b_flags, B_INVAL);
	binshash(bp, &invalhash);
	allocbuf(bp, size);
=======
	while ((bp = buf_get(NULL, 0, size)) == NULL)
		;
>>>>>>> origin/master

	return (bp);
}

/*
<<<<<<< HEAD
 * Expand or contract the actual memory allocated to a buffer.
 *
 * If the buffer shrinks, data is lost, so it's up to the
 * caller to have written it out *first*; this routine will not
 * start a write.  If the buffer grows, it's the caller's
 * responsibility to fill out the buffer's additional contents.
 */
void
allocbuf(struct buf *bp, int size)
{
	struct buf	*nbp;
	vsize_t		desired_size;
	int		s;

	desired_size = round_page(size);
	if (desired_size > MAXBSIZE)
		panic("allocbuf: buffer larger than MAXBSIZE requested");

	if (bp->b_bufsize == desired_size)
		goto out;

	/*
	 * If the buffer is smaller than the desired size, we need to snarf
	 * it from other buffers.  Get buffers (via getnewbuf()), and
	 * steal their pages.
	 */
	while (bp->b_bufsize < desired_size) {
		int amt;

		/* find a buffer */
		while ((nbp = getnewbuf(0, 0, NULL)) == NULL)
			;
 		SET(nbp->b_flags, B_INVAL);
		binshash(nbp, &invalhash);

		/* and steal its pages, up to the amount we need */
		amt = MIN(nbp->b_bufsize, (desired_size - bp->b_bufsize));
		pagemove((nbp->b_data + nbp->b_bufsize - amt),
			 bp->b_data + bp->b_bufsize, amt);
		bp->b_bufsize += amt;
		nbp->b_bufsize -= amt;

		/* reduce transfer count if we stole some data */
		if (nbp->b_bcount > nbp->b_bufsize)
			nbp->b_bcount = nbp->b_bufsize;

#ifdef DIAGNOSTIC
		if (nbp->b_bufsize < 0)
			panic("allocbuf: negative bufsize");
#endif

		brelse(nbp);
	}

	/*
	 * If we want a buffer smaller than the current size,
	 * shrink this buffer.  Grab a buf head from the EMPTY queue,
	 * move a page onto it, and put it on front of the AGE queue.
	 * If there are no free buffer headers, leave the buffer alone.
	 */
	if (bp->b_bufsize > desired_size) {
		s = splbio();
		if ((nbp = TAILQ_FIRST(&bufqueues[BQ_EMPTY])) == NULL) {
			/* No free buffer head */
			splx(s);
			goto out;
		}
		bremfree(nbp);
		SET(nbp->b_flags, B_BUSY);
		splx(s);

		/* move the page to it and note this change */
		pagemove(bp->b_data + desired_size,
		    nbp->b_data, bp->b_bufsize - desired_size);
		nbp->b_bufsize = bp->b_bufsize - desired_size;
		bp->b_bufsize = desired_size;
		nbp->b_bcount = 0;
		SET(nbp->b_flags, B_INVAL);

		/* release the newly-filled buffer and leave */
		brelse(nbp);
	}

out:
	bp->b_bcount = size;
}

/*
 * Find a buffer which is available for use.
 */
struct buf *
getnewbuf(int slpflag, int slptimeo, int *ep)
=======
 * Allocate a buffer.
 */
struct buf *
buf_get(struct vnode *vp, daddr64_t blkno, size_t size)
>>>>>>> origin/master
{
	static int gcount = 0;
	struct buf *bp;
<<<<<<< HEAD
	int s, error;
=======
	int poolwait = size == 0 ? PR_NOWAIT : PR_WAITOK;
	int npages;
	int s;
>>>>>>> origin/master

	/*
<<<<<<< HEAD
	 * Wake up cleaner if we're getting low on buffers.
	 */
	if (numdirtypages >= hidirtypages)
		wakeup(&bd_req);

	if ((numcleanpages <= locleanpages) &&
	    curproc != syncerproc && curproc != cleanerproc) {
		needbuffer++;
		error = tsleep(&needbuffer, slpflag | (PRIBIO + 1),
		    "getnewbuf", slptimeo);
		splx(s);
		if (ep != NULL)
			*ep = error;
		return (NULL);
	}

	bp = TAILQ_FIRST(&bufqueues[BQ_CLEAN]);
	if (bp == NULL) {
		/* wait for a free buffer of any kind */
		nobuffers = 1;
		error = tsleep(&nobuffers, slpflag | (PRIBIO - 3),
		    "getnewbuf", slptimeo);
		splx(s);
		if (ep != NULL)
			*ep = error;
		return (NULL);
	}

	bremfree(bp);

	/* Buffer is no longer on free lists. */
	SET(bp->b_flags, B_BUSY);
=======
	 * if we were previously backed off, slowly climb back up
	 * to the high water mark again.
	 */
	if ((backoffpages == 0) && (bufpages < bufhighpages)) {
		if ( gcount == 0 )  {
			bufadjust(bufpages + bufbackpages);
			gcount += bufbackpages;
		} else
			gcount--;
	}

	s = splbio();
	if (size) {
		/*
		 * Wake up cleaner if we're getting low on pages.
		 */
		if (bcstats.numdirtypages >= hidirtypages ||
		    bcstats.numcleanpages <= locleanpages)
			wakeup(&bd_req);

		/*
		 * If we're above the high water mark for clean pages,
		 * free down to the low water mark.
		 */
		if (bcstats.numcleanpages > hicleanpages) {
			while (bcstats.numcleanpages > locleanpages) {
				bp = TAILQ_FIRST(&bufqueues[BQ_CLEAN]);
				bremfree(bp);
				if (bp->b_vp) {
					RB_REMOVE(buf_rb_bufs,
					    &bp->b_vp->v_bufs_tree, bp);
					brelvp(bp);
				}
				buf_put(bp);
			}
		}

		npages = atop(round_page(size));

		/*
		 * Free some buffers until we have enough space.
		 */
		while ((bcstats.numbufpages + npages > bufpages)
		    || backoffpages) {
			int freemax = 5;
			int i = freemax;
			while ((bp = TAILQ_FIRST(&bufqueues[BQ_CLEAN])) && i--) {
				bremfree(bp);
				if (bp->b_vp) {
					RB_REMOVE(buf_rb_bufs,
					    &bp->b_vp->v_bufs_tree, bp);
					brelvp(bp);
				}
				buf_put(bp);
			}
			if (freemax == i &&
			    (bcstats.numbufpages + npages > bufpages)) {
				needbuffer++;
				tsleep(&needbuffer, PRIBIO, "needbuffer", 0);
				splx(s);
				return (NULL);
			}
		}
	}

	bp = pool_get(&bufpool, poolwait|PR_ZERO);
>>>>>>> origin/master

	if (bp == NULL) {
		splx(s);
		return (NULL);
	}

	bp->b_freelist.tqe_next = NOLIST;
	bp->b_synctime = time_uptime + 300;
	bp->b_dev = NODEV;
	LIST_INIT(&bp->b_dep);
	bp->b_bcount = size;

	buf_acquire_unmapped(bp);

	if (vp != NULL) {
		/*
		 * We insert the buffer into the hash with B_BUSY set
		 * while we allocate pages for it. This way any getblk
		 * that happens while we allocate pages will wait for
		 * this buffer instead of starting its own guf_get.
		 *
		 * But first, we check if someone beat us to it.
		 */
		if (incore(vp, blkno)) {
			pool_put(&bufpool, bp);
			splx(s);
			return (NULL);
		}

<<<<<<< HEAD
	/* clear out various other fields */
	bp->b_flags = B_BUSY;
	bp->b_dev = NODEV;
	bp->b_blkno = bp->b_lblkno = 0;
	bp->b_iodone = NULL;
	bp->b_error = 0;
	bp->b_resid = 0;
	bp->b_bcount = 0;
	bp->b_dirtyoff = bp->b_dirtyend = 0;
	bp->b_validoff = bp->b_validend = 0;
=======
		bp->b_blkno = bp->b_lblkno = blkno;
		bgetvp(vp, bp);
		if (RB_INSERT(buf_rb_bufs, &vp->v_bufs_tree, bp))
			panic("buf_get: dup lblk vp %p bp %p", vp, bp);
	} else {
		bp->b_vnbufs.le_next = NOLIST;
		SET(bp->b_flags, B_INVAL);
		bp->b_vp = NULL;
	}

	LIST_INSERT_HEAD(&bufhead, bp, b_list);
	bcstats.numbufs++;

	if (size) {
		buf_alloc_pages(bp, round_page(size));
		buf_map(bp);
	}

	splx(s);
>>>>>>> origin/master

	return (bp);
}

/*
 * Buffer cleaning daemon.
 */
void
buf_daemon(struct proc *p)
{
	int s;
	struct buf *bp;
	struct timeval starttime, timediff;

	cleanerproc = curproc;

	for (;;) {
<<<<<<< HEAD
		if (numdirtypages < hidirtypages) {
=======
		if (bcstats.numdirtypages < hidirtypages)
>>>>>>> origin/master
			tsleep(&bd_req, PRIBIO - 7, "cleaner", 0);
		}

		getmicrouptime(&starttime);
		s = splbio();
		while ((bp = TAILQ_FIRST(&bufqueues[BQ_DIRTY]))) {
			struct timeval tv;

<<<<<<< HEAD
=======
			if (bcstats.numdirtypages < lodirtypages)
				break;

>>>>>>> origin/master
			bremfree(bp);
			buf_acquire(bp);
			splx(s);

			if (ISSET(bp->b_flags, B_INVAL)) {
				brelse(bp);
				s = splbio();
				continue;
			}
#ifdef DIAGNOSTIC
			if (!ISSET(bp->b_flags, B_DELWRI))
				panic("Clean buffer on BQ_DIRTY");
#endif
			if (LIST_FIRST(&bp->b_dep) != NULL &&
			    !ISSET(bp->b_flags, B_DEFERRED) &&
			    buf_countdeps(bp, 0, 0)) {
				SET(bp->b_flags, B_DEFERRED);
				s = splbio();
<<<<<<< HEAD
				numfreepages += btoc(bp->b_bufsize);
				numdirtypages += btoc(bp->b_bufsize);
=======
				bcstats.numdirtypages += atop(bp->b_bufsize);
>>>>>>> origin/master
				binstailfree(bp, &bufqueues[BQ_DIRTY]);
				bcstats.freebufs++;
				buf_release(bp);
				continue;
			}

			bawrite(bp);

			if (numdirtypages < lodirtypages)
				break;
			/* Never allow processing to run for more than 1 sec */
			getmicrouptime(&tv);
			timersub(&tv, &starttime, &timediff);
			if (timediff.tv_sec)
				break;

			s = splbio();
		}
	}
}

/*
 * Wait for operations on the buffer to complete.
 * When they do, extract and return the I/O's error value.
 */
int
biowait(struct buf *bp)
{
	int s;

	KASSERT(!(bp->b_flags & B_ASYNC));

	s = splbio();
	while (!ISSET(bp->b_flags, B_DONE|B_DELWRI))
		tsleep(bp, PRIBIO + 1, "biowait", 0);
	splx(s);

	/* check for interruption of I/O (e.g. via NFS), then errors. */
	if (ISSET(bp->b_flags, B_EINTR)) {
		CLR(bp->b_flags, B_EINTR);
		return (EINTR);
	}

	if (ISSET(bp->b_flags, B_ERROR))
		return (bp->b_error ? bp->b_error : EIO);
	else
		return (0);
}

/*
 * Mark I/O complete on a buffer.
 *
 * If a callback has been requested, e.g. the pageout
 * daemon, do so. Otherwise, awaken waiting processes.
 *
 * [ Leffler, et al., says on p.247:
 *	"This routine wakes up the blocked process, frees the buffer
 *	for an asynchronous write, or, for a request by the pagedaemon
 *	process, invokes a procedure specified in the buffer structure" ]
 *
 * In real life, the pagedaemon (or other system processes) wants
 * to do async stuff to, and doesn't want the buffer brelse()'d.
 * (for swap pager, that puts swap buffers on the free lists (!!!),
 * for the vn device, that puts malloc'd buffers on the free lists!)
 *
 * Must be called at splbio().
 */
void
biodone(struct buf *bp)
{
	splassert(IPL_BIO);

	if (ISSET(bp->b_flags, B_DONE))
		panic("biodone already");
	SET(bp->b_flags, B_DONE);		/* note that it's done */

	if (bp->b_bq)
		bufq_done(bp->b_bq, bp);

	if (LIST_FIRST(&bp->b_dep) != NULL)
		buf_complete(bp);

	if (!ISSET(bp->b_flags, B_READ)) {
		CLR(bp->b_flags, B_WRITEINPROG);
		vwakeup(bp->b_vp);
	}
	if (bcstats.numbufs &&
	    (!(ISSET(bp->b_flags, B_RAW) || ISSET(bp->b_flags, B_PHYS)))) {
		if (!ISSET(bp->b_flags, B_READ))
			bcstats.pendingwrites--;
		else
			bcstats.pendingreads--;
	}
	if (ISSET(bp->b_flags, B_CALL)) {	/* if necessary, call out */
		CLR(bp->b_flags, B_CALL);	/* but note callout done */
		(*bp->b_iodone)(bp);
	} else {
		if (ISSET(bp->b_flags, B_ASYNC)) {/* if async, release it */
			brelse(bp);
		} else {			/* or just wakeup the buffer */
			CLR(bp->b_flags, B_WANTED);
			wakeup(bp);
		}
	}
}

<<<<<<< HEAD
#ifdef DEBUG
=======
#ifdef DDB
void	bcstats_print(int (*)(const char *, ...));
>>>>>>> origin/master
/*
 * bcstats_print: ddb hook to print interesting buffer cache counters
 */
void
bcstats_print(int (*pr)(const char *, ...))
{
<<<<<<< HEAD
	int s, i, j, count;
	register struct buf *bp;
	register struct bqueues *dp;
	int counts[MAXBSIZE/PAGE_SIZE+1];
	int totals[BQUEUES];
	long ptotals[BQUEUES];
	long pages;
	static char *bname[BQUEUES] = { "CLEAN", "DIRTY", "EMPTY" };

	s = splbio();
	for (dp = bufqueues, i = 0; dp < &bufqueues[BQUEUES]; dp++, i++) {
		count = 0;
		pages = 0;
		for (j = 0; j <= MAXBSIZE/PAGE_SIZE; j++)
			counts[j] = 0;
		TAILQ_FOREACH(bp, dp, b_freelist) {
			counts[bp->b_bufsize/PAGE_SIZE]++;
			count++;
			pages += btoc(bp->b_bufsize);
		}
		totals[i] = count;
		ptotals[i] = pages;
		printf("%s: total-%d(%d pages)", bname[i], count, pages);
		for (j = 0; j <= MAXBSIZE/PAGE_SIZE; j++)
			if (counts[j] != 0)
				printf(", %d-%d", j * PAGE_SIZE, counts[j]);
		printf("\n");
	}
	if (totals[BQ_EMPTY] != numemptybufs)
		printf("numemptybufs counter wrong: %d != %d\n",
			numemptybufs, totals[BQ_EMPTY]);
	if ((ptotals[BQ_CLEAN] + ptotals[BQ_DIRTY]) != numfreepages)
		printf("numfreepages counter wrong: %ld != %ld\n",
			numfreepages, ptotals[BQ_CLEAN] + ptotals[BQ_DIRTY]);
	if (ptotals[BQ_CLEAN] != numcleanpages)
		printf("numcleanpages counter wrong: %ld != %ld\n",
			numcleanpages, ptotals[BQ_CLEAN]);
	else
		printf("numcleanpages: %ld\n", numcleanpages);
	if (numdirtypages != ptotals[BQ_DIRTY])
		printf("numdirtypages counter wrong: %ld != %ld\n",
			numdirtypages, ptotals[BQ_DIRTY]);
	else
		printf("numdirtypages: %ld\n", numdirtypages);

	printf("syncer eating up to %ld pages from %ld reserved\n",
			locleanpages - mincleanpages, locleanpages);
	splx(s);
=======
	(*pr)("Current Buffer Cache status:\n");
	(*pr)("numbufs %lld, freebufs %lld\n",
	    bcstats.numbufs, bcstats.freebufs);
    	(*pr)("bufpages %lld, freepages %lld, dirtypages %lld\n",
	    bcstats.numbufpages, bcstats.numfreepages, bcstats.numdirtypages);
	(*pr)("pendingreads %lld, pendingwrites %lld\n",
	    bcstats.pendingreads, bcstats.pendingwrites);
>>>>>>> origin/master
}
#endif
