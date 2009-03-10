/*	$OpenBSD: if_zyd.c,v 1.52 2007/02/11 00:08:04 jsg Exp $	*/
/*	$NetBSD: if_zyd.c,v 1.7 2007/06/21 04:04:29 kiyohara Exp $	*/
/*	$FreeBSD: src/sys/legacy/dev/usb/if_zyd.c,v 1.1 2009/02/23 18:16:17 thompsa Exp $	*/

/*-
 * Copyright (c) 2006 by Damien Bergamini <damien.bergamini@free.fr>
 * Copyright (c) 2006 by Florian Stoehr <ich@florian-stoehr.de>
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
 * ZyDAS ZD1211/ZD1211B USB WLAN driver.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/endian.h>
#include <sys/linker.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>

#include <sys/bus.h>
#include <machine/bus.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_amrr.h>
#include <net80211/ieee80211_phy.h>
#include <net80211/ieee80211_radiotap.h>
#include <net80211/ieee80211_regdomain.h>

#include <net/bpf.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>
#include "usbdevs.h"
#include <dev/usb/usb_ethersubr.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/usb/if_zydreg.h>
#include <dev/usb/if_zydfw.h>

#ifdef ZYD_DEBUG
SYSCTL_NODE(_hw_usb, OID_AUTO, zyd, CTLFLAG_RW, 0, "ZyDAS zd1211/zd1211b");
int zyd_debug = 0;
SYSCTL_INT(_hw_usb_zyd, OID_AUTO, debug, CTLFLAG_RW, &zyd_debug, 0,
    "control debugging printfs");
TUNABLE_INT("hw.usb.zyd.debug", &zyd_debug);
enum {
	ZYD_DEBUG_XMIT		= 0x00000001,	/* basic xmit operation */
	ZYD_DEBUG_RECV		= 0x00000002,	/* basic recv operation */
	ZYD_DEBUG_RESET		= 0x00000004,	/* reset processing */
	ZYD_DEBUG_INIT		= 0x00000008,	/* device init */
	ZYD_DEBUG_TX_PROC	= 0x00000010,	/* tx ISR proc */
	ZYD_DEBUG_RX_PROC	= 0x00000020,	/* rx ISR proc */
	ZYD_DEBUG_STATE		= 0x00000040,	/* 802.11 state transitions */
	ZYD_DEBUG_STAT		= 0x00000080,	/* statistic */
	ZYD_DEBUG_FW		= 0x00000100,	/* firmware */
	ZYD_DEBUG_ANY		= 0xffffffff
};
#define	DPRINTF(sc, m, fmt, ...) do {				\
	if (sc->sc_debug & (m))					\
		printf(fmt, __VA_ARGS__);			\
} while (0)
#else
#define	DPRINTF(sc, m, fmt, ...) do {				\
	(void) sc;						\
} while (0)
#endif

static const struct zyd_phy_pair zyd_def_phy[] = ZYD_DEF_PHY;
static const struct zyd_phy_pair zyd_def_phyB[] = ZYD_DEF_PHYB;

/* various supported device vendors/products */
#define ZYD_ZD1211_DEV(v, p)	\
	{ { USB_VENDOR_##v, USB_PRODUCT_##v##_##p }, ZYD_ZD1211 }
#define ZYD_ZD1211B_DEV(v, p)	\
	{ { USB_VENDOR_##v, USB_PRODUCT_##v##_##p }, ZYD_ZD1211B }
static const struct zyd_type {
	struct usb_devno	dev;
	uint8_t			rev;
#define ZYD_ZD1211	0
#define ZYD_ZD1211B	1
} zyd_devs[] = {
	ZYD_ZD1211_DEV(3COM2,		3CRUSB10075),
	ZYD_ZD1211_DEV(ABOCOM,		WL54),
	ZYD_ZD1211_DEV(ASUS,		WL159G),
	ZYD_ZD1211_DEV(CYBERTAN,	TG54USB),
	ZYD_ZD1211_DEV(DRAYTEK,		VIGOR550),
	ZYD_ZD1211_DEV(PLANEX2,		GWUS54GD),
	ZYD_ZD1211_DEV(PLANEX2,		GWUS54GZL),
	ZYD_ZD1211_DEV(PLANEX3,		GWUS54GZ),
	ZYD_ZD1211_DEV(PLANEX3,		GWUS54MINI),
	ZYD_ZD1211_DEV(SAGEM,		XG760A),
	ZYD_ZD1211_DEV(SENAO,		NUB8301),
	ZYD_ZD1211_DEV(SITECOMEU,	WL113),
	ZYD_ZD1211_DEV(SWEEX,		ZD1211),
	ZYD_ZD1211_DEV(TEKRAM,		QUICKWLAN),
	ZYD_ZD1211_DEV(TEKRAM,		ZD1211_1),
	ZYD_ZD1211_DEV(TEKRAM,		ZD1211_2),
	ZYD_ZD1211_DEV(TWINMOS,		G240),
	ZYD_ZD1211_DEV(UMEDIA,		ALL0298V2),
	ZYD_ZD1211_DEV(UMEDIA,		TEW429UB_A),
	ZYD_ZD1211_DEV(UMEDIA,		TEW429UB),
	ZYD_ZD1211_DEV(WISTRONNEWEB,	UR055G),
	ZYD_ZD1211_DEV(ZCOM,		ZD1211),
	ZYD_ZD1211_DEV(ZYDAS,		ZD1211),
	ZYD_ZD1211_DEV(ZYXEL,		AG225H),
	ZYD_ZD1211_DEV(ZYXEL,		ZYAIRG220),
	ZYD_ZD1211_DEV(ZYXEL,		G200V2),
	ZYD_ZD1211_DEV(ZYXEL,		G202),

	ZYD_ZD1211B_DEV(ACCTON,		SMCWUSBG),
	ZYD_ZD1211B_DEV(ACCTON,		ZD1211B),
	ZYD_ZD1211B_DEV(ASUS,		A9T_WIFI),
	ZYD_ZD1211B_DEV(BELKIN,		F5D7050_V4000),
	ZYD_ZD1211B_DEV(BELKIN,		ZD1211B),
	ZYD_ZD1211B_DEV(CISCOLINKSYS,	WUSBF54G),
	ZYD_ZD1211B_DEV(FIBERLINE,	WL430U),
	ZYD_ZD1211B_DEV(MELCO,		KG54L),
	ZYD_ZD1211B_DEV(PHILIPS,	SNU5600),
	ZYD_ZD1211B_DEV(PLANEX2,	GW_US54GXS),
	ZYD_ZD1211B_DEV(SAGEM,		XG76NA),
	ZYD_ZD1211B_DEV(SITECOMEU,	ZD1211B),
	ZYD_ZD1211B_DEV(UMEDIA,		TEW429UBC1),
#if 0	/* Shall we needs? */
	ZYD_ZD1211B_DEV(UNKNOWN1,	ZD1211B_1),
	ZYD_ZD1211B_DEV(UNKNOWN1,	ZD1211B_2),
	ZYD_ZD1211B_DEV(UNKNOWN2,	ZD1211B),
	ZYD_ZD1211B_DEV(UNKNOWN3,	ZD1211B),
#endif
	ZYD_ZD1211B_DEV(USR,		USR5423),
	ZYD_ZD1211B_DEV(VTECH,		ZD1211B),
	ZYD_ZD1211B_DEV(ZCOM,		ZD1211B),
	ZYD_ZD1211B_DEV(ZYDAS,		ZD1211B),
	ZYD_ZD1211B_DEV(ZYXEL,		M202),
	ZYD_ZD1211B_DEV(ZYXEL,		G220V2),
};
#define zyd_lookup(v, p)						\
	((const struct zyd_type *)usb_lookup(zyd_devs, v, p))
#define zyd_read16_m(sc, val, data)	do {				\
	error = zyd_read16(sc, val, data);				\
	if (error != 0)							\
		goto fail;						\
} while (0)
#define zyd_write16_m(sc, val, data)	do {				\
	error = zyd_write16(sc, val, data);				\
	if (error != 0)							\
		goto fail;						\
} while (0)
#define zyd_read32_m(sc, val, data)	do {				\
	error = zyd_read32(sc, val, data);				\
	if (error != 0)							\
		goto fail;						\
} while (0)
#define zyd_write32_m(sc, val, data)	do {				\
	error = zyd_write32(sc, val, data);				\
	if (error != 0)							\
		goto fail;						\
} while (0)

static device_probe_t zyd_match;
static device_attach_t zyd_attach;
static device_detach_t zyd_detach;

static struct ieee80211vap *zyd_vap_create(struct ieee80211com *,
		    const char name[IFNAMSIZ], int unit, int opmode,
		    int flags, const uint8_t bssid[IEEE80211_ADDR_LEN],
		    const uint8_t mac[IEEE80211_ADDR_LEN]);
static void	zyd_vap_delete(struct ieee80211vap *);
static int	zyd_open_pipes(struct zyd_softc *);
static void	zyd_close_pipes(struct zyd_softc *);
static int	zyd_alloc_tx_list(struct zyd_softc *);
static void	zyd_free_tx_list(struct zyd_softc *);
static int	zyd_alloc_rx_list(struct zyd_softc *);
static void	zyd_free_rx_list(struct zyd_softc *);
static struct ieee80211_node *zyd_node_alloc(struct ieee80211vap *,
			    const uint8_t mac[IEEE80211_ADDR_LEN]);
static void	zyd_task(void *);
static int	zyd_newstate(struct ieee80211vap *, enum ieee80211_state, int);
static int	zyd_cmd(struct zyd_softc *, uint16_t, const void *, int,
		    void *, int, u_int);
static int	zyd_read16(struct zyd_softc *, uint16_t, uint16_t *);
static int	zyd_read32(struct zyd_softc *, uint16_t, uint32_t *);
static int	zyd_write16(struct zyd_softc *, uint16_t, uint16_t);
static int	zyd_write32(struct zyd_softc *, uint16_t, uint32_t);
static int	zyd_rfwrite(struct zyd_softc *, uint32_t);
static int	zyd_lock_phy(struct zyd_softc *);
static int	zyd_unlock_phy(struct zyd_softc *);
static int	zyd_rf_attach(struct zyd_softc *, uint8_t);
static const char *zyd_rf_name(uint8_t);
static int	zyd_hw_init(struct zyd_softc *);
static int	zyd_read_pod(struct zyd_softc *);
static int	zyd_read_eeprom(struct zyd_softc *);
static int	zyd_get_macaddr(struct zyd_softc *);
static int	zyd_set_macaddr(struct zyd_softc *, const uint8_t *);
static int	zyd_set_bssid(struct zyd_softc *, const uint8_t *);
static int	zyd_switch_radio(struct zyd_softc *, int);
static int	zyd_set_led(struct zyd_softc *, int, int);
static void	zyd_set_multi(void *);
static void	zyd_update_mcast(struct ifnet *);
static int	zyd_set_rxfilter(struct zyd_softc *);
static void	zyd_set_chan(struct zyd_softc *, struct ieee80211_channel *);
static int	zyd_set_beacon_interval(struct zyd_softc *, int);
static void	zyd_intr(usbd_xfer_handle, usbd_private_handle, usbd_status);
static void	zyd_rx_data(struct zyd_softc *, const uint8_t *, uint16_t);
static void	zyd_rxeof(usbd_xfer_handle, usbd_private_handle, usbd_status);
static void	zyd_txeof(usbd_xfer_handle, usbd_private_handle, usbd_status);
static int	zyd_tx_mgt(struct zyd_softc *, struct mbuf *,
		    struct ieee80211_node *);
static int	zyd_tx_data(struct zyd_softc *, struct mbuf *,
		    struct ieee80211_node *);
static void	zyd_start(struct ifnet *);
static int	zyd_raw_xmit(struct ieee80211_node *, struct mbuf *,
		    const struct ieee80211_bpf_params *);
static void	zyd_watchdog(void *);
static int	zyd_ioctl(struct ifnet *, u_long, caddr_t);
static void	zyd_init_locked(struct zyd_softc *);
static void	zyd_init(void *);
static void	zyd_stop(struct zyd_softc *, int);
static int	zyd_loadfirmware(struct zyd_softc *);
static void	zyd_newassoc(struct ieee80211_node *, int);
static void	zyd_scantask(void *);
static void	zyd_scan_start(struct ieee80211com *);
static void	zyd_scan_end(struct ieee80211com *);
static void	zyd_set_channel(struct ieee80211com *);
static void	zyd_wakeup(struct zyd_softc *);
static int	zyd_rfmd_init(struct zyd_rf *);
static int	zyd_rfmd_switch_radio(struct zyd_rf *, int);
static int	zyd_rfmd_set_channel(struct zyd_rf *, uint8_t);
static int	zyd_al2230_init(struct zyd_rf *);
static int	zyd_al2230_switch_radio(struct zyd_rf *, int);
static int	zyd_al2230_set_channel(struct zyd_rf *, uint8_t);
static int	zyd_al2230_set_channel_b(struct zyd_rf *, uint8_t);
static int	zyd_al2230_init_b(struct zyd_rf *);
static int	zyd_al7230B_init(struct zyd_rf *);
static int	zyd_al7230B_switch_radio(struct zyd_rf *, int);
static int	zyd_al7230B_set_channel(struct zyd_rf *, uint8_t);
static int	zyd_al2210_init(struct zyd_rf *);
static int	zyd_al2210_switch_radio(struct zyd_rf *, int);
static int	zyd_al2210_set_channel(struct zyd_rf *, uint8_t);
static int	zyd_gct_init(struct zyd_rf *);
static int	zyd_gct_switch_radio(struct zyd_rf *, int);
static int	zyd_gct_set_channel(struct zyd_rf *, uint8_t);
static int	zyd_maxim_init(struct zyd_rf *);
static int	zyd_maxim_switch_radio(struct zyd_rf *, int);
static int	zyd_maxim_set_channel(struct zyd_rf *, uint8_t);
static int	zyd_maxim2_init(struct zyd_rf *);
static int	zyd_maxim2_switch_radio(struct zyd_rf *, int);
static int	zyd_maxim2_set_channel(struct zyd_rf *, uint8_t);

static int
zyd_match(device_t dev)
{
	struct usb_attach_arg *uaa = device_get_ivars(dev);

	if (!uaa->iface)
		return (UMATCH_NONE);

	return (zyd_lookup(uaa->vendor, uaa->product) != NULL) ?
	    (UMATCH_VENDOR_PRODUCT) : (UMATCH_NONE);
}

