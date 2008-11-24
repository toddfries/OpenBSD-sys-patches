/*	$OpenBSD: trap.c,v 1.19 1998/09/30 12:40:41 pefo Exp $	*/
/* tracked to 1.23 */
/*-
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 * from: Utah Hdr: trap.c 1.32 91/04/06
 *
 *	from: @(#)trap.c	8.5 (Berkeley) 1/11/94
 *	JNPR: trap.c,v 1.13.2.2 2007/08/29 10:03:49 girish
 */
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/mips/mips/trap.c,v 1.3 2008/09/28 03:50:34 imp Exp $");

#include "opt_ddb.h"
#include "opt_global.h"

#define	NO_REG_DEFS	1	/* Prevent asm.h from including regdef.h */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysent.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>
#include <sys/lock.h>
#include <vm/vm.h>
#include <vm/vm_extern.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>
#include <vm/vm_map.h>
#include <vm/vm_param.h>
#include <sys/vmmeter.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/pioctl.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <sys/bus.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#include <net/netisr.h>

#include <machine/trap.h>
#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/pte.h>
#include <machine/pmap.h>
#include <machine/mips_opcode.h>
#include <machine/frame.h>
#include <machine/regnum.h>
#include <machine/rm7000.h>
#include <machine/archtype.h>
#include <machine/asm.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/ddb.h>
#include <sys/kdb.h>
#endif

#include <sys/cdefs.h>
#include <sys/syslog.h>


#ifdef TRAP_DEBUG
int trap_debug = 1;

#endif

extern unsigned onfault_table[];

extern void MipsKernGenException(void);
extern void MipsUserGenException(void);
extern void MipsKernIntr(void);
extern void MipsUserIntr(void);
extern void MipsTLBInvalidException(void);
extern void MipsKernTLBInvalidException(void);
extern void MipsUserTLBInvalidException(void);
extern void MipsTLBMissException(void);
static void log_bad_page_fault(char *, struct trapframe *, int);
static void log_frame_dump(struct trapframe *frame);
static void get_mapping_info(vm_offset_t, pd_entry_t **, pt_entry_t **);

#ifdef TRAP_DEBUG
static void trap_frame_dump(struct trapframe *frame);

#endif
extern char edata[];

void (*machExceptionTable[]) (void)= {
/*
 * The kernel exception handlers.
 */
	MipsKernIntr,		/* external interrupt */
	MipsKernGenException,	/* TLB modification */
	MipsKernTLBInvalidException,	/* TLB miss (load or instr. fetch) */
	MipsKernTLBInvalidException,	/* TLB miss (store) */
	MipsKernGenException,	/* address error (load or I-fetch) */
	MipsKernGenException,	/* address error (store) */
	MipsKernGenException,	/* bus error (I-fetch) */
	MipsKernGenException,	/* bus error (load or store) */
	MipsKernGenException,	/* system call */
	MipsKernGenException,	/* breakpoint */
	MipsKernGenException,	/* reserved instruction */
	MipsKernGenException,	/* coprocessor unusable */
	MipsKernGenException,	/* arithmetic overflow */
	MipsKernGenException,	/* trap exception */
	MipsKernGenException,	/* virtual coherence exception inst */
	MipsKernGenException,	/* floating point exception */
	MipsKernGenException,	/* reserved */
	MipsKernGenException,	/* reserved */
	MipsKernGenException,	/* reserved */
	MipsKernGenException,	/* reserved */
	MipsKernGenException,	/* reserved */
	MipsKernGenException,	/* reserved */
	MipsKernGenException,	/* reserved */
	MipsKernGenException,	/* watch exception */
	MipsKernGenException,	/* reserved */
	MipsKernGenException,	/* reserved */
	MipsKernGenException,	/* reserved */
	MipsKernGenException,	/* reserved */
	MipsKernGenException,	/* reserved */
	MipsKernGenException,	/* reserved */
	MipsKernGenException,	/* reserved */
	MipsKernGenException,	/* virtual coherence exception data */
/*
 * The user exception handlers.
 */
	MipsUserIntr,		/* 0 */
	MipsUserGenException,	/* 1 */
	MipsUserTLBInvalidException,	/* 2 */
	MipsUserTLBInvalidException,	/* 3 */
	MipsUserGenException,	/* 4 */
	MipsUserGenException,	/* 5 */
	MipsUserGenException,	/* 6 */
	MipsUserGenException,	/* 7 */
	MipsUserGenException,	/* 8 */
	MipsUserGenException,	/* 9 */
	MipsUserGenException,	/* 10 */
	MipsUserGenException,	/* 11 */
	MipsUserGenException,	/* 12 */
	MipsUserGenException,	/* 13 */
	MipsUserGenException,	/* 14 */
	MipsUserGenException,	/* 15 */
	MipsUserGenException,	/* 16 */
	MipsUserGenException,	/* 17 */
	MipsUserGenException,	/* 18 */
	MipsUserGenException,	/* 19 */
	MipsUserGenException,	/* 20 */
	MipsUserGenException,	/* 21 */
	MipsUserGenException,	/* 22 */
	MipsUserGenException,	/* 23 */
	MipsUserGenException,	/* 24 */
	MipsUserGenException,	/* 25 */
	MipsUserGenException,	/* 26 */
	MipsUserGenException,	/* 27 */
	MipsUserGenException,	/* 28 */
	MipsUserGenException,	/* 29 */
	MipsUserGenException,	/* 20 */
	MipsUserGenException,	/* 31 */
};

char *trap_type[] = {
	"external interrupt",
	"TLB modification",
	"TLB miss (load or instr. fetch)",
	"TLB miss (store)",
	"address error (load or I-fetch)",
	"address error (store)",
	"bus error (I-fetch)",
	"bus error (load or store)",
	"system call",
	"breakpoint",
	"reserved instruction",
	"coprocessor unusable",
	"arithmetic overflow",
	"trap",
	"virtual coherency instruction",
	"floating point",
	"reserved 16",
	"reserved 17",
	"reserved 18",
	"reserved 19",
	"reserved 20",
	"reserved 21",
	"reserved 22",
	"watch",
	"reserved 24",
	"reserved 25",
	"reserved 26",
	"reserved 27",
	"reserved 28",
	"reserved 29",
	"reserved 30",
	"virtual coherency data",
};

#if !defined(SMP) && (defined(DDB) || defined(DEBUG))
struct trapdebug trapdebug[TRAPSIZE], *trp = trapdebug;

#endif

#if defined(DDB) || defined(DEBUG)
void stacktrace(struct trapframe *);
void logstacktrace(struct trapframe *);
int kdbpeek(int *);

/* extern functions printed by name in stack backtraces */
extern void MipsTLBMiss(void);
extern void MipsUserSyscallException(void);
extern char _locore[];
extern char _locoreEnd[];

#endif				/* DDB || DEBUG */

extern void MipsSwitchFPState(struct thread *, struct trapframe *);
extern void MipsFPTrap(u_int, u_int, u_int);

u_int trap(struct trapframe *);
u_int MipsEmulateBranch(struct trapframe *, int, int, u_int);

