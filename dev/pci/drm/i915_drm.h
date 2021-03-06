/* $OpenBSD: i915_drm.h,v 1.20 2014/01/18 08:25:06 jsg Exp $ */
/*
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _I915_DRM_H_
#define _I915_DRM_H_

/* Please note that modifications to all structs defined here are
 * subject to backwards-compatibility constraints.
 */

#include "drm.h"

/* Each region is a minimum of 16k, and there are at most 255 of them.
 */
#define I915_NR_TEX_REGIONS 255	/* table size 2k - maximum due to use
				 * of chars for next/prev indices */
#define I915_LOG_MIN_TEX_REGION_SIZE 14

typedef struct _drm_i915_init {
	enum {
		I915_INIT_DMA = 0x01,
		I915_CLEANUP_DMA = 0x02,
		I915_RESUME_DMA = 0x03
	} func;
	unsigned int mmio_offset;
	int sarea_priv_offset;
	unsigned int ring_start;
	unsigned int ring_end;
	unsigned int ring_size;
	unsigned int front_offset;
	unsigned int back_offset;
	unsigned int depth_offset;
	unsigned int w;
	unsigned int h;
	unsigned int pitch;
	unsigned int pitch_bits;
	unsigned int back_pitch;
	unsigned int depth_pitch;
	unsigned int cpp;
	unsigned int chipset;
} drm_i915_init_t;

typedef struct drm_i915_sarea {
	struct drm_tex_region texList[I915_NR_TEX_REGIONS + 1];
	int last_upload;	/* last time texture was uploaded */
	int last_enqueue;	/* last time a buffer was enqueued */
	int last_dispatch;	/* age of the most recently dispatched buffer */
	int ctxOwner;		/* last context to upload state */
	int texAge;
	int pf_enabled;		/* is pageflipping allowed? */
	int pf_active;
	int pf_current_page;	/* which buffer is being displayed? */
	int perf_boxes;		/* performance boxes to be displayed */
	int width, height;      /* screen size in pixels */

	drm_handle_t front_handle;
	int front_offset;
	int front_size;

	drm_handle_t back_handle;
	int back_offset;
	int back_size;

	drm_handle_t depth_handle;
	int depth_offset;
	int depth_size;

	drm_handle_t tex_handle;
	int tex_offset;
	int tex_size;
	int log_tex_granularity;
	int pitch;
	int rotation;           /* 0, 90, 180 or 270 */
	int rotated_offset;
	int rotated_size;
	int rotated_pitch;
	int virtualX, virtualY;

	unsigned int front_tiled;
	unsigned int back_tiled;
	unsigned int depth_tiled;
	unsigned int rotated_tiled;
	unsigned int rotated2_tiled;

	int planeA_x;
	int planeA_y;
	int planeA_w;
	int planeA_h;
	int planeB_x;
	int planeB_y;
	int planeB_w;
	int planeB_h;

	/* Triple buffering */
	drm_handle_t third_handle;
	int third_offset;
	int third_size;
	unsigned int third_tiled;

	/* buffer object handles for the static buffers.  May change
	 * over the lifetime of the client, though it doesn't in our current
	 * implementation.
	 */
	unsigned int front_bo_handle;
	unsigned int back_bo_handle;
	unsigned int third_bo_handle;
	unsigned int depth_bo_handle;
} drm_i915_sarea_t;

/* Flags for perf_boxes
 */
#define I915_BOX_RING_EMPTY    0x1
#define I915_BOX_FLIP          0x2
#define I915_BOX_WAIT          0x4
#define I915_BOX_TEXTURE_LOAD  0x8
#define I915_BOX_LOST_CONTEXT  0x10

/* I915 specific ioctls
 * The device specific ioctl range is 0x40 to 0x79.
 */
