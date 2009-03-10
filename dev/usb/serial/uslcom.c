/*	$OpenBSD: uslcom.c,v 1.17 2007/11/24 10:52:12 jsg Exp $	*/

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/dev/usb/serial/uslcom.c,v 1.3 2009/03/02 05:37:05 thompsa Exp $");

/*
 * Copyright (c) 2006 Jonathan Gray <jsg@openbsd.org>
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

#include "usbdevs.h"
#include <dev/usb/usb.h>
#include <dev/usb/usb_mfunc.h>
#include <dev/usb/usb_error.h>

#define	USB_DEBUG_VAR uslcom_debug

#include <dev/usb/usb_core.h>
#include <dev/usb/usb_debug.h>
#include <dev/usb/usb_process.h>
#include <dev/usb/usb_request.h>
#include <dev/usb/usb_lookup.h>
#include <dev/usb/usb_util.h>
#include <dev/usb/usb_busdma.h>

#include <dev/usb/serial/usb_serial.h>

#if USB_DEBUG
static int uslcom_debug = 0;

SYSCTL_NODE(_hw_usb2, OID_AUTO, uslcom, CTLFLAG_RW, 0, "USB uslcom");
SYSCTL_INT(_hw_usb2_uslcom, OID_AUTO, debug, CTLFLAG_RW,
    &uslcom_debug, 0, "Debug level");
#endif

#define	USLCOM_BULK_BUF_SIZE		1024
#define	USLCOM_CONFIG_INDEX	0
#define	USLCOM_IFACE_INDEX	0

#define	USLCOM_SET_DATA_BITS(x)	((x) << 8)

#define	USLCOM_WRITE		0x41
#define	USLCOM_READ		0xc1

#define	USLCOM_UART		0x00
#define	USLCOM_BAUD_RATE	0x01	
#define	USLCOM_DATA		0x03
#define	USLCOM_BREAK		0x05
#define	USLCOM_CTRL		0x07

#define	USLCOM_UART_DISABLE	0x00
#define	USLCOM_UART_ENABLE	0x01

#define	USLCOM_CTRL_DTR_ON	0x0001	
#define	USLCOM_CTRL_DTR_SET	0x0100
#define	USLCOM_CTRL_RTS_ON	0x0002
#define	USLCOM_CTRL_RTS_SET	0x0200
#define	USLCOM_CTRL_CTS		0x0010
#define	USLCOM_CTRL_DSR		0x0020
#define	USLCOM_CTRL_DCD		0x0080

#define	USLCOM_BAUD_REF		0x384000

#define	USLCOM_STOP_BITS_1	0x00
#define	USLCOM_STOP_BITS_2	0x02

#define	USLCOM_PARITY_NONE	0x00
#define	USLCOM_PARITY_ODD	0x10
#define	USLCOM_PARITY_EVEN	0x20

#define	USLCOM_PORT_NO		0xFFFF /* XXX think this should be 0 --hps */

#define	USLCOM_BREAK_OFF	0x00
#define	USLCOM_BREAK_ON		0x01

enum {
	USLCOM_BULK_DT_WR,
	USLCOM_BULK_DT_RD,
	USLCOM_N_TRANSFER,
};

struct uslcom_softc {
	struct usb2_com_super_softc sc_super_ucom;
	struct usb2_com_softc sc_ucom;

	struct usb2_xfer *sc_xfer[USLCOM_N_TRANSFER];
	struct usb2_device *sc_udev;
	struct mtx sc_mtx;

	uint8_t		 sc_msr;
	uint8_t		 sc_lsr;
};

static device_probe_t uslcom_probe;
static device_attach_t uslcom_attach;
static device_detach_t uslcom_detach;

static usb2_callback_t uslcom_write_callback;
static usb2_callback_t uslcom_read_callback;

static void uslcom_open(struct usb2_com_softc *);
static void uslcom_close(struct usb2_com_softc *);
static void uslcom_set_dtr(struct usb2_com_softc *, uint8_t);
static void uslcom_set_rts(struct usb2_com_softc *, uint8_t);
static void uslcom_set_break(struct usb2_com_softc *, uint8_t);
static int uslcom_pre_param(struct usb2_com_softc *, struct termios *);
static void uslcom_param(struct usb2_com_softc *, struct termios *);
static void uslcom_get_status(struct usb2_com_softc *, uint8_t *, uint8_t *);
static void uslcom_start_read(struct usb2_com_softc *);
static void uslcom_stop_read(struct usb2_com_softc *);
static void uslcom_start_write(struct usb2_com_softc *);
static void uslcom_stop_write(struct usb2_com_softc *);

