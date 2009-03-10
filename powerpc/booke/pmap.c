/*-
 * Copyright (C) 2007-2008 Semihalf, Rafal Jaworowski <raj@semihalf.com>
 * Copyright (C) 2006 Semihalf, Marian Balakowicz <m8@semihalf.com>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Some hw specific parts of this pmap were derived or influenced
 * by NetBSD's ibm4xx pmap module. More generic code is shared with
 * a few other pmap modules from the FreeBSD tree.
 */

 /*
  * VM layout notes:
  *
  * Kernel and user threads run within one common virtual address space
  * defined by AS=0.
  *
  * Virtual address space layout:
  * -----------------------------
  * 0x0000_0000 - 0xafff_ffff	: user process
  * 0xb000_0000 - 0xbfff_ffff	: pmap_mapdev()-ed area (PCI/PCIE etc.)
  * 0xc000_0000 - 0xc0ff_ffff	: kernel reserved
  *   0xc000_0000 - kernelend	: kernel code+data, env, metadata etc.
  * 0xc100_0000 - 0xfeef_ffff	: KVA
  *   0xc100_0000 - 0xc100_3fff : reserved for page zero/copy
  *   0xc100_4000 - 0xc200_3fff : reserved for ptbl bufs
  *   0xc200_4000 - 0xc200_8fff : guard page + kstack0
  *   0xc200_9000 - 0xfeef_ffff	: actual free KVA space
  * 0xfef0_0000 - 0xffff_ffff	: I/O devices region
  */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/powerpc/booke/pmap.c,v 1.10 2009/02/28 16:21:25 ed Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/ktr.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/msgbuf.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/vmmeter.h>

#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/vm_kern.h>
#include <vm/vm_pageout.h>
#include <vm/vm_extern.h>
#include <vm/vm_object.h>
#include <vm/vm_param.h>
#include <vm/vm_map.h>
#include <vm/vm_pager.h>
#include <vm/uma.h>

#include <machine/cpu.h>
#include <machine/pcb.h>
#include <machine/powerpc.h>

#include <machine/tlb.h>
#include <machine/spr.h>
#include <machine/vmparam.h>
#include <machine/md_var.h>
#include <machine/mmuvar.h>
#include <machine/pmap.h>
#include <machine/pte.h>

#include "mmu_if.h"

#define DEBUG
#undef DEBUG

#ifdef  DEBUG
#define debugf(fmt, args...) printf(fmt, ##args)
#else
#define debugf(fmt, args...)
#endif

#define TODO			panic("%s: not implemented", __func__);

#include "opt_sched.h"
#ifndef SCHED_4BSD
#error "e500 only works with SCHED_4BSD which uses a global scheduler lock."
#endif
extern struct mtx sched_lock;

/* Kernel physical load address. */
extern uint32_t kernload;

struct mem_region availmem_regions[MEM_REGIONS];
int availmem_regions_sz;

/* Reserved KVA space and mutex for mmu_booke_zero_page. */
static vm_offset_t zero_page_va;
static struct mtx zero_page_mutex;

static struct mtx tlbivax_mutex;

/*
 * Reserved KVA space for mmu_booke_zero_page_idle. This is used
 * by idle thred only, no lock required.
 */
static vm_offset_t zero_page_idle_va;

/* Reserved KVA space and mutex for mmu_booke_copy_page. */
static vm_offset_t copy_page_src_va;
static vm_offset_t copy_page_dst_va;
static struct mtx copy_page_mutex;

/**************************************************************************/
/* PMAP */
/**************************************************************************/

static void mmu_booke_enter_locked(mmu_t, pmap_t, vm_offset_t, vm_page_t,
    vm_prot_t, boolean_t);

unsigned int kptbl_min;		/* Index of the first kernel ptbl. */
unsigned int kernel_ptbls;	/* Number of KVA ptbls. */

static int pagedaemon_waken;

/*
 * If user pmap is processed with mmu_booke_remove and the resident count
 * drops to 0, there are no more pages to remove, so we need not continue.
 */
#define PMAP_REMOVE_DONE(pmap) \
	((pmap) != kernel_pmap && (pmap)->pm_stats.resident_count == 0)

extern void tlb_lock(uint32_t *);
extern void tlb_unlock(uint32_t *);
extern void tid_flush(tlbtid_t);

/**************************************************************************/
/* TLB and TID handling */
/**************************************************************************/

/* Translation ID busy table */
static volatile pmap_t tidbusy[MAXCPU][TID_MAX + 1];

/*
 * TLB0 capabilities (entry, way numbers etc.). These can vary between e500
 * core revisions and should be read from h/w registers during early config.
 */
uint32_t tlb0_entries;
uint32_t tlb0_ways;
uint32_t tlb0_entries_per_way;

#define TLB0_ENTRIES		(tlb0_entries)
#define TLB0_WAYS		(tlb0_ways)
#define TLB0_ENTRIES_PER_WAY	(tlb0_entries_per_way)

#define TLB1_ENTRIES 16

/* In-ram copy of the TLB1 */
static tlb_entry_t tlb1[TLB1_ENTRIES];

/* Next free entry in the TLB1 */
static unsigned int tlb1_idx;

static tlbtid_t tid_alloc(struct pmap *);

static void tlb_print_entry(int, uint32_t, uint32_t, uint32_t, uint32_t);

static int tlb1_set_entry(vm_offset_t, vm_offset_t, vm_size_t, uint32_t);
static void tlb1_write_entry(unsigned int);
static int tlb1_iomapped(int, vm_paddr_t, vm_size_t, vm_offset_t *);
static vm_size_t tlb1_mapin_region(vm_offset_t, vm_offset_t, vm_size_t);

static vm_size_t tsize2size(unsigned int);
static unsigned int size2tsize(vm_size_t);
static unsigned int ilog2(unsigned int);

static void set_mas4_defaults(void);

static inline void tlb0_flush_entry(vm_offset_t);
static inline unsigned int tlb0_tableidx(vm_offset_t, unsigned int);

/**************************************************************************/
/* Page table management */
/**************************************************************************/

/* Data for the pv entry allocation mechanism */
static uma_zone_t pvzone;
static struct vm_object pvzone_obj;
static int pv_entry_count = 0, pv_entry_max = 0, pv_entry_high_water = 0;

#define PV_ENTRY_ZONE_MIN	2048	/* min pv entries in uma zone */

#ifndef PMAP_SHPGPERPROC
#define PMAP_SHPGPERPROC	200
#endif

static void ptbl_init(void);
static struct ptbl_buf *ptbl_buf_alloc(void);
static void ptbl_buf_free(struct ptbl_buf *);
static void ptbl_free_pmap_ptbl(pmap_t, pte_t *);

static pte_t *ptbl_alloc(mmu_t, pmap_t, unsigned int);
static void ptbl_free(mmu_t, pmap_t, unsigned int);
static void ptbl_hold(mmu_t, pmap_t, unsigned int);
static int ptbl_unhold(mmu_t, pmap_t, unsigned int);

static vm_paddr_t pte_vatopa(mmu_t, pmap_t, vm_offset_t);
static pte_t *pte_find(mmu_t, pmap_t, vm_offset_t);
static void pte_enter(mmu_t, pmap_t, vm_page_t, vm_offset_t, uint32_t);
static int pte_remove(mmu_t, pmap_t, vm_offset_t, uint8_t);

static pv_entry_t pv_alloc(void);
static void pv_free(pv_entry_t);
static void pv_insert(pmap_t, vm_offset_t, vm_page_t);
static void pv_remove(pmap_t, vm_offset_t, vm_page_t);

/* Number of kva ptbl buffers, each covering one ptbl (PTBL_PAGES). */
#define PTBL_BUFS		(128 * 16)

struct ptbl_buf {
	TAILQ_ENTRY(ptbl_buf) link;	/* list link */
	vm_offset_t kva;		/* va of mapping */
};

/* ptbl free list and a lock used for access synchronization. */
static TAILQ_HEAD(, ptbl_buf) ptbl_buf_freelist;
static struct mtx ptbl_buf_freelist_lock;

/* Base address of kva space allocated fot ptbl bufs. */
static vm_offset_t ptbl_buf_pool_vabase;

/* Pointer to ptbl_buf structures. */
static struct ptbl_buf *ptbl_bufs;

/*
 * Kernel MMU interface
 */
static void		mmu_booke_change_wiring(mmu_t, pmap_t, vm_offset_t, boolean_t);
static void		mmu_booke_clear_modify(mmu_t, vm_page_t);
static void		mmu_booke_clear_reference(mmu_t, vm_page_t);
static void		mmu_booke_copy(pmap_t, pmap_t, vm_offset_t, vm_size_t,
    vm_offset_t);
static void		mmu_booke_copy_page(mmu_t, vm_page_t, vm_page_t);
static void		mmu_booke_enter(mmu_t, pmap_t, vm_offset_t, vm_page_t,
    vm_prot_t, boolean_t);
static void		mmu_booke_enter_object(mmu_t, pmap_t, vm_offset_t, vm_offset_t,
    vm_page_t, vm_prot_t);
static void		mmu_booke_enter_quick(mmu_t, pmap_t, vm_offset_t, vm_page_t,
    vm_prot_t);
static vm_paddr_t	mmu_booke_extract(mmu_t, pmap_t, vm_offset_t);
static vm_page_t	mmu_booke_extract_and_hold(mmu_t, pmap_t, vm_offset_t,
    vm_prot_t);
static void		mmu_booke_init(mmu_t);
static boolean_t	mmu_booke_is_modified(mmu_t, vm_page_t);
static boolean_t	mmu_booke_is_prefaultable(mmu_t, pmap_t, vm_offset_t);
static boolean_t	mmu_booke_ts_referenced(mmu_t, vm_page_t);
static vm_offset_t	mmu_booke_map(mmu_t, vm_offset_t *, vm_offset_t, vm_offset_t,
    int);
static int		mmu_booke_mincore(mmu_t, pmap_t, vm_offset_t);
static void		mmu_booke_object_init_pt(mmu_t, pmap_t, vm_offset_t,
    vm_object_t, vm_pindex_t, vm_size_t);
static boolean_t	mmu_booke_page_exists_quick(mmu_t, pmap_t, vm_page_t);
static void		mmu_booke_page_init(mmu_t, vm_page_t);
static int		mmu_booke_page_wired_mappings(mmu_t, vm_page_t);
static void		mmu_booke_pinit(mmu_t, pmap_t);
static void		mmu_booke_pinit0(mmu_t, pmap_t);
static void		mmu_booke_protect(mmu_t, pmap_t, vm_offset_t, vm_offset_t,
    vm_prot_t);
static void		mmu_booke_qenter(mmu_t, vm_offset_t, vm_page_t *, int);
static void		mmu_booke_qremove(mmu_t, vm_offset_t, int);
static void		mmu_booke_release(mmu_t, pmap_t);
static void		mmu_booke_remove(mmu_t, pmap_t, vm_offset_t, vm_offset_t);
static void		mmu_booke_remove_all(mmu_t, vm_page_t);
static void		mmu_booke_remove_write(mmu_t, vm_page_t);
static void		mmu_booke_zero_page(mmu_t, vm_page_t);
static void		mmu_booke_zero_page_area(mmu_t, vm_page_t, int, int);
static void		mmu_booke_zero_page_idle(mmu_t, vm_page_t);
static void		mmu_booke_activate(mmu_t, struct thread *);
static void		mmu_booke_deactivate(mmu_t, struct thread *);
static void		mmu_booke_bootstrap(mmu_t, vm_offset_t, vm_offset_t);
static void		*mmu_booke_mapdev(mmu_t, vm_offset_t, vm_size_t);
static void		mmu_booke_unmapdev(mmu_t, vm_offset_t, vm_size_t);
static vm_offset_t	mmu_booke_kextract(mmu_t, vm_offset_t);
static void		mmu_booke_kenter(mmu_t, vm_offset_t, vm_offset_t);
static void		mmu_booke_kremove(mmu_t, vm_offset_t);
static boolean_t	mmu_booke_dev_direct_mapped(mmu_t, vm_offset_t, vm_size_t);
static boolean_t	mmu_booke_page_executable(mmu_t, vm_page_t);

