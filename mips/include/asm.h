/*	$NetBSD: asm.h,v 1.29 2000/12/14 21:29:51 jeffs Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)machAsmDefs.h	8.1 (Berkeley) 6/10/93
 *	JNPR: asm.h,v 1.10 2007/08/09 11:23:32 katta
 * $FreeBSD: src/sys/mips/include/asm.h,v 1.1 2008/04/13 07:22:52 imp Exp $
 */

/*
 * machAsmDefs.h --
 *
 *	Macros used when writing assembler programs.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/machAsmDefs.h,
 *	v 1.2 89/08/15 18:28:24 rab Exp  SPRITE (DECWRL)
 */

#ifndef _MACHINE_ASM_H_
#define	_MACHINE_ASM_H_

#ifndef NO_REG_DEFS
#include <machine/regdef.h>
#endif
#include <machine/endian.h>

#undef __FBSDID
#if !defined(lint) && !defined(STRIP_FBSDID)
#define	__FBSDID(s)	.ident s
#else
#define	__FBSDID(s)	/* nothing */
#endif

/*
 * Define -pg profile entry code.
 * Must always be noreorder, must never use a macro instruction
 * Final addiu to t9 must always equal the size of this _KERN_MCOUNT
 */
#define	_KERN_MCOUNT			\
	.set	push;			\
	.set	noreorder;		\
	.set	noat;			\
	subu	sp,sp,16;		\
	sw	t9,12(sp);		\
	move	AT,ra;			\
	lui	t9,%hi(_mcount);	\
	addiu	t9,t9,%lo(_mcount);	\
	jalr	t9;			\
	nop;				\
	lw	t9,4(sp);		\
	addiu	sp,sp,8;		\
	addiu	t9,t9,40;		\
	.set	pop;

#ifdef GPROF
#define	MCOUNT _KERN_MCOUNT
#else
#define	MCOUNT
#endif

#define	_C_LABEL(x)	x

/* 
 *  Endian-independent assembly-code aliases for unaligned memory accesses.
 */
#if BYTE_ORDER == LITTLE_ENDIAN
#define	LWLO	lwl
#define	LWHI	lwr
#define	SWLO	swl
#define	SWHI	swr
#endif

#if BYTE_ORDER == BIG_ENDIAN
#define	LWLO	lwr
#define	LWHI	lwl
#define	SWLO	swr
#define	SWHI	swl
#endif

#ifdef USE_AENT
#define	AENT(x)		\
	.aent	x, 0
#else
#define	AENT(x)
#endif

/*
 * WARN_REFERENCES: create a warning if the specified symbol is referenced
 */
#define	WARN_REFERENCES(_sym,_msg)				\
	.section .gnu.warning. ## _sym ; .ascii _msg ; .text

/*
 * These are temp registers whose names can be used in either the old
 * or new ABI, although they map to different physical registers.  In
 * the old ABI, they map to t4-t7, and in the new ABI, they map to a4-a7.
 *
 * Because they overlap with the last 4 arg regs in the new ABI, ta0-ta3
 * should be used only when we need more than t0-t3.
 */
#if defined(__mips_n32) || defined(__mips_n64)
#define ta0     $8
#define ta1     $9
#define ta2     $10
#define ta3     $11
#else
#define ta0     $12
#define ta1     $13
#define ta2     $14
#define ta3     $15
#endif /* __mips_n32 || __mips_n64 */

#ifdef __ELF__
# define _C_LABEL(x)    x
#else
#  define _C_LABEL(x)   _ ## x
#endif

/*
 * WEAK_ALIAS: create a weak alias.
 */
#define	WEAK_ALIAS(alias,sym)						\
	.weak alias;							\
	alias = sym

/*
 * STRONG_ALIAS: create a strong alias.
 */
#define STRONG_ALIAS(alias,sym)						\
	.globl alias;							\
	alias = sym

#define	GLOBAL(sym)						\
	.globl sym; sym:

#define	ENTRY(sym)						\
	.text; .globl sym; .ent sym; sym:

#define	ASM_ENTRY(sym)						\
	.text; .globl sym; .type sym,@function; sym:

/*
 * LEAF
 *	A leaf routine does
 *	- call no other function,
 *	- never use any register that callee-saved (S0-S8), and
 *	- not use any local stack storage.
 */
#define	LEAF(x)			\
	.globl	_C_LABEL(x);	\
	.ent	_C_LABEL(x), 0;	\
_C_LABEL(x): ;			\
	.frame sp, 0, ra;	\
	MCOUNT

/*
 * LEAF_NOPROFILE
 *	No profilable leaf routine.
 */
#define	LEAF_NOPROFILE(x)	\
	.globl	_C_LABEL(x);	\
	.ent	_C_LABEL(x), 0;	\
_C_LABEL(x): ;			\
	.frame	sp, 0, ra

/*
 * XLEAF
 *	declare alternate entry to leaf routine
 */
#define	XLEAF(x)		\
	.globl	_C_LABEL(x);	\
	AENT (_C_LABEL(x));	\
_C_LABEL(x):

/*
 * NESTED
 *	A function calls other functions and needs
 *	therefore stack space to save/restore registers.
 */
#define	NESTED(x, fsize, retpc)		\
	.globl	_C_LABEL(x);		\
	.ent	_C_LABEL(x), 0;		\
_C_LABEL(x): ;				\
	.frame	sp, fsize, retpc;	\
	MCOUNT

/*
 * NESTED_NOPROFILE(x)
 *	No profilable nested routine.
 */
#define	NESTED_NOPROFILE(x, fsize, retpc)	\
	.globl	_C_LABEL(x);			\
	.ent	_C_LABEL(x), 0;			\
_C_LABEL(x): ;					\
	.frame	sp, fsize, retpc

/*
 * XNESTED
 *	declare alternate entry point to nested routine.
 */
#define	XNESTED(x)		\
	.globl	_C_LABEL(x);	\
	AENT (_C_LABEL(x));	\
_C_LABEL(x):

/*
 * END
 *	Mark end of a procedure.
 */
#define	END(x)			\
	.end _C_LABEL(x)

/*
 * IMPORT -- import external symbol
 */
#define	IMPORT(sym, size)	\
	.extern _C_LABEL(sym),size

/*
 * EXPORT -- export definition of symbol
 */
#define	EXPORT(x)		\
	.globl	_C_LABEL(x);	\
_C_LABEL(x):

/*
 * VECTOR
 *	exception vector entrypoint
 *	XXX: regmask should be used to generate .mask
 */
#define	VECTOR(x, regmask)	\
	.ent	_C_LABEL(x),0;	\
	EXPORT(x);		\

