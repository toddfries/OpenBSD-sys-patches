<<<<<<< HEAD
/*	$OpenBSD: mgx.c,v 1.11 2006/06/02 20:00:54 miod Exp $	*/
=======
/*	$OpenBSD: mgx.c,v 1.16 2009/09/05 14:09:35 miod Exp $	*/
>>>>>>> origin/master
/*
 * Copyright (c) 2003, Miodrag Vallat.
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
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Driver for the Southland Media Systems (now Quantum 3D) MGX and MGXPlus
 * frame buffers.
 *
 * Pretty crude, due to the lack of documentation. Works as a dumb frame
 * buffer in 8 bit mode, although the hardware can run in an 32 bit
 * accelerated mode. Also, interrupts are not handled.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/pmap.h>
#include <machine/cpu.h>
#include <machine/conf.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <machine/fbvar.h>

#include <sparc/dev/sbusvar.h>

/*
 * MGX PROM register layout
 */

#define	MGX_NREG	9
#define	MGX_REG_CRTC	4	/* video control and ramdac */
#define	MGX_REG_CTRL	5	/* control engine */
#define	MGX_REG_VRAM8	8	/* 8-bit memory space */

/*
 * MGX CRTC empirical constants
 */
#if _BYTE_ORDER == _LITTLE_ENDIAN
#define	IO_ADDRESS(x)	(x)
#else
#define	IO_ADDRESS(x)	((x) ^ 0x03)
#endif
#define	CRTC_INDEX		IO_ADDRESS(0x03c4)
#define	CRTC_DATA		IO_ADDRESS(0x03c5)
#define	CD_DISABLEVIDEO	0x0020
#define	CMAP_READ_INDEX		IO_ADDRESS(0x03c7)
#define	CMAP_WRITE_INDEX	IO_ADDRESS(0x03c8)
#define	CMAP_DATA		IO_ADDRESS(0x03c9)

/* per-display variables */
struct mgx_softc {
	struct	sunfb	sc_sunfb;	/* common base device */
	struct	rom_reg sc_phys;
	u_int8_t	sc_cmap[256 * 3];	/* shadow colormap */
	volatile u_int8_t *sc_vidc;	/* ramdac registers */
};

void	mgx_burner(void *, u_int ,u_int);
int	mgx_getcmap(u_int8_t *, struct wsdisplay_cmap *);
int	mgx_ioctl(void *, u_long, caddr_t, int, struct proc *);
void	mgx_loadcmap(struct mgx_softc *, int, int);
paddr_t	mgx_mmap(void *, off_t, int);
int	mgx_putcmap(u_int8_t *, struct wsdisplay_cmap *);
void	mgx_setcolor(void *, u_int, u_int8_t, u_int8_t, u_int8_t);

struct wsdisplay_accessops mgx_accessops = {
	mgx_ioctl,
	mgx_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL,	/* load_font */
	NULL,	/* scrollback */
	NULL,	/* getchar */
	mgx_burner,
	NULL	/* pollc */
};

<<<<<<< HEAD
=======
int	mgx_getcmap(u_int8_t *, struct wsdisplay_cmap *);
void	mgx_loadcmap(struct mgx_softc *, int, int);
int	mgx_putcmap(u_int8_t *, struct wsdisplay_cmap *);
void	mgx_setcolor(void *, u_int, u_int8_t, u_int8_t, u_int8_t);

int	mgx_ras_copycols(void *, int, int, int, int);
int	mgx_ras_copyrows(void *, int, int, int);
int	mgx_ras_do_cursor(struct rasops_info *);
int	mgx_ras_erasecols(void *, int, int, int, long int);
int	mgx_ras_eraserows(void *, int, int, long int);
void	mgx_ras_init(struct mgx_softc *, uint);

uint8_t	mgx_read_1(vaddr_t, uint);
uint16_t mgx_read_2(vaddr_t, uint);
void	mgx_write_1(vaddr_t, uint, uint8_t);
void	mgx_write_4(vaddr_t, uint, uint32_t);

