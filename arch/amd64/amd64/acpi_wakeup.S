#include "machine/asm.h"
#include "machine/specialreg.h"

.data
	.globl _C_LABEL(acpi_saved_cr0)
	.globl _C_LABEL(acpi_saved_cr2)
	.globl _C_LABEL(acpi_saved_cr3)
	.globl _C_LABEL(acpi_saved_cr4)
	.globl _C_LABEL(acpi_saved_cr8)
	.globl _C_LABEL(acpi_saved_ldt)
	.globl _C_LABEL(acpi_saved_gdt)
	.globl _C_LABEL(acpi_saved_idt)
	.globl _C_LABEL(acpi_saved_tr)
	.globl _C_LABEL(acpi_saved_cs)
	.globl _C_LABEL(acpi_saved_ds)
	.globl _C_LABEL(acpi_saved_es)
	.globl _C_LABEL(acpi_saved_fs)
	.globl _C_LABEL(acpi_saved_gs)
	.globl _C_LABEL(acpi_saved_ss)
	.globl _C_LABEL(acpi_saved_rsp)
	.globl _C_LABEL(acpi_saved_efer)

_C_LABEL(acpi_saved_rbx):	.quad 0
_C_LABEL(acpi_saved_rcx):	.quad 0
_C_LABEL(acpi_saved_rdx):	.quad 0
_C_LABEL(acpi_saved_r8):	.quad 0
_C_LABEL(acpi_saved_r9):	.quad 0
_C_LABEL(acpi_saved_r10):	.quad 0
_C_LABEL(acpi_saved_r11):	.quad 0
_C_LABEL(acpi_saved_r12):	.quad 0
_C_LABEL(acpi_saved_r13):	.quad 0
_C_LABEL(acpi_saved_r14):	.quad 0
_C_LABEL(acpi_saved_r15):	.quad 0
_C_LABEL(acpi_saved_rbp):	.quad 0
_C_LABEL(acpi_saved_rsi):	.quad 0
_C_LABEL(acpi_saved_rdi):	.quad 0
_C_LABEL(acpi_saved_rsp):	.quad 0

_C_LABEL(acpi_saved_efer):	.long 0

_C_LABEL(acpi_saved_fl):	.quad 0
_C_LABEL(acpi_saved_cr0):	.quad 0
_C_LABEL(acpi_saved_cr2):	.quad 0
_C_LABEL(acpi_saved_cr3):	.quad 0
_C_LABEL(acpi_saved_cr4):	.quad 0
_C_LABEL(acpi_saved_cr8):	.quad 0

_C_LABEL(acpi_saved_dr0):	.quad 0
_C_LABEL(acpi_saved_dr1):	.quad 0
_C_LABEL(acpi_saved_dr2):	.quad 0
_C_LABEL(acpi_saved_dr3):	.quad 0
_C_LABEL(acpi_saved_dr4):	.quad 0
_C_LABEL(acpi_saved_dr5):	.quad 0
_C_LABEL(acpi_saved_dr6):	.quad 0
_C_LABEL(acpi_saved_dr7):	.quad 0

_C_LABEL(acpi_saved_ret):	.quad 0

_C_LABEL(acpi_saved_idt):	.space 6
_C_LABEL(acpi_saved_gdt):	.space 6

_C_LABEL(acpi_saved_ldt):	.short 0
_C_LABEL(acpi_saved_cs):	.short 0
_C_LABEL(acpi_saved_ds):	.short 0
_C_LABEL(acpi_saved_es):	.short 0
_C_LABEL(acpi_saved_fs):	.short 0
_C_LABEL(acpi_saved_gs):	.short 0
_C_LABEL(acpi_saved_ss):	.short 0
_C_LABEL(acpi_saved_tr):	.short 0