static mmu_method_t mmu_booke_methods[] = {
	/* pmap dispatcher interface */
	MMUMETHOD(mmu_change_wiring,	mmu_booke_change_wiring),
	MMUMETHOD(mmu_clear_modify,	mmu_booke_clear_modify),
	MMUMETHOD(mmu_clear_reference,	mmu_booke_clear_reference),
	MMUMETHOD(mmu_copy,		mmu_booke_copy),
	MMUMETHOD(mmu_copy_page,	mmu_booke_copy_page),
	MMUMETHOD(mmu_enter,		mmu_booke_enter),
	MMUMETHOD(mmu_enter_object,	mmu_booke_enter_object),
	MMUMETHOD(mmu_enter_quick,	mmu_booke_enter_quick),
	MMUMETHOD(mmu_extract,		mmu_booke_extract),
	MMUMETHOD(mmu_extract_and_hold,	mmu_booke_extract_and_hold),
	MMUMETHOD(mmu_init,		mmu_booke_init),
	MMUMETHOD(mmu_is_modified,	mmu_booke_is_modified),
	MMUMETHOD(mmu_is_prefaultable,	mmu_booke_is_prefaultable),
	MMUMETHOD(mmu_ts_referenced,	mmu_booke_ts_referenced),
	MMUMETHOD(mmu_map,		mmu_booke_map),
	MMUMETHOD(mmu_mincore,		mmu_booke_mincore),
	MMUMETHOD(mmu_object_init_pt,	mmu_booke_object_init_pt),
	MMUMETHOD(mmu_page_exists_quick,mmu_booke_page_exists_quick),
	MMUMETHOD(mmu_page_init,	mmu_booke_page_init),
	MMUMETHOD(mmu_page_wired_mappings, mmu_booke_page_wired_mappings),
	MMUMETHOD(mmu_pinit,		mmu_booke_pinit),
	MMUMETHOD(mmu_pinit0,		mmu_booke_pinit0),
	MMUMETHOD(mmu_protect,		mmu_booke_protect),
	MMUMETHOD(mmu_qenter,		mmu_booke_qenter),
	MMUMETHOD(mmu_qremove,		mmu_booke_qremove),
	MMUMETHOD(mmu_release,		mmu_booke_release),
	MMUMETHOD(mmu_remove,		mmu_booke_remove),
	MMUMETHOD(mmu_remove_all,	mmu_booke_remove_all),
	MMUMETHOD(mmu_remove_write,	mmu_booke_remove_write),
	MMUMETHOD(mmu_zero_page,	mmu_booke_zero_page),
	MMUMETHOD(mmu_zero_page_area,	mmu_booke_zero_page_area),
	MMUMETHOD(mmu_zero_page_idle,	mmu_booke_zero_page_idle),
	MMUMETHOD(mmu_activate,		mmu_booke_activate),
	MMUMETHOD(mmu_deactivate,	mmu_booke_deactivate),

	/* Internal interfaces */
	MMUMETHOD(mmu_bootstrap,	mmu_booke_bootstrap),
	MMUMETHOD(mmu_dev_direct_mapped,mmu_booke_dev_direct_mapped),
	MMUMETHOD(mmu_mapdev,		mmu_booke_mapdev),
	MMUMETHOD(mmu_kenter,		mmu_booke_kenter),
	MMUMETHOD(mmu_kextract,		mmu_booke_kextract),
/*	MMUMETHOD(mmu_kremove,		mmu_booke_kremove),	*/
	MMUMETHOD(mmu_page_executable,	mmu_booke_page_executable),
	MMUMETHOD(mmu_unmapdev,		mmu_booke_unmapdev),

	{ 0, 0 }
};

static mmu_def_t booke_mmu = {
	MMU_TYPE_BOOKE,
	mmu_booke_methods,
	0
};
MMU_DEF(booke_mmu);

/* Return number of entries in TLB0. */
static __inline void
tlb0_get_tlbconf(void)
{
	uint32_t tlb0_cfg;

	tlb0_cfg = mfspr(SPR_TLB0CFG);
	tlb0_entries = tlb0_cfg & TLBCFG_NENTRY_MASK;
	tlb0_ways = (tlb0_cfg & TLBCFG_ASSOC_MASK) >> TLBCFG_ASSOC_SHIFT;
	tlb0_entries_per_way = tlb0_entries / tlb0_ways;
}

/* Initialize pool of kva ptbl buffers. */
static void
ptbl_init(void)
{
	int i;

	CTR3(KTR_PMAP, "%s: s (ptbl_bufs = 0x%08x size 0x%08x)", __func__,
	    (uint32_t)ptbl_bufs, sizeof(struct ptbl_buf) * PTBL_BUFS);
	CTR3(KTR_PMAP, "%s: s (ptbl_buf_pool_vabase = 0x%08x size = 0x%08x)",
	    __func__, ptbl_buf_pool_vabase, PTBL_BUFS * PTBL_PAGES * PAGE_SIZE);

	mtx_init(&ptbl_buf_freelist_lock, "ptbl bufs lock", NULL, MTX_DEF);
	TAILQ_INIT(&ptbl_buf_freelist);

	for (i = 0; i < PTBL_BUFS; i++) {
		ptbl_bufs[i].kva = ptbl_buf_pool_vabase + i * PTBL_PAGES * PAGE_SIZE;
		TAILQ_INSERT_TAIL(&ptbl_buf_freelist, &ptbl_bufs[i], link);
	}
}

/* Get a ptbl_buf from the freelist. */
static struct ptbl_buf *
ptbl_buf_alloc(void)
{
	struct ptbl_buf *buf;

	mtx_lock(&ptbl_buf_freelist_lock);
	buf = TAILQ_FIRST(&ptbl_buf_freelist);
	if (buf != NULL)
		TAILQ_REMOVE(&ptbl_buf_freelist, buf, link);
	mtx_unlock(&ptbl_buf_freelist_lock);

	CTR2(KTR_PMAP, "%s: buf = %p", __func__, buf);

	return (buf);
}

/* Return ptbl buff to free pool. */
static void
ptbl_buf_free(struct ptbl_buf *buf)
{

	CTR2(KTR_PMAP, "%s: buf = %p", __func__, buf);

	mtx_lock(&ptbl_buf_freelist_lock);
	TAILQ_INSERT_TAIL(&ptbl_buf_freelist, buf, link);
	mtx_unlock(&ptbl_buf_freelist_lock);
}

/*
 * Search the list of allocated ptbl bufs and find on list of allocated ptbls
 */
static void
ptbl_free_pmap_ptbl(pmap_t pmap, pte_t *ptbl)
{
	struct ptbl_buf *pbuf;

	CTR2(KTR_PMAP, "%s: ptbl = %p", __func__, ptbl);

	PMAP_LOCK_ASSERT(pmap, MA_OWNED);

	TAILQ_FOREACH(pbuf, &pmap->pm_ptbl_list, link)
		if (pbuf->kva == (vm_offset_t)ptbl) {
			/* Remove from pmap ptbl buf list. */
			TAILQ_REMOVE(&pmap->pm_ptbl_list, pbuf, link);

			/* Free corresponding ptbl buf. */
			ptbl_buf_free(pbuf);
			break;
		}
}

/* Allocate page table. */
static pte_t *
ptbl_alloc(mmu_t mmu, pmap_t pmap, unsigned int pdir_idx)
{
	vm_page_t mtbl[PTBL_PAGES];
	vm_page_t m;
	struct ptbl_buf *pbuf;
	unsigned int pidx;
	pte_t *ptbl;
	int i;

	CTR4(KTR_PMAP, "%s: pmap = %p su = %d pdir_idx = %d", __func__, pmap,
	    (pmap == kernel_pmap), pdir_idx);

	KASSERT((pdir_idx <= (VM_MAXUSER_ADDRESS / PDIR_SIZE)),
	    ("ptbl_alloc: invalid pdir_idx"));
	KASSERT((pmap->pm_pdir[pdir_idx] == NULL),
	    ("pte_alloc: valid ptbl entry exists!"));

	pbuf = ptbl_buf_alloc();
	if (pbuf == NULL)
		panic("pte_alloc: couldn't alloc kernel virtual memory");
		
	ptbl = (pte_t *)pbuf->kva;

	CTR2(KTR_PMAP, "%s: ptbl kva = %p", __func__, ptbl);

	/* Allocate ptbl pages, this will sleep! */
	for (i = 0; i < PTBL_PAGES; i++) {
		pidx = (PTBL_PAGES * pdir_idx) + i;
		while ((m = vm_page_alloc(NULL, pidx,
		    VM_ALLOC_NOOBJ | VM_ALLOC_WIRED)) == NULL) {

			PMAP_UNLOCK(pmap);
			vm_page_unlock_queues();
			VM_WAIT;
			vm_page_lock_queues();
			PMAP_LOCK(pmap);
		}
		mtbl[i] = m;
	}

	/* Map allocated pages into kernel_pmap. */
	mmu_booke_qenter(mmu, (vm_offset_t)ptbl, mtbl, PTBL_PAGES);

	/* Zero whole ptbl. */
	bzero((caddr_t)ptbl, PTBL_PAGES * PAGE_SIZE);

	/* Add pbuf to the pmap ptbl bufs list. */
	TAILQ_INSERT_TAIL(&pmap->pm_ptbl_list, pbuf, link);

	return (ptbl);
}

/* Free ptbl pages and invalidate pdir entry. */
static void
ptbl_free(mmu_t mmu, pmap_t pmap, unsigned int pdir_idx)
{
	pte_t *ptbl;
	vm_paddr_t pa;
	vm_offset_t va;
	vm_page_t m;
	int i;

	CTR4(KTR_PMAP, "%s: pmap = %p su = %d pdir_idx = %d", __func__, pmap,
	    (pmap == kernel_pmap), pdir_idx);

	KASSERT((pdir_idx <= (VM_MAXUSER_ADDRESS / PDIR_SIZE)),
	    ("ptbl_free: invalid pdir_idx"));

	ptbl = pmap->pm_pdir[pdir_idx];

	CTR2(KTR_PMAP, "%s: ptbl = %p", __func__, ptbl);

	KASSERT((ptbl != NULL), ("ptbl_free: null ptbl"));

	/*
	 * Invalidate the pdir entry as soon as possible, so that other CPUs
	 * don't attempt to look up the page tables we are releasing.
	 */
	mtx_lock_spin(&tlbivax_mutex);
	
	pmap->pm_pdir[pdir_idx] = NULL;

	mtx_unlock_spin(&tlbivax_mutex);

	for (i = 0; i < PTBL_PAGES; i++) {
		va = ((vm_offset_t)ptbl + (i * PAGE_SIZE));
		pa = pte_vatopa(mmu, kernel_pmap, va);
		m = PHYS_TO_VM_PAGE(pa);
		vm_page_free_zero(m);
		atomic_subtract_int(&cnt.v_wire_count, 1);
		mmu_booke_kremove(mmu, va);
	}

	ptbl_free_pmap_ptbl(pmap, ptbl);
}

/*
 * Decrement ptbl pages hold count and attempt to free ptbl pages.
 * Called when removing pte entry from ptbl.
 *
 * Return 1 if ptbl pages were freed.
 */
static int
ptbl_unhold(mmu_t mmu, pmap_t pmap, unsigned int pdir_idx)
{
	pte_t *ptbl;
	vm_paddr_t pa;
	vm_page_t m;
	int i;

	CTR4(KTR_PMAP, "%s: pmap = %p su = %d pdir_idx = %d", __func__, pmap,
	    (pmap == kernel_pmap), pdir_idx);

	KASSERT((pdir_idx <= (VM_MAXUSER_ADDRESS / PDIR_SIZE)),
	    ("ptbl_unhold: invalid pdir_idx"));
	KASSERT((pmap != kernel_pmap),
	    ("ptbl_unhold: unholding kernel ptbl!"));

	ptbl = pmap->pm_pdir[pdir_idx];

	//debugf("ptbl_unhold: ptbl = 0x%08x\n", (u_int32_t)ptbl);
	KASSERT(((vm_offset_t)ptbl >= VM_MIN_KERNEL_ADDRESS),
	    ("ptbl_unhold: non kva ptbl"));

	/* decrement hold count */
	for (i = 0; i < PTBL_PAGES; i++) {
		pa = pte_vatopa(mmu, kernel_pmap,
		    (vm_offset_t)ptbl + (i * PAGE_SIZE));
		m = PHYS_TO_VM_PAGE(pa);
		m->wire_count--;
	}

	/*
	 * Free ptbl pages if there are no pte etries in this ptbl.
	 * wire_count has the same value for all ptbl pages, so check the last
	 * page.
	 */
	if (m->wire_count == 0) {
		ptbl_free(mmu, pmap, pdir_idx);

		//debugf("ptbl_unhold: e (freed ptbl)\n");
		return (1);
	}

	return (0);
}

/*
 * Increment hold count for ptbl pages. This routine is used when a new pte
 * entry is being inserted into the ptbl.
 */
