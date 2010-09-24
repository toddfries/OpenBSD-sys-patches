/*	$OpenBSD: pxa2x0_clock.c,v 1.6 2008/01/03 17:59:32 kettenis Exp $ */

/*
 * Copyright (c) 2005 Dale Rahn <drahn@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/timetc.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/sa11x0/sa11x0_reg.h>
#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa11x0_ostreg.h>
#include <arm/xscale/pxa2x0reg.h>

int	pxaost_match(struct device *, void *, void *);
void	pxaost_attach(struct device *, struct device *, void *);

int	doclockintr(void *);
int	clockintr(void *);
int	statintr(void *);
void	rtcinit(void);

struct pxaost_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	u_int32_t		sc_clk0_hz;
	u_int32_t		sc_clock_count;
	u_int32_t		sc_statclock_count;
	u_int32_t		sc_statclock_step;
	u_int32_t		sc_clock_step;
	u_int32_t		sc_clock_step_err_cnt;
	u_int32_t		sc_clock_step_error;
};

static struct pxaost_softc *pxaost_sc = NULL;

#ifndef STATHZ
#define STATHZ	64
#endif

struct cfattach pxaost_ca = {
	sizeof (struct pxaost_softc), pxaost_match, pxaost_attach
};

struct cfdriver pxaost_cd = {
	NULL, "pxaost", DV_DULL
};

u_int	pxaost_get_timecount(struct timecounter *tc);

static struct timecounter pxaost_timecounter = {
	pxaost_get_timecount, NULL, 0xffffffff, 0,
	"pxaost", 0, NULL
};

int
pxaost_match(parent, match, aux)
	struct device *parent;
	void *match;
	void *aux;
{
	return (1);
}

void
pxaost_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct pxaost_softc *sc = (struct pxaost_softc*)self;
	struct sa11x0_attach_args *sa = aux;

	printf("\n");

	if ((cputype & ~CPU_ID_XSCALE_COREREV_MASK) == CPU_ID_PXA27X)
		sc->sc_clk0_hz = 3250000;
	else /* PXA26x and PXA25x */
		sc->sc_clk0_hz = 3686400;

	sc->sc_iot = sa->sa_iot;

	pxaost_sc = sc;

	if (bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0,
	    &sc->sc_ioh))
		panic("%s: Cannot map registers", self->dv_xname);

	/* disable all channel and clear interrupt status */
	bus_space_write_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh, SAOST_IR, 0);
	bus_space_write_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh, SAOST_SR, 0x3f);

	bus_space_write_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh, OST_OSMR0,
	    pxaost_sc->sc_clock_count);
	bus_space_write_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh, OST_OSMR1,
	    pxaost_sc->sc_statclock_count);

	/* Zero the counter value */
	bus_space_write_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh, OST_OSCR0, 0);

}

u_int
pxaost_get_timecount(struct timecounter *tc)
{
	return bus_space_read_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh,
	    OST_OSCR0);
}

int
clockintr(void *arg)
{
	struct clockframe *frame = arg;
	u_int32_t oscr, match;
	u_int32_t match_error;

	bus_space_write_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh, SAOST_SR, 0x01);

	match = pxaost_sc->sc_clock_count;

	do {
		match += pxaost_sc->sc_clock_step;
		pxaost_sc->sc_clock_step_error +=
		    pxaost_sc->sc_clock_step_err_cnt;
		if (pxaost_sc->sc_clock_count > hz) {
			match_error = pxaost_sc->sc_clock_step_error / hz;
			pxaost_sc->sc_clock_step_error -= (match_error * hz);
			match += match_error;
		}
		pxaost_sc->sc_clock_count = match;
		hardclock(frame);

		oscr = bus_space_read_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh,
		    OST_OSCR0);

	} while ((signed)(oscr - match) > 0);

	 /* prevent missed interrupts */
	if (oscr - match < 500)
		match += 500;

	bus_space_write_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh, OST_OSMR0,
	    match);

	return 1;
}