#define DRM_I915_INIT		0x00
#define DRM_I915_FLUSH		0x01
#define DRM_I915_FLIP		0x02
#define DRM_I915_BATCHBUFFER	0x03
#define DRM_I915_IRQ_EMIT	0x04
#define DRM_I915_IRQ_WAIT	0x05
#define DRM_I915_GETPARAM	0x06
#define DRM_I915_SETPARAM	0x07
#define DRM_I915_ALLOC		0x08
#define DRM_I915_FREE		0x09
#define DRM_I915_INIT_HEAP	0x0a
#define DRM_I915_CMDBUFFER	0x0b
#define DRM_I915_DESTROY_HEAP	0x0c
#define DRM_I915_SET_VBLANK_PIPE	0x0d
#define DRM_I915_GET_VBLANK_PIPE	0x0e
#define DRM_I915_VBLANK_SWAP	0x0f
#define DRM_I915_HWS_ADDR	0x11
#define DRM_I915_GEM_INIT	0x13
#define DRM_I915_GEM_EXECBUFFER	0x14
#define DRM_I915_GEM_PIN	0x15
#define DRM_I915_GEM_UNPIN	0x16
#define DRM_I915_GEM_BUSY	0x17
#define DRM_I915_GEM_THROTTLE	0x18
#define DRM_I915_GEM_ENTERVT	0x19
#define DRM_I915_GEM_LEAVEVT	0x1a
#define DRM_I915_GEM_CREATE	0x1b
#define DRM_I915_GEM_PREAD	0x1c
#define DRM_I915_GEM_PWRITE	0x1d
#define DRM_I915_GEM_MMAP	0x1e
#define DRM_I915_GEM_SET_DOMAIN	0x1f
#define DRM_I915_GEM_SW_FINISH	0x20
#define DRM_I915_GEM_SET_TILING	0x21
#define DRM_I915_GEM_GET_TILING	0x22
#define DRM_I915_GEM_GET_APERTURE	0x23
#define DRM_I915_GEM_EXECBUFFER2	0x24	/* MMAP_GTT upstream */
#define DRM_I915_GEM_MADVISE	0x25		/* PIPE_FROM_CRTC_ID upstream */
#define DRM_I915_GEM_MMAP_GTT		0x26	/* MADVISE upstream */
#define DRM_I915_OVERLAY_PUT_IMAGE      0x27
#define DRM_I915_OVERLAY_ATTRS  0x28
#define DRM_I915_GET_PIPE_FROM_CRTC_ID  0x29	/* EXECBUFFER2 upstream */
#define DRM_I915_GET_SPRITE_COLORKEY 0x2a
#define DRM_I915_SET_SPRITE_COLORKEY 0x2b
#define DRM_I915_GEM_WAIT	0x2c
#define DRM_I915_GEM_CONTEXT_CREATE	0x2d
#define DRM_I915_GEM_CONTEXT_DESTROY	0x2e
#define DRM_I915_GEM_SET_CACHING	0x2f
#define DRM_I915_GEM_GET_CACHING	0x30
#define DRM_I915_REG_READ		0x31
#define DRM_I915_GET_RESET_STATS	0x32

