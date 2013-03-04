/*	$NetBSD: evtchn.c,v 1.14 2005/04/28 18:26:26 yamt Exp $	*/

/*
 *
 * Copyright (c) 2004 Christian Limpach.
 * Copyright (c) 2004, K A Fraser.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "isa.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/reboot.h>

#include <uvm/uvm.h>

#include <machine/intrdefs.h>

#include <machine/xen.h>
#include <machine/hypervisor.h>
#include <machine/evtchn.h>
#include <machine/xenfunc.h>
#include <machine/lock.h>

#if 0 /* XXX ??? */
/*
 * This lock protects updates to the following mapping and reference-count
 * arrays. The lock does not need to be acquired to read the mapping tables.
 */
static struct simplelock irq_mapping_update_lock = SIMPLELOCK_INITIALIZER;
#endif

/* event handlers */
struct evtsource *evtsource[NR_EVENT_CHANNELS];

/* Reference counts for bindings to event channels */
static u_int8_t evtch_bindcount[NR_EVENT_CHANNELS];

/* event-channel <-> VIRQ mapping. */
static int virq_to_evtch[NR_VIRQS];


#if NPCI > 0 || NISA > 0
/* event-channel <-> PIRQ mapping */
static int pirq_to_evtch[NR_PIRQS];
/* PIRQ needing notify */
static u_int32_t pirq_needs_unmask_notify[NR_EVENT_CHANNELS / 32];
int pirq_interrupt(void *);
physdev_op_t physdev_op_notify = {
	.cmd = PHYSDEVOP_IRQ_UNMASK_NOTIFY,
};
#endif

int debug_port;
int xen_debug_handler(void *);
int xen_misdirect_handler(void *);

/* #define IRQ_DEBUG 5 */

void
events_default_setup(void)
{
	int i;

	/* No VIRQ -> event mappings. */
	for (i = 0; i < NR_VIRQS; i++)
		virq_to_evtch[i] = -1;

#if NPCI > 0 || NISA > 0
	/* No PIRQ -> event mappings. */
	for (i = 0; i < NR_PIRQS; i++)
		pirq_to_evtch[i] = -1;
	for (i = 0; i < NR_EVENT_CHANNELS / 32; i++)
		pirq_needs_unmask_notify[i] = 0;
#endif

	/* No event-channel are 'live' right now. */
	for (i = 0; i < NR_EVENT_CHANNELS; i++) {
		evtsource[i] = NULL;
		evtch_bindcount[i] = 0;
		hypervisor_mask_event(i);
	}

}

void
init_events(void)
{
	int evtch;

	evtch = bind_virq_to_evtch(VIRQ_DEBUG);
	printf("debug virtual interrupt using event channel %d\n", evtch);
	event_set_handler(evtch, &xen_debug_handler, NULL, IPL_DEBUG,
	    "debugev");
	hypervisor_enable_event(evtch);

	enable_intr();		/* at long last... */
}

unsigned int
evtchn_do_event(int evtch, struct intrframe *regs)
{
	struct cpu_info *ci;
	int ilevel;
	struct intrhand *ih;
	int	(*ih_fun)(void *, void *);
	extern struct uvmexp uvmexp;
	u_int32_t iplmask;

#ifdef DIAGNOSTIC
	if (evtch >= NR_EVENT_CHANNELS) {
		printf("event number %d > NR_IRQS\n", evtch);
		panic("evtchn_do_event");
	}
#endif

#ifdef IRQ_DEBUG
	if (evtch == IRQ_DEBUG)
		printf("evtchn_do_event: evtch %d\n", evtch);
#endif
	ci = &cpu_info_primary;

	/*
	 * Shortcut for the debug handler, we want it to always run,
	 * regardless of the IPL level.
	 */

	if (evtch == debug_port) {
		xen_debug_handler(NULL);
		hypervisor_enable_event(evtch);
		return 0;
	}

#ifdef DIAGNOSTIC
	if (evtsource[evtch] == NULL) {
		panic("evtchn_do_event: unknown event");
	}
#endif
	uvmexp.intrs++;
	evtsource[evtch]->ev_evcnt.ec_count++;
	ilevel = cpl;
	if (evtsource[evtch]->ev_maxlevel <= ilevel) {
#ifdef IRQ_DEBUG
		if (evtch == IRQ_DEBUG)
			printf("evtsource[%d]->ev_maxlevel %d <= ilevel %d\n",
			    evtch, evtsource[evtch]->ev_maxlevel, ilevel);
#endif
		hypervisor_set_ipending(evtsource[evtch]->ev_imask,
		    evtch / 32, evtch % 32);
		/* leave masked */
		return 0;
	}
	cpl = evtsource[evtch]->ev_maxlevel;
	iplmask = evtsource[evtch]->ev_imask;
	sti();
	ci->ci_idepth++;
#ifdef MULTIPROCESSOR
	x86_intlock(regs);
#endif
	ih = evtsource[evtch]->ev_handlers;
	while (ih != NULL) {
		if (ih->ih_level <= ilevel) {
#ifdef IRQ_DEBUG
		if (evtch == IRQ_DEBUG)
		    printf("ih->ih_level %d <= ilevel %d\n", ih->ih_level, ilevel);
#endif
#ifdef MULTIPROCESSOR
			x86_intunlock(regs);
#endif
			cli();
			hypervisor_set_ipending(iplmask, evtch / 32,
			    evtch % 32);
			/* leave masked */
			ci->ci_idepth--;
			splx(ilevel);
			return 0;
		}
		iplmask &= ~IUNMASK(ih->ih_level);
		cpl = ih->ih_level;
		ih_fun = (void *)ih->ih_fun;
		ih_fun(ih->ih_arg, regs);
#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
		ih->ih_count.ec_count++;
#endif
		ih = ih->ih_evt_next;
	}
#ifdef MULTIPROCESSOR
	x86_intunlock(regs);
#endif
	cli();
	hypervisor_enable_event(evtch);
	ci->ci_idepth--;
	splx(ilevel);

	return 0;
}