static void
ptbl_hold(mmu_t mmu, pmap_t pmap, unsigned int pdir_idx)
{
	vm_paddr_t pa;
	pte_t *ptbl;
	vm_page_t m;
	int i;

	CTR3(KTR_PMAP, "%s: pmap = %p pdir_idx = %d", __func__, pmap,
	    pdir_idx);

	KASSERT((pdir_idx <= (VM_MAXUSER_ADDRESS / PDIR_SIZE)),
	    ("ptbl_hold: invalid pdir_idx"));
	KASSERT((pmap != kernel_pmap),
	    ("ptbl_hold: holding kernel ptbl!"));

	ptbl = pmap->pm_pdir[pdir_idx];

	KASSERT((ptbl != NULL), ("ptbl_hold: null ptbl"));

	for (i = 0; i < PTBL_PAGES; i++) {
		pa = pte_vatopa(mmu, kernel_pmap,
		    (vm_offset_t)ptbl + (i * PAGE_SIZE));
		m = PHYS_TO_VM_PAGE(pa);
		m->wire_count++;
	}
}

/* Allocate pv_entry structure. */
pv_entry_t
pv_alloc(void)
{
	pv_entry_t pv;

	pv_entry_count++;
	if ((pv_entry_count > pv_entry_high_water) &&
	    (pagedaemon_waken == 0)) {
		pagedaemon_waken = 1;
		wakeup(&vm_pages_needed);
	}
	pv = uma_zalloc(pvzone, M_NOWAIT);

	return (pv);
}

/* Free pv_entry structure. */
static __inline void
pv_free(pv_entry_t pve)
{

	pv_entry_count--;
	uma_zfree(pvzone, pve);
}


/* Allocate and initialize pv_entry structure. */
static void
pv_insert(pmap_t pmap, vm_offset_t va, vm_page_t m)
{
	pv_entry_t pve;

	//int su = (pmap == kernel_pmap);
	//debugf("pv_insert: s (su = %d pmap = 0x%08x va = 0x%08x m = 0x%08x)\n", su,
	//	(u_int32_t)pmap, va, (u_int32_t)m);

	pve = pv_alloc();
	if (pve == NULL)
		panic("pv_insert: no pv entries!");

	pve->pv_pmap = pmap;
	pve->pv_va = va;

	/* add to pv_list */
	PMAP_LOCK_ASSERT(pmap, MA_OWNED);
	mtx_assert(&vm_page_queue_mtx, MA_OWNED);

	TAILQ_INSERT_TAIL(&m->md.pv_list, pve, pv_link);

	//debugf("pv_insert: e\n");
}

/* Destroy pv entry. */
static void
pv_remove(pmap_t pmap, vm_offset_t va, vm_page_t m)
{
	pv_entry_t pve;

	//int su = (pmap == kernel_pmap);
	//debugf("pv_remove: s (su = %d pmap = 0x%08x va = 0x%08x)\n", su, (u_int32_t)pmap, va);

	PMAP_LOCK_ASSERT(pmap, MA_OWNED);
	mtx_assert(&vm_page_queue_mtx, MA_OWNED);

	/* find pv entry */
	TAILQ_FOREACH(pve, &m->md.pv_list, pv_link) {
		if ((pmap == pve->pv_pmap) && (va == pve->pv_va)) {
			/* remove from pv_list */
			TAILQ_REMOVE(&m->md.pv_list, pve, pv_link);
			if (TAILQ_EMPTY(&m->md.pv_list))
				vm_page_flag_clear(m, PG_WRITEABLE);

			/* free pv entry struct */
			pv_free(pve);
			break;
		}
	}

	//debugf("pv_remove: e\n");
}

/*
 * Clean pte entry, try to free page table page if requested.
 *
 * Return 1 if ptbl pages were freed, otherwise return 0.
 */
static int
pte_remove(mmu_t mmu, pmap_t pmap, vm_offset_t va, uint8_t flags)
{
	unsigned int pdir_idx = PDIR_IDX(va);
	unsigned int ptbl_idx = PTBL_IDX(va);
	vm_page_t m;
	pte_t *ptbl;
	pte_t *pte;

	//int su = (pmap == kernel_pmap);
	//debugf("pte_remove: s (su = %d pmap = 0x%08x va = 0x%08x flags = %d)\n",
	//		su, (u_int32_t)pmap, va, flags);

	ptbl = pmap->pm_pdir[pdir_idx];
	KASSERT(ptbl, ("pte_remove: null ptbl"));

	pte = &ptbl[ptbl_idx];

	if (pte == NULL || !PTE_ISVALID(pte))
		return (0);

	/* Get vm_page_t for mapped pte. */
	m = PHYS_TO_VM_PAGE(PTE_PA(pte));

	if (PTE_ISWIRED(pte))
		pmap->pm_stats.wired_count--;

	if (!PTE_ISFAKE(pte)) {
		/* Handle managed entry. */
		if (PTE_ISMANAGED(pte)) {

			/* Handle modified pages. */
			if (PTE_ISMODIFIED(pte))
				vm_page_dirty(m);

			/* Referenced pages. */
			if (PTE_ISREFERENCED(pte))
				vm_page_flag_set(m, PG_REFERENCED);

			/* Remove pv_entry from pv_list. */
			pv_remove(pmap, va, m);
		}
	}

	mtx_lock_spin(&tlbivax_mutex);

	tlb0_flush_entry(va);
	pte->flags = 0;
	pte->rpn = 0;

	mtx_unlock_spin(&tlbivax_mutex);

	pmap->pm_stats.resident_count--;

	if (flags & PTBL_UNHOLD) {
		//debugf("pte_remove: e (unhold)\n");
		return (ptbl_unhold(mmu, pmap, pdir_idx));
	}

	//debugf("pte_remove: e\n");
	return (0);
}

/*
 * Insert PTE for a given page and virtual address.
 */
static void
pte_enter(mmu_t mmu, pmap_t pmap, vm_page_t m, vm_offset_t va, uint32_t flags)
{
	unsigned int pdir_idx = PDIR_IDX(va);
	unsigned int ptbl_idx = PTBL_IDX(va);
	pte_t *ptbl, *pte;

	CTR4(KTR_PMAP, "%s: su = %d pmap = %p va = %p", __func__,
	    pmap == kernel_pmap, pmap, va);

	/* Get the page table pointer. */
	ptbl = pmap->pm_pdir[pdir_idx];

	if (ptbl == NULL) {
		/* Allocate page table pages. */
		ptbl = ptbl_alloc(mmu, pmap, pdir_idx);
	} else {
		/*
		 * Check if there is valid mapping for requested
		 * va, if there is, remove it.
		 */
		pte = &pmap->pm_pdir[pdir_idx][ptbl_idx];
		if (PTE_ISVALID(pte)) {
			pte_remove(mmu, pmap, va, PTBL_HOLD);
		} else {
			/*
			 * pte is not used, increment hold count
			 * for ptbl pages.
			 */
			if (pmap != kernel_pmap)
				ptbl_hold(mmu, pmap, pdir_idx);
		}
	}

	/*
	 * Insert pv_entry into pv_list for mapped page if part of managed
	 * memory.
	 */
        if ((m->flags & PG_FICTITIOUS) == 0) {
		if ((m->flags & PG_UNMANAGED) == 0) {
			flags |= PTE_MANAGED;

			/* Create and insert pv entry. */
			pv_insert(pmap, va, m);
		}
        } else {
		flags |= PTE_FAKE;
	}

	pmap->pm_stats.resident_count++;
	
	mtx_lock_spin(&tlbivax_mutex);

	tlb0_flush_entry(va);
	if (pmap->pm_pdir[pdir_idx] == NULL) {
		/*
		 * If we just allocated a new page table, hook it in
		 * the pdir.
		 */
		pmap->pm_pdir[pdir_idx] = ptbl;
	}
	pte = &(pmap->pm_pdir[pdir_idx][ptbl_idx]);
	pte->rpn = VM_PAGE_TO_PHYS(m) & ~PTE_PA_MASK;
	pte->flags |= (PTE_VALID | flags);

	mtx_unlock_spin(&tlbivax_mutex);
}

/* Return the pa for the given pmap/va. */
static vm_paddr_t
pte_vatopa(mmu_t mmu, pmap_t pmap, vm_offset_t va)
{
	vm_paddr_t pa = 0;
	pte_t *pte;

	pte = pte_find(mmu, pmap, va);
	if ((pte != NULL) && PTE_ISVALID(pte))
		pa = (PTE_PA(pte) | (va & PTE_PA_MASK));
	return (pa);
}

/* Get a pointer to a PTE in a page table. */
static pte_t *
pte_find(mmu_t mmu, pmap_t pmap, vm_offset_t va)
{
	unsigned int pdir_idx = PDIR_IDX(va);
	unsigned int ptbl_idx = PTBL_IDX(va);

	KASSERT((pmap != NULL), ("pte_find: invalid pmap"));

	if (pmap->pm_pdir[pdir_idx])
		return (&(pmap->pm_pdir[pdir_idx][ptbl_idx]));

	return (NULL);
}

/**************************************************************************/
/* PMAP related */
/**************************************************************************/

/*
 * This is called during e500_init, before the system is really initialized.
 */