#define DRM_IOCTL_I915_INIT		DRM_IOW( DRM_COMMAND_BASE + DRM_I915_INIT, drm_i915_init_t)
#define DRM_IOCTL_I915_FLUSH		DRM_IO ( DRM_COMMAND_BASE + DRM_I915_FLUSH)
#define DRM_IOCTL_I915_FLIP		DRM_IO ( DRM_COMMAND_BASE + DRM_I915_FLIP)
#define DRM_IOCTL_I915_BATCHBUFFER	DRM_IOW( DRM_COMMAND_BASE + DRM_I915_BATCHBUFFER, drm_i915_batchbuffer_t)
#define DRM_IOCTL_I915_IRQ_EMIT         DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_IRQ_EMIT, drm_i915_irq_emit_t)
#define DRM_IOCTL_I915_IRQ_WAIT         DRM_IOW( DRM_COMMAND_BASE + DRM_I915_IRQ_WAIT, drm_i915_irq_wait_t)
#define DRM_IOCTL_I915_GETPARAM         DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GETPARAM, drm_i915_getparam_t)
#define DRM_IOCTL_I915_SETPARAM         DRM_IOW( DRM_COMMAND_BASE + DRM_I915_SETPARAM, drm_i915_setparam_t)
#define DRM_IOCTL_I915_ALLOC            DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_ALLOC, drm_i915_mem_alloc_t)
#define DRM_IOCTL_I915_FREE             DRM_IOW( DRM_COMMAND_BASE + DRM_I915_FREE, drm_i915_mem_free_t)
#define DRM_IOCTL_I915_INIT_HEAP        DRM_IOW( DRM_COMMAND_BASE + DRM_I915_INIT_HEAP, drm_i915_mem_init_heap_t)
#define DRM_IOCTL_I915_CMDBUFFER	DRM_IOW( DRM_COMMAND_BASE + DRM_I915_CMDBUFFER, drm_i915_cmdbuffer_t)
#define DRM_IOCTL_I915_DESTROY_HEAP	DRM_IOW( DRM_COMMAND_BASE + DRM_I915_DESTROY_HEAP, drm_i915_mem_destroy_heap_t)
#define DRM_IOCTL_I915_SET_VBLANK_PIPE	DRM_IOW( DRM_COMMAND_BASE + DRM_I915_SET_VBLANK_PIPE, drm_i915_vblank_pipe_t)
#define DRM_IOCTL_I915_GET_VBLANK_PIPE	DRM_IOR( DRM_COMMAND_BASE + DRM_I915_GET_VBLANK_PIPE, drm_i915_vblank_pipe_t)
#define DRM_IOCTL_I915_VBLANK_SWAP	DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_VBLANK_SWAP, drm_i915_vblank_swap_t)
#define DRM_IOCTL_I915_HWS_ADDR		DRM_IOW(DRM_COMMAND_BASE + DRM_I915_HWS_ADDR, drm_i915_hws_addr_t)
#define DRM_IOCTL_I915_GEM_INIT		DRM_IOW(DRM_COMMAND_BASE + DRM_I915_GEM_INIT, struct drm_i915_gem_init)
#define DRM_IOCTL_I915_GEM_EXECBUFFER	DRM_IOW(DRM_COMMAND_BASE + DRM_I915_GEM_EXECBUFFER, struct drm_i915_gem_execbuffer)
#define DRM_IOCTL_I915_GEM_EXECBUFFER2	DRM_IOW(DRM_COMMAND_BASE + DRM_I915_GEM_EXECBUFFER2, struct drm_i915_gem_execbuffer2)
#define DRM_IOCTL_I915_GEM_PIN		DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_PIN, struct drm_i915_gem_pin)
#define DRM_IOCTL_I915_GEM_UNPIN	DRM_IOW(DRM_COMMAND_BASE + DRM_I915_GEM_UNPIN, struct drm_i915_gem_unpin)
#define DRM_IOCTL_I915_GEM_BUSY		DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_BUSY, struct drm_i915_gem_busy)
#define DRM_IOCTL_I915_GEM_SET_CACHING	DRM_IOW(DRM_COMMAND_BASE + DRM_I915_GEM_SET_CACHING, struct drm_i915_gem_caching)
#define DRM_IOCTL_I915_GEM_GET_CACHING	DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_GET_CACHING, struct drm_i915_gem_caching)
#define DRM_IOCTL_I915_GEM_THROTTLE	DRM_IO ( DRM_COMMAND_BASE + DRM_I915_GEM_THROTTLE)
#define DRM_IOCTL_I915_GEM_ENTERVT	DRM_IO(DRM_COMMAND_BASE + DRM_I915_GEM_ENTERVT)
#define DRM_IOCTL_I915_GEM_LEAVEVT	DRM_IO(DRM_COMMAND_BASE + DRM_I915_GEM_LEAVEVT)
#define DRM_IOCTL_I915_GEM_CREATE	DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_CREATE, struct drm_i915_gem_create)
#define DRM_IOCTL_I915_GEM_PREAD	DRM_IOW (DRM_COMMAND_BASE + DRM_I915_GEM_PREAD, struct drm_i915_gem_pread)
#define DRM_IOCTL_I915_GEM_PWRITE	DRM_IOW (DRM_COMMAND_BASE + DRM_I915_GEM_PWRITE, struct drm_i915_gem_pwrite)
#define DRM_IOCTL_I915_GEM_MMAP		DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_MMAP, struct drm_i915_gem_mmap)
#define DRM_IOCTL_I915_GEM_MMAP_GTT	DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_MMAP_GTT, struct drm_i915_gem_mmap_gtt)
#define DRM_IOCTL_I915_GEM_SET_DOMAIN	DRM_IOW (DRM_COMMAND_BASE + DRM_I915_GEM_SET_DOMAIN, struct drm_i915_gem_set_domain)
#define DRM_IOCTL_I915_GEM_SW_FINISH	DRM_IOW (DRM_COMMAND_BASE + DRM_I915_GEM_SW_FINISH, struct drm_i915_gem_sw_finish)
#define DRM_IOCTL_I915_GEM_SET_TILING	DRM_IOWR (DRM_COMMAND_BASE + DRM_I915_GEM_SET_TILING, struct drm_i915_gem_set_tiling)
#define DRM_IOCTL_I915_GEM_GET_TILING	DRM_IOWR (DRM_COMMAND_BASE + DRM_I915_GEM_GET_TILING, struct drm_i915_gem_get_tiling)
#define DRM_IOCTL_I915_GEM_GET_APERTURE	DRM_IOR  (DRM_COMMAND_BASE + DRM_I915_GEM_GET_APERTURE, struct drm_i915_gem_get_aperture)
#define DRM_IOCTL_I915_GET_PIPE_FROM_CRTC_ID	DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GET_PIPE_FROM_CRTC_ID, struct drm_i915_get_pipe_from_crtc_id)
#define DRM_IOCTL_I915_GEM_MADVISE	DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_MADVISE, struct drm_i915_gem_madvise)
#define DRM_IOCTL_I915_OVERLAY_PUT_IMAGE	DRM_IOW(DRM_COMMAND_BASE + DRM_I915_OVERLAY_PUT_IMAGE, struct drm_intel_overlay_put_image)
#define DRM_IOCTL_I915_OVERLAY_ATTRS	DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_OVERLAY_ATTRS, struct drm_intel_overlay_attrs)
#define DRM_IOCTL_I915_SET_SPRITE_COLORKEY	DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_SET_SPRITE_COLORKEY, struct drm_intel_sprite_colorkey)
#define DRM_IOCTL_I915_GET_SPRITE_COLORKEY	DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GET_SPRITE_COLORKEY, struct drm_intel_sprite_colorkey)
#define DRM_IOCTL_I915_GEM_WAIT		DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_WAIT, struct drm_i915_gem_wait)
#define DRM_IOCTL_I915_GEM_CONTEXT_CREATE	DRM_IOWR (DRM_COMMAND_BASE + DRM_I915_GEM_CONTEXT_CREATE, struct drm_i915_gem_context_create)
#define DRM_IOCTL_I915_GEM_CONTEXT_DESTROY	DRM_IOW (DRM_COMMAND_BASE + DRM_I915_GEM_CONTEXT_DESTROY, struct drm_i915_gem_context_destroy)
#define DRM_IOCTL_I915_REG_READ			DRM_IOWR (DRM_COMMAND_BASE + DRM_I915_REG_READ, struct drm_i915_reg_read)
#define DRM_IOCTL_I915_GET_RESET_STATS		DRM_IOWR (DRM_COMMAND_BASE + DRM_I915_GET_RESET_STATS, struct drm_i915_reset_stats)

