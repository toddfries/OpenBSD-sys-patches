/*-
 * Copyright (c) 1994, David Greenman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
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

/*
 * clist support routines
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/kern/subr_clist.c,v 1.48 2008/09/21 18:12:18 ed Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/clist.h>

static void clist_init(void *);
SYSINIT(clist, SI_SUB_CLIST, SI_ORDER_FIRST, clist_init, NULL);

static MALLOC_DEFINE(M_CLIST, "clist", "clist queue blocks");

static struct cblock *cfreelist = 0;
int cfreecount = 0;
static int cslushcount;
static int ctotcount;

#ifndef INITIAL_CBLOCKS
#define	INITIAL_CBLOCKS 50
#endif

static struct cblock *cblock_alloc(void);
static void cblock_alloc_cblocks(int number);
static void cblock_free(struct cblock *cblockp);
static void cblock_free_cblocks(int number);

#include "opt_ddb.h"
#ifdef DDB
#include <ddb/ddb.h>

DB_SHOW_COMMAND(cbstat, cbstat)
{
	int cbsize = CBSIZE;

	printf(
	"tot = %d (active = %d, free = %d (reserved = %d, slush = %d))\n",
	       ctotcount * cbsize, ctotcount * cbsize - cfreecount, cfreecount,
	       cfreecount - cslushcount * cbsize, cslushcount * cbsize);
}
#endif /* DDB */

/*
 * Called from init_main.c
 */
/* ARGSUSED*/
static void
clist_init(void *dummy)
{
	/*
	 * Allocate an initial base set of cblocks as a 'slush'.
	 * We allocate non-slush cblocks with each initial tty_open() and
	 * deallocate them with each tty_close().
	 * We should adjust the slush allocation.  This can't be done in
	 * the i/o routines because they are sometimes called from
	 * interrupt handlers when it may be unsafe to call malloc().
	 */
	cblock_alloc_cblocks(cslushcount = INITIAL_CBLOCKS);
}

/*
 * Remove a cblock from the cfreelist queue and return a pointer
 * to it.
 */
static __inline struct cblock *
cblock_alloc(void)
{
	struct cblock *cblockp;

	cblockp = cfreelist;
	if (cblockp == NULL)
		panic("clist reservation botch");
	cfreelist = cblockp->c_next;
	cblockp->c_next = NULL;
	cfreecount -= CBSIZE;
	return (cblockp);
}

/*
 * Add a cblock to the cfreelist queue.
 */
static __inline void
cblock_free(struct cblock *cblockp)
{
	cblockp->c_next = cfreelist;
	cfreelist = cblockp;
	cfreecount += CBSIZE;
}

/*
 * Allocate some cblocks for the cfreelist queue.
 */
static void
cblock_alloc_cblocks(int number)
{
	int i;
	struct cblock *cbp;

	for (i = 0; i < number; ++i) {
		cbp = malloc(sizeof *cbp, M_CLIST, M_NOWAIT);
		if (cbp == NULL) {
			printf(
"cblock_alloc_cblocks: M_NOWAIT malloc failed, trying M_WAITOK\n");
			cbp = malloc(sizeof *cbp, M_CLIST, M_WAITOK);
		}
		/*
		 * Freed cblocks have zero quotes and garbage elsewhere.
		 * Set the may-have-quote bit to force zeroing the quotes.
		 */
		cblock_free(cbp);
	}
	ctotcount += number;
}

/*
 * Set the cblock allocation policy for a clist.
 * Must be called in process context at spltty().
 */
void
clist_alloc_cblocks(struct clist *clistp, int ccmax, int ccreserved)
{
	int dcbr;

	/*
	 * Allow for wasted space at the head.
	 */
	if (ccmax != 0)
		ccmax += CBSIZE - 1;
	if (ccreserved != 0)
		ccreserved += CBSIZE - 1;

	clistp->c_cbmax = roundup(ccmax, CBSIZE) / CBSIZE;
	dcbr = roundup(ccreserved, CBSIZE) / CBSIZE - clistp->c_cbreserved;
	if (dcbr >= 0)
		cblock_alloc_cblocks(dcbr);
	else {
		if (clistp->c_cbreserved + dcbr < clistp->c_cbcount)
			dcbr = clistp->c_cbcount - clistp->c_cbreserved;
		cblock_free_cblocks(-dcbr);
	}
	clistp->c_cbreserved += dcbr;
}

/*
 * Free some cblocks from the cfreelist queue back to the
 * system malloc pool.
 */
static void
cblock_free_cblocks(int number)
{
	int i;

	for (i = 0; i < number; ++i)
		free(cblock_alloc(), M_CLIST);
	ctotcount -= number;
}

/*
 * Free the cblocks reserved for a clist.
 * Must be called at spltty().
 */
