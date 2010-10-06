/* $OpenBSD: pms.c,v 1.7 2010/10/02 00:28:57 krw Exp $ */
/* $NetBSD: psm.c,v 1.11 2000/06/05 22:20:57 sommerfeld Exp $ */

/*-
 * Copyright (c) 1994 Charles M. Hannum.
 * Copyright (c) 1992, 1993 Erik Forsberg.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL I BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>

#include <machine/bus.h>

#include <dev/ic/pckbcvar.h>

#include <dev/pckbc/pmsreg.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#ifdef PMS_DEBUG
#define DPRINTF(...) do { if (pmsdebug) printf(__VA_ARGS__); } while(0)
#define DPRINTFN(n, ...) do {						\
	if (pmsdebug > (n)) printf(__VA_ARGS__);			\
} while(0)
int pmsdebug = 1;
#else
#define DPRINTF(...)
#define DPRINTFN(n, ...)
#endif

#define DEVNAME(sc) ((sc)->sc_dev.dv_xname)

/* PS/2 mouse data packet */
#define PMS_PS2_BUTTONSMASK	0x07
#define PMS_PS2_BUTTON1		0x01	/* left */
#define PMS_PS2_BUTTON2		0x04	/* middle */
#define PMS_PS2_BUTTON3		0x02	/* right */
#define PMS_PS2_XNEG		0x10
#define PMS_PS2_YNEG		0x20

#define PMS_BUTTON1DOWN		0x01	/* left */
#define PMS_BUTTON2DOWN		0x02	/* middle */
#define PMS_BUTTON3DOWN		0x04	/* right */

struct pms_softc;

struct pms_protocol {
	int type;
#define PMS_STANDARD	0
#define PMS_INTELLI	1
	int packetsize;
	int syncmask;
	int syncval;
	int (*enable)(struct pms_softc *);
	int (*sync)(struct pms_softc *, int);
	void (*proc)(struct pms_softc *, int *, int *, int *, u_int *);
};

struct pms_softc {		/* driver status information */
	struct device sc_dev;

	pckbc_tag_t sc_kbctag;
	int sc_kbcslot;

	int poll;
	int sc_state;
#define PMS_STATE_DISABLED	0
#define PMS_STATE_ENABLED	1
#define PMS_STATE_SUSPENDED	2

	struct pms_protocol protocol;
	unsigned char packet[8];

	int inputstate;
	u_int buttons;		/* mouse button status */

	struct device *sc_wsmousedev;
};

int	pmsprobe(struct device *, void *, void *);
void	pmsattach(struct device *, struct device *, void *);
int	pmsactivate(struct device *, int);

void	pmsinput(void *, int);

int	pms_change_state(struct pms_softc *, int);
int	pms_ioctl(void *, u_long, caddr_t, int, struct proc *);
int	pms_enable(void *);
void	pms_disable(void *);

int	pms_cmd(struct pms_softc *, u_char *, int, u_char *, int);

int	pms_setintellimode(struct pms_softc *sc);

const struct wsmouse_accessops pms_accessops = {
	pms_enable,
	pms_ioctl,
	pms_disable,
};

int
pms_cmd(struct pms_softc *sc, u_char *cmd, int len, u_char *resp, int resplen)
{
	if (sc->poll) {
		return pckbc_poll_cmd(sc->sc_kbctag, sc->sc_kbcslot,
		    cmd, len, resplen, resp, 1);
	} else {
		return pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot,
		    cmd, len, resplen, 1, resp);
	}
}

int
pms_setintellimode(struct pms_softc *sc)
{
	u_char cmd[2], resp[1];
	int i, res;
	static const u_char rates[] = {200, 100, 80};

	cmd[0] = PMS_SET_SAMPLE;
	for (i = 0; i < 3; i++) {
		cmd[1] = rates[i];
		res = pms_cmd(sc, cmd, 2, NULL, 0);
		if (res)
			return (0);
	}

	cmd[0] = PMS_SEND_DEV_ID;
	res = pms_cmd(sc, cmd, 1, resp, 1);
	if (res || resp[0] != 3)
		return (0);

	return (1);
}

int
pmsprobe(struct device *parent, void *match, void *aux)
{
	struct pckbc_attach_args *pa = aux;
	int res;
	u_char cmd[1], resp[2];

	if (pa->pa_slot != PCKBC_AUX_SLOT)
		return 0;

	/* flush any garbage */
	pckbc_flush(pa->pa_tag, pa->pa_slot);

	/* reset the device */
	cmd[0] = PMS_RESET;
	res = pckbc_poll_cmd(pa->pa_tag, pa->pa_slot, cmd, 1, 2, resp, 1);
	if (res || resp[0] != PMS_RSTDONE || resp[1] != 0) {
		DPRINTF("pms: reset error (%d, response 0x%x, type 0x%x)\n",
		    res, resp[0], resp[1]);
		return 0;
	}

	return (1);
}

