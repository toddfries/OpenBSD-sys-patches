<<<<<<< HEAD
/*	$OpenBSD: ukbd.c,v 1.26 2007/01/13 07:43:15 miod Exp $	*/
=======
/*	$OpenBSD: ukbd.c,v 1.54 2010/08/29 15:28:11 miod Exp $	*/
>>>>>>> origin/master
/*      $NetBSD: ukbd.c,v 1.85 2003/03/11 16:44:00 augustss Exp $        */

/*
 * Copyright (c) 2010 Miodrag Vallat.
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
/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * HID spec: http://www.usb.org/developers/devclass_docs/HID1_11.pdf
 */

#include <sys/param.h>
#include <sys/systm.h>
#if defined(__OpenBSD__)
#include <sys/timeout.h>
#else
#include <sys/callout.h>
#endif
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/ioctl.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usb_quirks.h>
#include <dev/usb/uhidev.h>
#include <dev/usb/hid.h>
#include <dev/usb/ukbdvar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <dev/usb/hidkbdsc.h>
#include <dev/usb/hidkbdvar.h>

#ifdef UKBD_DEBUG
#define DPRINTF(x)	do { if (ukbddebug) logprintf x; } while (0)
#define DPRINTFN(n,x)	do { if (ukbddebug>(n)) logprintf x; } while (0)
int	ukbddebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

<<<<<<< HEAD
#define MAXKEYCODE 6
#define MAXMOD 8		/* max 32 */

struct ukbd_data {
	u_int32_t	modifiers;
	u_int8_t	keycode[MAXKEYCODE];
};

#define PRESS    0x000
#define RELEASE  0x100
#define CODEMASK 0x0ff

#if defined(WSDISPLAY_COMPAT_RAWKBD)
#define NN 0			/* no translation */
/*
 * Translate USB keycodes to US keyboard XT scancodes.
 * Scancodes >= 0x80 represent EXTENDED keycodes.
 *
 * See http://www.microsoft.com/whdc/device/input/Scancode.mspx
 */
Static const u_int8_t ukbd_trtab[256] = {
      NN,   NN,   NN,   NN, 0x1e, 0x30, 0x2e, 0x20, /* 00 - 07 */
    0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, /* 08 - 0f */
    0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1f, 0x14, /* 10 - 17 */
    0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c, 0x02, 0x03, /* 18 - 1f */
    0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, /* 20 - 27 */
    0x1c, 0x01, 0x0e, 0x0f, 0x39, 0x0c, 0x0d, 0x1a, /* 28 - 2f */
    0x1b, 0x2b, 0x2b, 0x27, 0x28, 0x29, 0x33, 0x34, /* 30 - 37 */
    0x35, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, /* 38 - 3f */
    0x41, 0x42, 0x43, 0x44, 0x57, 0x58, 0xaa, 0x46, /* 40 - 47 */
    0x7f, 0xd2, 0xc7, 0xc9, 0xd3, 0xcf, 0xd1, 0xcd, /* 48 - 4f */
    0xcb, 0xd0, 0xc8, 0x45, 0xb5, 0x37, 0x4a, 0x4e, /* 50 - 57 */
    0x9c, 0x4f, 0x50, 0x51, 0x4b, 0x4c, 0x4d, 0x47, /* 58 - 5f */
    0x48, 0x49, 0x52, 0x53, 0x56, 0xdd,   NN, 0x59, /* 60 - 67 */
    0x5d, 0x5e, 0x5f,   NN,   NN,   NN,   NN,   NN, /* 68 - 6f */
      NN,   NN,   NN,   NN, 0x97,   NN, 0x93, 0x95, /* 70 - 77 */
    0x91, 0x92, 0x94, 0x9a, 0x96, 0x98, 0x99,   NN, /* 78 - 7f */
      NN,   NN,   NN,   NN,   NN, 0x7e,   NN, 0x73, /* 80 - 87 */
    0x70, 0x7d, 0x79, 0x7b, 0x5c,   NN,   NN,   NN, /* 88 - 8f */
      NN,   NN, 0x78, 0x77, 0x76,   NN,   NN,   NN, /* 90 - 97 */
      NN,   NN,   NN,   NN,   NN,   NN,   NN,   NN, /* 98 - 9f */
      NN,   NN,   NN,   NN,   NN,   NN,   NN,   NN, /* a0 - a7 */
      NN,   NN,   NN,   NN,   NN,   NN,   NN,   NN, /* a8 - af */
      NN,   NN,   NN,   NN,   NN,   NN,   NN,   NN, /* b0 - b7 */
      NN,   NN,   NN,   NN,   NN,   NN,   NN,   NN, /* b8 - bf */
      NN,   NN,   NN,   NN,   NN,   NN,   NN,   NN, /* c0 - c7 */
      NN,   NN,   NN,   NN,   NN,   NN,   NN,   NN, /* c8 - cf */
      NN,   NN,   NN,   NN,   NN,   NN,   NN,   NN, /* d0 - d7 */
      NN,   NN,   NN,   NN,   NN,   NN,   NN,   NN, /* d8 - df */
    0x1d, 0x2a, 0x38, 0xdb, 0x9d, 0x36, 0xb8, 0xdc, /* e0 - e7 */
      NN,   NN,   NN,   NN,   NN,   NN,   NN,   NN, /* e8 - ef */
      NN,   NN,   NN,   NN,   NN,   NN,   NN,   NN, /* f0 - f7 */
      NN,   NN,   NN,   NN,   NN,   NN,   NN,   NN, /* f8 - ff */
};
#endif /* defined(WSDISPLAY_COMPAT_RAWKBD) */

