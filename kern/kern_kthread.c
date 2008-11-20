/*-
 * Copyright (c) 1999 Peter Wemm <peter@FreeBSD.org>
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
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/kern/kern_kthread.c,v 1.43 2007/10/27 00:54:16 julian Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kthread.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>
#include <sys/sx.h>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <sys/sched.h>
#include <vm/vm.h>
#include <vm/vm_extern.h>

#include <machine/stdarg.h>

/*
 * Start a kernel process.  This is called after a fork() call in
 * mi_startup() in the file kern/init_main.c.
 *
 * This function is used to start "internal" daemons and intended
 * to be called from SYSINIT().
 */
void
kproc_start(udata)
	const void *udata;
{
	const struct kproc_desc	*kp = udata;
	int error;

	error = kproc_create((void (*)(void *))kp->func, NULL,
		    kp->global_procpp, 0, 0, "%s", kp->arg0);
	if (error)
		panic("kproc_start: %s: error %d", kp->arg0, error);
}

/*
 * Create a kernel process/thread/whatever.  It shares its address space
 * with proc0 - ie: kernel only.
 *
 * func is the function to start.
 * arg is the parameter to pass to function on first startup.
 * newpp is the return value pointing to the thread's struct proc.
 * flags are flags to fork1 (in unistd.h)
 * fmt and following will be *printf'd into (*newpp)->p_comm (for ps, etc.).
 */
int
kproc_create(void (*func)(void *), void *arg,
    struct proc **newpp, int flags, int pages, const char *fmt, ...)
{
	int error;
	va_list ap;
	struct thread *td;
	struct proc *p2;

	if (!proc0.p_stats)
		panic("kproc_create called too soon");

	error = fork1(&thread0, RFMEM | RFFDG | RFPROC | RFSTOPPED | flags,
	    pages, &p2);
	if (error)
		return error;

	/* save a global descriptor, if desired */
	if (newpp != NULL)
		*newpp = p2;

	/* this is a non-swapped system process */
	PROC_LOCK(p2);
	td = FIRST_THREAD_IN_PROC(p2);
	p2->p_flag |= P_SYSTEM | P_KTHREAD;
	td->td_pflags |= TDP_KTHREAD;
	mtx_lock(&p2->p_sigacts->ps_mtx);
	p2->p_sigacts->ps_flag |= PS_NOCLDWAIT;
	mtx_unlock(&p2->p_sigacts->ps_mtx);
	PROC_UNLOCK(p2);

	/* set up arg0 for 'ps', et al */
	va_start(ap, fmt);
	vsnprintf(p2->p_comm, sizeof(p2->p_comm), fmt, ap);
	va_end(ap);
	/* set up arg0 for 'ps', et al */
	va_start(ap, fmt);
	vsnprintf(td->td_name, sizeof(td->td_name), fmt, ap);
	va_end(ap);

	/* call the processes' main()... */
	cpu_set_fork_handler(td, func, arg);
	TD_SET_CAN_RUN(td);

	/* Delay putting it on the run queue until now. */
	if (!(flags & RFSTOPPED)) {
		thread_lock(td);
		sched_add(td, SRQ_BORING); 
		thread_unlock(td);
	}

	return 0;
}

void
kproc_exit(int ecode)
{
	struct thread *td;
	struct proc *p;

	td = curthread;
	p = td->td_proc;

	/*
	 * Reparent curthread from proc0 to init so that the zombie
	 * is harvested.
	 */
	sx_xlock(&proctree_lock);
	PROC_LOCK(p);
	proc_reparent(p, initproc);
	PROC_UNLOCK(p);
	sx_xunlock(&proctree_lock);

	/*
	 * Wakeup anyone waiting for us to exit.
	 */
	wakeup(p);

	/* Buh-bye! */
	exit1(td, W_EXITCODE(ecode, 0));
}

/*
 * Advise a kernel process to suspend (or resume) in its main loop.
 * Participation is voluntary.
 */
int
kproc_suspend(struct proc *p, int timo)
{
	/*
	 * Make sure this is indeed a system process and we can safely
	 * use the p_siglist field.
	 */
	PROC_LOCK(p);
	if ((p->p_flag & P_KTHREAD) == 0) {
		PROC_UNLOCK(p);
		return (EINVAL);
	}
	SIGADDSET(p->p_siglist, SIGSTOP);
	wakeup(p);
	return msleep(&p->p_siglist, &p->p_mtx, PPAUSE | PDROP, "suspkp", timo);
}

int
kproc_resume(struct proc *p)
{
	/*
	 * Make sure this is indeed a system process and we can safely
	 * use the p_siglist field.
	 */
	PROC_LOCK(p);
	if ((p->p_flag & P_KTHREAD) == 0) {
		PROC_UNLOCK(p);
		return (EINVAL);
	}
	SIGDELSET(p->p_siglist, SIGSTOP);
	PROC_UNLOCK(p);
	wakeup(&p->p_siglist);
	return (0);
}

void
kproc_suspend_check(struct proc *p)
{
	PROC_LOCK(p);
	while (SIGISMEMBER(p->p_siglist, SIGSTOP)) {
		wakeup(&p->p_siglist);
		msleep(&p->p_siglist, &p->p_mtx, PPAUSE, "kpsusp", 0);
	}
	PROC_UNLOCK(p);
}


/*
 * Start a kernel thread.  
 *
 * This function is used to start "internal" daemons and intended
 * to be called from SYSINIT().
 */

void
kthread_start(udata)
	const void *udata;
{
	const struct kthread_desc	*kp = udata;
	int error;

	error = kthread_add((void (*)(void *))kp->func, NULL,
		    NULL, kp->global_threadpp, 0, 0, "%s", kp->arg0);
	if (error)
		panic("kthread_start: %s: error %d", kp->arg0, error);
}

