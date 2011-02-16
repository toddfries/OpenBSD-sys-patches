/*	$OpenBSD: clock.c,v 1.34 2010/09/20 06:33:47 matthew Exp $ */

/*
 * Copyright (c) 2001-2004 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/evcount.h>
#include <sys/timetc.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <mips64/dev/clockvar.h>
#include <mips64/archtype.h>

static struct evcount clk_count;
static int clk_irq = 5;

/* Definition of the driver for autoconfig. */
int	clockmatch(struct device *, void *, void *);
void	clockattach(struct device *, struct device *, void *);
intrmask_t clock_int5_dummy(intrmask_t, struct trap_frame *);
intrmask_t clock_int5(intrmask_t, struct trap_frame *);
void clock_int5_init(struct clock_softc *);

struct cfdriver clock_cd = {
	NULL, "clock", DV_DULL, NULL, 0
};

struct cfattach clock_ca = {
	sizeof(struct clock_softc), clockmatch, clockattach
};

void	clock_calibrate(struct cpu_info *);
uint32_t clock_int5(uint32_t, struct trap_frame *);

u_int cp0_get_timecount(struct timecounter *);

struct timecounter cp0_timecounter = {
	cp0_get_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	0xffffffff,		/* counter_mask */
	0,			/* frequency */
	"CP0",			/* name */
	0			/* quality */
};

#define	SECMIN	(60)		/* seconds per minute */
#define	SECHOUR	(60*SECMIN)	/* seconds per hour */

#define	YEARDAYS(year)	(((((year) + 1900) % 4) == 0 && \
			 ((((year) + 1900) % 100) != 0 || \
			  (((year) + 1900) % 400) == 0)) ? 366 : 365)

int
clockmatch(struct device *parent, void *cfdata, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	if (strcmp(maa->maa_name, clock_cd.cd_name) != 0)
		return 0;

	if (cf->cf_unit >= 1)
		return 0;
	return 10;	/* Try to get clock early */
}

void
clockattach(struct device *parent, struct device *self, void *aux)
{
	printf(": ticker on int5 using count register\n");

	/*
	 * We need to register the interrupt now, for idle_mask to
	 * be computed correctly.
	 */
	set_intr(INTPRI_CLOCK, CR_INT_5, clock_int5);
	evcount_attach(&clk_count, "clock", &clk_irq);
	/* try to avoid getting clock interrupts early */
	cp0_set_compare(cp0_get_count() - 1);
}

/*
 *	Clock interrupt code for machines using the on cpu chip
 *	counter register. This register counts at half the pipeline
 *	frequency so the frequency must be known and the options
 *	register wired to allow its use.
 *
 *	The code is enabled by setting 'cpu_counter_interval'.
 */

/*
 *  Dummy count register interrupt handler used on some targets.
 *  Just resets the compare register and acknowledge the interrupt.
 */
intrmask_t
clock_int5_dummy(intrmask_t mask, struct trap_frame *tf)
{
        cp0_set_compare(0);      /* Shut up counter int's for a while */
	return CR_INT_5;	/* Clock is always on 5 */
}

/*
 *  Interrupt handler for targets using the internal count register
 *  as interval clock. Normally the system is run with the clock
 *  interrupt always enabled. Masking is done here and if the clock
 *  can not be run the tick is just counted and handled later when
 *  the clock is unmasked again.
 */