const kbd_t ukbd_countrylayout[HCC_MAX] = {
=======
const kbd_t ukbd_countrylayout[1 + HCC_MAX] = {
>>>>>>> origin/master
	(kbd_t)-1,
	(kbd_t)-1,	/* arabic */
	KB_BE,		/* belgian */
	(kbd_t)-1,	/* canadian bilingual */
	KB_CF,		/* canadian french */
	(kbd_t)-1,	/* czech */
	KB_DK,		/* danish */
	(kbd_t)-1,	/* finnish */
	KB_FR,		/* french */
	KB_DE,		/* german */
	(kbd_t)-1,	/* greek */
	(kbd_t)-1,	/* hebrew */
	KB_HU,		/* hungary */
	(kbd_t)-1,	/* international (iso) */
	KB_IT,		/* italian */
	KB_JP,		/* japanese (katakana) */
	(kbd_t)-1,	/* korean */
	KB_LA,		/* latin american */
	(kbd_t)-1,	/* netherlands/dutch */
	KB_NO,		/* norwegian */
	(kbd_t)-1,	/* persian (farsi) */
	KB_PL,		/* polish */
	KB_PT,		/* portuguese */
	KB_RU,		/* russian */
	(kbd_t)-1,	/* slovakia */
	KB_ES,		/* spanish */
	KB_SV,		/* swedish */
	KB_SF,		/* swiss french */
	KB_SG,		/* swiss german */
	(kbd_t)-1,	/* switzerland */
	(kbd_t)-1,	/* taiwan */
	KB_TR,		/* turkish Q */
	KB_UK,		/* uk */
	KB_US,		/* us */
	(kbd_t)-1,	/* yugoslavia */
	(kbd_t)-1	/* turkish F */
};

struct ukbd_softc {
<<<<<<< HEAD
	struct uhidev sc_hdev;

	struct ukbd_data sc_ndata;
	struct ukbd_data sc_odata;
	struct hid_location sc_modloc[MAXMOD];
	u_int sc_nmod;
	struct {
		u_int32_t mask;
		u_int8_t key;
	} sc_mods[MAXMOD];

	struct hid_location sc_keycodeloc;
	u_int sc_nkeycode;

	char sc_enabled;

	int sc_console_keyboard;	/* we are the console keyboard */

	char sc_debounce;		/* for quirk handling */
	usb_callout_t sc_delay;		/* for quirk handling */
	struct ukbd_data sc_data;	/* for quirk handling */

	struct hid_location sc_numloc;
	struct hid_location sc_capsloc;
	struct hid_location sc_scroloc;
	int sc_leds;

	usb_callout_t sc_rawrepeat_ch;

	struct device *sc_wskbddev;
#if defined(WSDISPLAY_COMPAT_RAWKBD)
#define REP_DELAY1 400
#define REP_DELAYN 100
	int sc_rawkbd;
	int sc_nrep;
	char sc_rep[MAXKEYS];
#endif /* defined(WSDISPLAY_COMPAT_RAWKBD) */

	int sc_spl;
	int sc_polling;
	int sc_npollchar;
	u_int16_t sc_pollchars[MAXKEYS];

	u_char sc_dying;
};
=======
	struct uhidev		sc_hdev;
	struct hidkbd		sc_kbd;
>>>>>>> origin/master

	int			sc_spl;

	u_char			sc_dying;

<<<<<<< HEAD
Static int	ukbd_is_console;
=======
	void			(*sc_munge)(void *, uint8_t *, u_int);
};
>>>>>>> origin/master