int	mgx_wait_engine(struct mgx_softc *);
int	mgx_wait_fifo(struct mgx_softc *, uint);

/*
 * Attachment Glue
 */

>>>>>>> origin/master
int	mgxmatch(struct device *, void *, void *);
void	mgxattach(struct device *, struct device *, void *);

struct cfattach mgx_ca = {
	sizeof(struct mgx_softc), mgxmatch, mgxattach
};

struct cfdriver mgx_cd = {
	NULL, "mgx", DV_DULL
};

/*
 * Match an MGX or MGX+ card.
 */
int
mgxmatch(struct device *parent, void *vcf, void *aux)
{
	struct confargs *ca = aux;
	struct romaux *ra = &ca->ca_ra;

	if (strcmp(ra->ra_name, "SMSI,mgx") != 0 &&
	    strcmp(ra->ra_name, "mgx") != 0)
		return (0);

	return (1);
}

/*
 * Attach an MGX frame buffer.
 * This will keep the frame buffer in the actual PROM mode, and attach
 * a wsdisplay child device to itself.
 */
void
mgxattach(struct device *parent, struct device *self, void *args)
{
	struct mgx_softc *sc = (struct mgx_softc *)self;
	struct confargs *ca = args;
	int node, fbsize;
	int isconsole;

	node = ca->ca_ra.ra_node;

	printf(": %s", getpropstring(node, "model"));

	isconsole = node == fbnode;

	/* Check registers */
	if (ca->ca_ra.ra_nreg < MGX_NREG) {
		printf("\n%s: expected %d registers, got %d\n",
		    self->dv_xname, MGX_NREG, ca->ca_ra.ra_nreg);
		return;
	}

	sc->sc_vidc = (volatile u_int8_t *)mapiodev(
	    &ca->ca_ra.ra_reg[MGX_REG_CRTC], 0, PAGE_SIZE);

	/* enable video */
	mgx_burner(sc, 1, 0);

	fb_setsize(&sc->sc_sunfb, 8, 1152, 900, node, ca->ca_bustype);

	/* Sanity check frame buffer memory */
	fbsize = getpropint(node, "fb_size", 0);
	if (fbsize != 0 && sc->sc_sunfb.sf_fbsize > fbsize) {
		printf("\n%s: expected at least %d bytes of vram, but card "
		    "only provides %d\n",
		    self->dv_xname, sc->sc_sunfb.sf_fbsize, fbsize);
		return;
	}

	/* Map the frame buffer memory area we're interested in */
	sc->sc_phys = ca->ca_ra.ra_reg[MGX_REG_VRAM8];
	sc->sc_sunfb.sf_ro.ri_bits = mapiodev(&sc->sc_phys,
	    0, round_page(sc->sc_sunfb.sf_fbsize));
	sc->sc_sunfb.sf_ro.ri_hw = sc;

	printf(", %dx%d\n",
	    sc->sc_sunfb.sf_width, sc->sc_sunfb.sf_height);

	fbwscons_init(&sc->sc_sunfb, isconsole);

	bzero(sc->sc_cmap, sizeof(sc->sc_cmap));
	fbwscons_setcolormap(&sc->sc_sunfb, mgx_setcolor);

<<<<<<< HEAD
	printf(", %dx%d\n",
	    sc->sc_sunfb.sf_width, sc->sc_sunfb.sf_height);

	if (isconsole) {
=======
	if (chipid != ID_AT24) {
		printf("%s: unexpected engine id %04x\n",
		    self->dv_xname, chipid);
	}

	mgx_ras_init(sc, chipid);

	if (isconsole)
>>>>>>> origin/master
		fbwscons_console_init(&sc->sc_sunfb, -1);

	fbwscons_attach(&sc->sc_sunfb, &mgx_accessops, isconsole);
}

/*
 * wsdisplay operations
 */

int
mgx_ioctl(void *dev, u_long cmd, caddr_t data, int flags, struct proc *p)
{
	struct mgx_softc *sc = dev;
	struct wsdisplay_cmap *cm;
	struct wsdisplay_fbinfo *wdf;
	int error;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_MGX;
		break;
	case WSDISPLAYIO_GINFO:
		wdf = (struct wsdisplay_fbinfo *)data;
		wdf->height = sc->sc_sunfb.sf_height;
		wdf->width = sc->sc_sunfb.sf_width;
		wdf->depth = sc->sc_sunfb.sf_depth;
		wdf->cmsize = 256;
		break;
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_sunfb.sf_linebytes;
		break;

	case WSDISPLAYIO_GETCMAP:
		cm = (struct wsdisplay_cmap *)data;
		error = mgx_getcmap(sc->sc_cmap, cm);
		if (error != 0)
			return (error);
		break;
	case WSDISPLAYIO_PUTCMAP:
		cm = (struct wsdisplay_cmap *)data;
		error = mgx_putcmap(sc->sc_cmap, cm);
		if (error != 0)
			return (error);
		mgx_loadcmap(sc, cm->index, cm->count);
		break;

	case WSDISPLAYIO_SVIDEO:
	case WSDISPLAYIO_GVIDEO:
		break;

	default:
		return (-1);
	}

	return (0);
}