void
clist_free_cblocks(struct clist *clistp)
{
	if (clistp->c_cbcount != 0)
		panic("freeing active clist cblocks");
	cblock_free_cblocks(clistp->c_cbreserved);
	clistp->c_cbmax = 0;
	clistp->c_cbreserved = 0;
}

/*
 * Get a character from the head of a clist.
 */
int
getc(struct clist *clistp)
{
	int chr = -1;
	int s;
	struct cblock *cblockp;

	s = spltty();

	/* If there are characters in the list, get one */
	if (clistp->c_cc) {
		cblockp = (struct cblock *)((intptr_t)clistp->c_cf & ~CROUND);
		chr = (u_char)*clistp->c_cf;

		/*
		 * Advance to next character.
		 */
		clistp->c_cf++;
		clistp->c_cc--;
		/*
		 * If we have advanced the 'first' character pointer
		 * past the end of this cblock, advance to the next one.
		 * If there are no more characters, set the first and
		 * last pointers to NULL. In either case, free the
		 * current cblock.
		 */
		if ((clistp->c_cf >= (char *)(cblockp+1)) || (clistp->c_cc == 0)) {
			if (clistp->c_cc > 0) {
				clistp->c_cf = cblockp->c_next->c_info;
			} else {
				clistp->c_cf = clistp->c_cl = NULL;
			}
			cblock_free(cblockp);
			if (--clistp->c_cbcount >= clistp->c_cbreserved)
				++cslushcount;
		}
	}

	splx(s);
	return (chr);
}

/*
 * Copy 'amount' of chars, beginning at head of clist 'clistp' to
 * destination linear buffer 'dest'. Return number of characters
 * actually copied.
 */
int
q_to_b(struct clist *clistp, char *dest, int amount)
{
	struct cblock *cblockp;
	struct cblock *cblockn;
	char *dest_orig = dest;
	int numc;
	int s;

	s = spltty();

	while (clistp && amount && (clistp->c_cc > 0)) {
		cblockp = (struct cblock *)((intptr_t)clistp->c_cf & ~CROUND);
		cblockn = cblockp + 1; /* pointer arithmetic! */
		numc = min(amount, (char *)cblockn - clistp->c_cf);
		numc = min(numc, clistp->c_cc);
		bcopy(clistp->c_cf, dest, numc);
		amount -= numc;
		clistp->c_cf += numc;
		clistp->c_cc -= numc;
		dest += numc;
		/*
		 * If this cblock has been emptied, advance to the next
		 * one. If there are no more characters, set the first
		 * and last pointer to NULL. In either case, free the
		 * current cblock.
		 */
		if ((clistp->c_cf >= (char *)cblockn) || (clistp->c_cc == 0)) {
			if (clistp->c_cc > 0) {
				clistp->c_cf = cblockp->c_next->c_info;
			} else {
				clistp->c_cf = clistp->c_cl = NULL;
			}
			cblock_free(cblockp);
			if (--clistp->c_cbcount >= clistp->c_cbreserved)
				++cslushcount;
		}
	}

	splx(s);
	return (dest - dest_orig);
}

/*
 * Flush 'amount' of chars, beginning at head of clist 'clistp'.
 */
void
ndflush(struct clist *clistp, int amount)
{
	struct cblock *cblockp;
	struct cblock *cblockn;
	int numc;
	int s;

	s = spltty();

	while (amount && (clistp->c_cc > 0)) {
		cblockp = (struct cblock *)((intptr_t)clistp->c_cf & ~CROUND);
		cblockn = cblockp + 1; /* pointer arithmetic! */
		numc = min(amount, (char *)cblockn - clistp->c_cf);
		numc = min(numc, clistp->c_cc);
		amount -= numc;
		clistp->c_cf += numc;
		clistp->c_cc -= numc;
		/*
		 * If this cblock has been emptied, advance to the next
		 * one. If there are no more characters, set the first
		 * and last pointer to NULL. In either case, free the
		 * current cblock.
		 */
		if ((clistp->c_cf >= (char *)cblockn) || (clistp->c_cc == 0)) {
			if (clistp->c_cc > 0) {
				clistp->c_cf = cblockp->c_next->c_info;
			} else {
				clistp->c_cf = clistp->c_cl = NULL;
			}
			cblock_free(cblockp);
			if (--clistp->c_cbcount >= clistp->c_cbreserved)
				++cslushcount;
		}
	}

	splx(s);
}

/*
 * Add a character to the end of a clist. Return -1 is no
 * more clists, or 0 for success.
 */