static int
zyd_attach(device_t dev)
{
	int error = ENXIO;
	struct ieee80211com *ic;
	struct ifnet *ifp;
	struct usb_attach_arg *uaa = device_get_ivars(dev);
	struct zyd_softc *sc = device_get_softc(dev);
	usb_device_descriptor_t* ddesc;
	uint8_t bands;

	sc->sc_dev = dev;
	sc->sc_udev = uaa->device;
	sc->sc_macrev = zyd_lookup(uaa->vendor, uaa->product)->rev;
#ifdef ZYD_DEBUG
	sc->sc_debug = zyd_debug;
#endif

	ddesc = usbd_get_device_descriptor(sc->sc_udev);
	if (UGETW(ddesc->bcdDevice) < 0x4330) {
		device_printf(dev, "device version mismatch: 0x%x "
		    "(only >= 43.30 supported)\n",
		    UGETW(ddesc->bcdDevice));
		return (ENXIO);
	}

	if ((error = zyd_get_macaddr(sc)) != 0) {
		device_printf(sc->sc_dev, "could not read EEPROM\n");
		return (ENXIO);
	}

	mtx_init(&sc->sc_txmtx, device_get_nameunit(sc->sc_dev),
	    MTX_NETWORK_LOCK, MTX_DEF);
	usb_init_task(&sc->sc_mcasttask, zyd_set_multi, sc);
	usb_init_task(&sc->sc_scantask, zyd_scantask, sc);
	usb_init_task(&sc->sc_task, zyd_task, sc);
	callout_init(&sc->sc_watchdog_ch, 0);
	STAILQ_INIT(&sc->sc_rqh);

	ifp = sc->sc_ifp = if_alloc(IFT_IEEE80211);
	if (ifp == NULL) {
		device_printf(dev, "can not if_alloc()\n");
		error = ENXIO;
		goto fail0;
	}
	ifp->if_softc = sc;
	if_initname(ifp, "zyd", device_get_unit(sc->sc_dev));
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST |
	    IFF_NEEDSGIANT; /* USB stack is still under Giant lock */
	ifp->if_init = zyd_init;
	ifp->if_ioctl = zyd_ioctl;
	ifp->if_start = zyd_start;
	IFQ_SET_MAXLEN(&ifp->if_snd, IFQ_MAXLEN);
	IFQ_SET_READY(&ifp->if_snd);

	ic = ifp->if_l2com;
	ic->ic_ifp = ifp;
	ic->ic_phytype = IEEE80211_T_OFDM;	/* not only, but not used */
	ic->ic_opmode = IEEE80211_M_STA;
	IEEE80211_ADDR_COPY(ic->ic_myaddr, sc->sc_bssid);

	/* set device capabilities */
	ic->ic_caps =
		  IEEE80211_C_STA		/* station mode */
		| IEEE80211_C_MONITOR		/* monitor mode */
		| IEEE80211_C_SHPREAMBLE	/* short preamble supported */
	        | IEEE80211_C_SHSLOT		/* short slot time supported */
		| IEEE80211_C_BGSCAN		/* capable of bg scanning */
	        | IEEE80211_C_WPA		/* 802.11i */
		;

	bands = 0;
	setbit(&bands, IEEE80211_MODE_11B);
	setbit(&bands, IEEE80211_MODE_11G);
	ieee80211_init_channels(ic, NULL, &bands);

	ieee80211_ifattach(ic);
	ic->ic_newassoc = zyd_newassoc;
	ic->ic_raw_xmit = zyd_raw_xmit;
	ic->ic_node_alloc = zyd_node_alloc;
	ic->ic_scan_start = zyd_scan_start;
	ic->ic_scan_end = zyd_scan_end;
	ic->ic_set_channel = zyd_set_channel;

	ic->ic_vap_create = zyd_vap_create;
	ic->ic_vap_delete = zyd_vap_delete;
	ic->ic_update_mcast = zyd_update_mcast;

	bpfattach(ifp, DLT_IEEE802_11_RADIO,
	    sizeof(struct ieee80211_frame) + sizeof(sc->sc_txtap));
	sc->sc_rxtap_len = sizeof(sc->sc_rxtap);
	sc->sc_rxtap.wr_ihdr.it_len = htole16(sc->sc_rxtap_len);
	sc->sc_rxtap.wr_ihdr.it_present = htole32(ZYD_RX_RADIOTAP_PRESENT);
	sc->sc_txtap_len = sizeof(sc->sc_txtap);
	sc->sc_txtap.wt_ihdr.it_len = htole16(sc->sc_txtap_len);
	sc->sc_txtap.wt_ihdr.it_present = htole32(ZYD_TX_RADIOTAP_PRESENT);

	if (bootverbose)
		ieee80211_announce(ic);

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev, sc->sc_dev);

	return (0);

fail0:	mtx_destroy(&sc->sc_txmtx);
	return (error);
}

static int
zyd_detach(device_t dev)
{
	struct zyd_softc *sc = device_get_softc(dev);
	struct ifnet *ifp = sc->sc_ifp;
	struct ieee80211com *ic = ifp->if_l2com;

	if (!device_is_attached(dev))
		return (0);

	/* set a flag to indicate we're detaching.  */
	sc->sc_flags |= ZYD_FLAG_DETACHING;

	zyd_stop(sc, 1);
	bpfdetach(ifp);
	ieee80211_ifdetach(ic);

	zyd_wakeup(sc);
	zyd_close_pipes(sc);

	if_free(ifp);
	mtx_destroy(&sc->sc_txmtx);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev, sc->sc_dev);

	return (0);
}

static struct ieee80211vap *
zyd_vap_create(struct ieee80211com *ic,
	const char name[IFNAMSIZ], int unit, int opmode, int flags,
	const uint8_t bssid[IEEE80211_ADDR_LEN],
	const uint8_t mac[IEEE80211_ADDR_LEN])
{
	struct zyd_vap *zvp;
	struct ieee80211vap *vap;

	if (!TAILQ_EMPTY(&ic->ic_vaps))		/* only one at a time */
		return (NULL);
	zvp = (struct zyd_vap *) malloc(sizeof(struct zyd_vap),
	    M_80211_VAP, M_NOWAIT | M_ZERO);
	if (zvp == NULL)
		return (NULL);
	vap = &zvp->vap;
	/* enable s/w bmiss handling for sta mode */
	ieee80211_vap_setup(ic, vap, name, unit, opmode,
	    flags | IEEE80211_CLONE_NOBEACONS, bssid, mac);

	/* override state transition machine */
	zvp->newstate = vap->iv_newstate;
	vap->iv_newstate = zyd_newstate;

	ieee80211_amrr_init(&zvp->amrr, vap,
	    IEEE80211_AMRR_MIN_SUCCESS_THRESHOLD,
	    IEEE80211_AMRR_MAX_SUCCESS_THRESHOLD,
	    1000 /* 1 sec */);

	/* complete setup */
	ieee80211_vap_attach(vap, ieee80211_media_change,
	    ieee80211_media_status);
	ic->ic_opmode = opmode;
	return (vap);
}

static void
zyd_vap_delete(struct ieee80211vap *vap)
{
	struct zyd_vap *zvp = ZYD_VAP(vap);

	ieee80211_amrr_cleanup(&zvp->amrr);
	ieee80211_vap_detach(vap);
	free(zvp, M_80211_VAP);
}

static int
zyd_open_pipes(struct zyd_softc *sc)
{
	usb_endpoint_descriptor_t *edesc;
	int isize;
	usbd_status error;

	/* interrupt in */
	edesc = usbd_get_endpoint_descriptor(sc->sc_iface, 0x83);
	if (edesc == NULL)
		return (EINVAL);

	isize = UGETW(edesc->wMaxPacketSize);
	if (isize == 0)	/* should not happen */
		return (EINVAL);

	sc->sc_ibuf = malloc(isize, M_USBDEV, M_NOWAIT);
	if (sc->sc_ibuf == NULL)
		return (ENOMEM);

	error = usbd_open_pipe_intr(sc->sc_iface, 0x83, USBD_SHORT_XFER_OK,
	    &sc->sc_ep[ZYD_ENDPT_IIN], sc, sc->sc_ibuf, isize, zyd_intr,
	    USBD_DEFAULT_INTERVAL);
	if (error != 0) {
		device_printf(sc->sc_dev, "open rx intr pipe failed: %s\n",
		    usbd_errstr(error));
		goto fail;
	}

	/* interrupt out (not necessarily an interrupt pipe) */
	error = usbd_open_pipe(sc->sc_iface, 0x04, USBD_EXCLUSIVE_USE,
	    &sc->sc_ep[ZYD_ENDPT_IOUT]);
	if (error != 0) {
		device_printf(sc->sc_dev, "open tx intr pipe failed: %s\n",
		    usbd_errstr(error));
		goto fail;
	}

	/* bulk in */
	error = usbd_open_pipe(sc->sc_iface, 0x82, USBD_EXCLUSIVE_USE,
	    &sc->sc_ep[ZYD_ENDPT_BIN]);
	if (error != 0) {
		device_printf(sc->sc_dev, "open rx pipe failed: %s\n",
		    usbd_errstr(error));
		goto fail;
	}

	/* bulk out */
	error = usbd_open_pipe(sc->sc_iface, 0x01, USBD_EXCLUSIVE_USE,
	    &sc->sc_ep[ZYD_ENDPT_BOUT]);
	if (error != 0) {
		device_printf(sc->sc_dev, "open tx pipe failed: %s\n",
		    usbd_errstr(error));
		goto fail;
	}

	return (0);

fail:	zyd_close_pipes(sc);
	return (ENXIO);
}

static void
zyd_close_pipes(struct zyd_softc *sc)
{
	int i;

	for (i = 0; i < ZYD_ENDPT_CNT; i++) {
		if (sc->sc_ep[i] != NULL) {
			usbd_abort_pipe(sc->sc_ep[i]);
			usbd_close_pipe(sc->sc_ep[i]);
			sc->sc_ep[i] = NULL;
		}
	}
	if (sc->sc_ibuf != NULL) {
		free(sc->sc_ibuf, M_USBDEV);
		sc->sc_ibuf = NULL;
	}
}

static int
zyd_alloc_tx_list(struct zyd_softc *sc)
{
	int i, error;

	sc->sc_txqueued = 0;

	for (i = 0; i < ZYD_TX_LIST_CNT; i++) {
		struct zyd_tx_data *data = &sc->sc_txdata[i];

		data->sc = sc;	/* backpointer for callbacks */

		data->xfer = usbd_alloc_xfer(sc->sc_udev);
		if (data->xfer == NULL) {
			device_printf(sc->sc_dev,
			    "could not allocate tx xfer\n");
			error = ENOMEM;
			goto fail;
		}
		data->buf = usbd_alloc_buffer(data->xfer, ZYD_MAX_TXBUFSZ);
		if (data->buf == NULL) {
			device_printf(sc->sc_dev,
			    "could not allocate tx buffer\n");
			error = ENOMEM;
			goto fail;
		}

		/* clear Tx descriptor */
		bzero(data->buf, sizeof(struct zyd_tx_desc));
	}
	return (0);

fail:	zyd_free_tx_list(sc);
	return (error);
}

static void
zyd_free_tx_list(struct zyd_softc *sc)
{
	int i;

	for (i = 0; i < ZYD_TX_LIST_CNT; i++) {
		struct zyd_tx_data *data = &sc->sc_txdata[i];

		if (data->xfer != NULL) {
			usbd_free_xfer(data->xfer);
			data->xfer = NULL;
		}
		if (data->ni != NULL) {
			ieee80211_free_node(data->ni);
			data->ni = NULL;
		}
	}
}

static int
zyd_alloc_rx_list(struct zyd_softc *sc)
{
	int i, error;

	for (i = 0; i < ZYD_RX_LIST_CNT; i++) {
		struct zyd_rx_data *data = &sc->sc_rxdata[i];

		data->sc = sc;	/* backpointer for callbacks */

		data->xfer = usbd_alloc_xfer(sc->sc_udev);
		if (data->xfer == NULL) {
			device_printf(sc->sc_dev,
			    "could not allocate rx xfer\n");
			error = ENOMEM;
			goto fail;
		}
		data->buf = usbd_alloc_buffer(data->xfer, ZYX_MAX_RXBUFSZ);
		if (data->buf == NULL) {
			device_printf(sc->sc_dev,
			    "could not allocate rx buffer\n");
			error = ENOMEM;
			goto fail;
		}
	}
	return (0);

fail:	zyd_free_rx_list(sc);
	return (error);
}

static void
zyd_free_rx_list(struct zyd_softc *sc)
{
	int i;

	for (i = 0; i < ZYD_RX_LIST_CNT; i++) {
		struct zyd_rx_data *data = &sc->sc_rxdata[i];

		if (data->xfer != NULL) {
			usbd_free_xfer(data->xfer);
			data->xfer = NULL;
		}
	}
}

/* ARGUSED */
static struct ieee80211_node *
zyd_node_alloc(struct ieee80211vap *vap __unused,
	const uint8_t mac[IEEE80211_ADDR_LEN] __unused)
{
	struct zyd_node *zn;

	zn = malloc(sizeof(struct zyd_node), M_80211_NODE, M_NOWAIT | M_ZERO);
	return (zn != NULL) ? (&zn->ni) : (NULL);
}

static void
zyd_task(void *arg)
{
	int error;
	struct zyd_softc *sc = arg;
	struct ifnet *ifp = sc->sc_ifp;
	struct ieee80211com *ic = ifp->if_l2com;
	struct ieee80211vap *vap = TAILQ_FIRST(&ic->ic_vaps);
	struct ieee80211_node *ni = vap->iv_bss;
	struct zyd_vap *zvp = ZYD_VAP(vap);

	switch (sc->sc_state) {
	case IEEE80211_S_AUTH:
		zyd_set_chan(sc, ic->ic_curchan);
		break;
	case IEEE80211_S_RUN:
		if (vap->iv_opmode == IEEE80211_M_MONITOR)
			break;

		/* turn link LED on */
		error = zyd_set_led(sc, ZYD_LED1, 1);
		if (error != 0)
			goto fail;
		
		/* make data LED blink upon Tx */
		zyd_write32_m(sc, sc->sc_fwbase + ZYD_FW_LINK_STATUS, 1);
		
		IEEE80211_ADDR_COPY(sc->sc_bssid, ni->ni_bssid);
		zyd_set_bssid(sc, sc->sc_bssid);
		break;
	default:
		break;
	}

fail:
	IEEE80211_LOCK(ic);
	zvp->newstate(vap, sc->sc_state, sc->sc_arg);
	if (vap->iv_newstate_cb != NULL)
		vap->iv_newstate_cb(vap, sc->sc_state, sc->sc_arg);
	IEEE80211_UNLOCK(ic);
}

static int
zyd_newstate(struct ieee80211vap *vap, enum ieee80211_state nstate, int arg)
{
	struct zyd_vap *zvp = ZYD_VAP(vap);
	struct ieee80211com *ic = vap->iv_ic;
	struct zyd_softc *sc = ic->ic_ifp->if_softc;

	DPRINTF(sc, ZYD_DEBUG_STATE, "%s: %s -> %s\n", __func__,
	    ieee80211_state_name[vap->iv_state],
	    ieee80211_state_name[nstate]);

	usb_rem_task(sc->sc_udev, &sc->sc_scantask);
	usb_rem_task(sc->sc_udev, &sc->sc_task);
	callout_stop(&sc->sc_watchdog_ch);

	/* do it in a process context */
	sc->sc_state = nstate;
	sc->sc_arg = arg;

	if (nstate == IEEE80211_S_INIT) {
		zvp->newstate(vap, nstate, arg);
		return (0);
	} else {
		usb_add_task(sc->sc_udev, &sc->sc_task, USB_TASKQ_DRIVER);
		return (EINPROGRESS);
	}
}