paddr_t
mgx_mmap(void *v, off_t offset, int prot)
{
	struct mgx_softc *sc = v;

	if (offset & PGOFSET)
		return (-1);

	/* Allow mapping as a dumb framebuffer from offset 0 */
	if (offset >= 0 && offset < sc->sc_sunfb.sf_fbsize) {
		return (REG2PHYS(&sc->sc_phys, offset) | PMAP_NC);
	}

	return (-1);
}

void
mgx_burner(void *v, u_int on, u_int flags)
{
	struct mgx_softc *sc = v;

	sc->sc_vidc[CRTC_INDEX] = 1;	/* TS mode register */
	if (on)
		sc->sc_vidc[CRTC_DATA] &= ~CD_DISABLEVIDEO;
	else
		sc->sc_vidc[CRTC_DATA] |= CD_DISABLEVIDEO;
}

/*
 * Colormap handling routines
 */

void
mgx_setcolor(void *v, u_int index, u_int8_t r, u_int8_t g, u_int8_t b)
{
	struct mgx_softc *sc = v;

	index *= 3;
	sc->sc_cmap[index++] = r;
	sc->sc_cmap[index++] = g;
	sc->sc_cmap[index] = b;

	mgx_loadcmap(sc, index, 1);
}

void
mgx_loadcmap(struct mgx_softc *sc, int start, int ncolors)
{
	u_int8_t *color;
	int i;

#if 0
	sc->sc_vidc[CMAP_WRITE_INDEX] = start;
	color = sc->sc_cmap + start * 3;
#else
	/*
	 * Apparently there is no way to load an incomplete cmap to this
	 * DAC. What a waste.
	 */
	ncolors = 256;
	color = sc->sc_cmap;
#endif
	for (i = ncolors * 3; i != 0; i--)
		sc->sc_vidc[CMAP_DATA] = *color++;
}

int
mgx_getcmap(u_int8_t *cm, struct wsdisplay_cmap *rcm)
{
	u_int index = rcm->index, count = rcm->count, i;
	int error;

	if (index >= 256 || count > 256 - index)
		return (EINVAL);

	for (i = 0; i < count; i++) {
		if ((error =
		    copyout(cm + (index + i) * 3 + 0, &rcm->red[i], 1)) != 0)
			return (error);
		if ((error =
		    copyout(cm + (index + i) * 3 + 1, &rcm->green[i], 1)) != 0)
			return (error);
		if ((error =
		    copyout(cm + (index + i) * 3 + 2, &rcm->blue[i], 1)) != 0)
			return (error);
	}

	return (0);
}

