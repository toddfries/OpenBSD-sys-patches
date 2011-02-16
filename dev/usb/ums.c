<<<<<<< HEAD
/*	$OpenBSD: ums.c,v 1.17 2006/06/23 06:27:12 miod Exp $ */
=======
/*	$OpenBSD: ums.c,v 1.33 2010/08/02 23:17:34 miod Exp $ */
>>>>>>> origin/master
/*	$NetBSD: ums.c,v 1.60 2003/03/11 16:44:00 augustss Exp $	*/

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
#include <sys/kernel.h>
#include <sys/malloc.h>
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

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

<<<<<<< HEAD
#ifdef USB_DEBUG
#define DPRINTF(x)	do { if (umsdebug) logprintf x; } while (0)
#define DPRINTFN(n,x)	do { if (umsdebug>(n)) logprintf x; } while (0)
int	umsdebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define UMS_BUT(i) ((i) == 1 || (i) == 2 ? 3 - (i) : i)

#define UMSUNIT(s)	(minor(s))

#define PS2LBUTMASK	x01
#define PS2RBUTMASK	x02
#define PS2MBUTMASK	x04
#define PS2BUTMASK 0x0f

#define MAX_BUTTONS	16	/* must not exceed size of sc_buttons */

struct ums_softc {
	struct uhidev sc_hdev;

	struct hid_location sc_loc_x, sc_loc_y, sc_loc_z, sc_loc_w;
	struct hid_location sc_loc_btn[MAX_BUTTONS];

	int sc_enabled;

	int flags;		/* device configuration */
#define UMS_Z		0x01	/* z direction available */
#define UMS_SPUR_BUT_UP	0x02	/* spurious button up events */
#define UMS_REVZ	0x04	/* Z-axis is reversed */

	int nbuttons;

	u_int32_t sc_buttons;	/* mouse button status */
	struct device *sc_wsmousedev;

	char			sc_dying;
};

#define MOUSE_FLAGS_MASK (HIO_CONST|HIO_RELATIVE)
#define MOUSE_FLAGS (HIO_RELATIVE)

Static void ums_intr(struct uhidev *addr, void *ibuf, u_int len);
=======
#include <dev/usb/hidmsvar.h>

struct ums_softc {
	struct uhidev	sc_hdev;
	struct hidms	sc_ms;
	char		sc_dying;
};

void ums_intr(struct uhidev *addr, void *ibuf, u_int len);
>>>>>>> origin/master

Static int	ums_enable(void *);
Static void	ums_disable(void *);
Static int	ums_ioctl(void *, u_long, caddr_t, int, usb_proc_ptr);

const struct wsmouse_accessops ums_accessops = {
	ums_enable,
	ums_ioctl,
	ums_disable,
};

<<<<<<< HEAD
USB_DECLARE_DRIVER(ums);
=======
int ums_match(struct device *, void *, void *); 
void ums_attach(struct device *, struct device *, void *); 
int ums_detach(struct device *, int); 
int ums_activate(struct device *, int); 

struct cfdriver ums_cd = { 
	NULL, "ums", DV_DULL 
}; 

const struct cfattach ums_ca = { 
	sizeof(struct ums_softc), 
	ums_match, 
	ums_attach, 
	ums_detach, 
	ums_activate, 
};
>>>>>>> origin/master

USB_MATCH(ums)
{
	USB_MATCH_START(ums, uaa);
	struct uhidev_attach_arg *uha = (struct uhidev_attach_arg *)uaa;
	int size;
	void *desc;

	uhidev_get_report_desc(uha->parent, &desc, &size);
	if (!hid_is_collection(desc, size, uha->reportid,
			       HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_MOUSE)))
		return (UMATCH_NONE);

	return (UMATCH_IFACECLASS);
}

