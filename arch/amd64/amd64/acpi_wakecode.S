/*	$NetBSD: acpi_wakecode.S,v 1.8 2006/06/20 22:36:58 jmcneill Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Takuya SHIOZAKI.
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
 *	  This product includes software developed by the NetBSD
 *	  Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * This code is derived from FreeBSD.  Original copyrights:
 *
 * Copyright (c) 2001 Takanori Watanabe <takawata@jp.freebsd.org>
 * Copyright (c) 2001 Mitsuru IWASAKI <iwasaki@jp.freebsd.org>
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
 *	FreeBSD: src/sys/i386/acpica/acpi_wakecode.S,v 1.1 2001/07/20 06:07:31 takawata Exp
 */

#define _LOCORE

#include <machine/asm.h>
#include <machine/specialreg.h>
#include <machine/param.h>
#include <machine/segments.h>
#include <machine/psl.h>


	.globl wakeup_16

/* On wakeup, we'll start executing at wakeup_16. This is based on the 
   wakeup vector previously stored with ACPI before we went to sleep.
   ACPI's wakeup vector is a physical address - in our case, it's 0x8000.

   We wakeup in real mode, at phys addr 0080:0000, based on the ACPI
   specification (cs = phys>>8, ip = phys & 0xF). 

   The wakeup code needs to do the following:
	1. Reenable the video display
	2. Enter 32 bit protected mode (long mode on amd64)
	3. Reenable paging
	4. Restore saved CPU registers
	5. Restart APs (SMP) - Not currently performed
*/
					
	.code16
	.org 0
wakeup_16:
	nop
	cli
	cld

	movb 	$0xc0, %al
	outb	%al, $0x42
	movb	$0x04, %al
	outb	%al, $0x42
	inb	$0x61, %al
	orb	$0x3, %al
	outb	%al, $0x61

	/* Set up segment registers for real mode. 
           We'll only be in real mode for a moment, and we don't have
  	   ant real dependencies on data or stack, so we'll just use
           the code segment for data and stack (eg, a 64k memory space).
        */
          
	movw	%cs,%ax
	movw	%ax,%ds
	movw	%ax,%ss

	/* Set up stack to grow down from 0x1000.
           We will only be doing a few push/pops and no calls in real 
           mode, so as long as the real mode code in the segment 
           plus stack doesn't exceed 0x1000 (4096) bytes, we'll be ok.
	*/
	movw	$0x1000,%sp

	/* Clear flags */
	pushl	$0
	popfl

	/* Reset the video hardware (as best as we can).
           We call the video bios at c000:0003, similar to
           what the BIOS does on a machine restart.
           Note that this will only reset the video card, 
           and may not enable LCDs or other attached displays.
           
           This will also put the hardware in "factory default"
           display mode, which may not match what we had
           when we went to sleep. Caveat emptor.
	*/

	lcall	$0xc000,$3

	/* Restore our segment registers in case the call to 
           reset the video hardware clobbered them.
        */
	
	movw	%cs,%ax
	movw	%ax,%ds
	movw	%ax,%ss

	/* Get physical address of the intersegment jump location.
           We're going to calculate the jump location for entering
           protected mode and then move this address into a
           ljmp instruction later in the code. 
        */
	xorl	%esi,%esi
	movw	%cs,%si
	shll	$4,%esi

	/* Fill 16->32 address.
           We want the 32 bit physical address of wakeup_32. We
           build this as:    cs*16 + (wakeup_32 - 0x8000).
           The cs*16 was accomplished above via shifting esi
           4 bits left. The wakeup_32 - 0x8000 is actually 
           encoded as wakeup_32 - 0, but we are based at 0x8000,
           so wakeup_32 is calculated correctly, even if the offset
           is wrong here.
        */
	movl	%esi,%eax
	addl	$wakeup_32,%eax
	movl	%eax,wakeup_sw32+2

	/* Flush the instruction prefetch queue by making a few irrelevant
           jumps that effectively do nothing (aside from flushing the 
           prefetch queue, that is..)
        */

	jmp	1f
1:	jmp	1f
1:

	/* We're about to enter protected mode, so we need a GDT for that.
           Set up a temporary GDT describing 2 segments, one for code
           extending from 0x00000000-0xffffffff and one for data
           with the same range. This GDT will only be in use for a short
           time, until we restore the saved GDT that we had when we went
           to sleep.
	*/ 
           
	movl	%esi,%eax
	addl	$tmp_gdtable,%eax
	movl	%eax,tmp_gdt+2
	lgdt	tmp_gdt

	/* Enable protected mode by setting the PE bit in CR0 */
	mov	%cr0,%eax
	orl	$(CR0_PE),%eax
	mov	%eax,%cr0

	.globl wakeup_sw32
wakeup_sw32:
	/* Force the processor into protected mode
           by making an intersegment jump (to ourselves, just a few lines
           down from here.
           
           We've already replaced the 0x12345678 with the correct address
           earlier - we'll end up jumping to wakeup_32.
        */
	ljmpl	$0x8,$0x12345678	/* Code location, to be replaced */


	.code32
	.align	16
	.globl wakeup_32