/* Allow drivers to submit batchbuffers directly to hardware, relying
 * on the security mechanisms provided by hardware.
 */
typedef struct drm_i915_batchbuffer {
	int start;		/* agp offset */
	int used;		/* nr bytes in use */
	int DR1;		/* hw flags for GFX_OP_DRAWRECT_INFO */
	int DR4;		/* window origin for GFX_OP_DRAWRECT_INFO */
	int num_cliprects;	/* mulitpass with multiple cliprects? */
	struct drm_clip_rect __user *cliprects;	/* pointer to userspace cliprects */
} drm_i915_batchbuffer_t;

/* As above, but pass a pointer to userspace buffer which can be
 * validated by the kernel prior to sending to hardware.
 */
typedef struct _drm_i915_cmdbuffer {
	char __user *buf;	/* pointer to userspace command buffer */
	int sz;			/* nr bytes in buf */
	int DR1;		/* hw flags for GFX_OP_DRAWRECT_INFO */
	int DR4;		/* window origin for GFX_OP_DRAWRECT_INFO */
	int num_cliprects;	/* mulitpass with multiple cliprects? */
	struct drm_clip_rect __user *cliprects;	/* pointer to userspace cliprects */
} drm_i915_cmdbuffer_t;

/* Userspace can request & wait on irq's:
 */
typedef struct drm_i915_irq_emit {
	int __user *irq_seq;
} drm_i915_irq_emit_t;

typedef struct drm_i915_irq_wait {
	int irq_seq;
} drm_i915_irq_wait_t;

/* Ioctl to query kernel params:
 */
#define I915_PARAM_IRQ_ACTIVE            1
#define I915_PARAM_ALLOW_BATCHBUFFER     2
#define I915_PARAM_LAST_DISPATCH         3
#define I915_PARAM_CHIPSET_ID            4
#define I915_PARAM_HAS_GEM		 5
#define I915_PARAM_NUM_FENCES_AVAIL      6
#define I915_PARAM_HAS_OVERLAY           7
#define I915_PARAM_HAS_PAGEFLIPPING	 8
#define I915_PARAM_HAS_EXECBUF2          9
#define I915_PARAM_HAS_BSD		 10
#define I915_PARAM_HAS_BLT		 11
#define I915_PARAM_HAS_RELAXED_FENCING	 12
#define I915_PARAM_HAS_COHERENT_RINGS	 13
#define I915_PARAM_HAS_EXEC_CONSTANTS	 14
#define I915_PARAM_HAS_RELAXED_DELTA	 15
#define I915_PARAM_HAS_GEN7_SOL_RESET	 16
#define I915_PARAM_HAS_LLC		 17
#define I915_PARAM_HAS_ALIASING_PPGTT	 18
#define I915_PARAM_HAS_WAIT_TIMEOUT	 19
#define I915_PARAM_HAS_SEMAPHORES	 20
#define I915_PARAM_HAS_PRIME_VMAP_FLUSH	 21
#define I915_PARAM_HAS_VEBOX		 22
#define I915_PARAM_HAS_SECURE_BATCHES	 23
#define I915_PARAM_HAS_PINNED_BATCHES	 24

typedef struct drm_i915_getparam {
	int param;
	int __user *value;
} drm_i915_getparam_t;

/* Ioctl to set kernel params:
 */