Static void	ukbd_cngetc(void *, u_int *, int *);
Static void	ukbd_cnpollc(void *, int);

const struct wskbd_consops ukbd_consops = {
	ukbd_cngetc,
	ukbd_cnpollc,
};

<<<<<<< HEAD
Static const char *ukbd_parse_desc(struct ukbd_softc *sc);

Static void	ukbd_intr(struct uhidev *addr, void *ibuf, u_int len);
Static void	ukbd_decode(struct ukbd_softc *sc, struct ukbd_data *ud);
Static void	ukbd_delayed_decode(void *addr);

Static int	ukbd_enable(void *, int);
Static void	ukbd_set_leds(void *, int);

Static int	ukbd_ioctl(void *, u_long, caddr_t, int, usb_proc_ptr );
#ifdef WSDISPLAY_COMPAT_RAWKBD
Static void	ukbd_rawrepeat(void *v);
#endif
=======
void	ukbd_intr(struct uhidev *addr, void *ibuf, u_int len);

int	ukbd_enable(void *, int);
void	ukbd_set_leds(void *, int);
int	ukbd_ioctl(void *, u_long, caddr_t, int, struct proc *);
>>>>>>> origin/master

const struct wskbd_accessops ukbd_accessops = {
	ukbd_enable,
	ukbd_set_leds,
	ukbd_ioctl,
};

<<<<<<< HEAD
extern const struct wscons_keydesc ukbd_keydesctab[];

struct wskbd_mapdata ukbd_keymapdata = {
	ukbd_keydesctab
};

USB_DECLARE_DRIVER(ukbd);

USB_MATCH(ukbd)
=======
int ukbd_match(struct device *, void *, void *); 
void ukbd_attach(struct device *, struct device *, void *); 
int ukbd_detach(struct device *, int); 
int ukbd_activate(struct device *, int); 

struct cfdriver ukbd_cd = { 
	NULL, "ukbd", DV_DULL 
}; 

const struct cfattach ukbd_ca = { 
	sizeof(struct ukbd_softc), 
	ukbd_match, 
	ukbd_attach, 
	ukbd_detach, 
	ukbd_activate, 
};

struct ukbd_translation {
	uint8_t original;
	uint8_t translation;
};

#ifdef __loongson__
void	ukbd_gdium_munge(void *, uint8_t *, u_int);
#endif
uint8_t	ukbd_translate(const struct ukbd_translation *, size_t, uint8_t);

int
ukbd_match(struct device *parent, void *match, void *aux)
>>>>>>> origin/master
{
	USB_MATCH_START(ukbd, uaa);
	struct uhidev_attach_arg *uha = (struct uhidev_attach_arg *)uaa;
	int size;
	void *desc;

	uhidev_get_report_desc(uha->parent, &desc, &size);
	if (!hid_is_collection(desc, size, uha->reportid,
	    HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_KEYBOARD)))
		return (UMATCH_NONE);

	return (UMATCH_IFACECLASS);
}