static int
zyd_cmd(struct zyd_softc *sc, uint16_t code, const void *idata, int ilen,
    void *odata, int olen, u_int flags)
{
	usbd_xfer_handle xfer;
	struct zyd_cmd cmd;
	struct zyd_rq rq;
	uint16_t xferflags;
	usbd_status error;

	if (sc->sc_flags & ZYD_FLAG_DETACHING)
		return (ENXIO);

	if ((xfer = usbd_alloc_xfer(sc->sc_udev)) == NULL)
		return (ENOMEM);

	cmd.code = htole16(code);
	bcopy(idata, cmd.data, ilen);

	xferflags = USBD_FORCE_SHORT_XFER;
	if (!(flags & ZYD_CMD_FLAG_READ))
		xferflags |= USBD_SYNCHRONOUS;
	else {
		rq.idata = idata;
		rq.odata = odata;
		rq.len = olen / sizeof(struct zyd_pair);
		STAILQ_INSERT_TAIL(&sc->sc_rqh, &rq, rq);
	}

	usbd_setup_xfer(xfer, sc->sc_ep[ZYD_ENDPT_IOUT], 0, &cmd,
	    sizeof(uint16_t) + ilen, xferflags, ZYD_INTR_TIMEOUT, NULL);
	error = usbd_transfer(xfer);
	if (error != USBD_IN_PROGRESS && error != 0) {
		device_printf(sc->sc_dev, "could not send command (error=%s)\n",
		    usbd_errstr(error));
		(void)usbd_free_xfer(xfer);
		return (EIO);
	}
	if (!(flags & ZYD_CMD_FLAG_READ)) {
		(void)usbd_free_xfer(xfer);
		return (0);	/* write: don't wait for reply */
	}
	/* wait at most one second for command reply */
	error = tsleep(odata, PCATCH, "zydcmd", hz);
	if (error == EWOULDBLOCK)
		device_printf(sc->sc_dev, "zyd_read sleep timeout\n");
	STAILQ_REMOVE(&sc->sc_rqh, &rq, zyd_rq, rq);

	(void)usbd_free_xfer(xfer);
	return (error);
}

static int
zyd_read16(struct zyd_softc *sc, uint16_t reg, uint16_t *val)
{
	struct zyd_pair tmp;
	int error;

	reg = htole16(reg);
	error = zyd_cmd(sc, ZYD_CMD_IORD, &reg, sizeof(reg), &tmp, sizeof(tmp),
	    ZYD_CMD_FLAG_READ);
	if (error == 0)
		*val = le16toh(tmp.val);
	return (error);
}

static int
zyd_read32(struct zyd_softc *sc, uint16_t reg, uint32_t *val)
{
	struct zyd_pair tmp[2];
	uint16_t regs[2];
	int error;

	regs[0] = htole16(ZYD_REG32_HI(reg));
	regs[1] = htole16(ZYD_REG32_LO(reg));
	error = zyd_cmd(sc, ZYD_CMD_IORD, regs, sizeof(regs), tmp, sizeof(tmp),
	    ZYD_CMD_FLAG_READ);
	if (error == 0)
		*val = le16toh(tmp[0].val) << 16 | le16toh(tmp[1].val);
	return (error);
}

static int
zyd_write16(struct zyd_softc *sc, uint16_t reg, uint16_t val)
{
	struct zyd_pair pair;

	pair.reg = htole16(reg);
	pair.val = htole16(val);

	return zyd_cmd(sc, ZYD_CMD_IOWR, &pair, sizeof(pair), NULL, 0, 0);
}

static int
zyd_write32(struct zyd_softc *sc, uint16_t reg, uint32_t val)
{
	struct zyd_pair pair[2];

	pair[0].reg = htole16(ZYD_REG32_HI(reg));
	pair[0].val = htole16(val >> 16);
	pair[1].reg = htole16(ZYD_REG32_LO(reg));
	pair[1].val = htole16(val & 0xffff);

	return zyd_cmd(sc, ZYD_CMD_IOWR, pair, sizeof(pair), NULL, 0, 0);
}

static int
zyd_rfwrite(struct zyd_softc *sc, uint32_t val)
{
	struct zyd_rf *rf = &sc->sc_rf;
	struct zyd_rfwrite_cmd req;
	uint16_t cr203;
	int error, i;

	zyd_read16_m(sc, ZYD_CR203, &cr203);
	cr203 &= ~(ZYD_RF_IF_LE | ZYD_RF_CLK | ZYD_RF_DATA);

	req.code  = htole16(2);
	req.width = htole16(rf->width);
	for (i = 0; i < rf->width; i++) {
		req.bit[i] = htole16(cr203);
		if (val & (1 << (rf->width - 1 - i)))
			req.bit[i] |= htole16(ZYD_RF_DATA);
	}
	error = zyd_cmd(sc, ZYD_CMD_RFCFG, &req, 4 + 2 * rf->width, NULL, 0, 0);
fail:
	return (error);
}

static int
zyd_rfwrite_cr(struct zyd_softc *sc, uint32_t val)
{
	int error;

	zyd_write16_m(sc, ZYD_CR244, (val >> 16) & 0xff);
	zyd_write16_m(sc, ZYD_CR243, (val >>  8) & 0xff);
	zyd_write16_m(sc, ZYD_CR242, (val >>  0) & 0xff);
fail:
	return (error);
}

static int
zyd_lock_phy(struct zyd_softc *sc)
{
	int error;
	uint32_t tmp;

	zyd_read32_m(sc, ZYD_MAC_MISC, &tmp);
	tmp &= ~ZYD_UNLOCK_PHY_REGS;
	zyd_write32_m(sc, ZYD_MAC_MISC, tmp);
fail:
	return (error);
}

static int
zyd_unlock_phy(struct zyd_softc *sc)
{
	int error;
	uint32_t tmp;

	zyd_read32_m(sc, ZYD_MAC_MISC, &tmp);
	tmp |= ZYD_UNLOCK_PHY_REGS;
	zyd_write32_m(sc, ZYD_MAC_MISC, tmp);
fail:
	return (error);
}

/*
 * RFMD RF methods.
 */
static int
zyd_rfmd_init(struct zyd_rf *rf)
{
#define N(a)	(sizeof(a) / sizeof((a)[0]))
	struct zyd_softc *sc = rf->rf_sc;
	static const struct zyd_phy_pair phyini[] = ZYD_RFMD_PHY;
	static const uint32_t rfini[] = ZYD_RFMD_RF;
	int i, error;

	/* init RF-dependent PHY registers */
	for (i = 0; i < N(phyini); i++) {
		zyd_write16_m(sc, phyini[i].reg, phyini[i].val);
	}

	/* init RFMD radio */
	for (i = 0; i < N(rfini); i++) {
		if ((error = zyd_rfwrite(sc, rfini[i])) != 0)
			return (error);
	}
fail:
	return (error);
#undef N
}

static int
zyd_rfmd_switch_radio(struct zyd_rf *rf, int on)
{
	int error;
	struct zyd_softc *sc = rf->rf_sc;

	zyd_write16_m(sc, ZYD_CR10, on ? 0x89 : 0x15);
	zyd_write16_m(sc, ZYD_CR11, on ? 0x00 : 0x81);
fail:
	return (error);
}

static int
zyd_rfmd_set_channel(struct zyd_rf *rf, uint8_t chan)
{
	int error;
	struct zyd_softc *sc = rf->rf_sc;
	static const struct {
		uint32_t	r1, r2;
	} rfprog[] = ZYD_RFMD_CHANTABLE;

	error = zyd_rfwrite(sc, rfprog[chan - 1].r1);
	if (error != 0)
		goto fail;
	error = zyd_rfwrite(sc, rfprog[chan - 1].r2);
	if (error != 0)
		goto fail;

fail:
	return (error);
}

/*
 * AL2230 RF methods.
 */
static int
zyd_al2230_init(struct zyd_rf *rf)
{
#define N(a)	(sizeof(a) / sizeof((a)[0]))
	struct zyd_softc *sc = rf->rf_sc;
	static const struct zyd_phy_pair phyini[] = ZYD_AL2230_PHY;
	static const struct zyd_phy_pair phy2230s[] = ZYD_AL2230S_PHY_INIT;
	static const struct zyd_phy_pair phypll[] = {
		{ ZYD_CR251, 0x2f }, { ZYD_CR251, 0x3f },
		{ ZYD_CR138, 0x28 }, { ZYD_CR203, 0x06 }
	};
	static const uint32_t rfini1[] = ZYD_AL2230_RF_PART1;
	static const uint32_t rfini2[] = ZYD_AL2230_RF_PART2;
	static const uint32_t rfini3[] = ZYD_AL2230_RF_PART3;
	int i, error;

	/* init RF-dependent PHY registers */
	for (i = 0; i < N(phyini); i++)
		zyd_write16_m(sc, phyini[i].reg, phyini[i].val);

	if (sc->sc_rfrev == ZYD_RF_AL2230S || sc->sc_al2230s != 0) {
		for (i = 0; i < N(phy2230s); i++)
			zyd_write16_m(sc, phy2230s[i].reg, phy2230s[i].val);
	}

	/* init AL2230 radio */
	for (i = 0; i < N(rfini1); i++) {
		error = zyd_rfwrite(sc, rfini1[i]);
		if (error != 0)
			goto fail;
	}

	if (sc->sc_rfrev == ZYD_RF_AL2230S || sc->sc_al2230s != 0)
		error = zyd_rfwrite(sc, 0x000824);
	else
		error = zyd_rfwrite(sc, 0x0005a4);
	if (error != 0)
		goto fail;

	for (i = 0; i < N(rfini2); i++) {
		error = zyd_rfwrite(sc, rfini2[i]);
		if (error != 0)
			goto fail;
	}

	for (i = 0; i < N(phypll); i++)
		zyd_write16_m(sc, phypll[i].reg, phypll[i].val);

	for (i = 0; i < N(rfini3); i++) {
		error = zyd_rfwrite(sc, rfini3[i]);
		if (error != 0)
			goto fail;
	}
fail:
	return (error);
#undef N
}

static int
zyd_al2230_fini(struct zyd_rf *rf)
{
#define N(a)	(sizeof(a) / sizeof((a)[0]))
	int error, i;
	struct zyd_softc *sc = rf->rf_sc;
	static const struct zyd_phy_pair phy[] = ZYD_AL2230_PHY_FINI_PART1;

	for (i = 0; i < N(phy); i++)
		zyd_write16_m(sc, phy[i].reg, phy[i].val);

	if (sc->sc_newphy != 0)
		zyd_write16_m(sc, ZYD_CR9, 0xe1);

	zyd_write16_m(sc, ZYD_CR203, 0x6);
fail:
	return (error);
#undef N
}

static int
zyd_al2230_init_b(struct zyd_rf *rf)
{
#define N(a)	(sizeof(a) / sizeof((a)[0]))
	struct zyd_softc *sc = rf->rf_sc;
	static const struct zyd_phy_pair phy1[] = ZYD_AL2230_PHY_PART1;
	static const struct zyd_phy_pair phy2[] = ZYD_AL2230_PHY_PART2;
	static const struct zyd_phy_pair phy3[] = ZYD_AL2230_PHY_PART3;
	static const struct zyd_phy_pair phy2230s[] = ZYD_AL2230S_PHY_INIT;
	static const struct zyd_phy_pair phyini[] = ZYD_AL2230_PHY_B;
	static const uint32_t rfini_part1[] = ZYD_AL2230_RF_B_PART1;
	static const uint32_t rfini_part2[] = ZYD_AL2230_RF_B_PART2;
	static const uint32_t rfini_part3[] = ZYD_AL2230_RF_B_PART3;
	static const uint32_t zyd_al2230_chtable[][3] = ZYD_AL2230_CHANTABLE;
	int i, error;

	for (i = 0; i < N(phy1); i++)
		zyd_write16_m(sc, phy1[i].reg, phy1[i].val);

	/* init RF-dependent PHY registers */
	for (i = 0; i < N(phyini); i++)
		zyd_write16_m(sc, phyini[i].reg, phyini[i].val);

	if (sc->sc_rfrev == ZYD_RF_AL2230S || sc->sc_al2230s != 0) {
		for (i = 0; i < N(phy2230s); i++)
			zyd_write16_m(sc, phy2230s[i].reg, phy2230s[i].val);
	}

	for (i = 0; i < 3; i++) {
		error = zyd_rfwrite_cr(sc, zyd_al2230_chtable[0][i]);
		if (error != 0)
			return (error);
	}

	for (i = 0; i < N(rfini_part1); i++) {
		error = zyd_rfwrite_cr(sc, rfini_part1[i]);
		if (error != 0)
			return (error);
	}

	if (sc->sc_rfrev == ZYD_RF_AL2230S || sc->sc_al2230s != 0)
		error = zyd_rfwrite(sc, 0x241000);
	else
		error = zyd_rfwrite(sc, 0x25a000);
	if (error != 0)
		goto fail;

	for (i = 0; i < N(rfini_part2); i++) {
		error = zyd_rfwrite_cr(sc, rfini_part2[i]);
		if (error != 0)
			return (error);
	}

	for (i = 0; i < N(phy2); i++)
		zyd_write16_m(sc, phy2[i].reg, phy2[i].val);

	for (i = 0; i < N(rfini_part3); i++) {
		error = zyd_rfwrite_cr(sc, rfini_part3[i]);
		if (error != 0)
			return (error);
	}

	for (i = 0; i < N(phy3); i++)
		zyd_write16_m(sc, phy3[i].reg, phy3[i].val);

	error = zyd_al2230_fini(rf);
fail:
	return (error);
#undef N
}

static int
zyd_al2230_switch_radio(struct zyd_rf *rf, int on)
{
	struct zyd_softc *sc = rf->rf_sc;
	int error, on251 = (sc->sc_macrev == ZYD_ZD1211) ? 0x3f : 0x7f;

	zyd_write16_m(sc, ZYD_CR11,  on ? 0x00 : 0x04);
	zyd_write16_m(sc, ZYD_CR251, on ? on251 : 0x2f);
fail:
	return (error);
}

static int
zyd_al2230_set_channel(struct zyd_rf *rf, uint8_t chan)
{
#define N(a)	(sizeof(a) / sizeof((a)[0]))
	int error, i;
	struct zyd_softc *sc = rf->rf_sc;
	static const struct zyd_phy_pair phy1[] = {
		{ ZYD_CR138, 0x28 }, { ZYD_CR203, 0x06 },
	};
	static const struct {
		uint32_t	r1, r2, r3;
	} rfprog[] = ZYD_AL2230_CHANTABLE;

	error = zyd_rfwrite(sc, rfprog[chan - 1].r1);
	if (error != 0)
		goto fail;
	error = zyd_rfwrite(sc, rfprog[chan - 1].r2);
	if (error != 0)
		goto fail;
	error = zyd_rfwrite(sc, rfprog[chan - 1].r3);
	if (error != 0)
		goto fail;

	for (i = 0; i < N(phy1); i++)
		zyd_write16_m(sc, phy1[i].reg, phy1[i].val);
fail:
	return (error);
#undef N
}

