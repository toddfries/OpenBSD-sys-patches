/*-
 * Copyright (c) 1997, 1998, 1999, 2000
 *	Bill Paul <wpaul@ee.columbia.edu>.  All rights reserved.
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
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/dev/usb/net/if_kue.c,v 1.2 2009/03/02 05:37:05 thompsa Exp $");

/*
 * Kawasaki LSI KL5KUSB101B USB to ethernet adapter driver.
 *
 * Written by Bill Paul <wpaul@ee.columbia.edu>
 * Electrical Engineering Department
 * Columbia University, New York City
 */

/*
 * The KLSI USB to ethernet adapter chip contains an USB serial interface,
 * ethernet MAC and embedded microcontroller (called the QT Engine).
 * The chip must have firmware loaded into it before it will operate.
 * Packets are passed between the chip and host via bulk transfers.
 * There is an interrupt endpoint mentioned in the software spec, however
 * it's currently unused. This device is 10Mbps half-duplex only, hence
 * there is no media selection logic. The MAC supports a 128 entry
 * multicast filter, though the exact size of the filter can depend
 * on the firmware. Curiously, while the software spec describes various
 * ethernet statistics counters, my sample adapter and firmware combination
 * claims not to support any statistics counters at all.
 *
 * Note that once we load the firmware in the device, we have to be
 * careful not to load it again: if you restart your computer but
 * leave the adapter attached to the USB controller, it may remain
 * powered on and retain its firmware. In this case, we don't need
 * to load the firmware a second time.
 *
 * Special thanks to Rob Furr for providing an ADS Technologies
 * adapter for development and testing. No monkeys were harmed during
 * the development of this driver.
 */

#include "usbdevs.h"
#include <dev/usb/usb.h>
#include <dev/usb/usb_mfunc.h>
#include <dev/usb/usb_error.h>

#define	USB_DEBUG_VAR kue_debug

#include <dev/usb/usb_core.h>
#include <dev/usb/usb_lookup.h>
#include <dev/usb/usb_process.h>
#include <dev/usb/usb_debug.h>
#include <dev/usb/usb_request.h>
#include <dev/usb/usb_busdma.h>
#include <dev/usb/usb_util.h>

#include <dev/usb/net/usb_ethernet.h>
#include <dev/usb/net/if_kuereg.h>
#include <dev/usb/net/if_kuefw.h>

/*
 * Various supported device vendors/products.
 */