#define	VECTOR_END(x)		\
	EXPORT(x ## End);	\
	END(x)

#define	KSEG0TEXT_START
#define	KSEG0TEXT_END
#define	KSEG0TEXT	.text

/*
 * Macros to panic and printf from assembly language.
 */
#define	PANIC(msg)			\
	la	a0, 9f;			\
	jal	_C_LABEL(panic);	\
	nop;				\
	MSG(msg)

#define	PANIC_KSEG0(msg, reg)	PANIC(msg)

#define	PRINTF(msg)			\
	la	a0, 9f;			\
	jal	_C_LABEL(printf);	\
	nop;				\
	MSG(msg)

#define	MSG(msg)			\
	.rdata;				\
9:	.asciiz	msg;			\
	.text

#define	ASMSTR(str)			\
	.asciiz str;			\
	.align	3

/*
 * Call ast if required
 */
#define DO_AST				             \
44:				                     \
	la	s0, _C_LABEL(disableintr)           ;\
	jalr	s0                                  ;\
	nop                                         ;\
	GET_CPU_PCPU(s1)                            ;\
	lw	s3, PC_CURPCB(s1)                   ;\
	lw	s1, PC_CURTHREAD(s1)                ;\
	lw	s2, TD_FLAGS(s1)                    ;\
	li	s0, TDF_ASTPENDING | TDF_NEEDRESCHED;\
	and	s2, s0                              ;\
	la	s0, _C_LABEL(enableintr)            ;\
	jalr	s0                                  ;\
	nop                                         ;\
	beq	s2, zero, 4f                        ;\
	nop                                         ;\
	la	s0, _C_LABEL(ast)                   ;\
	jalr	s0                                  ;\
	addu	a0, s3, U_PCB_REGS                  ;\
	j 44b			                    ;\
        nop                                         ;\
4:


/*
 * XXX retain dialects XXX
 */
#define	ALEAF(x)			XLEAF(x)
#define	NLEAF(x)			LEAF_NOPROFILE(x)
#define	NON_LEAF(x, fsize, retpc)	NESTED(x, fsize, retpc)
#define	NNON_LEAF(x, fsize, retpc)	NESTED_NOPROFILE(x, fsize, retpc)

/*
 *  standard callframe {
 *  	register_t cf_args[4];		arg0 - arg3
 *  	register_t cf_sp;		frame pointer
 *  	register_t cf_ra;		return address
 *  };
 */
#define	CALLFRAME_SIZ	(4 * (4 + 2))
#define	CALLFRAME_SP	(4 * 4)
#define	CALLFRAME_RA	(4 * 5)
#define	START_FRAME	CALLFRAME_SIZ

/*
 * While it would be nice to be compatible with the SGI
 * REG_L and REG_S macros, because they do not take parameters, it
 * is impossible to use them with the _MIPS_SIM_ABIX32 model.
 *
 * These macros hide the use of mips3 instructions from the
 * assembler to prevent the assembler from generating 64-bit style
 * ABI calls.
 */

#if !defined(_MIPS_BSD_API) || _MIPS_BSD_API == _MIPS_BSD_API_LP32
#define	REG_L		lw
#define	REG_S		sw
#define	REG_LI		li
#define	REG_PROLOGUE	.set push
#define	REG_EPILOGUE	.set pop
#define	SZREG		4
#else
#define	REG_L		ld
#define	REG_S		sd
#define	REG_LI		dli
#define	REG_PROLOGUE	.set push ; .set mips3
#define	REG_EPILOGUE	.set pop
#define	SZREG		8
#endif	/* _MIPS_BSD_API */

#define	mfc0_macro(data, spr)						\
	__asm __volatile ("mfc0 %0, $%1"				\
			: "=r" (data)	/* outputs */			\
			: "i" (spr));	/* inputs */

#define	mtc0_macro(data, spr)						\
	__asm __volatile ("mtc0 %0, $%1"				\
			:				/* outputs */	\
			: "r" (data), "i" (spr));	/* inputs */

#define	cfc0_macro(data, spr)						\
	__asm __volatile ("cfc0 %0, $%1"				\
			: "=r" (data)	/* outputs */			\
			: "i" (spr));	/* inputs */

#define	ctc0_macro(data, spr)						\
	__asm __volatile ("ctc0 %0, $%1"				\
			:				/* outputs */	\
			: "r" (data), "i" (spr));	/* inputs */


#define	lbu_macro(data, addr)						\
	__asm __volatile ("lbu %0, 0x0(%1)"				\
			: "=r" (data)	/* outputs */			\
			: "r" (addr));	/* inputs */

#define	lb_macro(data, addr)						\
	__asm __volatile ("lb %0, 0x0(%1)"				\
			: "=r" (data)	/* outputs */			\
			: "r" (addr));	/* inputs */

#define	lwl_macro(data, addr)						\
	__asm __volatile ("lwl %0, 0x0(%1)"				\
			: "=r" (data)	/* outputs */			\
			: "r" (addr));	/* inputs */

#define	lwr_macro(data, addr)						\
	__asm __volatile ("lwr %0, 0x0(%1)"				\
			: "=r" (data)	/* outputs */			\
			: "r" (addr));	/* inputs */

#define	ldl_macro(data, addr)						\
	__asm __volatile ("ldl %0, 0x0(%1)"				\
			: "=r" (data)	/* outputs */			\
			: "r" (addr));	/* inputs */

#define	ldr_macro(data, addr)						\
	__asm __volatile ("ldr %0, 0x0(%1)"				\
			: "=r" (data)	/* outputs */			\
			: "r" (addr));	/* inputs */

#define	sb_macro(data, addr)						\
	__asm __volatile ("sb %0, 0x0(%1)"				\
			:				/* outputs */	\
			: "r" (data), "r" (addr));	/* inputs */

#define	swl_macro(data, addr)						\
	__asm __volatile ("swl %0, 0x0(%1)"				\
			: 				/* outputs */	\
			: "r" (data), "r" (addr));	/* inputs */

#define	swr_macro(data, addr)						\
	__asm __volatile ("swr %0, 0x0(%1)"				\
			: 				/* outputs */	\
			: "r" (data), "r" (addr));	/* inputs */

#define	sdl_macro(data, addr)						\
	__asm __volatile ("sdl %0, 0x0(%1)"				\
			: 				/* outputs */	\
			: "r" (data), "r" (addr));	/* inputs */

#define	sdr_macro(data, addr)						\
	__asm __volatile ("sdr %0, 0x0(%1)"				\
			:				/* outputs */	\
			: "r" (data), "r" (addr));	/* inputs */

#define	mfgr_macro(data, gr)						\
	__asm __volatile ("move %0, $%1"				\
			: "=r" (data)	/* outputs */			\
			: "i" (gr));	/* inputs */

#define	dmfc0_macro(data, spr)						\
	__asm __volatile ("dmfc0 %0, $%1"				\
			: "=r" (data)	/* outputs */			\
			: "i" (spr));	/* inputs */

#define	dmtc0_macro(data, spr, sel)					\
	__asm __volatile ("dmtc0	%0, $%1, %2"			\
			:			/* no  outputs */	\
			: "r" (data), "i" (spr), "i" (sel)); /* inputs */

/*
 * The DYNAMIC_STATUS_MASK option adds an additional masking operation
 * when updating the hardware interrupt mask in the status register.
 *
 * This is useful for platforms that need to at run-time mask
 * interrupts based on motherboard configuration or to handle
 * slowly clearing interrupts.
 *
 * XXX this is only currently implemented for mips3.
 */
#ifdef MIPS_DYNAMIC_STATUS_MASK
#define	DYNAMIC_STATUS_MASK(sr,scratch)			\
	lw	scratch, mips_dynamic_status_mask;	\
	and	sr, sr, scratch

#define	DYNAMIC_STATUS_MASK_TOUSER(sr,scratch1)		\
	ori	sr, (MIPS_INT_MASK | MIPS_SR_INT_IE);	\
	DYNAMIC_STATUS_MASK(sr,scratch1)
#else
#define	DYNAMIC_STATUS_MASK(sr,scratch)
#define	DYNAMIC_STATUS_MASK_TOUSER(sr,scratch1)
#endif

#ifdef SMP
	/*
	 * FREEBSD_DEVELOPERS_FIXME
	 * In multiprocessor case, store/retrieve the pcpu structure
	 * address for current CPU in scratch register for fast access.
	 */
#error "Write GET_CPU_PCPU for SMP"
#else
#define	GET_CPU_PCPU(reg)		\
	lw	reg, _C_LABEL(pcpup);
#endif

/*
 * Description of the setjmp buffer
 *
 * word  0	magic number	(dependant on creator)
 *       1	RA
 *       2	S0
 *       3	S1
 *       4	S2
 *       5	S3
 *       6	S4
 *       7	S5
 *       8	S6
 *       9	S7
 *       10	SP
 *       11	S8
 *       12	signal mask	(dependant on magic)
 *       13	(con't)
 *       14	(con't)
 *       15	(con't)
 *
 * The magic number number identifies the jmp_buf and
 * how the buffer was created as well as providing
 * a sanity check
 *
 */

#define _JB_MAGIC__SETJMP	0xBADFACED
#define _JB_MAGIC_SETJMP	0xFACEDBAD

/* Valid for all jmp_buf's */

#define _JB_MAGIC		0
#define _JB_REG_RA		1
#define _JB_REG_S0		2
#define _JB_REG_S1		3
#define _JB_REG_S2		4
#define _JB_REG_S3		5
#define _JB_REG_S4		6
#define _JB_REG_S5		7
#define _JB_REG_S6		8
#define _JB_REG_S7		9
#define _JB_REG_SP		10
#define _JB_REG_S8		11

/* Only valid with the _JB_MAGIC_SETJMP magic */

#define _JB_SIGMASK		12

#endif /* !_MACHINE_ASM_H_ */
