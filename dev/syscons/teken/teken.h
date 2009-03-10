/*-
 * Copyright (c) 2008-2009 Ed Schouten <ed@FreeBSD.org>
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
 * $FreeBSD: src/sys/dev/syscons/teken/teken.h,v 1.6 2009/03/10 11:28:54 ed Exp $
 */

#ifndef _TEKEN_H_
#define	_TEKEN_H_

/*
 * libteken: terminal emulation library.
 *
 * This library converts an UTF-8 stream of bytes to terminal drawing
 * commands.
 *
 * Configuration switches:
 * - TEKEN_UTF8: Enable/disable UTF-8 handling.
 * - TEKEN_XTERM: Enable xterm-style emulation, instead of cons25.
 */

#if defined(__FreeBSD__) && defined(_KERNEL)
#include "opt_teken.h"
#endif /* __FreeBSD__ && _KERNEL */

#ifdef TEKEN_UTF8
typedef uint32_t teken_char_t;
#else /* !TEKEN_UTF8 */
typedef unsigned char teken_char_t;
#endif /* TEKEN_UTF8 */
typedef unsigned short teken_unit_t;
typedef unsigned char teken_format_t;
#define	TF_BOLD		0x01
#define	TF_UNDERLINE	0x02
#define	TF_BLINK	0x04
typedef unsigned char teken_color_t;
#define	TC_BLACK	0
#define	TC_RED		1
#define	TC_GREEN	2
#define	TC_BROWN	3
#define	TC_BLUE		4
#define	TC_MAGENTA	5
#define	TC_CYAN		6
#define	TC_WHITE	7
#define	TC_NCOLORS	8

typedef struct {
	teken_unit_t	tp_row;
	teken_unit_t	tp_col;
} teken_pos_t;
typedef struct {
	teken_pos_t	tr_begin;
	teken_pos_t	tr_end;
} teken_rect_t;
typedef struct {
	teken_format_t	ta_format;
	teken_color_t	ta_fgcolor;
	teken_color_t	ta_bgcolor;
} teken_attr_t;
typedef struct {
	teken_unit_t	ts_begin;
	teken_unit_t	ts_end;
} teken_span_t;

typedef struct __teken teken_t;

typedef void teken_state_t(teken_t *, teken_char_t);

/*
 * Drawing routines supplied by the user.
 */

typedef void tf_bell_t(void *);
typedef void tf_cursor_t(void *, const teken_pos_t *);
typedef void tf_putchar_t(void *, const teken_pos_t *, teken_char_t,
    const teken_attr_t *);
typedef void tf_fill_t(void *, const teken_rect_t *, teken_char_t,
    const teken_attr_t *);
typedef void tf_copy_t(void *, const teken_rect_t *, const teken_pos_t *);
typedef void tf_param_t(void *, int, int);
#define	TP_SHOWCURSOR	0
#define	TP_CURSORKEYS	1
#define	TP_KEYPADAPP	2
#define	TP_AUTOREPEAT	3
#define	TP_SWITCHVT	4
#define	TP_132COLS	5
typedef void tf_respond_t(void *, const void *, size_t);

typedef struct {
	tf_bell_t	*tf_bell;
	tf_cursor_t	*tf_cursor;
	tf_putchar_t	*tf_putchar;
	tf_fill_t	*tf_fill;
	tf_copy_t	*tf_copy;
	tf_param_t	*tf_param;
	tf_respond_t	*tf_respond;
} teken_funcs_t;

#if defined(TEKEN_XTERM) && defined(TEKEN_UTF8)
typedef teken_char_t teken_scs_t(teken_char_t);
#endif /* TEKEN_XTERM && TEKEN_UTF8 */

/*
 * Terminal state.
 */

struct __teken {
	const teken_funcs_t *t_funcs;
	void		*t_softc;

	teken_state_t	*t_nextstate;
	unsigned int	 t_stateflags;

#define T_NUMSIZE	8
	unsigned int	 t_nums[T_NUMSIZE];
	unsigned int	 t_curnum;

	teken_pos_t	 t_cursor;
	teken_attr_t	 t_curattr;
	teken_pos_t	 t_saved_cursor;
	teken_attr_t	 t_saved_curattr;

	teken_attr_t	 t_defattr;
	teken_pos_t	 t_winsize;

	/* For DECSTBM. */
	teken_span_t	 t_scrollreg;
	/* For DECOM. */
	teken_span_t	 t_originreg;

#define	T_NUMCOL	160
	unsigned int	 t_tabstops[T_NUMCOL / (sizeof(unsigned int) * 8)];

#ifdef TEKEN_UTF8
	unsigned int	 t_utf8_left;
	teken_char_t	 t_utf8_partial;
#endif /* TEKEN_UTF8 */

#if defined(TEKEN_XTERM) && defined(TEKEN_UTF8)
	unsigned int	 t_curscs;
	teken_scs_t	*t_saved_curscs;
	teken_scs_t	*t_scs[2];
#endif /* TEKEN_XTERM && TEKEN_UTF8 */
};

/* Initialize teken structure. */
void	teken_init(teken_t *, const teken_funcs_t *, void *);

/* Deliver character input. */
void	teken_input(teken_t *, const void *, size_t);

/* Get/set teken attributes. */
const teken_attr_t *teken_get_curattr(teken_t *);
const teken_attr_t *teken_get_defattr(teken_t *);
void	teken_set_cursor(teken_t *, const teken_pos_t *);
void	teken_set_curattr(teken_t *, const teken_attr_t *);
void	teken_set_defattr(teken_t *, const teken_attr_t *);
void	teken_set_winsize(teken_t *, const teken_pos_t *);

#endif /* !_TEKEN_H_ */
