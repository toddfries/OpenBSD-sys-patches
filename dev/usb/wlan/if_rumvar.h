/*	$FreeBSD: src/sys/dev/usb/wlan/if_rumvar.h,v 1.3 2009/02/27 21:14:29 thompsa Exp $	*/

/*-
 * Copyright (c) 2005, 2006 Damien Bergamini <damien.bergamini@free.fr>
 * Copyright (c) 2006 Niall O'Higgins <niallo@openbsd.org>
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

#define RUM_TX_LIST_COUNT	8
#define RUM_TX_MINFREE		2

struct rum_rx_radiotap_header {
	struct ieee80211_radiotap_header wr_ihdr;
	uint8_t		wr_flags;
	uint8_t		wr_rate;
	uint16_t	wr_chan_freq;
	uint16_t	wr_chan_flags;
	uint8_t		wr_antenna;
	uint8_t		wr_antsignal;
};

#define RT2573_RX_RADIOTAP_PRESENT					\
	((1 << IEEE80211_RADIOTAP_FLAGS) |				\
	 (1 << IEEE80211_RADIOTAP_RATE) |				\
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |				\
	 (1 << IEEE80211_RADIOTAP_ANTENNA) |				\
	 (1 << IEEE80211_RADIOTAP_DB_ANTSIGNAL))

struct rum_tx_radiotap_header {
	struct ieee80211_radiotap_header wt_ihdr;
	uint8_t		wt_flags;
	uint8_t		wt_rate;
	uint16_t	wt_chan_freq;
	uint16_t	wt_chan_flags;
	uint8_t		wt_antenna;
};

#define RT2573_TX_RADIOTAP_PRESENT					\
	((1 << IEEE80211_RADIOTAP_FLAGS) |				\
	 (1 << IEEE80211_RADIOTAP_RATE) |				\
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |				\
	 (1 << IEEE80211_RADIOTAP_ANTENNA))

struct rum_softc;

struct rum_task {
	struct usb2_proc_msg	hdr;
	usb2_proc_callback_t	*func;
	struct rum_softc	*sc;
};

struct rum_tx_data {
	STAILQ_ENTRY(rum_tx_data)	next;
	struct rum_softc		*sc;
	struct rum_tx_desc		desc;
	struct mbuf			*m;
	struct ieee80211_node		*ni;
	int				rate;
};
typedef STAILQ_HEAD(, rum_tx_data) rum_txdhead;

struct rum_node {
	struct ieee80211_node	ni;
	struct ieee80211_amrr_node amn;
};
#define	RUM_NODE(ni)	((struct rum_node *)(ni))

struct rum_vap {
	struct ieee80211vap		vap;
	struct rum_softc		*sc;
	struct ieee80211_beacon_offsets	bo;
	struct ieee80211_amrr		amrr;
	struct usb2_callout		amrr_ch;
	struct rum_task			amrr_task[2];

	int				(*newstate)(struct ieee80211vap *,
					    enum ieee80211_state, int);
};
#define	RUM_VAP(vap)	((struct rum_vap *)(vap))

enum {
	RUM_BULK_WR,
	RUM_BULK_RD,
	RUM_N_TRANSFER = 2,
};

struct rum_softc {
	struct ifnet			*sc_ifp;
	device_t			sc_dev;
	struct usb2_device		*sc_udev;
	struct usb2_process		sc_tq;

	const struct ieee80211_rate_table *sc_rates;
	struct usb2_xfer		*sc_xfer[RUM_N_TRANSFER];
	struct rum_task			*sc_last_task;

	uint8_t				rf_rev;
	uint8_t				rffreq;

	enum ieee80211_state		sc_state;
	int				sc_arg;
	struct rum_task			sc_synctask[2];
	struct rum_task			sc_task[2];
	struct rum_task			sc_promisctask[2];
	struct rum_task			sc_scantask[2];
	int				sc_scan_action;
#define RUM_SCAN_START	0
#define RUM_SCAN_END	1
#define RUM_SET_CHANNEL	2

	struct rum_tx_data		tx_data[RUM_TX_LIST_COUNT];
	rum_txdhead			tx_q;
	rum_txdhead			tx_free;
	int				tx_nfree;
	struct rum_rx_desc		sc_rx_desc;

	struct cv			sc_cmd_cv;
	struct mtx			sc_mtx;

	uint32_t			sta[6];
	uint32_t			rf_regs[4];
	uint8_t				txpow[44];
	uint8_t				sc_bssid[6];

	struct {
		uint8_t	val;
		uint8_t	reg;
	} __packed			bbp_prom[16];

	int				hw_radio;
	int				rx_ant;
	int				tx_ant;
	int				nb_ant;
	int				ext_2ghz_lna;
	int				ext_5ghz_lna;
	int				rssi_2ghz_corr;
	int				rssi_5ghz_corr;
	uint8_t				bbp17;

	struct rum_rx_radiotap_header	sc_rxtap;
	int				sc_rxtap_len;

	struct rum_tx_radiotap_header	sc_txtap;
	int				sc_txtap_len;
};

#define RUM_LOCK(sc)		mtx_lock(&(sc)->sc_mtx)
#define RUM_UNLOCK(sc)		mtx_unlock(&(sc)->sc_mtx)
#define RUM_LOCK_ASSERT(sc, t)	mtx_assert(&(sc)->sc_mtx, t)