#define	KERNLAND(x) ((int)(x) < 0)
#define	DELAYBRANCH(x) ((int)(x) < 0)

/*
 * kdbpeekD(addr) - skip one word starting at 'addr', then read the second word
 */
#define	kdbpeekD(addr)	kdbpeek(((int *)(addr)) + 1)
int rrs_debug = 0;

/*
 * MIPS load/store access type
 */
enum {
	MIPS_LHU_ACCESS = 1,
	MIPS_LH_ACCESS,
	MIPS_LWU_ACCESS,
	MIPS_LW_ACCESS,
	MIPS_LD_ACCESS,
	MIPS_SH_ACCESS,
	MIPS_SW_ACCESS,
	MIPS_SD_ACCESS
};

char *access_name[] = {
	"Load Halfword Unsigned",
	"Load Halfword",
	"Load Word Unsigned",
	"Load Word",
	"Load Doubleword",
	"Store Halfword",
	"Store Word",
	"Store Doubleword"
};


static int allow_unaligned_acc = 1;

SYSCTL_INT(_vm, OID_AUTO, allow_unaligned_acc, CTLFLAG_RW,
    &allow_unaligned_acc, 0, "Allow unaligned accesses");

static int emulate_unaligned_access(struct trapframe *frame);

extern char *syscallnames[];

/*
 * Handle an exception.
 * Called from MipsKernGenException() or MipsUserGenException()
 * when a processor trap occurs.
 * In the case of a kernel trap, we return the pc where to resume if
 * p->p_addr->u_pcb.pcb_onfault is set, otherwise, return old pc.
 */
u_int
trap(trapframe)
	struct trapframe *trapframe;
{
	int type, usermode;
	int i = 0;
	unsigned ucode = 0;
	struct thread *td = curthread;
	struct proc *p = curproc;
	vm_prot_t ftype;
	pt_entry_t *pte;
	unsigned int entry;
	pmap_t pmap;
	int quad_syscall = 0;
	int access_type;
	ksiginfo_t ksi;
	char *msg = NULL;
	register_t addr = 0;

	trapdebug_enter(trapframe, 0);
	
	type = (trapframe->cause & CR_EXC_CODE) >> CR_EXC_CODE_SHIFT;
	if (USERMODE(trapframe->sr)) {
		type |= T_USER;
		usermode = 1;
	} else {
		usermode = 0;
	}