int
bind_virq_to_evtch(int virq)
{
	evtchn_op_t op;
	int evtchn, s;

	s = splhigh();
	simple_lock(&irq_mapping_update_lock);

	evtchn = virq_to_evtch[virq];
	if (evtchn == -1) {
		op.cmd = EVTCHNOP_bind_virq;
		op.u.bind_virq.virq = virq;
		op.u.bind_virq.vcpu = 0;
		if (HYPERVISOR_event_channel_op(&op) != 0)
			panic("Failed to bind virtual IRQ %d\n", virq);
		evtchn = op.u.bind_virq.port;
		if (virq == VIRQ_DEBUG)
			debug_port = evtchn;

		virq_to_evtch[virq] = evtchn;
	}

	evtch_bindcount[evtchn]++;

	simple_unlock(&irq_mapping_update_lock);
	splx(s);

	return evtchn;
}

void
unbind_virq_from_evtch(int virq)
{
	evtchn_op_t op;
	int evtchn = virq_to_evtch[virq];
	int s = splhigh();

	simple_lock(&irq_mapping_update_lock);

	evtch_bindcount[evtchn]--;
	if (evtch_bindcount[evtchn] == 0) {
		op.cmd = EVTCHNOP_close;
		op.u.close.port = evtchn;
		if (HYPERVISOR_event_channel_op(&op) != 0)
			panic("Failed to unbind virtual IRQ %d\n", virq);

		virq_to_evtch[virq] = -1;
	}

	simple_unlock(&irq_mapping_update_lock);
	splx(s);
}

#if NPCI > 0 || NISA > 0
int
bind_pirq_to_evtch(int pirq)
{
	evtchn_op_t op;
	int evtchn, s;

	if (pirq >= NR_PIRQS) {
		panic("pirq %d out of bound, increase NR_PIRQS", pirq);
	}

	s = splhigh();
	simple_lock(&irq_mapping_update_lock);

	evtchn = pirq_to_evtch[pirq];
	if (evtchn == -1) {
		op.cmd = EVTCHNOP_bind_pirq;
		op.u.bind_pirq.pirq = pirq;
		op.u.bind_pirq.flags = BIND_PIRQ__WILL_SHARE;
		if (HYPERVISOR_event_channel_op(&op) != 0)
			panic("Failed to bind physical IRQ %d\n", pirq);
		evtchn = op.u.bind_pirq.port;

#ifdef IRQ_DEBUG
		printf("pirq %d evtchn %d\n", pirq, evtchn);
#endif
		pirq_to_evtch[pirq] = evtchn;
	}

	evtch_bindcount[evtchn]++;

	simple_unlock(&irq_mapping_update_lock);
	splx(s);

	return evtchn;
}

