/*-
 * Copyright (c) 1999 Kazutaka YOKOTA <yokota@zodiac.mech.utsunomiya-u.ac.jp>
 * All rights reserved.
 *
 * Copyright (c) 2008-2009 Ed Schouten <ed@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer as
 *    the first lines of this file unmodified.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/dev/syscons/scterm-teken.c,v 1.1 2009/01/01 13:26:53 ed Exp $");

#include "opt_syscons.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/consio.h>

#if defined(__sparc64__) || defined(__powerpc__)
#include <machine/sc_machdep.h>
#else
#include <machine/pc/display.h>
#endif

#include <dev/syscons/syscons.h>

#include <dev/syscons/teken/teken.h>

static void scteken_revattr(unsigned char, teken_attr_t *);

static sc_term_init_t	scteken_init;
static sc_term_term_t	scteken_term;
static sc_term_puts_t	scteken_puts;
static sc_term_ioctl_t	scteken_ioctl;
static sc_term_default_attr_t scteken_default_attr;
static sc_term_clear_t	scteken_clear;
static sc_term_input_t	scteken_input;
static void		scteken_nop(void);

typedef struct {
	teken_t		ts_teken;
	int		ts_busy;
} teken_stat;

static teken_stat	reserved_teken_stat;

static sc_term_sw_t sc_term_scteken = {
	{ NULL, NULL },
	"scteken",			/* emulator name */
	"teken terminal",		/* description */
	"*",				/* matching renderer */
	sizeof(teken_stat),		/* softc size */
	0,
	scteken_init,
	scteken_term,
	scteken_puts,
	scteken_ioctl,
	(sc_term_reset_t *)scteken_nop,
	scteken_default_attr,
	scteken_clear,
	(sc_term_notify_t *)scteken_nop,
	scteken_input,
};

SCTERM_MODULE(scteken, sc_term_scteken);

static tf_bell_t	scteken_bell;
static tf_cursor_t	scteken_cursor;
static tf_putchar_t	scteken_putchar;
static tf_fill_t	scteken_fill;
static tf_copy_t	scteken_copy;
static tf_param_t	scteken_param;
static tf_respond_t	scteken_respond;

static const teken_funcs_t scteken_funcs = {
	.tf_bell	= scteken_bell,
	.tf_cursor	= scteken_cursor,
	.tf_putchar	= scteken_putchar,
	.tf_fill	= scteken_fill,
	.tf_copy	= scteken_copy,
	.tf_param	= scteken_param,
	.tf_respond	= scteken_respond,
};

static int
scteken_init(scr_stat *scp, void **softc, int code)
{
	teken_stat *ts = scp->ts;
	teken_pos_t tp;

	if (*softc == NULL) {
		if (reserved_teken_stat.ts_busy)
			return (EINVAL);
		*softc = &reserved_teken_stat;
	}
	ts = *softc;

	switch (code) {
	case SC_TE_COLD_INIT:
		++sc_term_scteken.te_refcount;
		ts->ts_busy = 1;
		/* FALLTHROUGH */
	case SC_TE_WARM_INIT:
		teken_init(&ts->ts_teken, &scteken_funcs, scp);

		tp.tp_row = scp->ysize;
		tp.tp_col = scp->xsize;
		teken_set_winsize(&ts->ts_teken, &tp);

		tp.tp_row = scp->cursor_pos / scp->xsize;
		tp.tp_col = scp->cursor_pos % scp->xsize;
		teken_set_cursor(&ts->ts_teken, &tp);
		break;
	}

	return (0);
}

static int
scteken_term(scr_stat *scp, void **softc)
{

	if (*softc == &reserved_teken_stat) {
		*softc = NULL;
		reserved_teken_stat.ts_busy = 0;
	}
	--sc_term_scteken.te_refcount;

	return (0);
}

static void
scteken_puts(scr_stat *scp, u_char *buf, int len)
{
	teken_stat *ts = scp->ts;

	scp->sc->write_in_progress++;
	teken_input(&ts->ts_teken, buf, len);
	scp->sc->write_in_progress--;
}

static int
scteken_ioctl(scr_stat *scp, struct tty *tp, u_long cmd, caddr_t data,
	     struct thread *td)
{
	vid_info_t *vi;

	switch (cmd) {
	case GIO_ATTR:      	/* get current attributes */
		*(int*)data = SC_NORM_ATTR;
		return (0);
	case CONS_GETINFO:  	/* get current (virtual) console info */
		/* XXX: INCORRECT! */
		vi = (vid_info_t *)data;
		if (vi->size != sizeof(struct vid_info))
			return EINVAL;
		vi->mv_norm.fore = SC_NORM_ATTR & 0x0f;
		vi->mv_norm.back = (SC_NORM_ATTR >> 4) & 0x0f;
		vi->mv_rev.fore = SC_NORM_ATTR & 0x0f;
		vi->mv_rev.back = (SC_NORM_ATTR >> 4) & 0x0f;
		/*
		 * The other fields are filled by the upper routine. XXX
		 */
		return (ENOIOCTL);
	}

	return (ENOIOCTL);
}