uint32_t
clock_int5(uint32_t mask, struct trap_frame *tf)
{
	u_int32_t clkdiff;
	struct cpu_info *ci = curcpu();

	/*
	 * If we got an interrupt before we got ready to process it,
	 * retrigger it as far as possible. cpu_initclocks() will
	 * take care of retriggering it correctly.
	 */
	if (ci->ci_clock_started == 0) {
		cp0_set_compare(cp0_get_count() - 1);

		return CR_INT_5;
	}

	/*
	 * Count how many ticks have passed since the last clock interrupt...
	 */
	clkdiff = cp0_get_count() - ci->ci_cpu_counter_last;
	while (clkdiff >= ci->ci_cpu_counter_interval) {
		ci->ci_cpu_counter_last += ci->ci_cpu_counter_interval;
		clkdiff = cp0_get_count() - ci->ci_cpu_counter_last;
		ci->ci_pendingticks++;
	}
	ci->ci_pendingticks++;
	ci->ci_cpu_counter_last += ci->ci_cpu_counter_interval;

	/*
	 * Set up next tick, and check if it has just been hit; in this
	 * case count it and schedule one tick ahead.
	 */
	cp0_set_compare(ci->ci_cpu_counter_last);
	clkdiff = cp0_get_count() - ci->ci_cpu_counter_last;
	if ((int)clkdiff >= 0) {
		ci->ci_cpu_counter_last += ci->ci_cpu_counter_interval;
		ci->ci_pendingticks++;
		cp0_set_compare(ci->ci_cpu_counter_last);
	}

	/*
	 * Process clock interrupt unless it is currently masked.
	 */
	if (tf->ipl < IPL_CLOCK) {
#ifdef MULTIPROCESSOR
		u_int32_t sr;
		/* Enable interrupts at this (hardware) level again */
		sr = getsr();
		ENABLEIPI();
		__mp_lock(&kernel_lock);
#endif
		while (ci->ci_pendingticks) {
			clk_count.ec_count++;
			hardclock(tf);
			ci->ci_pendingticks--;
		}
#ifdef MULTIPROCESSOR
		__mp_unlock(&kernel_lock);
		setsr(sr);
#endif
	}

	return CR_INT_5;	/* Clock is always on 5 */
}

/*
 * Wait "n" microseconds.
 */
void
delay(int n)
{
	int dly;
	int p, c;
	struct cpu_info *ci = curcpu();
	uint32_t delayconst;

	delayconst = ci->ci_delayconst;
	if (delayconst == 0)
		delayconst = bootcpu_hwinfo.clock / 2;
	p = cp0_get_count();
	dly = (delayconst / 1000000) * n;
	while (dly > 0) {
		c = cp0_get_count();
		dly -= c - p;
		p = c;
	}
}

/*
 *	Mips machine independent clock routines.
 */

/*
 * Calibrate cpu clock against the TOD clock if available.
 */
void
clock_calibrate(struct cpu_info *ci)
{
	struct clock_softc *sc = (struct clock_softc *)clock_cd.cd_devs[0];

	if (cd->tod_get == NULL)
		return;

	(*cd->tod_get)(cd->tod_cookie, 0, &ct);
	first_sec = ct.sec;

	/* Let the clock tick one second. */
	do {
		first_cp0 = cp0_get_count();
		(*cd->tod_get)(cd->tod_cookie, 0, &ct);
	} while (ct.sec == first_sec);
	first_sec = ct.sec;
	/* Let the clock tick one more second. */
	do {
		second_cp0 = cp0_get_count();
		(*cd->tod_get)(cd->tod_cookie, 0, &ct);
	} while (ct.sec == first_sec);

	cycles_per_sec = second_cp0 - first_cp0;
	ci->ci_hw.clock = cycles_per_sec * 2;
	ci->ci_delayconst = cycles_per_sec;
}

/*
 * Start the real-time and statistics clocks. Leave stathz 0 since there
 * are no other timers available.
 */
void
cpu_initclocks()
{
	struct cpu_info *ci = curcpu();

	hz = 100;
	profhz = 100;
	stathz = 0;	/* XXX no stat clock yet */

	clock_calibrate(ci);

	tick = 1000000 / hz;	/* number of micro-seconds between interrupts */
	tickadj = 240000 / (60 * hz);		/* can adjust 240ms in 60s */

	cp0_timecounter.tc_frequency = (uint64_t)ci->ci_hw.clock / 2;
	tc_init(&cp0_timecounter);
	cpu_startclock(ci);
}

void
cpu_startclock(struct cpu_info *ci)
{
	int s;

	if (!CPU_IS_PRIMARY(ci)) {
		s = splhigh();
		microuptime(&ci->ci_schedstate.spc_runtime);
		splx(s);

		/* try to avoid getting clock interrupts early */
		cp0_set_compare(cp0_get_count() - 1);

		clock_calibrate(ci);
	}

	/* Start the clock. */
	s = splclock();
	ci->ci_cpu_counter_interval = (ci->ci_hw.clock / 2) / hz;
	ci->ci_cpu_counter_last = cp0_get_count() + ci->ci_cpu_counter_interval;
	cp0_set_compare(ci->ci_cpu_counter_last);
	ci->ci_clock_started++;
	splx(s);
}