static int
zyd_al2230_set_channel_b(struct zyd_rf *rf, uint8_t chan)
{
#define N(a)	(sizeof(a) / sizeof((a)[0]))
	int error, i;
	struct zyd_softc *sc = rf->rf_sc;
	static const struct zyd_phy_pair phy1[] = ZYD_AL2230_PHY_PART1;
	static const struct {
		uint32_t	r1, r2, r3;
	} rfprog[] = ZYD_AL2230_CHANTABLE_B;

	for (i = 0; i < N(phy1); i++)
		zyd_write16_m(sc, phy1[i].reg, phy1[i].val);

	error = zyd_rfwrite_cr(sc, rfprog[chan - 1].r1);
	if (error != 0)
		goto fail;
	error = zyd_rfwrite_cr(sc, rfprog[chan - 1].r2);
	if (error != 0)
		goto fail;
	error = zyd_rfwrite_cr(sc, rfprog[chan - 1].r3);
	if (error != 0)
		goto fail;
	error = zyd_al2230_fini(rf);
fail:
	return (error);
#undef N
}

#define	ZYD_AL2230_PHY_BANDEDGE6					\
{									\
	{ ZYD_CR128, 0x14 }, { ZYD_CR129, 0x12 }, { ZYD_CR130, 0x10 },	\
	{ ZYD_CR47,  0x1e }						\
}

static int
zyd_al2230_bandedge6(struct zyd_rf *rf, struct ieee80211_channel *c)
{
#define N(a)	(sizeof(a) / sizeof((a)[0]))
	int error = 0, i;
	struct zyd_softc *sc = rf->rf_sc;
	struct ifnet *ifp = sc->sc_ifp;
	struct ieee80211com *ic = ifp->if_l2com;
	struct zyd_phy_pair r[] = ZYD_AL2230_PHY_BANDEDGE6;
	u_int chan = ieee80211_chan2ieee(ic, c);

	if (chan == 1 || chan == 11)
		r[0].val = 0x12;
	
	for (i = 0; i < N(r); i++)
		zyd_write16_m(sc, r[i].reg, r[i].val);
fail:
	return (error);
#undef N
}

/*
 * AL7230B RF methods.
 */
static int
zyd_al7230B_init(struct zyd_rf *rf)
{
#define N(a)	(sizeof(a) / sizeof((a)[0]))
	struct zyd_softc *sc = rf->rf_sc;
	static const struct zyd_phy_pair phyini_1[] = ZYD_AL7230B_PHY_1;
	static const struct zyd_phy_pair phyini_2[] = ZYD_AL7230B_PHY_2;
	static const struct zyd_phy_pair phyini_3[] = ZYD_AL7230B_PHY_3;
	static const uint32_t rfini_1[] = ZYD_AL7230B_RF_1;
	static const uint32_t rfini_2[] = ZYD_AL7230B_RF_2;
	int i, error;

	/* for AL7230B, PHY and RF need to be initialized in "phases" */

	/* init RF-dependent PHY registers, part one */
	for (i = 0; i < N(phyini_1); i++)
		zyd_write16_m(sc, phyini_1[i].reg, phyini_1[i].val);

	/* init AL7230B radio, part one */
	for (i = 0; i < N(rfini_1); i++) {
		if ((error = zyd_rfwrite(sc, rfini_1[i])) != 0)
			return (error);
	}
	/* init RF-dependent PHY registers, part two */
	for (i = 0; i < N(phyini_2); i++)
		zyd_write16_m(sc, phyini_2[i].reg, phyini_2[i].val);

	/* init AL7230B radio, part two */
	for (i = 0; i < N(rfini_2); i++) {
		if ((error = zyd_rfwrite(sc, rfini_2[i])) != 0)
			return (error);
	}
	/* init RF-dependent PHY registers, part three */
	for (i = 0; i < N(phyini_3); i++)
		zyd_write16_m(sc, phyini_3[i].reg, phyini_3[i].val);
fail:
	return (error);
#undef N
}

static int
zyd_al7230B_switch_radio(struct zyd_rf *rf, int on)
{
	int error;
	struct zyd_softc *sc = rf->rf_sc;

	zyd_write16_m(sc, ZYD_CR11,  on ? 0x00 : 0x04);
	zyd_write16_m(sc, ZYD_CR251, on ? 0x3f : 0x2f);
fail:
	return (error);
}

static int
zyd_al7230B_set_channel(struct zyd_rf *rf, uint8_t chan)
{
#define N(a)	(sizeof(a) / sizeof((a)[0]))
	struct zyd_softc *sc = rf->rf_sc;
	static const struct {
		uint32_t	r1, r2;
	} rfprog[] = ZYD_AL7230B_CHANTABLE;
	static const uint32_t rfsc[] = ZYD_AL7230B_RF_SETCHANNEL;
	int i, error;

	zyd_write16_m(sc, ZYD_CR240, 0x57);
	zyd_write16_m(sc, ZYD_CR251, 0x2f);

	for (i = 0; i < N(rfsc); i++) {
		if ((error = zyd_rfwrite(sc, rfsc[i])) != 0)
			return (error);
	}

	zyd_write16_m(sc, ZYD_CR128, 0x14);
	zyd_write16_m(sc, ZYD_CR129, 0x12);
	zyd_write16_m(sc, ZYD_CR130, 0x10);
	zyd_write16_m(sc, ZYD_CR38,  0x38);
	zyd_write16_m(sc, ZYD_CR136, 0xdf);

	error = zyd_rfwrite(sc, rfprog[chan - 1].r1);
	if (error != 0)
		goto fail;
	error = zyd_rfwrite(sc, rfprog[chan - 1].r2);
	if (error != 0)
		goto fail;
	error = zyd_rfwrite(sc, 0x3c9000);
	if (error != 0)
		goto fail;

	zyd_write16_m(sc, ZYD_CR251, 0x3f);
	zyd_write16_m(sc, ZYD_CR203, 0x06);
	zyd_write16_m(sc, ZYD_CR240, 0x08);
fail:
	return (error);
#undef N
}

/*
 * AL2210 RF methods.
 */
static int
zyd_al2210_init(struct zyd_rf *rf)
{
#define N(a)	(sizeof(a) / sizeof((a)[0]))
	struct zyd_softc *sc = rf->rf_sc;
	static const struct zyd_phy_pair phyini[] = ZYD_AL2210_PHY;
	static const uint32_t rfini[] = ZYD_AL2210_RF;
	uint32_t tmp;
	int i, error;

	zyd_write32_m(sc, ZYD_CR18, 2);

	/* init RF-dependent PHY registers */
	for (i = 0; i < N(phyini); i++)
		zyd_write16_m(sc, phyini[i].reg, phyini[i].val);

	/* init AL2210 radio */
	for (i = 0; i < N(rfini); i++) {
		if ((error = zyd_rfwrite(sc, rfini[i])) != 0)
			return (error);
	}
	zyd_write16_m(sc, ZYD_CR47, 0x1e);
	zyd_read32_m(sc, ZYD_CR_RADIO_PD, &tmp);
	zyd_write32_m(sc, ZYD_CR_RADIO_PD, tmp & ~1);
	zyd_write32_m(sc, ZYD_CR_RADIO_PD, tmp | 1);
	zyd_write32_m(sc, ZYD_CR_RFCFG, 0x05);
	zyd_write32_m(sc, ZYD_CR_RFCFG, 0x00);
	zyd_write16_m(sc, ZYD_CR47, 0x1e);
	zyd_write32_m(sc, ZYD_CR18, 3);
fail:
	return (error);
#undef N
}

static int
zyd_al2210_switch_radio(struct zyd_rf *rf, int on)
{
	/* vendor driver does nothing for this RF chip */

	return (0);
}

static int
zyd_al2210_set_channel(struct zyd_rf *rf, uint8_t chan)
{
	int error;
	struct zyd_softc *sc = rf->rf_sc;
	static const uint32_t rfprog[] = ZYD_AL2210_CHANTABLE;
	uint32_t tmp;

	zyd_write32_m(sc, ZYD_CR18, 2);
	zyd_write16_m(sc, ZYD_CR47, 0x1e);
	zyd_read32_m(sc, ZYD_CR_RADIO_PD, &tmp);
	zyd_write32_m(sc, ZYD_CR_RADIO_PD, tmp & ~1);
	zyd_write32_m(sc, ZYD_CR_RADIO_PD, tmp | 1);
	zyd_write32_m(sc, ZYD_CR_RFCFG, 0x05);
	zyd_write32_m(sc, ZYD_CR_RFCFG, 0x00);
	zyd_write16_m(sc, ZYD_CR47, 0x1e);

	/* actually set the channel */
	error = zyd_rfwrite(sc, rfprog[chan - 1]);
	if (error != 0)
		goto fail;

	zyd_write32_m(sc, ZYD_CR18, 3);
fail:
	return (error);
}

/*
 * GCT RF methods.
 */
static int
zyd_gct_init(struct zyd_rf *rf)
{
#define N(a)	(sizeof(a) / sizeof((a)[0]))
	struct zyd_softc *sc = rf->rf_sc;
	static const struct zyd_phy_pair phyini[] = ZYD_GCT_PHY;
	static const uint32_t rfini[] = ZYD_GCT_RF;
	int i, error;

	/* init RF-dependent PHY registers */
	for (i = 0; i < N(phyini); i++)
		zyd_write16_m(sc, phyini[i].reg, phyini[i].val);

	/* init cgt radio */
	for (i = 0; i < N(rfini); i++) {
		if ((error = zyd_rfwrite(sc, rfini[i])) != 0)
			return (error);
	}
fail:
	return (error);
#undef N
}

static int
zyd_gct_switch_radio(struct zyd_rf *rf, int on)
{
	/* vendor driver does nothing for this RF chip */

	return (0);
}

static int
zyd_gct_set_channel(struct zyd_rf *rf, uint8_t chan)
{
	int error;
	struct zyd_softc *sc = rf->rf_sc;
	static const uint32_t rfprog[] = ZYD_GCT_CHANTABLE;

	error = zyd_rfwrite(sc, 0x1c0000);
	if (error != 0)
		goto fail;
	error = zyd_rfwrite(sc, rfprog[chan - 1]);
	if (error != 0)
		goto fail;
	error = zyd_rfwrite(sc, 0x1c0008);
fail:
	return (error);
}

/*
 * Maxim RF methods.
 */
static int
zyd_maxim_init(struct zyd_rf *rf)
{
#define N(a)	(sizeof(a) / sizeof((a)[0]))
	struct zyd_softc *sc = rf->rf_sc;
	static const struct zyd_phy_pair phyini[] = ZYD_MAXIM_PHY;
	static const uint32_t rfini[] = ZYD_MAXIM_RF;
	uint16_t tmp;
	int i, error;

	/* init RF-dependent PHY registers */
	for (i = 0; i < N(phyini); i++)
		zyd_write16_m(sc, phyini[i].reg, phyini[i].val);

	zyd_read16_m(sc, ZYD_CR203, &tmp);
	zyd_write16_m(sc, ZYD_CR203, tmp & ~(1 << 4));

	/* init maxim radio */
	for (i = 0; i < N(rfini); i++) {
		if ((error = zyd_rfwrite(sc, rfini[i])) != 0)
			return (error);
	}
	zyd_read16_m(sc, ZYD_CR203, &tmp);
	zyd_write16_m(sc, ZYD_CR203, tmp | (1 << 4));
fail:
	return (error);
#undef N
}

static int
zyd_maxim_switch_radio(struct zyd_rf *rf, int on)
{

	/* vendor driver does nothing for this RF chip */
	return (0);
}

static int
zyd_maxim_set_channel(struct zyd_rf *rf, uint8_t chan)
{
#define N(a)	(sizeof(a) / sizeof((a)[0]))
	struct zyd_softc *sc = rf->rf_sc;
	static const struct zyd_phy_pair phyini[] = ZYD_MAXIM_PHY;
	static const uint32_t rfini[] = ZYD_MAXIM_RF;
	static const struct {
		uint32_t	r1, r2;
	} rfprog[] = ZYD_MAXIM_CHANTABLE;
	uint16_t tmp;
	int i, error;

	/*
	 * Do the same as we do when initializing it, except for the channel
	 * values coming from the two channel tables.
	 */

	/* init RF-dependent PHY registers */
	for (i = 0; i < N(phyini); i++)
		zyd_write16_m(sc, phyini[i].reg, phyini[i].val);

	zyd_read16_m(sc, ZYD_CR203, &tmp);
	zyd_write16_m(sc, ZYD_CR203, tmp & ~(1 << 4));

	/* first two values taken from the chantables */
	error = zyd_rfwrite(sc, rfprog[chan - 1].r1);
	if (error != 0)
		goto fail;
	error = zyd_rfwrite(sc, rfprog[chan - 1].r2);
	if (error != 0)
		goto fail;

	/* init maxim radio - skipping the two first values */
	for (i = 2; i < N(rfini); i++) {
		if ((error = zyd_rfwrite(sc, rfini[i])) != 0)
			return (error);
	}
	zyd_read16_m(sc, ZYD_CR203, &tmp);
	zyd_write16_m(sc, ZYD_CR203, tmp | (1 << 4));
fail:
	return (error);
#undef N
}

/*
 * Maxim2 RF methods.
 */
static int
zyd_maxim2_init(struct zyd_rf *rf)
{
#define N(a)	(sizeof(a) / sizeof((a)[0]))
	struct zyd_softc *sc = rf->rf_sc;
	static const struct zyd_phy_pair phyini[] = ZYD_MAXIM2_PHY;
	static const uint32_t rfini[] = ZYD_MAXIM2_RF;
	uint16_t tmp;
	int i, error;

	/* init RF-dependent PHY registers */
	for (i = 0; i < N(phyini); i++)
		zyd_write16_m(sc, phyini[i].reg, phyini[i].val);

	zyd_read16_m(sc, ZYD_CR203, &tmp);
	zyd_write16_m(sc, ZYD_CR203, tmp & ~(1 << 4));

	/* init maxim2 radio */
	for (i = 0; i < N(rfini); i++) {
		if ((error = zyd_rfwrite(sc, rfini[i])) != 0)
			return (error);
	}
	zyd_read16_m(sc, ZYD_CR203, &tmp);
	zyd_write16_m(sc, ZYD_CR203, tmp | (1 << 4));
fail:
	return (error);
#undef N
}

static int
zyd_maxim2_switch_radio(struct zyd_rf *rf, int on)
{

	/* vendor driver does nothing for this RF chip */
	return (0);
}