	/*
	 * Enable hardware interrupts if they were on before the trap. If it
	 * was off disable all so we don't accidently enable it when doing a
	 * return to userland.
	 */
	if (trapframe->sr & SR_INT_ENAB) {
		set_intr_mask(~(trapframe->sr & ALL_INT_MASK));
		enableintr();
	} else {
		disableintr();
	}

#ifdef TRAP_DEBUG
	if (trap_debug) {
		static vm_offset_t last_badvaddr = 0;
		static vm_offset_t this_badvaddr = 0;
		static int count = 0;
		u_int32_t pid;

		printf("trap type %x (%s - ", type,
		    trap_type[type & (~T_USER)]);

		if (type & T_USER)
			printf("user mode)\n");
		else
			printf("kernel mode)\n");

#ifdef SMP
		printf("cpuid = %d\n", PCPU_GET(cpuid));
#endif
		MachTLBGetPID(pid);
		printf("badaddr = %p, pc = %p, ra = %p, sp = %p, sr = 0x%x, pid = %d, ASID = 0x%x\n",
		    trapframe->badvaddr, trapframe->pc, trapframe->ra,
		    trapframe->sp, trapframe->sr,
		    (curproc ? curproc->p_pid : -1), pid);

		switch (type & ~T_USER) {
		case T_TLB_MOD:
		case T_TLB_LD_MISS:
		case T_TLB_ST_MISS:
		case T_ADDR_ERR_LD:
		case T_ADDR_ERR_ST:
			this_badvaddr = trapframe->badvaddr;
			break;
		case T_SYSCALL:
			this_badvaddr = trapframe->ra;
			break;
		default:
			this_badvaddr = trapframe->pc;
			break;
		}
		if ((last_badvaddr == this_badvaddr) &&
		    ((type & ~T_USER) != T_SYSCALL)) {
			if (++count == 3) {
				trap_frame_dump(trapframe);
				panic("too many faults at %p\n", last_badvaddr);
			}
		} else {
			last_badvaddr = this_badvaddr;
			count = 0;
		}
	}
#endif
	switch (type) {
	case T_MCHECK:
#ifdef DDB
		kdb_trap(type, 0, trapframe);
#endif
		panic("MCHECK\n");
		break;
	case T_TLB_MOD:
		/* check for kernel address */
		if (KERNLAND(trapframe->badvaddr)) {
			vm_offset_t pa;

			PMAP_LOCK(kernel_pmap);
			if (!(pte = pmap_segmap(kernel_pmap,
			    trapframe->badvaddr)))
				panic("trap: ktlbmod: invalid segmap");
			pte += (trapframe->badvaddr >> PGSHIFT) & (NPTEPG - 1);
			entry = *pte;
#ifdef SMP
			/* It is possible that some other CPU changed m-bit */
			if (!mips_pg_v(entry) || (entry & mips_pg_m_bit())) {
				trapframe->badvaddr &= ~PGOFSET;
				pmap_update_page(kernel_pmap,
				    trapframe->badvaddr, entry);
				PMAP_UNLOCK(kernel_pmap);
				return (trapframe->pc);
			}
#else
			if (!mips_pg_v(entry) || (entry & mips_pg_m_bit()))
				panic("trap: ktlbmod: invalid pte");
#endif
			if (entry & mips_pg_ro_bit()) {
				/* write to read only page in the kernel */
				ftype = VM_PROT_WRITE;
				PMAP_UNLOCK(kernel_pmap);
				goto kernel_fault;
			}
			entry |= mips_pg_m_bit();
			*pte = entry;
			trapframe->badvaddr &= ~PGOFSET;
			pmap_update_page(kernel_pmap, trapframe->badvaddr, entry);
			pa = mips_tlbpfn_to_paddr(entry);
			if (!page_is_managed(pa))
				panic("trap: ktlbmod: unmanaged page");
			pmap_set_modified(pa);
			PMAP_UNLOCK(kernel_pmap);
			return (trapframe->pc);
		}
		/* FALLTHROUGH */

	case T_TLB_MOD + T_USER:
		{
			vm_offset_t pa;

			pmap = &p->p_vmspace->vm_pmap;

			PMAP_LOCK(pmap);
			if (!(pte = pmap_segmap(pmap, trapframe->badvaddr)))
				panic("trap: utlbmod: invalid segmap");
			pte += (trapframe->badvaddr >> PGSHIFT) & (NPTEPG - 1);
			entry = *pte;
#ifdef SMP
			/* It is possible that some other CPU changed m-bit */
			if (!mips_pg_v(entry) || (entry & mips_pg_m_bit())) {
				trapframe->badvaddr = (trapframe->badvaddr & ~PGOFSET);
				pmap_update_page(pmap, trapframe->badvaddr, entry);
				PMAP_UNLOCK(pmap);
				goto out;
			}
#else
			if (!mips_pg_v(entry) || (entry & mips_pg_m_bit())) {
				panic("trap: utlbmod: invalid pte");
			}
#endif

			if (entry & mips_pg_ro_bit()) {
				/* write to read only page */
				ftype = VM_PROT_WRITE;
				PMAP_UNLOCK(pmap);
				goto dofault;
			}
			entry |= mips_pg_m_bit();
			*pte = entry;
			trapframe->badvaddr = (trapframe->badvaddr & ~PGOFSET);
			pmap_update_page(pmap, trapframe->badvaddr, entry);
			trapframe->badvaddr |= (pmap->pm_asid[PCPU_GET(cpuid)].asid << VMTLB_PID_SHIFT);
			pa = mips_tlbpfn_to_paddr(entry);
			if (!page_is_managed(pa))
				panic("trap: utlbmod: unmanaged page");
			pmap_set_modified(pa);

			PMAP_UNLOCK(pmap);
			if (!usermode) {
				return (trapframe->pc);
			}
			goto out;
		}

	case T_TLB_LD_MISS:
	case T_TLB_ST_MISS:
		ftype = (type == T_TLB_ST_MISS) ? VM_PROT_WRITE : VM_PROT_READ;
		/* check for kernel address */
		if (KERNLAND(trapframe->badvaddr)) {
			vm_offset_t va;
			int rv;

	kernel_fault:
			va = trunc_page((vm_offset_t)trapframe->badvaddr);
			rv = vm_fault(kernel_map, va, ftype, VM_FAULT_NORMAL);
			if (rv == KERN_SUCCESS)
				return (trapframe->pc);
			if ((i = td->td_pcb->pcb_onfault) != 0) {
				td->td_pcb->pcb_onfault = 0;
				return (onfault_table[i]);
			}
			goto err;
		}
		/*
		 * It is an error for the kernel to access user space except
		 * through the copyin/copyout routines.
		 */
		if ((i = td->td_pcb->pcb_onfault) == 0)
			goto err;
		/* check for fuswintr() or suswintr() getting a page fault */
		if (i == 4) {
			return (onfault_table[i]);
		}
		goto dofault;

	case T_TLB_LD_MISS + T_USER:
		ftype = VM_PROT_READ;
		goto dofault;

	case T_TLB_ST_MISS + T_USER:
		ftype = VM_PROT_WRITE;
dofault:
		{
			vm_offset_t va;
			struct vmspace *vm;
			vm_map_t map;
			int rv = 0;
			int flag;

			vm = p->p_vmspace;
			map = &vm->vm_map;
			va = trunc_page((vm_offset_t)trapframe->badvaddr);
			if ((vm_offset_t)trapframe->badvaddr < VM_MIN_KERNEL_ADDRESS) {
				if (ftype & VM_PROT_WRITE)
					flag = VM_FAULT_DIRTY;
				else
					flag = VM_FAULT_NORMAL;
			} else {
				/*
				 * Don't allow user-mode faults in kernel
				 * address space.
				 */
				goto nogo;
			}

			/*
			 * Keep swapout from messing with us during this
			 * critical time.
			 */
			PROC_LOCK(p);
			++p->p_lock;
			PROC_UNLOCK(p);

			rv = vm_fault(map, va, ftype, flag);

			PROC_LOCK(p);
			--p->p_lock;
			PROC_UNLOCK(p);
#ifdef VMFAULT_TRACE
			printf("vm_fault(%x (pmap %x), %x (%x), %x, %d) -> %x at pc %x\n",
			    map, &vm->vm_pmap, va, trapframe->badvaddr, ftype, flag,
			    rv, trapframe->pc);
#endif

			if (rv == KERN_SUCCESS) {
				if (!usermode) {
					return (trapframe->pc);
				}
				goto out;
			}
	nogo:
			if (!usermode) {
				if ((i = td->td_pcb->pcb_onfault) != 0) {
					td->td_pcb->pcb_onfault = 0;
					return (onfault_table[i]);
				}
				goto err;
			}
			ucode = ftype;
			i = ((rv == KERN_PROTECTION_FAILURE) ? SIGBUS : SIGSEGV);
			addr = trapframe->pc;

			msg = "BAD_PAGE_FAULT";
			log_bad_page_fault(msg, trapframe, type);

			break;
		}

	case T_ADDR_ERR_LD + T_USER:	/* misaligned or kseg access */
	case T_ADDR_ERR_ST + T_USER:	/* misaligned or kseg access */
		if (allow_unaligned_acc) {
			int mode;

			if (type == (T_ADDR_ERR_LD + T_USER))
				mode = VM_PROT_READ;
			else
				mode = VM_PROT_WRITE;

			/*
			 * ADDR_ERR faults have higher priority than TLB
			 * Miss faults.  Therefore, it is necessary to
			 * verify that the faulting address is a valid
			 * virtual address within the process' address space
			 * before trying to emulate the unaligned access.
			 */
			if (useracc((caddr_t)
			    (((vm_offset_t)trapframe->badvaddr) &
			    ~(sizeof(int) - 1)), sizeof(int) * 2, mode)) {
				access_type = emulate_unaligned_access(
				    trapframe);
				if (access_type != 0)
					goto out;
			}
		}
		msg = "ADDRESS_ERR";

		/* FALL THROUGH */

	case T_BUS_ERR_IFETCH + T_USER:	/* BERR asserted to cpu */
	case T_BUS_ERR_LD_ST + T_USER:	/* BERR asserted to cpu */
		ucode = 0;	/* XXX should be VM_PROT_something */
		i = SIGBUS;
		addr = trapframe->pc;
		if (!msg)
			msg = "BUS_ERR";
		log_bad_page_fault(msg, trapframe, type);
		break;

	case T_SYSCALL + T_USER:
		{
			struct trapframe *locr0 = td->td_frame;
			struct sysent *callp;
			unsigned int code;
			unsigned int tpc;
			int nargs, nsaved;
			register_t args[8];

			/*
			 * note: PCPU_LAZY_INC() can only be used if we can
			 * afford occassional inaccuracy in the count.
			 */
			PCPU_LAZY_INC(cnt.v_syscall);
			if (td->td_ucred != p->p_ucred)
				cred_update_thread(td);
#ifdef KSE
			if (p->p_flag & P_SA)
				thread_user_enter(td);
#endif
			/* compute next PC after syscall instruction */
			tpc = trapframe->pc;	/* Remember if restart */
			if (DELAYBRANCH(trapframe->cause)) {	/* Check BD bit */
				locr0->pc = MipsEmulateBranch(locr0, trapframe->pc, 0,
				    0);
			} else {
				locr0->pc += sizeof(int);
			}
			code = locr0->v0;

			switch (code) {
			case SYS_syscall:
				/*
				 * Code is first argument, followed by
				 * actual args.
				 */
				code = locr0->a0;
				args[0] = locr0->a1;
				args[1] = locr0->a2;
				args[2] = locr0->a3;
				nsaved = 3;
				break;

			case SYS___syscall:
				/*
				 * Like syscall, but code is a quad, so as
				 * to maintain quad alignment for the rest
				 * of the arguments.
				 */
				if (_QUAD_LOWWORD == 0) {
					code = locr0->a0;
				} else {
					code = locr0->a1;
				}
				args[0] = locr0->a2;
				args[1] = locr0->a3;
				nsaved = 2;
				quad_syscall = 1;
				break;

			default:
				args[0] = locr0->a0;
				args[1] = locr0->a1;
				args[2] = locr0->a2;
				args[3] = locr0->a3;
				nsaved = 4;
			}
#ifdef TRAP_DEBUG
			printf("SYSCALL #%d pid:%u\n", code, p->p_pid);
#endif

			if (p->p_sysent->sv_mask)
				code &= p->p_sysent->sv_mask;

			if (code >= p->p_sysent->sv_size)
				callp = &p->p_sysent->sv_table[0];
			else
				callp = &p->p_sysent->sv_table[code];

			nargs = callp->sy_narg;

			if (nargs > nsaved) {
				i = copyin((caddr_t)(locr0->sp +
				    4 * sizeof(register_t)), (caddr_t)&args[nsaved],
				    (u_int)(nargs - nsaved) * sizeof(register_t));
				if (i) {
					locr0->v0 = i;
					locr0->a3 = 1;
#ifdef KTRACE
					if (KTRPOINT(td, KTR_SYSCALL))
						ktrsyscall(code, nargs, args);
#endif
					goto done;
				}
			}
#ifdef KTRACE
			if (KTRPOINT(td, KTR_SYSCALL))
				ktrsyscall(code, nargs, args);
#endif
			td->td_retval[0] = 0;
			td->td_retval[1] = locr0->v1;

#if !defined(SMP) && (defined(DDB) || defined(DEBUG))
			if (trp == trapdebug)
				trapdebug[TRAPSIZE - 1].code = code;
			else
				trp[-1].code = code;
#endif
			STOPEVENT(p, S_SCE, nargs);

			PTRACESTOP_SC(p, td, S_PT_SCE);
			i = (*callp->sy_call) (td, args);
#if 0
			/*
			 * Reinitialize proc pointer `p' as it may be
			 * different if this is a child returning from fork
			 * syscall.
			 */
			td = curthread;
			locr0 = td->td_frame;
#endif
			trapdebug_enter(locr0, -code);
			switch (i) {
			case 0:
				if (quad_syscall && code != SYS_lseek) {
					/*
					 * System call invoked through the
					 * SYS___syscall interface but the
					 * return value is really just 32
					 * bits.
					 */
					locr0->v0 = td->td_retval[0];
					if (_QUAD_LOWWORD)
						locr0->v1 = td->td_retval[0];
					locr0->a3 = 0;
				} else {
					locr0->v0 = td->td_retval[0];
					locr0->v1 = td->td_retval[1];
					locr0->a3 = 0;
				}
				break;

			case ERESTART:
				locr0->pc = tpc;
				break;

			case EJUSTRETURN:
				break;	/* nothing to do */

			default:
				if (quad_syscall && code != SYS_lseek) {
					locr0->v0 = i;
					if (_QUAD_LOWWORD)
						locr0->v1 = i;
					locr0->a3 = 1;
				} else {
					locr0->v0 = i;
					locr0->a3 = 1;
				}
			}

			/*
			 * The sync'ing of I & D caches for SYS_ptrace() is
			 * done by procfs_domem() through procfs_rwmem()
			 * instead of being done here under a special check
			 * for SYS_ptrace().
			 */
	done:
			/*
			 * Check for misbehavior.
			 */
			WITNESS_WARN(WARN_PANIC, NULL, "System call %s returning",
			    (code >= 0 && code < SYS_MAXSYSCALL) ?
			    syscallnames[code] : "???");
			KASSERT(td->td_critnest == 0,
			    ("System call %s returning in a critical section",
			    (code >= 0 && code < SYS_MAXSYSCALL) ?
			    syscallnames[code] : "???"));
			KASSERT(td->td_locks == 0,
			    ("System call %s returning with %d locks held",
			    (code >= 0 && code < SYS_MAXSYSCALL) ?
			    syscallnames[code] : "???",
			    td->td_locks));
			userret(td, trapframe);
#ifdef KTRACE
			if (KTRPOINT(p, KTR_SYSRET))
				ktrsysret(code, i, td->td_retval[0]);
#endif
			/*
			 * This works because errno is findable through the
			 * register set.  If we ever support an emulation
			 * where this is not the case, this code will need
			 * to be revisited.
			 */
			STOPEVENT(p, S_SCX, code);

			PTRACESTOP_SC(p, td, S_PT_SCX);

			mtx_assert(&Giant, MA_NOTOWNED);
			return (trapframe->pc);
		}

#ifdef DDB
	case T_BREAK:
		kdb_trap(type, 0, trapframe);
		return (trapframe->pc);
#endif

	case T_BREAK + T_USER:
		{
			unsigned int va, instr;

			/* compute address of break instruction */
			va = trapframe->pc;
			if (DELAYBRANCH(trapframe->cause))
				va += sizeof(int);

			/* read break instruction */
			instr = fuword((caddr_t)va);
#if 0
			printf("trap: %s (%d) breakpoint %x at %x: (adr %x ins %x)\n",
			    p->p_comm, p->p_pid, instr, trapframe->pc,
			    p->p_md.md_ss_addr, p->p_md.md_ss_instr);	/* XXX */
#endif
			if (td->td_md.md_ss_addr != va || instr != BREAK_SSTEP) {
				i = SIGTRAP;
				addr = trapframe->pc;
				break;
			}
			/*
			 * The restoration of the original instruction and
			 * the clearing of the berakpoint will be done later
			 * by the call to ptrace_clear_single_step() in
			 * issignal() when SIGTRAP is processed.
			 */
			addr = trapframe->pc;
			i = SIGTRAP;
			break;
		}

	case T_IWATCH + T_USER:
	case T_DWATCH + T_USER:
		{
			unsigned int va;

			/* compute address of trapped instruction */
			va = trapframe->pc;
			if (DELAYBRANCH(trapframe->cause))
				va += sizeof(int);
			printf("watch exception @ 0x%x\n", va);
			i = SIGTRAP;
			addr = va;
			break;
		}

	case T_TRAP + T_USER:
		{
			unsigned int va, instr;
			struct trapframe *locr0 = td->td_frame;

			/* compute address of trap instruction */
			va = trapframe->pc;
			if (DELAYBRANCH(trapframe->cause))
				va += sizeof(int);
			/* read break instruction */
			instr = fuword((caddr_t)va);

			if (DELAYBRANCH(trapframe->cause)) {	/* Check BD bit */
				locr0->pc = MipsEmulateBranch(locr0, trapframe->pc, 0,
				    0);
			} else {
				locr0->pc += sizeof(int);
			}
			addr = va;
			i = SIGEMT;	/* Stuff it with something for now */
			break;
		}

	case T_RES_INST + T_USER:
		i = SIGILL;
		addr = trapframe->pc;
		break;
	case T_C2E:
	case T_C2E + T_USER:
		goto err;
		break;
	case T_COP_UNUSABLE:
		goto err;
		break;
	case T_COP_UNUSABLE + T_USER:
#if defined(SOFTFLOAT)
		/* FP (COP1) instruction */
		if ((trapframe->cause & CR_COP_ERR) == 0x10000000) {
			i = SIGILL;
			break;
		}
#endif
		if ((trapframe->cause & CR_COP_ERR) != 0x10000000) {
			i = SIGILL;	/* only FPU instructions allowed */
			break;
		}
		addr = trapframe->pc;
		MipsSwitchFPState(PCPU_GET(fpcurthread), td->td_frame);
		PCPU_SET(fpcurthread, td);
		td->td_frame->sr |= SR_COP_1_BIT;
		td->td_md.md_flags |= MDTD_FPUSED;
		goto out;

	case T_FPE:
#if !defined(SMP) && (defined(DDB) || defined(DEBUG))
		trapDump("fpintr");
#else
		printf("FPU Trap: PC %x CR %x SR %x\n",
		    trapframe->pc, trapframe->cause, trapframe->sr);
		goto err;
#endif

	case T_FPE + T_USER:
		MachFPTrap(trapframe->sr, trapframe->cause, trapframe->pc);
		goto out;

	case T_OVFLOW + T_USER:
		i = SIGFPE;
		addr = trapframe->pc;
		break;

	case T_ADDR_ERR_LD:	/* misaligned access */
	case T_ADDR_ERR_ST:	/* misaligned access */
#ifdef TRAP_DEBUG
		printf("+++ ADDR_ERR: type = %d, badvaddr = %x\n", type,
		    trapframe->badvaddr);
#endif
		/* Only allow emulation on a user address */
		if (allow_unaligned_acc &&
		    ((vm_offset_t)trapframe->badvaddr < VM_MAXUSER_ADDRESS)) {
			int mode;

			if (type == T_ADDR_ERR_LD)
				mode = VM_PROT_READ;
			else
				mode = VM_PROT_WRITE;

			/*
			 * ADDR_ERR faults have higher priority than TLB
			 * Miss faults.  Therefore, it is necessary to
			 * verify that the faulting address is a valid
			 * virtual address within the process' address space
			 * before trying to emulate the unaligned access.
			 */
			if (useracc((caddr_t)
			    (((vm_offset_t)trapframe->badvaddr) &
			    ~(sizeof(int) - 1)), sizeof(int) * 2, mode)) {
				access_type = emulate_unaligned_access(
				    trapframe);
				if (access_type != 0) {
					return (trapframe->pc);
				}
			}
		}
		/* FALLTHROUGH */

	case T_BUS_ERR_LD_ST:	/* BERR asserted to cpu */
		if ((i = td->td_pcb->pcb_onfault) != 0) {
			td->td_pcb->pcb_onfault = 0;
			return (onfault_table[i]);
		}
		/* FALLTHROUGH */

	default:
err:

#if !defined(SMP) && defined(DEBUG)
		stacktrace(!usermode ? trapframe : td->td_frame);
		trapDump("trap");
#endif
#ifdef SMP
		printf("cpu:%d-", PCPU_GET(cpuid));
#endif
		printf("Trap cause = %d (%s - ", type,
		    trap_type[type & (~T_USER)]);

		if (type & T_USER)
			printf("user mode)\n");
		else
			printf("kernel mode)\n");

#ifdef TRAP_DEBUG
		printf("badvaddr = %x, pc = %x, ra = %x, sr = 0x%x\n",
		       trapframe->badvaddr, trapframe->pc, trapframe->ra,
		       trapframe->sr);
#endif

#ifdef KDB
		if (debugger_on_panic || kdb_active) {
			kdb_trap(type, 0, trapframe);
		}
#endif
		panic("trap");
	}
	td->td_frame->pc = trapframe->pc;
	td->td_frame->cause = trapframe->cause;
	td->td_frame->badvaddr = trapframe->badvaddr;
	ksiginfo_init_trap(&ksi);
	ksi.ksi_signo = i;
	ksi.ksi_code = ucode;
	ksi.ksi_addr = (void *)addr;
	ksi.ksi_trapno = type;
	trapsignal(td, &ksi);
out:

