/*	$OpenBSD: isa_machdep.h,v 1.4 1999/01/19 10:04:55 niklas Exp $	*/

/*
 * Copyright (c) 1995, 1996 Niklas Hallqvist
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Niklas Hallqvist.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _ISA_MACHDEP_H_
#define _ISA_MACHDEP_H_

#include <machine/endian.h>

/*
 * Types provided to machine-independent ISA code.
 */
typedef struct amiga_isa_chipset *isa_chipset_tag_t;

struct amiga_isa_chipset {
	void	*ic_data;

	void	(*ic_attach_hook) __P((struct device *, struct device *,
		    struct isabus_attach_args *));
	void	*(*ic_intr_establish) __P((void *, int, int, int,
		    int (*)(void *), void *, char *));
	void	(*ic_intr_disestablish) __P((void *, void *));
	int	(*ic_intr_check) __P((void *, int, int));
};

int	__isa_intr_check __P((int, int, int *));

/*
 * Functions provided to machine-independent ISA code.
 */
#define	isa_attach_hook(p, s, a)					\
    (*(a)->iba_ic->ic_attach_hook)((p), (s), (a))
#define	isa_intr_establish(c, i, t, l, f, a, w)				\
    (*(c)->ic_intr_establish)((c)->ic_data, (i), (t), (l), (f), (a), (w))
#define	isa_intr_disestablish(c, h)					\
    (*(c)->ic_intr_disestablish)((c)->ic_data, (h))
#define	isa_intr_check(c, i, t)						\
    (*(c)->ic_intr_check)((c)->ic_data, (i), (t))

#endif
