/* $OpenBSD: pms.c,v 1.3 2010/07/22 14:25:41 deraadt Exp $ */
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
#define	DPRINTF(...) do { if (pmsdebug) printf(__VA_ARGS__); } while(0)
#define	DPRINTFN(n, ...) do {						\
	if (pmsdebug > (n)) printf(__VA_ARGS__);			\
} while(0)
int pmsdebug = 1;
#else
#define	DPRINTF(...)
#define	DPRINTFN(n, ...)
#endif
#define	DEVNAME(sc) ((sc)->sc_dev.dv_xname)

/* PS/2 mouse data packet */
#define	PMS_PS2_BUTTONSMASK	0x07
#define	PMS_PS2_BUTTON1		0x01	/* left */
#define	PMS_PS2_BUTTON2		0x04	/* middle */
#define	PMS_PS2_BUTTON3		0x02	/* right */
#define	PMS_PS2_XNEG		0x10
#define	PMS_PS2_YNEG		0x20

/* MS IntelliMouse Explorer data packet */
#define	PMS_EXPLORER_ZNEG	0x08
#define	PMS_EXPLORER_BUTTON4	0x10
#define	PMS_EXPLORER_BUTTON5	0x20

/* Genius NetMouse data packet */
#define	PMS_NETMOUSE_BUTTON4	0x40
#define	PMS_NETMOUSE_BUTTON5	0x80

/* Genius NetScroll data packet */
#define	PMS_NETSCROLL_BUTTON4	0x01
#define	PMS_NETSCROLL_BUTTON5	0x02
#define	PMS_NETSCROLL_ZNEG	0x10

#define	PMS_BUTTON1DOWN		0x01	/* left */
#define	PMS_BUTTON2DOWN		0x02	/* middle */
#define	PMS_BUTTON3DOWN		0x04	/* right */
#define	PMS_BUTTON4DOWN		0x08
#define	PMS_BUTTON5DOWN		0x10

struct pms_softc;

struct pms_protocol {
	int type;
#define	PMS_STANDARD	0
#define	PMS_INTELLI	1
#define	PMS_EXPLORER	2
#define	PMS_NETMOUSE	3
#define	PMS_NETSCROLL	4
	int packetsize;
	int syncmask;
	int sync;
	int (*enable)(struct pms_softc *);
};

struct pms_softc {		/* driver status information */
	struct device sc_dev;

	pckbc_tag_t sc_kbctag;
	int sc_kbcslot;

	int sc_state;
#define	PMS_STATE_DISABLED	0
#define	PMS_STATE_ENABLED	1
#define	PMS_STATE_SUSPENDED	2

	struct pms_protocol protocol;
	unsigned char packet[8];

	int inputstate;
	u_int buttons;	/* mouse button status */

	struct device *sc_wsmousedev;
};

int	pmsmatch(struct device *, void *, void *);
void	pmsattach(struct device *, struct device *, void *);
int	pmsactivate(struct device *, int);

void	pmsinput(void *, int);

int	pms_change_state(struct pms_softc *, int);
int	pms_ioctl(void *, u_long, caddr_t, int, struct proc *);
int	pms_enable(void *);
void	pms_disable(void *);

int	pms_get_devid(struct pms_softc *, u_char *);
int	pms_get_status(struct pms_softc *, u_char *);
int	pms_set_rate(struct pms_softc *, int);
int	pms_set_resolution(struct pms_softc *, int);
int	pms_set_scaling(struct pms_softc *, int);

int	pms_enable_intelli(struct pms_softc *);
int	pms_enable_explorer(struct pms_softc *);
int	pms_enable_netmouse(struct pms_softc *);
int	pms_enable_netscroll(struct pms_softc *);

struct cfattach pms_ca = {
	sizeof(struct pms_softc), pmsmatch, pmsattach, NULL, pmsactivate
};

struct cfdriver pms_cd = {
	NULL, "pms", DV_DULL
};

const struct wsmouse_accessops pms_accessops = {
	pms_enable,
	pms_ioctl,
	pms_disable,
};