static void
scteken_default_attr(scr_stat *scp, int color, int rev_color)
{
	teken_stat *ts = scp->ts;
	teken_attr_t ta;

	scteken_revattr(color, &ta);
	teken_set_defattr(&ts->ts_teken, &ta);
}

static void
scteken_clear(scr_stat *scp)
{

	sc_move_cursor(scp, 0, 0);
	sc_vtb_clear(&scp->vtb, scp->sc->scr_map[0x20], SC_NORM_ATTR << 8);
	mark_all(scp);
}

static int
scteken_input(scr_stat *scp, int c, struct tty *tp)
{

	return FALSE;
}

static void
scteken_nop(void)
{

}

/*
 * libteken routines.
 */

static const unsigned char fgcolors_normal[TC_NCOLORS] = {
	FG_BLACK,     FG_RED,          FG_GREEN,      FG_BROWN,
	FG_BLUE,      FG_MAGENTA,      FG_CYAN,       FG_LIGHTGREY,
};

static const unsigned char fgcolors_bold[TC_NCOLORS] = {
	FG_DARKGREY,  FG_LIGHTRED,     FG_LIGHTGREEN, FG_YELLOW,
	FG_LIGHTBLUE, FG_LIGHTMAGENTA, FG_LIGHTCYAN,  FG_WHITE,
};

static const unsigned char bgcolors[TC_NCOLORS] = {
	BG_BLACK,     BG_RED,          BG_GREEN,      BG_BROWN,
	BG_BLUE,      BG_MAGENTA,      BG_CYAN,       BG_LIGHTGREY,
};

static void
scteken_revattr(unsigned char color, teken_attr_t *a)
{
	teken_color_t fg, bg;

	/*
	 * XXX: Reverse conversion of syscons to teken attributes. Not
	 * realiable. Maybe we should turn it into a 1:1 mapping one of
	 * these days?
	 */

	a->ta_format = 0;
	a->ta_fgcolor = TC_WHITE;
	a->ta_bgcolor = TC_BLACK;

#ifdef FG_BLINK
	if (color & FG_BLINK) {
		a->ta_format |= TF_BLINK;
		color &= ~FG_BLINK;
	}
#endif /* FG_BLINK */

	for (fg = 0; fg < TC_NCOLORS; fg++) {
		for (bg = 0; bg < TC_NCOLORS; bg++) {
			if ((fgcolors_normal[fg] | bgcolors[bg]) == color) {
				a->ta_fgcolor = fg;
				a->ta_bgcolor = bg;
				return;
			}

			if ((fgcolors_bold[fg] | bgcolors[bg]) == color) {
				a->ta_fgcolor = fg;
				a->ta_bgcolor = bg;
				a->ta_format |= TF_BOLD;
				return;
			}
		}
	}
}

static inline unsigned int
scteken_attr(const teken_attr_t *a)
{
	unsigned int attr = 0;

	if (a->ta_format & TF_BOLD)
		attr |= fgcolors_bold[a->ta_fgcolor];
	else
		attr |= fgcolors_normal[a->ta_fgcolor];
	attr |= bgcolors[a->ta_bgcolor];

#ifdef FG_UNDERLINE
	if (a->ta_format & TF_UNDERLINE)
		attr |= FG_UNDERLINE;
#endif /* FG_UNDERLINE */
#ifdef FG_BLINK
	if (a->ta_format & TF_BLINK)
		attr |= FG_BLINK;
#endif /* FG_BLINK */

	return (attr << 8);
}

static void
scteken_bell(void *arg)
{
	scr_stat *scp = arg;

	sc_bell(scp, scp->bell_pitch, scp->bell_duration);
}

static void
scteken_cursor(void *arg, const teken_pos_t *p)
{
	scr_stat *scp = arg;

	sc_move_cursor(scp, p->tp_col, p->tp_row);
}

static void
scteken_putchar(void *arg, const teken_pos_t *tp, teken_char_t c,
    const teken_attr_t *a)
{
	scr_stat *scp = arg;
	u_char *map;
	u_char ch;
	vm_offset_t p;
	int cursor, attr;

#ifdef TEKEN_UTF8
	if (c >= 0x80) {
		/* XXX: Don't display UTF-8 yet. */
		attr = (FG_YELLOW|BG_RED) << 8;
		ch = '?';
	} else
#endif /* TEKEN_UTF8 */
	{
		attr = scteken_attr(a);
		ch = c;
	}

	map = scp->sc->scr_map;

	cursor = tp->tp_row * scp->xsize + tp->tp_col;
	p = sc_vtb_pointer(&scp->vtb, cursor);
	sc_vtb_putchar(&scp->vtb, p, map[ch], attr);

	mark_for_update(scp, cursor);
	/*
	 * XXX: Why do we need this? Only marking `cursor' should be
	 * enough. Without this line, we get artifacts.
	 */
	mark_for_update(scp, imin(cursor + 1, scp->xsize * scp->ysize - 1));
}