static const struct usb2_config uslcom_config[USLCOM_N_TRANSFER] = {

	[USLCOM_BULK_DT_WR] = {
		.type = UE_BULK,
		.endpoint = UE_ADDR_ANY,
		.direction = UE_DIR_OUT,
		.mh.bufsize = USLCOM_BULK_BUF_SIZE,
		.mh.flags = {.pipe_bof = 1,.force_short_xfer = 1,},
		.mh.callback = &uslcom_write_callback,
	},

	[USLCOM_BULK_DT_RD] = {
		.type = UE_BULK,
		.endpoint = UE_ADDR_ANY,
		.direction = UE_DIR_IN,
		.mh.bufsize = USLCOM_BULK_BUF_SIZE,
		.mh.flags = {.pipe_bof = 1,.short_xfer_ok = 1,},
		.mh.callback = &uslcom_read_callback,
	},
};

struct usb2_com_callback uslcom_callback = {
	.usb2_com_cfg_open = &uslcom_open,
	.usb2_com_cfg_close = &uslcom_close,
	.usb2_com_cfg_get_status = &uslcom_get_status,
	.usb2_com_cfg_set_dtr = &uslcom_set_dtr,
	.usb2_com_cfg_set_rts = &uslcom_set_rts,
	.usb2_com_cfg_set_break = &uslcom_set_break,
	.usb2_com_cfg_param = &uslcom_param,
	.usb2_com_pre_param = &uslcom_pre_param,
	.usb2_com_start_read = &uslcom_start_read,
	.usb2_com_stop_read = &uslcom_stop_read,
	.usb2_com_start_write = &uslcom_start_write,
	.usb2_com_stop_write = &uslcom_stop_write,
};

static const struct usb2_device_id uslcom_devs[] = {
    { USB_VPI(USB_VENDOR_BALTECH,	USB_PRODUCT_BALTECH_CARDREADER, 0) },
    { USB_VPI(USB_VENDOR_DYNASTREAM,	USB_PRODUCT_DYNASTREAM_ANTDEVBOARD, 0) },
    { USB_VPI(USB_VENDOR_JABLOTRON,	USB_PRODUCT_JABLOTRON_PC60B, 0) },
    { USB_VPI(USB_VENDOR_SILABS,	USB_PRODUCT_SILABS_ARGUSISP, 0) },
    { USB_VPI(USB_VENDOR_SILABS,	USB_PRODUCT_SILABS_CRUMB128, 0) },
    { USB_VPI(USB_VENDOR_SILABS,	USB_PRODUCT_SILABS_DEGREE, 0) },
    { USB_VPI(USB_VENDOR_SILABS,	USB_PRODUCT_SILABS_BURNSIDE, 0) },
    { USB_VPI(USB_VENDOR_SILABS,	USB_PRODUCT_SILABS_HELICOM, 0) },
    { USB_VPI(USB_VENDOR_SILABS,	USB_PRODUCT_SILABS_LIPOWSKY_HARP, 0) },
    { USB_VPI(USB_VENDOR_SILABS,	USB_PRODUCT_SILABS_LIPOWSKY_JTAG, 0) },
    { USB_VPI(USB_VENDOR_SILABS,	USB_PRODUCT_SILABS_LIPOWSKY_LIN, 0) },
    { USB_VPI(USB_VENDOR_SILABS,	USB_PRODUCT_SILABS_POLOLU, 0) },
    { USB_VPI(USB_VENDOR_SILABS,	USB_PRODUCT_SILABS_CP2102, 0) },
    { USB_VPI(USB_VENDOR_SILABS,	USB_PRODUCT_SILABS_CP210X_2, 0) },
    { USB_VPI(USB_VENDOR_SILABS,	USB_PRODUCT_SILABS_SUUNTO, 0) },
    { USB_VPI(USB_VENDOR_SILABS,	USB_PRODUCT_SILABS_TRAQMATE, 0) },
    { USB_VPI(USB_VENDOR_SILABS2,	USB_PRODUCT_SILABS2_DCU11CLONE, 0) },
    { USB_VPI(USB_VENDOR_USI,		USB_PRODUCT_USI_MC60, 0) },
};