void
unbind_pirq_from_evtch(int pirq)
{
	evtchn_op_t op;
	int evtchn = pirq_to_evtch[pirq];
	int s = splhigh();

	simple_lock(&irq_mapping_update_lock);

	evtch_bindcount[evtchn]--;
	if (evtch_bindcount[evtchn] == 0) {
		op.cmd = EVTCHNOP_close;
		op.u.close.port = evtchn;
		if (HYPERVISOR_event_channel_op(&op) != 0)
			panic("Failed to unbind physical IRQ %d\n", pirq);

		pirq_to_evtch[pirq] = -1;
	}

	simple_unlock(&irq_mapping_update_lock);
	splx(s);
}

struct pintrhand *
pirq_establish(int pirq, int evtch, int (*func)(void *), void *arg, int level)
{
	struct pintrhand *ih;
	physdev_op_t physdev_op;
	char evname[8];

	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL) {
		printf("pirq_establish: can't malloc handler info\n");
		return NULL;
	}
	snprintf(evname, sizeof(evname), "irq%d", pirq);
	if (event_set_handler(evtch, pirq_interrupt, ih, level, evname) != 0) {
		free(ih, M_DEVBUF);
		return NULL;
	}
	ih->pirq = pirq;
	ih->evtch = evtch;
	ih->func = func;
	ih->arg = arg;

	physdev_op.cmd = PHYSDEVOP_IRQ_STATUS_QUERY;
	physdev_op.u.irq_status_query.irq = pirq;
	if (HYPERVISOR_physdev_op(&physdev_op) < 0)
		panic("HYPERVISOR_physdev_op(PHYSDEVOP_IRQ_STATUS_QUERY)");
	if (physdev_op.u.irq_status_query.flags &
	    PHYSDEVOP_IRQ_NEEDS_UNMASK_NOTIFY) {
		pirq_needs_unmask_notify[evtch >> 5] |= (1 << (evtch & 0x1f));
#ifdef IRQ_DEBUG
		printf("pirq %d needs notify\n", pirq);
#endif
	}
	hypervisor_enable_event(evtch);
	return ih;
}

int
pirq_interrupt(void *arg)
{
	struct pintrhand *ih = arg;
	int ret;

#ifdef IRQ_DEBUG
	if (ih->evtch == IRQ_DEBUG)
	    printf("pirq_interrupt irq %d\n", ih->pirq);
#endif
	ret = ih->func(ih->arg);
#ifdef IRQ_DEBUG
	if (ih->evtch == IRQ_DEBUG)
	    printf("pirq_interrupt irq %d ret %d\n", ih->pirq, ret);
#endif
	return ret;
}

#endif /* NPCI > 0 || NISA > 0 */

int
event_set_handler(int evtch, int (*func)(void *), void *arg, int level,
    const char *evname)
{
	struct iplsource *ipls;
	struct evtsource *evts;
	struct intrhand *ih, **ihp;
	struct cpu_info *ci;
	int s;


#ifdef DIAGNOSTIC
	if (evtch >= NR_EVENT_CHANNELS) {
		printf("evtch number %d > NR_EVENT_CHANNELS\n", evtch);
		panic("event_set_handler");
	}
#endif

#if IRQ_DEBUG
	printf("event_set_handler evtch %d handler %p level %d\n", evtch,
	       func, level);
#endif
	MALLOC(ih, struct intrhand *, sizeof (struct intrhand), M_DEVBUF,
	    M_WAITOK);
	if (ih == NULL)
		panic("can't allocate fixed interrupt source");
	bzero(ih, sizeof(struct intrhand));

	ih->ih_level = level;
	ih->ih_fun = func;
	ih->ih_arg = arg;
#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
	ih->ih_irq = evtch;
#endif
	ih->ih_evt_next = NULL;
	ih->ih_ipl_next = NULL;

	ci = &cpu_info_primary;
	s = splhigh();
	if (ci->ci_isources[level] == NULL) {
		MALLOC(ipls, struct iplsource *, sizeof (struct iplsource),
		    M_DEVBUF, M_WAITOK);
		if (ipls == NULL)
			panic("can't allocate fixed interrupt source");
		bzero(ipls, sizeof(struct iplsource));
		ipls->ipl_recurse = xenev_stubs[level].ist_recurse;
		ipls->ipl_resume = xenev_stubs[level].ist_resume;
		ipls->ipl_handlers = ih;
		ci->ci_isources[level] = ipls;
	} else {
		ipls = ci->ci_isources[level];
		ih->ih_ipl_next = ipls->ipl_handlers;
		ipls->ipl_handlers = ih;
	}
	if (evtsource[evtch] == NULL) {
		MALLOC(evts, struct evtsource *, sizeof (struct evtsource),
		    M_DEVBUF, M_WAITOK);
		if (evts == NULL)
			panic("can't allocate fixed interrupt source");
		bzero(evts, sizeof(struct evtsource));
		evts->ev_handlers = ih;
		evtsource[evtch] = evts;
		if (evname)
			strlcpy(evts->ev_evname, evname,
			    sizeof(evts->ev_evname));
		else
			snprintf(evts->ev_evname, sizeof(evts->ev_evname),
			    "evt%d", evtch);

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
		evcount_attach(&evts->ev_evcnt, evts->ev_evname,
			(void *)&evts->ev_evcnt.ec_count, &evcount_intr);
#else
		evcount_attach(&ih->ih_count, evts->ev_evname,
			(void *)&ih->ih_irq, &evcount_intr);
#endif
	} else {
		evts = evtsource[evtch];
		/* sort by IPL order, higher first */
		for (ihp = &evts->ev_handlers; ; ihp = &((*ihp)->ih_evt_next)) {
			if ((*ihp)->ih_level < ih->ih_level) {
				/* insert before *ihp */
				ih->ih_evt_next = *ihp;
				*ihp = ih;
				break;
			}
			if ((*ihp)->ih_evt_next == NULL) {
				(*ihp)->ih_evt_next = ih;
				break;
			}
		}
	}

	intr_calculatemasks(evts);
	splx(s);

	return 0;
}

