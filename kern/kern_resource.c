/*-
 * Copyright (c) 1982, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)kern_resource.c	8.5 (Berkeley) 1/21/94
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/kern/kern_resource.c,v 1.193 2008/10/24 01:09:24 davidxu Exp $");

#include "opt_compat.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysproto.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/priv.h>
#include <sys/proc.h>
#include <sys/refcount.h>
#include <sys/resourcevar.h>
#include <sys/rwlock.h>
#include <sys/sched.h>
#include <sys/sx.h>
#include <sys/syscallsubr.h>
#include <sys/sysent.h>
#include <sys/time.h>
#include <sys/umtx.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>


static MALLOC_DEFINE(M_PLIMIT, "plimit", "plimit structures");
static MALLOC_DEFINE(M_UIDINFO, "uidinfo", "uidinfo structures");
#define	UIHASH(uid)	(&uihashtbl[(uid) & uihash])
static struct rwlock uihashtbl_lock;
static LIST_HEAD(uihashhead, uidinfo) *uihashtbl;
static u_long uihash;		/* size of hash table - 1 */

static void	calcru1(struct proc *p, struct rusage_ext *ruxp,
		    struct timeval *up, struct timeval *sp);
static int	donice(struct thread *td, struct proc *chgp, int n);
static struct uidinfo *uilookup(uid_t uid);

/*
 * Resource controls and accounting.
 */
#ifndef _SYS_SYSPROTO_H_
struct getpriority_args {
	int	which;
	int	who;
};
#endif
int
getpriority(td, uap)
	struct thread *td;
	register struct getpriority_args *uap;
{
	struct proc *p;
	struct pgrp *pg;
	int error, low;

	error = 0;
	low = PRIO_MAX + 1;
	switch (uap->which) {

	case PRIO_PROCESS:
		if (uap->who == 0)
			low = td->td_proc->p_nice;
		else {
			p = pfind(uap->who);
			if (p == NULL)
				break;
			if (p_cansee(td, p) == 0)
				low = p->p_nice;
			PROC_UNLOCK(p);
		}
		break;

	case PRIO_PGRP:
		sx_slock(&proctree_lock);
		if (uap->who == 0) {
			pg = td->td_proc->p_pgrp;
			PGRP_LOCK(pg);
		} else {
			pg = pgfind(uap->who);
			if (pg == NULL) {
				sx_sunlock(&proctree_lock);
				break;
			}
		}
		sx_sunlock(&proctree_lock);
		LIST_FOREACH(p, &pg->pg_members, p_pglist) {
			PROC_LOCK(p);
			if (p_cansee(td, p) == 0) {
				if (p->p_nice < low)
					low = p->p_nice;
			}
			PROC_UNLOCK(p);
		}
		PGRP_UNLOCK(pg);
		break;

	case PRIO_USER:
		if (uap->who == 0)
			uap->who = td->td_ucred->cr_uid;
		sx_slock(&allproc_lock);
		FOREACH_PROC_IN_SYSTEM(p) {
			/* Do not bother to check PRS_NEW processes */
			if (p->p_state == PRS_NEW)
				continue;
			PROC_LOCK(p);
			if (p_cansee(td, p) == 0 &&
			    p->p_ucred->cr_uid == uap->who) {
				if (p->p_nice < low)
					low = p->p_nice;
			}
			PROC_UNLOCK(p);
		}
		sx_sunlock(&allproc_lock);
		break;

	default:
		error = EINVAL;
		break;
	}
	if (low == PRIO_MAX + 1 && error == 0)
		error = ESRCH;
	td->td_retval[0] = low;
	return (error);
}

#ifndef _SYS_SYSPROTO_H_
struct setpriority_args {
	int	which;
	int	who;
	int	prio;
};
#endif
int
setpriority(td, uap)
	struct thread *td;
	struct setpriority_args *uap;
{
	struct proc *curp, *p;
	struct pgrp *pg;
	int found = 0, error = 0;

	curp = td->td_proc;
	switch (uap->which) {
	case PRIO_PROCESS:
		if (uap->who == 0) {
			PROC_LOCK(curp);
			error = donice(td, curp, uap->prio);
			PROC_UNLOCK(curp);
		} else {
			p = pfind(uap->who);
			if (p == NULL)
				break;
			error = p_cansee(td, p);
			if (error == 0)
				error = donice(td, p, uap->prio);
			PROC_UNLOCK(p);
		}
		found++;
		break;

	case PRIO_PGRP:
		sx_slock(&proctree_lock);
		if (uap->who == 0) {
			pg = curp->p_pgrp;
			PGRP_LOCK(pg);
		} else {
			pg = pgfind(uap->who);
			if (pg == NULL) {
				sx_sunlock(&proctree_lock);
				break;
			}
		}
		sx_sunlock(&proctree_lock);
		LIST_FOREACH(p, &pg->pg_members, p_pglist) {
			PROC_LOCK(p);
			if (p_cansee(td, p) == 0) {
				error = donice(td, p, uap->prio);
				found++;
			}
			PROC_UNLOCK(p);
		}
		PGRP_UNLOCK(pg);
		break;

	case PRIO_USER:
		if (uap->who == 0)
			uap->who = td->td_ucred->cr_uid;
		sx_slock(&allproc_lock);
		FOREACH_PROC_IN_SYSTEM(p) {
			PROC_LOCK(p);
			if (p->p_ucred->cr_uid == uap->who &&
			    p_cansee(td, p) == 0) {
				error = donice(td, p, uap->prio);
				found++;
			}
			PROC_UNLOCK(p);
		}
		sx_sunlock(&allproc_lock);
		break;

	default:
		error = EINVAL;
		break;
	}
	if (found == 0 && error == 0)
		error = ESRCH;
	return (error);
}