/*
 * Create a kernel thread.  It shares its address space
 * with proc0 - ie: kernel only.
 *
 * func is the function to start.
 * arg is the parameter to pass to function on first startup.
 * newtdp is the return value pointing to the thread's struct thread.
 *  ** XXX fix this --> flags are flags to fork1 (in unistd.h) 
 * fmt and following will be *printf'd into (*newtd)->td_name (for ps, etc.).
 */
int
kthread_add(void (*func)(void *), void *arg, struct proc *p,
    struct thread **newtdp, int flags, int pages, const char *fmt, ...)
{
	va_list ap;
	struct thread *newtd, *oldtd;
	int error;

	if (!proc0.p_stats)
		panic("kthread_add called too soon");

	error = 0;
	/* If no process supplied, put it on proc0 */
	if (p == NULL) {
		p = &proc0;
		oldtd = &thread0;
	} else {
		oldtd = FIRST_THREAD_IN_PROC(p);
	}

	/* Initialize our new td  */
	newtd = thread_alloc();
	if (newtd == NULL)
		return (ENOMEM);

	bzero(&newtd->td_startzero,
	    __rangeof(struct thread, td_startzero, td_endzero));
/* XXX check if we should zero. */
	bcopy(&oldtd->td_startcopy, &newtd->td_startcopy,
	    __rangeof(struct thread, td_startcopy, td_endcopy));

	/* set up arg0 for 'ps', et al */
	va_start(ap, fmt);
	vsnprintf(newtd->td_name, sizeof(newtd->td_name), fmt, ap);
	va_end(ap);

	newtd->td_proc = p;  /* needed for cpu_set_upcall */

	/* XXX optimise this probably? */
	/* On x86 (and probably the others too) it is way too full of junk */
	/* Needs a better name */
	cpu_set_upcall(newtd, oldtd);
	/* put the designated function(arg) as the resume context */
	cpu_set_fork_handler(newtd, func, arg);

	newtd->td_pflags |= TDP_KTHREAD;
	newtd->td_ucred = crhold(p->p_ucred);
	/* Allocate and switch to an alternate kstack if specified. */
	if (pages != 0)
		vm_thread_new_altkstack(newtd, pages);

	/* this code almost the same as create_thread() in kern_thr.c */
	PROC_LOCK(p);
	p->p_flag |= P_HADTHREADS;
	newtd->td_sigmask = oldtd->td_sigmask; /* XXX dubious */
	PROC_SLOCK(p);
	thread_link(newtd, p);
	thread_lock(oldtd);
	/* let the scheduler know about these things. */
	sched_fork_thread(oldtd, newtd);
	TD_SET_CAN_RUN(newtd);
	thread_unlock(oldtd);
	PROC_SUNLOCK(p);
	PROC_UNLOCK(p);


	/* Delay putting it on the run queue until now. */
	if (!(flags & RFSTOPPED)) {
		thread_lock(newtd);
		sched_add(newtd, SRQ_BORING); 
		thread_unlock(newtd);
	}
	if (newtdp)
		*newtdp = newtd;
	return 0;
}

void
kthread_exit(void)
{
	/* a module may be waiting for us to exit */
	wakeup(curthread);
	/*
	 * We could rely on thread_exit to call exit1() but
	 * there is extra work that needs to be done
	 */
	if (curthread->td_proc->p_numthreads == 1)
		kproc_exit(0);	/* never returns */
	thread_exit();
}

/*
 * Advise a kernel process to suspend (or resume) in its main loop.
 * Participation is voluntary.
 */
int
kthread_suspend(struct thread *td, int timo)
{
	if ((td->td_pflags & TDP_KTHREAD) == 0) {
		return (EINVAL);
	}
	thread_lock(td);
	td->td_flags |= TDF_KTH_SUSP;
	thread_unlock(td);
	/*
	 * If it's stopped for some other reason,
	 * kick it to notice our request 
	 * or we'll end up timing out
	 */
	wakeup(td); /* traditional  place for kernel threads to sleep on */ /* XXX ?? */
	return (tsleep(&td->td_flags, PPAUSE | PDROP, "suspkt", timo));
}

/*
 * let the kthread it can keep going again.
 */
int
kthread_resume(struct thread *td)
{
	if ((td->td_pflags & TDP_KTHREAD) == 0) {
		return (EINVAL);
	}
	thread_lock(td);
	td->td_flags &= ~TDF_KTH_SUSP;
	thread_unlock(td);
	wakeup(&td->td_name);
	return (0);
}

/*
 * Used by the thread to poll as to whether it should yield/sleep
 * and notify the caller that is has happened.
 */
void
kthread_suspend_check(struct thread *td)
{
	while (td->td_flags & TDF_KTH_SUSP) {
		/*
		 * let the caller know we got the message then sleep
		 */
		wakeup(&td->td_flags);
		tsleep(&td->td_name, PPAUSE, "ktsusp", 0);
	}
}

int
kproc_kthread_add(void (*func)(void *), void *arg,
            struct proc **procptr, struct thread **tdptr,
            int flags, int pages, char * procname, const char *fmt, ...) 
{
	int error;
	va_list ap;
	char buf[100];
	struct thread *td;

	if (*procptr == 0) {
		error = kproc_create(func, arg,
		    	procptr, flags, pages, "%s", procname);
		if (error)
			return (error);
		td = FIRST_THREAD_IN_PROC(*procptr);
		*tdptr = td;
		va_start(ap, fmt);
		vsnprintf(td->td_name, sizeof(td->td_name), fmt, ap);
		va_end(ap);
		return (0); 
	}
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	error = kthread_add(func, arg, *procptr,
		    tdptr, flags, pages, "%s", buf);
	return (error);
}