static const struct usb2_device_id kue_devs[] = {
	{USB_VPI(USB_VENDOR_3COM, USB_PRODUCT_3COM_3C19250, 0)},
	{USB_VPI(USB_VENDOR_3COM, USB_PRODUCT_3COM_3C460, 0)},
	{USB_VPI(USB_VENDOR_ABOCOM, USB_PRODUCT_ABOCOM_URE450, 0)},
	{USB_VPI(USB_VENDOR_ADS, USB_PRODUCT_ADS_UBS10BT, 0)},
	{USB_VPI(USB_VENDOR_ADS, USB_PRODUCT_ADS_UBS10BTX, 0)},
	{USB_VPI(USB_VENDOR_AOX, USB_PRODUCT_AOX_USB101, 0)},
	{USB_VPI(USB_VENDOR_ASANTE, USB_PRODUCT_ASANTE_EA, 0)},
	{USB_VPI(USB_VENDOR_ATEN, USB_PRODUCT_ATEN_DSB650C, 0)},
	{USB_VPI(USB_VENDOR_ATEN, USB_PRODUCT_ATEN_UC10T, 0)},
	{USB_VPI(USB_VENDOR_COREGA, USB_PRODUCT_COREGA_ETHER_USB_T, 0)},
	{USB_VPI(USB_VENDOR_DLINK, USB_PRODUCT_DLINK_DSB650C, 0)},
	{USB_VPI(USB_VENDOR_ENTREGA, USB_PRODUCT_ENTREGA_E45, 0)},
	{USB_VPI(USB_VENDOR_ENTREGA, USB_PRODUCT_ENTREGA_XX1, 0)},
	{USB_VPI(USB_VENDOR_ENTREGA, USB_PRODUCT_ENTREGA_XX2, 0)},
	{USB_VPI(USB_VENDOR_IODATA, USB_PRODUCT_IODATA_USBETT, 0)},
	{USB_VPI(USB_VENDOR_JATON, USB_PRODUCT_JATON_EDA, 0)},
	{USB_VPI(USB_VENDOR_KINGSTON, USB_PRODUCT_KINGSTON_XX1, 0)},
	{USB_VPI(USB_VENDOR_KLSI, USB_PRODUCT_AOX_USB101, 0)},
	{USB_VPI(USB_VENDOR_KLSI, USB_PRODUCT_KLSI_DUH3E10BT, 0)},
	{USB_VPI(USB_VENDOR_KLSI, USB_PRODUCT_KLSI_DUH3E10BTN, 0)},
	{USB_VPI(USB_VENDOR_LINKSYS, USB_PRODUCT_LINKSYS_USB10T, 0)},
	{USB_VPI(USB_VENDOR_MOBILITY, USB_PRODUCT_MOBILITY_EA, 0)},
	{USB_VPI(USB_VENDOR_NETGEAR, USB_PRODUCT_NETGEAR_EA101, 0)},
	{USB_VPI(USB_VENDOR_NETGEAR, USB_PRODUCT_NETGEAR_EA101X, 0)},
	{USB_VPI(USB_VENDOR_PERACOM, USB_PRODUCT_PERACOM_ENET, 0)},
	{USB_VPI(USB_VENDOR_PERACOM, USB_PRODUCT_PERACOM_ENET2, 0)},
	{USB_VPI(USB_VENDOR_PERACOM, USB_PRODUCT_PERACOM_ENET3, 0)},
	{USB_VPI(USB_VENDOR_PORTGEAR, USB_PRODUCT_PORTGEAR_EA8, 0)},
	{USB_VPI(USB_VENDOR_PORTGEAR, USB_PRODUCT_PORTGEAR_EA9, 0)},
	{USB_VPI(USB_VENDOR_PORTSMITH, USB_PRODUCT_PORTSMITH_EEA, 0)},
	{USB_VPI(USB_VENDOR_SHARK, USB_PRODUCT_SHARK_PA, 0)},
	{USB_VPI(USB_VENDOR_SILICOM, USB_PRODUCT_SILICOM_GPE, 0)},
	{USB_VPI(USB_VENDOR_SILICOM, USB_PRODUCT_SILICOM_U2E, 0)},
	{USB_VPI(USB_VENDOR_SMC, USB_PRODUCT_SMC_2102USB, 0)},
};

/* prototypes */

static device_probe_t kue_probe;
static device_attach_t kue_attach;
static device_detach_t kue_detach;
static device_shutdown_t kue_shutdown;

static usb2_callback_t kue_bulk_read_callback;
static usb2_callback_t kue_bulk_write_callback;

static usb2_ether_fn_t kue_attach_post;
static usb2_ether_fn_t kue_init;
static usb2_ether_fn_t kue_stop;
static usb2_ether_fn_t kue_start;
static usb2_ether_fn_t kue_setmulti;
static usb2_ether_fn_t kue_setpromisc;

static int	kue_do_request(struct kue_softc *,
		    struct usb2_device_request *, void *);
static int	kue_setword(struct kue_softc *, uint8_t, uint16_t);
static int	kue_ctl(struct kue_softc *, uint8_t, uint8_t, uint16_t,
		    void *, int);
static int	kue_load_fw(struct kue_softc *);
static void	kue_reset(struct kue_softc *);

#if USB_DEBUG
static int kue_debug = 0;

SYSCTL_NODE(_hw_usb2, OID_AUTO, kue, CTLFLAG_RW, 0, "USB kue");
SYSCTL_INT(_hw_usb2_kue, OID_AUTO, debug, CTLFLAG_RW, &kue_debug, 0,
    "Debug level");
#endif

static const struct usb2_config kue_config[KUE_N_TRANSFER] = {

	[KUE_BULK_DT_WR] = {
		.type = UE_BULK,
		.endpoint = UE_ADDR_ANY,
		.direction = UE_DIR_OUT,
		.mh.bufsize = (MCLBYTES + 2 + 64),
		.mh.flags = {.pipe_bof = 1,},
		.mh.callback = kue_bulk_write_callback,
		.mh.timeout = 10000,	/* 10 seconds */
	},

	[KUE_BULK_DT_RD] = {
		.type = UE_BULK,
		.endpoint = UE_ADDR_ANY,
		.direction = UE_DIR_IN,
		.mh.bufsize = (MCLBYTES + 2),
		.mh.flags = {.pipe_bof = 1,.short_xfer_ok = 1,},
		.mh.callback = kue_bulk_read_callback,
		.mh.timeout = 0,	/* no timeout */
	},
};

