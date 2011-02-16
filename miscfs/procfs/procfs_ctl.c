<<<<<<< HEAD
/*	$OpenBSD: procfs_ctl.c,v 1.19 2006/11/29 12:24:18 miod Exp $	*/
=======
/*	$OpenBSD: procfs_ctl.c,v 1.22 2010/07/26 01:56:27 guenther Exp $	*/
>>>>>>> origin/master
/*	$NetBSD: procfs_ctl.c,v 1.14 1996/02/09 22:40:48 christos Exp $	*/

/*
 * Copyright (c) 1993 Jan-Simon Pendry
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)procfs_ctl.c	8.4 (Berkeley) 6/15/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>
#include <sys/ptrace.h>
#include <sys/sched.h>
#include <miscfs/procfs/procfs.h>

/*
 * True iff process (p) is in trace wait state
 * relative to process (curp)
 */
#define TRACE_WAIT_P(curp, p) \
	((p)->p_stat == SSTOP && \
	 (p)->p_p->ps_pptr == (curp)->p_p && \
	 ISSET((p)->p_flag, P_TRACED))

#ifdef PTRACE

#define PROCFS_CTL_ATTACH	1
#define PROCFS_CTL_DETACH	2
#define PROCFS_CTL_STEP		3
#define PROCFS_CTL_RUN		4
#define PROCFS_CTL_WAIT		5

static const vfs_namemap_t ctlnames[] = {
	/* special /proc commands */
	{ "attach",	PROCFS_CTL_ATTACH },
	{ "detach",	PROCFS_CTL_DETACH },
	{ "step",	PROCFS_CTL_STEP },
	{ "run",	PROCFS_CTL_RUN },
	{ "wait",	PROCFS_CTL_WAIT },
	{ 0 },
};

#endif

static const vfs_namemap_t signames[] = {
	/* regular signal names */
	{ "hup",	SIGHUP },	{ "int",	SIGINT },
	{ "quit",	SIGQUIT },	{ "ill",	SIGILL },
	{ "trap",	SIGTRAP },	{ "abrt",	SIGABRT },
	{ "iot",	SIGIOT },	{ "emt",	SIGEMT },
	{ "fpe",	SIGFPE },	{ "kill",	SIGKILL },
	{ "bus",	SIGBUS },	{ "segv",	SIGSEGV },
	{ "sys",	SIGSYS },	{ "pipe",	SIGPIPE },
	{ "alrm",	SIGALRM },	{ "term",	SIGTERM },
	{ "urg",	SIGURG },	{ "stop",	SIGSTOP },
	{ "tstp",	SIGTSTP },	{ "cont",	SIGCONT },
	{ "chld",	SIGCHLD },	{ "ttin",	SIGTTIN },
	{ "ttou",	SIGTTOU },	{ "io",		SIGIO },
	{ "xcpu",	SIGXCPU },	{ "xfsz",	SIGXFSZ },
	{ "vtalrm",	SIGVTALRM },	{ "prof",	SIGPROF },
	{ "winch",	SIGWINCH },	{ "info",	SIGINFO },
	{ "usr1",	SIGUSR1 },	{ "usr2",	SIGUSR2 },
	{ 0 },
};

#ifdef PTRACE
static int procfs_control(struct proc *, struct proc *, int);