static device_method_t uslcom_methods[] = {
	DEVMETHOD(device_probe, uslcom_probe),
	DEVMETHOD(device_attach, uslcom_attach),
	DEVMETHOD(device_detach, uslcom_detach),
	{0, 0}
};

static devclass_t uslcom_devclass;

static driver_t uslcom_driver = {
	.name = "uslcom",
	.methods = uslcom_methods,
	.size = sizeof(struct uslcom_softc),
};

DRIVER_MODULE(uslcom, uhub, uslcom_driver, uslcom_devclass, NULL, 0);
MODULE_DEPEND(uslcom, ucom, 1, 1, 1);
MODULE_DEPEND(uslcom, usb, 1, 1, 1);
MODULE_VERSION(uslcom, 1);

static int
uslcom_probe(device_t dev)
{
	struct usb2_attach_arg *uaa = device_get_ivars(dev);

	DPRINTFN(11, "\n");

	if (uaa->usb2_mode != USB_MODE_HOST) {
		return (ENXIO);
	}
	if (uaa->info.bConfigIndex != USLCOM_CONFIG_INDEX) {
		return (ENXIO);
	}
	if (uaa->info.bIfaceIndex != USLCOM_IFACE_INDEX) {
		return (ENXIO);
	}
	return (usb2_lookup_id_by_uaa(uslcom_devs, sizeof(uslcom_devs), uaa));
}

static int
uslcom_attach(device_t dev)
{
	struct usb2_attach_arg *uaa = device_get_ivars(dev);
	struct uslcom_softc *sc = device_get_softc(dev);
	int error;

	DPRINTFN(11, "\n");

	device_set_usb2_desc(dev);
	mtx_init(&sc->sc_mtx, "uslcom", NULL, MTX_DEF);

	sc->sc_udev = uaa->device;

	error = usb2_transfer_setup(uaa->device,
	    &uaa->info.bIfaceIndex, sc->sc_xfer, uslcom_config,
	    USLCOM_N_TRANSFER, sc, &sc->sc_mtx);
	if (error) {
		DPRINTF("one or more missing USB endpoints, "
		    "error=%s\n", usb2_errstr(error));
		goto detach;
	}
	/* clear stall at first run */
	mtx_lock(&sc->sc_mtx);
	usb2_transfer_set_stall(sc->sc_xfer[USLCOM_BULK_DT_WR]);
	usb2_transfer_set_stall(sc->sc_xfer[USLCOM_BULK_DT_RD]);
	mtx_unlock(&sc->sc_mtx);

	error = usb2_com_attach(&sc->sc_super_ucom, &sc->sc_ucom, 1, sc,
	    &uslcom_callback, &sc->sc_mtx);
	if (error) {
		goto detach;
	}
	return (0);

detach:
	uslcom_detach(dev);
	return (ENXIO);
}

static int
uslcom_detach(device_t dev)
{
	struct uslcom_softc *sc = device_get_softc(dev);

	DPRINTF("sc=%p\n", sc);

	usb2_com_detach(&sc->sc_super_ucom, &sc->sc_ucom, 1);
	usb2_transfer_unsetup(sc->sc_xfer, USLCOM_N_TRANSFER);
	mtx_destroy(&sc->sc_mtx);

	return (0);
}

static void
uslcom_open(struct usb2_com_softc *ucom)
{
	struct uslcom_softc *sc = ucom->sc_parent;
	struct usb2_device_request req;

	req.bmRequestType = USLCOM_WRITE;
	req.bRequest = USLCOM_UART;
	USETW(req.wValue, USLCOM_UART_ENABLE);
	USETW(req.wIndex, USLCOM_PORT_NO);
	USETW(req.wLength, 0);

        if (usb2_com_cfg_do_request(sc->sc_udev, &sc->sc_ucom, 
	    &req, NULL, 0, 1000)) {
		DPRINTF("UART enable failed (ignored)\n");
	}
}

