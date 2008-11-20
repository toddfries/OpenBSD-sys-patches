/*	$FreeBSD: src/sys/dev/usb/if_uralvar.h,v 1.10 2008/04/20 20:35:38 sam Exp $	*/

/*-
 * Copyright (c) 2005
 *	Damien Bergamini <damien.bergamini@free.fr>
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

#define RAL_RX_LIST_COUNT	1
#define RAL_TX_LIST_COUNT	8

#define URAL_SCAN_START         1
#define URAL_SCAN_END           2
#define URAL_SET_CHANNEL        3


struct ural_rx_radiotap_header {
	struct ieee80211_radiotap_header wr_ihdr;
	uint8_t		wr_flags;
	uint8_t		wr_rate;
	uint16_t	wr_chan_freq;
	uint16_t	wr_chan_flags;
	uint8_t		wr_antenna;
	uint8_t		wr_antsignal;
};

#define RAL_RX_RADIOTAP_PRESENT						\
	((1 << IEEE80211_RADIOTAP_FLAGS) |				\
	 (1 << IEEE80211_RADIOTAP_RATE) |				\
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |				\
	 (1 << IEEE80211_RADIOTAP_ANTENNA) |				\
	 (1 << IEEE80211_RADIOTAP_DB_ANTSIGNAL))

struct ural_tx_radiotap_header {
	struct ieee80211_radiotap_header wt_ihdr;
	uint8_t		wt_flags;
	uint8_t		wt_rate;
	uint16_t	wt_chan_freq;
	uint16_t	wt_chan_flags;
	uint8_t		wt_antenna;
};

#define RAL_TX_RADIOTAP_PRESENT						\
	((1 << IEEE80211_RADIOTAP_FLAGS) |				\
	 (1 << IEEE80211_RADIOTAP_RATE) |				\
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |				\
	 (1 << IEEE80211_RADIOTAP_ANTENNA))

struct ural_softc;

struct ural_tx_data {
	struct ural_softc	*sc;
	usbd_xfer_handle	xfer;
	uint8_t			*buf;
	struct mbuf		*m;
	struct ieee80211_node	*ni;
};

struct ural_rx_data {
	struct ural_softc	*sc;
	usbd_xfer_handle	xfer;
	uint8_t			*buf;
	struct mbuf		*m;
};

struct ural_node {
	struct ieee80211_node		ni;
	struct ieee80211_amrr_node	amn;
};
#define	URAL_NODE(ni)	((struct ural_node *)(ni))

struct ural_vap {
	struct ieee80211vap		vap;
	struct ieee80211_beacon_offsets	bo;
	struct ieee80211_amrr		amrr;
	struct callout			amrr_ch;

	int				(*newstate)(struct ieee80211vap *,
					    enum ieee80211_state, int);
};
#define	URAL_VAP(vap)	((struct ural_vap *)(vap))

struct ural_softc {
	struct ifnet			*sc_ifp;
	device_t			sc_dev;
	usbd_device_handle		sc_udev;
	usbd_interface_handle		sc_iface;

	const struct ieee80211_rate_table *sc_rates;

	int				sc_rx_no;
	int				sc_tx_no;

	uint32_t			asic_rev;
	uint8_t				rf_rev;

	usbd_xfer_handle		amrr_xfer;

	usbd_pipe_handle		sc_rx_pipeh;
	usbd_pipe_handle		sc_tx_pipeh;

	enum ieee80211_state		sc_state;
	int				sc_arg;
	int                             sc_scan_action; /* should be an enum */
	struct usb_task			sc_task;
	struct usb_task			sc_scantask;

	struct ural_rx_data		rx_data[RAL_RX_LIST_COUNT];
	struct ural_tx_data		tx_data[RAL_TX_LIST_COUNT];
	int				tx_queued;
	int				tx_cur;

	struct mtx			sc_mtx;

	struct callout			watchdog_ch;
	int				sc_tx_timer;

	uint16_t			sta[11];
	uint32_t			rf_regs[4];
	uint8_t				txpow[14];

	struct {
		uint8_t			val;
		uint8_t			reg;
	} __packed			bbp_prom[16];

	int				led_mode;
	int				hw_radio;
	int				rx_ant;
	int				tx_ant;
	int				nb_ant;

	struct ural_rx_radiotap_header	sc_rxtap;
	int				sc_rxtap_len;

	struct ural_tx_radiotap_header	sc_txtap;
	int				sc_txtap_len;
};

#if 0
#define RAL_LOCK(sc)	mtx_lock(&(sc)->sc_mtx)
#define RAL_UNLOCK(sc)	mtx_unlock(&(sc)->sc_mtx)
#else
#define RAL_LOCK(sc)	do { ((sc) = (sc)); mtx_lock(&Giant); } while (0)
#define RAL_UNLOCK(sc)	mtx_unlock(&Giant)
#endif