static int
zyd_maxim2_set_channel(struct zyd_rf *rf, uint8_t chan)
{
#define N(a)	(sizeof(a) / sizeof((a)[0]))
	struct zyd_softc *sc = rf->rf_sc;
	static const struct zyd_phy_pair phyini[] = ZYD_MAXIM2_PHY;
	static const uint32_t rfini[] = ZYD_MAXIM2_RF;
	static const struct {
		uint32_t	r1, r2;
	} rfprog[] = ZYD_MAXIM2_CHANTABLE;
	uint16_t tmp;
	int i, error;

	/*
	 * Do the same as we do when initializing it, except for the channel
	 * values coming from the two channel tables.
	 */

	/* init RF-dependent PHY registers */
	for (i = 0; i < N(phyini); i++)
		zyd_write16_m(sc, phyini[i].reg, phyini[i].val);

	zyd_read16_m(sc, ZYD_CR203, &tmp);
	zyd_write16_m(sc, ZYD_CR203, tmp & ~(1 << 4));

	/* first two values taken from the chantables */
	error = zyd_rfwrite(sc, rfprog[chan - 1].r1);
	if (error != 0)
		goto fail;
	error = zyd_rfwrite(sc, rfprog[chan - 1].r2);
	if (error != 0)
		goto fail;

	/* init maxim2 radio - skipping the two first values */
	for (i = 2; i < N(rfini); i++) {
		if ((error = zyd_rfwrite(sc, rfini[i])) != 0)
			return (error);
	}
	zyd_read16_m(sc, ZYD_CR203, &tmp);
	zyd_write16_m(sc, ZYD_CR203, tmp | (1 << 4));
fail:
	return (error);
#undef N
}

static int
zyd_rf_attach(struct zyd_softc *sc, uint8_t type)
{
	struct zyd_rf *rf = &sc->sc_rf;

	rf->rf_sc = sc;

	switch (type) {
	case ZYD_RF_RFMD:
		rf->init         = zyd_rfmd_init;
		rf->switch_radio = zyd_rfmd_switch_radio;
		rf->set_channel  = zyd_rfmd_set_channel;
		rf->width        = 24;	/* 24-bit RF values */
		break;
	case ZYD_RF_AL2230:
	case ZYD_RF_AL2230S:
		if (sc->sc_macrev == ZYD_ZD1211B) {
			rf->init = zyd_al2230_init_b;
			rf->set_channel = zyd_al2230_set_channel_b;
		} else {
			rf->init = zyd_al2230_init;
			rf->set_channel = zyd_al2230_set_channel;
		}
		rf->switch_radio = zyd_al2230_switch_radio;
		rf->bandedge6	 = zyd_al2230_bandedge6;
		rf->width        = 24;	/* 24-bit RF values */
		break;
	case ZYD_RF_AL7230B:
		rf->init         = zyd_al7230B_init;
		rf->switch_radio = zyd_al7230B_switch_radio;
		rf->set_channel  = zyd_al7230B_set_channel;
		rf->width        = 24;	/* 24-bit RF values */
		break;
	case ZYD_RF_AL2210:
		rf->init         = zyd_al2210_init;
		rf->switch_radio = zyd_al2210_switch_radio;
		rf->set_channel  = zyd_al2210_set_channel;
		rf->width        = 24;	/* 24-bit RF values */
		break;
	case ZYD_RF_GCT:
		rf->init         = zyd_gct_init;
		rf->switch_radio = zyd_gct_switch_radio;
		rf->set_channel  = zyd_gct_set_channel;
		rf->width        = 21;	/* 21-bit RF values */
		break;
	case ZYD_RF_MAXIM_NEW:
		rf->init         = zyd_maxim_init;
		rf->switch_radio = zyd_maxim_switch_radio;
		rf->set_channel  = zyd_maxim_set_channel;
		rf->width        = 18;	/* 18-bit RF values */
		break;
	case ZYD_RF_MAXIM_NEW2:
		rf->init         = zyd_maxim2_init;
		rf->switch_radio = zyd_maxim2_switch_radio;
		rf->set_channel  = zyd_maxim2_set_channel;
		rf->width        = 18;	/* 18-bit RF values */
		break;
	default:
		device_printf(sc->sc_dev,
		    "sorry, radio \"%s\" is not supported yet\n",
		    zyd_rf_name(type));
		return (EINVAL);
	}
	return (0);
}

static const char *
zyd_rf_name(uint8_t type)
{
	static const char * const zyd_rfs[] = {
		"unknown", "unknown", "UW2451",   "UCHIP",     "AL2230",
		"AL7230B", "THETA",   "AL2210",   "MAXIM_NEW", "GCT",
		"AL2230S",  "RALINK",  "INTERSIL", "RFMD",      "MAXIM_NEW2",
		"PHILIPS"
	};

	return zyd_rfs[(type > 15) ? 0 : type];
}

static int
zyd_hw_init(struct zyd_softc *sc)
{
	int error;
	const struct zyd_phy_pair *phyp;
	struct zyd_rf *rf = &sc->sc_rf;
	uint16_t val;

	/* specify that the plug and play is finished */
	zyd_write32_m(sc, ZYD_MAC_AFTER_PNP, 1);
	zyd_read16_m(sc, ZYD_FIRMWARE_BASE_ADDR, &sc->sc_fwbase);
	DPRINTF(sc, ZYD_DEBUG_FW, "firmware base address=0x%04x\n",
	    sc->sc_fwbase);

	/* retrieve firmware revision number */
	zyd_read16_m(sc, sc->sc_fwbase + ZYD_FW_FIRMWARE_REV, &sc->sc_fwrev);
	zyd_write32_m(sc, ZYD_CR_GPI_EN, 0);
	zyd_write32_m(sc, ZYD_MAC_CONT_WIN_LIMIT, 0x7f043f);
	/* set mandatory rates - XXX assumes 802.11b/g */
	zyd_write32_m(sc, ZYD_MAC_MAN_RATE, 0x150f);

	/* disable interrupts */
	zyd_write32_m(sc, ZYD_CR_INTERRUPT, 0);

	if ((error = zyd_read_pod(sc)) != 0) {
		device_printf(sc->sc_dev, "could not read EEPROM\n");
		goto fail;
	}

	/* PHY init (resetting) */
	error = zyd_lock_phy(sc);
	if (error != 0)
		goto fail;
	phyp = (sc->sc_macrev == ZYD_ZD1211B) ? zyd_def_phyB : zyd_def_phy;
	for (; phyp->reg != 0; phyp++)
		zyd_write16_m(sc, phyp->reg, phyp->val);
	if (sc->sc_macrev == ZYD_ZD1211 && sc->sc_fix_cr157 != 0) {
		zyd_read16_m(sc, ZYD_EEPROM_PHY_REG, &val);
		zyd_write32_m(sc, ZYD_CR157, val >> 8);
	}
	error = zyd_unlock_phy(sc);
	if (error != 0)
		goto fail;

	/* HMAC init */
	zyd_write32_m(sc, ZYD_MAC_ACK_EXT, 0x00000020);
	zyd_write32_m(sc, ZYD_CR_ADDA_MBIAS_WT, 0x30000808);
	zyd_write32_m(sc, ZYD_MAC_SNIFFER, 0x00000000);
	zyd_write32_m(sc, ZYD_MAC_RXFILTER, 0x00000000);
	zyd_write32_m(sc, ZYD_MAC_GHTBL, 0x00000000);
	zyd_write32_m(sc, ZYD_MAC_GHTBH, 0x80000000);
	zyd_write32_m(sc, ZYD_MAC_MISC, 0x000000a4);
	zyd_write32_m(sc, ZYD_CR_ADDA_PWR_DWN, 0x0000007f);
	zyd_write32_m(sc, ZYD_MAC_BCNCFG, 0x00f00401);
	zyd_write32_m(sc, ZYD_MAC_PHY_DELAY2, 0x00000000);
	zyd_write32_m(sc, ZYD_MAC_ACK_EXT, 0x00000080);
	zyd_write32_m(sc, ZYD_CR_ADDA_PWR_DWN, 0x00000000);
	zyd_write32_m(sc, ZYD_MAC_SIFS_ACK_TIME, 0x00000100);
	zyd_write32_m(sc, ZYD_CR_RX_PE_DELAY, 0x00000070);
	zyd_write32_m(sc, ZYD_CR_PS_CTRL, 0x10000000);
	zyd_write32_m(sc, ZYD_MAC_RTSCTSRATE, 0x02030203);
	zyd_write32_m(sc, ZYD_MAC_AFTER_PNP, 1);
	zyd_write32_m(sc, ZYD_MAC_BACKOFF_PROTECT, 0x00000114);
	zyd_write32_m(sc, ZYD_MAC_DIFS_EIFS_SIFS, 0x0a47c032);
	zyd_write32_m(sc, ZYD_MAC_CAM_MODE, 0x3);

	if (sc->sc_macrev == ZYD_ZD1211) {
		zyd_write32_m(sc, ZYD_MAC_RETRY, 0x00000002);
		zyd_write32_m(sc, ZYD_MAC_RX_THRESHOLD, 0x000c0640);
	} else {
		zyd_write32_m(sc, ZYD_MACB_MAX_RETRY, 0x02020202);
		zyd_write32_m(sc, ZYD_MACB_TXPWR_CTL4, 0x007f003f);
		zyd_write32_m(sc, ZYD_MACB_TXPWR_CTL3, 0x007f003f);
		zyd_write32_m(sc, ZYD_MACB_TXPWR_CTL2, 0x003f001f);
		zyd_write32_m(sc, ZYD_MACB_TXPWR_CTL1, 0x001f000f);
		zyd_write32_m(sc, ZYD_MACB_AIFS_CTL1, 0x00280028);
		zyd_write32_m(sc, ZYD_MACB_AIFS_CTL2, 0x008C003C);
		zyd_write32_m(sc, ZYD_MACB_TXOP, 0x01800824);
		zyd_write32_m(sc, ZYD_MAC_RX_THRESHOLD, 0x000c0eff);
	}

	/* init beacon interval to 100ms */
	if ((error = zyd_set_beacon_interval(sc, 100)) != 0)
		goto fail;

	if ((error = zyd_rf_attach(sc, sc->sc_rfrev)) != 0) {
		device_printf(sc->sc_dev, "could not attach RF, rev 0x%x\n",
		    sc->sc_rfrev);
		goto fail;
	}

	/* RF chip init */
	error = zyd_lock_phy(sc);
	if (error != 0)
		goto fail;
	error = (*rf->init)(rf);
	if (error != 0) {
		device_printf(sc->sc_dev,
		    "radio initialization failed, error %d\n", error);
		goto fail;
	}
	error = zyd_unlock_phy(sc);
	if (error != 0)
		goto fail;

	if ((error = zyd_read_eeprom(sc)) != 0) {
		device_printf(sc->sc_dev, "could not read EEPROM\n");
		goto fail;
	}

fail:	return (error);
}

static int
zyd_read_pod(struct zyd_softc *sc)
{
	int error;
	uint32_t tmp;

	zyd_read32_m(sc, ZYD_EEPROM_POD, &tmp);
	sc->sc_rfrev     = tmp & 0x0f;
	sc->sc_ledtype   = (tmp >>  4) & 0x01;
	sc->sc_al2230s   = (tmp >>  7) & 0x01;
	sc->sc_cckgain   = (tmp >>  8) & 0x01;
	sc->sc_fix_cr157 = (tmp >> 13) & 0x01;
	sc->sc_parev     = (tmp >> 16) & 0x0f;
	sc->sc_bandedge6 = (tmp >> 21) & 0x01;
	sc->sc_newphy    = (tmp >> 31) & 0x01;
	sc->sc_txled     = ((tmp & (1 << 24)) && (tmp & (1 << 29))) ? 0 : 1;
fail:
	return (error);
}

static int
zyd_read_eeprom(struct zyd_softc *sc)
{
	uint16_t val;
	int error, i;

	/* read Tx power calibration tables */
	for (i = 0; i < 7; i++) {
		zyd_read16_m(sc, ZYD_EEPROM_PWR_CAL + i, &val);
		sc->sc_pwrcal[i * 2] = val >> 8;
		sc->sc_pwrcal[i * 2 + 1] = val & 0xff;
		zyd_read16_m(sc, ZYD_EEPROM_PWR_INT + i, &val);
		sc->sc_pwrint[i * 2] = val >> 8;
		sc->sc_pwrint[i * 2 + 1] = val & 0xff;
		zyd_read16_m(sc, ZYD_EEPROM_36M_CAL + i, &val);
		sc->sc_ofdm36_cal[i * 2] = val >> 8;
		sc->sc_ofdm36_cal[i * 2 + 1] = val & 0xff;
		zyd_read16_m(sc, ZYD_EEPROM_48M_CAL + i, &val);
		sc->sc_ofdm48_cal[i * 2] = val >> 8;
		sc->sc_ofdm48_cal[i * 2 + 1] = val & 0xff;
		zyd_read16_m(sc, ZYD_EEPROM_54M_CAL + i, &val);
		sc->sc_ofdm54_cal[i * 2] = val >> 8;
		sc->sc_ofdm54_cal[i * 2 + 1] = val & 0xff;
	}
fail:
	return (error);
}

static int
zyd_get_macaddr(struct zyd_softc *sc)
{
	usb_device_request_t req;
	usbd_status error;

	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = ZYD_READFWDATAREQ;
	USETW(req.wValue, ZYD_EEPROM_MAC_ADDR_P1);
	USETW(req.wIndex, 0);
	USETW(req.wLength, IEEE80211_ADDR_LEN);

	error = usbd_do_request(sc->sc_udev, &req, sc->sc_bssid);
	if (error != 0) {
		device_printf(sc->sc_dev, "could not read EEPROM: %s\n",
		    usbd_errstr(error));
	}

	return (error);
}

static int
zyd_set_macaddr(struct zyd_softc *sc, const uint8_t *addr)
{
	int error;
	uint32_t tmp;

	tmp = addr[3] << 24 | addr[2] << 16 | addr[1] << 8 | addr[0];
	zyd_write32_m(sc, ZYD_MAC_MACADRL, tmp);
	tmp = addr[5] << 8 | addr[4];
	zyd_write32_m(sc, ZYD_MAC_MACADRH, tmp);
fail:
	return (error);
}

static int
zyd_set_bssid(struct zyd_softc *sc, const uint8_t *addr)
{
	int error;
	uint32_t tmp;

	tmp = addr[3] << 24 | addr[2] << 16 | addr[1] << 8 | addr[0];
	zyd_write32_m(sc, ZYD_MAC_BSSADRL, tmp);
	tmp = addr[5] << 8 | addr[4];
	zyd_write32_m(sc, ZYD_MAC_BSSADRH, tmp);
fail:
	return (error);
}

static int
zyd_switch_radio(struct zyd_softc *sc, int on)
{
	struct zyd_rf *rf = &sc->sc_rf;
	int error;

	error = zyd_lock_phy(sc);
	if (error != 0)
		goto fail;
	error = (*rf->switch_radio)(rf, on);
	if (error != 0)
		goto fail;
	error = zyd_unlock_phy(sc);
fail:
	return (error);
}