int
event_remove_handler(int evtch, int (*func)(void *), void *arg)
{
	struct iplsource *ipls;
	struct evtsource *evts;
	struct intrhand *ih;
	struct intrhand **ihp;
	struct cpu_info *ci = &cpu_info_primary;

	evts = evtsource[evtch];
	if (evts == NULL)
		return ENOENT;

	for (ihp = &evts->ev_handlers, ih = evts->ev_handlers;
	    ih != NULL;
	    ihp = &ih->ih_evt_next, ih = ih->ih_evt_next) {
		if (ih->ih_fun == func && ih->ih_arg == arg)
			break;
	}
	if (ih == NULL)
		return ENOENT;
	*ihp = ih->ih_evt_next;

	ipls = ci->ci_isources[ih->ih_level];
	for (ihp = &ipls->ipl_handlers, ih = ipls->ipl_handlers;
	    ih != NULL;
	    ihp = &ih->ih_ipl_next, ih = ih->ih_ipl_next) {
		if (ih->ih_fun == func && ih->ih_arg == arg)
			break;
	}
	if (ih == NULL)
		panic("event_remove_handler");
	*ihp = ih->ih_ipl_next;
	FREE(ih, M_DEVBUF);
	if (evts->ev_handlers == NULL) {
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
		evcount_detach(&evts->ev_evcnt);
#else
		evcount_detach(&ih->ih_count);
#endif
		FREE(evts, M_DEVBUF);
		evtsource[evtch] = NULL;
	} else {
		intr_calculatemasks(evts);
	}
	return 0;
}

void
hypervisor_enable_event(unsigned int evtch)
{
#ifdef IRQ_DEBUG
	if (evtch == IRQ_DEBUG)
		printf("hypervisor_enable_event: evtch %i\n", evtch);
#endif

	hypervisor_unmask_event(evtch);
#if NPCI > 0 || NISA > 0 
	if (pirq_needs_unmask_notify[evtch >> 5] & (1 << (evtch & 0x1f))) {
#ifdef  IRQ_DEBUG
		if (evtch == IRQ_DEBUG)
		    printf("pirq_notify(%d)\n", evtch);
#endif
		(void)HYPERVISOR_physdev_op(&physdev_op_notify);
	}
#endif /* NPCI > 0 || NISA > 0 */
}

int
xen_debug_handler(void *arg)
{
	struct cpu_info *ci = curcpu();
	int i;
	printf("debug event\n");
	printf("cpl 0x%x ipending 0x%x ci_idepth %d\n",
	    cpl, ipending, ci->ci_idepth);
	printf("evtchn_upcall_pending %d evtchn_upcall_mask %d"
	    " evtchn_pending_sel 0x%x\n",
		HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_pending,
		HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_mask,
		HYPERVISOR_shared_info->evtchn_pending_sel);
	printf("evtchn_mask");
	for (i = 0 ; i < 32; i++)
		printf(" %x", HYPERVISOR_shared_info->evtchn_mask[i]);
	printf("\n");
	printf("evtchn_pending");
	for (i = 0 ; i < 32; i++)
		printf(" %x", HYPERVISOR_shared_info->evtchn_pending[i]);
	printf("\n");
	return 0;
}