#define I915_SETPARAM_USE_MI_BATCHBUFFER_START            1
#define I915_SETPARAM_TEX_LRU_LOG_GRANULARITY             2
#define I915_SETPARAM_ALLOW_BATCHBUFFER                   3
#define I915_SETPARAM_NUM_USED_FENCES                     4

typedef struct drm_i915_setparam {
	int param;
	int value;
} drm_i915_setparam_t;

/* A memory manager for regions of shared memory:
 */
#define I915_MEM_REGION_AGP 1

typedef struct drm_i915_mem_alloc {
	int region;
	int alignment;
	int size;
	int __user *region_offset;	/* offset from start of fb or agp */
} drm_i915_mem_alloc_t;

typedef struct drm_i915_mem_free {
	int region;
	int region_offset;
} drm_i915_mem_free_t;

typedef struct drm_i915_mem_init_heap {
	int region;
	int size;
	int start;
} drm_i915_mem_init_heap_t;

/* Allow memory manager to be torn down and re-initialized (eg on
 * rotate):
 */
typedef struct drm_i915_mem_destroy_heap {
	int region;
} drm_i915_mem_destroy_heap_t;

/* Allow X server to configure which pipes to monitor for vblank signals
 */
#define	DRM_I915_VBLANK_PIPE_A	1
#define	DRM_I915_VBLANK_PIPE_B	2

typedef struct drm_i915_vblank_pipe {
	int pipe;
} drm_i915_vblank_pipe_t;

/* Schedule buffer swap at given vertical blank:
 */
typedef struct drm_i915_vblank_swap {
	drm_drawable_t drawable;
	enum drm_vblank_seq_type seqtype;
	unsigned int sequence;
} drm_i915_vblank_swap_t;

typedef struct drm_i915_hws_addr {
	uint64_t addr;
} drm_i915_hws_addr_t;

struct drm_i915_gem_init {
	/**
	 * Beginning offset in the GTT to be managed by the DRM memory
	 * manager.
	 */
	uint64_t gtt_start;
	/**
	 * Ending offset in the GTT to be managed by the DRM memory
	 * manager.
	 */
	uint64_t gtt_end;
};

struct drm_i915_gem_create {
	/**
	 * Requested size for the object.
	 *
	 * The (page-aligned) allocated size for the object will be returned.
	 */
	uint64_t size;
	/**
	 * Returned handle for the object.
	 *
	 * Object handles are nonzero.
	 */
	uint32_t handle;
	uint32_t pad;
};

struct drm_i915_gem_pread {
	/** Handle for the object being read. */
	uint32_t handle;
	uint32_t pad;
	/** Offset into the object to read from */
	uint64_t offset;
	/** Length of data to read */
	uint64_t size;
	/**
	 * Pointer to write the data into.
	 *
	 * This is a fixed-size type for 32/64 compatibility.
	 */
	uint64_t data_ptr;
};

struct drm_i915_gem_pwrite {
	/** Handle for the object being written to. */
	uint32_t handle;
	uint32_t pad;
	/** Offset into the object to write to */
	uint64_t offset;
	/** Length of data to write */
	uint64_t size;
	/**
	 * Pointer to read the data from.
	 *
	 * This is a fixed-size type for 32/64 compatibility.
	 */
	uint64_t data_ptr;
};

struct drm_i915_gem_mmap {
	/** Handle for the object being mapped. */
	uint32_t handle;
	uint32_t pad;
	/** Offset in the object to map. */
	uint64_t offset;
	/**
	 * Length of data to map.
	 *
	 * The value will be page-aligned.
	 */
	uint64_t size;
	/**
	 * Returned pointer the data was mapped at.
	 *
	 * This is a fixed-size type for 32/64 compatibility.
	 */
	uint64_t addr_ptr;
};

struct drm_i915_gem_mmap_gtt {
	/** Handle for the object being mapped. */
	uint32_t handle;
	uint32_t pad;
	/**
	 * Fake offset to use for subsequent mmap call
	 *
	 * This is a fixed-size type for 32/64 compatibility.
	 */
	uint64_t offset;
};

struct drm_i915_gem_set_domain {
	/** Handle for the object */
	uint32_t handle;

	/** New read domains */
	uint32_t read_domains;

	/** New write domain */
	uint32_t write_domain;
};

struct drm_i915_gem_sw_finish {
	/** Handle for the object */
	uint32_t handle;
};

struct drm_i915_gem_relocation_entry {
	/**
	 * Handle of the buffer being pointed to by this relocation entry.
	 *
	 * It's appealing to make this be an index into the mm_validate_entry
	 * list to refer to the buffer, but this allows the driver to create
	 * a relocation list for state buffers and not re-write it per
	 * exec using the buffer.
	 */
	uint32_t target_handle;