int
mgx_putcmap(u_int8_t *cm, struct wsdisplay_cmap *rcm)
{
	u_int index = rcm->index, count = rcm->count, i;
	int error;

	if (index >= 256 || count > 256 - index)
		return (EINVAL);

	for (i = 0; i < count; i++) {
		if ((error =
		    copyin(&rcm->red[i], cm + (index + i) * 3 + 0, 1)) != 0)
			return (error);
		if ((error =
		    copyin(&rcm->green[i], cm + (index + i) * 3 + 1, 1)) != 0)
			return (error);
		if ((error =
		    copyin(&rcm->blue[i], cm + (index + i) * 3 + 2, 1)) != 0)
			return (error);
	}

	return (0);
}
<<<<<<< HEAD
=======

/*
 * Accelerated Text Console Code
 *
 * The X driver makes sure there are at least as many FIFOs available as
 * registers to write. They can thus be considered as write slots.
 *
 * The code below expects to run on at least an AT24 chip, and does not
 * care for the AP6422 which has fewer FIFOs; some operations would need
 * to be done in two steps to support this chip.
 */

int
mgx_wait_engine(struct mgx_softc *sc)
{
	uint i;
	uint stat;

	for (i = 10000; i != 0; i--) {
		stat = mgx_read_1(sc->sc_xreg, ATR_BLT_STATUS);
		if (!ISSET(stat, BLT_HOST_BUSY | BLT_ENGINE_BUSY))
			break;
	}

	return i;
}

int
mgx_wait_fifo(struct mgx_softc *sc, uint nfifo)
{
	uint i;
	uint stat;

	for (i = 10000; i != 0; i--) {
		stat = (mgx_read_1(sc->sc_xreg, ATR_FIFO_STATUS) & FIFO_MASK) >>
		    FIFO_SHIFT;
		if (stat >= nfifo)
			break;
		mgx_write_1(sc->sc_xreg, ATR_FIFO_STATUS, 0);
	}

	return i;
}

void
mgx_ras_init(struct mgx_softc *sc, uint chipid)
{
	/*
	 * Check the chip ID. If it's not a 6424, do not plug the
	 * accelerated routines.
	 */

	if (chipid != ID_AT24)
		return;

	/*
	 * Wait until the chip is completely idle.
	 */

	if (mgx_wait_engine(sc) == 0)
		return;
	if (mgx_wait_fifo(sc, FIFO_AT24) == 0)
		return;

	/*
	 * Compute the invariant bits of the DEC register.
	 */

	switch (sc->sc_sunfb.sf_depth) {
	case 8:
		sc->sc_dec = DEC_DEPTH_8 << DEC_DEPTH_SHIFT;
		break;
	case 15:
	case 16:
		sc->sc_dec = DEC_DEPTH_16 << DEC_DEPTH_SHIFT;
		break;
	case 32:
		sc->sc_dec = DEC_DEPTH_32 << DEC_DEPTH_SHIFT;
		break;
	default:
		return;	/* not supported */
	}

	switch (sc->sc_sunfb.sf_width) {
	case 640:
		sc->sc_dec |= DEC_WIDTH_640 << DEC_WIDTH_SHIFT;
		break;
	case 800:
		sc->sc_dec |= DEC_WIDTH_800 << DEC_WIDTH_SHIFT;
		break;
	case 1024:
		sc->sc_dec |= DEC_WIDTH_1024 << DEC_WIDTH_SHIFT;
		break;
	case 1152:
		sc->sc_dec |= DEC_WIDTH_1152 << DEC_WIDTH_SHIFT;
		break;
	case 1280:
		sc->sc_dec |= DEC_WIDTH_1280 << DEC_WIDTH_SHIFT;
		break;
	case 1600:
		sc->sc_dec |= DEC_WIDTH_1600 << DEC_WIDTH_SHIFT;
		break;
	default:
		return;	/* not supported */
	}

	sc->sc_sunfb.sf_ro.ri_ops.copycols = mgx_ras_copycols;
	sc->sc_sunfb.sf_ro.ri_ops.copyrows = mgx_ras_copyrows;
	sc->sc_sunfb.sf_ro.ri_ops.erasecols = mgx_ras_erasecols;
	sc->sc_sunfb.sf_ro.ri_ops.eraserows = mgx_ras_eraserows;
	sc->sc_sunfb.sf_ro.ri_do_cursor = mgx_ras_do_cursor;

#ifdef notneeded
	mgx_write_1(sc->sc_xreg, ATR_CLIP_CONTROL, 1);
	mgx_write_4(sc->sc_xreg, ATR_CLIP_LEFTTOP, ATR_DUAL(0, 0));
	mgx_write_4(sc->sc_xreg, ATR_CLIP_RIGHTBOTTOM,
	    ATR_DUAL(sc->sc_sunfb.sf_width - 1, sc->sc_sunfb.sf_depth - 1));
#else
	mgx_write_1(sc->sc_xreg, ATR_CLIP_CONTROL, 0);
#endif
	mgx_write_1(sc->sc_xreg, ATR_BYTEMASK, 0xff);
}