USB_ATTACH(ukbd)
{
<<<<<<< HEAD
	USB_ATTACH_START(ukbd, sc, uaa);
=======
	struct ukbd_softc *sc = (struct ukbd_softc *)self;
	struct hidkbd *kbd = &sc->sc_kbd;
	struct usb_attach_arg *uaa = aux;
>>>>>>> origin/master
	struct uhidev_attach_arg *uha = (struct uhidev_attach_arg *)uaa;
	usb_hid_descriptor_t *hid;
	u_int32_t qflags;
	int dlen, repid;
	void *desc;
	kbd_t layout = (kbd_t)-1;

	sc->sc_hdev.sc_intr = ukbd_intr;
	sc->sc_hdev.sc_parent = uha->parent;
	sc->sc_hdev.sc_report_id = uha->reportid;

<<<<<<< HEAD
	parseerr = ukbd_parse_desc(sc);
	if (parseerr != NULL) {
		printf("\n%s: attach failed, %s\n",
		       sc->sc_hdev.sc_dev.dv_xname, parseerr);
		USB_ATTACH_ERROR_RETURN;
	}

	hid = usbd_get_hid_descriptor(uha->uaa->iface);

#ifdef DIAGNOSTIC
	printf(": %d modifier keys, %d key codes",
	    sc->sc_nmod, sc->sc_nkeycode);
#endif
=======
	uhidev_get_report_desc(uha->parent, &desc, &dlen);
	repid = uha->reportid;
	sc->sc_hdev.sc_isize = hid_report_size(desc, dlen, hid_input, repid);
	sc->sc_hdev.sc_osize = hid_report_size(desc, dlen, hid_output, repid);
	sc->sc_hdev.sc_fsize = hid_report_size(desc, dlen, hid_feature, repid);
>>>>>>> origin/master

	qflags = usbd_get_quirks(uha->parent->sc_udev)->uq_flags;
	if (hidkbd_attach(self, kbd, 1, qflags, repid, desc, dlen) != 0)
		return;

	if (uha->uaa->vendor == USB_VENDOR_TOPRE &&
	    uha->uaa->product == USB_PRODUCT_TOPRE_HHKB) {
		/* ignore country code on purpose */
	} else {
		hid = usbd_get_hid_descriptor(uha->uaa->iface);

		if (hid->bCountryCode <= HCC_MAX)
			layout = ukbd_countrylayout[hid->bCountryCode];
#ifdef DIAGNOSTIC
		if (hid->bCountryCode != 0)
			printf(", country code %d", hid->bCountryCode);
#endif
	}
	if (layout == (kbd_t)-1) {
#ifdef UKBD_LAYOUT
		layout = UKBD_LAYOUT;
#else
		layout = KB_US;
#endif
	}

	printf("\n");

#ifdef __loongson__
	if (uha->uaa->vendor == USB_VENDOR_CYPRESS &&
	    uha->uaa->product == USB_PRODUCT_CYPRESS_LPRDK)
		sc->sc_munge = ukbd_gdium_munge;
#endif

	if (kbd->sc_console_keyboard) {
		extern struct wskbd_mapdata ukbd_keymapdata;

		DPRINTF(("ukbd_attach: console keyboard sc=%p\n", sc));
		ukbd_keymapdata.layout = layout;
		wskbd_cnattach(&ukbd_consops, sc, &ukbd_keymapdata);
		ukbd_enable(sc, 1);
	}

<<<<<<< HEAD
	a.console = sc->sc_console_keyboard;

	a.keymap = &ukbd_keymapdata;

	a.accessops = &ukbd_accessops;
	a.accesscookie = sc;

	usb_callout_init(sc->sc_rawrepeat_ch);
	usb_callout_init(sc->sc_delay);

=======
>>>>>>> origin/master
	/* Flash the leds; no real purpose, just shows we're alive. */
	ukbd_set_leds(sc, WSKBD_LED_SCROLL | WSKBD_LED_NUM | WSKBD_LED_CAPS);
	usbd_delay_ms(uha->parent->sc_udev, 400);
	ukbd_set_leds(sc, 0);

<<<<<<< HEAD
	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);

	USB_ATTACH_SUCCESS_RETURN;
}

int
ukbd_enable(void *v, int on)
{
	struct ukbd_softc *sc = v;

	if (on && sc->sc_dying)
		return (EIO);

	/* Should only be called to change state */
	if (sc->sc_enabled == on) {
		DPRINTF(("ukbd_enable: %s: bad call on=%d\n",
			 USBDEVNAME(sc->sc_hdev.sc_dev), on));
		return (EBUSY);
	}

	DPRINTF(("ukbd_enable: sc=%p on=%d\n", sc, on));
	sc->sc_enabled = on;
	if (on) {
		return (uhidev_open(&sc->sc_hdev));
	} else {
		uhidev_close(&sc->sc_hdev);
		return (0);
	}
}

int
ukbd_activate(device_ptr_t self, enum devact act)
=======
	hidkbd_attach_wskbd(kbd, layout, &ukbd_accessops);
}

int
ukbd_activate(struct device *self, int act)
>>>>>>> origin/master
{
	struct ukbd_softc *sc = (struct ukbd_softc *)self;
	struct hidkbd *kbd = &sc->sc_kbd;
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		break;

	case DVACT_DEACTIVATE:
		if (kbd->sc_wskbddev != NULL)
			rv = config_deactivate(kbd->sc_wskbddev);
		sc->sc_dying = 1;
		break;
	}
	return (rv);
}