static device_method_t kue_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe, kue_probe),
	DEVMETHOD(device_attach, kue_attach),
	DEVMETHOD(device_detach, kue_detach),
	DEVMETHOD(device_shutdown, kue_shutdown),

	{0, 0}
};

static driver_t kue_driver = {
	.name = "kue",
	.methods = kue_methods,
	.size = sizeof(struct kue_softc),
};

static devclass_t kue_devclass;

DRIVER_MODULE(kue, uhub, kue_driver, kue_devclass, NULL, 0);
MODULE_DEPEND(kue, uether, 1, 1, 1);
MODULE_DEPEND(kue, usb, 1, 1, 1);
MODULE_DEPEND(kue, ether, 1, 1, 1);

static const struct usb2_ether_methods kue_ue_methods = {
	.ue_attach_post = kue_attach_post,
	.ue_start = kue_start,
	.ue_init = kue_init,
	.ue_stop = kue_stop,
	.ue_setmulti = kue_setmulti,
	.ue_setpromisc = kue_setpromisc,
};

/*
 * We have a custom do_request function which is almost like the
 * regular do_request function, except it has a much longer timeout.
 * Why? Because we need to make requests over the control endpoint
 * to download the firmware to the device, which can take longer
 * than the default timeout.
 */
static int
kue_do_request(struct kue_softc *sc, struct usb2_device_request *req,
    void *data)
{
	usb2_error_t err;

	err = usb2_ether_do_request(&sc->sc_ue, req, data, 60000);

	return (err);
}

static int
kue_setword(struct kue_softc *sc, uint8_t breq, uint16_t word)
{
	struct usb2_device_request req;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = breq;
	USETW(req.wValue, word);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 0);

	return (kue_do_request(sc, &req, NULL));
}

static int
kue_ctl(struct kue_softc *sc, uint8_t rw, uint8_t breq,
    uint16_t val, void *data, int len)
{
	struct usb2_device_request req;

	if (rw == KUE_CTL_WRITE)
		req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	else
		req.bmRequestType = UT_READ_VENDOR_DEVICE;


	req.bRequest = breq;
	USETW(req.wValue, val);
	USETW(req.wIndex, 0);
	USETW(req.wLength, len);

	return (kue_do_request(sc, &req, data));
}

static int
kue_load_fw(struct kue_softc *sc)
{
	struct usb2_device_descriptor *dd;
	uint16_t hwrev;
	usb2_error_t err;

	dd = usb2_get_device_descriptor(sc->sc_ue.ue_udev);
	hwrev = UGETW(dd->bcdDevice);

	/*
	 * First, check if we even need to load the firmware.
	 * If the device was still attached when the system was
	 * rebooted, it may already have firmware loaded in it.
	 * If this is the case, we don't need to do it again.
	 * And in fact, if we try to load it again, we'll hang,
	 * so we have to avoid this condition if we don't want
	 * to look stupid.
	 *
	 * We can test this quickly by checking the bcdRevision
	 * code. The NIC will return a different revision code if
	 * it's probed while the firmware is still loaded and
	 * running.
	 */
	if (hwrev == 0x0202)
		return(0);

	/* Load code segment */
	err = kue_ctl(sc, KUE_CTL_WRITE, KUE_CMD_SEND_SCAN,
	    0, kue_code_seg, sizeof(kue_code_seg));
	if (err) {
		device_printf(sc->sc_ue.ue_dev, "failed to load code segment: %s\n",
		    usb2_errstr(err));
		return(ENXIO);
	}

	/* Load fixup segment */
	err = kue_ctl(sc, KUE_CTL_WRITE, KUE_CMD_SEND_SCAN,
	    0, kue_fix_seg, sizeof(kue_fix_seg));
	if (err) {
		device_printf(sc->sc_ue.ue_dev, "failed to load fixup segment: %s\n",
		    usb2_errstr(err));
		return(ENXIO);
	}

	/* Send trigger command. */
	err = kue_ctl(sc, KUE_CTL_WRITE, KUE_CMD_SEND_SCAN,
	    0, kue_trig_seg, sizeof(kue_trig_seg));
	if (err) {
		device_printf(sc->sc_ue.ue_dev, "failed to load trigger segment: %s\n",
		    usb2_errstr(err));
		return(ENXIO);
	}

	return (0);
}