USB_ATTACH(ums)
{
<<<<<<< HEAD
	USB_ATTACH_START(ums, sc, uaa);
=======
	struct ums_softc *sc = (struct ums_softc *)self;
	struct hidms *ms = &sc->sc_ms;
	struct usb_attach_arg *uaa = aux;
>>>>>>> origin/master
	struct uhidev_attach_arg *uha = (struct uhidev_attach_arg *)uaa;
	int size, repid;
	void *desc;
<<<<<<< HEAD
	u_int32_t flags, quirks;
	int i;
	struct hid_location loc_btn;
=======
	u_int32_t quirks;
>>>>>>> origin/master

	sc->sc_hdev.sc_intr = ums_intr;
	sc->sc_hdev.sc_parent = uha->parent;
	sc->sc_hdev.sc_report_id = uha->reportid;

	quirks = usbd_get_quirks(uha->parent->sc_udev)->uq_flags;
<<<<<<< HEAD
	if (quirks & UQ_MS_REVZ)
		sc->flags |= UMS_REVZ;
	if (quirks & UQ_SPUR_BUT_UP)
		sc->flags |= UMS_SPUR_BUT_UP;

=======
>>>>>>> origin/master
	uhidev_get_report_desc(uha->parent, &desc, &size);
	repid = uha->reportid;
	sc->sc_hdev.sc_isize = hid_report_size(desc, size, hid_input, repid);
	sc->sc_hdev.sc_osize = hid_report_size(desc, size, hid_output, repid);
	sc->sc_hdev.sc_fsize = hid_report_size(desc, size, hid_feature, repid);

<<<<<<< HEAD
	if (!hid_locate(desc, size, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_X),
	       uha->reportid, hid_input, &sc->sc_loc_x, &flags)) {
		printf("\n%s: mouse has no X report\n",
		       USBDEVNAME(sc->sc_hdev.sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}
	if ((flags & MOUSE_FLAGS_MASK) != MOUSE_FLAGS) {
		printf("\n%s: X report 0x%04x not supported\n",
		       USBDEVNAME(sc->sc_hdev.sc_dev), flags);
		USB_ATTACH_ERROR_RETURN;
	}

	if (!hid_locate(desc, size, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_Y),
	       uha->reportid, hid_input, &sc->sc_loc_y, &flags)) {
		printf("\n%s: mouse has no Y report\n",
		       USBDEVNAME(sc->sc_hdev.sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}
	if ((flags & MOUSE_FLAGS_MASK) != MOUSE_FLAGS) {
		printf("\n%s: Y report 0x%04x not supported\n",
		       USBDEVNAME(sc->sc_hdev.sc_dev), flags);
		USB_ATTACH_ERROR_RETURN;
	}

	/* Try the wheel as Z activator first */
	if (hid_locate(desc, size, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_WHEEL),
	    uha->reportid, hid_input, &sc->sc_loc_z, &flags)) {
		if ((flags & MOUSE_FLAGS_MASK) != MOUSE_FLAGS) {
			DPRINTF(("\n%s: Wheel report 0x%04x not supported\n",
				USBDEVNAME(sc->sc_hdev.sc_dev), flags));
			sc->sc_loc_z.size = 0; /* Bad Z coord, ignore it */
		} else {
			sc->flags |= UMS_Z;
			/* Wheels need the Z axis reversed. */
			sc->flags ^= UMS_REVZ;
		}
		/*
		 * We might have both a wheel and Z direction; in this case,
		 * report the Z direction on the W axis.
		*/
		if (hid_locate(desc, size,
		    HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_Z),
		    uha->reportid, hid_input, &sc->sc_loc_w, &flags)) {
			if ((flags & MOUSE_FLAGS_MASK) != MOUSE_FLAGS) {
				DPRINTF(("\n%s: Z report 0x%04x not supported\n",
					USBDEVNAME(sc->sc_hdev.sc_dev), flags));
				/* Bad Z coord, ignore it */
				sc->sc_loc_w.size = 0;
			}
		}
	} else if (hid_locate(desc, size,
	    HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_Z),
	    uha->reportid, hid_input, &sc->sc_loc_z, &flags)) {
		if ((flags & MOUSE_FLAGS_MASK) != MOUSE_FLAGS) {
			DPRINTF(("\n%s: Z report 0x%04x not supported\n",
				USBDEVNAME(sc->sc_hdev.sc_dev), flags));
			sc->sc_loc_z.size = 0; /* Bad Z coord, ignore it */
		} else {
			sc->flags |= UMS_Z;
		}
	}

	/* figure out the number of buttons */
	for (i = 1; i <= MAX_BUTTONS; i++)
		if (!hid_locate(desc, size, HID_USAGE2(HUP_BUTTON, i),
			uha->reportid, hid_input, &loc_btn, 0))
			break;
	sc->nbuttons = i - 1;

	printf(": %d button%s%s\n",
	    sc->nbuttons, sc->nbuttons == 1 ? "" : "s",
	    sc->flags & UMS_Z ? " and Z dir." : "");

	for (i = 1; i <= sc->nbuttons; i++)
		hid_locate(desc, size, HID_USAGE2(HUP_BUTTON, i),
			   uha->reportid, hid_input,
			   &sc->sc_loc_btn[i-1], 0);

#ifdef USB_DEBUG
	DPRINTF(("ums_attach: sc=%p\n", sc));
	DPRINTF(("ums_attach: X\t%d/%d\n",
		 sc->sc_loc_x.pos, sc->sc_loc_x.size));
	DPRINTF(("ums_attach: Y\t%d/%d\n",
		 sc->sc_loc_y.pos, sc->sc_loc_y.size));
	if (sc->flags & UMS_Z)
		DPRINTF(("ums_attach: Z\t%d/%d\n",
			 sc->sc_loc_z.pos, sc->sc_loc_z.size));
	for (i = 1; i <= sc->nbuttons; i++) {
		DPRINTF(("ums_attach: B%d\t%d/%d\n",
			 i, sc->sc_loc_btn[i-1].pos,sc->sc_loc_btn[i-1].size));
	}
#endif

	a.accessops = &ums_accessops;
	a.accesscookie = sc;

	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint);

	USB_ATTACH_SUCCESS_RETURN;
}