static int
procfs_control(curp, p, op)
	struct proc *curp;		/* tracer */
	struct proc *p;			/* traced */
	int op;
{
	int error;
	int s;

	/*
	 * Attach - attaches the target process for debugging
	 * by the calling process.
	 */
	if (op == PROCFS_CTL_ATTACH) {
		/* Can't trace yourself! */
		if (p->p_pid == curp->p_pid)
			return (EINVAL);

		/* Check whether already being traced. */
		if (ISSET(p->p_flag, P_TRACED))
			return (EBUSY);

		if ((error = process_checkioperm(curp, p)) != 0)
			return (error);

		/*
		 * Go ahead and set the trace flag.
		 * Save the old parent (it's reset in
		 *   _DETACH, and also in kern_exit.c:wait4()
		 * Reparent the process so that the tracing
		 *   proc gets to see all the action.
		 * Stop the target.
		 */
		atomic_setbits_int(&p->p_flag, P_TRACED);
		p->p_xstat = 0;		/* XXX ? */
		if (p->p_p->ps_pptr != curp->p_p) {
			p->p_oppid = p->p_p->ps_pptr->ps_pid;
			proc_reparent(p->p_p, curp->p_p);
		}
		psignal(p, SIGSTOP);
		return (0);
	}

	/*
	 * Target process must be stopped, owned by (curp) and
	 * be set up for tracing (P_TRACED flag set).
	 * Allow DETACH to take place at any time for sanity.
	 * Allow WAIT any time, of course.
	 */
	switch (op) {
	case PROCFS_CTL_DETACH:
	case PROCFS_CTL_WAIT:
		break;

	default:
		if (!TRACE_WAIT_P(curp, p))
			return (EBUSY);
	}

	/*
	 * do single-step fixup if needed
	 */
	FIX_SSTEP(p);

	/*
	 * Don't deliver any signal by default.
	 * To continue with a signal, just send
	 * the signal name to the ctl file
	 */
	p->p_xstat = 0;

	switch (op) {
	/*
	 * Detach.  Cleans up the target process, reparent it if possible
	 * and set it running once more.
	 */
	case PROCFS_CTL_DETACH:
		/* if not being traced, then this is a painless no-op */
		if (!ISSET(p->p_flag, P_TRACED))
			return (0);

		/* not being traced any more */
		atomic_clearbits_int(&p->p_flag, P_TRACED);

		/* give process back to original parent */
		if (p->p_oppid != p->p_p->ps_pptr->ps_pid) {
			struct process *ppr;

			ppr = prfind(p->p_oppid);
			if (ppr)
				proc_reparent(p->p_p, ppr);
		}

		p->p_oppid = 0;
		atomic_clearbits_int(&p->p_flag, P_WAITED);
		wakeup(curp);	/* XXX for CTL_WAIT below ? */

		break;

	/*
	 * Step.  Let the target process execute a single instruction.
	 */
	case PROCFS_CTL_STEP:
#ifdef PT_STEP
		error = process_sstep(p, 1);
		if (error)
			return (error);
		break;
#else
		return (EOPNOTSUPP);
#endif

	/*
	 * Run.  Let the target process continue running until a breakpoint
	 * or some other trap.
	 */
	case PROCFS_CTL_RUN:
		break;

	/*
	 * Wait for the target process to stop.
	 * If the target is not being traced then just wait
	 * to enter
	 */
	case PROCFS_CTL_WAIT:
		error = 0;
		if (ISSET(p->p_flag, P_TRACED)) {
			while (error == 0 &&
					(p->p_stat != SSTOP) &&
					ISSET(p->p_flag, P_TRACED) &&
					(p->p_p->ps_pptr == curp->p_p)) {
				error = tsleep(p, PWAIT|PCATCH, "procfsx", 0);
			}
			if (error == 0 && !TRACE_WAIT_P(curp, p))
				error = EBUSY;
		} else {
			while (error == 0 && p->p_stat != SSTOP) {
				error = tsleep(p, PWAIT|PCATCH, "procfs", 0);
			}
		}
		return (error);

#ifdef DIAGNOSTIC
	default:
		panic("procfs_control");
#endif
	}

	SCHED_LOCK(s);
	if (p->p_stat == SSTOP)
		setrunnable(p);
	SCHED_UNLOCK(s);
	return (0);
}
#endif

int
procfs_doctl(curp, p, pfs, uio)
	struct proc *curp;
	struct pfsnode *pfs;
	struct uio *uio;
	struct proc *p;
{
	int xlen;
	int error;
	char msg[PROCFS_CTLLEN+1];
	const vfs_namemap_t *nm;
	int s;

	if (uio->uio_rw != UIO_WRITE)
		return (EOPNOTSUPP);

	xlen = PROCFS_CTLLEN;
	error = vfs_getuserstr(uio, msg, &xlen);
	if (error)
		return (error);

	/*
	 * Map signal names into signal generation
	 * or debug control.  Unknown commands and/or signals
	 * return EOPNOTSUPP.
	 *
	 * Sending a signal while the process is being debugged
	 * also has the side effect of letting the target continue
	 * to run.  There is no way to single-step a signal delivery.
	 */
	error = EOPNOTSUPP;

#ifdef PTRACE
	nm = vfs_findname(ctlnames, msg, xlen);
	if (nm) {
		error = procfs_control(curp, p, nm->nm_val);
	} else
#endif
	{
		nm = vfs_findname(signames, msg, xlen);
		if (nm) {
			if (TRACE_WAIT_P(curp, p)) {
				p->p_xstat = nm->nm_val;
				FIX_SSTEP(p);
				SCHED_LOCK(s);
				setrunnable(p);
				SCHED_UNLOCK(s);
			} else {
				psignal(p, nm->nm_val);
			}
			error = 0;
		}
	}

	return (error);
}