/*
 * Set "nice" for a (whole) process.
 */
static int
donice(struct thread *td, struct proc *p, int n)
{
	int error;

	PROC_LOCK_ASSERT(p, MA_OWNED);
	if ((error = p_cansched(td, p)))
		return (error);
	if (n > PRIO_MAX)
		n = PRIO_MAX;
	if (n < PRIO_MIN)
		n = PRIO_MIN;
	if (n < p->p_nice && priv_check(td, PRIV_SCHED_SETPRIORITY) != 0)
		return (EACCES);
	sched_nice(p, n);
	return (0);
}

/*
 * Set realtime priority for LWP.
 */
#ifndef _SYS_SYSPROTO_H_
struct rtprio_thread_args {
	int		function;
	lwpid_t		lwpid;
	struct rtprio	*rtp;
};
#endif
int
rtprio_thread(struct thread *td, struct rtprio_thread_args *uap)
{
	struct proc *p;
	struct rtprio rtp;
	struct thread *td1;
	int cierror, error;

	/* Perform copyin before acquiring locks if needed. */
	if (uap->function == RTP_SET)
		cierror = copyin(uap->rtp, &rtp, sizeof(struct rtprio));
	else
		cierror = 0;

	/*
	 * Though lwpid is unique, only current process is supported
	 * since there is no efficient way to look up a LWP yet.
	 */
	p = td->td_proc;
	PROC_LOCK(p);

	switch (uap->function) {
	case RTP_LOOKUP:
		if ((error = p_cansee(td, p)))
			break;
		if (uap->lwpid == 0 || uap->lwpid == td->td_tid)
			td1 = td;
		else
			td1 = thread_find(p, uap->lwpid);
		if (td1 != NULL)
			pri_to_rtp(td1, &rtp);
		else
			error = ESRCH;
		PROC_UNLOCK(p);
		return (copyout(&rtp, uap->rtp, sizeof(struct rtprio)));
	case RTP_SET:
		if ((error = p_cansched(td, p)) || (error = cierror))
			break;

		/* Disallow setting rtprio in most cases if not superuser. */
/*
 * Realtime priority has to be restricted for reasons which should be
 * obvious.  However, for idle priority, there is a potential for
 * system deadlock if an idleprio process gains a lock on a resource
 * that other processes need (and the idleprio process can't run
 * due to a CPU-bound normal process).  Fix me!  XXX
 */
#if 0
		if (RTP_PRIO_IS_REALTIME(rtp.type)) {
#else
		if (rtp.type != RTP_PRIO_NORMAL) {
#endif
			error = priv_check(td, PRIV_SCHED_RTPRIO);
			if (error)
				break;
		}

		if (uap->lwpid == 0 || uap->lwpid == td->td_tid)
			td1 = td;
		else
			td1 = thread_find(p, uap->lwpid);
		if (td1 != NULL)
			error = rtp_to_pri(&rtp, td1);
		else
			error = ESRCH;
		break;
	default:
		error = EINVAL;
		break;
	}
	PROC_UNLOCK(p);
	return (error);
}

/*
 * Set realtime priority.
 */
#ifndef _SYS_SYSPROTO_H_
struct rtprio_args {
	int		function;
	pid_t		pid;
	struct rtprio	*rtp;
};
#endif
int
rtprio(td, uap)
	struct thread *td;		/* curthread */
	register struct rtprio_args *uap;
{
	struct proc *p;
	struct thread *tdp;
	struct rtprio rtp;
	int cierror, error;

	/* Perform copyin before acquiring locks if needed. */
	if (uap->function == RTP_SET)
		cierror = copyin(uap->rtp, &rtp, sizeof(struct rtprio));
	else
		cierror = 0;

	if (uap->pid == 0) {
		p = td->td_proc;
		PROC_LOCK(p);
	} else {
		p = pfind(uap->pid);
		if (p == NULL)
			return (ESRCH);
	}

	switch (uap->function) {
	case RTP_LOOKUP:
		if ((error = p_cansee(td, p)))
			break;
		/*
		 * Return OUR priority if no pid specified,
		 * or if one is, report the highest priority
		 * in the process.  There isn't much more you can do as
		 * there is only room to return a single priority.
		 * Note: specifying our own pid is not the same
		 * as leaving it zero.
		 */
		if (uap->pid == 0) {
			pri_to_rtp(td, &rtp);
		} else {
			struct rtprio rtp2;

			rtp.type = RTP_PRIO_IDLE;
			rtp.prio = RTP_PRIO_MAX;
			FOREACH_THREAD_IN_PROC(p, tdp) {
				pri_to_rtp(tdp, &rtp2);
				if (rtp2.type <  rtp.type ||
				    (rtp2.type == rtp.type &&
				    rtp2.prio < rtp.prio)) {
					rtp.type = rtp2.type;
					rtp.prio = rtp2.prio;
				}
			}
		}
		PROC_UNLOCK(p);
		return (copyout(&rtp, uap->rtp, sizeof(struct rtprio)));
	case RTP_SET:
		if ((error = p_cansched(td, p)) || (error = cierror))
			break;

		/* Disallow setting rtprio in most cases if not superuser. */
/*
 * Realtime priority has to be restricted for reasons which should be
 * obvious.  However, for idle priority, there is a potential for
 * system deadlock if an idleprio process gains a lock on a resource
 * that other processes need (and the idleprio process can't run
 * due to a CPU-bound normal process).  Fix me!  XXX
 */
#if 0
		if (RTP_PRIO_IS_REALTIME(rtp.type)) {
#else
		if (rtp.type != RTP_PRIO_NORMAL) {
#endif
			error = priv_check(td, PRIV_SCHED_RTPRIO);
			if (error)
				break;
		}

		/*
		 * If we are setting our own priority, set just our
		 * thread but if we are doing another process,
		 * do all the threads on that process. If we
		 * specify our own pid we do the latter.
		 */
		if (uap->pid == 0) {
			error = rtp_to_pri(&rtp, td);
		} else {
			FOREACH_THREAD_IN_PROC(p, td) {
				if ((error = rtp_to_pri(&rtp, td)) != 0)
					break;
			}
		}
		break;
	default:
		error = EINVAL;
		break;
	}
	PROC_UNLOCK(p);
	return (error);
}

int
rtp_to_pri(struct rtprio *rtp, struct thread *td)
{
	u_char	newpri;
	u_char	oldpri;

	if (rtp->prio > RTP_PRIO_MAX)
		return (EINVAL);
	thread_lock(td);
	switch (RTP_PRIO_BASE(rtp->type)) {
	case RTP_PRIO_REALTIME:
		newpri = PRI_MIN_REALTIME + rtp->prio;
		break;
	case RTP_PRIO_NORMAL:
		newpri = PRI_MIN_TIMESHARE + rtp->prio;
		break;
	case RTP_PRIO_IDLE:
		newpri = PRI_MIN_IDLE + rtp->prio;
		break;
	default:
		thread_unlock(td);
		return (EINVAL);
	}
	sched_class(td, rtp->type);	/* XXX fix */
	oldpri = td->td_user_pri;
	sched_user_prio(td, newpri);
	if (curthread == td)
		sched_prio(curthread, td->td_user_pri); /* XXX dubious */
	if (TD_ON_UPILOCK(td) && oldpri != newpri) {
		thread_unlock(td);
		umtx_pi_adjust(td, oldpri);
	} else
		thread_unlock(td);
	return (0);
}

void
pri_to_rtp(struct thread *td, struct rtprio *rtp)
{

	thread_lock(td);
	switch (PRI_BASE(td->td_pri_class)) {
	case PRI_REALTIME:
		rtp->prio = td->td_base_user_pri - PRI_MIN_REALTIME;
		break;
	case PRI_TIMESHARE:
		rtp->prio = td->td_base_user_pri - PRI_MIN_TIMESHARE;
		break;
	case PRI_IDLE:
		rtp->prio = td->td_base_user_pri - PRI_MIN_IDLE;
		break;
	default:
		break;
	}
	rtp->type = td->td_pri_class;
	thread_unlock(td);
}

#if defined(COMPAT_43)
#ifndef _SYS_SYSPROTO_H_
struct osetrlimit_args {
	u_int	which;
	struct	orlimit *rlp;
};
#endif
int
osetrlimit(td, uap)
	struct thread *td;
	register struct osetrlimit_args *uap;
{
	struct orlimit olim;
	struct rlimit lim;
	int error;

	if ((error = copyin(uap->rlp, &olim, sizeof(struct orlimit))))
		return (error);
	lim.rlim_cur = olim.rlim_cur;
	lim.rlim_max = olim.rlim_max;
	error = kern_setrlimit(td, uap->which, &lim);
	return (error);
}

#ifndef _SYS_SYSPROTO_H_
struct ogetrlimit_args {
	u_int	which;
	struct	orlimit *rlp;
};
#endif
int
ogetrlimit(td, uap)
	struct thread *td;
	register struct ogetrlimit_args *uap;
{
	struct orlimit olim;
	struct rlimit rl;
	struct proc *p;
	int error;

	if (uap->which >= RLIM_NLIMITS)
		return (EINVAL);
	p = td->td_proc;
	PROC_LOCK(p);
	lim_rlimit(p, uap->which, &rl);
	PROC_UNLOCK(p);

	/*
	 * XXX would be more correct to convert only RLIM_INFINITY to the
	 * old RLIM_INFINITY and fail with EOVERFLOW for other larger
	 * values.  Most 64->32 and 32->16 conversions, including not
	 * unimportant ones of uids are even more broken than what we
	 * do here (they blindly truncate).  We don't do this correctly
	 * here since we have little experience with EOVERFLOW yet.
	 * Elsewhere, getuid() can't fail...
	 */
	olim.rlim_cur = rl.rlim_cur > 0x7fffffff ? 0x7fffffff : rl.rlim_cur;
	olim.rlim_max = rl.rlim_max > 0x7fffffff ? 0x7fffffff : rl.rlim_max;
	error = copyout(&olim, uap->rlp, sizeof(olim));
	return (error);
}
#endif /* COMPAT_43 */

#ifndef _SYS_SYSPROTO_H_
struct __setrlimit_args {
	u_int	which;
	struct	rlimit *rlp;
};
#endif
int
setrlimit(td, uap)
	struct thread *td;
	register struct __setrlimit_args *uap;
{
	struct rlimit alim;
	int error;

	if ((error = copyin(uap->rlp, &alim, sizeof(struct rlimit))))
		return (error);
	error = kern_setrlimit(td, uap->which, &alim);
	return (error);
}

static void
lim_cb(void *arg)
{
	struct rlimit rlim;
	struct thread *td;
	struct proc *p;

	p = arg;
	PROC_LOCK_ASSERT(p, MA_OWNED);
	/*
	 * Check if the process exceeds its cpu resource allocation.  If
	 * it reaches the max, arrange to kill the process in ast().
	 */
	if (p->p_cpulimit == RLIM_INFINITY)
		return;
	PROC_SLOCK(p);
	FOREACH_THREAD_IN_PROC(p, td) {
		thread_lock(td);
		ruxagg(&p->p_rux, td);
		thread_unlock(td);
	}
	PROC_SUNLOCK(p);
	if (p->p_rux.rux_runtime > p->p_cpulimit * cpu_tickrate()) {
		lim_rlimit(p, RLIMIT_CPU, &rlim);
		if (p->p_rux.rux_runtime >= rlim.rlim_max * cpu_tickrate()) {
			killproc(p, "exceeded maximum CPU limit");
		} else {
			if (p->p_cpulimit < rlim.rlim_max)
				p->p_cpulimit += 5;
			psignal(p, SIGXCPU);
		}
	}
	if ((p->p_flag & P_WEXIT) == 0)
		callout_reset(&p->p_limco, hz, lim_cb, p);
}

int
kern_setrlimit(td, which, limp)
	struct thread *td;
	u_int which;
	struct rlimit *limp;
{
	struct plimit *newlim, *oldlim;
	struct proc *p;
	register struct rlimit *alimp;
	struct rlimit oldssiz;
	int error;

	if (which >= RLIM_NLIMITS)
		return (EINVAL);

	/*
	 * Preserve historical bugs by treating negative limits as unsigned.
	 */
	if (limp->rlim_cur < 0)
		limp->rlim_cur = RLIM_INFINITY;
	if (limp->rlim_max < 0)
		limp->rlim_max = RLIM_INFINITY;

	oldssiz.rlim_cur = 0;
	p = td->td_proc;
	newlim = lim_alloc();
	PROC_LOCK(p);
	oldlim = p->p_limit;
	alimp = &oldlim->pl_rlimit[which];
	if (limp->rlim_cur > alimp->rlim_max ||
	    limp->rlim_max > alimp->rlim_max)
		if ((error = priv_check(td, PRIV_PROC_SETRLIMIT))) {
			PROC_UNLOCK(p);
			lim_free(newlim);
			return (error);
		}
	if (limp->rlim_cur > limp->rlim_max)
		limp->rlim_cur = limp->rlim_max;
	lim_copy(newlim, oldlim);
	alimp = &newlim->pl_rlimit[which];

	switch (which) {

	case RLIMIT_CPU:
		if (limp->rlim_cur != RLIM_INFINITY &&
		    p->p_cpulimit == RLIM_INFINITY)
			callout_reset(&p->p_limco, hz, lim_cb, p);
		p->p_cpulimit = limp->rlim_cur;
		break;
	case RLIMIT_DATA:
		if (limp->rlim_cur > maxdsiz)
			limp->rlim_cur = maxdsiz;
		if (limp->rlim_max > maxdsiz)
			limp->rlim_max = maxdsiz;
		break;

	case RLIMIT_STACK:
		if (limp->rlim_cur > maxssiz)
			limp->rlim_cur = maxssiz;
		if (limp->rlim_max > maxssiz)
			limp->rlim_max = maxssiz;
		oldssiz = *alimp;
		if (td->td_proc->p_sysent->sv_fixlimit != NULL)
			td->td_proc->p_sysent->sv_fixlimit(&oldssiz,
			    RLIMIT_STACK);
		break;

	case RLIMIT_NOFILE:
		if (limp->rlim_cur > maxfilesperproc)
			limp->rlim_cur = maxfilesperproc;
		if (limp->rlim_max > maxfilesperproc)
			limp->rlim_max = maxfilesperproc;
		break;

	case RLIMIT_NPROC:
		if (limp->rlim_cur > maxprocperuid)
			limp->rlim_cur = maxprocperuid;
		if (limp->rlim_max > maxprocperuid)
			limp->rlim_max = maxprocperuid;
		if (limp->rlim_cur < 1)
			limp->rlim_cur = 1;
		if (limp->rlim_max < 1)
			limp->rlim_max = 1;
		break;
	}
	if (td->td_proc->p_sysent->sv_fixlimit != NULL)
		td->td_proc->p_sysent->sv_fixlimit(limp, which);
	*alimp = *limp;
	p->p_limit = newlim;
	PROC_UNLOCK(p);
	lim_free(oldlim);

	if (which == RLIMIT_STACK) {
		/*
		 * Stack is allocated to the max at exec time with only
		 * "rlim_cur" bytes accessible.  If stack limit is going
		 * up make more accessible, if going down make inaccessible.
		 */
		if (limp->rlim_cur != oldssiz.rlim_cur) {
			vm_offset_t addr;
			vm_size_t size;
			vm_prot_t prot;

			if (limp->rlim_cur > oldssiz.rlim_cur) {
				prot = p->p_sysent->sv_stackprot;
				size = limp->rlim_cur - oldssiz.rlim_cur;
				addr = p->p_sysent->sv_usrstack -
				    limp->rlim_cur;
			} else {
				prot = VM_PROT_NONE;
				size = oldssiz.rlim_cur - limp->rlim_cur;
				addr = p->p_sysent->sv_usrstack -
				    oldssiz.rlim_cur;
			}
			addr = trunc_page(addr);
			size = round_page(size);
			(void)vm_map_protect(&p->p_vmspace->vm_map,
			    addr, addr + size, prot, FALSE);
		}
	}

	return (0);
}

#ifndef _SYS_SYSPROTO_H_
struct __getrlimit_args {
	u_int	which;
	struct	rlimit *rlp;
};
#endif
/* ARGSUSED */
int
getrlimit(td, uap)
	struct thread *td;
	register struct __getrlimit_args *uap;
{
	struct rlimit rlim;
	struct proc *p;
	int error;

	if (uap->which >= RLIM_NLIMITS)
		return (EINVAL);
	p = td->td_proc;
	PROC_LOCK(p);
	lim_rlimit(p, uap->which, &rlim);
	PROC_UNLOCK(p);
	error = copyout(&rlim, uap->rlp, sizeof(struct rlimit));
	return (error);
}

/*
 * Transform the running time and tick information for children of proc p
 * into user and system time usage.
 */
void
calccru(p, up, sp)
	struct proc *p;
	struct timeval *up;
	struct timeval *sp;
{

	PROC_LOCK_ASSERT(p, MA_OWNED);
	calcru1(p, &p->p_crux, up, sp);
}

/*
 * Transform the running time and tick information in proc p into user
 * and system time usage.  If appropriate, include the current time slice
 * on this CPU.
 */
void
calcru(struct proc *p, struct timeval *up, struct timeval *sp)
{
	struct thread *td;
	uint64_t u;

	PROC_LOCK_ASSERT(p, MA_OWNED);
	PROC_SLOCK_ASSERT(p, MA_OWNED);
	/*
	 * If we are getting stats for the current process, then add in the
	 * stats that this thread has accumulated in its current time slice.
	 * We reset the thread and CPU state as if we had performed a context
	 * switch right here.
	 */
	td = curthread;
	if (td->td_proc == p) {
		u = cpu_ticks();
		p->p_rux.rux_runtime += u - PCPU_GET(switchtime);
		PCPU_SET(switchtime, u);
	}
	/* Make sure the per-thread stats are current. */
	FOREACH_THREAD_IN_PROC(p, td) {
		if (td->td_incruntime == 0)
			continue;
		thread_lock(td);
		ruxagg(&p->p_rux, td);
		thread_unlock(td);
	}
	calcru1(p, &p->p_rux, up, sp);
}

static void
calcru1(struct proc *p, struct rusage_ext *ruxp, struct timeval *up,
    struct timeval *sp)
{
	/* {user, system, interrupt, total} {ticks, usec}: */
	u_int64_t ut, uu, st, su, it, tt, tu;

	ut = ruxp->rux_uticks;
	st = ruxp->rux_sticks;
	it = ruxp->rux_iticks;
	tt = ut + st + it;
	if (tt == 0) {
		/* Avoid divide by zero */
		st = 1;
		tt = 1;
	}
	tu = cputick2usec(ruxp->rux_runtime);
	if ((int64_t)tu < 0) {
		/* XXX: this should be an assert /phk */
		printf("calcru: negative runtime of %jd usec for pid %d (%s)\n",
		    (intmax_t)tu, p->p_pid, p->p_comm);
		tu = ruxp->rux_tu;
	}

	if (tu >= ruxp->rux_tu) {
		/*
		 * The normal case, time increased.
		 * Enforce monotonicity of bucketed numbers.
		 */
		uu = (tu * ut) / tt;
		if (uu < ruxp->rux_uu)
			uu = ruxp->rux_uu;
		su = (tu * st) / tt;
		if (su < ruxp->rux_su)
			su = ruxp->rux_su;
	} else if (tu + 3 > ruxp->rux_tu || 101 * tu > 100 * ruxp->rux_tu) {
		/*
		 * When we calibrate the cputicker, it is not uncommon to
		 * see the presumably fixed frequency increase slightly over
		 * time as a result of thermal stabilization and NTP
		 * discipline (of the reference clock).  We therefore ignore
		 * a bit of backwards slop because we  expect to catch up
		 * shortly.  We use a 3 microsecond limit to catch low
		 * counts and a 1% limit for high counts.
		 */
		uu = ruxp->rux_uu;
		su = ruxp->rux_su;
		tu = ruxp->rux_tu;
	} else { /* tu < ruxp->rux_tu */
		/*
		 * What happened here was likely that a laptop, which ran at
		 * a reduced clock frequency at boot, kicked into high gear.
		 * The wisdom of spamming this message in that case is
		 * dubious, but it might also be indicative of something
		 * serious, so lets keep it and hope laptops can be made
		 * more truthful about their CPU speed via ACPI.
		 */
		printf("calcru: runtime went backwards from %ju usec "
		    "to %ju usec for pid %d (%s)\n",
		    (uintmax_t)ruxp->rux_tu, (uintmax_t)tu,
		    p->p_pid, p->p_comm);
		uu = (tu * ut) / tt;
		su = (tu * st) / tt;
	}

	ruxp->rux_uu = uu;
	ruxp->rux_su = su;
	ruxp->rux_tu = tu;

	up->tv_sec = uu / 1000000;
	up->tv_usec = uu % 1000000;
	sp->tv_sec = su / 1000000;
	sp->tv_usec = su % 1000000;
}

#ifndef _SYS_SYSPROTO_H_
struct getrusage_args {
	int	who;
	struct	rusage *rusage;
};
#endif
int
getrusage(td, uap)
	register struct thread *td;
	register struct getrusage_args *uap;
{
	struct rusage ru;
	int error;

	error = kern_getrusage(td, uap->who, &ru);
	if (error == 0)
		error = copyout(&ru, uap->rusage, sizeof(struct rusage));
	return (error);
}

int
kern_getrusage(td, who, rup)
	struct thread *td;
	int who;
	struct rusage *rup;
{
	struct proc *p;
	int error;

	error = 0;
	p = td->td_proc;
	PROC_LOCK(p);
	switch (who) {
	case RUSAGE_SELF:
		rufetchcalc(p, rup, &rup->ru_utime,
		    &rup->ru_stime);
		break;

	case RUSAGE_CHILDREN:
		*rup = p->p_stats->p_cru;
		calccru(p, &rup->ru_utime, &rup->ru_stime);
		break;

	default:
		error = EINVAL;
	}
	PROC_UNLOCK(p);
	return (error);
}

void
rucollect(struct rusage *ru, struct rusage *ru2)
{
	long *ip, *ip2;
	int i;

	if (ru->ru_maxrss < ru2->ru_maxrss)
		ru->ru_maxrss = ru2->ru_maxrss;
	ip = &ru->ru_first;
	ip2 = &ru2->ru_first;
	for (i = &ru->ru_last - &ru->ru_first; i >= 0; i--)
		*ip++ += *ip2++;
}

void
ruadd(struct rusage *ru, struct rusage_ext *rux, struct rusage *ru2,
    struct rusage_ext *rux2)
{

	rux->rux_runtime += rux2->rux_runtime;
	rux->rux_uticks += rux2->rux_uticks;
	rux->rux_sticks += rux2->rux_sticks;
	rux->rux_iticks += rux2->rux_iticks;
	rux->rux_uu += rux2->rux_uu;
	rux->rux_su += rux2->rux_su;
	rux->rux_tu += rux2->rux_tu;
	rucollect(ru, ru2);
}

/*
 * Aggregate tick counts into the proc's rusage_ext.
 */
void
ruxagg(struct rusage_ext *rux, struct thread *td)
{

	THREAD_LOCK_ASSERT(td, MA_OWNED);
	PROC_SLOCK_ASSERT(td->td_proc, MA_OWNED);
	rux->rux_runtime += td->td_incruntime;
	rux->rux_uticks += td->td_uticks;
	rux->rux_sticks += td->td_sticks;
	rux->rux_iticks += td->td_iticks;
	td->td_incruntime = 0;
	td->td_uticks = 0;
	td->td_iticks = 0;
	td->td_sticks = 0;
}

/*
 * Update the rusage_ext structure and fetch a valid aggregate rusage
 * for proc p if storage for one is supplied.
 */
void
rufetch(struct proc *p, struct rusage *ru)
{
	struct thread *td;

	PROC_SLOCK_ASSERT(p, MA_OWNED);

	*ru = p->p_ru;
	if (p->p_numthreads > 0)  {
		FOREACH_THREAD_IN_PROC(p, td) {
			thread_lock(td);
			ruxagg(&p->p_rux, td);
			thread_unlock(td);
			rucollect(ru, &td->td_ru);
		}
	}
}

/*
 * Atomically perform a rufetch and a calcru together.
 * Consumers, can safely assume the calcru is executed only once
 * rufetch is completed.
 */
void
rufetchcalc(struct proc *p, struct rusage *ru, struct timeval *up,
    struct timeval *sp)
{

	PROC_SLOCK(p);
	rufetch(p, ru);
	calcru(p, up, sp);
	PROC_SUNLOCK(p);
}

/*
 * Allocate a new resource limits structure and initialize its
 * reference count and mutex pointer.
 */
struct plimit *
lim_alloc()
{
	struct plimit *limp;

	limp = malloc(sizeof(struct plimit), M_PLIMIT, M_WAITOK);
	refcount_init(&limp->pl_refcnt, 1);
	return (limp);
}

struct plimit *
lim_hold(limp)
	struct plimit *limp;
{

	refcount_acquire(&limp->pl_refcnt);
	return (limp);
}

void
lim_fork(struct proc *p1, struct proc *p2)
{
	p2->p_limit = lim_hold(p1->p_limit);
	callout_init_mtx(&p2->p_limco, &p2->p_mtx, 0);
	if (p1->p_cpulimit != RLIM_INFINITY)
		callout_reset(&p2->p_limco, hz, lim_cb, p2);
}

void
lim_free(limp)
	struct plimit *limp;
{

	KASSERT(limp->pl_refcnt > 0, ("plimit refcnt underflow"));
	if (refcount_release(&limp->pl_refcnt))
		free((void *)limp, M_PLIMIT);
}

/*
 * Make a copy of the plimit structure.
 * We share these structures copy-on-write after fork.
 */
void
lim_copy(dst, src)
	struct plimit *dst, *src;
{

	KASSERT(dst->pl_refcnt == 1, ("lim_copy to shared limit"));
	bcopy(src->pl_rlimit, dst->pl_rlimit, sizeof(src->pl_rlimit));
}

/*
 * Return the hard limit for a particular system resource.  The
 * which parameter specifies the index into the rlimit array.
 */
rlim_t
lim_max(struct proc *p, int which)
{
	struct rlimit rl;

	lim_rlimit(p, which, &rl);
	return (rl.rlim_max);
}

/*
 * Return the current (soft) limit for a particular system resource.
 * The which parameter which specifies the index into the rlimit array
 */
rlim_t
lim_cur(struct proc *p, int which)
{
	struct rlimit rl;

	lim_rlimit(p, which, &rl);
	return (rl.rlim_cur);
}

/*
 * Return a copy of the entire rlimit structure for the system limit
 * specified by 'which' in the rlimit structure pointed to by 'rlp'.
 */
void
lim_rlimit(struct proc *p, int which, struct rlimit *rlp)
{

	PROC_LOCK_ASSERT(p, MA_OWNED);
	KASSERT(which >= 0 && which < RLIM_NLIMITS,
	    ("request for invalid resource limit"));
	*rlp = p->p_limit->pl_rlimit[which];
	if (p->p_sysent->sv_fixlimit != NULL)
		p->p_sysent->sv_fixlimit(rlp, which);
}

/*
 * Find the uidinfo structure for a uid.  This structure is used to
 * track the total resource consumption (process count, socket buffer
 * size, etc.) for the uid and impose limits.
 */
void
uihashinit()
{

	uihashtbl = hashinit(maxproc / 16, M_UIDINFO, &uihash);
	rw_init(&uihashtbl_lock, "uidinfo hash");
}

/*
 * Look up a uidinfo struct for the parameter uid.
 * uihashtbl_lock must be locked.
 */
static struct uidinfo *
uilookup(uid)
	uid_t uid;
{
	struct uihashhead *uipp;
	struct uidinfo *uip;

	rw_assert(&uihashtbl_lock, RA_LOCKED);
	uipp = UIHASH(uid);
	LIST_FOREACH(uip, uipp, ui_hash)
		if (uip->ui_uid == uid)
			break;

	return (uip);
}

/*
 * Find or allocate a struct uidinfo for a particular uid.
 * Increase refcount on uidinfo struct returned.
 * uifree() should be called on a struct uidinfo when released.
 */
struct uidinfo *
uifind(uid)
	uid_t uid;
{
	struct uidinfo *old_uip, *uip;

	rw_rlock(&uihashtbl_lock);
	uip = uilookup(uid);
	if (uip == NULL) {
		rw_runlock(&uihashtbl_lock);
		uip = malloc(sizeof(*uip), M_UIDINFO, M_WAITOK | M_ZERO);
		rw_wlock(&uihashtbl_lock);
		/*
		 * There's a chance someone created our uidinfo while we
		 * were in malloc and not holding the lock, so we have to
		 * make sure we don't insert a duplicate uidinfo.
		 */
		if ((old_uip = uilookup(uid)) != NULL) {
			/* Someone else beat us to it. */
			free(uip, M_UIDINFO);
			uip = old_uip;
		} else {
			refcount_init(&uip->ui_ref, 0);
			uip->ui_uid = uid;
			LIST_INSERT_HEAD(UIHASH(uid), uip, ui_hash);
		}
	}
	uihold(uip);
	rw_unlock(&uihashtbl_lock);
	return (uip);
}

/*
 * Place another refcount on a uidinfo struct.
 */
void
uihold(uip)
	struct uidinfo *uip;
{

	refcount_acquire(&uip->ui_ref);
}

/*-
 * Since uidinfo structs have a long lifetime, we use an
 * opportunistic refcounting scheme to avoid locking the lookup hash
 * for each release.
 *
 * If the refcount hits 0, we need to free the structure,
 * which means we need to lock the hash.
 * Optimal case:
 *   After locking the struct and lowering the refcount, if we find
 *   that we don't need to free, simply unlock and return.
 * Suboptimal case:
 *   If refcount lowering results in need to free, bump the count
 *   back up, lose the lock and acquire the locks in the proper
 *   order to try again.
 */
void
uifree(uip)
	struct uidinfo *uip;
{
	int old;

	/* Prepare for optimal case. */
	old = uip->ui_ref;
	if (old > 1 && atomic_cmpset_int(&uip->ui_ref, old, old - 1))
		return;

	/* Prepare for suboptimal case. */
	rw_wlock(&uihashtbl_lock);
	if (refcount_release(&uip->ui_ref)) {
		LIST_REMOVE(uip, ui_hash);
		rw_wunlock(&uihashtbl_lock);
		if (uip->ui_sbsize != 0)
			printf("freeing uidinfo: uid = %d, sbsize = %ld\n",
			    uip->ui_uid, uip->ui_sbsize);
		if (uip->ui_proccnt != 0)
			printf("freeing uidinfo: uid = %d, proccnt = %ld\n",
			    uip->ui_uid, uip->ui_proccnt);
		free(uip, M_UIDINFO);
		return;
	}
	/*
	 * Someone added a reference between atomic_cmpset_int() and
	 * rw_wlock(&uihashtbl_lock).
	 */
	rw_wunlock(&uihashtbl_lock);
}

/*
 * Change the count associated with number of processes
 * a given user is using.  When 'max' is 0, don't enforce a limit
 */
int
chgproccnt(uip, diff, max)
	struct	uidinfo	*uip;
	int	diff;
	rlim_t	max;
{

	/* Don't allow them to exceed max, but allow subtraction. */
	if (diff > 0 && max != 0) {
		if (atomic_fetchadd_long(&uip->ui_proccnt, (long)diff) + diff > max) {
			atomic_subtract_long(&uip->ui_proccnt, (long)diff);
			return (0);
		}
	} else {
		atomic_add_long(&uip->ui_proccnt, (long)diff);
		if (uip->ui_proccnt < 0)
			printf("negative proccnt for uid = %d\n", uip->ui_uid);
	}
	return (1);
}

/*
 * Change the total socket buffer size a user has used.
 */
int
chgsbsize(uip, hiwat, to, max)
	struct	uidinfo	*uip;
	u_int  *hiwat;
	u_int	to;
	rlim_t	max;
{
	int diff;

	diff = to - *hiwat;
	if (diff > 0) {
		if (atomic_fetchadd_long(&uip->ui_sbsize, (long)diff) + diff > max) {
			atomic_subtract_long(&uip->ui_sbsize, (long)diff);
			return (0);
		}
	} else {
		atomic_add_long(&uip->ui_sbsize, (long)diff);
		if (uip->ui_sbsize < 0)
			printf("negative sbsize for uid = %d\n", uip->ui_uid);
	}
	*hiwat = to;
	return (1);
}

/*
 * Change the count associated with number of pseudo-terminals
 * a given user is using.  When 'max' is 0, don't enforce a limit
 */
int
chgptscnt(uip, diff, max)
	struct	uidinfo	*uip;
	int	diff;
	rlim_t	max;
{

	/* Don't allow them to exceed max, but allow subtraction. */
	if (diff > 0 && max != 0) {
		if (atomic_fetchadd_long(&uip->ui_ptscnt, (long)diff) + diff > max) {
			atomic_subtract_long(&uip->ui_ptscnt, (long)diff);
			return (0);
		}
	} else {
		atomic_add_long(&uip->ui_ptscnt, (long)diff);
		if (uip->ui_ptscnt < 0)
			printf("negative ptscnt for uid = %d\n", uip->ui_uid);
	}
	return (1);
}