int
ums_activate(device_ptr_t self, enum devact act)
=======
	if (hidms_setup(self, ms, quirks, uha->reportid, desc, size) != 0)
		return;

	/*
	 * The Microsoft Wireless Notebook Optical Mouse 3000 Model 1049 has
	 * five Report IDs: 19, 23, 24, 17, 18 (in the order they appear in
	 * report descriptor), it seems that report 17 contains the necessary
	 * mouse information (3-buttons, X, Y, wheel) so we specify it
	 * manually.
	 */
	if (uaa->vendor == USB_VENDOR_MICROSOFT &&
	    uaa->product == USB_PRODUCT_MICROSOFT_WLNOTEBOOK3) {
		ms->sc_flags = HIDMS_Z;
		ms->sc_num_buttons = 3;
		/* XXX change sc_hdev isize to 5? */
		ms->sc_loc_x.pos = 8;
		ms->sc_loc_y.pos = 16;
		ms->sc_loc_z.pos = 24;
		ms->sc_loc_btn[0].pos = 0;
		ms->sc_loc_btn[1].pos = 1;
		ms->sc_loc_btn[2].pos = 2;
	}

	hidms_attach(ms, &ums_accessops);
}

int
ums_activate(struct device *self, int act)
>>>>>>> origin/master
{
	struct ums_softc *sc = (struct ums_softc *)self;
	struct hidms *ms = &sc->sc_ms;
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		break;

	case DVACT_DEACTIVATE:
		if (ms->sc_wsmousedev != NULL)
			rv = config_deactivate(ms->sc_wsmousedev);
		sc->sc_dying = 1;
		break;
	}
	return (rv);
}

USB_DETACH(ums)
{
<<<<<<< HEAD
	USB_DETACH_START(ums, sc);
	int rv = 0;
=======
	struct ums_softc *sc = (struct ums_softc *)self;
	struct hidms *ms = &sc->sc_ms;
>>>>>>> origin/master

	return hidms_detach(ms, flags);
}

void
ums_intr(struct uhidev *addr, void *ibuf, u_int len)
{
	struct ums_softc *sc = (struct ums_softc *)addr;
<<<<<<< HEAD
	int dx, dy, dz, dw;
	u_int32_t buttons = 0;
	int i;
	int s;

	DPRINTFN(5,("ums_intr: len=%d\n", len));

	dx =  hid_get_data(ibuf, &sc->sc_loc_x);
	dy = -hid_get_data(ibuf, &sc->sc_loc_y);
	dz =  hid_get_data(ibuf, &sc->sc_loc_z);
	dw =  hid_get_data(ibuf, &sc->sc_loc_w);
	if (sc->flags & UMS_REVZ)
		dz = -dz;
	for (i = 0; i < sc->nbuttons; i++)
		if (hid_get_data(ibuf, &sc->sc_loc_btn[i]))
			buttons |= (1 << UMS_BUT(i));

	if (dx != 0 || dy != 0 || dz != 0 || dw != 0 ||
	    buttons != sc->sc_buttons) {
		DPRINTFN(10, ("ums_intr: x:%d y:%d z:%d w:%d buttons:0x%x\n",
			dx, dy, dz, dw, buttons));
		sc->sc_buttons = buttons;
		if (sc->sc_wsmousedev != NULL) {
			s = spltty();
			wsmouse_input(sc->sc_wsmousedev, buttons,
			    dx, dy, dz, dw, WSMOUSE_INPUT_DELTA);
			splx(s);
		}
	}
=======
	struct hidms *ms = &sc->sc_ms;

	if (ms->sc_enabled != 0)
		hidms_input(ms, (uint8_t *)buf, len);
>>>>>>> origin/master
}

Static int
ums_enable(void *v)
{
	struct ums_softc *sc = v;
	struct hidms *ms = &sc->sc_ms;
	int rv;

	if (sc->sc_dying)
		return EIO;

	if ((rv = hidms_enable(ms)) != 0)
		return rv;

	return uhidev_open(&sc->sc_hdev);
}

Static void
ums_disable(void *v)
{
	struct ums_softc *sc = v;
	struct hidms *ms = &sc->sc_ms;

	hidms_disable(ms);
	uhidev_close(&sc->sc_hdev);
}

<<<<<<< HEAD
Static int
ums_ioctl(void *v, u_long cmd, caddr_t data, int flag, usb_proc_ptr p)

=======
int
ums_ioctl(void *v, u_long cmd, caddr_t data, int flag, struct proc *p)
>>>>>>> origin/master
{
	struct ums_softc *sc = v;
	struct hidms *ms = &sc->sc_ms;
	int rc;

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_USB;
		return 0;
	default:
		rc = uhidev_ioctl(&sc->sc_hdev, cmd, data, flag, p);
		if (rc != -1)
			return rc;
		else
			return hidms_ioctl(ms, cmd, data, flag, p);
	}
}