NENTRY(acpi_savecpu)
	# %rax gets clobbered anyway.
	movq	(%rsp), %rax
	movq	%rax, acpi_saved_ret

	movw	%cs, acpi_saved_cs
	movw	%ds, acpi_saved_ds
	movw	%es, acpi_saved_es
	movw	%fs, acpi_saved_fs
	movw	%gs, acpi_saved_gs
	movw	%ss, acpi_saved_ss

	movq	%rbx, acpi_saved_rbx
	movq	%rcx, acpi_saved_rcx
	movq	%rdx, acpi_saved_rdx
	movq	%r8, acpi_saved_r8
	movq	%r9, acpi_saved_r9
	movq	%r10, acpi_saved_r10
	movq	%r11, acpi_saved_r11
	movq	%r12, acpi_saved_r12
	movq	%r13, acpi_saved_r13
	movq	%r14, acpi_saved_r14
	movq	%r15, acpi_saved_r15
	movq	%rbp, acpi_saved_rbp
	movq	%rsi, acpi_saved_rsi
	movq	%rdi, acpi_saved_rdi
	movq	%rsp, acpi_saved_rsp

	pushq %rcx
	movl $MSR_EFER, %ecx
	rdmsr
	movl %eax, acpi_saved_efer
	popq %rcx

	pushfq
	popq	acpi_saved_fl

	movq	%cr0, %rax
	movq	%rax, acpi_saved_cr0
	movq	%cr2, %rax
	movq	%rax, acpi_saved_cr2
	movq	%cr3, %rax
	movq	%rax, acpi_saved_cr3
	movq	%cr4, %rax
	movq	%rax, acpi_saved_cr4
	movq	%cr8, %rax
	movq	%rax, acpi_saved_cr8

        movq    %dr0, %rax
        movq    %rax, acpi_saved_dr0
        movq    %dr1, %rax
        movq    %rax, acpi_saved_dr1   
        movq    %dr2, %rax
        movq    %rax, acpi_saved_dr2   
        movq    %dr3, %rax
        movq    %rax, acpi_saved_dr3   
        movq    %dr4, %rax
        movq    %rax, acpi_saved_dr4   
        movq    %dr5, %rax
        movq    %rax, acpi_saved_dr5   
        movq    %dr6, %rax
        movq    %rax, acpi_saved_dr6
        movq    %dr7, %rax
        movq    %rax, acpi_saved_dr7

	sgdt	acpi_saved_gdt
	sidt	acpi_saved_idt
	sldt	acpi_saved_ldt
	str	acpi_saved_tr

	movq	$1, %rax
	ret

NENTRY(acpi_restorecpu)

	movq	acpi_saved_rbx, %rbx
	movq	acpi_saved_rcx, %rcx
	movq	acpi_saved_rdx, %rdx
	movq	acpi_saved_rbp, %rbp
	movq	acpi_saved_rsi, %rsi
	movq	acpi_saved_rdi, %rdi
	movq	acpi_saved_rsp, %rsp
	movq	acpi_saved_cr0, %rax
	movq	%rax, %cr0
	movq	acpi_saved_cr2, %rax
	movq	%rax, %cr2
	movq	acpi_saved_cr4, %rax
	movq	%rax, %cr4
	movq	acpi_saved_cr3, %rax
	movq	%rax, %cr3
	movq	acpi_saved_cr8, %rax
	movq	%rax, %cr8

        movq    acpi_saved_dr0, %rax
        movq    %rax, %dr0
        movq    acpi_saved_dr1, %rax
        movq    %rax, %dr1
        movq    acpi_saved_dr2, %rax
        movq    %rax, %dr2
        movq    acpi_saved_dr3, %rax
        movq    %rax, %dr3
        movq    acpi_saved_dr4, %rax
        movq    %rax, %dr4
        movq    acpi_saved_dr5, %rax
        movq    %rax, %dr5
        movq    acpi_saved_dr6, %rax
        movq    %rax, %dr6
        movq    acpi_saved_dr7, %rax
        movq    %rax, %dr7

	lldt	acpi_saved_ldt
	lidt	acpi_saved_idt
	lgdt	acpi_saved_gdt

/*
 * XXX - The following ltr instruction occasionally causes
 *       A GP(0) violation. According to Intel, this can
 *       happen for various reasons, none of which is
 *       possible based on the register restore code above.
 *       The task register restore is thereby disabled
 *       until the reason for the occasional GP(0) can
 *       be identified.
 *
 *       It's either a bad DS (unlikely), or the saved TR
 *       points to something other than a TSS (equally
 *       unlikely).
 */
/*	ltr	acpi_saved_tr */

	xorq  %rax, %rax
	jmp *acpi_saved_ret