	/**
	 * Value to be added to the offset of the target buffer to make up
	 * the relocation entry.
	 */
	uint32_t delta;

	/** Offset in the buffer the relocation entry will be written into */
	uint64_t offset;

	/**
	 * Offset value of the target buffer that the relocation entry was last
	 * written as.
	 *
	 * If the buffer has the same offset as last time, we can skip syncing
	 * and writing the relocation.  This value is written back out by
	 * the execbuffer ioctl when the relocation is written.
	 */
	uint64_t presumed_offset;

	/**
	 * Target memory domains read by this operation.
	 */
	uint32_t read_domains;

	/**
	 * Target memory domains written by this operation.
	 *
	 * Note that only one domain may be written by the whole
	 * execbuffer operation, so that where there are conflicts,
	 * the application will get -EINVAL back.
	 */
	uint32_t write_domain;
};

/** @{
 * Intel memory domains
 *
 * Most of these just align with the various caches in
 * the system and are used to flush and invalidate as
 * objects end up cached in different domains.
 */
/** CPU cache */
#define I915_GEM_DOMAIN_CPU		0x00000001
/** Render cache, used by 2D and 3D drawing */
#define I915_GEM_DOMAIN_RENDER		0x00000002
/** Sampler cache, used by texture engine */
#define I915_GEM_DOMAIN_SAMPLER		0x00000004
/** Command queue, used to load batch buffers */
#define I915_GEM_DOMAIN_COMMAND		0x00000008
/** Instruction cache, used by shader programs */
#define I915_GEM_DOMAIN_INSTRUCTION	0x00000010
/** Vertex address cache */
#define I915_GEM_DOMAIN_VERTEX		0x00000020
/** GTT domain - aperture and scanout */
#define I915_GEM_DOMAIN_GTT		0x00000040
/** @} */

struct drm_i915_gem_exec_object2 {
	/**
	 * User's handle for a buffer to be bound into the GTT for this
	 * operation.
	 */
	u_int32_t handle;

	/** Number of relocations to be performed on this buffer */
	u_int32_t relocation_count;
	/**
	 * Pointer to array of struct drm_i915_gem_relocation_entry containing
	 * the relocations to be performed in this buffer.
	 */
	u_int64_t relocs_ptr;

	/** Required alignment in graphics aperture */
	u_int64_t alignment;

	/**
	 * Returned value of the updated offset of the object, for future
	 * presumed_offset writes.
	 */
	u_int64_t offset;

#define EXEC_OBJECT_NEEDS_FENCE (1<<0)
	u_int64_t flags;
	u_int64_t rsvd1;
	u_int64_t rsvd2;
};

struct drm_i915_gem_execbuffer2 {
	/**
	 * List of gem_exec_object2 structs
	 */
	u_int64_t buffers_ptr;
	u_int32_t buffer_count;

	/** Offset in the batchbuffer to start execution from. */
	u_int32_t batch_start_offset;
	/** Bytes used in batchbuffer from batch_start_offset */
	u_int32_t batch_len;
#ifndef __OpenBSD__
	u_int32_t DR1;
	u_int32_t DR4;
	u_int32_t num_cliprects;
	/** This is a struct drm_clip_rect *cliprects */
	u_int64_t cliprects_ptr;
#endif
#define I915_EXEC_RING_MASK              (7<<0)
#define I915_EXEC_DEFAULT                (0<<0)
#define I915_EXEC_RENDER                 (1<<0)
#define I915_EXEC_BSD                    (2<<0)
#define I915_EXEC_BLT                    (3<<0)
#define I915_EXEC_VEBOX                  (4<<0)

/* Used for switching the constants addressing mode on gen4+ RENDER ring.
 * Gen6+ only supports relative addressing to dynamic state (default) and
 * absolute addressing.
 *
 * These flags are ignored for the BSD and BLT rings.
 */
#define I915_EXEC_CONSTANTS_MASK 	(3<<6)
#define I915_EXEC_CONSTANTS_REL_GENERAL (0<<6) /* default */
#define I915_EXEC_CONSTANTS_ABSOLUTE 	(1<<6)
#define I915_EXEC_CONSTANTS_REL_SURFACE (2<<6) /* gen4/5 only */
	u_int64_t flags;
	u_int64_t rsvd1;
	u_int64_t rsvd2;
};

/** Resets the SO write offset registers for transform feedback on gen7. */
#define I915_EXEC_GEN7_SOL_RESET	(1<<8)

/** Request a privileged ("secure") batch buffer. Note only available for
 * DRM_ROOT_ONLY | DRM_MASTER processes.
 */
#define I915_EXEC_SECURE		(1<<9)