void
pmsattach(struct device *parent, struct device *self, void *aux)
{
	struct pms_softc *sc = (void *)self;
	struct pckbc_attach_args *pa = aux;
	struct wsmousedev_attach_args a;

	sc->sc_kbctag = pa->pa_tag;
	sc->sc_kbcslot = pa->pa_slot;

	printf("\n");

	pckbc_set_inputhandler(sc->sc_kbctag, sc->sc_kbcslot,
	    pmsinput, sc, DEVNAME(sc));

	a.accessops = &pms_accessops;
	a.accesscookie = sc;

	/*
	 * Attach the wsmouse, saving a handle to it.
	 * Note that we don't need to check this pointer against NULL
	 * here or in pmsintr, because if this fails pms_enable() will
	 * never be called, so pmsinput() will never be called.
	 */
	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint);

	/* no interrupts until enabled */
	sc->poll = 1;
	pms_change_state(sc, PMS_STATE_DISABLED);
}

int
pmsactivate(struct device *self, int act)
{
	struct pms_softc *sc = (struct pms_softc *)self;

	switch (act) {
	case DVACT_SUSPEND:
		if (sc->sc_state == PMS_STATE_ENABLED)
			pms_change_state(sc, PMS_STATE_SUSPENDED);
		break;
	case DVACT_RESUME:
		if (sc->sc_state == PMS_STATE_SUSPENDED)
			pms_change_state(sc, PMS_STATE_ENABLED);
		break;
	}
	return 0;
}

int
pms_cmd(struct pms_softc *sc, u_char *cmd, int len, u_char *resp, int resplen)
{
	if (sc->poll) {
		return pckbc_poll_cmd(sc->sc_kbctag, sc->sc_kbcslot,
		    cmd, len, resplen, resp, 1);
	} else {
		return pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot,
		    cmd, len, resplen, 1, resp);
	}
}

int
pms_get_devid(struct pms_softc *sc, u_char *resp)
{
	u_char cmd[1];

	cmd[0] = PMS_SEND_DEV_ID;
	return pms_cmd(sc, cmd, 1, resp, 1);
}

int
pms_get_status(struct pms_softc *sc, u_char *resp)
{
	u_char cmd[1], resp[2];
	int res;
	u_char cmd[1], resp[2];

	cmd[0] = PMS_RESET;
	res = pms_cmd(sc, cmd, 1, resp, 2);
#ifdef PMS_DEBUG
	if (res || resp[0] != PMS_RSTDONE || resp[1] != 0) {
		DPRINTF("%s: reset error (%d, response 0x%x, type 0x%x)\n",
		    DEVNAME(sc), res, resp[0], resp[1]);
	}
#endif
}

void
pms_dev_disable(struct pms_softc *sc)
{
	int res;
	u_char cmd[1];

	cmd[0] = PMS_DEV_DISABLE;
	res = pms_cmd(sc, cmd, 1, NULL, 0);
#ifdef PMS_DEBUG
	if (res)
		DPRINTF("%s: disable error (%d)\n", DEVNAME(sc), res);
#endif
	pckbc_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 0);
}

void
pms_dev_enable(struct pms_softc *sc)
{
	int i, res;
	u_char cmd[1];

	pckbc_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 1);

	pms_dev_reset(sc);

	sc->protocol = pms_protocols[0];
	for (i = 1; i < nitems(pms_protocols); i++)
		if (pms_protocols[i].enable(sc))
			sc->protocol = pms_protocols[i];

	DPRINTF("%s: protocol type %d\n", DEVNAME(sc), sc->protocol.type);

	cmd[0] = PMS_DEV_ENABLE;
	res = pms_cmd(sc, cmd, 1, NULL, 0);
#ifdef PMS_DEBUG
	if (res)
		DPRINTF("%s: enable error (%d)\n", DEVNAME(sc), res);
#endif
}

int
pms_enable_intelli(struct pms_softc *sc)
{
	static const int rates[] = {200, 100, 80};
	int res, i;
	u_char resp;

	for (i = 0; i < nitems(rates); i++)
		if (pms_set_rate(sc, rates[i]))
			return 0;

	res = pms_get_devid(sc, &resp);
	if (res || (resp != 0x03))
		return 0;

	return 1;
}

int
pms_change_state(struct pms_softc *sc, int newstate)
{
	switch (newstate) {
	case PMS_STATE_ENABLED:
		if (sc->sc_state == PMS_STATE_ENABLED)
			return EBUSY;

		sc->inputstate = 0;
		sc->oldbuttons = 0;

		pckbc_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 1);

		pckbc_flush(sc->sc_kbctag, sc->sc_kbcslot);

		cmd[0] = PMS_RESET;
		res = pms_cmd(sc, cmd, 1, resp, 2);

		sc->intelli = pms_setintellimode(sc);

		cmd[0] = PMS_DEV_ENABLE;
		res = pms_cmd(sc, cmd, 1, NULL, 0);
		if (res)
			printf("pms_enable: command error\n");
#if 0
		{
			u_char scmd[2];

			scmd[0] = PMS_SET_RES;
			scmd[1] = 3; /* 8 counts/mm */
			res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot, scmd,
						2, 0, 1, 0);
			if (res)
				printf("pms_enable: setup error1 (%d)\n", res);

			scmd[0] = PMS_SET_SCALE21;
			res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot, scmd,
						1, 0, 1, 0);
			if (res)
				printf("pms_enable: setup error2 (%d)\n", res);

			scmd[0] = PMS_SET_SAMPLE;
			scmd[1] = 100; /* 100 samples/sec */
			res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot, scmd,
						2, 0, 1, 0);
			if (res)
				printf("pms_enable: setup error3 (%d)\n", res);
		}
