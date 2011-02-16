<<<<<<< HEAD
/*	$OpenBSD: pool.h,v 1.20 2006/12/23 15:00:15 miod Exp $	*/
=======
/*	$OpenBSD: pool.h,v 1.36 2010/09/26 21:03:57 tedu Exp $	*/
>>>>>>> origin/master
/*	$NetBSD: pool.h,v 1.27 2001/06/06 22:00:17 rafal Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg; by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_POOL_H_
#define _SYS_POOL_H_

/*
 * sysctls.
 * kern.pool.npools
 * kern.pool.name.<number>
 * kern.pool.pool.<number>
 */
#define KERN_POOL_NPOOLS	1
#define KERN_POOL_NAME		2
#define KERN_POOL_POOL		3

#include <sys/lock.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <sys/tree.h>

struct pool_cache {
	TAILQ_ENTRY(pool_cache)
			pc_poollist;	/* entry on pool's group list */
	TAILQ_HEAD(, pool_cache_group)
			pc_grouplist;	/* Cache group list */
	struct pool_cache_group
			*pc_allocfrom;	/* group to allocate from */
	struct pool_cache_group
			*pc_freeto;	/* group to free to */
	struct pool	*pc_pool;	/* parent pool */
	struct simplelock pc_slock;	/* mutex */

	int		(*pc_ctor)(void *, void *, int);
	void		(*pc_dtor)(void *, void *);
	void		*pc_arg;

	/* Statistics. */
	unsigned long	pc_hits;	/* cache hits */
	unsigned long	pc_misses;	/* cache misses */

	unsigned long	pc_ngroups;	/* # cache groups */

	unsigned long	pc_nitems;	/* # objects currently in cache */
};

struct pool_allocator {
	void *(*pa_alloc)(struct pool *, int, int *);
	void (*pa_free)(struct pool *, void *);
	int pa_pagesz;

	/* The following fields are for internal use only */
	struct simplelock pa_slock;
	TAILQ_HEAD(,pool) pa_list;
	int pa_flags;
#define PA_INITIALIZED	0x01
#define PA_WANT	0x02			/* wakeup any sleeping pools on free */
	int pa_pagemask;
	int pa_pageshift;
};

LIST_HEAD(pool_pagelist,pool_item_header);

struct pool {
	TAILQ_ENTRY(pool)
			pr_poollist;
	struct pool_pagelist
			pr_emptypages;	/* Empty pages */
	struct pool_pagelist
			pr_fullpages;	/* Full pages */
	struct pool_pagelist
			pr_partpages;	/* Partially-allocated pages */
	struct pool_item_header	*pr_curpage;
	TAILQ_HEAD(,pool_cache)
			pr_cachelist;	/* Caches for this pool */
	unsigned int	pr_size;	/* Size of item */
	unsigned int	pr_align;	/* Requested alignment, must be 2^n */
	unsigned int	pr_itemoffset;	/* Align this offset in item */
	unsigned int	pr_minitems;	/* minimum # of items to keep */
	unsigned int	pr_minpages;	/* same in page units */
	unsigned int	pr_maxpages;	/* maximum # of idle pages to keep */
	unsigned int	pr_npages;	/* # of pages allocated */
	unsigned int	pr_itemsperpage;/* # items that fit in a page */
	unsigned int	pr_slack;	/* unused space in a page */
	unsigned int	pr_nitems;	/* number of available items in pool */
	unsigned int	pr_nout;	/* # items currently allocated */
	unsigned int	pr_hardlimit;	/* hard limit to number of allocated
					   items */
	unsigned int	pr_serial;	/* unique serial number of the pool */
	struct pool_allocator *pr_alloc;/* backend allocator */
	TAILQ_ENTRY(pool) pr_alloc_list;/* list of pools using this allocator */
	const char	*pr_wchan;	/* tsleep(9) identifier */
	unsigned int	pr_flags;	/* r/w flags */
	unsigned int	pr_roflags;	/* r/o flags */
<<<<<<< HEAD
#define PR_MALLOCOK	0x01
#define	PR_NOWAIT	0x00		/* for symmetry */
#define PR_WAITOK	0x02
#define PR_WANTED	0x04
#define PR_PHINPAGE	0x08
#define PR_LOGGING	0x10
#define PR_LIMITFAIL	0x20	/* even if waiting, fail if we hit limit */
#define PR_DEBUG	0x40
=======
#define PR_WAITOK	0x0001 /* M_WAITOK */
#define PR_NOWAIT	0x0002 /* M_NOWAIT */
#define PR_LIMITFAIL	0x0004 /* M_CANFAIL */
#define PR_ZERO		0x0008 /* M_ZERO */
#define PR_WANTED	0x0100
#define PR_PHINPAGE	0x0200
#define PR_LOGGING	0x0400
#define PR_DEBUG	0x0800
>>>>>>> origin/master