const struct pms_protocol pms_protocols[] = {
	/* Generic PS/2 mouse */
	{PMS_STANDARD, 3, 0xc0, 0, NULL},
	/* Microsoft IntelliMouse */
	{PMS_INTELLI, 4, 0x08, 0x08, pms_enable_intelli},
	/* Microsoft IntelliMouse Explorer */
	{PMS_EXPLORER, 4, 0xc8, 0x08, pms_enable_explorer},
	/* Genius NetMouse/NetScroll Optical */
	{PMS_NETMOUSE, 4, 0x08, 0x08, pms_enable_netmouse},
	/* Genius NetScroll */
	{PMS_NETSCROLL, 6, 0xc0, 0x00, pms_enable_netscroll}
};

int
pms_enable_intelli(struct pms_softc *sc)
{
	static const int rates[] = {200, 100, 80};
	int res, i;
	u_char resp;

	for (i = 0; i < nitems(rates); i++)
		if (pms_set_rate(sc, rates[i]))
			return (0);

	res = pms_get_devid(sc, &resp);
	if (res || (resp != 0x03))
		return (0);

	return (1);
}

int
pms_enable_explorer(struct pms_softc *sc)
{
	static const int rates[] = {200, 200, 80};
	int res, i;
	u_char resp;

	for (i = 0; i < nitems(rates); i++)
		if (pms_set_rate(sc, rates[i]))
			return (0);

	res = pms_get_devid(sc, &resp);
	if (res || (resp != 0x04))
		return (0);

	return (1);
}

int
pms_enable_netmouse(struct pms_softc *sc)
{
	u_char resp[3];

	if (pms_set_resolution(sc, 3) ||
	    pms_set_scaling(sc, 1) ||
	    pms_set_scaling(sc, 1) ||
	    pms_set_scaling(sc, 1) ||
	    pms_get_status(sc, resp) ||
	    resp[1] != '3' || resp[2] != 'U')
		return (0);

	return (1);
}

int
pms_enable_netscroll(struct pms_softc *sc)
{
	u_char resp[3];

	if (pms_set_resolution(sc, 3) ||
	    pms_set_scaling(sc, 1) ||
	    pms_set_scaling(sc, 1) ||
	    pms_set_scaling(sc, 1) ||
	    pms_get_status(sc, resp) ||
	    resp[1] != '3' || resp[2] != 'D')
		return (0);

	return (1);
}

int
pms_get_devid(struct pms_softc *sc, u_char *resp)
{
	int res;
	u_char cmd[1];

	cmd[0] = PMS_SEND_DEV_ID;
	res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot,
	    cmd, 1, 1, 1, resp);

	return (res);
}

int
pms_get_status(struct pms_softc *sc, u_char *resp)
{
	int res;
	u_char cmd[1];

	cmd[0] = PMS_SEND_DEV_STATUS;
	res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot,
	    cmd, 1, 3, 1, resp);

	return (res);
}

int
pms_set_rate(struct pms_softc *sc, int value)
{
	int res;
	u_char cmd[2];

	cmd[0] = PMS_SET_SAMPLE;
	cmd[1] = value;
	res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot,
	    cmd, 2, 0, 1, NULL);

	return (res);
}

int
pms_set_resolution(struct pms_softc *sc, int value)
{
	int res;
	u_char cmd[2];

	cmd[0] = PMS_SET_RES;
	cmd[1] = value;
	res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot,
	    cmd, 2, 0, 1, NULL);

	return (res);
}

int
pms_set_scaling(struct pms_softc *sc, int scale)
{
	int res;
	u_char cmd[1];

	switch (scale) {
	case 1:
	default:
		cmd[0] = PMS_SET_SCALE11;
		break;
	case 2:
		cmd[0] = PMS_SET_SCALE21;
		break;
	}

	res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot,
	    cmd, 1, 0, 1, NULL);

	return (res);
}

int
pmsmatch(struct device *parent, void *match, void *aux)
{
	struct pckbc_attach_args *pa = aux;
	int res;
	u_char cmd[1], resp[2];

	if (pa->pa_slot != PCKBC_AUX_SLOT)
		return (0);

	/* flush any garbage */
	pckbc_flush(pa->pa_tag, pa->pa_slot);

	/* reset the device */
	cmd[0] = PMS_RESET;
	res = pckbc_poll_cmd(pa->pa_tag, pa->pa_slot, cmd, 1, 2, resp, 1);
	if (res || resp[0] != PMS_RSTDONE || resp[1] != 0) {
		DPRINTF("pms: reset error %d (response 0x%x, type 0x%x)\n",
		    res, resp[0], resp[1]);
		return (0);
	}

	return (1);
}