static void
kue_setpromisc(struct usb2_ether *ue)
{
	struct kue_softc *sc = usb2_ether_getsc(ue);
	struct ifnet *ifp = usb2_ether_getifp(ue);

	KUE_LOCK_ASSERT(sc, MA_OWNED);

	if (ifp->if_flags & IFF_PROMISC)
		sc->sc_rxfilt |= KUE_RXFILT_PROMISC;
	else
		sc->sc_rxfilt &= ~KUE_RXFILT_PROMISC;

	kue_setword(sc, KUE_CMD_SET_PKT_FILTER, sc->sc_rxfilt);
}

static void
kue_setmulti(struct usb2_ether *ue)
{
	struct kue_softc *sc = usb2_ether_getsc(ue);
	struct ifnet *ifp = usb2_ether_getifp(ue);
	struct ifmultiaddr *ifma;
	int i = 0;

	KUE_LOCK_ASSERT(sc, MA_OWNED);

	if (ifp->if_flags & IFF_ALLMULTI || ifp->if_flags & IFF_PROMISC) {
		sc->sc_rxfilt |= KUE_RXFILT_ALLMULTI;
		sc->sc_rxfilt &= ~KUE_RXFILT_MULTICAST;
		kue_setword(sc, KUE_CMD_SET_PKT_FILTER, sc->sc_rxfilt);
		return;
	}

	sc->sc_rxfilt &= ~KUE_RXFILT_ALLMULTI;

	IF_ADDR_LOCK(ifp);
	TAILQ_FOREACH(ifma, &ifp->if_multiaddrs, ifma_link)
	{
		if (ifma->ifma_addr->sa_family != AF_LINK)
			continue;
		/*
		 * If there are too many addresses for the
		 * internal filter, switch over to allmulti mode.
		 */
		if (i == KUE_MCFILTCNT(sc))
			break;
		bcopy(LLADDR((struct sockaddr_dl *)ifma->ifma_addr),
		    KUE_MCFILT(sc, i), ETHER_ADDR_LEN);
		i++;
	}
	IF_ADDR_UNLOCK(ifp);

	if (i == KUE_MCFILTCNT(sc))
		sc->sc_rxfilt |= KUE_RXFILT_ALLMULTI;
	else {
		sc->sc_rxfilt |= KUE_RXFILT_MULTICAST;
		kue_ctl(sc, KUE_CTL_WRITE, KUE_CMD_SET_MCAST_FILTERS,
		    i, sc->sc_mcfilters, i * ETHER_ADDR_LEN);
	}

	kue_setword(sc, KUE_CMD_SET_PKT_FILTER, sc->sc_rxfilt);
}

/*
 * Issue a SET_CONFIGURATION command to reset the MAC. This should be
 * done after the firmware is loaded into the adapter in order to
 * bring it into proper operation.
 */
static void
kue_reset(struct kue_softc *sc)
{
	struct usb2_config_descriptor *cd;
	usb2_error_t err;

	cd = usb2_get_config_descriptor(sc->sc_ue.ue_udev);

	err = usb2_req_set_config(sc->sc_ue.ue_udev, &sc->sc_mtx,
	    cd->bConfigurationValue);
	if (err)
		DPRINTF("reset failed (ignored)\n");

	/* wait a little while for the chip to get its brains in order */
	usb2_ether_pause(&sc->sc_ue, hz / 100);
}

static void
kue_attach_post(struct usb2_ether *ue)
{
	struct kue_softc *sc = usb2_ether_getsc(ue);
	int error;

	/* load the firmware into the NIC */
	error = kue_load_fw(sc);
	if (error) {
		device_printf(sc->sc_ue.ue_dev, "could not load firmware\n");
		/* ignore the error */
	}

	/* reset the adapter */
	kue_reset(sc);

	/* read ethernet descriptor */
	kue_ctl(sc, KUE_CTL_READ, KUE_CMD_GET_ETHER_DESCRIPTOR,
	    0, &sc->sc_desc, sizeof(sc->sc_desc));

	/* copy in ethernet address */
	memcpy(ue->ue_eaddr, sc->sc_desc.kue_macaddr, sizeof(ue->ue_eaddr));
}