int
putc(char chr, struct clist *clistp)
{
	struct cblock *cblockp;
	int s;

	s = spltty();

	if (clistp->c_cl == NULL) {
		if (clistp->c_cbreserved < 1) {
			splx(s);
			printf("putc to a clist with no reserved cblocks\n");
			return (-1);		/* nothing done */
		}
		cblockp = cblock_alloc();
		clistp->c_cbcount = 1;
		clistp->c_cf = clistp->c_cl = cblockp->c_info;
		clistp->c_cc = 0;
	} else {
		cblockp = (struct cblock *)((intptr_t)clistp->c_cl & ~CROUND);
		if (((intptr_t)clistp->c_cl & CROUND) == 0) {
			struct cblock *prev = (cblockp - 1);

			if (clistp->c_cbcount >= clistp->c_cbreserved) {
				if (clistp->c_cbcount >= clistp->c_cbmax
				    || cslushcount <= 0) {
					splx(s);
					return (-1);
				}
				--cslushcount;
			}
			cblockp = cblock_alloc();
			clistp->c_cbcount++;
			prev->c_next = cblockp;
			clistp->c_cl = cblockp->c_info;
		}
	}

	*clistp->c_cl++ = chr;
	clistp->c_cc++;

	splx(s);
	return (0);
}

/*
 * Copy data from linear buffer to clist chain. Return the
 * number of characters not copied.
 */
int
b_to_q(char *src, int amount, struct clist *clistp)
{
	struct cblock *cblockp;
	int numc, s;

	/*
	 * Avoid allocating an initial cblock and then not using it.
	 * c_cc == 0 must imply c_cbount == 0.
	 */
	if (amount <= 0)
		return (amount);

	s = spltty();

	/*
	 * If there are no cblocks assigned to this clist yet,
	 * then get one.
	 */
	if (clistp->c_cl == NULL) {
		if (clistp->c_cbreserved < 1) {
			splx(s);
			printf("b_to_q to a clist with no reserved cblocks.\n");
			return (amount);	/* nothing done */
		}
		cblockp = cblock_alloc();
		clistp->c_cbcount = 1;
		clistp->c_cf = clistp->c_cl = cblockp->c_info;
		clistp->c_cc = 0;
	} else {
		cblockp = (struct cblock *)((intptr_t)clistp->c_cl & ~CROUND);
	}

	while (amount) {
		/*
		 * Get another cblock if needed.
		 */
		if (((intptr_t)clistp->c_cl & CROUND) == 0) {
			struct cblock *prev = cblockp - 1;

			if (clistp->c_cbcount >= clistp->c_cbreserved) {
				if (clistp->c_cbcount >= clistp->c_cbmax
				    || cslushcount <= 0) {
					splx(s);
					return (amount);
				}
				--cslushcount;
			}
			cblockp = cblock_alloc();
			clistp->c_cbcount++;
			prev->c_next = cblockp;
			clistp->c_cl = cblockp->c_info;
		}

		/*
		 * Copy a chunk of the linear buffer up to the end
		 * of this cblock.
		 */
		numc = min(amount, (char *)(cblockp + 1) - clistp->c_cl);
		bcopy(src, clistp->c_cl, numc);

		/*
		 * ...and update pointer for the next chunk.
		 */
		src += numc;
		clistp->c_cl += numc;
		clistp->c_cc += numc;
		amount -= numc;
		/*
		 * If we go through the loop again, it's always
		 * for data in the next cblock, so by adding one (cblock),
		 * (which makes the pointer 1 beyond the end of this
		 * cblock) we prepare for the assignment of 'prev'
		 * above.
		 */
		cblockp += 1;

	}

	splx(s);
	return (amount);
}

/*
 * "Unput" a character from a clist.
 */
int
unputc(struct clist *clistp)
{
	struct cblock *cblockp = 0, *cbp = 0;
	int s;
	int chr = -1;


	s = spltty();

	if (clistp->c_cc) {
		--clistp->c_cc;
		--clistp->c_cl;

		chr = (u_char)*clistp->c_cl;

		cblockp = (struct cblock *)((intptr_t)clistp->c_cl & ~CROUND);

		/*
		 * If all of the characters have been unput in this
		 * cblock, then find the previous one and free this
		 * one.
		 */
		if (clistp->c_cc && (clistp->c_cl <= (char *)cblockp->c_info)) {
			cbp = (struct cblock *)((intptr_t)clistp->c_cf & ~CROUND);

			while (cbp->c_next != cblockp)
				cbp = cbp->c_next;

			/*
			 * When the previous cblock is at the end, the 'last'
			 * pointer always points (invalidly) one past.
			 */
			clistp->c_cl = (char *)(cbp+1);
			cblock_free(cblockp);
			if (--clistp->c_cbcount >= clistp->c_cbreserved)
				++cslushcount;
			cbp->c_next = NULL;
		}
	}

	/*
	 * If there are no more characters on the list, then
	 * free the last cblock.
	 */
	if ((clistp->c_cc == 0) && clistp->c_cl) {
		cblockp = (struct cblock *)((intptr_t)clistp->c_cl & ~CROUND);
		cblock_free(cblockp);
		if (--clistp->c_cbcount >= clistp->c_cbreserved)
			++cslushcount;
		clistp->c_cf = clistp->c_cl = NULL;
	}

	splx(s);
	return (chr);
}