static void
mmu_booke_bootstrap(mmu_t mmu, vm_offset_t kernelstart, vm_offset_t kernelend)
{
	vm_offset_t phys_kernelend;
	struct mem_region *mp, *mp1;
	int cnt, i, j;
	u_int s, e, sz;
	u_int phys_avail_count;
	vm_size_t physsz, hwphyssz, kstack0_sz;
	vm_offset_t kernel_pdir, kstack0;
	vm_paddr_t kstack0_phys;

	debugf("mmu_booke_bootstrap: entered\n");

	/* Initialize invalidation mutex */
	mtx_init(&tlbivax_mutex, "tlbivax", NULL, MTX_SPIN);

	/* Read TLB0 size and associativity. */
	tlb0_get_tlbconf();

	/* Align kernel start and end address (kernel image). */
	kernelstart = trunc_page(kernelstart);
	kernelend = round_page(kernelend);

	/* Allocate space for the message buffer. */
	msgbufp = (struct msgbuf *)kernelend;
	kernelend += MSGBUF_SIZE;
	debugf(" msgbufp at 0x%08x end = 0x%08x\n", (uint32_t)msgbufp,
	    kernelend);

	kernelend = round_page(kernelend);

	/* Allocate space for ptbl_bufs. */
	ptbl_bufs = (struct ptbl_buf *)kernelend;
	kernelend += sizeof(struct ptbl_buf) * PTBL_BUFS;
	debugf(" ptbl_bufs at 0x%08x end = 0x%08x\n", (uint32_t)ptbl_bufs,
	    kernelend);

	kernelend = round_page(kernelend);

	/* Allocate PTE tables for kernel KVA. */
	kernel_pdir = kernelend;
	kernel_ptbls = (VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS +
	    PDIR_SIZE - 1) / PDIR_SIZE;
	kernelend += kernel_ptbls * PTBL_PAGES * PAGE_SIZE;
	debugf(" kernel ptbls: %d\n", kernel_ptbls);
	debugf(" kernel pdir at 0x%08x end = 0x%08x\n", kernel_pdir, kernelend);

	debugf(" kernelend: 0x%08x\n", kernelend);
	if (kernelend - kernelstart > 0x1000000) {
		kernelend = (kernelend + 0x3fffff) & ~0x3fffff;
		tlb1_mapin_region(kernelstart + 0x1000000,
		    kernload + 0x1000000, kernelend - kernelstart - 0x1000000);
	} else
		kernelend = (kernelend + 0xffffff) & ~0xffffff;

	debugf(" updated kernelend: 0x%08x\n", kernelend);

	/*
	 * Clear the structures - note we can only do it safely after the
	 * possible additional TLB1 translations are in place (above) so that
	 * all range up to the currently calculated 'kernelend' is covered.
	 */
	memset((void *)ptbl_bufs, 0, sizeof(struct ptbl_buf) * PTBL_SIZE);
	memset((void *)kernel_pdir, 0, kernel_ptbls * PTBL_PAGES * PAGE_SIZE);

	/*******************************************************/
	/* Set the start and end of kva. */
	/*******************************************************/
	virtual_avail = kernelend;
	virtual_end = VM_MAX_KERNEL_ADDRESS;

	/* Allocate KVA space for page zero/copy operations. */
	zero_page_va = virtual_avail;
	virtual_avail += PAGE_SIZE;
	zero_page_idle_va = virtual_avail;
	virtual_avail += PAGE_SIZE;
	copy_page_src_va = virtual_avail;
	virtual_avail += PAGE_SIZE;
	copy_page_dst_va = virtual_avail;
	virtual_avail += PAGE_SIZE;
	debugf("zero_page_va = 0x%08x\n", zero_page_va);
	debugf("zero_page_idle_va = 0x%08x\n", zero_page_idle_va);
	debugf("copy_page_src_va = 0x%08x\n", copy_page_src_va);
	debugf("copy_page_dst_va = 0x%08x\n", copy_page_dst_va);

	/* Initialize page zero/copy mutexes. */
	mtx_init(&zero_page_mutex, "mmu_booke_zero_page", NULL, MTX_DEF);
	mtx_init(&copy_page_mutex, "mmu_booke_copy_page", NULL, MTX_DEF);

	/* Allocate KVA space for ptbl bufs. */
	ptbl_buf_pool_vabase = virtual_avail;
	virtual_avail += PTBL_BUFS * PTBL_PAGES * PAGE_SIZE;
	debugf("ptbl_buf_pool_vabase = 0x%08x end = 0x%08x\n",
	    ptbl_buf_pool_vabase, virtual_avail);

	/* Calculate corresponding physical addresses for the kernel region. */
	phys_kernelend = kernload + (kernelend - kernelstart);
	debugf("kernel image and allocated data:\n");
	debugf(" kernload    = 0x%08x\n", kernload);
	debugf(" kernelstart = 0x%08x\n", kernelstart);
	debugf(" kernelend   = 0x%08x\n", kernelend);
	debugf(" kernel size = 0x%08x\n", kernelend - kernelstart);

	if (sizeof(phys_avail) / sizeof(phys_avail[0]) < availmem_regions_sz)
		panic("mmu_booke_bootstrap: phys_avail too small");

	/*
	 * Remove kernel physical address range from avail regions list. Page
	 * align all regions.  Non-page aligned memory isn't very interesting
	 * to us.  Also, sort the entries for ascending addresses.
	 */
	sz = 0;
	cnt = availmem_regions_sz;
	debugf("processing avail regions:\n");
	for (mp = availmem_regions; mp->mr_size; mp++) {
		s = mp->mr_start;
		e = mp->mr_start + mp->mr_size;
		debugf(" %08x-%08x -> ", s, e);
		/* Check whether this region holds all of the kernel. */
		if (s < kernload && e > phys_kernelend) {
			availmem_regions[cnt].mr_start = phys_kernelend;
			availmem_regions[cnt++].mr_size = e - phys_kernelend;
			e = kernload;
		}
		/* Look whether this regions starts within the kernel. */
		if (s >= kernload && s < phys_kernelend) {
			if (e <= phys_kernelend)
				goto empty;
			s = phys_kernelend;
		}
		/* Now look whether this region ends within the kernel. */
		if (e > kernload && e <= phys_kernelend) {
			if (s >= kernload)
				goto empty;
			e = kernload;
		}
		/* Now page align the start and size of the region. */
		s = round_page(s);
		e = trunc_page(e);
		if (e < s)
			e = s;
		sz = e - s;
		debugf("%08x-%08x = %x\n", s, e, sz);

		/* Check whether some memory is left here. */
		if (sz == 0) {
		empty:
			memmove(mp, mp + 1,
			    (cnt - (mp - availmem_regions)) * sizeof(*mp));
			cnt--;
			mp--;
			continue;
		}

		/* Do an insertion sort. */
		for (mp1 = availmem_regions; mp1 < mp; mp1++)
			if (s < mp1->mr_start)
				break;
		if (mp1 < mp) {
			memmove(mp1 + 1, mp1, (char *)mp - (char *)mp1);
			mp1->mr_start = s;
			mp1->mr_size = sz;
		} else {
			mp->mr_start = s;
			mp->mr_size = sz;
		}
	}
	availmem_regions_sz = cnt;

	/*******************************************************/
	/* Steal physical memory for kernel stack from the end */
	/* of the first avail region                           */
	/*******************************************************/
	kstack0_sz = KSTACK_PAGES * PAGE_SIZE;
	kstack0_phys = availmem_regions[0].mr_start +
	    availmem_regions[0].mr_size;
	kstack0_phys -= kstack0_sz;
	availmem_regions[0].mr_size -= kstack0_sz;

	/*******************************************************/
	/* Fill in phys_avail table, based on availmem_regions */
	/*******************************************************/
	phys_avail_count = 0;
	physsz = 0;
	hwphyssz = 0;
	TUNABLE_ULONG_FETCH("hw.physmem", (u_long *) &hwphyssz);

	debugf("fill in phys_avail:\n");
	for (i = 0, j = 0; i < availmem_regions_sz; i++, j += 2) {

		debugf(" region: 0x%08x - 0x%08x (0x%08x)\n",
		    availmem_regions[i].mr_start,
		    availmem_regions[i].mr_start +
		        availmem_regions[i].mr_size,
		    availmem_regions[i].mr_size);

		if (hwphyssz != 0 &&
		    (physsz + availmem_regions[i].mr_size) >= hwphyssz) {
			debugf(" hw.physmem adjust\n");
			if (physsz < hwphyssz) {
				phys_avail[j] = availmem_regions[i].mr_start;
				phys_avail[j + 1] =
				    availmem_regions[i].mr_start +
				    hwphyssz - physsz;
				physsz = hwphyssz;
				phys_avail_count++;
			}
			break;
		}

		phys_avail[j] = availmem_regions[i].mr_start;
		phys_avail[j + 1] = availmem_regions[i].mr_start +
		    availmem_regions[i].mr_size;
		phys_avail_count++;
		physsz += availmem_regions[i].mr_size;
	}
	physmem = btoc(physsz);

	/* Calculate the last available physical address. */
	for (i = 0; phys_avail[i + 2] != 0; i += 2)
		;
	Maxmem = powerpc_btop(phys_avail[i + 1]);

	debugf("Maxmem = 0x%08lx\n", Maxmem);
	debugf("phys_avail_count = %d\n", phys_avail_count);
	debugf("physsz = 0x%08x physmem = %ld (0x%08lx)\n", physsz, physmem,
	    physmem);

	/*******************************************************/
	/* Initialize (statically allocated) kernel pmap. */
	/*******************************************************/
	PMAP_LOCK_INIT(kernel_pmap);
	kptbl_min = VM_MIN_KERNEL_ADDRESS / PDIR_SIZE;

	debugf("kernel_pmap = 0x%08x\n", (uint32_t)kernel_pmap);
	debugf("kptbl_min = %d, kernel_ptbls = %d\n", kptbl_min, kernel_ptbls);
	debugf("kernel pdir range: 0x%08x - 0x%08x\n",
	    kptbl_min * PDIR_SIZE, (kptbl_min + kernel_ptbls) * PDIR_SIZE - 1);

	/* Initialize kernel pdir */
	for (i = 0; i < kernel_ptbls; i++)
		kernel_pmap->pm_pdir[kptbl_min + i] =
		    (pte_t *)(kernel_pdir + (i * PAGE_SIZE * PTBL_PAGES));

	for (i = 0; i < MAXCPU; i++) {
		kernel_pmap->pm_tid[i] = TID_KERNEL;
		
		/* Initialize each CPU's tidbusy entry 0 with kernel_pmap */
		tidbusy[i][0] = kernel_pmap;
	}
	/* Mark kernel_pmap active on all CPUs */
	kernel_pmap->pm_active = ~0;

	/*******************************************************/
	/* Final setup */
	/*******************************************************/

	/* Enter kstack0 into kernel map, provide guard page */
	kstack0 = virtual_avail + KSTACK_GUARD_PAGES * PAGE_SIZE;
	thread0.td_kstack = kstack0;
	thread0.td_kstack_pages = KSTACK_PAGES;

	debugf("kstack_sz = 0x%08x\n", kstack0_sz);
	debugf("kstack0_phys at 0x%08x - 0x%08x\n",
	    kstack0_phys, kstack0_phys + kstack0_sz);
	debugf("kstack0 at 0x%08x - 0x%08x\n", kstack0, kstack0 + kstack0_sz);
	
	virtual_avail += KSTACK_GUARD_PAGES * PAGE_SIZE + kstack0_sz;
	for (i = 0; i < KSTACK_PAGES; i++) {
		mmu_booke_kenter(mmu, kstack0, kstack0_phys);
		kstack0 += PAGE_SIZE;
		kstack0_phys += PAGE_SIZE;
	}
	
	debugf("virtual_avail = %08x\n", virtual_avail);
	debugf("virtual_end   = %08x\n", virtual_end);

	debugf("mmu_booke_bootstrap: exit\n");
}

/*
 * Get the physical page address for the given pmap/virtual address.
 */
static vm_paddr_t
mmu_booke_extract(mmu_t mmu, pmap_t pmap, vm_offset_t va)
{
	vm_paddr_t pa;

	PMAP_LOCK(pmap);
	pa = pte_vatopa(mmu, pmap, va);
	PMAP_UNLOCK(pmap);

	return (pa);
}

/*
 * Extract the physical page address associated with the given
 * kernel virtual address.
 */
static vm_paddr_t
mmu_booke_kextract(mmu_t mmu, vm_offset_t va)
{

	return (pte_vatopa(mmu, kernel_pmap, va));
}

/*
 * Initialize the pmap module.
 * Called by vm_init, to initialize any structures that the pmap
 * system needs to map virtual memory.
 */
static void
mmu_booke_init(mmu_t mmu)
{
	int shpgperproc = PMAP_SHPGPERPROC;

	/*
	 * Initialize the address space (zone) for the pv entries.  Set a
	 * high water mark so that the system can recover from excessive
	 * numbers of pv entries.
	 */
	pvzone = uma_zcreate("PV ENTRY", sizeof(struct pv_entry), NULL, NULL,
	    NULL, NULL, UMA_ALIGN_PTR, UMA_ZONE_VM | UMA_ZONE_NOFREE);

	TUNABLE_INT_FETCH("vm.pmap.shpgperproc", &shpgperproc);
	pv_entry_max = shpgperproc * maxproc + cnt.v_page_count;

	TUNABLE_INT_FETCH("vm.pmap.pv_entries", &pv_entry_max);
	pv_entry_high_water = 9 * (pv_entry_max / 10);

	uma_zone_set_obj(pvzone, &pvzone_obj, pv_entry_max);

	/* Pre-fill pvzone with initial number of pv entries. */
	uma_prealloc(pvzone, PV_ENTRY_ZONE_MIN);

	/* Initialize ptbl allocation. */
	ptbl_init();
}

/*
 * Map a list of wired pages into kernel virtual address space.  This is
 * intended for temporary mappings which do not need page modification or
 * references recorded.  Existing mappings in the region are overwritten.
 */
static void
mmu_booke_qenter(mmu_t mmu, vm_offset_t sva, vm_page_t *m, int count)
{
	vm_offset_t va;

	va = sva;
	while (count-- > 0) {
		mmu_booke_kenter(mmu, va, VM_PAGE_TO_PHYS(*m));
		va += PAGE_SIZE;
		m++;
	}
}

/*
 * Remove page mappings from kernel virtual address space.  Intended for
 * temporary mappings entered by mmu_booke_qenter.
 */
static void
mmu_booke_qremove(mmu_t mmu, vm_offset_t sva, int count)
{
	vm_offset_t va;

	va = sva;
	while (count-- > 0) {
		mmu_booke_kremove(mmu, va);
		va += PAGE_SIZE;
	}
}

/*
 * Map a wired page into kernel virtual address space.
 */
static void
mmu_booke_kenter(mmu_t mmu, vm_offset_t va, vm_offset_t pa)
{
	unsigned int pdir_idx = PDIR_IDX(va);
	unsigned int ptbl_idx = PTBL_IDX(va);
	uint32_t flags;
	pte_t *pte;

	KASSERT(((va >= VM_MIN_KERNEL_ADDRESS) &&
	    (va <= VM_MAX_KERNEL_ADDRESS)), ("mmu_booke_kenter: invalid va"));

#if 0
	/* assume IO mapping, set I, G bits */
	flags = (PTE_G | PTE_I | PTE_FAKE);

	/* if mapping is within system memory, do not set I, G bits */
	for (i = 0; i < totalmem_regions_sz; i++) {
		if ((pa >= totalmem_regions[i].mr_start) &&
				(pa < (totalmem_regions[i].mr_start +
				       totalmem_regions[i].mr_size))) {
			flags &= ~(PTE_I | PTE_G | PTE_FAKE);
			break;
		}
	}
#else
	flags = 0;
#endif

	flags |= (PTE_SR | PTE_SW | PTE_SX | PTE_WIRED | PTE_VALID);
	flags |= PTE_M;

	pte = &(kernel_pmap->pm_pdir[pdir_idx][ptbl_idx]);

	mtx_lock_spin(&tlbivax_mutex);
	
	if (PTE_ISVALID(pte)) {
	
		CTR1(KTR_PMAP, "%s: replacing entry!", __func__);

		/* Flush entry from TLB0 */
		tlb0_flush_entry(va);
	}

	pte->rpn = pa & ~PTE_PA_MASK;
	pte->flags = flags;

	//debugf("mmu_booke_kenter: pdir_idx = %d ptbl_idx = %d va=0x%08x "
	//		"pa=0x%08x rpn=0x%08x flags=0x%08x\n",
	//		pdir_idx, ptbl_idx, va, pa, pte->rpn, pte->flags);

	/* Flush the real memory from the instruction cache. */
	if ((flags & (PTE_I | PTE_G)) == 0) {
		__syncicache((void *)va, PAGE_SIZE);
	}

	mtx_unlock_spin(&tlbivax_mutex);
}