#endif
		sc->sc_state = newstate;
		sc->poll = 0;
		break;
	case PMS_STATE_DISABLED:
	case PMS_STATE_SUSPENDED:
		cmd[0] = PMS_DEV_DISABLE;
		res = pms_cmd(sc, cmd, 1, NULL, 0);
		if (res)
			printf("pms_disable: command error\n");
		pckbc_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 0);
		sc->sc_state = newstate;
		sc->poll = (newstate == PMS_STATE_SUSPENDED) ? 1 : 0;
		break;
	}
	sc->sc_state = newstate;
	return 0;
}

int
pms_enable(void *vsc)
{
	struct pms_softc *sc = vsc;

	return pms_change_state(sc, PMS_STATE_ENABLED);
}

void
pms_disable(void *vsc)
{
	struct pms_softc *sc = vsc;

	pms_change_state(sc, PMS_STATE_DISABLED);
}

int
pms_ioctl(void *vsc, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct pms_softc *sc = vsc;
	int i;

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_PS2;
		break;
	case WSMOUSEIO_SRES:
		i = ((int) *(u_int *)data - 12) / 25;
		/* valid values are {0,1,2,3} */
		if (i < 0)
			i = 0;
		if (i > 3)
			i = 3;

		if (pms_set_resolution(sc, i)) {
			DPRINTF("%s: ioctl: set resolution error\n",
			    DEVNAME(sc));
		}
		break;
	default:
		return -1;
	}
	return 0;
}

int
pms_sync_generic(struct pms_softc *sc, int data)
{
	if ((sc->inputstate == 0) &&
	    ((data & sc->protocol.syncmask) != sc->protocol.syncval)) {
		return -1;
	}
	return 0;
}

void
pms_proc_generic(struct pms_softc *sc, int *dx, int *dy, int *dz, u_int *buttons)
{
	static const u_int butmap[8] = {
	    0,
	    PMS_BUTTON1DOWN,
	    PMS_BUTTON3DOWN,
	    PMS_BUTTON1DOWN | PMS_BUTTON3DOWN,
	    PMS_BUTTON2DOWN,
	    PMS_BUTTON1DOWN | PMS_BUTTON2DOWN,
	    PMS_BUTTON2DOWN | PMS_BUTTON3DOWN,
	    PMS_BUTTON1DOWN | PMS_BUTTON2DOWN | PMS_BUTTON3DOWN
	};

	*buttons = butmap[sc->packet[0] & PMS_PS2_BUTTONSMASK];
	*dx = (sc->packet[0] & PMS_PS2_XNEG) ?
	    sc->packet[1] - 256 : sc->packet[1];
	*dy = (sc->packet[0] & PMS_PS2_YNEG) ?
	    sc->packet[2] - 256 : sc->packet[2];

	switch (sc->protocol.type) {
	case PMS_STANDARD:
		*dz = 0;
		break;
	case PMS_INTELLI:
		*dz = (char)sc->packet[3];
		break;
	}
}

void pmsinput(void *vsc, int data)
{
	struct pms_softc *sc = vsc;
	u_int changed, newbuttons;
	int  dx, dy, dz;

	if (sc->sc_state != PMS_STATE_ENABLED) {
		/* Interrupts are not expected.  Discard the byte. */
		return;
	}

	if (sc->protocol.sync(sc, data)) {
		DPRINTF("%s: not in sync yet, discard input\n", DEVNAME(sc));
		sc->inputstate = 0;
		return;
	}

	if (sc->inputstate < sc->protocol.packetsize) {
		sc->packet[sc->inputstate++] = data & 0xff;
		if (sc->inputstate != sc->protocol.packetsize)
			return;
	}

	sc->protocol.proc(sc, &dx, &dy, &dz, &newbuttons);

	changed = (sc->buttons ^ newbuttons);
	sc->buttons = newbuttons;
	sc->inputstate = 0;

#ifdef PMS_DEBUG
	int i;

	DPRINTFN(3, "%s: packet 0x", DEVNAME(sc));
	for (i = 0; i < sc->protocol.packetsize; i++)
		DPRINTFN(3, "%02x", sc->packet[i]);
	DPRINTFN(3, "\n");

	DPRINTFN(2, "%s: dx %+03d dy %+03d dz %+03d buttons 0x%02x\n",
	    DEVNAME(sc), dx, dy, dz, newbuttons);
#endif

	if (dx || dy || dz || changed) {
		wsmouse_input(sc->sc_wsmousedev,
		    newbuttons, dx, dy, dz, 0, WSMOUSE_INPUT_DELTA);
	}

	memset(sc->packet, 0, sc->protocol.packetsize);
}
