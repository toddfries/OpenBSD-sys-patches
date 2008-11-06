#include "machine/asm.h"

.data
	.globl _C_LABEL(acpi_saved_cr0)
	.globl _C_LABEL(acpi_saved_cr2)
	.globl _C_LABEL(acpi_saved_cr3)
	.globl _C_LABEL(acpi_saved_cr4)
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
	.globl _C_LABEL(acpi_saved_esp)

_C_LABEL(acpi_saved_ebx):	.long 0
_C_LABEL(acpi_saved_ecx):	.long 0
_C_LABEL(acpi_saved_edx):	.long 0
_C_LABEL(acpi_saved_ebp):	.long 0
_C_LABEL(acpi_saved_esi):	.long 0
_C_LABEL(acpi_saved_edi):	.long 0
_C_LABEL(acpi_saved_esp):	.long 0

_C_LABEL(acpi_saved_dr0):	.long 0
_C_LABEL(acpi_saved_dr1):	.long 0
_C_LABEL(acpi_saved_dr2):	.long 0
_C_LABEL(acpi_saved_dr3):	.long 0
_C_LABEL(acpi_saved_dr4):	.long 0
_C_LABEL(acpi_saved_dr5):	.long 0
_C_LABEL(acpi_saved_dr6):	.long 0
_C_LABEL(acpi_saved_dr7):	.long 0

_C_LABEL(acpi_saved_fl):	.long 0
_C_LABEL(acpi_saved_cr0):	.long 0
_C_LABEL(acpi_saved_cr2):	.long 0
_C_LABEL(acpi_saved_cr3):	.long 0
_C_LABEL(acpi_saved_cr4):	.long 0

_C_LABEL(acpi_saved_ret):	.long 0

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

	# %eax gets clobbered anyway.
	movl	(%esp), %eax
	movl	%eax, acpi_saved_ret

	movw	%cs, acpi_saved_cs
	movw	%ds, acpi_saved_ds
	movw	%es, acpi_saved_es
	movw	%fs, acpi_saved_fs
	movw	%gs, acpi_saved_gs
	movw	%ss, acpi_saved_ss

	movl	%ebx, acpi_saved_ebx
	movl	%ecx, acpi_saved_ecx
	movl	%edx, acpi_saved_edx
	movl	%ebp, acpi_saved_ebp
	movl	%esi, acpi_saved_esi
	movl	%edi, acpi_saved_edi
	movl	%esp, acpi_saved_esp

	pushfl
	popl	acpi_saved_fl

	movl	%cr0, %eax
	movl	%eax, acpi_saved_cr0
	movl	%cr2, %eax
	movl	%eax, acpi_saved_cr2
	movl	%cr3, %eax
	movl	%eax, acpi_saved_cr3
	movl	%cr4, %eax
	movl	%eax, acpi_saved_cr4

	movl	%dr0, %eax
	movl	%eax, acpi_saved_dr0
	movl	%dr1, %eax
	movl	%eax, acpi_saved_dr1
	movl	%dr2, %eax
	movl	%eax, acpi_saved_dr2
	movl	%dr3, %eax
	movl	%eax, acpi_saved_dr3
	movl	%dr4, %eax
	movl	%eax, acpi_saved_dr4
	movl	%dr5, %eax
	movl	%eax, acpi_saved_dr5
	movl	%dr6, %eax
	movl	%eax, acpi_saved_dr6
	movl	%dr7, %eax
	movl	%eax, acpi_saved_dr7

	sgdt	acpi_saved_gdt
	sidt	acpi_saved_idt
	sldt	acpi_saved_ldt
	str	acpi_saved_tr


	movl	$1, %eax
	ret

NENTRY(acpi_restorecpu)


	movl	acpi_saved_ebx, %ebx
	movl	acpi_saved_ecx, %ecx
	movl	acpi_saved_edx, %edx
	movl	acpi_saved_ebp, %ebp
	movl	acpi_saved_esi, %esi
	movl	acpi_saved_edi, %edi
	movl	acpi_saved_esp, %esp
	movl	acpi_saved_cr0, %eax
	movl	%eax, %cr0
	movl	acpi_saved_cr2, %eax
	movl	%eax, %cr2
	movl	acpi_saved_cr4, %eax
	movl	%eax, %cr4
	movl	acpi_saved_cr3, %eax
	movl	%eax, %cr3

	movl	acpi_saved_dr0, %eax
	movl	%eax, %dr0
	movl	acpi_saved_dr1, %eax
	movl	%eax, %dr1
	movl	acpi_saved_dr2, %eax
	movl	%eax, %dr2
	movl	acpi_saved_dr3, %eax
	movl	%eax, %dr3
	movl	acpi_saved_dr4, %eax
	movl	%eax, %dr4
	movl	acpi_saved_dr5, %eax
	movl	%eax, %dr5
	movl	acpi_saved_dr6, %eax
	movl	%eax, %dr6
	movl	acpi_saved_dr7, %eax
	movl	%eax, %dr7

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
/*	ltr	acpi_saved_tr  */


	xorl  %eax, %eax
	jmp   *acpi_saved_ret
	