/*
 * Remove a page from kernel page table.
 */
static void
mmu_booke_kremove(mmu_t mmu, vm_offset_t va)
{
	unsigned int pdir_idx = PDIR_IDX(va);
	unsigned int ptbl_idx = PTBL_IDX(va);
	pte_t *pte;

//	CTR2(KTR_PMAP,("%s: s (va = 0x%08x)\n", __func__, va));

	KASSERT(((va >= VM_MIN_KERNEL_ADDRESS) &&
	    (va <= VM_MAX_KERNEL_ADDRESS)),
	    ("mmu_booke_kremove: invalid va"));

	pte = &(kernel_pmap->pm_pdir[pdir_idx][ptbl_idx]);

	if (!PTE_ISVALID(pte)) {
	
		CTR1(KTR_PMAP, "%s: invalid pte", __func__);

		return;
	}

	mtx_lock_spin(&tlbivax_mutex);

	/* Invalidate entry in TLB0, update PTE. */
	tlb0_flush_entry(va);
	pte->flags = 0;
	pte->rpn = 0;

	mtx_unlock_spin(&tlbivax_mutex);
}

/*
 * Initialize pmap associated with process 0.
 */
static void
mmu_booke_pinit0(mmu_t mmu, pmap_t pmap)
{

	mmu_booke_pinit(mmu, pmap);
	PCPU_SET(curpmap, pmap);
}

/*
 * Initialize a preallocated and zeroed pmap structure,
 * such as one in a vmspace structure.
 */
static void
mmu_booke_pinit(mmu_t mmu, pmap_t pmap)
{
	int i;

	CTR4(KTR_PMAP, "%s: pmap = %p, proc %d '%s'", __func__, pmap,
	    curthread->td_proc->p_pid, curthread->td_proc->p_comm);

	KASSERT((pmap != kernel_pmap), ("pmap_pinit: initializing kernel_pmap"));

	PMAP_LOCK_INIT(pmap);
	for (i = 0; i < MAXCPU; i++)
		pmap->pm_tid[i] = TID_NONE;
	pmap->pm_active = 0;
	bzero(&pmap->pm_stats, sizeof(pmap->pm_stats));
	bzero(&pmap->pm_pdir, sizeof(pte_t *) * PDIR_NENTRIES);
	TAILQ_INIT(&pmap->pm_ptbl_list);
}

/*
 * Release any resources held by the given physical map.
 * Called when a pmap initialized by mmu_booke_pinit is being released.
 * Should only be called if the map contains no valid mappings.
 */
static void
mmu_booke_release(mmu_t mmu, pmap_t pmap)
{

	printf("mmu_booke_release: s\n");

	KASSERT(pmap->pm_stats.resident_count == 0,
	    ("pmap_release: pmap resident count %ld != 0",
	    pmap->pm_stats.resident_count));

	PMAP_LOCK_DESTROY(pmap);
}

#if 0
/* Not needed, kernel page tables are statically allocated. */
void
mmu_booke_growkernel(vm_offset_t maxkvaddr)
{
}
#endif

/*
 * Insert the given physical page at the specified virtual address in the
 * target physical map with the protection requested. If specified the page
 * will be wired down.
 */
static void
mmu_booke_enter(mmu_t mmu, pmap_t pmap, vm_offset_t va, vm_page_t m,
    vm_prot_t prot, boolean_t wired)
{

	vm_page_lock_queues();
	PMAP_LOCK(pmap);
	mmu_booke_enter_locked(mmu, pmap, va, m, prot, wired);
	vm_page_unlock_queues();
	PMAP_UNLOCK(pmap);
}

static void
mmu_booke_enter_locked(mmu_t mmu, pmap_t pmap, vm_offset_t va, vm_page_t m,
    vm_prot_t prot, boolean_t wired)
{
	pte_t *pte;
	vm_paddr_t pa;
	uint32_t flags;
	int su, sync;

	pa = VM_PAGE_TO_PHYS(m);
	su = (pmap == kernel_pmap);
	sync = 0;

	//debugf("mmu_booke_enter_locked: s (pmap=0x%08x su=%d tid=%d m=0x%08x va=0x%08x "
	//		"pa=0x%08x prot=0x%08x wired=%d)\n",
	//		(u_int32_t)pmap, su, pmap->pm_tid,
	//		(u_int32_t)m, va, pa, prot, wired);

	if (su) {
		KASSERT(((va >= virtual_avail) &&
		    (va <= VM_MAX_KERNEL_ADDRESS)),
		    ("mmu_booke_enter_locked: kernel pmap, non kernel va"));
	} else {
		KASSERT((va <= VM_MAXUSER_ADDRESS),
		    ("mmu_booke_enter_locked: user pmap, non user va"));
	}

	PMAP_LOCK_ASSERT(pmap, MA_OWNED);

	/*
	 * If there is an existing mapping, and the physical address has not
	 * changed, must be protection or wiring change.
	 */
	if (((pte = pte_find(mmu, pmap, va)) != NULL) &&
	    (PTE_ISVALID(pte)) && (PTE_PA(pte) == pa)) {
	    
		/*
		 * Before actually updating pte->flags we calculate and
		 * prepare its new value in a helper var.
		 */
		flags = pte->flags;
		flags &= ~(PTE_UW | PTE_UX | PTE_SW | PTE_SX | PTE_MODIFIED);

		/* Wiring change, just update stats. */
		if (wired) {
			if (!PTE_ISWIRED(pte)) {
				flags |= PTE_WIRED;
				pmap->pm_stats.wired_count++;
			}
		} else {
			if (PTE_ISWIRED(pte)) {
				flags &= ~PTE_WIRED;
				pmap->pm_stats.wired_count--;
			}
		}

		if (prot & VM_PROT_WRITE) {
			/* Add write permissions. */
			flags |= PTE_SW;
			if (!su)
				flags |= PTE_UW;
		} else {
			/* Handle modified pages, sense modify status. */

			/*
			 * The PTE_MODIFIED flag could be set by underlying
			 * TLB misses since we last read it (above), possibly
			 * other CPUs could update it so we check in the PTE
			 * directly rather than rely on that saved local flags
			 * copy.
			 */
			if (PTE_ISMODIFIED(pte))
				vm_page_dirty(m);
		}

		if (prot & VM_PROT_EXECUTE) {
			flags |= PTE_SX;
			if (!su)
				flags |= PTE_UX;

			/*
			 * Check existing flags for execute permissions: if we
			 * are turning execute permissions on, icache should
			 * be flushed.
			 */
			if ((flags & (PTE_UX | PTE_SX)) == 0)
				sync++;
		}

		flags &= ~PTE_REFERENCED;

		/*
		 * The new flags value is all calculated -- only now actually
		 * update the PTE.
		 */
		mtx_lock_spin(&tlbivax_mutex);

		tlb0_flush_entry(va);
		pte->flags = flags;

		mtx_unlock_spin(&tlbivax_mutex);

	} else {
		/*
		 * If there is an existing mapping, but it's for a different
		 * physical address, pte_enter() will delete the old mapping.
		 */
		//if ((pte != NULL) && PTE_ISVALID(pte))
		//	debugf("mmu_booke_enter_locked: replace\n");
		//else
		//	debugf("mmu_booke_enter_locked: new\n");

		/* Now set up the flags and install the new mapping. */
		flags = (PTE_SR | PTE_VALID);
		flags |= PTE_M;

		if (!su)
			flags |= PTE_UR;

		if (prot & VM_PROT_WRITE) {
			flags |= PTE_SW;
			if (!su)
				flags |= PTE_UW;
		}

		if (prot & VM_PROT_EXECUTE) {
			flags |= PTE_SX;
			if (!su)
				flags |= PTE_UX;
		}

		/* If its wired update stats. */
		if (wired) {
			pmap->pm_stats.wired_count++;
			flags |= PTE_WIRED;
		}

		pte_enter(mmu, pmap, m, va, flags);

		/* Flush the real memory from the instruction cache. */
		if (prot & VM_PROT_EXECUTE)
			sync++;
	}

	if (sync && (su || pmap == PCPU_GET(curpmap))) {
		__syncicache((void *)va, PAGE_SIZE);
		sync = 0;
	}

	if (sync) {
		/* Create a temporary mapping. */
		pmap = PCPU_GET(curpmap);

		va = 0;
		pte = pte_find(mmu, pmap, va);
		KASSERT(pte == NULL, ("%s:%d", __func__, __LINE__));

		flags = PTE_SR | PTE_VALID | PTE_UR | PTE_M;
		
		pte_enter(mmu, pmap, m, va, flags);
		__syncicache((void *)va, PAGE_SIZE);
		pte_remove(mmu, pmap, va, PTBL_UNHOLD);
	}
}

/*
 * Maps a sequence of resident pages belonging to the same object.
 * The sequence begins with the given page m_start.  This page is
 * mapped at the given virtual address start.  Each subsequent page is
 * mapped at a virtual address that is offset from start by the same
 * amount as the page is offset from m_start within the object.  The
 * last page in the sequence is the page with the largest offset from
 * m_start that can be mapped at a virtual address less than the given
 * virtual address end.  Not every virtual page between start and end
 * is mapped; only those for which a resident page exists with the
 * corresponding offset from m_start are mapped.
 */
static void
mmu_booke_enter_object(mmu_t mmu, pmap_t pmap, vm_offset_t start,
    vm_offset_t end, vm_page_t m_start, vm_prot_t prot)
{
	vm_page_t m;
	vm_pindex_t diff, psize;

	psize = atop(end - start);
	m = m_start;
	PMAP_LOCK(pmap);
	while (m != NULL && (diff = m->pindex - m_start->pindex) < psize) {
		mmu_booke_enter_locked(mmu, pmap, start + ptoa(diff), m,
		    prot & (VM_PROT_READ | VM_PROT_EXECUTE), FALSE);
		m = TAILQ_NEXT(m, listq);
	}
	PMAP_UNLOCK(pmap);
}

static void
mmu_booke_enter_quick(mmu_t mmu, pmap_t pmap, vm_offset_t va, vm_page_t m,
    vm_prot_t prot)
{

	PMAP_LOCK(pmap);
	mmu_booke_enter_locked(mmu, pmap, va, m,
	    prot & (VM_PROT_READ | VM_PROT_EXECUTE), FALSE);
	PMAP_UNLOCK(pmap);
}

/*
 * Remove the given range of addresses from the specified map.
 *
 * It is assumed that the start and end are properly rounded to the page size.
 */
static void
mmu_booke_remove(mmu_t mmu, pmap_t pmap, vm_offset_t va, vm_offset_t endva)
{
	pte_t *pte;
	uint8_t hold_flag;

	int su = (pmap == kernel_pmap);

	//debugf("mmu_booke_remove: s (su = %d pmap=0x%08x tid=%d va=0x%08x endva=0x%08x)\n",
	//		su, (u_int32_t)pmap, pmap->pm_tid, va, endva);

	if (su) {
		KASSERT(((va >= virtual_avail) &&
		    (va <= VM_MAX_KERNEL_ADDRESS)),
		    ("mmu_booke_remove: kernel pmap, non kernel va"));
	} else {
		KASSERT((va <= VM_MAXUSER_ADDRESS),
		    ("mmu_booke_remove: user pmap, non user va"));
	}

	if (PMAP_REMOVE_DONE(pmap)) {
		//debugf("mmu_booke_remove: e (empty)\n");
		return;
	}

	hold_flag = PTBL_HOLD_FLAG(pmap);
	//debugf("mmu_booke_remove: hold_flag = %d\n", hold_flag);

	vm_page_lock_queues();
	PMAP_LOCK(pmap);
	for (; va < endva; va += PAGE_SIZE) {
		pte = pte_find(mmu, pmap, va);
		if ((pte != NULL) && PTE_ISVALID(pte))
			pte_remove(mmu, pmap, va, hold_flag);
	}
	PMAP_UNLOCK(pmap);
	vm_page_unlock_queues();

	//debugf("mmu_booke_remove: e\n");
}