	/*
	 * Note: we should only get here if returning to user mode.
	 */
	userret(td, trapframe);
	mtx_assert(&Giant, MA_NOTOWNED);
	return (trapframe->pc);
}

#if !defined(SMP) && (defined(DDB) || defined(DEBUG))
void
trapDump(char *msg)
{
	int i, s;

	s = disableintr();
	printf("trapDump(%s)\n", msg);
	for (i = 0; i < TRAPSIZE; i++) {
		if (trp == trapdebug) {
			trp = &trapdebug[TRAPSIZE - 1];
		} else {
			trp--;
		}

		if (trp->cause == 0)
			break;

		printf("%s: ADR %x PC %x CR %x SR %x\n",
		    trap_type[(trp->cause & CR_EXC_CODE) >> CR_EXC_CODE_SHIFT],
		    trp->vadr, trp->pc, trp->cause, trp->status);

		printf("   RA %x SP %x code %d\n", trp->ra, trp->sp, trp->code);
	}
	restoreintr(s);
}

#endif


/*
 * Return the resulting PC as if the branch was executed.
 */
u_int
MipsEmulateBranch(struct trapframe *framePtr, int instPC, int fpcCSR,
    u_int instptr)
{
	InstFmt inst;
	register_t *regsPtr = (register_t *) framePtr;
	unsigned retAddr = 0;
	int condition;

#define	GetBranchDest(InstPtr, inst) \
	((unsigned)InstPtr + 4 + ((short)inst.IType.imm << 2))


	if (instptr) {
		if (instptr < MIPS_KSEG0_START)
			inst.word = fuword((void *)instptr);
		else
			inst = *(InstFmt *) instptr;
	} else {
		if ((vm_offset_t)instPC < MIPS_KSEG0_START)
			inst.word = fuword((void *)instPC);
		else
			inst = *(InstFmt *) instPC;
	}

	switch ((int)inst.JType.op) {
	case OP_SPECIAL:
		switch ((int)inst.RType.func) {
		case OP_JR:
		case OP_JALR:
			retAddr = regsPtr[inst.RType.rs];
			break;

		default:
			retAddr = instPC + 4;
			break;
		}
		break;

	case OP_BCOND:
		switch ((int)inst.IType.rt) {
		case OP_BLTZ:
		case OP_BLTZL:
		case OP_BLTZAL:
		case OP_BLTZALL:
			if ((int)(regsPtr[inst.RType.rs]) < 0)
				retAddr = GetBranchDest(instPC, inst);
			else
				retAddr = instPC + 8;
			break;

		case OP_BGEZ:
		case OP_BGEZL:
		case OP_BGEZAL:
		case OP_BGEZALL:
			if ((int)(regsPtr[inst.RType.rs]) >= 0)
				retAddr = GetBranchDest(instPC, inst);
			else
				retAddr = instPC + 8;
			break;

		case OP_TGEI:
		case OP_TGEIU:
		case OP_TLTI:
		case OP_TLTIU:
		case OP_TEQI:
		case OP_TNEI:
			retAddr = instPC + 4;	/* Like syscall... */
			break;

		default:
			panic("MipsEmulateBranch: Bad branch cond");
		}
		break;

	case OP_J:
	case OP_JAL:
		retAddr = (inst.JType.target << 2) |
		    ((unsigned)instPC & 0xF0000000);
		break;

	case OP_BEQ:
	case OP_BEQL:
		if (regsPtr[inst.RType.rs] == regsPtr[inst.RType.rt])
			retAddr = GetBranchDest(instPC, inst);
		else
			retAddr = instPC + 8;
		break;

	case OP_BNE:
	case OP_BNEL:
		if (regsPtr[inst.RType.rs] != regsPtr[inst.RType.rt])
			retAddr = GetBranchDest(instPC, inst);
		else
			retAddr = instPC + 8;
		break;

	case OP_BLEZ:
	case OP_BLEZL:
		if ((int)(regsPtr[inst.RType.rs]) <= 0)
			retAddr = GetBranchDest(instPC, inst);
		else
			retAddr = instPC + 8;
		break;

	case OP_BGTZ:
	case OP_BGTZL:
		if ((int)(regsPtr[inst.RType.rs]) > 0)
			retAddr = GetBranchDest(instPC, inst);
		else
			retAddr = instPC + 8;
		break;

	case OP_COP1:
		switch (inst.RType.rs) {
		case OP_BCx:
		case OP_BCy:
			if ((inst.RType.rt & COPz_BC_TF_MASK) == COPz_BC_TRUE)
				condition = fpcCSR & FPC_COND_BIT;
			else
				condition = !(fpcCSR & FPC_COND_BIT);
			if (condition)
				retAddr = GetBranchDest(instPC, inst);
			else
				retAddr = instPC + 8;
			break;

		default:
			retAddr = instPC + 4;
		}
		break;

	default:
		retAddr = instPC + 4;
	}
	return (retAddr);
}