/** Inform the kernel that the batch is and will always be pinned. This
 * negates the requirement for a workaround to be performed to avoid
 * an incoherent CS (such as can be found on 830/845). If this flag is
 * not passed, the kernel will endeavour to make sure the batch is
 * coherent with the CS before execution. If this flag is passed,
 * userspace assumes the responsibility for ensuring the same.
 */
#define I915_EXEC_IS_PINNED		(1<<10)

#define I915_EXEC_CONTEXT_ID_MASK	(0xffffffff)
#define i915_execbuffer2_set_context_id(eb2, context) \
	(eb2).rsvd1 = context & I915_EXEC_CONTEXT_ID_MASK
#define i915_execbuffer2_get_context_id(eb2) \
	((eb2).rsvd1 & I915_EXEC_CONTEXT_ID_MASK)

struct drm_i915_gem_pin {
	/** Handle of the buffer to be pinned. */
	uint32_t handle;
	uint32_t pad;

	/** alignment required within the aperture */
	uint64_t alignment;

	/** Returned GTT offset of the buffer. */
	uint64_t offset;
};

struct drm_i915_gem_unpin {
	/** Handle of the buffer to be unpinned. */
	uint32_t handle;
	uint32_t pad;
};

struct drm_i915_gem_busy {
	/** Handle of the buffer to check for busy */
	uint32_t handle;

	/** Return busy status (1 if busy, 0 if idle) */
	uint32_t busy;
};

#define I915_CACHING_NONE		0
#define I915_CACHING_CACHED		1

struct drm_i915_gem_caching {
	/**
	 * Handle of the buffer to set/get the caching level of. */
	uint32_t handle;

	/**
	 * Cacheing level to apply or return value
	 *
	 * bits0-15 are for generic caching control (i.e. the above defined
	 * values). bits16-31 are reserved for platform-specific variations
	 * (e.g. l3$ caching on gen7). */
	uint32_t caching;
};

#define I915_TILING_NONE	0
#define I915_TILING_X		1
#define I915_TILING_Y		2

#define I915_BIT_6_SWIZZLE_NONE		0
#define I915_BIT_6_SWIZZLE_9		1
#define I915_BIT_6_SWIZZLE_9_10		2
#define I915_BIT_6_SWIZZLE_9_11		3
#define I915_BIT_6_SWIZZLE_9_10_11	4
/* Not seen by userland */
#define I915_BIT_6_SWIZZLE_UNKNOWN	5
/* Seen by userland. */
#define I915_BIT_6_SWIZZLE_9_17		6
#define I915_BIT_6_SWIZZLE_9_10_17	7

struct drm_i915_gem_set_tiling {
	/** Handle of the buffer to have its tiling state updated */
	uint32_t handle;

	/**
	 * Tiling mode for the object (I915_TILING_NONE, I915_TILING_X,
	 * I915_TILING_Y).
	 *
	 * This value is to be set on request, and will be updated by the
	 * kernel on successful return with the actual chosen tiling layout.
	 *
	 * The tiling mode may be demoted to I915_TILING_NONE when the system
	 * has bit 6 swizzling that can't be managed correctly by GEM.
	 *
	 * Buffer contents become undefined when changing tiling_mode.
	 */
	uint32_t tiling_mode;

	/**
	 * Stride in bytes for the object when in I915_TILING_X or
	 * I915_TILING_Y.
	 */
	uint32_t stride;

	/**
	 * Returned address bit 6 swizzling required for CPU access through
	 * mmap mapping.
	 */
	uint32_t swizzle_mode;
};

struct drm_i915_gem_get_tiling {
	/** Handle of the buffer to get tiling state for. */
	uint32_t handle;

	/**
	 * Current tiling mode for the object (I915_TILING_NONE, I915_TILING_X,
	 * I915_TILING_Y).
	 */
	uint32_t tiling_mode;

	/**
	 * Returned address bit 6 swizzling required for CPU access through
	 * mmap mapping.
	 */
	uint32_t swizzle_mode;
};

struct drm_i915_gem_get_aperture {
	/** Total size of the aperture used by i915_gem_execbuffer, in bytes */
	uint64_t aper_size;

	/**
	 * Available space in the aperture used by i915_gem_execbuffer, in
	 * bytes
	 */
	uint64_t aper_available_size;
};

struct drm_i915_get_pipe_from_crtc_id {
	/** ID of CRTC being requested **/
	uint32_t crtc_id;

	/** pipe of requested CRTC **/
	uint32_t pipe;
};

#define I915_MADV_WILLNEED 0
#define I915_MADV_DONTNEED 1
#define __I915_MADV_PURGED 2 /* internal state */

struct drm_i915_gem_madvise {
	/** Handle of the buffer to change the backing store advice */
	u_int32_t handle;

	/* Advice: either the buffer will be needed again in the near future,
	 *         or wont be and could be discarded under memory pressure.
	 */
	u_int32_t madv;

