<<<<<<< HEAD
/*	$OpenBSD: ieee80211_ioctl.h,v 1.6 2006/06/23 21:34:15 reyk Exp $	*/
=======
/*	$OpenBSD: ieee80211_ioctl.h,v 1.17 2009/02/15 08:34:36 damien Exp $	*/
>>>>>>> origin/master
/*	$NetBSD: ieee80211_ioctl.h,v 1.7 2004/04/30 22:51:04 dyoung Exp $	*/

/*-
 * Copyright (c) 2001 Atsushi Onoe
 * Copyright (c) 2002, 2003 Sam Leffler, Errno Consulting
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/net80211/ieee80211_ioctl.h,v 1.5 2004/03/30 22:57:57 sam Exp $
 */
#ifndef _NET80211_IEEE80211_IOCTL_H_
#define _NET80211_IEEE80211_IOCTL_H_

/*
 * IEEE 802.11 ioctls.
 */

/* per-interface statistics */
struct ieee80211_stats {
	u_int32_t	is_rx_badversion;	/* rx frame with bad version */
	u_int32_t	is_rx_tooshort;		/* rx frame too short */
	u_int32_t	is_rx_wrongbss;		/* rx from wrong bssid */
	u_int32_t	is_rx_dup;		/* rx discard 'cuz dup */
	u_int32_t	is_rx_wrongdir;		/* rx w/ wrong direction */
	u_int32_t	is_rx_mcastecho;	/* rx discard 'cuz mcast echo */
	u_int32_t	is_rx_notassoc;		/* rx discard 'cuz sta !assoc */
	u_int32_t	is_rx_nowep;		/* rx w/ wep but wep !config */
	u_int32_t	is_rx_wepfail;		/* rx wep processing failed */
	u_int32_t	is_rx_decap;		/* rx decapsulation failed */
	u_int32_t	is_rx_mgtdiscard;	/* rx discard mgt frames */
	u_int32_t	is_rx_ctl;		/* rx discard ctrl frames */
	u_int32_t	is_rx_rstoobig;		/* rx rate set truncated */
	u_int32_t	is_rx_elem_missing;	/* rx required element missing*/
	u_int32_t	is_rx_elem_toobig;	/* rx element too big */
	u_int32_t	is_rx_elem_toosmall;	/* rx element too small */
	u_int32_t	is_rx_elem_unknown;	/* rx element unknown */
	u_int32_t	is_rx_badchan;		/* rx frame w/ invalid chan */
	u_int32_t	is_rx_chanmismatch;	/* rx frame chan mismatch */
	u_int32_t	is_rx_nodealloc;	/* rx frame dropped */
	u_int32_t	is_rx_ssidmismatch;	/* rx frame ssid mismatch  */
	u_int32_t	is_rx_auth_unsupported;	/* rx w/ unsupported auth alg */
	u_int32_t	is_rx_auth_fail;	/* rx sta auth failure */
	u_int32_t	is_rx_assoc_bss;	/* rx assoc from wrong bssid */
	u_int32_t	is_rx_assoc_notauth;	/* rx assoc w/o auth */
	u_int32_t	is_rx_assoc_capmismatch;/* rx assoc w/ cap mismatch */
	u_int32_t	is_rx_assoc_norate;	/* rx assoc w/ no rate match */
	u_int32_t	is_rx_deauth;		/* rx deauthentication */
	u_int32_t	is_rx_disassoc;		/* rx disassociation */
	u_int32_t	is_rx_badsubtype;	/* rx frame w/ unknown subtype*/
	u_int32_t	is_rx_nombuf;		/* rx failed for lack of mbuf */
	u_int32_t	is_rx_decryptcrc;	/* rx decrypt failed on crc */
	u_int32_t	is_rx_ahdemo_mgt;	/* rx discard ahdemo mgt frame*/
	u_int32_t	is_rx_bad_auth;		/* rx bad auth request */
	u_int32_t	is_tx_nombuf;		/* tx failed for lack of mbuf */
	u_int32_t	is_tx_nonode;		/* tx failed for no node */
	u_int32_t	is_tx_unknownmgt;	/* tx of unknown mgt frame */
	u_int32_t	is_scan_active;		/* active scans started */
	u_int32_t	is_scan_passive;	/* passive scans started */
	u_int32_t	is_node_timeout;	/* nodes timed out inactivity */
	u_int32_t	is_crypto_nomem;	/* no memory for crypto ctx */
<<<<<<< HEAD
=======
	u_int32_t	is_rx_assoc_badrsnie;	/* rx assoc w/ bad RSN IE */
	u_int32_t	is_rx_unauth;		/* rx port not valid */
	u_int32_t	is_tx_noauth;		/* tx port not valid */
	u_int32_t	is_rx_eapol_key;	/* rx eapol-key frames */
	u_int32_t	is_rx_eapol_replay;	/* rx replayed eapol frames */
	u_int32_t	is_rx_eapol_badmic;	/* rx eapol frames w/ bad mic */
	u_int32_t	is_rx_remmicfail;	/* rx tkip remote mic fails */
	u_int32_t	is_rx_locmicfail;	/* rx tkip local mic fails */
	u_int32_t	is_tkip_replays;
	u_int32_t	is_tkip_icv_errs;
	u_int32_t	is_ccmp_replays;
	u_int32_t	is_ccmp_dec_errs;
	u_int32_t	is_cmac_replays;
	u_int32_t	is_cmac_icv_errs;
	u_int32_t	is_pbac_errs;
>>>>>>> origin/master
};