int
statintr(void *arg)
{
	struct clockframe *frame = arg;
	u_int32_t oscr, match = 0;

	bus_space_write_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh, SAOST_SR, 0x02);

	/* schedule next clock intr */
	match = pxaost_sc->sc_statclock_count;
	do {
		match += pxaost_sc->sc_statclock_step;
		pxaost_sc->sc_statclock_count = match;
		statclock(frame);

		oscr = bus_space_read_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh,
		    OST_OSCR0);

	} while ((signed)(oscr - match) > 0);

	 /* prevent missed interrupts */
	if (oscr - match < 500)
		match += 500;
	bus_space_write_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh, OST_OSMR1,
	    match);

	return 1;
}

void
setstatclockrate(int newstathz)
{
	u_int32_t count;
	pxaost_sc->sc_statclock_step = pxaost_sc->sc_clk0_hz / newstathz;
	count = pxaost_sc->sc_statclock_step;
	count += bus_space_read_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh, OST_OSCR0);
	bus_space_write_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh,
	    OST_OSMR1, count);
	pxaost_sc->sc_statclock_count = count;
}

void
cpu_initclocks()
{
	u_int32_t clk;

	stathz = STATHZ;
	profhz = stathz;
	pxaost_sc->sc_statclock_step = pxaost_sc->sc_clk0_hz / stathz;
	pxaost_sc->sc_clock_step = pxaost_sc->sc_clk0_hz / hz;
	pxaost_sc->sc_clock_step_err_cnt = pxaost_sc->sc_clk0_hz % hz;
	pxaost_sc->sc_clock_step_error = 0;

	/* Use the channels 0 and 1 for hardclock and statclock, respectively */
	pxaost_sc->sc_clock_count = pxaost_sc->sc_clock_step;
	pxaost_sc->sc_statclock_count = pxaost_sc->sc_clk0_hz / stathz;

	pxa2x0_intr_establish(PXA2X0_INT_OST0, IPL_CLOCK, clockintr, 0, "clock");
	pxa2x0_intr_establish(PXA2X0_INT_OST1, IPL_CLOCK, statintr, 0, "statclock");

	bus_space_write_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh, SAOST_SR, 0x3f);
	bus_space_write_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh, SAOST_IR, 0x03);

	pxaost_timecounter.tc_frequency = pxaost_sc->sc_clk0_hz;

	clk = bus_space_read_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh, OST_OSCR0);

	bus_space_write_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh, OST_OSMR0,
	    clk + pxaost_sc->sc_clock_count);
	bus_space_write_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh, OST_OSMR1,
	    clk + pxaost_sc->sc_statclock_count);

	tc_init(&pxaost_timecounter);
}

void
delay(u_int usecs)
{
	u_int32_t ost, i;
	volatile int j;

	if (!pxaost_sc) {
		/* clock isn't initialized yet */
		for (; usecs > 0; usecs--)
			for (j = 100; j > 0; j--)
				;
		return;
	}

	/* 1us == 3.25 ticks on pxa27x */
	/* 1us == 3.6864 ticks on pxa25x and 26x */
	i = usecs / 1000000;
	usecs %= 1000000;

	while (i--) {
		ost = bus_space_read_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh,
		    OST_OSCR0) + pxaost_sc->sc_clk0_hz;	/* 1s ahead */
		while((signed)(ost - bus_space_read_4(pxaost_sc->sc_iot,
			pxaost_sc->sc_ioh, OST_OSCR0)) > 0);
	}

	ost = bus_space_read_4(pxaost_sc->sc_iot, pxaost_sc->sc_ioh,
	    OST_OSCR0) + ((pxaost_sc->sc_clk0_hz * usecs) / 1000000);
	while((signed)(ost - bus_space_read_4(pxaost_sc->sc_iot,
		pxaost_sc->sc_ioh, OST_OSCR0)) > 0);
}