#if defined(DDB) || defined(DEBUG)
#define	MIPS_JR_RA	0x03e00008	/* instruction code for jr ra */

/* forward */
char *fn_name(unsigned addr);

/*
 * Print a stack backtrace.
 */
void
stacktrace(struct trapframe *regs)
{
	stacktrace_subr(regs, printf);
}

void
stacktrace_subr(struct trapframe *regs, int (*printfn) (const char *,...))
{
	InstFmt i;
	unsigned a0, a1, a2, a3, pc, sp, fp, ra, va, subr;
	unsigned instr, mask;
	unsigned int frames = 0;
	int more, stksize;

	/* get initial values from the exception frame */
	sp = regs->sp;
	pc = regs->pc;
	fp = regs->s8;
	ra = regs->ra;		/* May be a 'leaf' function */
	a0 = regs->a0;
	a1 = regs->a1;
	a2 = regs->a2;
	a3 = regs->a3;

/* Jump here when done with a frame, to start a new one */
loop:

/* Jump here after a nonstandard (interrupt handler) frame */
	stksize = 0;
	subr = 0;
	if (frames++ > 100) {
		(*printfn) ("\nstackframe count exceeded\n");
		/* return breaks stackframe-size heuristics with gcc -O2 */
		goto finish;	/* XXX */
	}
	/* check for bad SP: could foul up next frame */
	if (sp & 3 || sp < 0x80000000) {
		(*printfn) ("SP 0x%x: not in kernel\n", sp);
		ra = 0;
		subr = 0;
		goto done;
	}
#define Between(x, y, z) \
		( ((x) <= (y)) && ((y) < (z)) )
#define pcBetween(a,b) \
		Between((unsigned)a, pc, (unsigned)b)

	/*
	 * Check for current PC in  exception handler code that don't have a
	 * preceding "j ra" at the tail of the preceding function. Depends
	 * on relative ordering of functions in exception.S, swtch.S.
	 */
	if (pcBetween(MipsKernGenException, MipsUserGenException))
		subr = (unsigned)MipsKernGenException;
	else if (pcBetween(MipsUserGenException, MipsKernIntr))
		subr = (unsigned)MipsUserGenException;
	else if (pcBetween(MipsKernIntr, MipsUserIntr))
		subr = (unsigned)MipsKernIntr;
	else if (pcBetween(MipsUserIntr, MipsTLBInvalidException))
		subr = (unsigned)MipsUserIntr;
	else if (pcBetween(MipsTLBInvalidException,
	    MipsKernTLBInvalidException))
		subr = (unsigned)MipsTLBInvalidException;
	else if (pcBetween(MipsKernTLBInvalidException,
	    MipsUserTLBInvalidException))
		subr = (unsigned)MipsKernTLBInvalidException;
	else if (pcBetween(MipsUserTLBInvalidException, MipsTLBMissException))
		subr = (unsigned)MipsUserTLBInvalidException;
	else if (pcBetween(cpu_switch, MipsSwitchFPState))
		subr = (unsigned)cpu_switch;
	else if (pcBetween(_locore, _locoreEnd)) {
		subr = (unsigned)_locore;
		ra = 0;
		goto done;
	}
	/* check for bad PC */
	if (pc & 3 || pc < (unsigned)0x80000000 || pc >= (unsigned)edata) {
		(*printfn) ("PC 0x%x: not in kernel\n", pc);
		ra = 0;
		goto done;
	}
	/*
	 * Find the beginning of the current subroutine by scanning
	 * backwards from the current PC for the end of the previous
	 * subroutine.
	 */
	if (!subr) {
		va = pc - sizeof(int);
		while ((instr = kdbpeek((int *)va)) != MIPS_JR_RA)
			va -= sizeof(int);
		va += 2 * sizeof(int);	/* skip back over branch & delay slot */
		/* skip over nulls which might separate .o files */
		while ((instr = kdbpeek((int *)va)) == 0)
			va += sizeof(int);
		subr = va;
	}
	/* scan forwards to find stack size and any saved registers */
	stksize = 0;
	more = 3;
	mask = 0;
	for (va = subr; more; va += sizeof(int),
	    more = (more == 3) ? 3 : more - 1) {
		/* stop if hit our current position */
		if (va >= pc)
			break;
		instr = kdbpeek((int *)va);
		i.word = instr;
		switch (i.JType.op) {
		case OP_SPECIAL:
			switch (i.RType.func) {
			case OP_JR:
			case OP_JALR:
				more = 2;	/* stop after next instruction */
				break;

			case OP_SYSCALL:
			case OP_BREAK:
				more = 1;	/* stop now */
			};
			break;

		case OP_BCOND:
		case OP_J:
		case OP_JAL:
		case OP_BEQ:
		case OP_BNE:
		case OP_BLEZ:
		case OP_BGTZ:
			more = 2;	/* stop after next instruction */
			break;

		case OP_COP0:
		case OP_COP1:
		case OP_COP2:
		case OP_COP3:
			switch (i.RType.rs) {
			case OP_BCx:
			case OP_BCy:
				more = 2;	/* stop after next instruction */
			};
			break;

		case OP_SW:
			/* look for saved registers on the stack */
			if (i.IType.rs != 29)
				break;
			/* only restore the first one */
			if (mask & (1 << i.IType.rt))
				break;
			mask |= (1 << i.IType.rt);
			switch (i.IType.rt) {
			case 4:/* a0 */
				a0 = kdbpeek((int *)(sp + (short)i.IType.imm));
				break;

			case 5:/* a1 */
				a1 = kdbpeek((int *)(sp + (short)i.IType.imm));
				break;

			case 6:/* a2 */
				a2 = kdbpeek((int *)(sp + (short)i.IType.imm));
				break;

			case 7:/* a3 */
				a3 = kdbpeek((int *)(sp + (short)i.IType.imm));
				break;

			case 30:	/* fp */
				fp = kdbpeek((int *)(sp + (short)i.IType.imm));
				break;

			case 31:	/* ra */
				ra = kdbpeek((int *)(sp + (short)i.IType.imm));
			}
			break;

		case OP_SD:
			/* look for saved registers on the stack */
			if (i.IType.rs != 29)
				break;
			/* only restore the first one */
			if (mask & (1 << i.IType.rt))
				break;
			mask |= (1 << i.IType.rt);
			switch (i.IType.rt) {
			case 4:/* a0 */
				a0 = kdbpeekD((int *)(sp + (short)i.IType.imm));
				break;

			case 5:/* a1 */
				a1 = kdbpeekD((int *)(sp + (short)i.IType.imm));
				break;

			case 6:/* a2 */
				a2 = kdbpeekD((int *)(sp + (short)i.IType.imm));
				break;

			case 7:/* a3 */
				a3 = kdbpeekD((int *)(sp + (short)i.IType.imm));
				break;

			case 30:	/* fp */
				fp = kdbpeekD((int *)(sp + (short)i.IType.imm));
				break;

			case 31:	/* ra */
				ra = kdbpeekD((int *)(sp + (short)i.IType.imm));
			}
			break;

		case OP_ADDI:
		case OP_ADDIU:
			/* look for stack pointer adjustment */
			if (i.IType.rs != 29 || i.IType.rt != 29)
				break;
			stksize = -((short)i.IType.imm);
		}
	}

done:
	(*printfn) ("%s+%x (%x,%x,%x,%x) ra %x sz %d\n",
	    fn_name(subr), pc - subr, a0, a1, a2, a3, ra, stksize);

	if (ra) {
		if (pc == ra && stksize == 0)
			(*printfn) ("stacktrace: loop!\n");
		else {
			pc = ra;
			sp += stksize;
			ra = 0;
			goto loop;
		}
	} else {
finish:
		if (curproc)
			(*printfn) ("pid %d\n", curproc->p_pid);
		else
			(*printfn) ("curproc NULL\n");
	}
}