/*
 * Remove physical page from all pmaps in which it resides.
 */
static void
mmu_booke_remove_all(mmu_t mmu, vm_page_t m)
{
	pv_entry_t pv, pvn;
	uint8_t hold_flag;

	mtx_assert(&vm_page_queue_mtx, MA_OWNED);

	for (pv = TAILQ_FIRST(&m->md.pv_list); pv != NULL; pv = pvn) {
		pvn = TAILQ_NEXT(pv, pv_link);

		PMAP_LOCK(pv->pv_pmap);
		hold_flag = PTBL_HOLD_FLAG(pv->pv_pmap);
		pte_remove(mmu, pv->pv_pmap, pv->pv_va, hold_flag);
		PMAP_UNLOCK(pv->pv_pmap);
	}
	vm_page_flag_clear(m, PG_WRITEABLE);
}

/*
 * Map a range of physical addresses into kernel virtual address space.
 */
static vm_offset_t
mmu_booke_map(mmu_t mmu, vm_offset_t *virt, vm_offset_t pa_start,
    vm_offset_t pa_end, int prot)
{
	vm_offset_t sva = *virt;
	vm_offset_t va = sva;

	//debugf("mmu_booke_map: s (sva = 0x%08x pa_start = 0x%08x pa_end = 0x%08x)\n",
	//		sva, pa_start, pa_end);

	while (pa_start < pa_end) {
		mmu_booke_kenter(mmu, va, pa_start);
		va += PAGE_SIZE;
		pa_start += PAGE_SIZE;
	}
	*virt = va;

	//debugf("mmu_booke_map: e (va = 0x%08x)\n", va);
	return (sva);
}

/*
 * The pmap must be activated before it's address space can be accessed in any
 * way.
 */
static void
mmu_booke_activate(mmu_t mmu, struct thread *td)
{
	pmap_t pmap;

	pmap = &td->td_proc->p_vmspace->vm_pmap;

	CTR5(KTR_PMAP, "%s: s (td = %p, proc = '%s', id = %d, pmap = 0x%08x)",
	    __func__, td, td->td_proc->p_comm, td->td_proc->p_pid, pmap);

	KASSERT((pmap != kernel_pmap), ("mmu_booke_activate: kernel_pmap!"));

	mtx_lock_spin(&sched_lock);

	atomic_set_int(&pmap->pm_active, PCPU_GET(cpumask));
	PCPU_SET(curpmap, pmap);
	
	if (pmap->pm_tid[PCPU_GET(cpuid)] == TID_NONE)
		tid_alloc(pmap);

	/* Load PID0 register with pmap tid value. */
	mtspr(SPR_PID0, pmap->pm_tid[PCPU_GET(cpuid)]);
	__asm __volatile("isync");

	mtx_unlock_spin(&sched_lock);

	CTR3(KTR_PMAP, "%s: e (tid = %d for '%s')", __func__,
	    pmap->pm_tid[PCPU_GET(cpuid)], td->td_proc->p_comm);
}

/*
 * Deactivate the specified process's address space.
 */
static void
mmu_booke_deactivate(mmu_t mmu, struct thread *td)
{
	pmap_t pmap;

	pmap = &td->td_proc->p_vmspace->vm_pmap;
	
	CTR5(KTR_PMAP, "%s: td=%p, proc = '%s', id = %d, pmap = 0x%08x",
	    __func__, td, td->td_proc->p_comm, td->td_proc->p_pid, pmap);

	atomic_clear_int(&pmap->pm_active, PCPU_GET(cpumask));
	PCPU_SET(curpmap, NULL);
}

/*
 * Copy the range specified by src_addr/len
 * from the source map to the range dst_addr/len
 * in the destination map.
 *
 * This routine is only advisory and need not do anything.
 */
static void
mmu_booke_copy(pmap_t dst_pmap, pmap_t src_pmap, vm_offset_t dst_addr,
    vm_size_t len, vm_offset_t src_addr)
{

}

/*
 * Set the physical protection on the specified range of this map as requested.
 */
static void
mmu_booke_protect(mmu_t mmu, pmap_t pmap, vm_offset_t sva, vm_offset_t eva,
    vm_prot_t prot)
{
	vm_offset_t va;
	vm_page_t m;
	pte_t *pte;

	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		mmu_booke_remove(mmu, pmap, sva, eva);
		return;
	}

	if (prot & VM_PROT_WRITE)
		return;

	vm_page_lock_queues();
	PMAP_LOCK(pmap);
	for (va = sva; va < eva; va += PAGE_SIZE) {
		if ((pte = pte_find(mmu, pmap, va)) != NULL) {
			if (PTE_ISVALID(pte)) {
				m = PHYS_TO_VM_PAGE(PTE_PA(pte));

				mtx_lock_spin(&tlbivax_mutex);

				/* Handle modified pages. */
				if (PTE_ISMODIFIED(pte))
					vm_page_dirty(m);

				/* Referenced pages. */
				if (PTE_ISREFERENCED(pte))
					vm_page_flag_set(m, PG_REFERENCED);

				tlb0_flush_entry(va);
				pte->flags &= ~(PTE_UW | PTE_SW | PTE_MODIFIED |
				    PTE_REFERENCED);

				mtx_unlock_spin(&tlbivax_mutex);
			}
		}
	}
	PMAP_UNLOCK(pmap);
	vm_page_unlock_queues();
}

/*
 * Clear the write and modified bits in each of the given page's mappings.
 */
static void
mmu_booke_remove_write(mmu_t mmu, vm_page_t m)
{
	pv_entry_t pv;
	pte_t *pte;

	mtx_assert(&vm_page_queue_mtx, MA_OWNED);
	if ((m->flags & (PG_FICTITIOUS | PG_UNMANAGED)) != 0 ||
	    (m->flags & PG_WRITEABLE) == 0)
		return;

	TAILQ_FOREACH(pv, &m->md.pv_list, pv_link) {
		PMAP_LOCK(pv->pv_pmap);
		if ((pte = pte_find(mmu, pv->pv_pmap, pv->pv_va)) != NULL) {
			if (PTE_ISVALID(pte)) {
				m = PHYS_TO_VM_PAGE(PTE_PA(pte));

				mtx_lock_spin(&tlbivax_mutex);

				/* Handle modified pages. */
				if (PTE_ISMODIFIED(pte))
					vm_page_dirty(m);

				/* Referenced pages. */
				if (PTE_ISREFERENCED(pte))
					vm_page_flag_set(m, PG_REFERENCED);

				/* Flush mapping from TLB0. */
				pte->flags &= ~(PTE_UW | PTE_SW | PTE_MODIFIED |
				    PTE_REFERENCED);

				mtx_unlock_spin(&tlbivax_mutex);
			}
		}
		PMAP_UNLOCK(pv->pv_pmap);
	}
	vm_page_flag_clear(m, PG_WRITEABLE);
}

static boolean_t
mmu_booke_page_executable(mmu_t mmu, vm_page_t m)
{
	pv_entry_t pv;
	pte_t *pte;
	boolean_t executable;

	executable = FALSE;
	TAILQ_FOREACH(pv, &m->md.pv_list, pv_link) {
		PMAP_LOCK(pv->pv_pmap);
		pte = pte_find(mmu, pv->pv_pmap, pv->pv_va);
		if (pte != NULL && PTE_ISVALID(pte) && (pte->flags & PTE_UX))
			executable = TRUE;
		PMAP_UNLOCK(pv->pv_pmap);
		if (executable)
			break;
	}

	return (executable);
}

/*
 * Atomically extract and hold the physical page with the given
 * pmap and virtual address pair if that mapping permits the given
 * protection.
 */
static vm_page_t
mmu_booke_extract_and_hold(mmu_t mmu, pmap_t pmap, vm_offset_t va,
    vm_prot_t prot)
{
	pte_t *pte;
	vm_page_t m;
	uint32_t pte_wbit;

	m = NULL;
	vm_page_lock_queues();
	PMAP_LOCK(pmap);

	pte = pte_find(mmu, pmap, va);
	if ((pte != NULL) && PTE_ISVALID(pte)) {
		if (pmap == kernel_pmap)
			pte_wbit = PTE_SW;
		else
			pte_wbit = PTE_UW;

		if ((pte->flags & pte_wbit) || ((prot & VM_PROT_WRITE) == 0)) {
			m = PHYS_TO_VM_PAGE(PTE_PA(pte));
			vm_page_hold(m);
		}
	}

	vm_page_unlock_queues();
	PMAP_UNLOCK(pmap);
	return (m);
}

/*
 * Initialize a vm_page's machine-dependent fields.
 */
static void
mmu_booke_page_init(mmu_t mmu, vm_page_t m)
{

	TAILQ_INIT(&m->md.pv_list);
}

/*
 * mmu_booke_zero_page_area zeros the specified hardware page by
 * mapping it into virtual memory and using bzero to clear
 * its contents.
 *
 * off and size must reside within a single page.
 */
static void
mmu_booke_zero_page_area(mmu_t mmu, vm_page_t m, int off, int size)
{
	vm_offset_t va;

	/* XXX KASSERT off and size are within a single page? */

	mtx_lock(&zero_page_mutex);
	va = zero_page_va;

	mmu_booke_kenter(mmu, va, VM_PAGE_TO_PHYS(m));
	bzero((caddr_t)va + off, size);
	mmu_booke_kremove(mmu, va);

	mtx_unlock(&zero_page_mutex);
}

/*
 * mmu_booke_zero_page zeros the specified hardware page.
 */
static void
mmu_booke_zero_page(mmu_t mmu, vm_page_t m)
{

	mmu_booke_zero_page_area(mmu, m, 0, PAGE_SIZE);
}

/*
 * mmu_booke_copy_page copies the specified (machine independent) page by
 * mapping the page into virtual memory and using memcopy to copy the page,
 * one machine dependent page at a time.
 */
static void
mmu_booke_copy_page(mmu_t mmu, vm_page_t sm, vm_page_t dm)
{
	vm_offset_t sva, dva;

	sva = copy_page_src_va;
	dva = copy_page_dst_va;

	mtx_lock(&copy_page_mutex);
	mmu_booke_kenter(mmu, sva, VM_PAGE_TO_PHYS(sm));
	mmu_booke_kenter(mmu, dva, VM_PAGE_TO_PHYS(dm));
	memcpy((caddr_t)dva, (caddr_t)sva, PAGE_SIZE);
	mmu_booke_kremove(mmu, dva);
	mmu_booke_kremove(mmu, sva);
	mtx_unlock(&copy_page_mutex);
}

#if 0
/*
 * Remove all pages from specified address space, this aids process exit
 * speeds. This is much faster than mmu_booke_remove in the case of running
 * down an entire address space. Only works for the current pmap.
 */
void
mmu_booke_remove_pages(pmap_t pmap)
{
}
#endif

/*
 * mmu_booke_zero_page_idle zeros the specified hardware page by mapping it
 * into virtual memory and using bzero to clear its contents. This is intended
 * to be called from the vm_pagezero process only and outside of Giant. No
 * lock is required.
 */
static void
mmu_booke_zero_page_idle(mmu_t mmu, vm_page_t m)
{
	vm_offset_t va;

	va = zero_page_idle_va;
	mmu_booke_kenter(mmu, va, VM_PAGE_TO_PHYS(m));
	bzero((caddr_t)va, PAGE_SIZE);
	mmu_booke_kremove(mmu, va);
}

/*
 * Return whether or not the specified physical page was modified
 * in any of physical maps.
 */
static boolean_t
mmu_booke_is_modified(mmu_t mmu, vm_page_t m)
{
	pte_t *pte;
	pv_entry_t pv;

	mtx_assert(&vm_page_queue_mtx, MA_OWNED);
	if ((m->flags & (PG_FICTITIOUS | PG_UNMANAGED)) != 0)
		return (FALSE);

	TAILQ_FOREACH(pv, &m->md.pv_list, pv_link) {
		PMAP_LOCK(pv->pv_pmap);
		if ((pte = pte_find(mmu, pv->pv_pmap, pv->pv_va)) != NULL) {
			if (!PTE_ISVALID(pte))
				goto make_sure_to_unlock;

			if (PTE_ISMODIFIED(pte)) {
				PMAP_UNLOCK(pv->pv_pmap);
				return (TRUE);
			}
		}
make_sure_to_unlock:
		PMAP_UNLOCK(pv->pv_pmap);
	}
	return (FALSE);
}