/*
 * Probe for a KLSI chip.
 */
static int
kue_probe(device_t dev)
{
	struct usb2_attach_arg *uaa = device_get_ivars(dev);

	if (uaa->usb2_mode != USB_MODE_HOST)
		return (ENXIO);
	if (uaa->info.bConfigIndex != KUE_CONFIG_IDX)
		return (ENXIO);
	if (uaa->info.bIfaceIndex != KUE_IFACE_IDX)
		return (ENXIO);

	return (usb2_lookup_id_by_uaa(kue_devs, sizeof(kue_devs), uaa));
}

/*
 * Attach the interface. Allocate softc structures, do
 * setup and ethernet/BPF attach.
 */
static int
kue_attach(device_t dev)
{
	struct usb2_attach_arg *uaa = device_get_ivars(dev);
	struct kue_softc *sc = device_get_softc(dev);
	struct usb2_ether *ue = &sc->sc_ue;
	uint8_t iface_index;
	int error;

	device_set_usb2_desc(dev);
	mtx_init(&sc->sc_mtx, device_get_nameunit(dev), NULL, MTX_DEF);

	iface_index = KUE_IFACE_IDX;
	error = usb2_transfer_setup(uaa->device, &iface_index,
	    sc->sc_xfer, kue_config, KUE_N_TRANSFER, sc, &sc->sc_mtx);
	if (error) {
		device_printf(dev, "allocating USB transfers failed!\n");
		goto detach;
	}

	sc->sc_mcfilters = malloc(KUE_MCFILTCNT(sc) * ETHER_ADDR_LEN,
	    M_USBDEV, M_WAITOK);
	if (sc->sc_mcfilters == NULL) {
		device_printf(dev, "failed allocating USB memory!\n");
		goto detach;
	}

	ue->ue_sc = sc;
	ue->ue_dev = dev;
	ue->ue_udev = uaa->device;
	ue->ue_mtx = &sc->sc_mtx;
	ue->ue_methods = &kue_ue_methods;

	error = usb2_ether_ifattach(ue);
	if (error) {
		device_printf(dev, "could not attach interface\n");
		goto detach;
	}
	return (0);			/* success */

detach:
	kue_detach(dev);
	return (ENXIO);			/* failure */
}

static int
kue_detach(device_t dev)
{
	struct kue_softc *sc = device_get_softc(dev);
	struct usb2_ether *ue = &sc->sc_ue;

	usb2_transfer_unsetup(sc->sc_xfer, KUE_N_TRANSFER);
	usb2_ether_ifdetach(ue);
	mtx_destroy(&sc->sc_mtx);
	free(sc->sc_mcfilters, M_USBDEV);

	return (0);
}

/*
 * A frame has been uploaded: pass the resulting mbuf chain up to
 * the higher level protocols.
 */
static void
kue_bulk_read_callback(struct usb2_xfer *xfer)
{
	struct kue_softc *sc = xfer->priv_sc;
	struct usb2_ether *ue = &sc->sc_ue;
	struct ifnet *ifp = usb2_ether_getifp(ue);
	uint8_t buf[2];
	int len;

	switch (USB_GET_STATE(xfer)) {
	case USB_ST_TRANSFERRED:

		if (xfer->actlen <= (2 + sizeof(struct ether_header))) {
			ifp->if_ierrors++;
			goto tr_setup;
		}
		usb2_copy_out(xfer->frbuffers, 0, buf, 2);
		xfer->actlen -= 2;
		len = buf[0] | (buf[1] << 8);
		len = min(xfer->actlen, len);

		usb2_ether_rxbuf(ue, xfer->frbuffers, 2, len);
		/* FALLTHROUGH */
	case USB_ST_SETUP:
tr_setup:
		xfer->frlengths[0] = xfer->max_data_length;
		usb2_start_hardware(xfer);
		usb2_ether_rxflush(ue);
		return;

	default:			/* Error */
		DPRINTF("bulk read error, %s\n",
		    usb2_errstr(xfer->error));

		if (xfer->error != USB_ERR_CANCELLED) {
			/* try to clear stall first */
			xfer->flags.stall_pipe = 1;
			goto tr_setup;
		}
		return;

	}
}