static int
zyd_set_led(struct zyd_softc *sc, int which, int on)
{
	int error;
	uint32_t tmp;

	zyd_read32_m(sc, ZYD_MAC_TX_PE_CONTROL, &tmp);
	tmp &= ~which;
	if (on)
		tmp |= which;
	zyd_write32_m(sc, ZYD_MAC_TX_PE_CONTROL, tmp);
fail:
	return (error);
}

static void
zyd_set_multi(void *arg)
{
	int error;
	struct zyd_softc *sc = arg;
	struct ifnet *ifp = sc->sc_ifp;
	struct ieee80211com *ic = ifp->if_l2com;
	struct ifmultiaddr *ifma;
	uint32_t low, high;
	uint8_t v;

	if (!(ifp->if_flags & IFF_UP))
		return;

	low = 0x00000000;
	high = 0x80000000;

	if (ic->ic_opmode == IEEE80211_M_MONITOR ||
	    (ifp->if_flags & (IFF_ALLMULTI | IFF_PROMISC))) {
		low = 0xffffffff;
		high = 0xffffffff;
	} else {
		IF_ADDR_LOCK(ifp);
		TAILQ_FOREACH(ifma, &ifp->if_multiaddrs, ifma_link) {
			if (ifma->ifma_addr->sa_family != AF_LINK)
				continue;
			v = ((uint8_t *)LLADDR((struct sockaddr_dl *)
			    ifma->ifma_addr))[5] >> 2;
			if (v < 32)
				low |= 1 << v;
			else
				high |= 1 << (v - 32);
		}
		IF_ADDR_UNLOCK(ifp);
	}

	/* reprogram multicast global hash table */
	zyd_write32_m(sc, ZYD_MAC_GHTBL, low);
	zyd_write32_m(sc, ZYD_MAC_GHTBH, high);
fail:
	if (error != 0)
		device_printf(sc->sc_dev,
		    "could not set multicast hash table\n");
}

static void
zyd_update_mcast(struct ifnet *ifp)
{
	struct zyd_softc *sc = ifp->if_softc;

	if (!(sc->sc_flags & ZYD_FLAG_INITDONE))
		return;

	usb_add_task(sc->sc_udev, &sc->sc_mcasttask, USB_TASKQ_DRIVER);
}

static int
zyd_set_rxfilter(struct zyd_softc *sc)
{
	struct ifnet *ifp = sc->sc_ifp;
	struct ieee80211com *ic = ifp->if_l2com;
	uint32_t rxfilter;

	switch (ic->ic_opmode) {
	case IEEE80211_M_STA:
		rxfilter = ZYD_FILTER_BSS;
		break;
	case IEEE80211_M_IBSS:
	case IEEE80211_M_HOSTAP:
		rxfilter = ZYD_FILTER_HOSTAP;
		break;
	case IEEE80211_M_MONITOR:
		rxfilter = ZYD_FILTER_MONITOR;
		break;
	default:
		/* should not get there */
		return (EINVAL);
	}
	return zyd_write32(sc, ZYD_MAC_RXFILTER, rxfilter);
}

static void
zyd_set_chan(struct zyd_softc *sc, struct ieee80211_channel *c)
{
	int error;
	struct ifnet *ifp = sc->sc_ifp;
	struct ieee80211com *ic = ifp->if_l2com;
	struct zyd_rf *rf = &sc->sc_rf;
	uint32_t tmp;
	u_int chan;

	chan = ieee80211_chan2ieee(ic, c);
	if (chan == 0 || chan == IEEE80211_CHAN_ANY) {
		/* XXX should NEVER happen */
		device_printf(sc->sc_dev,
		    "%s: invalid channel %x\n", __func__, chan);
		return;
	}

	error = zyd_lock_phy(sc);
	if (error != 0)
		goto fail;

	error = (*rf->set_channel)(rf, chan);
	if (error != 0)
		goto fail;

	/* update Tx power */
	zyd_write16_m(sc, ZYD_CR31, sc->sc_pwrint[chan - 1]);

	if (sc->sc_macrev == ZYD_ZD1211B) {
		zyd_write16_m(sc, ZYD_CR67, sc->sc_ofdm36_cal[chan - 1]);
		zyd_write16_m(sc, ZYD_CR66, sc->sc_ofdm48_cal[chan - 1]);
		zyd_write16_m(sc, ZYD_CR65, sc->sc_ofdm54_cal[chan - 1]);
		zyd_write16_m(sc, ZYD_CR68, sc->sc_pwrcal[chan - 1]);
		zyd_write16_m(sc, ZYD_CR69, 0x28);
		zyd_write16_m(sc, ZYD_CR69, 0x2a);
	}
	if (sc->sc_cckgain) {
		/* set CCK baseband gain from EEPROM */
		if (zyd_read32(sc, ZYD_EEPROM_PHY_REG, &tmp) == 0)
			zyd_write16_m(sc, ZYD_CR47, tmp & 0xff);
	}
	if (sc->sc_bandedge6 && rf->bandedge6 != NULL) {
		error = (*rf->bandedge6)(rf, c);
		if (error != 0)
			goto fail;
	}
	zyd_write32_m(sc, ZYD_CR_CONFIG_PHILIPS, 0);

	error = zyd_unlock_phy(sc);
	if (error != 0)
		goto fail;

	sc->sc_rxtap.wr_chan_freq = sc->sc_txtap.wt_chan_freq =
	    htole16(c->ic_freq);
	sc->sc_rxtap.wr_chan_flags = sc->sc_txtap.wt_chan_flags =
	    htole16(c->ic_flags);
fail:
	return;
}

static int
zyd_set_beacon_interval(struct zyd_softc *sc, int bintval)
{
	int error;
	uint32_t val;

	zyd_read32_m(sc, ZYD_CR_ATIM_WND_PERIOD, &val);
	sc->sc_atim_wnd = val;
	zyd_read32_m(sc, ZYD_CR_PRE_TBTT, &val);
	sc->sc_pre_tbtt = val;
	sc->sc_bcn_int = bintval;

	if (sc->sc_bcn_int <= 5)
		sc->sc_bcn_int = 5;
	if (sc->sc_pre_tbtt < 4 || sc->sc_pre_tbtt >= sc->sc_bcn_int)
		sc->sc_pre_tbtt = sc->sc_bcn_int - 1;
	if (sc->sc_atim_wnd >= sc->sc_pre_tbtt)
		sc->sc_atim_wnd = sc->sc_pre_tbtt - 1;

	zyd_write32_m(sc, ZYD_CR_ATIM_WND_PERIOD, sc->sc_atim_wnd);
	zyd_write32_m(sc, ZYD_CR_PRE_TBTT, sc->sc_pre_tbtt);
	zyd_write32_m(sc, ZYD_CR_BCN_INTERVAL, sc->sc_bcn_int);
fail:
	return (error);
}

static void
zyd_intr(usbd_xfer_handle xfer, usbd_private_handle priv, usbd_status status)
{
	struct zyd_softc *sc = (struct zyd_softc *)priv;
	struct zyd_cmd *cmd;
	uint32_t datalen;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED)
			return;

		if (status == USBD_STALLED) {
			usbd_clear_endpoint_stall_async(
			    sc->sc_ep[ZYD_ENDPT_IIN]);
		}
		return;
	}

	cmd = (struct zyd_cmd *)sc->sc_ibuf;

	if (le16toh(cmd->code) == ZYD_NOTIF_RETRYSTATUS) {
		struct zyd_notif_retry *retry =
		    (struct zyd_notif_retry *)cmd->data;
		struct ifnet *ifp = sc->sc_ifp;
		struct ieee80211com *ic = ifp->if_l2com;
		struct ieee80211vap *vap = TAILQ_FIRST(&ic->ic_vaps);
		struct ieee80211_node *ni;

		DPRINTF(sc, ZYD_DEBUG_TX_PROC,
		    "retry intr: rate=0x%x addr=%s count=%d (0x%x)\n",
		    le16toh(retry->rate), ether_sprintf(retry->macaddr),
		    le16toh(retry->count) & 0xff, le16toh(retry->count));

		/*
		 * Find the node to which the packet was sent and update its
		 * retry statistics.  In BSS mode, this node is the AP we're
		 * associated to so no lookup is actually needed.
		 */
		ni = ieee80211_find_txnode(vap, retry->macaddr);
		if (ni != NULL) {
			ieee80211_amrr_tx_complete(&ZYD_NODE(ni)->amn,
			    IEEE80211_AMRR_FAILURE, 1);
			ieee80211_free_node(ni);
		}
		if (le16toh(retry->count) & 0x100)
			ifp->if_oerrors++;	/* too many retries */
	} else if (le16toh(cmd->code) == ZYD_NOTIF_IORD) {
		struct zyd_rq *rqp;

		if (le16toh(*(uint16_t *)cmd->data) == ZYD_CR_INTERRUPT)
			return;	/* HMAC interrupt */

		usbd_get_xfer_status(xfer, NULL, NULL, &datalen, NULL);
		datalen -= sizeof(cmd->code);
		datalen -= 2;	/* XXX: padding? */

		STAILQ_FOREACH(rqp, &sc->sc_rqh, rq) {
			int i;

			if (sizeof(struct zyd_pair) * rqp->len != datalen)
				continue;
			for (i = 0; i < rqp->len; i++) {
				if (*(((const uint16_t *)rqp->idata) + i) !=
				    (((struct zyd_pair *)cmd->data) + i)->reg)
					break;
			}
			if (i != rqp->len)
				continue;

			/* copy answer into caller-supplied buffer */
			bcopy(cmd->data, rqp->odata,
			    sizeof(struct zyd_pair) * rqp->len);
			wakeup(rqp->odata);	/* wakeup caller */

			return;
		}
		return;	/* unexpected IORD notification */
	} else {
		device_printf(sc->sc_dev, "unknown notification %x\n",
		    le16toh(cmd->code));
	}
}

static void
zyd_rx_data(struct zyd_softc *sc, const uint8_t *buf, uint16_t len)
{
	struct ifnet *ifp = sc->sc_ifp;
	struct ieee80211com *ic = ifp->if_l2com;
	struct ieee80211_node *ni;
	const struct zyd_plcphdr *plcp;
	const struct zyd_rx_stat *stat;
	struct mbuf *m;
	int rlen, rssi, nf;

	if (len < ZYD_MIN_FRAGSZ) {
		DPRINTF(sc, ZYD_DEBUG_RECV, "%s: frame too short (length=%d)\n",
		    device_get_nameunit(sc->sc_dev), len);
		ifp->if_ierrors++;
		return;
	}

	plcp = (const struct zyd_plcphdr *)buf;
	stat = (const struct zyd_rx_stat *)
	    (buf + len - sizeof(struct zyd_rx_stat));

	if (stat->flags & ZYD_RX_ERROR) {
		DPRINTF(sc, ZYD_DEBUG_RECV,
		    "%s: RX status indicated error (%x)\n",
		    device_get_nameunit(sc->sc_dev), stat->flags);
		ifp->if_ierrors++;
		return;
	}

	/* compute actual frame length */
	rlen = len - sizeof(struct zyd_plcphdr) -
	    sizeof(struct zyd_rx_stat) - IEEE80211_CRC_LEN;

	/* allocate a mbuf to store the frame */
	if (rlen > MHLEN)
		m = m_getcl(M_DONTWAIT, MT_DATA, M_PKTHDR);
	else
		m = m_gethdr(M_DONTWAIT, MT_DATA);
	if (m == NULL) {
		DPRINTF(sc, ZYD_DEBUG_RECV, "%s: could not allocate rx mbuf\n",
		    device_get_nameunit(sc->sc_dev));
		ifp->if_ierrors++;
		return;
	}
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = m->m_len = rlen;
	bcopy((const uint8_t *)(plcp + 1), mtod(m, uint8_t *), rlen);

	if (bpf_peers_present(ifp->if_bpf)) {
		struct zyd_rx_radiotap_header *tap = &sc->sc_rxtap;

		tap->wr_flags = 0;
		if (stat->flags & (ZYD_RX_BADCRC16 | ZYD_RX_BADCRC32))
			tap->wr_flags |= IEEE80211_RADIOTAP_F_BADFCS;
		/* XXX toss, no way to express errors */
		if (stat->flags & ZYD_RX_DECRYPTERR)
			tap->wr_flags |= IEEE80211_RADIOTAP_F_BADFCS;
		tap->wr_rate = ieee80211_plcp2rate(plcp->signal,
		    (stat->flags & ZYD_RX_OFDM) ?
			IEEE80211_T_OFDM : IEEE80211_T_CCK);
		tap->wr_antsignal = stat->rssi + -95;
		tap->wr_antnoise = -95;		/* XXX */
		
		bpf_mtap2(ifp->if_bpf, tap, sc->sc_rxtap_len, m);
	}

	rssi = stat->rssi > 63 ? 127 : 2 * stat->rssi;
	nf = -95;		/* XXX */

	ni = ieee80211_find_rxnode(ic, mtod(m, struct ieee80211_frame_min *));
	if (ni != NULL) {
		(void)ieee80211_input(ni, m, rssi, nf, 0);
		ieee80211_free_node(ni);
	} else
		(void)ieee80211_input_all(ic, m, rssi, nf, 0);
}

static void
zyd_rxeof(usbd_xfer_handle xfer, usbd_private_handle priv, usbd_status status)
{
	struct zyd_rx_data *data = priv;
	struct zyd_softc *sc = data->sc;
	struct ifnet *ifp = sc->sc_ifp;
	const struct zyd_rx_desc *desc;
	int len;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED)
			return;

		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall(sc->sc_ep[ZYD_ENDPT_BIN]);

		goto skip;
	}
	usbd_get_xfer_status(xfer, NULL, NULL, &len, NULL);

	if (len < ZYD_MIN_RXBUFSZ) {
		DPRINTF(sc, ZYD_DEBUG_RECV, "%s: xfer too short (length=%d)\n",
		    device_get_nameunit(sc->sc_dev), len);
		ifp->if_ierrors++;		/* XXX not really errors */
		goto skip;
	}

	desc = (const struct zyd_rx_desc *)
	    (data->buf + len - sizeof(struct zyd_rx_desc));

	if (UGETW(desc->tag) == ZYD_TAG_MULTIFRAME) {
		const uint8_t *p = data->buf, *end = p + len;
		int i;

		DPRINTF(sc, ZYD_DEBUG_RECV,
		    "%s: received multi-frame transfer\n", __func__);

		for (i = 0; i < ZYD_MAX_RXFRAMECNT; i++) {
			const uint16_t len16 = UGETW(desc->len[i]);

			if (len16 == 0 || p + len16 > end)
				break;

			zyd_rx_data(sc, p, len16);
			/* next frame is aligned on a 32-bit boundary */
			p += (len16 + 3) & ~3;
		}
	} else {
		DPRINTF(sc, ZYD_DEBUG_RECV,
		    "%s: received single-frame transfer\n", __func__);

		zyd_rx_data(sc, data->buf, len);
	}