wakeup_32:
	/* We're in protected mode now, without paging enabled. */

	nop

	movb 	$0x50, %al
	outb	%al, $0x42
	movb	$0x04, %al
	outb	%al, $0x42
	inb	$0x61, %al
	orb	$0x3, %al
	outb	%al, $0x61

	/* Set up segment selectors for protected mode.
           We've already set up our cs via the intersegment jump earlier,
           but we need to set ds,es,fs,gs,ss to all point to the 
           4GB flat data segment we defined earlier.
        */
	movw	$GSEL(GDATA_SEL,SEL_KPL),%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss

	/* Reset ESP based on protected mode. We can do this here
           because we haven't put anything on the stack via a 
           call or push that we haven't cleaned up already.
        */

	movl    %esi, %esp
	addl    $0x800, %esp

	/* Set flags to well-known value before entering long mode */
	pushl $PSL_MBO
	popfl

	/* PAE/PSE is assumed in long mode, so we set those now. */
	movl $(CR4_PAE|CR4_OSFXSR|CR4_OSXMMEXCPT|CR4_PSE), %eax
	movl %eax, %cr4

	/* We'll lose track of our ds when we enter long mode.
           This causes us a potential problem later when we need
           to restore the efer msr, since the previously saved
           efer will be somewhere lost in the "old" ds. To
           accomodate this, we load the old efer value into ebx now, so
           it's available later when we need it.
        */
        movl previous_efer(%esi), %ebx
	

	/* Enter long mode by enabling LME bit in efer msr */
	movl $MSR_EFER, %ecx
	rdmsr
	orl $EFER_LME, %eax
	wrmsr	


	/* Flush the prefetch queue, again. */
	jmp	1f
1:	jmp	1f
1:

	/* Fixup a temporary 64 bit GDT - real GDT to be restored later.
           Set up a temporary GDT describing 2 segments, one for code
           extending from 0x0000000000000000-0xffffffffffffffff and one for data
           with the same range. This GDT will only be in use for a short
           time, until we restore the saved GDT that we had when we went
           to sleep.
	*/ 
           
	movl	%esi,%eax
	addl	$tmp_gdtable64,%eax
	movl	%eax,tmp_gdt64+2
	lgdt	tmp_gdt64

	.globl wakeup_sw64
wakeup_sw64:
	/* Enter long mode operation using another intersegment jump */
	ljmp $0x8, $0x12345678	



	.code64
	.globl wakeup_64
wakeup_64:
	
	/* Restore our saved efer msr value. We saved this msr value
           earlier into ebx.
         */

	movb 	$0x10, %al
	outb	%al, $0x42
	movb	$0x04, %al
	outb	%al, $0x42
	inb	$0x61, %al
	orb	$0x3, %al
	outb	%al, $0x61

	movl %ebx, %eax
	movl $MSR_EFER, %ecx
	wrmsr

        jmp     1f
1:      jmp     1f
1:


	nop

        /* Restore the boot processor's saved registers. Note
           that the order in which we restore the registers is
           important.
        */
	lgdt	previous_gdt(%rsi)
	lidt	previous_idt(%rsi)
        lldt	previous_ldt(%rsi)

	mov	previous_cr2(%rsi),%rax
	mov	%rax,%cr2

	mov	previous_cr4(%rsi),%rax
	mov	%rax,%cr4

        movw	previous_es(%rsi),%ax
        movw	%ax,%es
        movw	previous_fs(%rsi),%ax
        movw	%ax,%fs
        movw	previous_gs(%rsi),%ax
        movw	%ax,%gs
        movw	previous_ss(%rsi),%ax
        movw	%ax,%ss

        /* We're almost done - load the location where we are supposed to
           resume to (saved earlier before suspend) into ebx.
        */
        movq	where_to_recover(%rsi),%rbx
        movw	previous_ds(%rsi),%ax
        movw	%ax,%ds

        /* Everything is almost reset back to the way it was immediately before
           suspend. There are a few more registers to restore - this is
           done in acpi_restorecpu, whose address was placed previously into
           "where_to_recover" below. We'll jump to that routine, and after
           that, jump back to the OS. There's still some things
           to do there, like re-enable interrupts, resume devices, APICs,
           etc.
        */
        jmp     *%rbx


	.align	8
tmp_gdt:
	.word	0xffff
	.long	0

	.align	8, 0
tmp_gdtable:
	/* null */
	.word	0, 0
	.byte	0, 0, 0, 0
	/* code
           Limit: 0xffffffff
           Base: 0x00000000
           Descriptor Type: Code
           Segment Type: CRA 
           Present: True
           Priv: 0
           AVL: False
           64-bit: False
           32-bit: True
        */ 
	.word	0xffff, 0
	.byte	0, 0x9f, 0xcf, 0

	/* data
           Limit: 0xffffffff
           Base: 0x00000000
           Descriptor Type: 
           Segment Type: W
           Present: True
           Priv: 0
           AVL: False
           64-bit: False
           32-bit: True
        */ 
	.word	0xffff, 0
	.byte	0, 0x93, 0xcf, 0


	.align  8,0

tmp_gdt64:
        .word 0xffff
        .long 0x0    /* tmp_gdtable64 + phys wakeup addr (0x8000)  */

tmp_gdtable64:
        .quad 0x0000000000000000
        .quad 0x00af9a000000ffff
        .quad 0x00cf92000000ffff


	.align	16, 0

	.globl physical_gdt
physical_gdt:		.word 0
			.long 0

	.globl previous_cr2
previous_cr2:		.quad 0

	.globl previous_cr3
previous_cr3:		.quad 0

	.globl previous_cr4	
previous_cr4:		.quad 0

	.globl previous_cr0
previous_cr0:		.quad 0

	.globl previous_tr
previous_tr:		.word 0

	.globl previous_gdt
previous_gdt:		.word 0
			.long 0

	.globl previous_ldt
previous_ldt:		.word 0

	.globl previous_idt
previous_idt:		.word 0
			.long 0

	.globl previous_ds
previous_ds:		.word 0

	.globl previous_es
previous_es:		.word 0

	.globl previous_fs
previous_fs:		.word 0

	.globl previous_gs
previous_gs:		.word 0

	.globl previous_ss
previous_ss:		.word 0

	.globl previous_efer
previous_efer:		.long 0

	.globl where_to_recover
where_to_recover:	.quad 0

