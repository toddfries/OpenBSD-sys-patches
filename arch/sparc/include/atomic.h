<<<<<<< HEAD
/*	$OpenBSD: atomic.h,v 1.1 2007/02/06 17:13:33 art Exp $	*/
=======
/*	$OpenBSD: atomic.h,v 1.4 2010/04/21 03:03:24 deraadt Exp $	*/
>>>>>>> origin/master

/* Public Domain */

#ifndef __SPARC_ATOMIC_H__
#define __SPARC_ATOMIC_H__

#if defined(_KERNEL)

#include <machine/psl.h>

static __inline void
atomic_setbits_int(__volatile unsigned int *uip, unsigned int v)
{
	*uip |= v;
}

static __inline void
atomic_clearbits_int(__volatile unsigned int *uip, unsigned int v)
{
	*uip &= ~v;
}

#endif /* defined(_KERNEL) */
#endif /* __SPARC_ATOMIC_H__ */
