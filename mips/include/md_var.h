/*-
 * Copyright (c) 1995 Bruce D. Evans.
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
 * 3. Neither the name of the author nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *	from: src/sys/i386/include/md_var.h,v 1.35 2000/02/20 20:51:23 bsd
 *	JNPR: md_var.h,v 1.4 2006/10/16 12:30:34 katta
 * $FreeBSD: src/sys/mips/include/md_var.h,v 1.1 2008/04/13 07:22:52 imp Exp $
 */

#ifndef _MACHINE_MD_VAR_H_
#define	_MACHINE_MD_VAR_H_

#include <machine/reg.h>

/*
 * Miscellaneous machine-dependent declarations.
 */
extern	int	Maxmem;
extern	char	sigcode[];
extern	int	szsigcode, szosigcode;

extern vm_offset_t kstack0;

void	MipsSaveCurFPState(struct thread *);
void	fork_trampoline(void);
void	cpu_swapin(struct proc *);
u_int	MipsEmulateBranch(struct trapframe *, int, int, u_int);
u_long	kvtop(void *addr);
int	is_physical_memory(vm_offset_t addr);
int	is_cacheable_mem(vm_offset_t pa);

#define	MIPS_DEBUG   0

#if MIPS_DEBUG
#define	MIPS_DEBUG_PRINT(fmt, args...)	printf("%s: " fmt "\n" , __FUNCTION__ , ## args)
#else
#define	MIPS_DEBUG_PRINT(fmt, args...)
#endif

void	mips_vector_init(void);
void	cpu_identify(void);
void	mips_cpu_init(void);
void	mips_proc0_init(void);

/* Platform call-downs. */
void	platform_identify(void);

#endif /* !_MACHINE_MD_VAR_H_ */