	/*
	 * `pr_slock' protects the pool's data structures when removing
	 * items from or returning items to the pool, or when reading
	 * or updating read/write fields in the pool descriptor.
	 *
	 * We assume back-end page allocators provide their own locking
	 * scheme.  They will be called with the pool descriptor _unlocked_,
	 * since the page allocators may block.
	 */
	struct simplelock	pr_slock;

	RB_HEAD(phtree, pool_item_header) pr_phtree;

	int		pr_maxcolor;	/* Cache colouring */
	int		pr_curcolor;
	int		pr_phoffset;	/* Offset in page of page header */

	/*
	 * Warning message to be issued, and a per-time-delta rate cap,
	 * if the hard limit is reached.
	 */
	const char	*pr_hardlimit_warning;
	struct timeval	pr_hardlimit_ratecap;
	struct timeval	pr_hardlimit_warning_last;

	/*
	 * Instrumentation
	 */
	unsigned long	pr_nget;	/* # of successful requests */
	unsigned long	pr_nfail;	/* # of unsuccessful requests */
	unsigned long	pr_nput;	/* # of releases */
	unsigned long	pr_npagealloc;	/* # of pages allocated */
	unsigned long	pr_npagefree;	/* # of pages released */
	unsigned int	pr_hiwat;	/* max # of pages in pool */
	unsigned long	pr_nidle;	/* # of idle pages */

<<<<<<< HEAD
	/*
	 * Diagnostic aides.
	 */
	struct pool_log	*pr_log;
	int		pr_curlogentry;
	int		pr_logsize;

	const char	*pr_entered_file; /* reentrancy check */
	long		pr_entered_line;
};

#ifdef _KERNEL
/* old nointr allocator, still needed for large allocations */
extern struct pool_allocator pool_allocator_oldnointr;
/* interrupt safe (name preserved for compat) new default allocator */
=======
	/* Physical memory configuration. */
	struct uvm_constraint_range *pr_crange;
	int		pr_pa_nsegs;
};

#ifdef _KERNEL
>>>>>>> origin/master
extern struct pool_allocator pool_allocator_nointr;
/* previous interrupt safe allocator, allocates from kmem */
extern struct pool_allocator pool_allocator_kmem;

void		pool_init(struct pool *, size_t, u_int, u_int, int,
		    const char *, struct pool_allocator *);
void		pool_destroy(struct pool *);
<<<<<<< HEAD
=======
void		pool_setipl(struct pool *, int);
void		pool_setlowat(struct pool *, int);
void		pool_sethiwat(struct pool *, int);
int		pool_sethardlimit(struct pool *, u_int, const char *, int);
struct uvm_constraint_range; /* XXX */
void		pool_set_constraints(struct pool *,
		    struct uvm_constraint_range *, int);
void		pool_set_ctordtor(struct pool *, int (*)(void *, void *, int),
		    void(*)(void *, void *), void *);
>>>>>>> origin/master

void		*pool_get(struct pool *, int) __malloc;
void		pool_put(struct pool *, void *);
int		pool_reclaim(struct pool *);

#ifdef POOL_DIAGNOSTIC
/*
 * These versions do reentrancy checking.
 */
void		*_pool_get(struct pool *, int, const char *, long);
void		_pool_put(struct pool *, void *, const char *, long);
int		_pool_reclaim(struct pool *, const char *, long);
#define		pool_get(h, f)	_pool_get((h), (f), __FILE__, __LINE__)
#define		pool_put(h, v)	_pool_put((h), (v), __FILE__, __LINE__)
#define		pool_reclaim(h)	_pool_reclaim((h), __FILE__, __LINE__)
#endif /* POOL_DIAGNOSTIC */

int		pool_prime(struct pool *, int);
void		pool_setlowat(struct pool *, int);
void		pool_sethiwat(struct pool *, int);
int		pool_sethardlimit(struct pool *, unsigned, const char *, int);

#ifdef DDB
/*
 * Debugging and diagnostic aides.
 */
void		pool_printit(struct pool *, const char *,
		    int (*)(const char *, ...));
int		pool_chk(struct pool *, const char *);
void		pool_walk(struct pool *, int, int (*)(const char *, ...),
		    void (*)(void *, int, int (*)(const char *, ...)));
#endif

<<<<<<< HEAD
/*
 * Pool cache routines.
 */
void		pool_cache_init(struct pool_cache *, struct pool *,
		    int (*ctor)(void *, void *, int),
		    void (*dtor)(void *, void *),
		    void *);
void		pool_cache_destroy(struct pool_cache *);
void		*pool_cache_get(struct pool_cache *, int);
void		pool_cache_put(struct pool_cache *, void *);
void		pool_cache_destruct_object(struct pool_cache *, void *);
void		pool_cache_invalidate(struct pool_cache *);
=======
/* the allocator for dma-able memory is a thin layer on top of pool  */
void			 dma_alloc_init(void);
void			*dma_alloc(size_t size, int flags);
void			 dma_free(void *m, size_t size);
>>>>>>> origin/master
#endif /* _KERNEL */

#endif /* _SYS_POOL_H_ */