USB_DETACH(ukbd)
{
<<<<<<< HEAD
	USB_DETACH_START(ukbd, sc);
	int rv = 0;

	DPRINTF(("ukbd_detach: sc=%p flags=%d\n", sc, flags));

	if (sc->sc_console_keyboard) {
#if 0
		/*
		 * XXX Should probably disconnect our consops,
		 * XXX and either notify some other keyboard that
		 * XXX it can now be the console, or if there aren't
		 * XXX any more USB keyboards, set ukbd_is_console
		 * XXX back to 1 so that the next USB keyboard attached
		 * XXX to the system will get it.
		 */
		panic("ukbd_detach: console keyboard");
#else
		/*
		 * Disconnect our consops and set ukbd_is_console
		 * back to 1 so that the next USB keyboard attached
		 * to the system will get it.
		 * XXX Should notify some other keyboard that it can be
		 * XXX console, if there are any other keyboards.
		 */
		printf("%s: was console keyboard\n",
		       USBDEVNAME(sc->sc_hdev.sc_dev));
		wskbd_cndetach();
		ukbd_is_console = 1;
#endif
	}
	/* No need to do reference counting of ukbd, wskbd has all the goo. */
	if (sc->sc_wskbddev != NULL)
		rv = config_detach(sc->sc_wskbddev, flags);
=======
	struct ukbd_softc *sc = (struct ukbd_softc *)self;
	struct hidkbd *kbd = &sc->sc_kbd;
	int rv;

	rv = hidkbd_detach(kbd, flags);
>>>>>>> origin/master

	/* The console keyboard does not get a disable call, so check pipe. */
	if (sc->sc_hdev.sc_state & UHIDEV_OPEN)
		uhidev_close(&sc->sc_hdev);

	return (rv);
}

