<<<<<<< HEAD
/*	$OpenBSD: agpvar.h,v 1.4 2006/03/10 21:52:02 matthieu Exp $	*/
=======
/*	$OpenBSD: agpvar.h,v 1.22 2010/05/10 22:06:04 oga Exp $	*/
>>>>>>> origin/master
/*	$NetBSD: agpvar.h,v 1.4 2001/10/01 21:54:48 fvdl Exp $	*/

/*-
 * Copyright (c) 2000 Doug Rabson
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
 *
 *	$FreeBSD: src/sys/pci/agppriv.h,v 1.3 2000/07/12 10:13:04 dfr Exp $
 */

#ifndef _PCI_AGPVAR_H_
#define _PCI_AGPVAR_H_

#include <sys/lock.h>
#include <dev/pci/vga_pcivar.h>

/* #define	AGP_DEBUG */
#ifdef AGP_DEBUG
#define AGP_DPF(fmt, arg...) do { printf("agp: " fmt ,##arg); } while (0)
#else
#define AGP_DPF(fmt, arg...) do {} while (0)
#endif

#define AGPUNIT(x)	minor(x)

<<<<<<< HEAD
struct agp_methods {
	u_int32_t (*get_aperture)(struct vga_pci_softc *);
	int	(*set_aperture)(struct vga_pci_softc *, u_int32_t);
	int	(*bind_page)(struct vga_pci_softc *, off_t, bus_addr_t);
	int	(*unbind_page)(struct vga_pci_softc *, off_t);
	void	(*flush_tlb)(struct vga_pci_softc *);
	int	(*enable)(struct vga_pci_softc *, u_int32_t mode);
	struct agp_memory *
		(*alloc_memory)(struct vga_pci_softc *, int, vsize_t);
	int	(*free_memory)(struct vga_pci_softc *, struct agp_memory *);
	int	(*bind_memory)(struct vga_pci_softc *, struct agp_memory *,
		    off_t);
	int	(*unbind_memory)(struct vga_pci_softc *, struct agp_memory *);
=======
struct agp_attach_args {
	char			*aa_busname;
	struct pci_attach_args	*aa_pa;
};

struct agpbus_attach_args {
	char				*aa_busname; /*so pci doesn't conflict*/
        struct pci_attach_args		*aa_pa;
	const struct agp_methods	*aa_methods;
	bus_addr_t			 aa_apaddr;
	bus_size_t			 aa_apsize;
};

enum agp_acquire_state {
	AGP_ACQUIRE_FREE,
	AGP_ACQUIRE_USER,
	AGP_ACQUIRE_KERNEL
};

/*
 * Data structure to describe an AGP memory allocation.
 */
TAILQ_HEAD(agp_memory_list, agp_memory);
struct agp_memory {
	TAILQ_ENTRY(agp_memory)	 am_link;	/* wiring for the tailq */
	bus_dmamap_t		 am_dmamap;
	bus_dma_segment_t	*am_dmaseg;
	bus_size_t		 am_size;	/* number of bytes allocated */
	bus_size_t		 am_offset;	/* page offset if bound */
	paddr_t			 am_physical;
	int			 am_id;		/* unique id for block */
	int			 am_is_bound;	/* non-zero if bound */
	int			 am_nseg;
	int			 am_type;	/* chipset specific type */
};

/*
 * This structure is used to query the state of the AGP system.
 */
struct agp_info {
	u_int32_t       ai_mode;
	bus_addr_t      ai_aperture_base;
	bus_size_t      ai_aperture_size;
	vsize_t         ai_memory_allowed;
	vsize_t         ai_memory_used;
	u_int32_t       ai_devid;
};

struct agp_memory_info {
        vsize_t         ami_size;       /* size in bytes */
        bus_addr_t      ami_physical;   /* bogus hack for i810 */
        off_t           ami_offset;     /* page offset if bound */
        int             ami_is_bound;   /* non-zero if bound */
};

struct agp_methods {
	void	(*bind_page)(void *, bus_addr_t, paddr_t, int);
	void	(*unbind_page)(void *, bus_addr_t);
	void	(*flush_tlb)(void *);
	void	(*dma_sync)(bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
		    bus_size_t, int);
	int	(*enable)(void *, u_int32_t mode);
	struct agp_memory *
		(*alloc_memory)(void *, int, vsize_t);
	int	(*free_memory)(void *, struct agp_memory *);
	int	(*bind_memory)(void *, struct agp_memory *, bus_size_t);
	int	(*unbind_memory)(void *, struct agp_memory *);
>>>>>>> origin/master
};

/*
 * All chipset drivers must have this at the start of their softc.
 */
<<<<<<< HEAD
=======
struct agp_softc {
	struct device			 sc_dev;

	struct agp_memory_list		 sc_memory; 	/* mem blocks */
	struct rwlock			 sc_lock;	/* GATT access lock */
	const struct agp_methods 	*sc_methods;	/* callbacks */
	void				*sc_chipc;	/* chipset softc */

	bus_dma_tag_t			 sc_dmat;
	pci_chipset_tag_t		 sc_pc;
	pcitag_t			 sc_pcitag;
	bus_addr_t			 sc_apaddr;
	bus_size_t			 sc_apsize;
	pcireg_t			 sc_id;

	int				 sc_opened;
	int				 sc_capoff;			
	int				 sc_nextid;	/* next mem block id */
	enum agp_acquire_state		 sc_state;