/*
 * Return whether or not the specified virtual address is eligible
 * for prefault.
 */
static boolean_t
mmu_booke_is_prefaultable(mmu_t mmu, pmap_t pmap, vm_offset_t addr)
{

	return (FALSE);
}

/*
 * Clear the modify bits on the specified physical page.
 */
static void
mmu_booke_clear_modify(mmu_t mmu, vm_page_t m)
{
	pte_t *pte;
	pv_entry_t pv;

	mtx_assert(&vm_page_queue_mtx, MA_OWNED);
	if ((m->flags & (PG_FICTITIOUS | PG_UNMANAGED)) != 0)
		return;

	TAILQ_FOREACH(pv, &m->md.pv_list, pv_link) {
		PMAP_LOCK(pv->pv_pmap);
		if ((pte = pte_find(mmu, pv->pv_pmap, pv->pv_va)) != NULL) {
			if (!PTE_ISVALID(pte))
				goto make_sure_to_unlock;

			mtx_lock_spin(&tlbivax_mutex);
			
			if (pte->flags & (PTE_SW | PTE_UW | PTE_MODIFIED)) {
				tlb0_flush_entry(pv->pv_va);
				pte->flags &= ~(PTE_SW | PTE_UW | PTE_MODIFIED |
				    PTE_REFERENCED);
			}

			mtx_unlock_spin(&tlbivax_mutex);
		}
make_sure_to_unlock:
		PMAP_UNLOCK(pv->pv_pmap);
	}
}

/*
 * Return a count of reference bits for a page, clearing those bits.
 * It is not necessary for every reference bit to be cleared, but it
 * is necessary that 0 only be returned when there are truly no
 * reference bits set.
 *
 * XXX: The exact number of bits to check and clear is a matter that
 * should be tested and standardized at some point in the future for
 * optimal aging of shared pages.
 */
static int
mmu_booke_ts_referenced(mmu_t mmu, vm_page_t m)
{
	pte_t *pte;
	pv_entry_t pv;
	int count;

	mtx_assert(&vm_page_queue_mtx, MA_OWNED);
	if ((m->flags & (PG_FICTITIOUS | PG_UNMANAGED)) != 0)
		return (0);

	count = 0;
	TAILQ_FOREACH(pv, &m->md.pv_list, pv_link) {
		PMAP_LOCK(pv->pv_pmap);
		if ((pte = pte_find(mmu, pv->pv_pmap, pv->pv_va)) != NULL) {
			if (!PTE_ISVALID(pte))
				goto make_sure_to_unlock;

			if (PTE_ISREFERENCED(pte)) {
				mtx_lock_spin(&tlbivax_mutex);

				tlb0_flush_entry(pv->pv_va);
				pte->flags &= ~PTE_REFERENCED;

				mtx_unlock_spin(&tlbivax_mutex);

				if (++count > 4) {
					PMAP_UNLOCK(pv->pv_pmap);
					break;
				}
			}
		}
make_sure_to_unlock:
		PMAP_UNLOCK(pv->pv_pmap);
	}
	return (count);
}

/*
 * Clear the reference bit on the specified physical page.
 */
static void
mmu_booke_clear_reference(mmu_t mmu, vm_page_t m)
{
	pte_t *pte;
	pv_entry_t pv;

	mtx_assert(&vm_page_queue_mtx, MA_OWNED);
	if ((m->flags & (PG_FICTITIOUS | PG_UNMANAGED)) != 0)
		return;

	TAILQ_FOREACH(pv, &m->md.pv_list, pv_link) {
		PMAP_LOCK(pv->pv_pmap);
		if ((pte = pte_find(mmu, pv->pv_pmap, pv->pv_va)) != NULL) {
			if (!PTE_ISVALID(pte))
				goto make_sure_to_unlock;

			if (PTE_ISREFERENCED(pte)) {
				mtx_lock_spin(&tlbivax_mutex);
				
				tlb0_flush_entry(pv->pv_va);
				pte->flags &= ~PTE_REFERENCED;

				mtx_unlock_spin(&tlbivax_mutex);
			}
		}
make_sure_to_unlock:
		PMAP_UNLOCK(pv->pv_pmap);
	}
}

/*
 * Change wiring attribute for a map/virtual-address pair.
 */
static void
mmu_booke_change_wiring(mmu_t mmu, pmap_t pmap, vm_offset_t va, boolean_t wired)
{
	pte_t *pte;;

	PMAP_LOCK(pmap);
	if ((pte = pte_find(mmu, pmap, va)) != NULL) {
		if (wired) {
			if (!PTE_ISWIRED(pte)) {
				pte->flags |= PTE_WIRED;
				pmap->pm_stats.wired_count++;
			}
		} else {
			if (PTE_ISWIRED(pte)) {
				pte->flags &= ~PTE_WIRED;
				pmap->pm_stats.wired_count--;
			}
		}
	}
	PMAP_UNLOCK(pmap);
}

/*
 * Return true if the pmap's pv is one of the first 16 pvs linked to from this
 * page.  This count may be changed upwards or downwards in the future; it is
 * only necessary that true be returned for a small subset of pmaps for proper
 * page aging.
 */
static boolean_t
mmu_booke_page_exists_quick(mmu_t mmu, pmap_t pmap, vm_page_t m)
{
	pv_entry_t pv;
	int loops;

	mtx_assert(&vm_page_queue_mtx, MA_OWNED);
	if ((m->flags & (PG_FICTITIOUS | PG_UNMANAGED)) != 0)
		return (FALSE);

	loops = 0;
	TAILQ_FOREACH(pv, &m->md.pv_list, pv_link) {
		if (pv->pv_pmap == pmap)
			return (TRUE);

		if (++loops >= 16)
			break;
	}
	return (FALSE);
}

/*
 * Return the number of managed mappings to the given physical page that are
 * wired.
 */
static int
mmu_booke_page_wired_mappings(mmu_t mmu, vm_page_t m)
{
	pv_entry_t pv;
	pte_t *pte;
	int count = 0;

	if ((m->flags & PG_FICTITIOUS) != 0)
		return (count);
	mtx_assert(&vm_page_queue_mtx, MA_OWNED);

	TAILQ_FOREACH(pv, &m->md.pv_list, pv_link) {
		PMAP_LOCK(pv->pv_pmap);
		if ((pte = pte_find(mmu, pv->pv_pmap, pv->pv_va)) != NULL)
			if (PTE_ISVALID(pte) && PTE_ISWIRED(pte))
				count++;
		PMAP_UNLOCK(pv->pv_pmap);
	}

	return (count);
}

static int
mmu_booke_dev_direct_mapped(mmu_t mmu, vm_offset_t pa, vm_size_t size)
{
	int i;
	vm_offset_t va;

	/*
	 * This currently does not work for entries that
	 * overlap TLB1 entries.
	 */
	for (i = 0; i < tlb1_idx; i ++) {
		if (tlb1_iomapped(i, pa, size, &va) == 0)
			return (0);
	}

	return (EFAULT);
}

/*
 * Map a set of physical memory pages into the kernel virtual address space.
 * Return a pointer to where it is mapped. This routine is intended to be used
 * for mapping device memory, NOT real memory.
 */
static void *
mmu_booke_mapdev(mmu_t mmu, vm_offset_t pa, vm_size_t size)
{
	void *res;
	uintptr_t va;
	vm_size_t sz;

	va = (pa >= 0x80000000) ? pa : (0xe2000000 + pa);
	res = (void *)va;

	do {
		sz = 1 << (ilog2(size) & ~1);
		if (bootverbose)
			printf("Wiring VA=%x to PA=%x (size=%x), "
			    "using TLB1[%d]\n", va, pa, sz, tlb1_idx);
		tlb1_set_entry(va, pa, sz, _TLB_ENTRY_IO);
		size -= sz;
		pa += sz;
		va += sz;
	} while (size > 0);

	return (res);
}

/*
 * 'Unmap' a range mapped by mmu_booke_mapdev().
 */
static void
mmu_booke_unmapdev(mmu_t mmu, vm_offset_t va, vm_size_t size)
{
	vm_offset_t base, offset;

	/*
	 * Unmap only if this is inside kernel virtual space.
	 */
	if ((va >= VM_MIN_KERNEL_ADDRESS) && (va <= VM_MAX_KERNEL_ADDRESS)) {
		base = trunc_page(va);
		offset = va & PAGE_MASK;
		size = roundup(offset + size, PAGE_SIZE);
		kmem_free(kernel_map, base, size);
	}
}

/*
 * mmu_booke_object_init_pt preloads the ptes for a given object into the
 * specified pmap. This eliminates the blast of soft faults on process startup
 * and immediately after an mmap.
 */
static void
mmu_booke_object_init_pt(mmu_t mmu, pmap_t pmap, vm_offset_t addr,
    vm_object_t object, vm_pindex_t pindex, vm_size_t size)
{

	VM_OBJECT_LOCK_ASSERT(object, MA_OWNED);
	KASSERT(object->type == OBJT_DEVICE,
	    ("mmu_booke_object_init_pt: non-device object"));
}

/*
 * Perform the pmap work for mincore.
 */
static int
mmu_booke_mincore(mmu_t mmu, pmap_t pmap, vm_offset_t addr)
{

	TODO;
	return (0);
}

/**************************************************************************/
/* TID handling */
/**************************************************************************/

/*
 * Allocate a TID. If necessary, steal one from someone else.
 * The new TID is flushed from the TLB before returning.
 */
static tlbtid_t
tid_alloc(pmap_t pmap)
{
	tlbtid_t tid;
	int thiscpu;

	KASSERT((pmap != kernel_pmap), ("tid_alloc: kernel pmap"));

	CTR2(KTR_PMAP, "%s: s (pmap = %p)", __func__, pmap);

	thiscpu = PCPU_GET(cpuid);

	tid = PCPU_GET(tid_next);
	if (tid > TID_MAX)
		tid = TID_MIN;
	PCPU_SET(tid_next, tid + 1);

	/* If we are stealing TID then clear the relevant pmap's field */
	if (tidbusy[thiscpu][tid] != NULL) {

		CTR2(KTR_PMAP, "%s: warning: stealing tid %d", __func__, tid);
		
		tidbusy[thiscpu][tid]->pm_tid[thiscpu] = TID_NONE;

		/* Flush all entries from TLB0 matching this TID. */
		tid_flush(tid);
	}

	tidbusy[thiscpu][tid] = pmap;
	pmap->pm_tid[thiscpu] = tid;
	__asm __volatile("msync; isync");

	CTR3(KTR_PMAP, "%s: e (%02d next = %02d)", __func__, tid,
	    PCPU_GET(tid_next));

	return (tid);
}

/**************************************************************************/
/* TLB0 handling */
/**************************************************************************/

static void
tlb_print_entry(int i, uint32_t mas1, uint32_t mas2, uint32_t mas3,
    uint32_t mas7)
{
	int as;
	char desc[3];
	tlbtid_t tid;
	vm_size_t size;
	unsigned int tsize;

	desc[2] = '\0';
	if (mas1 & MAS1_VALID)
		desc[0] = 'V';
	else
		desc[0] = ' ';

	if (mas1 & MAS1_IPROT)
		desc[1] = 'P';
	else
		desc[1] = ' ';

	as = (mas1 & MAS1_TS_MASK) ? 1 : 0;
	tid = MAS1_GETTID(mas1);

	tsize = (mas1 & MAS1_TSIZE_MASK) >> MAS1_TSIZE_SHIFT;
	size = 0;
	if (tsize)
		size = tsize2size(tsize);

	debugf("%3d: (%s) [AS=%d] "
	    "sz = 0x%08x tsz = %d tid = %d mas1 = 0x%08x "
	    "mas2(va) = 0x%08x mas3(pa) = 0x%08x mas7 = 0x%08x\n",
	    i, desc, as, size, tsize, tid, mas1, mas2, mas3, mas7);
}

/* Convert TLB0 va and way number to tlb0[] table index. */
static inline unsigned int
tlb0_tableidx(vm_offset_t va, unsigned int way)
{
	unsigned int idx;

	idx = (way * TLB0_ENTRIES_PER_WAY);
	idx += (va & MAS2_TLB0_ENTRY_IDX_MASK) >> MAS2_TLB0_ENTRY_IDX_SHIFT;
	return (idx);
}

/*
 * Invalidate TLB0 entry.
 */
