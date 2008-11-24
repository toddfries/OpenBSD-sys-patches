/* $FreeBSD: src/sys/powerpc/include/trap.h,v 1.6 2008/03/03 13:20:52 raj Exp $ */

#if defined(AIM)
#include <machine/trap_aim.h>
#elif defined(E500)
#include <machine/trap_booke.h>
#endif

#ifndef LOCORE
struct trapframe;
void    trap(struct trapframe *);
#endif