	/** Whether the backing store still exists. */
	u_int32_t retained;
};

#define I915_OVERLAY_TYPE_MASK 		0xff
#define I915_OVERLAY_YUV_PLANAR 	0x01
#define I915_OVERLAY_YUV_PACKED 	0x02
#define I915_OVERLAY_RGB		0x03

#define I915_OVERLAY_DEPTH_MASK		0xff00
#define I915_OVERLAY_RGB24		0x1000
#define I915_OVERLAY_RGB16		0x2000
#define I915_OVERLAY_RGB15		0x3000
#define I915_OVERLAY_YUV422		0x0100
#define I915_OVERLAY_YUV411		0x0200
#define I915_OVERLAY_YUV420		0x0300
#define I915_OVERLAY_YUV410		0x0400

#define I915_OVERLAY_SWAP_MASK		0xff0000
#define I915_OVERLAY_NO_SWAP		0x000000
#define I915_OVERLAY_UV_SWAP		0x010000
#define I915_OVERLAY_Y_SWAP		0x020000
#define I915_OVERLAY_Y_AND_UV_SWAP	0x030000

#define I915_OVERLAY_FLAGS_MASK		0xff000000
#define I915_OVERLAY_ENABLE		0x01000000

struct drm_intel_overlay_put_image {
	/* various flags and src format description */
	uint32_t flags;
	/* source picture description */
	uint32_t bo_handle;
	/* stride values and offsets are in bytes, buffer relative */
	uint16_t stride_Y; /* stride for packed formats */
	uint16_t stride_UV;
	uint32_t offset_Y; /* offset for packet formats */
	uint32_t offset_U;
	uint32_t offset_V;
	/* in pixels */
	uint16_t src_width;
	uint16_t src_height;
	/* to compensate the scaling factors for partially covered surfaces */
	uint16_t src_scan_width;
	uint16_t src_scan_height;
	/* output crtc description */
	uint32_t crtc_id;
	uint16_t dst_x;
	uint16_t dst_y;
	uint16_t dst_width;
	uint16_t dst_height;
};

/* flags */
#define I915_OVERLAY_UPDATE_ATTRS	(1<<0)
#define I915_OVERLAY_UPDATE_GAMMA	(1<<1)
struct drm_intel_overlay_attrs {
	uint32_t flags;
	uint32_t color_key;
	int32_t brightness;
	uint32_t contrast;
	uint32_t saturation;
	uint32_t gamma0;
	uint32_t gamma1;
	uint32_t gamma2;
	uint32_t gamma3;
	uint32_t gamma4;
	uint32_t gamma5;
};

/*
 * Intel sprite handling
 *
 * Color keying works with a min/mask/max tuple.  Both source and destination
 * color keying is allowed.
 *
 * Source keying:
 * Sprite pixels within the min & max values, masked against the color channels
 * specified in the mask field, will be transparent.  All other pixels will
 * be displayed on top of the primary plane.  For RGB surfaces, only the min
 * and mask fields will be used; ranged compares are not allowed.
 *
 * Destination keying:
 * Primary plane pixels that match the min value, masked against the color
 * channels specified in the mask field, will be replaced by corresponding
 * pixels from the sprite plane.
 *
 * Note that source & destination keying are exclusive; only one can be
 * active on a given plane.
 */

#define I915_SET_COLORKEY_NONE		(1<<0) /* disable color key matching */
#define I915_SET_COLORKEY_DESTINATION	(1<<1)
#define I915_SET_COLORKEY_SOURCE	(1<<2)
struct drm_intel_sprite_colorkey {
	uint32_t plane_id;
	uint32_t min_value;
	uint32_t channel_mask;
	uint32_t max_value;
	uint32_t flags;
};

struct drm_i915_gem_wait {
	/** Handle of BO we shall wait on */
	uint32_t bo_handle;
	uint32_t flags;
	/** Number of nanoseconds to wait, Returns time remaining. */
	int64_t timeout_ns;
};

struct drm_i915_gem_context_create {
	/*  output: id of new context*/
	uint32_t ctx_id;
	uint32_t pad;
};

struct drm_i915_gem_context_destroy {
	uint32_t ctx_id;
	uint32_t pad;
};

struct drm_i915_reg_read {
	uint64_t offset;
	uint64_t val; /* Return value */
};

struct drm_i915_reset_stats {
	uint32_t ctx_id;
	uint32_t flags;

	/* All resets since boot/module reload, for all contexts */
	uint32_t reset_count;

	/* Number of batches lost when active in GPU, for this context */
	uint32_t batch_active;

	/* Number of batches lost pending for execution, for this context */
	uint32_t batch_pending;

	uint32_t pad;
};

#endif				/* _I915_DRM_H_ */