static inline void
tlb0_flush_entry(vm_offset_t va)
{

	CTR2(KTR_PMAP, "%s: s va=0x%08x", __func__, va);

	mtx_assert(&tlbivax_mutex, MA_OWNED);

	__asm __volatile("tlbivax 0, %0" :: "r"(va & MAS2_EPN_MASK));
	__asm __volatile("isync; msync");
	__asm __volatile("tlbsync; msync");

	CTR1(KTR_PMAP, "%s: e", __func__);
}

/* Print out contents of the MAS registers for each TLB0 entry */
void
tlb0_print_tlbentries(void)
{
	uint32_t mas0, mas1, mas2, mas3, mas7;
	int entryidx, way, idx;

	debugf("TLB0 entries:\n");
	for (way = 0; way < TLB0_WAYS; way ++)
		for (entryidx = 0; entryidx < TLB0_ENTRIES_PER_WAY; entryidx++) {

			mas0 = MAS0_TLBSEL(0) | MAS0_ESEL(way);
			mtspr(SPR_MAS0, mas0);
			__asm __volatile("isync");

			mas2 = entryidx << MAS2_TLB0_ENTRY_IDX_SHIFT;
			mtspr(SPR_MAS2, mas2);

			__asm __volatile("isync; tlbre");

			mas1 = mfspr(SPR_MAS1);
			mas2 = mfspr(SPR_MAS2);
			mas3 = mfspr(SPR_MAS3);
			mas7 = mfspr(SPR_MAS7);

			idx = tlb0_tableidx(mas2, way);
			tlb_print_entry(idx, mas1, mas2, mas3, mas7);
		}
}

/**************************************************************************/
/* TLB1 handling */
/**************************************************************************/

/*
 * TLB1 mapping notes:
 *
 * TLB1[0]	CCSRBAR
 * TLB1[1]	Kernel text and data.
 * TLB1[2-15]	Additional kernel text and data mappings (if required), PCI
 *		windows, other devices mappings.
 */

/*
 * Write given entry to TLB1 hardware.
 * Use 32 bit pa, clear 4 high-order bits of RPN (mas7).
 */
static void
tlb1_write_entry(unsigned int idx)
{
	uint32_t mas0, mas7;

	//debugf("tlb1_write_entry: s\n");

	/* Clear high order RPN bits */
	mas7 = 0;

	/* Select entry */
	mas0 = MAS0_TLBSEL(1) | MAS0_ESEL(idx);
	//debugf("tlb1_write_entry: mas0 = 0x%08x\n", mas0);

	mtspr(SPR_MAS0, mas0);
	__asm __volatile("isync");
	mtspr(SPR_MAS1, tlb1[idx].mas1);
	__asm __volatile("isync");
	mtspr(SPR_MAS2, tlb1[idx].mas2);
	__asm __volatile("isync");
	mtspr(SPR_MAS3, tlb1[idx].mas3);
	__asm __volatile("isync");
	mtspr(SPR_MAS7, mas7);
	__asm __volatile("isync; tlbwe; isync; msync");

	//debugf("tlb1_write_entry: e\n");;
}

/*
 * Return the largest uint value log such that 2^log <= num.
 */
static unsigned int
ilog2(unsigned int num)
{
	int lz;

	__asm ("cntlzw %0, %1" : "=r" (lz) : "r" (num));
	return (31 - lz);
}

/*
 * Convert TLB TSIZE value to mapped region size.
 */
static vm_size_t
tsize2size(unsigned int tsize)
{

	/*
	 * size = 4^tsize KB
	 * size = 4^tsize * 2^10 = 2^(2 * tsize - 10)
	 */

	return ((1 << (2 * tsize)) * 1024);
}

/*
 * Convert region size (must be power of 4) to TLB TSIZE value.
 */
static unsigned int
size2tsize(vm_size_t size)
{

	return (ilog2(size) / 2 - 5);
}

/*
 * Register permanent kernel mapping in TLB1.
 *
 * Entries are created starting from index 0 (current free entry is
 * kept in tlb1_idx) and are not supposed to be invalidated.
 */
static int
tlb1_set_entry(vm_offset_t va, vm_offset_t pa, vm_size_t size,
    uint32_t flags)
{
	uint32_t ts, tid;
	int tsize;
	
	if (tlb1_idx >= TLB1_ENTRIES) {
		printf("tlb1_set_entry: TLB1 full!\n");
		return (-1);
	}

	/* Convert size to TSIZE */
	tsize = size2tsize(size);

	tid = (TID_KERNEL << MAS1_TID_SHIFT) & MAS1_TID_MASK;
	/* XXX TS is hard coded to 0 for now as we only use single address space */
	ts = (0 << MAS1_TS_SHIFT) & MAS1_TS_MASK;

	/* XXX LOCK tlb1[] */

	tlb1[tlb1_idx].mas1 = MAS1_VALID | MAS1_IPROT | ts | tid;
	tlb1[tlb1_idx].mas1 |= ((tsize << MAS1_TSIZE_SHIFT) & MAS1_TSIZE_MASK);
	tlb1[tlb1_idx].mas2 = (va & MAS2_EPN_MASK) | flags;

	/* Set supervisor RWX permission bits */
	tlb1[tlb1_idx].mas3 = (pa & MAS3_RPN) | MAS3_SR | MAS3_SW | MAS3_SX;

	tlb1_write_entry(tlb1_idx++);

	/* XXX UNLOCK tlb1[] */

	/*
	 * XXX in general TLB1 updates should be propagated between CPUs,
	 * since current design assumes to have the same TLB1 set-up on all
	 * cores.
	 */
	return (0);
}

static int
tlb1_entry_size_cmp(const void *a, const void *b)
{
	const vm_size_t *sza;
	const vm_size_t *szb;

	sza = a;
	szb = b;
	if (*sza > *szb)
		return (-1);
	else if (*sza < *szb)
		return (1);
	else
		return (0);
}

/*
 * Map in contiguous RAM region into the TLB1 using maximum of
 * KERNEL_REGION_MAX_TLB_ENTRIES entries.
 *
 * If necessary round up last entry size and return total size
 * used by all allocated entries.
 */
vm_size_t
tlb1_mapin_region(vm_offset_t va, vm_offset_t pa, vm_size_t size)
{
	vm_size_t entry_size[KERNEL_REGION_MAX_TLB_ENTRIES];
	vm_size_t mapped_size, sz, esz;
	unsigned int log;
	int i;

	CTR4(KTR_PMAP, "%s: region size = 0x%08x va = 0x%08x pa = 0x%08x",
	    __func__, size, va, pa);

	mapped_size = 0;
	sz = size;
	memset(entry_size, 0, sizeof(entry_size));

	/* Calculate entry sizes. */
	for (i = 0; i < KERNEL_REGION_MAX_TLB_ENTRIES && sz > 0; i++) {

		/* Largest region that is power of 4 and fits within size */
		log = ilog2(sz) / 2;
		esz = 1 << (2 * log);

		/* If this is last entry cover remaining size. */
		if (i ==  KERNEL_REGION_MAX_TLB_ENTRIES - 1) {
			while (esz < sz)
				esz = esz << 2;
		}

		entry_size[i] = esz;
		mapped_size += esz;
		if (esz < sz)
			sz -= esz;
		else
			sz = 0;
	}

	/* Sort entry sizes, required to get proper entry address alignment. */
	qsort(entry_size, KERNEL_REGION_MAX_TLB_ENTRIES,
	    sizeof(vm_size_t), tlb1_entry_size_cmp);

	/* Load TLB1 entries. */
	for (i = 0; i < KERNEL_REGION_MAX_TLB_ENTRIES; i++) {
		esz = entry_size[i];
		if (!esz)
			break;

		CTR5(KTR_PMAP, "%s: entry %d: sz  = 0x%08x (va = 0x%08x "
		    "pa = 0x%08x)", __func__, tlb1_idx, esz, va, pa);

		tlb1_set_entry(va, pa, esz, _TLB_ENTRY_MEM);

		va += esz;
		pa += esz;
	}

	CTR3(KTR_PMAP, "%s: mapped size 0x%08x (wasted space 0x%08x)",
	    __func__, mapped_size, mapped_size - size);

	return (mapped_size);
}

/*
 * TLB1 initialization routine, to be called after the very first
 * assembler level setup done in locore.S.
 */
void
tlb1_init(vm_offset_t ccsrbar)
{
	uint32_t mas0;

	/* TLB1[1] is used to map the kernel. Save that entry. */
	mas0 = MAS0_TLBSEL(1) | MAS0_ESEL(1);
	mtspr(SPR_MAS0, mas0);
	__asm __volatile("isync; tlbre");

	tlb1[1].mas1 = mfspr(SPR_MAS1);
	tlb1[1].mas2 = mfspr(SPR_MAS2);
	tlb1[1].mas3 = mfspr(SPR_MAS3);

	/* Map in CCSRBAR in TLB1[0] */
	tlb1_idx = 0;
	tlb1_set_entry(CCSRBAR_VA, ccsrbar, CCSRBAR_SIZE, _TLB_ENTRY_IO);
	/*
	 * Set the next available TLB1 entry index. Note TLB[1] is reserved
	 * for initial mapping of kernel text+data, which was set early in
	 * locore, we need to skip this [busy] entry.
	 */
	tlb1_idx = 2;

	/* Setup TLB miss defaults */
	set_mas4_defaults();
}

/*
 * Setup MAS4 defaults.
 * These values are loaded to MAS0-2 on a TLB miss.
 */
static void
set_mas4_defaults(void)
{
	uint32_t mas4;

	/* Defaults: TLB0, PID0, TSIZED=4K */
	mas4 = MAS4_TLBSELD0;
	mas4 |= (TLB_SIZE_4K << MAS4_TSIZED_SHIFT) & MAS4_TSIZED_MASK;

	mtspr(SPR_MAS4, mas4);
	__asm __volatile("isync");
}

/*
 * Print out contents of the MAS registers for each TLB1 entry
 */
void
tlb1_print_tlbentries(void)
{
	uint32_t mas0, mas1, mas2, mas3, mas7;
	int i;

	debugf("TLB1 entries:\n");
	for (i = 0; i < TLB1_ENTRIES; i++) {

		mas0 = MAS0_TLBSEL(1) | MAS0_ESEL(i);
		mtspr(SPR_MAS0, mas0);

		__asm __volatile("isync; tlbre");

		mas1 = mfspr(SPR_MAS1);
		mas2 = mfspr(SPR_MAS2);
		mas3 = mfspr(SPR_MAS3);
		mas7 = mfspr(SPR_MAS7);

		tlb_print_entry(i, mas1, mas2, mas3, mas7);
	}
}

/*
 * Print out contents of the in-ram tlb1 table.
 */
void
tlb1_print_entries(void)
{
	int i;

	debugf("tlb1[] table entries:\n");
	for (i = 0; i < TLB1_ENTRIES; i++)
		tlb_print_entry(i, tlb1[i].mas1, tlb1[i].mas2, tlb1[i].mas3, 0);
}

/*
 * Return 0 if the physical IO range is encompassed by one of the
 * the TLB1 entries, otherwise return related error code.
 */
static int
tlb1_iomapped(int i, vm_paddr_t pa, vm_size_t size, vm_offset_t *va)
{
	uint32_t prot;
	vm_paddr_t pa_start;
	vm_paddr_t pa_end;
	unsigned int entry_tsize;
	vm_size_t entry_size;

	*va = (vm_offset_t)NULL;

	/* Skip invalid entries */
	if (!(tlb1[i].mas1 & MAS1_VALID))
		return (EINVAL);

	/*
	 * The entry must be cache-inhibited, guarded, and r/w
	 * so it can function as an i/o page
	 */
	prot = tlb1[i].mas2 & (MAS2_I | MAS2_G);
	if (prot != (MAS2_I | MAS2_G))
		return (EPERM);

	prot = tlb1[i].mas3 & (MAS3_SR | MAS3_SW);
	if (prot != (MAS3_SR | MAS3_SW))
		return (EPERM);

	/* The address should be within the entry range. */
	entry_tsize = (tlb1[i].mas1 & MAS1_TSIZE_MASK) >> MAS1_TSIZE_SHIFT;
	KASSERT((entry_tsize), ("tlb1_iomapped: invalid entry tsize"));

	entry_size = tsize2size(entry_tsize);
	pa_start = tlb1[i].mas3 & MAS3_RPN;
	pa_end = pa_start + entry_size - 1;

	if ((pa < pa_start) || ((pa + size) > pa_end))
		return (ERANGE);

	/* Return virtual address of this mapping. */
	*va = (tlb1[i].mas2 & MAS2_EPN_MASK) + (pa - pa_start);
	return (0);
}