skip:	/* setup a new transfer */
	usbd_setup_xfer(xfer, sc->sc_ep[ZYD_ENDPT_BIN], data, NULL,
	    ZYX_MAX_RXBUFSZ, USBD_NO_COPY | USBD_SHORT_XFER_OK,
	    USBD_NO_TIMEOUT, zyd_rxeof);
	(void)usbd_transfer(xfer);
}

static uint8_t
zyd_plcp_signal(int rate)
{
	switch (rate) {
	/* OFDM rates (cf IEEE Std 802.11a-1999, pp. 14 Table 80) */
	case 12:
		return (0xb);
	case 18:
		return (0xf);
	case 24:
		return (0xa);
	case 36:
		return (0xe);
	case 48:
		return (0x9);
	case 72:
		return (0xd);
	case 96:
		return (0x8);
	case 108:
		return (0xc);
	/* CCK rates (NB: not IEEE std, device-specific) */
	case 2:
		return (0x0);
	case 4:
		return (0x1);
	case 11:
		return (0x2);
	case 22:
		return (0x3);
	}
	return (0xff);		/* XXX unsupported/unknown rate */
}

static int
zyd_tx_mgt(struct zyd_softc *sc, struct mbuf *m0, struct ieee80211_node *ni)
{
	struct ieee80211vap *vap = ni->ni_vap;
	struct ieee80211com *ic = ni->ni_ic;
	struct ifnet *ifp = sc->sc_ifp;
	struct zyd_tx_desc *desc;
	struct zyd_tx_data *data;
	struct ieee80211_frame *wh;
	struct ieee80211_key *k;
	int data_idx, rate, totlen, xferlen;
	uint16_t pktlen;
	usbd_status error;

	data_idx = sc->sc_txidx;
	sc->sc_txidx = (sc->sc_txidx + 1) % ZYD_TX_LIST_CNT;

	data = &sc->sc_txdata[data_idx];
	desc = (struct zyd_tx_desc *)data->buf;

	rate = IEEE80211_IS_CHAN_5GHZ(ic->ic_curchan) ? 12 : 2;

	wh = mtod(m0, struct ieee80211_frame *);

	if (wh->i_fc[1] & IEEE80211_FC1_WEP) {
		k = ieee80211_crypto_encap(ni, m0);
		if (k == NULL) {
			m_freem(m0);
			return (ENOBUFS);
		}
	}

	data->ni = ni;
	data->m = m0;

	wh = mtod(m0, struct ieee80211_frame *);

	xferlen = sizeof(struct zyd_tx_desc) + m0->m_pkthdr.len;
	totlen = m0->m_pkthdr.len + IEEE80211_CRC_LEN;

	/* fill Tx descriptor */
	desc->len = htole16(totlen);

	desc->flags = ZYD_TX_FLAG_BACKOFF;
	if (!IEEE80211_IS_MULTICAST(wh->i_addr1)) {
		/* multicast frames are not sent at OFDM rates in 802.11b/g */
		if (totlen > vap->iv_rtsthreshold) {
			desc->flags |= ZYD_TX_FLAG_RTS;
		} else if (ZYD_RATE_IS_OFDM(rate) &&
		    (ic->ic_flags & IEEE80211_F_USEPROT)) {
			if (ic->ic_protmode == IEEE80211_PROT_CTSONLY)
				desc->flags |= ZYD_TX_FLAG_CTS_TO_SELF;
			else if (ic->ic_protmode == IEEE80211_PROT_RTSCTS)
				desc->flags |= ZYD_TX_FLAG_RTS;
		}
	} else
		desc->flags |= ZYD_TX_FLAG_MULTICAST;

	if ((wh->i_fc[0] &
	    (IEEE80211_FC0_TYPE_MASK | IEEE80211_FC0_SUBTYPE_MASK)) ==
	    (IEEE80211_FC0_TYPE_CTL | IEEE80211_FC0_SUBTYPE_PS_POLL))
		desc->flags |= ZYD_TX_FLAG_TYPE(ZYD_TX_TYPE_PS_POLL);

	desc->phy = zyd_plcp_signal(rate);
	if (ZYD_RATE_IS_OFDM(rate)) {
		desc->phy |= ZYD_TX_PHY_OFDM;
		if (IEEE80211_IS_CHAN_5GHZ(ic->ic_curchan))
			desc->phy |= ZYD_TX_PHY_5GHZ;
	} else if (rate != 2 && (ic->ic_flags & IEEE80211_F_SHPREAMBLE))
		desc->phy |= ZYD_TX_PHY_SHPREAMBLE;

	/* actual transmit length (XXX why +10?) */
	pktlen = sizeof(struct zyd_tx_desc) + 10;
	if (sc->sc_macrev == ZYD_ZD1211)
		pktlen += totlen;
	desc->pktlen = htole16(pktlen);

	desc->plcp_length = (16 * totlen + rate - 1) / rate;
	desc->plcp_service = 0;
	if (rate == 22) {
		const int remainder = (16 * totlen) % 22;
		if (remainder != 0 && remainder < 7)
			desc->plcp_service |= ZYD_PLCP_LENGEXT;
	}

	if (bpf_peers_present(ifp->if_bpf)) {
		struct zyd_tx_radiotap_header *tap = &sc->sc_txtap;

		tap->wt_flags = 0;
		tap->wt_rate = rate;

		bpf_mtap2(ifp->if_bpf, tap, sc->sc_txtap_len, m0);
	}

	m_copydata(m0, 0, m0->m_pkthdr.len,
	    data->buf + sizeof(struct zyd_tx_desc));

	DPRINTF(sc, ZYD_DEBUG_XMIT,
	    "%s: sending mgt frame len=%zu rate=%u xferlen=%u\n",
	    device_get_nameunit(sc->sc_dev), (size_t)m0->m_pkthdr.len,
		rate, xferlen);

	usbd_setup_xfer(data->xfer, sc->sc_ep[ZYD_ENDPT_BOUT], data,
	    data->buf, xferlen, USBD_FORCE_SHORT_XFER | USBD_NO_COPY,
	    ZYD_TX_TIMEOUT, zyd_txeof);
	error = usbd_transfer(data->xfer);
	if (error != USBD_IN_PROGRESS && error != 0) {
		ifp->if_oerrors++;
		return (EIO);
	}
	sc->sc_txqueued++;

	return (0);
}

static void
zyd_txeof(usbd_xfer_handle xfer, usbd_private_handle priv, usbd_status status)
{
	struct zyd_tx_data *data = priv;
	struct zyd_softc *sc = data->sc;
	struct ifnet *ifp = sc->sc_ifp;
	struct ieee80211_node *ni;
	struct mbuf *m;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED)
			return;

		device_printf(sc->sc_dev, "could not transmit buffer: %s\n",
		    usbd_errstr(status));

		if (status == USBD_STALLED) {
			usbd_clear_endpoint_stall_async(
			    sc->sc_ep[ZYD_ENDPT_BOUT]);
		}
		ifp->if_oerrors++;
		return;
	}

	ni = data->ni;
	/* update rate control statistics */
	ieee80211_amrr_tx_complete(&ZYD_NODE(ni)->amn,
	    IEEE80211_AMRR_SUCCESS, 0);

	/*
	 * Do any tx complete callback.  Note this must
	 * be done before releasing the node reference.
	 */
	m = data->m;
	if (m != NULL && m->m_flags & M_TXCB) {
		ieee80211_process_callback(ni, m, 0);	/* XXX status? */
		m_freem(m);
		data->m = NULL;
	}

	ieee80211_free_node(ni);
	data->ni = NULL;

	ZYD_TX_LOCK(sc);
	sc->sc_txqueued--;
	ifp->if_drv_flags &= ~IFF_DRV_OACTIVE;
	ZYD_TX_UNLOCK(sc);

	ifp->if_opackets++;
	sc->sc_txtimer = 0;
	zyd_start(ifp);
}

static int
zyd_tx_data(struct zyd_softc *sc, struct mbuf *m0, struct ieee80211_node *ni)
{
	struct ieee80211vap *vap = ni->ni_vap;
	struct ieee80211com *ic = ni->ni_ic;
	struct ifnet *ifp = sc->sc_ifp;
	struct zyd_tx_desc *desc;
	struct zyd_tx_data *data;
	struct ieee80211_frame *wh;
	const struct ieee80211_txparam *tp;
	struct ieee80211_key *k;
	int data_idx, rate, totlen, xferlen;
	uint16_t pktlen;
	usbd_status error;

	data_idx = sc->sc_txidx;
	sc->sc_txidx = (sc->sc_txidx + 1) % ZYD_TX_LIST_CNT;

	wh = mtod(m0, struct ieee80211_frame *);
	data = &sc->sc_txdata[data_idx];
	desc = (struct zyd_tx_desc *)data->buf;

	desc->flags = ZYD_TX_FLAG_BACKOFF;
	tp = &vap->iv_txparms[ieee80211_chan2mode(ni->ni_chan)];
	if (IEEE80211_IS_MULTICAST(wh->i_addr1)) {
		rate = tp->mcastrate;
		desc->flags |= ZYD_TX_FLAG_MULTICAST;
	} else if (tp->ucastrate != IEEE80211_FIXED_RATE_NONE) {
		rate = tp->ucastrate;
	} else {
		(void) ieee80211_amrr_choose(ni, &ZYD_NODE(ni)->amn);
		rate = ni->ni_txrate;
	}

	if (wh->i_fc[1] & IEEE80211_FC1_WEP) {
		k = ieee80211_crypto_encap(ni, m0);
		if (k == NULL) {
			m_freem(m0);
			return (ENOBUFS);
		}
		/* packet header may have moved, reset our local pointer */
		wh = mtod(m0, struct ieee80211_frame *);
	}

	data->ni = ni;
	data->m = NULL;

	xferlen = sizeof(struct zyd_tx_desc) + m0->m_pkthdr.len;
	totlen = m0->m_pkthdr.len + IEEE80211_CRC_LEN;

	/* fill Tx descriptor */
	desc->len = htole16(totlen);

	if (!IEEE80211_IS_MULTICAST(wh->i_addr1)) {
		/* multicast frames are not sent at OFDM rates in 802.11b/g */
		if (totlen > vap->iv_rtsthreshold) {
			desc->flags |= ZYD_TX_FLAG_RTS;
		} else if (ZYD_RATE_IS_OFDM(rate) &&
		    (ic->ic_flags & IEEE80211_F_USEPROT)) {
			if (ic->ic_protmode == IEEE80211_PROT_CTSONLY)
				desc->flags |= ZYD_TX_FLAG_CTS_TO_SELF;
			else if (ic->ic_protmode == IEEE80211_PROT_RTSCTS)
				desc->flags |= ZYD_TX_FLAG_RTS;
		}
	}

	if ((wh->i_fc[0] &
	    (IEEE80211_FC0_TYPE_MASK | IEEE80211_FC0_SUBTYPE_MASK)) ==
	    (IEEE80211_FC0_TYPE_CTL | IEEE80211_FC0_SUBTYPE_PS_POLL))
		desc->flags |= ZYD_TX_FLAG_TYPE(ZYD_TX_TYPE_PS_POLL);

	desc->phy = zyd_plcp_signal(rate);
	if (ZYD_RATE_IS_OFDM(rate)) {
		desc->phy |= ZYD_TX_PHY_OFDM;
		if (IEEE80211_IS_CHAN_5GHZ(ic->ic_curchan))
			desc->phy |= ZYD_TX_PHY_5GHZ;
	} else if (rate != 2 && (ic->ic_flags & IEEE80211_F_SHPREAMBLE))
		desc->phy |= ZYD_TX_PHY_SHPREAMBLE;

	/* actual transmit length (XXX why +10?) */
	pktlen = sizeof(struct zyd_tx_desc) + 10;
	if (sc->sc_macrev == ZYD_ZD1211)
		pktlen += totlen;
	desc->pktlen = htole16(pktlen);

	desc->plcp_length = (16 * totlen + rate - 1) / rate;
	desc->plcp_service = 0;
	if (rate == 22) {
		const int remainder = (16 * totlen) % 22;
		if (remainder != 0 && remainder < 7)
			desc->plcp_service |= ZYD_PLCP_LENGEXT;
	}

	if (bpf_peers_present(ifp->if_bpf)) {
		struct zyd_tx_radiotap_header *tap = &sc->sc_txtap;

		tap->wt_flags = 0;
		tap->wt_rate = rate;
		tap->wt_chan_freq = htole16(ic->ic_curchan->ic_freq);
		tap->wt_chan_flags = htole16(ic->ic_curchan->ic_flags);

		bpf_mtap2(ifp->if_bpf, tap, sc->sc_txtap_len, m0);
	}

	m_copydata(m0, 0, m0->m_pkthdr.len,
	    data->buf + sizeof(struct zyd_tx_desc));

	DPRINTF(sc, ZYD_DEBUG_XMIT,
	    "%s: sending data frame len=%zu rate=%u xferlen=%u\n",
	    device_get_nameunit(sc->sc_dev), (size_t)m0->m_pkthdr.len,
		rate, xferlen);

	m_freem(m0);	/* mbuf no longer needed */

	usbd_setup_xfer(data->xfer, sc->sc_ep[ZYD_ENDPT_BOUT], data,
	    data->buf, xferlen, USBD_FORCE_SHORT_XFER | USBD_NO_COPY,
	    ZYD_TX_TIMEOUT, zyd_txeof);
	error = usbd_transfer(data->xfer);
	if (error != USBD_IN_PROGRESS && error != 0) {
		ifp->if_oerrors++;
		return (EIO);
	}
	sc->sc_txqueued++;

	return (0);
}

static void
zyd_start(struct ifnet *ifp)
{
	struct zyd_softc *sc = ifp->if_softc;
	struct ieee80211_node *ni;
	struct mbuf *m;

	ZYD_TX_LOCK(sc);
	for (;;) {
		IFQ_DRV_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;
		if (sc->sc_txqueued >= ZYD_TX_LIST_CNT) {
			IFQ_DRV_PREPEND(&ifp->if_snd, m);
			ifp->if_drv_flags |= IFF_DRV_OACTIVE;
			break;
		}
		ni = (struct ieee80211_node *)m->m_pkthdr.rcvif;
		m = ieee80211_encap(ni, m);
		if (m == NULL) {
			ieee80211_free_node(ni);
			ifp->if_oerrors++;
			continue;
		}
		if (zyd_tx_data(sc, m, ni) != 0) {
			ieee80211_free_node(ni);
			ifp->if_oerrors++;
			break;
		}

		sc->sc_txtimer = 5;
	}
	ZYD_TX_UNLOCK(sc);
}