void
ukbd_intr(struct uhidev *addr, void *ibuf, u_int len)
{
	struct ukbd_softc *sc = (struct ukbd_softc *)addr;
	struct hidkbd *kbd = &sc->sc_kbd;

<<<<<<< HEAD
	ud->modifiers = 0;
	for (i = 0; i < sc->sc_nmod; i++)
		if (hid_get_data(ibuf, &sc->sc_modloc[i]))
			ud->modifiers |= sc->sc_mods[i].mask;
	memcpy(ud->keycode, (char *)ibuf + sc->sc_keycodeloc.pos / 8,
	       sc->sc_nkeycode);

	if (sc->sc_debounce && !sc->sc_polling) {
		/*
		 * Some keyboards have a peculiar quirk.  They sometimes
		 * generate a key up followed by a key down for the same
		 * key after about 10 ms.
		 * We avoid this bug by holding off decoding for 20 ms.
		 */
		sc->sc_data = *ud;
		usb_callout(sc->sc_delay, hz / 50, ukbd_delayed_decode, sc);
#ifdef DDB
	} else if (sc->sc_console_keyboard && !sc->sc_polling) {
		/*
		 * For the console keyboard we can't deliver CTL-ALT-ESC
		 * from the interrupt routine.  Doing so would start
		 * polling from inside the interrupt routine and that
		 * loses bigtime.
		 */
		sc->sc_data = *ud;
		usb_callout(sc->sc_delay, 1, ukbd_delayed_decode, sc);
#endif
	} else {
		ukbd_decode(sc, ud);
=======
	if (kbd->sc_enabled != 0) {
		if (sc->sc_munge != NULL)
			(*sc->sc_munge)(sc, (uint8_t *)ibuf, len);
		hidkbd_input(kbd, (uint8_t *)ibuf, len);
>>>>>>> origin/master
	}
}

int
ukbd_enable(void *v, int on)
{
	struct ukbd_softc *sc = v;
	struct hidkbd *kbd = &sc->sc_kbd;
	int rv;

	if (on && sc->sc_dying)
		return EIO;

<<<<<<< HEAD
	if (sc->sc_polling) {
		DPRINTFN(1,("ukbd_intr: pollchar = 0x%03x\n", ibuf[0]));
		memcpy(sc->sc_pollchars, ibuf, nkeys * sizeof(u_int16_t));
		sc->sc_npollchar = nkeys;
		return;
	}
#ifdef WSDISPLAY_COMPAT_RAWKBD
	if (sc->sc_rawkbd) {
		u_char cbuf[MAXKEYS * 2];
		int c;
		int npress;

		for (npress = i = j = 0; i < nkeys; i++) {
			key = ibuf[i];
			c = ukbd_trtab[key & CODEMASK];
			if (c == NN)
				continue;
			if (c & 0x80)
				cbuf[j++] = 0xe0;
			cbuf[j] = c & 0x7f;
			if (key & RELEASE)
				cbuf[j] |= 0x80;
			else {
				/* remember pressed keys for autorepeat */
				if (c & 0x80)
					sc->sc_rep[npress++] = 0xe0;
				sc->sc_rep[npress++] = c & 0x7f;
			}
			DPRINTFN(1,("ukbd_intr: raw = %s0x%02x\n",
				    c & 0x80 ? "0xe0 " : "",
				    cbuf[j]));
			j++;
		}
		s = spltty();
		wskbd_rawinput(sc->sc_wskbddev, cbuf, j);
		splx(s);
		usb_uncallout(sc->sc_rawrepeat_ch, ukbd_rawrepeat, sc);
		if (npress != 0) {
			sc->sc_nrep = npress;
			usb_callout(sc->sc_rawrepeat_ch,
			    hz * REP_DELAY1 / 1000, ukbd_rawrepeat, sc);
		}
		return;
	}
#endif
=======
	if ((rv = hidkbd_enable(kbd, on)) != 0)
		return rv;
>>>>>>> origin/master

	if (on) {
		return uhidev_open(&sc->sc_hdev);
	} else {
		uhidev_close(&sc->sc_hdev);
		return 0;
	}
}

void
ukbd_set_leds(void *v, int leds)
{
	struct ukbd_softc *sc = v;
	struct hidkbd *kbd = &sc->sc_kbd;
	u_int8_t res;

	if (sc->sc_dying)
		return;

<<<<<<< HEAD
	if (sc->sc_leds == leds)
		return;
	sc->sc_leds = leds;
	res = 0;
	/* XXX not really right */
	if ((leds & WSKBD_LED_SCROLL) && sc->sc_scroloc.size == 1)
		res |= 1 << sc->sc_scroloc.pos;
	if ((leds & WSKBD_LED_NUM) && sc->sc_numloc.size == 1)
		res |= 1 << sc->sc_numloc.pos;
	if ((leds & WSKBD_LED_CAPS) && sc->sc_capsloc.size == 1)
		res |= 1 << sc->sc_capsloc.pos;
	uhidev_set_report_async(&sc->sc_hdev, UHID_OUTPUT_REPORT, &res, 1);
}

#ifdef WSDISPLAY_COMPAT_RAWKBD
void
ukbd_rawrepeat(void *v)
{
	struct ukbd_softc *sc = v;
	int s;

	s = spltty();
	wskbd_rawinput(sc->sc_wskbddev, sc->sc_rep, sc->sc_nrep);
	splx(s);
	usb_callout(sc->sc_rawrepeat_ch, hz * REP_DELAYN / 1000,
	    ukbd_rawrepeat, sc);
=======
	if (hidkbd_set_leds(kbd, leds, &res) != 0)
		uhidev_set_report_async(&sc->sc_hdev, UHID_OUTPUT_REPORT,
		    &res, 1);
>>>>>>> origin/master
}

int
ukbd_ioctl(void *v, u_long cmd, caddr_t data, int flag, usb_proc_ptr p)
{
	struct ukbd_softc *sc = v;
	struct hidkbd *kbd = &sc->sc_kbd;
	int rc;

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_USB;
		return (0);
	case WSKBDIO_SETLEDS:
		ukbd_set_leds(v, *(int *)data);
		return (0);
<<<<<<< HEAD
	case WSKBDIO_GETLEDS:
		*(int *)data = sc->sc_leds;
		return (0);
#ifdef WSDISPLAY_COMPAT_RAWKBD
	case WSKBDIO_SETMODE:
		DPRINTF(("ukbd_ioctl: set raw = %d\n", *(int *)data));
		sc->sc_rawkbd = *(int *)data == WSKBD_RAW;
		usb_uncallout(sc->sc_rawrepeat_ch, ukbd_rawrepeat, sc);
		return (0);
#endif
	}
	return (-1);
}

/*
 * This is a hack to work around some broken ports that don't call
 * cnpollc() before cngetc().
 */
static int pollenter, warned;