int
mgx_ras_copycols(void *v, int row, int src, int dst, int n)
{
	struct rasops_info *ri = v;
	struct mgx_softc *sc = ri->ri_hw;
	uint dec = sc->sc_dec;

	n *= ri->ri_font->fontwidth;
	src *= ri->ri_font->fontwidth;
	src += ri->ri_xorigin;
	dst *= ri->ri_font->fontwidth;
	dst += ri->ri_xorigin;
	row *= ri->ri_font->fontheight;
	row += ri->ri_yorigin;

	dec |= (DEC_COMMAND_BLT << DEC_COMMAND_SHIFT) |
	    (DEC_START_DIMX << DEC_START_SHIFT);
	if (src < dst) {
		src += n - 1;
		dst += n - 1;
		dec |= DEC_DIR_X_REVERSE;
	}
	mgx_wait_fifo(sc, 5);
	mgx_write_1(sc->sc_xreg, ATR_ROP, ROP_SRC);
	mgx_write_4(sc->sc_xreg, ATR_DEC, dec);
	mgx_write_4(sc->sc_xreg, ATR_SRC_XY, ATR_DUAL(row, src));
	mgx_write_4(sc->sc_xreg, ATR_DST_XY, ATR_DUAL(row, dst));
	mgx_write_4(sc->sc_xreg, ATR_WH, ATR_DUAL(ri->ri_font->fontheight, n));
	mgx_wait_engine(sc);

	return 0;
}

int
mgx_ras_copyrows(void *v, int src, int dst, int n)
{
	struct rasops_info *ri = v;
	struct mgx_softc *sc = ri->ri_hw;
	uint dec = sc->sc_dec;

	n *= ri->ri_font->fontheight;
	src *= ri->ri_font->fontheight;
	src += ri->ri_yorigin;
	dst *= ri->ri_font->fontheight;
	dst += ri->ri_yorigin;

	dec |= (DEC_COMMAND_BLT << DEC_COMMAND_SHIFT) |
	    (DEC_START_DIMX << DEC_START_SHIFT);
	if (src < dst) {
		src += n - 1;
		dst += n - 1;
		dec |= DEC_DIR_Y_REVERSE;
	}
	mgx_wait_fifo(sc, 5);
	mgx_write_1(sc->sc_xreg, ATR_ROP, ROP_SRC);
	mgx_write_4(sc->sc_xreg, ATR_DEC, dec);
	mgx_write_4(sc->sc_xreg, ATR_SRC_XY, ATR_DUAL(src, ri->ri_xorigin));
	mgx_write_4(sc->sc_xreg, ATR_DST_XY, ATR_DUAL(dst, ri->ri_xorigin));
	mgx_write_4(sc->sc_xreg, ATR_WH, ATR_DUAL(n, ri->ri_emuwidth));
	mgx_wait_engine(sc);

	return 0;
}

