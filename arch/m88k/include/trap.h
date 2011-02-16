/*	$OpenBSD: trap.h,v 1.9 2009/03/01 17:43:23 miod Exp $ */
/*
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * Trap codes
 */
#ifndef __MACHINE_TRAP_H__
#define __MACHINE_TRAP_H__

/*
 * Trap type values
 */

#define	T_USER		19	/* user mode fault */

#ifndef _LOCORE

void	ast(struct trapframe *);
void	cache_flush(struct trapframe *);
void	interrupt(struct trapframe *);
int	nmi(struct trapframe *);
void	nmi_wrapup(struct trapframe *);

void	m88100_syscall(register_t, struct trapframe *);
void	m88100_trap(u_int, struct trapframe *);
void	m88110_syscall(register_t, struct trapframe *);
void	m88110_trap(u_int, struct trapframe *);

void	m88110_fpu_exception(struct trapframe *);

#endif /* _LOCORE */

#endif /* __MACHINE_TRAP_H__ */