static void
scteken_fill(void *arg, const teken_rect_t *r, teken_char_t c,
    const teken_attr_t *a)
{
	scr_stat *scp = arg;
	u_char *map;
	u_char ch;
	unsigned int width;
	int attr, row;

#ifdef TEKEN_UTF8
	if (c >= 0x80) {
		/* XXX: Don't display UTF-8 yet. */
		attr = (FG_YELLOW|BG_RED) << 8;
		ch = '?';
	} else
#endif /* TEKEN_UTF8 */
	{
		attr = scteken_attr(a);
		ch = c;
	}

	map = scp->sc->scr_map;

	if (r->tr_begin.tp_col == 0 && r->tr_end.tp_col == scp->xsize) {
		/* Single contiguous region to fill. */
		sc_vtb_erase(&scp->vtb, r->tr_begin.tp_row * scp->xsize,
		    (r->tr_end.tp_row - r->tr_begin.tp_row) * scp->xsize,
		    map[ch], attr);
	} else {
		/* Fill display line by line. */
		width = r->tr_end.tp_col - r->tr_begin.tp_col;

		for (row = r->tr_begin.tp_row; row < r->tr_end.tp_row; row++) {
			sc_vtb_erase(&scp->vtb, r->tr_begin.tp_row *
			    scp->xsize + r->tr_begin.tp_col,
			    width, map[ch], attr);
		}
	}

	/* Mark begin and end positions to be refreshed. */
	mark_for_update(scp,
	    r->tr_begin.tp_row * scp->xsize + r->tr_begin.tp_col);
	mark_for_update(scp,
	    (r->tr_end.tp_row - 1) * scp->xsize + (r->tr_end.tp_col - 1));
	sc_remove_cutmarking(scp);
}

static void
scteken_copy(void *arg, const teken_rect_t *r, const teken_pos_t *p)
{
	scr_stat *scp = arg;
	unsigned int width;
	int src, dst, end;

#ifndef SC_NO_HISTORY
	/*
	 * We count a line of input as history if we perform a copy of
	 * one whole line upward. In other words: if a line of text gets
	 * overwritten by a rectangle that's right below it.
	 */
	if (scp->history != NULL &&
	    r->tr_begin.tp_col == 0 && r->tr_end.tp_col == scp->xsize &&
	    r->tr_begin.tp_row == p->tp_row + 1) {
		sc_hist_save_one_line(scp, p->tp_row);
	}
#endif

	if (r->tr_begin.tp_col == 0 && r->tr_end.tp_col == scp->xsize) {
		/* Single contiguous region to copy. */
		sc_vtb_move(&scp->vtb, r->tr_begin.tp_row * scp->xsize,
		    p->tp_row * scp->xsize,
		    (r->tr_end.tp_row - r->tr_begin.tp_row) * scp->xsize);
	} else {
		/* Copy line by line. */
		width = r->tr_end.tp_col - r->tr_begin.tp_col;

		if (p->tp_row < r->tr_begin.tp_row) {
			/* Copy from top to bottom. */
			src = r->tr_begin.tp_row * scp->xsize +
			    r->tr_begin.tp_col;
			end = r->tr_end.tp_row * scp->xsize +
			    r->tr_end.tp_col;
			dst = p->tp_row * scp->xsize + p->tp_col;

			while (src < end) {
				sc_vtb_move(&scp->vtb, src, dst, width);
			
				src += scp->xsize;
				dst += scp->xsize;
			}
		} else {
			/* Copy from bottom to top. */
			src = (r->tr_end.tp_row - 1) * scp->xsize +
			    r->tr_begin.tp_col;
			end = r->tr_begin.tp_row * scp->xsize +
			    r->tr_begin.tp_col;
			dst = (p->tp_row + r->tr_end.tp_row -
			    r->tr_begin.tp_row - 1) * scp->xsize + p->tp_col;

			while (src >= end) {
				sc_vtb_move(&scp->vtb, src, dst, width);
			
				src -= scp->xsize;
				dst -= scp->xsize;
			}
		}
	}

	/* Mark begin and end positions to be refreshed. */
	mark_for_update(scp,
	    p->tp_row * scp->xsize + p->tp_col);
	mark_for_update(scp,
	    (p->tp_row + r->tr_end.tp_row - r->tr_begin.tp_row - 1) *
	    scp->xsize +
	    (p->tp_col + r->tr_end.tp_col - r->tr_begin.tp_col - 1));
	sc_remove_cutmarking(scp);
}

static void
scteken_param(void *arg, int cmd, int value)
{
	scr_stat *scp = arg;

	switch (cmd) {
	case TP_SHOWCURSOR:
		if (value) {
			sc_change_cursor_shape(scp,
			    CONS_RESET_CURSOR|CONS_LOCAL_CURSOR, -1, -1);
		} else {
			sc_change_cursor_shape(scp,
			    CONS_HIDDEN_CURSOR|CONS_LOCAL_CURSOR, -1, -1);
		}
		break;
	case TP_SWITCHVT:
		sc_switch_scr(scp->sc, value);
		break;
	}
}

static void
scteken_respond(void *arg, const void *buf, size_t len)
{
	scr_stat *scp = arg;

	sc_respond(scp, buf, len);
}