void
pmsattach(struct device *parent, struct device *self, void *aux)
{
	struct pms_softc *sc = (void *)self;
	struct pckbc_attach_args *pa = aux;
	struct wsmousedev_attach_args a;
	int res;
	u_char cmd[1];

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
	cmd[0] = PMS_DEV_DISABLE;
	res = pckbc_poll_cmd(pa->pa_tag, pa->pa_slot, cmd, 1, 0, NULL, 0);
	if (res)
		printf("%s: disable error\n", DEVNAME(sc));
	pckbc_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 0);
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
	return (0);
}

int
pms_change_state(struct pms_softc *sc, int newstate)
{
	int res, i;
	u_char cmd[1], resp[2];

	switch (newstate) {
	case PMS_STATE_ENABLED:
		if (sc->sc_state == PMS_STATE_ENABLED)
			return EBUSY;
		sc->inputstate = 0;
		sc->buttons = 0;

		pckbc_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 1);

		cmd[0] = PMS_RESET;
		res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot,
		    cmd, 1, 2, 1, resp);

		sc->protocol = pms_protocols[0];
		for (i = 1; i < nitems(pms_protocols); i++)
			if (pms_protocols[i].enable(sc))
				sc->protocol = pms_protocols[i];

		DPRINTF("%s: protocol type %d\n", DEVNAME(sc),
		    sc->protocol.type);

		cmd[0] = PMS_DEV_ENABLE;
		res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot,
		    cmd, 1, 0, 1, NULL);
		if (res)
			printf("%s: enable command error\n", DEVNAME(sc));
		break;
	case PMS_STATE_DISABLED:
	case PMS_STATE_SUSPENDED:
		cmd[0] = PMS_DEV_DISABLE;
		res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot,
		    cmd, 1, 0, 1, NULL);
		if (res)
			printf("%s: disable command error\n", DEVNAME(sc));
		pckbc_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 0);
		break;
	}
	sc->sc_state = newstate;
	return 0;
}

int
pms_enable(void *v)
{
	struct pms_softc *sc = v;

	return pms_change_state(sc, PMS_STATE_ENABLED);
}

void
pms_disable(void *v)
{
	struct pms_softc *sc = v;

	pms_change_state(sc, PMS_STATE_DISABLED);
}

int
pms_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct pms_softc *sc = v;
	u_char kbcmd[2];
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
		
		kbcmd[0] = PMS_SET_RES;
		kbcmd[1] = (unsigned char) i;			
		i = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot, kbcmd, 
		    2, 0, 1, 0);
		
		if (i)
			printf("pms_ioctl: SET_RES command error\n");
		break;
		
	default:
		return (-1);
	}
	return (0);
}

/* Masks for the first byte of a packet */
#define PS2LBUTMASK 0x01
#define PS2RBUTMASK 0x02
#define PS2MBUTMASK 0x04

void pmsinput(vsc, data)
void *vsc;
int data;
{
	struct pms_softc *sc = vsc;
	signed char dy;
	u_int changed;

	if (sc->sc_state != PMS_STATE_ENABLED) {
		/* Interrupts are not expected.  Discard the byte. */
		return;
	}

	switch (sc->inputstate) {

	case 0:
		if ((data & 0xc0) == 0) { /* no ovfl, bit 3 == 1 too? */
			sc->buttons = ((data & PS2LBUTMASK) ? 0x1 : 0) |
			    ((data & PS2MBUTMASK) ? 0x2 : 0) |
			    ((data & PS2RBUTMASK) ? 0x4 : 0);
			++sc->inputstate;
		}
		break;

	case 1:
		sc->dx = data;
		/* Bounding at -127 avoids a bug in XFree86. */
		sc->dx = (sc->dx == -128) ? -127 : sc->dx;
		++sc->inputstate;
		break;

	case 2:
		dy = data;
		dy = (dy == -128) ? -127 : dy;
		sc->inputstate = 0;

		changed = (sc->buttons ^ sc->oldbuttons);
		sc->oldbuttons = sc->buttons;

		if (sc->dx || dy || changed)
			wsmouse_input(sc->sc_wsmousedev,
				      sc->buttons, sc->dx, dy, 0, 0,
				      WSMOUSE_INPUT_DELTA);
		break;
	}

	return;
}

struct cfdriver pms_cd = {
	NULL, "pms", DV_DULL
};