int
mgx_ras_erasecols(void *v, int row, int col, int n, long int attr)
{
	struct rasops_info *ri = v;
	struct mgx_softc *sc = ri->ri_hw;
	int fg, bg;
	uint dec = sc->sc_dec;

	ri->ri_ops.unpack_attr(v, attr, &fg, &bg, NULL);
	bg = ri->ri_devcmap[bg];

	n *= ri->ri_font->fontwidth;
	col *= ri->ri_font->fontwidth;
	col += ri->ri_xorigin;
	row *= ri->ri_font->fontheight;
	row += ri->ri_yorigin;

	dec |= (DEC_COMMAND_RECT << DEC_COMMAND_SHIFT) |
	    (DEC_START_DIMX << DEC_START_SHIFT);
	mgx_wait_fifo(sc, 5);
	mgx_write_1(sc->sc_xreg, ATR_ROP, ROP_SRC);
	mgx_write_4(sc->sc_xreg, ATR_FG, bg);
	mgx_write_4(sc->sc_xreg, ATR_DEC, dec);
	mgx_write_4(sc->sc_xreg, ATR_DST_XY, ATR_DUAL(row, col));
	mgx_write_4(sc->sc_xreg, ATR_WH, ATR_DUAL(ri->ri_font->fontheight, n));
	mgx_wait_engine(sc);

	return 0;
}

int
mgx_ras_eraserows(void *v, int row, int n, long int attr)
{
	struct rasops_info *ri = v;
	struct mgx_softc *sc = ri->ri_hw;
	int fg, bg;
	uint dec = sc->sc_dec;

	ri->ri_ops.unpack_attr(v, attr, &fg, &bg, NULL);
	bg = ri->ri_devcmap[bg];

	dec |= (DEC_COMMAND_RECT << DEC_COMMAND_SHIFT) |
	    (DEC_START_DIMX << DEC_START_SHIFT);
	mgx_wait_fifo(sc, 5);
	mgx_write_1(sc->sc_xreg, ATR_ROP, ROP_SRC);
	mgx_write_4(sc->sc_xreg, ATR_FG, bg);
	mgx_write_4(sc->sc_xreg, ATR_DEC, dec);
	if (n == ri->ri_rows && ISSET(ri->ri_flg, RI_FULLCLEAR)) {
		mgx_write_4(sc->sc_xreg, ATR_DST_XY, ATR_DUAL(0, 0));
		mgx_write_4(sc->sc_xreg, ATR_WH,
		    ATR_DUAL(ri->ri_height, ri->ri_width));
	} else {
		n *= ri->ri_font->fontheight;
		row *= ri->ri_font->fontheight;
		row += ri->ri_yorigin;

		mgx_write_4(sc->sc_xreg, ATR_DST_XY,
		    ATR_DUAL(row, ri->ri_xorigin));
		mgx_write_4(sc->sc_xreg, ATR_WH, ATR_DUAL(n, ri->ri_emuwidth));
	}
	mgx_wait_engine(sc);

	return 0;
}

int
mgx_ras_do_cursor(struct rasops_info *ri)
{
	struct mgx_softc *sc = ri->ri_hw;
	int row, col;
	uint dec = sc->sc_dec;

	row = ri->ri_crow * ri->ri_font->fontheight + ri->ri_yorigin;
	col = ri->ri_ccol * ri->ri_font->fontwidth + ri->ri_xorigin;

	dec |= (DEC_COMMAND_BLT << DEC_COMMAND_SHIFT) |
	    (DEC_START_DIMX << DEC_START_SHIFT);
	mgx_wait_fifo(sc, 5);
	mgx_write_1(sc->sc_xreg, ATR_ROP, (uint8_t)~ROP_SRC);
	mgx_write_4(sc->sc_xreg, ATR_DEC, dec);
	mgx_write_4(sc->sc_xreg, ATR_SRC_XY, ATR_DUAL(row, col));
	mgx_write_4(sc->sc_xreg, ATR_DST_XY, ATR_DUAL(row, col));
	mgx_write_4(sc->sc_xreg, ATR_WH,
	    ATR_DUAL(ri->ri_font->fontheight, ri->ri_font->fontwidth));
	mgx_wait_engine(sc);

	return 0;
}
>>>>>>> origin/master