#define	SIOCG80211STATS		_IOWR('i', 242, struct ifreq)

/* network identifier (ESSID), nwid is pointed at by ifr.ifr_data */
struct ieee80211_nwid {
	u_int8_t	i_len;
	u_int8_t	i_nwid[IEEE80211_NWID_LEN];
};

#define	SIOCS80211NWID		_IOWR('i', 230, struct ifreq)
#define	SIOCG80211NWID		_IOWR('i', 231, struct ifreq)

/* network key (WEP), the first member must be matched with struct ifreq */
struct ieee80211_nwkey {
	char		i_name[IFNAMSIZ];	/* if_name, e.g. "wi0" */
	int		i_wepon;		/* wep enabled flag */
	int		i_defkid;		/* default encrypt key id */
	struct {
		int		i_keylen;
		u_int8_t	*i_keydat;
	}		i_key[IEEE80211_WEP_NKID];
};

#define	IEEE80211_NWKEY_OPEN	0		/* No privacy */
#define	IEEE80211_NWKEY_WEP	1		/* WEP enabled */
#define	IEEE80211_NWKEY_EAP	2		/* EAP enabled */
#define	IEEE80211_NWKEY_PERSIST	0x100		/* designate persist keyset */

#define	SIOCS80211NWKEY		 _IOW('i', 232, struct ieee80211_nwkey)
#define	SIOCG80211NWKEY		_IOWR('i', 233, struct ieee80211_nwkey)

/* power management parameters */
struct ieee80211_power {
	char		i_name[IFNAMSIZ];	/* if_name, e.g. "wi0" */
	int		i_enabled;		/* 1 == on, 0 == off */
	int		i_maxsleep;		/* max sleep in ms */
};
#define	SIOCS80211POWER		 _IOW('i', 234, struct ieee80211_power)
#define	SIOCG80211POWER		_IOWR('i', 235, struct ieee80211_power)

/* authentication type */
struct ieee80211_auth {
	char		i_name[IFNAMSIZ];	/* if_name, e.g. "wi0" */
	int		i_authtype;
};

#define	IEEE80211_AUTH_NONE	0
#define	IEEE80211_AUTH_OPEN	1
#define	IEEE80211_AUTH_SHARED	2

#define	SIOCS80211AUTH		 _IOW('i', 236, struct ieee80211_auth)
#define	SIOCG80211AUTH		_IOWR('i', 237, struct ieee80211_auth)

/* channel request */
struct ieee80211chanreq {
	char		i_name[IFNAMSIZ];	/* if_name, e.g. "wi0" */
	u_int16_t	i_channel;
};

#ifndef _KERNEL
/*
 * Channels are specified by frequency and attributes.
 */
struct ieee80211_channel {
	u_int16_t	ic_freq;	/* setting in MHz */
	u_int16_t	ic_flags;	/* see below */
};

/*
 * Channel attributes (XXX must keep in sync with radiotap flags).
 */
#define IEEE80211_CHAN_TURBO	0x0010	/* Turbo channel */
#define IEEE80211_CHAN_CCK	0x0020	/* CCK channel */
#define IEEE80211_CHAN_OFDM	0x0040	/* OFDM channel */
#define IEEE80211_CHAN_2GHZ	0x0080	/* 2 GHz spectrum channel */
#define IEEE80211_CHAN_5GHZ	0x0100	/* 5 GHz spectrum channel */
#define IEEE80211_CHAN_PASSIVE	0x0200	/* Only passive scan allowed */
#define IEEE80211_CHAN_DYN	0x0400	/* Dynamic CCK-OFDM channel */
#define IEEE80211_CHAN_XR	0x1000	/* Extended range OFDM channel */
#endif	/* !_KERNEL */