static int
zyd_raw_xmit(struct ieee80211_node *ni, struct mbuf *m,
	const struct ieee80211_bpf_params *params)
{
	struct ieee80211com *ic = ni->ni_ic;
	struct ifnet *ifp = ic->ic_ifp;
	struct zyd_softc *sc = ifp->if_softc;

	/* prevent management frames from being sent if we're not ready */
	if (!(ifp->if_drv_flags & IFF_DRV_RUNNING)) {
		m_freem(m);
		ieee80211_free_node(ni);
		return (ENETDOWN);
	}
	ZYD_TX_LOCK(sc);
	if (sc->sc_txqueued >= ZYD_TX_LIST_CNT) {
		ifp->if_drv_flags |= IFF_DRV_OACTIVE;
		m_freem(m);
		ieee80211_free_node(ni);
		return (ENOBUFS);		/* XXX */
	}

	/*
	 * Legacy path; interpret frame contents to decide
	 * precisely how to send the frame.
	 * XXX raw path
	 */
	if (zyd_tx_mgt(sc, m, ni) != 0) {
		ZYD_TX_UNLOCK(sc);
		ifp->if_oerrors++;
		ieee80211_free_node(ni);
		return (EIO);
	}

	ZYD_TX_UNLOCK(sc);
	ifp->if_opackets++;
	sc->sc_txtimer = 5;
	return (0);
}

static void
zyd_watchdog(void *arg)
{
	struct zyd_softc *sc = arg;
	struct ifnet *ifp = sc->sc_ifp;

	if (sc->sc_txtimer > 0) {
		if (--sc->sc_txtimer == 0) {
			device_printf(sc->sc_dev, "device timeout\n");
			/* zyd_init(ifp); XXX needs a process context ? */
			ifp->if_oerrors++;
			return;
		}
		callout_reset(&sc->sc_watchdog_ch, hz, zyd_watchdog, sc);
	}
}

static int
zyd_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct zyd_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = ifp->if_l2com;
	struct ifreq *ifr = (struct ifreq *) data;
	int error = 0, startall = 0;

	switch (cmd) {
	case SIOCSIFFLAGS:
		ZYD_LOCK(sc);
		if (ifp->if_flags & IFF_UP) {
			if (ifp->if_drv_flags & IFF_DRV_RUNNING) {
				if ((ifp->if_flags ^ sc->sc_if_flags) &
				    (IFF_ALLMULTI | IFF_PROMISC))
					zyd_set_multi(sc);
			} else {
				zyd_init_locked(sc);
				startall = 1;
			}
		} else {
			if (ifp->if_drv_flags & IFF_DRV_RUNNING)
				zyd_stop(sc, 1);
		}
		sc->sc_if_flags = ifp->if_flags;
		ZYD_UNLOCK(sc);
		if (startall)
			ieee80211_start_all(ic);
		break;
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &ic->ic_media, cmd);
		break;
	case SIOCGIFADDR:
		error = ether_ioctl(ifp, cmd, data);
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

static void
zyd_init_locked(struct zyd_softc *sc)
{
	int error, i;
	struct ifnet *ifp = sc->sc_ifp;
	struct ieee80211com *ic = ifp->if_l2com;
	uint32_t val;

	if (!(sc->sc_flags & ZYD_FLAG_INITONCE)) {
		error = zyd_loadfirmware(sc);
		if (error != 0) {
			device_printf(sc->sc_dev,
			    "could not load firmware (error=%d)\n", error);
			goto fail;
		}

		error = usbd_set_config_no(sc->sc_udev, ZYD_CONFIG_NO, 1);
		if (error != 0) {
			device_printf(sc->sc_dev, "setting config no failed\n");
			goto fail;
		}
		error = usbd_device2interface_handle(sc->sc_udev,
		    ZYD_IFACE_INDEX, &sc->sc_iface);
		if (error != 0) {
			device_printf(sc->sc_dev,
			    "getting interface handle failed\n");
			goto fail;
		}

		if ((error = zyd_open_pipes(sc)) != 0) {
			device_printf(sc->sc_dev, "could not open pipes\n");
			goto fail;
		}
		if ((error = zyd_hw_init(sc)) != 0) {
			device_printf(sc->sc_dev,
			    "hardware initialization failed\n");
			goto fail;
		}

		device_printf(sc->sc_dev,
		    "HMAC ZD1211%s, FW %02x.%02x, RF %s S%x, PA%x LED %x "
		    "BE%x NP%x Gain%x F%x\n",
		    (sc->sc_macrev == ZYD_ZD1211) ? "": "B",
		    sc->sc_fwrev >> 8, sc->sc_fwrev & 0xff,
		    zyd_rf_name(sc->sc_rfrev), sc->sc_al2230s, sc->sc_parev,
		    sc->sc_ledtype, sc->sc_bandedge6, sc->sc_newphy,
		    sc->sc_cckgain, sc->sc_fix_cr157);

		/* read regulatory domain (currently unused) */
		zyd_read32_m(sc, ZYD_EEPROM_SUBID, &val);
		sc->sc_regdomain = val >> 16;
		DPRINTF(sc, ZYD_DEBUG_INIT, "regulatory domain %x\n",
		    sc->sc_regdomain);

		/* we'll do software WEP decryption for now */
		DPRINTF(sc, ZYD_DEBUG_INIT, "%s: setting encryption type\n",
		    __func__);
		zyd_write32_m(sc, ZYD_MAC_ENCRYPTION_TYPE, ZYD_ENC_SNIFFER);

		sc->sc_flags |= ZYD_FLAG_INITONCE;
	}

	if (ifp->if_drv_flags & IFF_DRV_RUNNING)
		zyd_stop(sc, 0);

	/* reset softc variables.  */
	sc->sc_txidx = 0;

	IEEE80211_ADDR_COPY(ic->ic_myaddr, IF_LLADDR(ifp));
	DPRINTF(sc, ZYD_DEBUG_INIT, "setting MAC address to %s\n",
	    ether_sprintf(ic->ic_myaddr));
	error = zyd_set_macaddr(sc, ic->ic_myaddr);
	if (error != 0)
		return;

	/* set basic rates */
	if (ic->ic_curmode == IEEE80211_MODE_11B)
		zyd_write32_m(sc, ZYD_MAC_BAS_RATE, 0x0003);
	else if (ic->ic_curmode == IEEE80211_MODE_11A)
		zyd_write32_m(sc, ZYD_MAC_BAS_RATE, 0x1500);
	else	/* assumes 802.11b/g */
		zyd_write32_m(sc, ZYD_MAC_BAS_RATE, 0xff0f);

	/* promiscuous mode */
	zyd_write32_m(sc, ZYD_MAC_SNIFFER, 0);
	/* multicast setup */
	zyd_set_multi(sc);
	/* set RX filter  */
	error = zyd_set_rxfilter(sc);
	if (error != 0)
		goto fail;

	/* switch radio transmitter ON */
	error = zyd_switch_radio(sc, 1);
	if (error != 0)
		goto fail;
	/* set default BSS channel */
	zyd_set_chan(sc, ic->ic_curchan);

	/*
	 * Allocate Tx and Rx xfer queues.
	 */
	if ((error = zyd_alloc_tx_list(sc)) != 0) {
		device_printf(sc->sc_dev, "could not allocate Tx list\n");
		goto fail;
	}
	if ((error = zyd_alloc_rx_list(sc)) != 0) {
		device_printf(sc->sc_dev, "could not allocate Rx list\n");
		goto fail;
	}

	/*
	 * Start up the receive pipe.
	 */
	for (i = 0; i < ZYD_RX_LIST_CNT; i++) {
		struct zyd_rx_data *data = &sc->sc_rxdata[i];

		usbd_setup_xfer(data->xfer, sc->sc_ep[ZYD_ENDPT_BIN], data,
		    NULL, ZYX_MAX_RXBUFSZ, USBD_NO_COPY | USBD_SHORT_XFER_OK,
		    USBD_NO_TIMEOUT, zyd_rxeof);
		error = usbd_transfer(data->xfer);
		if (error != USBD_IN_PROGRESS && error != 0) {
			device_printf(sc->sc_dev,
			    "could not queue Rx transfer\n");
			goto fail;
		}
	}

	/* enable interrupts */
	zyd_write32_m(sc, ZYD_CR_INTERRUPT, ZYD_HWINT_MASK);

	ifp->if_drv_flags &= ~IFF_DRV_OACTIVE;
	ifp->if_drv_flags |= IFF_DRV_RUNNING;
	sc->sc_flags |= ZYD_FLAG_INITDONE;

	callout_reset(&sc->sc_watchdog_ch, hz, zyd_watchdog, sc);
	return;

fail:	zyd_stop(sc, 1);
	return;
}

static void
zyd_init(void *priv)
{
	struct zyd_softc *sc = priv;
	struct ifnet *ifp = sc->sc_ifp;
	struct ieee80211com *ic = ifp->if_l2com;

	ZYD_LOCK(sc);
	zyd_init_locked(sc);
	ZYD_UNLOCK(sc);

	if (ifp->if_drv_flags & IFF_DRV_RUNNING)
		ieee80211_start_all(ic);		/* start all vap's */
}

static void
zyd_stop(struct zyd_softc *sc, int disable)
{
	int error;
	struct ifnet *ifp = sc->sc_ifp;

	sc->sc_txtimer = 0;
	ifp->if_drv_flags &= ~(IFF_DRV_RUNNING | IFF_DRV_OACTIVE);

	/* switch radio transmitter OFF */
	error = zyd_switch_radio(sc, 0);
	if (error != 0)
		goto fail;
	/* disable Rx */
	zyd_write32_m(sc, ZYD_MAC_RXFILTER, 0);
	/* disable interrupts */
	zyd_write32_m(sc, ZYD_CR_INTERRUPT, 0);

	usb_rem_task(sc->sc_udev, &sc->sc_scantask);
	usb_rem_task(sc->sc_udev, &sc->sc_task);
	callout_stop(&sc->sc_watchdog_ch);

	usbd_abort_pipe(sc->sc_ep[ZYD_ENDPT_BIN]);
	usbd_abort_pipe(sc->sc_ep[ZYD_ENDPT_BOUT]);

	zyd_free_rx_list(sc);
	zyd_free_tx_list(sc);
fail:
	return;
}

static int
zyd_loadfirmware(struct zyd_softc *sc)
{
	usb_device_request_t req;
	size_t size;
	u_char *fw;
	uint8_t stat;
	uint16_t addr;

	if (sc->sc_flags & ZYD_FLAG_FWLOADED)
		return (0);

	if (sc->sc_macrev == ZYD_ZD1211) {
		fw = (u_char *)zd1211_firmware;
		size = sizeof(zd1211_firmware);
	} else {
		fw = (u_char *)zd1211b_firmware;
		size = sizeof(zd1211b_firmware);
	}

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = ZYD_DOWNLOADREQ;
	USETW(req.wIndex, 0);

	addr = ZYD_FIRMWARE_START_ADDR;
	while (size > 0) {
		/*
		 * When the transfer size is 4096 bytes, it is not
		 * likely to be able to transfer it.
		 * The cause is port or machine or chip?
		 */
		const int mlen = min(size, 64);

		DPRINTF(sc, ZYD_DEBUG_FW,
		    "loading firmware block: len=%d, addr=0x%x\n", mlen, addr);

		USETW(req.wValue, addr);
		USETW(req.wLength, mlen);
		if (usbd_do_request(sc->sc_udev, &req, fw) != 0)
			return (EIO);

		addr += mlen / 2;
		fw   += mlen;
		size -= mlen;
	}

	/* check whether the upload succeeded */
	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = ZYD_DOWNLOADSTS;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, sizeof(stat));
	if (usbd_do_request(sc->sc_udev, &req, &stat) != 0)
		return (EIO);

	sc->sc_flags |= ZYD_FLAG_FWLOADED;

	return (stat & 0x80) ? (EIO) : (0);
}

static void
zyd_newassoc(struct ieee80211_node *ni, int isnew)
{
	struct ieee80211vap *vap = ni->ni_vap;

	ieee80211_amrr_node_init(&ZYD_VAP(vap)->amrr, &ZYD_NODE(ni)->amn, ni);
}

static void
zyd_scan_start(struct ieee80211com *ic)
{
	struct zyd_softc *sc = ic->ic_ifp->if_softc;

	usb_rem_task(sc->sc_udev, &sc->sc_scantask);

	/* do it in a process context */
	sc->sc_scan_action = ZYD_SCAN_START;
	usb_add_task(sc->sc_udev, &sc->sc_scantask, USB_TASKQ_DRIVER);
}

static void
zyd_scan_end(struct ieee80211com *ic)
{
	struct zyd_softc *sc = ic->ic_ifp->if_softc;

	usb_rem_task(sc->sc_udev, &sc->sc_scantask);

	/* do it in a process context */
	sc->sc_scan_action = ZYD_SCAN_END;
	usb_add_task(sc->sc_udev, &sc->sc_scantask, USB_TASKQ_DRIVER);
}

static void
zyd_set_channel(struct ieee80211com *ic)
{
	struct zyd_softc *sc = ic->ic_ifp->if_softc;

	usb_rem_task(sc->sc_udev, &sc->sc_scantask);
	/* do it in a process context */
	sc->sc_scan_action = ZYD_SET_CHANNEL;
	usb_add_task(sc->sc_udev, &sc->sc_scantask, USB_TASKQ_DRIVER);
}

static void
zyd_scantask(void *arg)
{
	struct zyd_softc *sc = arg;
	struct ifnet *ifp = sc->sc_ifp;
	struct ieee80211com *ic = ifp->if_l2com;

	ZYD_LOCK(sc);

	switch (sc->sc_scan_action) {
	case ZYD_SCAN_START:
		/* want broadcast address while scanning */
                zyd_set_bssid(sc, ifp->if_broadcastaddr);
                break;
        case ZYD_SCAN_END:
		/* restore previous bssid */
                zyd_set_bssid(sc, sc->sc_bssid);
                break;
        case ZYD_SET_CHANNEL:
                zyd_set_chan(sc, ic->ic_curchan);
                break;
        default:
                device_printf(sc->sc_dev, "unknown scan action %d\n",
		    sc->sc_scan_action);
                break;
        }

        ZYD_UNLOCK(sc);
}

static void
zyd_wakeup(struct zyd_softc *sc)
{
	struct zyd_rq *rqp;

	STAILQ_FOREACH(rqp, &sc->sc_rqh, rq)
		wakeup(rqp->odata);		/* wakeup sleeping caller */
}

static device_method_t zyd_methods[] = {
        /* Device interface */
        DEVMETHOD(device_probe, zyd_match),
        DEVMETHOD(device_attach, zyd_attach),
        DEVMETHOD(device_detach, zyd_detach),
	
	{ 0, 0 }
};

static driver_t zyd_driver = {
        "zyd",
        zyd_methods,
        sizeof(struct zyd_softc)
};

static devclass_t zyd_devclass;

DRIVER_MODULE(zyd, uhub, zyd_driver, zyd_devclass, usbd_driver_load, 0);
MODULE_DEPEND(zyd, wlan, 1, 1, 1);
MODULE_DEPEND(zyd, wlan_amrr, 1, 1, 1);
MODULE_DEPEND(zyd, usb, 1, 1, 1);