/*
 * Functions ``special'' enough to print by name
 */
#ifdef __STDC__
#define	Name(_fn)  { (void*)_fn, # _fn }
#else
#define	Name(_fn) { _fn, "_fn"}
#endif
static struct {
	void *addr;
	char *name;
}      names[] = {

	Name(trap),
	Name(MipsKernGenException),
	Name(MipsUserGenException),
	Name(MipsKernIntr),
	Name(MipsUserIntr),
	Name(cpu_switch),
	{
		0, 0
	}
};

/*
 * Map a function address to a string name, if known; or a hex string.
 */
char *
fn_name(unsigned addr)
{
	static char buf[17];
	int i = 0;

#ifdef DDB
	db_expr_t diff;
	c_db_sym_t sym;
	char *symname;

	diff = 0;
	symname = NULL;
	sym = db_search_symbol((db_addr_t)addr, DB_STGY_ANY, &diff);
	db_symbol_values(sym, (const char **)&symname, (db_expr_t *)0);
	if (symname && diff == 0)
		return (symname);
#endif

	for (i = 0; names[i].name; i++)
		if (names[i].addr == (void *)addr)
			return (names[i].name);
	sprintf(buf, "%x", addr);
	return (buf);
}

#endif				/* DDB */

static void
log_frame_dump(struct trapframe *frame)
{
	log(LOG_ERR, "Trapframe Register Dump:\n");
	log(LOG_ERR, "\tzero: %08x\tat: %08x\tv0: %08x\tv1: %08x\n",
	    0, frame->ast, frame->v0, frame->v1);

	log(LOG_ERR, "\ta0: %08x\ta1: %08x\ta2: %08x\ta3: %08x\n",
	    frame->a0, frame->a1, frame->a2, frame->a3);

	log(LOG_ERR, "\tt0: %08x\tt1: %08x\tt2: %08x\tt3: %08x\n",
	    frame->t0, frame->t1, frame->t2, frame->t3);

	log(LOG_ERR, "\tt4: %08x\tt5: %08x\tt6: %08x\tt7: %08x\n",
	    frame->t4, frame->t5, frame->t6, frame->t7);

	log(LOG_ERR, "\tt8: %08x\tt9: %08x\ts0: %08x\ts1: %08x\n",
	    frame->t8, frame->t9, frame->s0, frame->s1);

	log(LOG_ERR, "\ts2: %08x\ts3: %08x\ts4: %08x\ts5: %08x\n",
	    frame->s2, frame->s3, frame->s4, frame->s5);

	log(LOG_ERR, "\ts6: %08x\ts7: %08x\tk0: %08x\tk1: %08x\n",
	    frame->s6, frame->s7, frame->k0, frame->k1);

	log(LOG_ERR, "\tgp: %08x\tsp: %08x\ts8: %08x\tra: %08x\n",
	    frame->gp, frame->sp, frame->s8, frame->ra);

	log(LOG_ERR, "\tsr: %08x\tmullo: %08x\tmulhi: %08x\tbadvaddr: %08x\n",
	    frame->sr, frame->mullo, frame->mulhi, frame->badvaddr);

#ifdef IC_REG
	log(LOG_ERR, "\tcause: %08x\tpc: %08x\tic: %08x\n",
	    frame->cause, frame->pc, frame->ic);
#else
	log(LOG_ERR, "\tcause: %08x\tpc: %08x\n",
	    frame->cause, frame->pc);
#endif
}