struct ieee80211_chanreq_all {
	char		i_name[IFNAMSIZ];	/* if_name, e.g. "wi0" */
	struct ieee80211_channel *i_chans;
};

#ifndef IEEE80211_CHAN_ANY
#define	IEEE80211_CHAN_ANY	0xffff
#endif

#define	SIOCS80211CHANNEL	 _IOW('i', 238, struct ieee80211chanreq)
#define	SIOCG80211CHANNEL	_IOWR('i', 239, struct ieee80211chanreq)
#define	SIOCG80211ALLCHANS	_IOWR('i', 215, struct ieee80211_chanreq_all)

/* BSS identifier */
struct ieee80211_bssid {
	char		i_name[IFNAMSIZ];	/* if_name, e.g. "wi0" */
	u_int8_t	i_bssid[IEEE80211_ADDR_LEN];
};

#define	SIOCS80211BSSID		 _IOW('i', 240, struct ieee80211_bssid)
#define	SIOCG80211BSSID		_IOWR('i', 241, struct ieee80211_bssid)

/* transmit power */
struct ieee80211_txpower {
	char		i_name[IFNAMSIZ];	/* if_name, e.g. "wi0" */
	int		i_mode;			/* auto, manual */
	int16_t		i_val;			/* dBm */
};

#define	SIOCS80211TXPOWER	 _IOW('i', 243, struct ieee80211_txpower)
#define	SIOCG80211TXPOWER	_IOWR('i', 244, struct ieee80211_txpower)

#define IEEE80211_TXPOWER_MODE_FIXED	0	/* fixed tx power value */
#define IEEE80211_TXPOWER_MODE_AUTO	1	/* auto level control */

<<<<<<< HEAD
=======
struct ieee80211_wpapsk {
	char		i_name[IFNAMSIZ];	/* if_name, e.g. "wi0" */
	int		i_enabled;
	u_int8_t	i_psk[32];
};

#define SIOCS80211WPAPSK	 _IOW('i', 245, struct ieee80211_wpapsk)
#define SIOCG80211WPAPSK	_IOWR('i', 246, struct ieee80211_wpapsk)

#define IEEE80211_WPA_PROTO_WPA1	0x01
#define IEEE80211_WPA_PROTO_WPA2	0x02

#define IEEE80211_WPA_CIPHER_NONE	0x00
#define IEEE80211_WPA_CIPHER_USEGROUP	0x01
#define IEEE80211_WPA_CIPHER_WEP40	0x02
#define IEEE80211_WPA_CIPHER_TKIP	0x04
#define IEEE80211_WPA_CIPHER_CCMP	0x08
#define IEEE80211_WPA_CIPHER_WEP104	0x10

#define IEEE80211_WPA_AKM_PSK		0x01
#define IEEE80211_WPA_AKM_8021X		0x02
#define IEEE80211_WPA_AKM_SHA256_PSK	0x04
#define IEEE80211_WPA_AKM_SHA256_8021X	0x08

struct ieee80211_wpaparams {
	char	i_name[IFNAMSIZ];		/* if_name, e.g. "wi0" */
	int	i_enabled;
	u_int	i_protos;
	u_int	i_akms;
	u_int	i_ciphers;
	u_int	i_groupcipher;
};

#define SIOCS80211WPAPARMS	 _IOW('i', 247, struct ieee80211_wpaparams)
#define SIOCG80211WPAPARMS	_IOWR('i', 248, struct ieee80211_wpaparams)

struct ieee80211_wmmparams {
	char	i_name[IFNAMSIZ];		/* if_name, e.g. "wi0" */
	int	i_enabled;
	/* XXX more */
};

#define SIOCS80211WMMPARMS	 _IOW('i', 249, struct ieee80211_wmmparams)
#define SIOCG80211WMMPARMS	_IOWR('i', 250, struct ieee80211_wmmparams)

struct ieee80211_keyavail {
	char		i_name[IFNAMSIZ];	/* if_name, e.g. "wi0" */
	u_int8_t	i_macaddr[IEEE80211_ADDR_LEN];
	u_int8_t	i_key[32];
	u_int32_t	i_lifetime;
};

struct ieee80211_keyrun {
	char		i_name[IFNAMSIZ];	/* if_name, e.g. "wi0" */
	u_int8_t	i_macaddr[IEEE80211_ADDR_LEN];
};

#define SIOCS80211KEYAVAIL	 _IOW('i', 251, struct ieee80211_keyavail)
#define SIOCS80211KEYRUN	 _IOW('i', 252, struct ieee80211_keyrun)

>>>>>>> origin/master
/* scan request (will block) */
#define IEEE80211_SCAN_TIMEOUT	30	/* timeout in seconds */

#define SIOCS80211SCAN		 _IOW('i', 210, struct ifreq)