=======
	default:
		rc = uhidev_ioctl(&sc->sc_hdev, cmd, data, flag, p);
		if (rc != -1)
			return rc;
		else
			return hidkbd_ioctl(kbd, cmd, data, flag, p);
	}
}

>>>>>>> origin/master
/* Console interface. */
void
ukbd_cngetc(void *v, u_int *type, int *data)
{
	struct ukbd_softc *sc = v;
	struct hidkbd *kbd = &sc->sc_kbd;

	DPRINTFN(0,("ukbd_cngetc: enter\n"));
	kbd->sc_polling = 1;
	while (kbd->sc_npollchar <= 0)
		usbd_dopoll(sc->sc_hdev.sc_parent->sc_iface);
	kbd->sc_polling = 0;
	hidkbd_cngetc(kbd, type, data);
	DPRINTFN(0,("ukbd_cngetc: return 0x%02x\n", *data));
}

void
ukbd_cnpollc(void *v, int on)
{
	struct ukbd_softc *sc = v;
	usbd_device_handle dev;

	DPRINTFN(2,("ukbd_cnpollc: sc=%p on=%d\n", v, on));

	usbd_interface2device_handle(sc->sc_hdev.sc_parent->sc_iface, &dev);
	if (on)
		sc->sc_spl = splusb();
	else
		splx(sc->sc_spl);
	usbd_set_polling(dev, on);
}

<<<<<<< HEAD
=======
void
ukbd_cnbell(void *v, u_int pitch, u_int period, u_int volume) 
{
	hidkbd_bell(pitch, period, volume, 1);
}	

>>>>>>> origin/master
int
ukbd_cnattach(void)
{

	/*
	 * XXX USB requires too many parts of the kernel to be running
	 * XXX in order to work, so we can't do much for the console
	 * XXX keyboard until autconfiguration has run its course.
	 */
	hidkbd_is_console = 1;
	return (0);
}

uint8_t
ukbd_translate(const struct ukbd_translation *table, size_t tsize,
    uint8_t keycode)
{
	for (; tsize != 0; table++, tsize--)
		if (table->original == keycode)
			return table->translation;
	return 0;
}

#ifdef __loongson__
/*
 * Software Fn- translation for Gdium Liberty keyboard.
 */
#define	GDIUM_FN_CODE	0x82
void
ukbd_gdium_munge(void *vsc, uint8_t *ibuf, u_int ilen)
{
	struct ukbd_softc *sc = vsc;
	struct hidkbd *kbd = &sc->sc_kbd;
	uint8_t *pos, *spos, *epos, xlat;
	int fn;

	static const struct ukbd_translation gdium_fn_trans[] = {
#ifdef notyet
		{ 58, 0 },	/* F1 -> toggle camera */
		{ 59, 0 },	/* F2 -> toggle wireless */
#endif
		{ 60, 127 },	/* F3 -> audio mute */
		{ 61, 128 },	/* F4 -> audio raise */
		{ 62, 129 },	/* F5 -> audio lower */
#ifdef notyet
		{ 63, 0 },	/* F6 -> toggle ext. video */
		{ 64, 0 },	/* F7 -> toggle mouse */
		{ 65, 0 },	/* F8 -> brightness up */
		{ 66, 0 },	/* F9 -> brightness down */
		{ 67, 0 },	/* F10 -> suspend */
		{ 68, 0 },	/* F11 -> user1 */
		{ 69, 0 },	/* F12 -> user2 */
		{ 70, 0 },	/* print screen -> sysrq */
#endif
		{ 76, 71 },	/* delete -> scroll lock */
		{ 81, 78 },	/* down -> page down */
		{ 82, 75 }	/* up -> page up */
	};

	spos = ibuf + kbd->sc_keycodeloc.pos / 8;
	epos = spos + kbd->sc_nkeycode;

	/*
	 * Check for Fn key being down and remove it from the report.
	 */

	fn = 0;
	for (pos = spos; pos != epos; pos++)
		if (*pos == GDIUM_FN_CODE) {
			fn = 1;
			*pos = 0;
			break;
		}

	/*
	 * Rewrite keycodes on the fly to perform Fn-key translation.
	 * Keycodes without a translation are passed unaffected.
	 */

	if (fn != 0)
		for (pos = spos; pos != epos; pos++) {
			xlat = ukbd_translate(gdium_fn_trans,
			    nitems(gdium_fn_trans), *pos);
			if (xlat != 0)
				*pos = xlat;
		}

}
#endif