/*
 * We assume newhz is either stathz or profhz, and that neither will
 * change after being set up above.  Could recalculate intervals here
 * but that would be a drag.
 */
void
setstatclockrate(int newhz)
{
}

/*
 * This code is defunct after 2099. Will Unix still be here then??
 */
static short dayyr[12] = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

/*
 * Initialize the time of day register, based on the time base which
 * is, e.g. from a filesystem.
 */
void
inittodr(time_t base)
{
	struct timespec ts;
	struct tod_time c;
	struct clock_softc *sc = (struct clock_softc *)clock_cd.cd_devs[0];
	int days, yr;

	ts.tv_nsec = 0;

	if (base < 15*SECYR) {
		printf("WARNING: preposterous time in file system");
		/* read the system clock anyway */
		base = 40 * SECYR;	/* 2010 */
	}

	/*
	 * Read RTC chip registers NOTE: Read routines are responsible
	 * for sanity checking clock. Dates after 19991231 should be
	 * returned as year >= 100.
	 */
	if (sc->sc_clock.clk_get) {
		(*sc->sc_clock.clk_get)(sc, base, &c);
	} else {
		printf("WARNING: No TOD clock, believing file system.\n");
		goto bad;
	}

	days = 0;
	for (yr = 70; yr < c.year; yr++) {
		days += YEARDAYS(yr);
	}

	days += dayyr[c.mon - 1] + c.day - 1;
	if (YEARDAYS(c.year) == 366 && c.mon > 2) {
		days++;
	}

	/* now have days since Jan 1, 1970; the rest is easy... */
	ts.tv_sec = days * SECDAY + c.hour * 3600 + c.min * 60 + c.sec;
	tc_setclock(&ts);
	sc->sc_initted = 1;

	/*
	 * See if we gained/lost time.
	 */
	if (base < ts.tv_sec - 5*SECYR) {
		printf("WARNING: file system time much less than clock time\n");
	} else if (base > ts.tv_sec + 5*SECYR) {
		printf("WARNING: clock time much less than file system time\n");
		printf("WARNING: using file system time\n");
	} else {
		return;
	}

bad:
	ts.tv_sec = base;
	tc_setclock(&ts);
	sc->sc_initted = 1;
	printf("WARNING: CHECK AND RESET THE DATE!\n");
}

/*
 * Reset the TOD clock. This is done when the system is halted or
 * when the time is reset by the stime system call.
 */
void
resettodr()
{
	struct tod_time c;
	struct tod_desc *cd = &sys_tod;
	int t, t2;

	/*
	 *  Don't reset clock if time has not been set!
	 */
	if (!sc->sc_initted) {
		return;
	}

	/* compute the day of week. 1 is Sunday*/
	t2 = time_second / SECDAY;
	c.dow = (t2 + 5) % 7 + 1;	/* 1/1/1970 was thursday */

	/* compute the year */
	t = 0;
	t2 = time_second / SECDAY;
	c.year = 69;
	while (t2 >= 0) {	/* whittle off years */
		t = t2;
		c.year++;
		t2 -= YEARDAYS(c.year);
	}

	/* t = month + day; separate */
	t2 = YEARDAYS(c.year);
	for (c.mon = 1; c.mon < 12; c.mon++) {
		if (t < dayyr[c.mon] + (t2 == 366 && c.mon > 1))
			break;
	}

	c.day = t - dayyr[c.mon - 1] + 1;
	if (t2 == 366 && c.mon > 2) {
		c.day--;
	}

	t = time_second % SECDAY;
	c.hour = t / 3600;
	t %= 3600;
	c.min = t / 60;
	c.sec = t % 60;

	if (sc->sc_clock.clk_set) {
		(*sc->sc_clock.clk_set)(sc, &c);
	}
}

u_int
cp0_get_timecount(struct timecounter *tc)
{
	/* XXX SMP */
	return (cp0_get_count());
}
