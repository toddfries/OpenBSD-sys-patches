<<<<<<< HEAD
/*	$OpenBSD$	*/
=======
/*	$OpenBSD: atomic.h,v 1.6 2010/07/07 15:36:18 kettenis Exp $	*/
>>>>>>> origin/master
/*
 * Copyright (c) 2007 Artur Grabowski <art@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _SPARC64_ATOMIC_H_
#define _SPARC64_ATOMIC_H_

#if defined(_KERNEL)

static __inline unsigned int
sparc64_casa(volatile unsigned int *uip, unsigned int expect, unsigned int new)
{
	__asm __volatile("casa [%2] %3, %4, %0"
	    : "+r" (new), "=m" (*uip)
	    : "r" (uip), "n" (ASI_N), "r" (expect), "m" (*uip));

	return (new);
}

static __inline void
atomic_setbits_int(volatile unsigned int *uip, unsigned int v)
{
	volatile unsigned int e, r;

	r = *uip;
	do {
		e = r;
		r = sparc64_casa(uip, e, e | v);
	} while (r != e);
}

static __inline void
atomic_clearbits_int(volatile unsigned int *uip, unsigned int v)
{
	volatile unsigned int e, r;

	r = *uip;
	do {
		e = r;
		r = sparc64_casa(uip, e, e & ~v);
	} while (r != e);
}

static __inline void
atomic_add_ulong(volatile unsigned long *ulp, unsigned long v)
{
	volatile unsigned long e, r;

	r = *ulp;
	do {
		e = r;
		r = sparc64_casx(ulp, e, e + v);
	} while (r != e);
}

#endif /* defined(_KERNEL) */
#endif /* _SPARC64_ATOMIC_H_ */