static void
kue_bulk_write_callback(struct usb2_xfer *xfer)
{
	struct kue_softc *sc = xfer->priv_sc;
	struct ifnet *ifp = usb2_ether_getifp(&sc->sc_ue);
	struct mbuf *m;
	int total_len;
	int temp_len;
	uint8_t buf[2];

	switch (USB_GET_STATE(xfer)) {
	case USB_ST_TRANSFERRED:
		DPRINTFN(11, "transfer complete\n");
		ifp->if_opackets++;

		/* FALLTHROUGH */
	case USB_ST_SETUP:
tr_setup:
		IFQ_DRV_DEQUEUE(&ifp->if_snd, m);

		if (m == NULL)
			return;
		if (m->m_pkthdr.len > MCLBYTES)
			m->m_pkthdr.len = MCLBYTES;
		temp_len = (m->m_pkthdr.len + 2);
		total_len = (temp_len + (64 - (temp_len % 64)));

		/* the first two bytes are the frame length */

		buf[0] = (uint8_t)(m->m_pkthdr.len);
		buf[1] = (uint8_t)(m->m_pkthdr.len >> 8);

		usb2_copy_in(xfer->frbuffers, 0, buf, 2);

		usb2_m_copy_in(xfer->frbuffers, 2,
		    m, 0, m->m_pkthdr.len);

		usb2_bzero(xfer->frbuffers, temp_len,
		    total_len - temp_len);

		xfer->frlengths[0] = total_len;

		/*
		 * if there's a BPF listener, bounce a copy
		 * of this frame to him:
		 */
		BPF_MTAP(ifp, m);

		m_freem(m);

		usb2_start_hardware(xfer);

		return;

	default:			/* Error */
		DPRINTFN(11, "transfer error, %s\n",
		    usb2_errstr(xfer->error));

		ifp->if_oerrors++;

		if (xfer->error != USB_ERR_CANCELLED) {
			/* try to clear stall first */
			xfer->flags.stall_pipe = 1;
			goto tr_setup;
		}
		return;

	}
}

static void
kue_start(struct usb2_ether *ue)
{
	struct kue_softc *sc = usb2_ether_getsc(ue);

	/*
	 * start the USB transfers, if not already started:
	 */
	usb2_transfer_start(sc->sc_xfer[KUE_BULK_DT_RD]);
	usb2_transfer_start(sc->sc_xfer[KUE_BULK_DT_WR]);
}

static void
kue_init(struct usb2_ether *ue)
{
	struct kue_softc *sc = usb2_ether_getsc(ue);
	struct ifnet *ifp = usb2_ether_getifp(ue);

	KUE_LOCK_ASSERT(sc, MA_OWNED);

	/* set MAC address */
	kue_ctl(sc, KUE_CTL_WRITE, KUE_CMD_SET_MAC,
	    0, IF_LLADDR(ifp), ETHER_ADDR_LEN);

	/* I'm not sure how to tune these. */
#if 0
	/*
	 * Leave this one alone for now; setting it
	 * wrong causes lockups on some machines/controllers.
	 */
	kue_setword(sc, KUE_CMD_SET_SOFS, 1);
#endif
	kue_setword(sc, KUE_CMD_SET_URB_SIZE, 64);

	/* load the multicast filter */
	kue_setpromisc(ue);

	usb2_transfer_set_stall(sc->sc_xfer[KUE_BULK_DT_WR]);

	ifp->if_drv_flags |= IFF_DRV_RUNNING;
	kue_start(ue);
}

static void
kue_stop(struct usb2_ether *ue)
{
	struct kue_softc *sc = usb2_ether_getsc(ue);
	struct ifnet *ifp = usb2_ether_getifp(ue);

	KUE_LOCK_ASSERT(sc, MA_OWNED);

	ifp->if_drv_flags &= ~IFF_DRV_RUNNING;

	/*
	 * stop all the transfers, if not already stopped:
	 */
	usb2_transfer_stop(sc->sc_xfer[KUE_BULK_DT_WR]);
	usb2_transfer_stop(sc->sc_xfer[KUE_BULK_DT_RD]);
}

/*
 * Stop all chip I/O so that the kernel's probe routines don't
 * get confused by errant DMAs when rebooting.
 */
static int
kue_shutdown(device_t dev)
{
	struct kue_softc *sc = device_get_softc(dev);

	usb2_ether_ifshutdown(&sc->sc_ue);

	return (0);
}