static void
uslcom_close(struct usb2_com_softc *ucom)
{
	struct uslcom_softc *sc = ucom->sc_parent;
	struct usb2_device_request req;

	req.bmRequestType = USLCOM_WRITE;
	req.bRequest = USLCOM_UART;
	USETW(req.wValue, USLCOM_UART_DISABLE);
	USETW(req.wIndex, USLCOM_PORT_NO);
	USETW(req.wLength, 0);

        if (usb2_com_cfg_do_request(sc->sc_udev, &sc->sc_ucom, 
	    &req, NULL, 0, 1000)) {
		DPRINTF("UART disable failed (ignored)\n");
	}
}

static void
uslcom_set_dtr(struct usb2_com_softc *ucom, uint8_t onoff)
{
        struct uslcom_softc *sc = ucom->sc_parent;
	struct usb2_device_request req;
	uint16_t ctl;

        DPRINTF("onoff = %d\n", onoff);

	ctl = onoff ? USLCOM_CTRL_DTR_ON : 0;
	ctl |= USLCOM_CTRL_DTR_SET;

	req.bmRequestType = USLCOM_WRITE;
	req.bRequest = USLCOM_CTRL;
	USETW(req.wValue, ctl);
	USETW(req.wIndex, USLCOM_PORT_NO);
	USETW(req.wLength, 0);

        if (usb2_com_cfg_do_request(sc->sc_udev, &sc->sc_ucom, 
	    &req, NULL, 0, 1000)) {
		DPRINTF("Setting DTR failed (ignored)\n");
	}
}

static void
uslcom_set_rts(struct usb2_com_softc *ucom, uint8_t onoff)
{
        struct uslcom_softc *sc = ucom->sc_parent;
	struct usb2_device_request req;
	uint16_t ctl;

        DPRINTF("onoff = %d\n", onoff);

	ctl = onoff ? USLCOM_CTRL_RTS_ON : 0;
	ctl |= USLCOM_CTRL_RTS_SET;

	req.bmRequestType = USLCOM_WRITE;
	req.bRequest = USLCOM_CTRL;
	USETW(req.wValue, ctl);
	USETW(req.wIndex, USLCOM_PORT_NO);
	USETW(req.wLength, 0);

        if (usb2_com_cfg_do_request(sc->sc_udev, &sc->sc_ucom, 
	    &req, NULL, 0, 1000)) {
		DPRINTF("Setting DTR failed (ignored)\n");
	}
}

static int
uslcom_pre_param(struct usb2_com_softc *ucom, struct termios *t)
{
	if (t->c_ospeed <= 0 || t->c_ospeed > 921600)
		return (EINVAL);
	return (0);
}

static void
uslcom_param(struct usb2_com_softc *ucom, struct termios *t)
{
	struct uslcom_softc *sc = ucom->sc_parent;
	struct usb2_device_request req;
	uint16_t data;

	DPRINTF("\n");

	req.bmRequestType = USLCOM_WRITE;
	req.bRequest = USLCOM_BAUD_RATE;
	USETW(req.wValue, USLCOM_BAUD_REF / t->c_ospeed);
	USETW(req.wIndex, USLCOM_PORT_NO);
	USETW(req.wLength, 0);

        if (usb2_com_cfg_do_request(sc->sc_udev, &sc->sc_ucom, 
	    &req, NULL, 0, 1000)) {
		DPRINTF("Set baudrate failed (ignored)\n");
	}

	if (t->c_cflag & CSTOPB)
		data = USLCOM_STOP_BITS_2;
	else
		data = USLCOM_STOP_BITS_1;
	if (t->c_cflag & PARENB) {
		if (t->c_cflag & PARODD)
			data |= USLCOM_PARITY_ODD;
		else
			data |= USLCOM_PARITY_EVEN;
	} else
		data |= USLCOM_PARITY_NONE;
	switch (t->c_cflag & CSIZE) {
	case CS5:
		data |= USLCOM_SET_DATA_BITS(5);
		break;
	case CS6:
		data |= USLCOM_SET_DATA_BITS(6);
		break;
	case CS7:
		data |= USLCOM_SET_DATA_BITS(7);
		break;
	case CS8:
		data |= USLCOM_SET_DATA_BITS(8);
		break;
	}

	req.bmRequestType = USLCOM_WRITE;
	req.bRequest = USLCOM_DATA;
	USETW(req.wValue, data);
	USETW(req.wIndex, USLCOM_PORT_NO);
	USETW(req.wLength, 0);

        if (usb2_com_cfg_do_request(sc->sc_udev, &sc->sc_ucom, 
	    &req, NULL, 0, 1000)) {
		DPRINTF("Set format failed (ignored)\n");
	}
	return;
}