/* node and requests */
struct ieee80211_nodereq {
	char		nr_ifname[IFNAMSIZ];		/* e.g. "ath0" */

	/* Node address and name information */
	u_int8_t	nr_macaddr[IEEE80211_ADDR_LEN];	/* node lladdr */
	u_int8_t	nr_bssid[IEEE80211_ADDR_LEN];	/* bssid */
	u_int8_t	nr_nwid_len;			/* ESSID length */
	u_int8_t	nr_nwid[IEEE80211_NWID_LEN];	/* ESSID */

	/* Channel and rates */
	u_int16_t	nr_channel;			/* last channel */
	u_int16_t	nr_chan_flags;			/* channel flags */
	u_int8_t	nr_nrates;			/* rate count */
	u_int8_t	nr_rates[IEEE80211_RATE_MAXSIZE];	/* rate set */

	/* Node status information */
	u_int8_t	nr_rssi;	/* received signal strength */
	u_int8_t	nr_max_rssi;	/* maximum rssi */
	u_int8_t	nr_tstamp[8];	/* from last received beacon */
	u_int16_t	nr_intval;	/* beacon interval */
	u_int16_t	nr_capinfo;	/* capabilities */
	u_int16_t	nr_fhdwell;	/* FH only */
	u_int8_t	nr_fhindex;	/* FH only */
	u_int8_t	nr_erp;		/* 11g only */
	u_int8_t	nr_pwrsave;	/* power saving mode */
	u_int16_t	nr_associd;	/* assoc response */
	u_int16_t	nr_txseq;	/* seq to be transmitted */
	u_int16_t	nr_rxseq;	/* seq previous received */
	u_int32_t	nr_fails;	/* failure count to associate */
	u_int32_t	nr_inact;	/* inactivity mark count */
	u_int8_t	nr_txrate;	/* index to nr_rates[] */
	u_int16_t	nr_state;	/* node state in the cache */

	/* Node flags */
	u_int8_t	nr_flags;
};

#define IEEE80211_NODEREQ_STATE(_s)	(1 << _s)
#define IEEE80211_NODEREQ_STATE_BITS					\
	"\20\01CACHE\02BSS\03AUTH\04ASSOC\05COLLECT"

#define IEEE80211_NODEREQ_RSSI(_nr)					\
	((u_int)(((float)(_nr)->nr_rssi / (_nr)->nr_max_rssi) * 100))

#define IEEE80211_NODEREQ_STA		0x00	/* station */
#define IEEE80211_NODEREQ_AP		0x01	/* access point */
#define IEEE80211_NODEREQ_AP_BSS	0x02	/* current bss access point */
#define IEEE80211_NODEREQ_COPY		0x04	/* add node with flags */

#define SIOCG80211NODE		_IOWR('i', 211, struct ieee80211_nodereq)
#define SIOCS80211NODE		 _IOW('i', 212, struct ieee80211_nodereq)
#define SIOCS80211DELNODE	 _IOW('i', 213, struct ieee80211_nodereq)

/* get the entire node cache */
struct ieee80211_nodereq_all {
	char				na_ifname[IFNAMSIZ];	/* e.g. "ath0" */

	int				na_nodes;	/* returned count */
	size_t				na_size;	/* size of node buffer */
	struct ieee80211_nodereq	*na_node;	/* allocated node buffer */

	/* Match nodes by flag */
	u_int8_t			na_flags;	/* IEEE80211_NODEREQ_* */
};

#define SIOCG80211ALLNODES	_IOWR('i', 214, struct ieee80211_nodereq)

/* net80211 specific interface flags */
#define IEEE80211_F_HIDENWID	0x10000000	/* CONF: hidden ssid mode */
#define IEEE80211_F_NOBRIDGE	0x20000000	/* CONF: no internal bridging */
#define IEEE80211_F_HOSTAPMASK	0x30000000
#define IEEE80211_F_USERSHIFT	28
#define IEEE80211_F_USERBITS	"\20\01HIDENWID\02NOBRIDGE"

struct ieee80211_flags {
	const char		*f_name;
	u_int			f_flag;
};

#define IEEE80211_FLAGS	{						\
	{ "hidenwid", IEEE80211_F_HIDENWID >> IEEE80211_F_USERSHIFT },	\
	{ "nobridge", IEEE80211_F_NOBRIDGE >> IEEE80211_F_USERSHIFT }	\
}

#define SIOCG80211FLAGS		_IOWR('i', 216, struct ifreq)
#define SIOCS80211FLAGS		 _IOW('i', 217, struct ifreq)

#endif /* _NET80211_IEEE80211_IOCTL_H_ */