#ifdef TRAP_DEBUG
static void
trap_frame_dump(struct trapframe *frame)
{
	printf("Trapframe Register Dump:\n");
	printf("\tzero: %08x\tat: %08x\tv0: %08x\tv1: %08x\n",
	    0, frame->ast, frame->v0, frame->v1);

	printf("\ta0: %08x\ta1: %08x\ta2: %08x\ta3: %08x\n",
	    frame->a0, frame->a1, frame->a2, frame->a3);

	printf("\tt0: %08x\tt1: %08x\tt2: %08x\tt3: %08x\n",
	    frame->t0, frame->t1, frame->t2, frame->t3);

	printf("\tt4: %08x\tt5: %08x\tt6: %08x\tt7: %08x\n",
	    frame->t4, frame->t5, frame->t6, frame->t7);

	printf("\tt8: %08x\tt9: %08x\ts0: %08x\ts1: %08x\n",
	    frame->t8, frame->t9, frame->s0, frame->s1);

	printf("\ts2: %08x\ts3: %08x\ts4: %08x\ts5: %08x\n",
	    frame->s2, frame->s3, frame->s4, frame->s5);

	printf("\ts6: %08x\ts7: %08x\tk0: %08x\tk1: %08x\n",
	    frame->s6, frame->s7, frame->k0, frame->k1);

	printf("\tgp: %08x\tsp: %08x\ts8: %08x\tra: %08x\n",
	    frame->gp, frame->sp, frame->s8, frame->ra);

	printf("\tsr: %08x\tmullo: %08x\tmulhi: %08x\tbadvaddr: %08x\n",
	    frame->sr, frame->mullo, frame->mulhi, frame->badvaddr);

#ifdef IC_REG
	printf("\tcause: %08x\tpc: %08x\tic: %08x\n",
	    frame->cause, frame->pc, frame->ic);
#else
	printf("\tcause: %08x\tpc: %08x\n",
	    frame->cause, frame->pc);
#endif
}

