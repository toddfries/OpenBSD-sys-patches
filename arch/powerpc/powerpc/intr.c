/*	$OpenBSD: intr.c,v 1.5 2009/06/02 21:38:10 drahn Exp $	*/

/*
 * Copyright (c) 1997 Per Fogelstrom, Opsycon AB and RTMX Inc, USA.
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
 *	This product includes software developed under OpenBSD by
 *	Per Fogelstrom, Opsycon AB, Sweden for RTMX Inc, North Carolina USA.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <sys/param.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/lock.h>


ppc_splraise_t ppc_dflt_splraise;
ppc_spllower_t ppc_dflt_spllower;
ppc_splx_t ppc_dflt_splx;
ppc_setipl_t ppc_dflt_setipl;

/* provide a function for asm code to call */
#undef splraise
#undef spllower
#undef splx

int ppc_smask[IPL_NUM];

void
ppc_smask_init()
{
	int i;

        for (i = IPL_NONE; i <= IPL_HIGH; i++)  {
                ppc_smask[i] = 0;
#if 0
	/* NOT YET */
                if (i < IPL_SOFT)
                        ppc_smask[i] |= SI_TO_IRQBIT(SI_SOFT);
#endif
                if (i < IPL_SOFTCLOCK)
                        ppc_smask[i] |= SI_TO_IRQBIT(SI_SOFTCLOCK);
                if (i < IPL_SOFTNET)
                        ppc_smask[i] |= SI_TO_IRQBIT(SI_SOFTNET);
                if (i < IPL_SOFTTTY)
                        ppc_smask[i] |= SI_TO_IRQBIT(SI_SOFTTTY);
        }
}


int
splraise(int newcpl)
{
	return ppc_intr_func.raise(newcpl);
}

int
spllower(int newcpl)
{
	return ppc_intr_func.lower(newcpl);
}

void
splx(int newcpl)
{
	ppc_intr_func.x(newcpl);
}

/*
 * functions with 'default' behavior to use before the real
 * interrupt controller attaches
 */
int
ppc_dflt_splraise(int newcpl)
{
	struct cpu_info *ci = curcpu();
	int oldcpl;

	oldcpl = ci->ci_cpl;
	if (newcpl < oldcpl)
		newcpl = oldcpl;
	ci->ci_cpl = newcpl;

	return (oldcpl);
}

int
ppc_dflt_spllower(int newcpl)
{
	struct cpu_info *ci = curcpu();
	int oldcpl;

	oldcpl = ci->ci_cpl;

	splx(newcpl);

	return (oldcpl);
}

void
ppc_dflt_splx(int newcpl)
{
	ppc_do_pending_int(newcpl);
}

void
ppc_dflt_setipl(int newcpl)
{
	struct cpu_info *ci = curcpu();
	ci->ci_cpl = newcpl;
}

struct ppc_intr_func ppc_intr_func =
{
	ppc_dflt_splraise,
	ppc_dflt_spllower,
	ppc_dflt_splx,
	ppc_dflt_setipl
};

char *
ppc_intr_typename(int type)
{
	switch (type) {
        case IST_NONE :
		return ("none");
        case IST_PULSE:
		return ("pulsed");
        case IST_EDGE:
		return ("edge-triggered");
        case IST_LEVEL:
		return ("level-triggered");
	default:
		return ("unknown");
	}
}

void
ppc_do_pending_int(int pcpl)
{
	int s;
	s = ppc_intr_disable();
	ppc_do_pending_int_dis(pcpl, s);
	ppc_intr_enable(s);

}

/*
 * This function expect interrupts disabled on entry and exit,
 * the s argument indicates if interrupts may be enabled during
 * the processing of off level interrupts, s 'should' always be 1.
 *
 * This is called from clock and hardware interrupt service routines
 * which can cause recursion, however they will only recurse
 * once because interrupts are only enabled while CI_IACTIVE_PROCESSING_SOFT
 * is set. This prevents a second recursion from occurring.
 */
void
ppc_do_pending_int_dis(int pcpl, int s)
{
	struct cpu_info *ci = curcpu();
	int loopcount = 0;

	if (ci->ci_iactive & CI_IACTIVE_PROCESSING_SOFT) {
		/* soft interrupts are being processed, just set ipl/return */
		ppc_intr_func.setipl(pcpl);
		return;
	}

	atomic_setbits_int(&ci->ci_iactive, CI_IACTIVE_PROCESSING_SOFT);

	do {
		loopcount ++;
		if (loopcount > 50)
			printf("do_pending looping %d pcpl %x %x\n", loopcount,
			    pcpl, ci->ci_cpl);
		if((ci->ci_ipending & SI_TO_IRQBIT(SI_SOFTTTY)) &&
		    (pcpl < IPL_SOFTTTY)) {
			ci->ci_ipending &= ~SI_TO_IRQBIT(SI_SOFTTTY);

			ppc_intr_func.setipl(IPL_SOFTTTY);
			ppc_intr_enable(s);
			KERNEL_LOCK();
			softtty();
			KERNEL_UNLOCK();
			ppc_intr_disable();
			continue;
		}
		if((ci->ci_ipending & SI_TO_IRQBIT(SI_SOFTNET)) &&
		    (pcpl < IPL_SOFTNET)) {
			extern int netisr;
			int pisr;

			ci->ci_ipending &= ~SI_TO_IRQBIT(SI_SOFTNET);
			ppc_intr_func.setipl(IPL_SOFTNET);
			ppc_intr_enable(s);
			KERNEL_LOCK();
			while ((pisr = netisr) != 0) {
				atomic_clearbits_int(&netisr, pisr);
				softnet(pisr);
			}
			KERNEL_UNLOCK();
			ppc_intr_disable();
			continue;
		}
		if((ci->ci_ipending & SI_TO_IRQBIT(SI_SOFTCLOCK)) &&
		    (pcpl < IPL_SOFTCLOCK)) {
			ci->ci_ipending &= ~SI_TO_IRQBIT(SI_SOFTCLOCK);
			ppc_intr_func.setipl(IPL_SOFTCLOCK);
			ppc_intr_enable(s);
			KERNEL_LOCK();
			softclock();
			KERNEL_UNLOCK();
			ppc_intr_disable();
			continue;
		}
		break;
	} while (ci->ci_ipending & ppc_smask[pcpl]);
	/*
	 * return to original priority, notice that interrupts are
	 * disabled here because we do not want to take recursive interrupts
	 * at this point
	 */
	ppc_intr_func.setipl(pcpl);

	atomic_clearbits_int(&ci->ci_iactive, CI_IACTIVE_PROCESSING_SOFT);
}