	u_int32_t			 sc_maxmem;	/* mem upper bound */
	u_int32_t			 sc_allocated;	/* amount allocated */
};
>>>>>>> origin/master

struct agp_gatt {
	u_int32_t	ag_entries;
	u_int32_t	*ag_virtual;
	bus_addr_t	ag_physical;
	bus_dmamap_t	ag_dmamap;
	bus_dma_segment_t ag_dmaseg;
	size_t		ag_size;
};

struct agp_map;

/*
 * Functions private to the AGP code.
 */
<<<<<<< HEAD

int	agp_find_caps(pci_chipset_tag_t, pcitag_t);
int	agp_map_aperture(struct vga_pci_softc *);
struct agp_gatt *
	agp_alloc_gatt(struct vga_pci_softc *);
void	agp_free_gatt(struct vga_pci_softc *, struct agp_gatt *);
void	agp_flush_cache(void);
int	agp_generic_attach(struct vga_pci_softc *);
int	agp_generic_detach(struct vga_pci_softc *);
int	agp_generic_enable(struct vga_pci_softc *, u_int32_t);
struct agp_memory *
	agp_generic_alloc_memory(struct vga_pci_softc *, int, vsize_t size);
int	agp_generic_free_memory(struct vga_pci_softc *, struct agp_memory *);
int	agp_generic_bind_memory(struct vga_pci_softc *, struct agp_memory *,
	    off_t);
int	agp_generic_unbind_memory(struct vga_pci_softc *, struct agp_memory *);

int	agp_ali_attach(struct vga_pci_softc *, struct pci_attach_args *,
	    struct pci_attach_args *);
int	agp_amd_attach(struct vga_pci_softc *, struct pci_attach_args *,
	    struct pci_attach_args *);
int	agp_i810_attach(struct vga_pci_softc *, struct pci_attach_args *,
	    struct pci_attach_args *);
int	agp_intel_attach(struct vga_pci_softc *, struct pci_attach_args *,
	    struct pci_attach_args *);
int	agp_via_attach(struct vga_pci_softc *, struct pci_attach_args *,
	    struct pci_attach_args *);
int	agp_sis_attach(struct vga_pci_softc *, struct pci_attach_args *,
	    struct pci_attach_args *);

int	agp_alloc_dmamem(bus_dma_tag_t, size_t, int, bus_dmamap_t *,
	    caddr_t *, bus_addr_t *, bus_dma_segment_t *, int, int *);
=======
struct device	*agp_attach_bus(struct pci_attach_args *,
		     const struct agp_methods *, bus_addr_t, bus_size_t,
		     struct device *);
struct agp_gatt *
	agp_alloc_gatt(bus_dma_tag_t, u_int32_t);
void	agp_free_gatt(bus_dma_tag_t, struct agp_gatt *);
void	agp_flush_cache(void);
void	agp_flush_cache_range(vaddr_t, vsize_t);
int	agp_generic_bind_memory(struct agp_softc *, struct agp_memory *,
	    bus_size_t);
int	agp_generic_unbind_memory(struct agp_softc *, struct agp_memory *);
int	agp_init_map(bus_space_tag_t, bus_addr_t, bus_size_t, int, struct
	    agp_map **);
void	agp_destroy_map(struct agp_map *);
int	agp_map_subregion(struct agp_map *, bus_size_t, bus_size_t,
	    bus_space_handle_t *);
void	agp_unmap_subregion(struct agp_map *, bus_space_handle_t,
	    bus_size_t);

int	agp_alloc_dmamem(bus_dma_tag_t, size_t, bus_dmamap_t *,
	    bus_addr_t *, bus_dma_segment_t *);
>>>>>>> origin/master
void	agp_free_dmamem(bus_dma_tag_t, size_t, bus_dmamap_t,
	    bus_dma_segment_t *);
int	agpdev_print(void *, const char *);
int	agpbus_probe(struct agp_attach_args *aa);

<<<<<<< HEAD
=======

int	agp_bus_dma_init(struct agp_softc *, bus_addr_t, bus_addr_t,
	    bus_dma_tag_t *);
void	agp_bus_dma_destroy(struct agp_softc *, bus_dma_tag_t);
void	agp_bus_dma_set_alignment(bus_dma_tag_t, bus_dmamap_t,
	    u_long);
/*
 * Kernel API
 */
/*
 * Find the AGP device and return it.
 */
void	*agp_find_device(int);

/*
 * Return the current owner of the AGP chipset.
 */
enum	 agp_acquire_state agp_state(void *);

/*
 * Query the state of the AGP system.
 */
void	 agp_get_info(void *, struct agp_info *);

/*
 * Acquire the AGP chipset for use by the kernel. Returns EBUSY if the
 * AGP chipset is already acquired by another user.
 */
int	 agp_acquire(void *);

/*
 * Release the AGP chipset.
 */
int	 agp_release(void *);

/*
 * Enable the agp hardware with the relavent mode. The mode bits are
 * defined in <dev/pci/agpreg.h>
 */
int	 agp_enable(void *, u_int32_t);

/*
 * Allocate physical memory suitable for mapping into the AGP
 * aperture.  The value returned is an opaque handle which can be
 * passed to agp_bind(), agp_unbind() or agp_deallocate().
 */
void	*agp_alloc_memory(void *, int, vsize_t);

/*
 * Free memory which was allocated with agp_allocate().
 */
void	 agp_free_memory(void *, void *);

/*
 * Bind memory allocated with agp_allocate() at a given offset within
 * the AGP aperture. Returns EINVAL if the memory is already bound or
 * the offset is not at an AGP page boundary.
 */
int	 agp_bind_memory(void *, void *, off_t);

/*
 * Unbind memory from the AGP aperture. Returns EINVAL if the memory
 * is not bound.
 */
int	 agp_unbind_memory(void *, void *);

/*
 * Retrieve information about a memory block allocated with
 * agp_alloc_memory().
 */
void	 agp_memory_info(void *, void *, struct agp_memory_info *);

>>>>>>> origin/master
#endif /* !_PCI_AGPVAR_H_ */