#endif


static void
get_mapping_info(vm_offset_t va, pd_entry_t **pdepp, pt_entry_t **ptepp)
{
	pt_entry_t *ptep;
	pd_entry_t *pdep;
	struct proc *p = curproc;

	pdep = (&(p->p_vmspace->vm_pmap.pm_segtab[va >> SEGSHIFT]));
	if (*pdep)
		ptep = pmap_pte(&p->p_vmspace->vm_pmap, va);
	else
		ptep = (pt_entry_t *)0;

	*pdepp = pdep;
	*ptepp = ptep;
}


static void
log_bad_page_fault(char *msg, struct trapframe *frame, int trap_type)
{
	pt_entry_t *ptep;
	pd_entry_t *pdep;
	unsigned int *addr;
	struct proc *p = curproc;
	char *read_or_write;
	register_t pc;

	trap_type &= ~T_USER;

#ifdef SMP
	printf("cpuid = %d\n", PCPU_GET(cpuid));
#endif
	switch (trap_type) {
	case T_TLB_ST_MISS:
	case T_ADDR_ERR_ST:
		read_or_write = "write";
		break;
	case T_TLB_LD_MISS:
	case T_ADDR_ERR_LD:
	case T_BUS_ERR_IFETCH:
		read_or_write = "read";
		break;
	default:
		read_or_write = "";
	}

	pc = frame->pc + (DELAYBRANCH(frame->cause) ? 4 : 0);
	log(LOG_ERR, "%s: pid %d (%s), uid %d: pc 0x%x got a %s fault at 0x%x\n",
	    msg, p->p_pid, p->p_comm,
	    p->p_ucred ? p->p_ucred->cr_uid : -1,
	    pc,
	    read_or_write,
	    frame->badvaddr);

	/* log registers in trap frame */
	log_frame_dump(frame);

	get_mapping_info((vm_offset_t)pc, &pdep, &ptep);

	/*
	 * Dump a few words around faulting instruction, if the addres is
	 * valid.
	 */
	if (!(pc & 3) && (pc != frame->badvaddr) &&
	    (trap_type != T_BUS_ERR_IFETCH) &&
	    useracc((caddr_t)pc, sizeof(int) * 4, VM_PROT_READ)) {
		/* dump page table entry for faulting instruction */
		log(LOG_ERR, "Page table info for pc address 0x%x: pde = %p, pte = 0x%lx\n",
		    pc, *pdep, ptep ? *ptep : 0);

		addr = (unsigned int *)pc;
		log(LOG_ERR, "Dumping 4 words starting at pc address %p: \n",
		    addr);
		log(LOG_ERR, "%08x %08x %08x %08x\n",
		    addr[0], addr[1], addr[2], addr[3]);
	} else {
		log(LOG_ERR, "pc address 0x%x is inaccessible, pde = 0x%p, pte = 0x%lx\n",
		    pc, *pdep, ptep ? *ptep : 0);
	}
	/*	panic("Bad trap");*/
}


/*
 * Unaligned load/store emulation
 */
static int
mips_unaligned_load_store(struct trapframe *frame, register_t addr, register_t pc)
{
	register_t *reg = (register_t *) frame;
	u_int32_t inst = *((u_int32_t *) pc);
	u_int32_t value_msb, value;
	int access_type = 0;

	switch (MIPS_INST_OPCODE(inst)) {
	case OP_LHU:
		lbu_macro(value_msb, addr);
		addr += 1;
		lbu_macro(value, addr);
		value |= value_msb << 8;
		reg[MIPS_INST_RT(inst)] = value;
		access_type = MIPS_LHU_ACCESS;
		break;

	case OP_LH:
		lb_macro(value_msb, addr);
		addr += 1;
		lbu_macro(value, addr);
		value |= value_msb << 8;
		reg[MIPS_INST_RT(inst)] = value;
		access_type = MIPS_LH_ACCESS;
		break;

	case OP_LWU:
		lwl_macro(value, addr);
		addr += 3;
		lwr_macro(value, addr);
		value &= 0xffffffff;
		reg[MIPS_INST_RT(inst)] = value;
		access_type = MIPS_LWU_ACCESS;
		break;

	case OP_LW:
		lwl_macro(value, addr);
		addr += 3;
		lwr_macro(value, addr);
		reg[MIPS_INST_RT(inst)] = value;
		access_type = MIPS_LW_ACCESS;
		break;

	case OP_SH:
		value = reg[MIPS_INST_RT(inst)];
		value_msb = value >> 8;
		sb_macro(value_msb, addr);
		addr += 1;
		sb_macro(value, addr);
		access_type = MIPS_SH_ACCESS;
		break;

	case OP_SW:
		value = reg[MIPS_INST_RT(inst)];
		swl_macro(value, addr);
		addr += 3;
		swr_macro(value, addr);
		access_type = MIPS_SW_ACCESS;
		break;

	default:
		break;
	}

	return access_type;
}


static int
emulate_unaligned_access(struct trapframe *frame)
{
	register_t pc;
	int access_type = 0;

	pc = frame->pc + (DELAYBRANCH(frame->cause) ? 4 : 0);

	/*
	 * Fall through if it's instruction fetch exception
	 */
	if (!((pc & 3) || (pc == frame->badvaddr))) {

		/*
		 * Handle unaligned load and store
		 */

		/*
		 * Return access type if the instruction was emulated.
		 * Otherwise restore pc and fall through.
		 */
		access_type = mips_unaligned_load_store(frame,
		    frame->badvaddr, pc);

		if (access_type) {
			if (DELAYBRANCH(frame->cause))
				frame->pc = MipsEmulateBranch(frame, frame->pc,
				    0, 0);
			else
				frame->pc += 4;

			log(LOG_INFO, "Unaligned %s: pc=0x%x, badvaddr=0x%x\n",
			    access_name[access_type - 1], pc, frame->badvaddr);
		}
	}
	return access_type;
}