static void
uslcom_get_status(struct usb2_com_softc *ucom, uint8_t *lsr, uint8_t *msr)
{
	struct uslcom_softc *sc = ucom->sc_parent;

	DPRINTF("\n");

	*lsr = sc->sc_lsr;
	*msr = sc->sc_msr;
}

static void
uslcom_set_break(struct usb2_com_softc *ucom, uint8_t onoff)
{
        struct uslcom_softc *sc = ucom->sc_parent;
	struct usb2_device_request req;
	uint16_t brk = onoff ? USLCOM_BREAK_ON : USLCOM_BREAK_OFF;

	req.bmRequestType = USLCOM_WRITE;
	req.bRequest = USLCOM_BREAK;
	USETW(req.wValue, brk);
	USETW(req.wIndex, USLCOM_PORT_NO);
	USETW(req.wLength, 0);

        if (usb2_com_cfg_do_request(sc->sc_udev, &sc->sc_ucom, 
	    &req, NULL, 0, 1000)) {
		DPRINTF("Set BREAK failed (ignored)\n");
	}
}

static void
uslcom_write_callback(struct usb2_xfer *xfer)
{
	struct uslcom_softc *sc = xfer->priv_sc;
	uint32_t actlen;

	switch (USB_GET_STATE(xfer)) {
	case USB_ST_SETUP:
	case USB_ST_TRANSFERRED:
tr_setup:
		if (usb2_com_get_data(&sc->sc_ucom, xfer->frbuffers, 0,
		    USLCOM_BULK_BUF_SIZE, &actlen)) {

			DPRINTF("actlen = %d\n", actlen);

			xfer->frlengths[0] = actlen;
			usb2_start_hardware(xfer);
		}
		return;

	default:			/* Error */
		if (xfer->error != USB_ERR_CANCELLED) {
			/* try to clear stall first */
			xfer->flags.stall_pipe = 1;
			goto tr_setup;
		}
		return;
	}
}

static void
uslcom_read_callback(struct usb2_xfer *xfer)
{
	struct uslcom_softc *sc = xfer->priv_sc;

	switch (USB_GET_STATE(xfer)) {
	case USB_ST_TRANSFERRED:
		usb2_com_put_data(&sc->sc_ucom, xfer->frbuffers, 0, xfer->actlen);

	case USB_ST_SETUP:
tr_setup:
		xfer->frlengths[0] = xfer->max_data_length;
		usb2_start_hardware(xfer);
		return;

	default:			/* Error */
		if (xfer->error != USB_ERR_CANCELLED) {
			/* try to clear stall first */
			xfer->flags.stall_pipe = 1;
			goto tr_setup;
		}
		return;
	}
}

static void
uslcom_start_read(struct usb2_com_softc *ucom)
{
	struct uslcom_softc *sc = ucom->sc_parent;

	/* start read endpoint */
	usb2_transfer_start(sc->sc_xfer[USLCOM_BULK_DT_RD]);
}

static void
uslcom_stop_read(struct usb2_com_softc *ucom)
{
	struct uslcom_softc *sc = ucom->sc_parent;

	/* stop read endpoint */
	usb2_transfer_stop(sc->sc_xfer[USLCOM_BULK_DT_RD]);
}

static void
uslcom_start_write(struct usb2_com_softc *ucom)
{
	struct uslcom_softc *sc = ucom->sc_parent;

	usb2_transfer_start(sc->sc_xfer[USLCOM_BULK_DT_WR]);
}

static void
uslcom_stop_write(struct usb2_com_softc *ucom)
{
	struct uslcom_softc *sc = ucom->sc_parent;

	usb2_transfer_stop(sc->sc_xfer[USLCOM_BULK_DT_WR]);
}
