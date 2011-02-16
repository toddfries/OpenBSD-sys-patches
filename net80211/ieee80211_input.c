<<<<<<< HEAD
/*	$NetBSD: ieee80211_input.c,v 1.24 2004/05/31 11:12:24 dyoung Exp $	*/
/*	$OpenBSD: ieee80211_input.c,v 1.21 2006/08/29 18:10:34 damien Exp $	*/
/*-
 * Copyright (c) 2001 Atsushi Onoe
 * Copyright (c) 2002, 2003 Sam Leffler, Errno Consulting
=======
/*	$OpenBSD: ieee80211_input.c,v 1.116 2010/06/07 16:51:22 damien Exp $	*/

/*-
 * Copyright (c) 2001 Atsushi Onoe
 * Copyright (c) 2002, 2003 Sam Leffler, Errno Consulting
 * Copyright (c) 2007-2009 Damien Bergamini
>>>>>>> origin/master
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
 */

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/endian.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_arp.h>
#include <net/if_llc.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_ether.h>
#endif

#include <net80211/ieee80211_var.h>
<<<<<<< HEAD

#include <dev/rndvar.h>

const struct timeval ieee80211_merge_print_intvl = {
	.tv_sec = 1,
	.tv_usec = 0
};

static void ieee80211_recv_pspoll(struct ieee80211com *,
    struct mbuf *, int, u_int32_t);
static int ieee80211_do_slow_print(struct ieee80211com *, int *);
=======
#include <net80211/ieee80211_priv.h>

struct	mbuf *ieee80211_defrag(struct ieee80211com *, struct mbuf *, int);
void	ieee80211_defrag_timeout(void *);
#ifndef IEEE80211_NO_HT
void	ieee80211_input_ba(struct ifnet *, struct mbuf *,
	    struct ieee80211_node *, int, struct ieee80211_rxinfo *);
void	ieee80211_ba_move_window(struct ieee80211com *,
	    struct ieee80211_node *, u_int8_t, u_int16_t);
#endif
struct	mbuf *ieee80211_align_mbuf(struct mbuf *);
void	ieee80211_decap(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *, int);
#ifndef IEEE80211_NO_HT
void	ieee80211_amsdu_decap(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *, int);
#endif
void	ieee80211_deliver_data(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *);
int	ieee80211_parse_edca_params_body(struct ieee80211com *,
	    const u_int8_t *);
int	ieee80211_parse_edca_params(struct ieee80211com *, const u_int8_t *);
int	ieee80211_parse_wmm_params(struct ieee80211com *, const u_int8_t *);
enum	ieee80211_cipher ieee80211_parse_rsn_cipher(const u_int8_t[]);
enum	ieee80211_akm ieee80211_parse_rsn_akm(const u_int8_t[]);
int	ieee80211_parse_rsn_body(struct ieee80211com *, const u_int8_t *,
	    u_int, struct ieee80211_rsnparams *);
int	ieee80211_save_ie(const u_int8_t *, u_int8_t **);
void	ieee80211_recv_probe_resp(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *, struct ieee80211_rxinfo *, int);
#ifndef IEEE80211_STA_ONLY
void	ieee80211_recv_probe_req(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *, struct ieee80211_rxinfo *);
#endif
void	ieee80211_recv_auth(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *, struct ieee80211_rxinfo *);
#ifndef IEEE80211_STA_ONLY
void	ieee80211_recv_assoc_req(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *, struct ieee80211_rxinfo *, int);
#endif
void	ieee80211_recv_assoc_resp(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *, int);
void	ieee80211_recv_deauth(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *);
void	ieee80211_recv_disassoc(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *);
#ifndef IEEE80211_NO_HT
void	ieee80211_recv_addba_req(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *);
void	ieee80211_recv_addba_resp(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *);
void	ieee80211_recv_delba(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *);
#endif
void	ieee80211_recv_sa_query_req(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *);
#ifndef IEEE80211_STA_ONLY
void	ieee80211_recv_sa_query_resp(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *);
#endif
void	ieee80211_recv_action(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *);
#ifndef IEEE80211_STA_ONLY
void	ieee80211_recv_pspoll(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *);
#endif
#ifndef IEEE80211_NO_HT
void	ieee80211_recv_bar(struct ieee80211com *, struct mbuf *,
	    struct ieee80211_node *);
void	ieee80211_bar_tid(struct ieee80211com *, struct ieee80211_node *,
	    u_int8_t, u_int16_t);
#endif

/*
 * Retrieve the length in bytes of an 802.11 header.
 */
u_int
ieee80211_get_hdrlen(const struct ieee80211_frame *wh)
{
	u_int size = sizeof(*wh);

	/* NB: does not work with control frames */
	KASSERT(ieee80211_has_seq(wh));

	if (ieee80211_has_addr4(wh))
		size += IEEE80211_ADDR_LEN;	/* i_addr4 */
	if (ieee80211_has_qos(wh))
		size += sizeof(u_int16_t);	/* i_qos */
	if (ieee80211_has_htc(wh))
		size += sizeof(u_int32_t);	/* i_ht */
	return size;
}
>>>>>>> origin/master

/*
 * Process a received frame.  The node associated with the sender
 * should be supplied.  If nothing was found in the node table then
 * the caller is assumed to supply a reference to ic_bss instead.
 * The RSSI and a timestamp are also supplied.  The RSSI data is used
 * during AP scanning to select a AP to associate with; it can have
 * any units so long as values have consistent units and higher values
 * mean ``better signal''.  The receive timestamp is currently not used
 * by the 802.11 layer.
 */
void
ieee80211_input(struct ifnet *ifp, struct mbuf *m, struct ieee80211_node *ni,
<<<<<<< HEAD
	int rssi, u_int32_t rstamp)
{
	struct ieee80211com *ic = (void *)ifp;
	struct ieee80211_frame *wh;
	struct ether_header *eh;
	struct mbuf *m1;
	int error, len;
	u_int8_t dir, type, subtype;
	u_int16_t rxseq;

	if (ni == NULL)
		panic("null mode");

	/* trim CRC here so WEP can find its own CRC at the end of packet. */
	if (m->m_flags & M_HASFCS) {
		m_adj(m, -IEEE80211_CRC_LEN);
		m->m_flags &= ~M_HASFCS;
	}
=======
    struct ieee80211_rxinfo *rxi)
{
	struct ieee80211com *ic = (void *)ifp;
	struct ieee80211_frame *wh;
	u_int16_t *orxseq, nrxseq, qos;
	u_int8_t dir, type, subtype, tid;
	int hdrlen, hasqos;

	KASSERT(ni != NULL);
>>>>>>> origin/master

	/* in monitor mode, send everything directly to bpf */
	if (ic->ic_opmode == IEEE80211_M_MONITOR)
		goto out;

	/*
	 * Do not process frames without an Address 2 field any further.
	 * Only CTS and ACK control frames do not have this field.
	 */
	if (m->m_len < sizeof(struct ieee80211_frame_min)) {
		DPRINTF(("frame too short, len %u\n", m->m_len));
		ic->ic_stats.is_rx_tooshort++;
		goto out;
	}

	wh = mtod(m, struct ieee80211_frame *);
	if ((wh->i_fc[0] & IEEE80211_FC0_VERSION_MASK) !=
	    IEEE80211_FC0_VERSION_0) {
		DPRINTF(("frame with wrong version: %x\n", wh->i_fc[0]));
		ic->ic_stats.is_rx_badversion++;
		goto err;
	}

	dir = wh->i_fc[1] & IEEE80211_FC1_DIR_MASK;
	type = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;
<<<<<<< HEAD
	/*
	 * NB: We are not yet prepared to handle control frames,
	 *     but permitting drivers to send them to us allows
	 *     them to go through bpf tapping at the 802.11 layer.
	 */
	if (m->m_pkthdr.len < sizeof(struct ieee80211_frame)) {
		IEEE80211_DPRINTF2(("%s: frame too short (2), len %u\n",
			__func__, m->m_pkthdr.len));
		ic->ic_stats.is_rx_tooshort++;
		goto out;
	}
	if (ic->ic_state != IEEE80211_S_SCAN) {
		ni->ni_rssi = rssi;
		ni->ni_rstamp = rstamp;
		rxseq = ni->ni_rxseq;
		ni->ni_rxseq =
		    letoh16(*(u_int16_t *)wh->i_seq) >> IEEE80211_SEQ_SEQ_SHIFT;
		/* TODO: fragment */
		if ((wh->i_fc[1] & IEEE80211_FC1_RETRY) &&
		    rxseq == ni->ni_rxseq) {
=======

	if (type != IEEE80211_FC0_TYPE_CTL) {
		hdrlen = ieee80211_get_hdrlen(wh);
		if (m->m_len < hdrlen) {
			DPRINTF(("frame too short, len %u\n", m->m_len));
			ic->ic_stats.is_rx_tooshort++;
			goto err;
		}
	}
	if ((hasqos = ieee80211_has_qos(wh))) {
		qos = ieee80211_get_qos(wh);
		tid = qos & IEEE80211_QOS_TID;
	}

	/* duplicate detection (see 9.2.9) */
	if (ieee80211_has_seq(wh) &&
	    ic->ic_state != IEEE80211_S_SCAN) {
		nrxseq = letoh16(*(u_int16_t *)wh->i_seq) >>
		    IEEE80211_SEQ_SEQ_SHIFT;
		if (hasqos)
			orxseq = &ni->ni_qos_rxseqs[tid];
		else
			orxseq = &ni->ni_rxseq;
		if ((wh->i_fc[1] & IEEE80211_FC1_RETRY) &&
		    nrxseq == *orxseq) {
>>>>>>> origin/master
			/* duplicate, silently discarded */
			ic->ic_stats.is_rx_dup++;
			goto out;
		}
		*orxseq = nrxseq;
	}
	if (ic->ic_state != IEEE80211_S_SCAN) {
		ni->ni_rssi = rxi->rxi_rssi;
		ni->ni_rstamp = rxi->rxi_tstamp;
		ni->ni_inact = 0;
		if (ic->ic_opmode == IEEE80211_M_MONITOR)
			goto out;
	}

<<<<<<< HEAD
	if (ic->ic_set_tim != NULL &&
	    (wh->i_fc[1] & IEEE80211_FC1_PWR_MGT)
	    && ni->ni_pwrsave == 0) {
		/* turn on power save mode */

		if (ifp->if_flags & IFF_DEBUG)
			printf("%s: power save mode on for %s\n",
			    ifp->if_xname, ether_sprintf(wh->i_addr2));

		ni->ni_pwrsave = IEEE80211_PS_SLEEP;
	}
	if (ic->ic_set_tim != NULL &&
	    (wh->i_fc[1] & IEEE80211_FC1_PWR_MGT) == 0 &&
	    ni->ni_pwrsave != 0) {
		/* turn off power save mode, dequeue stored packets */

		ni->ni_pwrsave = 0;
		if (ic->ic_set_tim)
			ic->ic_set_tim(ic, ni->ni_associd, 0);

		if (ifp->if_flags & IFF_DEBUG)
			printf("%s: power save mode off for %s\n",
			    ifp->if_xname, ether_sprintf(wh->i_addr2));

		while (!IF_IS_EMPTY(&ni->ni_savedq)) {
			struct mbuf *m;
			IF_DEQUEUE(&ni->ni_savedq, m);
			IF_ENQUEUE(&ic->ic_pwrsaveq, m);
			(*ifp->if_start)(ifp);
=======
#ifndef IEEE80211_STA_ONLY
	if (ic->ic_opmode == IEEE80211_M_HOSTAP &&
	    (ic->ic_caps & IEEE80211_C_APPMGT) &&
	    ni->ni_state == IEEE80211_STA_ASSOC) {
		if (wh->i_fc[1] & IEEE80211_FC1_PWR_MGT) {
			if (ni->ni_pwrsave == IEEE80211_PS_AWAKE) {
				/* turn on PS mode */
				ni->ni_pwrsave = IEEE80211_PS_DOZE;
				ic->ic_pssta++;
				DPRINTF(("PS mode on for %s, count %d\n",
				    ether_sprintf(wh->i_addr2), ic->ic_pssta));
			}
		} else if (ni->ni_pwrsave == IEEE80211_PS_DOZE) {
			/* turn off PS mode */
			ni->ni_pwrsave = IEEE80211_PS_AWAKE;
			ic->ic_pssta--;
			DPRINTF(("PS mode off for %s, count %d\n",
			    ether_sprintf(wh->i_addr2), ic->ic_pssta));

			(*ic->ic_set_tim)(ic, ni->ni_associd, 0);

			/* dequeue buffered unicast frames */
			while (!IF_IS_EMPTY(&ni->ni_savedq)) {
				struct mbuf *m;
				IF_DEQUEUE(&ni->ni_savedq, m);
				IF_ENQUEUE(&ic->ic_pwrsaveq, m);
				(*ifp->if_start)(ifp);
			}
>>>>>>> origin/master
		}
	}
#endif
	switch (type) {
	case IEEE80211_FC0_TYPE_DATA:
		switch (ic->ic_opmode) {
		case IEEE80211_M_STA:
			if (dir != IEEE80211_FC1_DIR_FROMDS) {
				ic->ic_stats.is_rx_wrongdir++;
				goto out;
			}
			if (ic->ic_state != IEEE80211_S_SCAN &&
			    !IEEE80211_ADDR_EQ(wh->i_addr2, ni->ni_bssid)) {
				/* Source address is not our BSS. */
				DPRINTF(("discard frame from SA %s\n",
				    ether_sprintf(wh->i_addr2)));
				ic->ic_stats.is_rx_wrongbss++;
				goto out;
			}
			if ((ifp->if_flags & IFF_SIMPLEX) &&
			    IEEE80211_IS_MULTICAST(wh->i_addr1) &&
			    IEEE80211_ADDR_EQ(wh->i_addr3, ic->ic_myaddr)) {
				/*
				 * In IEEE802.11 network, multicast frame
				 * sent from me is broadcasted from AP.
				 * It should be silently discarded for
				 * SIMPLEX interface.
				 */
				ic->ic_stats.is_rx_mcastecho++;
				goto out;
			}
			break;
#ifndef IEEE80211_STA_ONLY
		case IEEE80211_M_IBSS:
		case IEEE80211_M_AHDEMO:
			if (dir != IEEE80211_FC1_DIR_NODS) {
				ic->ic_stats.is_rx_wrongdir++;
				goto out;
			}
			if (ic->ic_state != IEEE80211_S_SCAN &&
			    !IEEE80211_ADDR_EQ(wh->i_addr3,
				ic->ic_bss->ni_bssid) &&
			    !IEEE80211_ADDR_EQ(wh->i_addr3,
				etherbroadcastaddr)) {
				/* Destination is not our BSS or broadcast. */
				DPRINTF(("discard data frame to DA %s\n",
				    ether_sprintf(wh->i_addr3)));
				ic->ic_stats.is_rx_wrongbss++;
				goto out;
			}
			break;
		case IEEE80211_M_HOSTAP:
			if (dir != IEEE80211_FC1_DIR_TODS) {
				ic->ic_stats.is_rx_wrongdir++;
				goto out;
			}
			if (ic->ic_state != IEEE80211_S_SCAN &&
			    !IEEE80211_ADDR_EQ(wh->i_addr1,
				ic->ic_bss->ni_bssid) &&
			    !IEEE80211_ADDR_EQ(wh->i_addr1,
				etherbroadcastaddr)) {
				/* BSS is not us or broadcast. */
				DPRINTF(("discard data frame to BSS %s\n",
				    ether_sprintf(wh->i_addr1)));
				ic->ic_stats.is_rx_wrongbss++;
				goto out;
			}
			/* check if source STA is associated */
			if (ni == ic->ic_bss) {
				DPRINTF(("data from unknown src %s\n",
				    ether_sprintf(wh->i_addr2)));
				/* NB: caller deals with reference */
				ni = ieee80211_dup_bss(ic, wh->i_addr2);
				if (ni != NULL) {
					IEEE80211_SEND_MGMT(ic, ni,
					    IEEE80211_FC0_SUBTYPE_DEAUTH,
					    IEEE80211_REASON_NOT_AUTHED);
				}
				ic->ic_stats.is_rx_notassoc++;
				goto err;
			}
			if (ni->ni_associd == 0) {
				DPRINTF(("data from unassoc src %s\n",
				    ether_sprintf(wh->i_addr2)));
				IEEE80211_SEND_MGMT(ic, ni,
				    IEEE80211_FC0_SUBTYPE_DISASSOC,
				    IEEE80211_REASON_NOT_ASSOCED);
				ic->ic_stats.is_rx_notassoc++;
				goto err;
			}
			break;
<<<<<<< HEAD
		case IEEE80211_M_MONITOR:
			break;
		}
		if (wh->i_fc[1] & IEEE80211_FC1_WEP) {
			if (ic->ic_flags & IEEE80211_F_WEPON) {
				m = ieee80211_wep_crypt(ifp, m, 0);
=======
#endif	/* IEEE80211_STA_ONLY */
		default:
			/* can't get there */
			goto out;
		}

#ifndef IEEE80211_NO_HT
		if (!(rxi->rxi_flags & IEEE80211_RXI_AMPDU_DONE) &&
		    hasqos && (qos & IEEE80211_QOS_ACK_POLICY_MASK) ==
		    IEEE80211_QOS_ACK_POLICY_BA) {
			/* check if we have a BA agreement for this RA/TID */
			if (ni->ni_rx_ba[tid].ba_state !=
			    IEEE80211_BA_AGREED) {
				DPRINTF(("no BA agreement for %s, TID %d\n",
				    ether_sprintf(ni->ni_macaddr), tid));
				/* send a DELBA with reason code UNKNOWN-BA */
				IEEE80211_SEND_ACTION(ic, ni,
				    IEEE80211_CATEG_BA, IEEE80211_ACTION_DELBA,
				    IEEE80211_REASON_SETUP_REQUIRED << 16 |
				    tid);
				goto err;
			}
			/* go through A-MPDU reordering */
			ieee80211_input_ba(ifp, m, ni, tid, rxi);
			return;	/* don't free m! */
		}
#endif
		if ((ic->ic_flags & IEEE80211_F_WEPON) ||
		    ((ic->ic_flags & IEEE80211_F_RSNON) &&
		     (ni->ni_flags & IEEE80211_NODE_RXPROT))) {
			/* protection is on for Rx */
			if (!(rxi->rxi_flags & IEEE80211_RXI_HWDEC)) {
				if (!(wh->i_fc[1] & IEEE80211_FC1_PROTECTED)) {
					/* drop unencrypted */
					ic->ic_stats.is_rx_unencrypted++;
					goto err;
				}
				/* do software decryption */
				m = ieee80211_decrypt(ic, m, ni);
>>>>>>> origin/master
				if (m == NULL) {
					ic->ic_stats.is_rx_wepfail++;
					goto err;
				}
				wh = mtod(m, struct ieee80211_frame *);
			}
		} else if ((wh->i_fc[1] & IEEE80211_FC1_PROTECTED) ||
		    (rxi->rxi_flags & IEEE80211_RXI_HWDEC)) {
			/* frame encrypted but protection off for Rx */
			ic->ic_stats.is_rx_nowep++;
			goto out;
		}
<<<<<<< HEAD
=======

>>>>>>> origin/master
#if NBPFILTER > 0
		/* copy to listener after decrypt */
		if (ic->ic_rawbpf)
			bpf_mtap(ic->ic_rawbpf, m, BPF_DIRECTION_IN);
#endif
<<<<<<< HEAD
		m = ieee80211_decap(ifp, m);
		if (m == NULL) {
			IEEE80211_DPRINTF(("%s: "
			    "decapsulation error for src %s\n",
			    __func__, ether_sprintf(wh->i_addr2)));
			ic->ic_stats.is_rx_decap++;
			goto err;
		}
		ifp->if_ipackets++;

		/* perform as a bridge within the AP */
		m1 = NULL;
		if (ic->ic_opmode == IEEE80211_M_HOSTAP &&
		    (ic->ic_flags & IEEE80211_F_NOBRIDGE) == 0) {
			eh = mtod(m, struct ether_header *);
			if (ETHER_IS_MULTICAST(eh->ether_dhost)) {
				m1 = m_copym(m, 0, M_COPYALL, M_DONTWAIT);
				if (m1 == NULL)
					ifp->if_oerrors++;
				else
					m1->m_flags |= M_MCAST;
			} else {
				ni = ieee80211_find_node(ic, eh->ether_dhost);
				if (ni != NULL) {
					if (ni->ni_associd != 0) {
						m1 = m;
						m = NULL;
					}
				}
			}
			if (m1 != NULL) {
				len = m1->m_pkthdr.len;
				IFQ_ENQUEUE(&ifp->if_snd, m1, NULL, error);
				if (error)
					ifp->if_oerrors++;
				else {
					if (m != NULL)
						ifp->if_omcasts++;
					ifp->if_obytes += len;
				}
			}
		}
		if (m != NULL) {
#if NBPFILTER > 0
			/*
			 * If we forward packet into transmitter of the AP,
			 * we don't need to duplicate for DLT_EN10MB.
			 */
			if (ifp->if_bpf && m1 == NULL)
				bpf_mtap(ifp->if_bpf, m, BPF_DIRECTION_IN);
#endif
			ether_input_mbuf(ifp, m);
		}
=======

#ifndef IEEE80211_NO_HT
		if ((ni->ni_flags & IEEE80211_NODE_HT) &&
		    hasqos && (qos & IEEE80211_QOS_AMSDU))
			ieee80211_amsdu_decap(ic, m, ni, hdrlen);
		else
#endif
			ieee80211_decap(ic, m, ni, hdrlen);
>>>>>>> origin/master
		return;

	case IEEE80211_FC0_TYPE_MGT:
		if (dir != IEEE80211_FC1_DIR_NODS) {
			ic->ic_stats.is_rx_wrongdir++;
			goto err;
		}
#ifndef IEEE80211_STA_ONLY
		if (ic->ic_opmode == IEEE80211_M_AHDEMO) {
			ic->ic_stats.is_rx_ahdemo_mgt++;
			goto out;
		}
#endif
		subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;

		/* drop frames without interest */
		if (ic->ic_state == IEEE80211_S_SCAN) {
			if (subtype != IEEE80211_FC0_SUBTYPE_BEACON &&
			    subtype != IEEE80211_FC0_SUBTYPE_PROBE_RESP) {
				ic->ic_stats.is_rx_mgtdiscard++;
				goto out;
			}
		}

		if (ni->ni_flags & IEEE80211_NODE_RXMGMTPROT) {
			/* MMPDU protection is on for Rx */
			if (subtype == IEEE80211_FC0_SUBTYPE_DISASSOC ||
			    subtype == IEEE80211_FC0_SUBTYPE_DEAUTH ||
			    subtype == IEEE80211_FC0_SUBTYPE_ACTION) {
				if (!IEEE80211_IS_MULTICAST(wh->i_addr1) &&
				    !(wh->i_fc[1] & IEEE80211_FC1_PROTECTED)) {
					/* unicast mgmt not encrypted */
					goto out;
				}
				/* do software decryption */
				m = ieee80211_decrypt(ic, m, ni);
				if (m == NULL) {
					/* XXX stats */
					goto out;
				}
				wh = mtod(m, struct ieee80211_frame *);
			}
		} else if ((ic->ic_flags & IEEE80211_F_RSNON) &&
		    (wh->i_fc[1] & IEEE80211_FC1_PROTECTED)) {
			/* encrypted but MMPDU Rx protection off for TA */
			goto out;
		}

		if (ifp->if_flags & IFF_DEBUG) {
			/* avoid to print too many frames */
			int doprint = 0;

			switch (subtype) {
			case IEEE80211_FC0_SUBTYPE_BEACON:
				if (ic->ic_state == IEEE80211_S_SCAN)
					doprint = 1;
				break;
#ifndef IEEE80211_STA_ONLY
			case IEEE80211_FC0_SUBTYPE_PROBE_REQ:
				if (ic->ic_opmode == IEEE80211_M_IBSS)
					doprint = 1;
				break;
#endif
			default:
				doprint = 1;
				break;
			}
#ifdef IEEE80211_DEBUG
			doprint += ieee80211_debug;
#endif
			if (doprint)
				printf("%s: received %s from %s rssi %d mode %s\n",
				    ifp->if_xname,
				    ieee80211_mgt_subtype_name[subtype
				    >> IEEE80211_FC0_SUBTYPE_SHIFT],
				    ether_sprintf(wh->i_addr2), rxi->rxi_rssi,
				    ieee80211_phymode_name[ieee80211_chan2mode(ic,
				    ic->ic_bss->ni_chan)]);
		}
#if NBPFILTER > 0
		if (ic->ic_rawbpf)
			bpf_mtap(ic->ic_rawbpf, m, BPF_DIRECTION_IN);
		/*
		 * Drop mbuf if it was filtered by bpf. Normally, this is
		 * done in ether_input() but IEEE 802.11 management frames
		 * are a special case.
		 */
		if (m->m_flags & M_FILDROP) {
			m_freem(m);
			return;
		}
#endif
		(*ic->ic_recv_mgmt)(ic, m, ni, rxi, subtype);
		m_freem(m);
		return;

	case IEEE80211_FC0_TYPE_CTL:
		ic->ic_stats.is_rx_ctl++;
		subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;
<<<<<<< HEAD
		if (subtype == IEEE80211_FC0_SUBTYPE_PS_POLL) {
			/* XXX statistic */
			/* Dump out a single packet from the host */
			if (ifp->if_flags & IFF_DEBUG)
				printf("%s: got power save probe from %s\n",
				    ifp->if_xname,
				    ether_sprintf(wh->i_addr2));
			ieee80211_recv_pspoll(ic, m, rssi, rstamp);
=======
		switch (subtype) {
#ifndef IEEE80211_STA_ONLY
		case IEEE80211_FC0_SUBTYPE_PS_POLL:
			ieee80211_recv_pspoll(ic, m, ni);
			break;
#endif
#ifndef IEEE80211_NO_HT
		case IEEE80211_FC0_SUBTYPE_BAR:
			ieee80211_recv_bar(ic, m, ni);
			break;
#endif
		default:
			break;
>>>>>>> origin/master
		}
		goto out;

	default:
		DPRINTF(("bad frame type %x\n", type));
		/* should not come here */
		break;
	}
 err:
	ifp->if_ierrors++;
 out:
	if (m != NULL) {
#if NBPFILTER > 0
		if (ic->ic_rawbpf)
			bpf_mtap(ic->ic_rawbpf, m, BPF_DIRECTION_IN);
#endif
		m_freem(m);
	}
}

/*
 * Handle defragmentation (see 9.5 and Annex C).  We support the concurrent
 * reception of fragments of three fragmented MSDUs or MMPDUs.
 */
struct mbuf *
<<<<<<< HEAD
ieee80211_decap(struct ifnet *ifp, struct mbuf *m)
{
	struct ether_header *eh;
	struct ieee80211_frame wh;
	struct llc *llc;

	if (m->m_len < sizeof(wh) + sizeof(*llc)) {
		m = m_pullup(m, sizeof(wh) + sizeof(*llc));
		if (m == NULL)
			return NULL;
	}
	memcpy(&wh, mtod(m, caddr_t), sizeof(wh));
	llc = (struct llc *)(mtod(m, caddr_t) + sizeof(wh));
	if (llc->llc_dsap == LLC_SNAP_LSAP && llc->llc_ssap == LLC_SNAP_LSAP &&
	    llc->llc_control == LLC_UI && llc->llc_snap.org_code[0] == 0 &&
	    llc->llc_snap.org_code[1] == 0 && llc->llc_snap.org_code[2] == 0) {
		m_adj(m, sizeof(wh) + sizeof(struct llc) - sizeof(*eh));
		llc = NULL;
	} else {
		m_adj(m, sizeof(wh) - sizeof(*eh));
	}
	eh = mtod(m, struct ether_header *);
	switch (wh.i_fc[1] & IEEE80211_FC1_DIR_MASK) {
	case IEEE80211_FC1_DIR_NODS:
		IEEE80211_ADDR_COPY(eh->ether_dhost, wh.i_addr1);
		IEEE80211_ADDR_COPY(eh->ether_shost, wh.i_addr2);
		break;
	case IEEE80211_FC1_DIR_TODS:
		IEEE80211_ADDR_COPY(eh->ether_dhost, wh.i_addr3);
		IEEE80211_ADDR_COPY(eh->ether_shost, wh.i_addr2);
		break;
	case IEEE80211_FC1_DIR_FROMDS:
		IEEE80211_ADDR_COPY(eh->ether_dhost, wh.i_addr1);
		IEEE80211_ADDR_COPY(eh->ether_shost, wh.i_addr3);
=======
ieee80211_defrag(struct ieee80211com *ic, struct mbuf *m, int hdrlen)
{
	const struct ieee80211_frame *owh, *wh;
	struct ieee80211_defrag *df;
	u_int16_t rxseq, seq;
	u_int8_t frag;
	int i;

	wh = mtod(m, struct ieee80211_frame *);
	rxseq = letoh16(*(const u_int16_t *)wh->i_seq);
	seq = rxseq >> IEEE80211_SEQ_SEQ_SHIFT;
	frag = rxseq & IEEE80211_SEQ_FRAG_MASK;

	if (frag == 0 && !(wh->i_fc[1] & IEEE80211_FC1_MORE_FRAG))
		return m;	/* not fragmented */

	if (frag == 0) {
		/* first fragment, setup entry in the fragment cache */
		if (++ic->ic_defrag_cur == IEEE80211_DEFRAG_SIZE)
			ic->ic_defrag_cur = 0;
		df = &ic->ic_defrag[ic->ic_defrag_cur];
		if (df->df_m != NULL)
			m_freem(df->df_m);	/* discard old entry */
		df->df_seq = seq;
		df->df_frag = 0;
		df->df_m = m;
		/* start receive MSDU timer of aMaxReceiveLifetime */
		timeout_add_sec(&df->df_to, 1);
		return NULL;	/* MSDU or MMPDU not yet complete */
	}

	/* find matching entry in the fragment cache */
	for (i = 0; i < IEEE80211_DEFRAG_SIZE; i++) {
		df = &ic->ic_defrag[i];
		if (df->df_m == NULL)
			continue;
		if (df->df_seq != seq || df->df_frag + 1 != frag)
			continue;
		owh = mtod(df->df_m, struct ieee80211_frame *);
		/* frame type, source and destination must match */
		if (((wh->i_fc[0] ^ owh->i_fc[0]) & IEEE80211_FC0_TYPE_MASK) ||
		    !IEEE80211_ADDR_EQ(wh->i_addr1, owh->i_addr1) ||
		    !IEEE80211_ADDR_EQ(wh->i_addr2, owh->i_addr2))
			continue;
		/* matching entry found */
>>>>>>> origin/master
		break;
	}
	if (i == IEEE80211_DEFRAG_SIZE) {
		/* no matching entry found, discard fragment */
		ic->ic_if.if_ierrors++;
		m_freem(m);
		return NULL;
	}

	df->df_frag = frag;
	/* strip 802.11 header and concatenate fragment */
	m_adj(m, hdrlen);
	m_cat(df->df_m, m);
	df->df_m->m_pkthdr.len += m->m_pkthdr.len;

	if (wh->i_fc[1] & IEEE80211_FC1_MORE_FRAG)
		return NULL;	/* MSDU or MMPDU not yet complete */

	/* MSDU or MMPDU complete */
	timeout_del(&df->df_to);
	m = df->df_m;
	df->df_m = NULL;
	return m;
}

/*
<<<<<<< HEAD
 * Install received rate set information in the node's state block.
 */
static int
ieee80211_setup_rates(struct ieee80211com *ic, struct ieee80211_node *ni,
	u_int8_t *rates, u_int8_t *xrates, int flags)
{
	struct ieee80211_rateset *rs = &ni->ni_rates;

	memset(rs, 0, sizeof(*rs));
	rs->rs_nrates = rates[1];
	memcpy(rs->rs_rates, rates + 2, rs->rs_nrates);
	if (xrates != NULL) {
		u_int8_t nxrates;
		/*
		 * Tack on 11g extended supported rate element.
		 */
		nxrates = xrates[1];
		if (rs->rs_nrates + nxrates > IEEE80211_RATE_MAXSIZE) {
			nxrates = IEEE80211_RATE_MAXSIZE - rs->rs_nrates;
			IEEE80211_DPRINTF(("%s: extended rate set too large;"
				" only using %u of %u rates\n",
				__func__, nxrates, xrates[1]));
			ic->ic_stats.is_rx_rstoobig++;
		}
		memcpy(rs->rs_rates + rs->rs_nrates, xrates+2, nxrates);
		rs->rs_nrates += nxrates;
	}
	return ieee80211_fix_rate(ic, ni, flags);
}

/* Verify the existence and length of __elem or get out. */
#define IEEE80211_VERIFY_ELEMENT(__elem, __maxlen) do {			\
	if ((__elem) == NULL) {						\
		IEEE80211_DPRINTF(("%s: no " #__elem "in %s frame\n",	\
			__func__, ieee80211_mgt_subtype_name[subtype >>	\
				IEEE80211_FC0_SUBTYPE_SHIFT]));		\
		ic->ic_stats.is_rx_elem_missing++;			\
		return;							\
	}								\
	if ((__elem)[1] > (__maxlen)) {					\
		IEEE80211_DPRINTF(("%s: bad " #__elem " len %d in %s "	\
			"frame from %s\n", __func__, (__elem)[1],	\
			ieee80211_mgt_subtype_name[subtype >>		\
				IEEE80211_FC0_SUBTYPE_SHIFT],		\
			ether_sprintf(wh->i_addr2)));			\
		ic->ic_stats.is_rx_elem_toobig++;			\
		return;							\
	}								\
} while (0)

#define	IEEE80211_VERIFY_LENGTH(_len, _minlen) do {			\
	if ((_len) < (_minlen)) {					\
		IEEE80211_DPRINTF(("%s: %s frame too short from %s\n",	\
			__func__,					\
			ieee80211_mgt_subtype_name[subtype >>		\
				IEEE80211_FC0_SUBTYPE_SHIFT],		\
			ether_sprintf(wh->i_addr2)));			\
		ic->ic_stats.is_rx_elem_toosmall++;			\
		return;							\
	}								\
} while (0)

#ifdef IEEE80211_DEBUG
static void
ieee80211_ssid_mismatch(struct ieee80211com *ic, const char *tag,
    u_int8_t mac[IEEE80211_ADDR_LEN], u_int8_t *ssid)
{
	printf("[%s] %s req ssid mismatch: ", ether_sprintf(mac), tag);
	ieee80211_print_essid(ssid + 2, ssid[1]);
	printf("\n");
=======
 * Receive MSDU defragmentation timer exceeds aMaxReceiveLifetime.
 */
void
ieee80211_defrag_timeout(void *arg)
{
	struct ieee80211_defrag *df = arg;
	int s = splnet();

	/* discard all received fragments */
	m_freem(df->df_m);
	df->df_m = NULL;

	splx(s);
}

#ifndef IEEE80211_NO_HT
/*
 * Process a received data MPDU related to a specific HT-immediate Block Ack
 * agreement (see 9.10.7.6).
 */
void
ieee80211_input_ba(struct ifnet *ifp, struct mbuf *m,
    struct ieee80211_node *ni, int tid, struct ieee80211_rxinfo *rxi)
{
	struct ieee80211_rx_ba *ba = &ni->ni_rx_ba[tid];
	struct ieee80211_frame *wh;
	int idx, count;
	u_int16_t sn;

	wh = mtod(m, struct ieee80211_frame *);
	sn = letoh16(*(u_int16_t *)wh->i_seq) >> IEEE80211_SEQ_SEQ_SHIFT;

	/* reset Block Ack inactivity timer */
	timeout_add_usec(&ba->ba_to, ba->ba_timeout_val);

	if (SEQ_LT(sn, ba->ba_winstart)) {	/* SN < WinStartB */
		ifp->if_ierrors++;
		m_freem(m);	/* discard the MPDU */
		return;
	}
	if (SEQ_LT(ba->ba_winend, sn)) {	/* WinEndB < SN */
		count = (sn - ba->ba_winend) & 0xfff;
		if (count > ba->ba_winsize)	/* no overlap */
			count = ba->ba_winsize;
		while (count-- > 0) {
			/* gaps may exist */
			if (ba->ba_buf[ba->ba_head].m != NULL) {
				ieee80211_input(ifp, ba->ba_buf[ba->ba_head].m,
				    ni, &ba->ba_buf[ba->ba_head].rxi);
				ba->ba_buf[ba->ba_head].m = NULL;
			}
			ba->ba_head = (ba->ba_head + 1) %
			    IEEE80211_BA_MAX_WINSZ;
		}
		/* move window forward */
		ba->ba_winend = sn;
		ba->ba_winstart = (sn - ba->ba_winsize + 1) & 0xfff;
	}
	/* WinStartB <= SN <= WinEndB */

	idx = (sn - ba->ba_winstart) & 0xfff;
	idx = (ba->ba_head + idx) % IEEE80211_BA_MAX_WINSZ;
	/* store the received MPDU in the buffer */
	if (ba->ba_buf[idx].m != NULL) {
		ifp->if_ierrors++;
		m_freem(m);
		return;
	}
	ba->ba_buf[idx].m = m;
	/* store Rx meta-data too */
	rxi->rxi_flags |= IEEE80211_RXI_AMPDU_DONE;
	ba->ba_buf[idx].rxi = *rxi;

	/* pass reordered MPDUs up to the next MAC process */
	while (ba->ba_buf[ba->ba_head].m != NULL) {
		ieee80211_input(ifp, ba->ba_buf[ba->ba_head].m, ni,
		    &ba->ba_buf[ba->ba_head].rxi);
		ba->ba_buf[ba->ba_head].m = NULL;

		ba->ba_head = (ba->ba_head + 1) % IEEE80211_BA_MAX_WINSZ;
		/* move window forward */
		ba->ba_winstart = (ba->ba_winstart + 1) & 0xfff;
	}
	ba->ba_winend = (ba->ba_winstart + ba->ba_winsize - 1) & 0xfff;
}

/*
 * Change the value of WinStartB (move window forward) upon reception of a
 * BlockAckReq frame or an ADDBA Request (PBAC).
 */
void
ieee80211_ba_move_window(struct ieee80211com *ic, struct ieee80211_node *ni,
    u_int8_t tid, u_int16_t ssn)
{
	struct ifnet *ifp = &ic->ic_if;
	struct ieee80211_rx_ba *ba = &ni->ni_rx_ba[tid];
	int count;

	/* assert(WinStartB <= SSN) */

	count = (ssn - ba->ba_winstart) & 0xfff;
	if (count > ba->ba_winsize)	/* no overlap */
		count = ba->ba_winsize;
	while (count-- > 0) {
		/* gaps may exist */
		if (ba->ba_buf[ba->ba_head].m != NULL) {
			ieee80211_input(ifp, ba->ba_buf[ba->ba_head].m, ni,
			    &ba->ba_buf[ba->ba_head].rxi);
			ba->ba_buf[ba->ba_head].m = NULL;
		}
		ba->ba_head = (ba->ba_head + 1) % IEEE80211_BA_MAX_WINSZ;
	}
	/* move window forward */
	ba->ba_winstart = ssn;

	/* pass reordered MPDUs up to the next MAC process */
	while (ba->ba_buf[ba->ba_head].m != NULL) {
		ieee80211_input(ifp, ba->ba_buf[ba->ba_head].m, ni,
		    &ba->ba_buf[ba->ba_head].rxi);
		ba->ba_buf[ba->ba_head].m = NULL;

		ba->ba_head = (ba->ba_head + 1) % IEEE80211_BA_MAX_WINSZ;
		/* move window forward */
		ba->ba_winstart = (ba->ba_winstart + 1) & 0xfff;
	}
	ba->ba_winend = (ba->ba_winstart + ba->ba_winsize - 1) & 0xfff;
>>>>>>> origin/master
}
#endif	/* !IEEE80211_NO_HT */

<<<<<<< HEAD
#define IEEE80211_VERIFY_SSID(_ni, _ssid, _packet_type) do {		\
	if ((_ssid)[1] != 0 &&						\
	    ((_ssid)[1] != (_ni)->ni_esslen ||				\
	    memcmp((_ssid) + 2, (_ni)->ni_essid, (_ssid)[1]) != 0)) {   \
		if (ieee80211_debug)					\
			ieee80211_ssid_mismatch(ic, _packet_type,       \
				wh->i_addr2, _ssid);			\
		ic->ic_stats.is_rx_ssidmismatch++;			\
		return;							\
	}								\
} while (0)
#else /* !IEEE80211_DEBUG */
#define IEEE80211_VERIFY_SSID(_ni, _ssid, _packet_type) do {		\
	if ((_ssid)[1] != 0 &&						\
	    ((_ssid)[1] != (_ni)->ni_esslen ||				\
	    memcmp((_ssid) + 2, (_ni)->ni_essid, (_ssid)[1]) != 0)) {   \
		ic->ic_stats.is_rx_ssidmismatch++;			\
		return;							\
	}								\
} while (0)
#endif /* !IEEE80211_DEBUG */

static void
ieee80211_auth_open(struct ieee80211com *ic, struct ieee80211_frame *wh,
    struct ieee80211_node *ni, int rssi, u_int32_t rstamp, u_int16_t seq,
    u_int16_t status)
{
	struct ifnet *ifp = &ic->ic_if;
	switch (ic->ic_opmode) {
	case IEEE80211_M_IBSS:
		if (ic->ic_state != IEEE80211_S_RUN ||
		    seq != IEEE80211_AUTH_OPEN_REQUEST) {
			IEEE80211_DPRINTF(("%s: discard auth from %s; "
			    "state %u, seq %u\n", __func__,
			    ether_sprintf(wh->i_addr2),
			    ic->ic_state, seq));
			ic->ic_stats.is_rx_bad_auth++;
			return;
		}
		ieee80211_new_state(ic, IEEE80211_S_AUTH,
		    wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK);
		break;
=======
void
ieee80211_deliver_data(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni)
{
	struct ifnet *ifp = &ic->ic_if;
	struct ether_header *eh;
	struct mbuf *m1;

	eh = mtod(m, struct ether_header *);

	if ((ic->ic_flags & IEEE80211_F_RSNON) && !ni->ni_port_valid &&
	    eh->ether_type != htons(ETHERTYPE_PAE)) {
		DPRINTF(("port not valid: %s\n",
		    ether_sprintf(eh->ether_dhost)));
		ic->ic_stats.is_rx_unauth++;
		m_freem(m);
		return;
	}
	ifp->if_ipackets++;

	/*
	 * Perform as a bridge within the AP.  Notice that we do not
	 * bridge EAPOL frames as suggested in C.1.1 of IEEE Std 802.1X.
	 */
	m1 = NULL;
#ifndef IEEE80211_STA_ONLY
	if (ic->ic_opmode == IEEE80211_M_HOSTAP &&
	    !(ic->ic_flags & IEEE80211_F_NOBRIDGE) &&
	    eh->ether_type != htons(ETHERTYPE_PAE)) {
		struct ieee80211_node *ni1;
		int error, len;

		if (ETHER_IS_MULTICAST(eh->ether_dhost)) {
			m1 = m_copym(m, 0, M_COPYALL, M_DONTWAIT);
			if (m1 == NULL)
				ifp->if_oerrors++;
			else
				m1->m_flags |= M_MCAST;
		} else {
			ni1 = ieee80211_find_node(ic, eh->ether_dhost);
			if (ni1 != NULL &&
			    ni1->ni_state == IEEE80211_STA_ASSOC) {
				m1 = m;
				m = NULL;
			}
		}
		if (m1 != NULL) {
			len = m1->m_pkthdr.len;
			IFQ_ENQUEUE(&ifp->if_snd, m1, NULL, error);
			if (error)
				ifp->if_oerrors++;
			else {
				if (m != NULL)
					ifp->if_omcasts++;
				ifp->if_obytes += len;
				if_start(ifp);
			}
		}
	}
#endif
	if (m != NULL) {
#if NBPFILTER > 0
		/*
		 * If we forward frame into transmitter of the AP,
		 * we don't need to duplicate for DLT_EN10MB.
		 */
		if (ifp->if_bpf && m1 == NULL)
			bpf_mtap(ifp->if_bpf, m, BPF_DIRECTION_IN);
#endif
		if ((ic->ic_flags & IEEE80211_F_RSNON) &&
		    eh->ether_type == htons(ETHERTYPE_PAE))
			ieee80211_eapol_key_input(ic, m, ni);
		else
			ether_input_mbuf(ifp, m);
	}
}

#ifdef __STRICT_ALIGNMENT
/*
 * Make sure protocol header (e.g. IP) is aligned on a 32-bit boundary.
 * This is achieved by copying mbufs so drivers should try to map their
 * buffers such that this copying is not necessary.  It is however not
 * always possible because 802.11 header length may vary (non-QoS+LLC
 * is 32 bytes while QoS+LLC is 34 bytes).  Some devices are smart and
 * add 2 padding bytes after the 802.11 header in the QoS case so this
 * function is there for stupid drivers/devices only.
 */
struct mbuf *
ieee80211_align_mbuf(struct mbuf *m)
{
	struct mbuf *n, *n0, **np;
	caddr_t newdata;
	int off, pktlen;

	n0 = NULL;
	np = &n0;
	off = 0;
	pktlen = m->m_pkthdr.len;
	while (pktlen > off) {
		if (n0 == NULL) {
			MGETHDR(n, M_DONTWAIT, MT_DATA);
			if (n == NULL) {
				m_freem(m);
				return NULL;
			}
			if (m_dup_pkthdr(n, m)) {
				m_free(n);
				m_freem(m);
				return (NULL);
			}
			n->m_len = MHLEN;
		} else {
			MGET(n, M_DONTWAIT, MT_DATA);
			if (n == NULL) {
				m_freem(m);
				m_freem(n0);
				return NULL;
			}
			n->m_len = MLEN;
		}
		if (pktlen - off >= MINCLSIZE) {
			MCLGET(n, M_DONTWAIT);
			if (n->m_flags & M_EXT)
				n->m_len = n->m_ext.ext_size;
		}
		if (n0 == NULL) {
			newdata = (caddr_t)ALIGN(n->m_data + ETHER_HDR_LEN) -
			    ETHER_HDR_LEN;
			n->m_len -= newdata - n->m_data;
			n->m_data = newdata;
		}
		if (n->m_len > pktlen - off)
			n->m_len = pktlen - off;
		m_copydata(m, off, n->m_len, mtod(n, caddr_t));
		off += n->m_len;
		*np = n;
		np = &n->m_next;
	}
	m_freem(m);
	return n0;
}
#endif	/* __STRICT_ALIGNMENT */

void
ieee80211_decap(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni, int hdrlen)
{
	struct ether_header eh;
	struct ieee80211_frame *wh;
	struct llc *llc;

	if (m->m_len < hdrlen + LLC_SNAPFRAMELEN &&
	    (m = m_pullup(m, hdrlen + LLC_SNAPFRAMELEN)) == NULL) {
		ic->ic_stats.is_rx_decap++;
		return;
	}
	wh = mtod(m, struct ieee80211_frame *);
	switch (wh->i_fc[1] & IEEE80211_FC1_DIR_MASK) {
	case IEEE80211_FC1_DIR_NODS:
		IEEE80211_ADDR_COPY(eh.ether_dhost, wh->i_addr1);
		IEEE80211_ADDR_COPY(eh.ether_shost, wh->i_addr2);
		break;
	case IEEE80211_FC1_DIR_TODS:
		IEEE80211_ADDR_COPY(eh.ether_dhost, wh->i_addr3);
		IEEE80211_ADDR_COPY(eh.ether_shost, wh->i_addr2);
		break;
	case IEEE80211_FC1_DIR_FROMDS:
		IEEE80211_ADDR_COPY(eh.ether_dhost, wh->i_addr1);
		IEEE80211_ADDR_COPY(eh.ether_shost, wh->i_addr3);
		break;
	case IEEE80211_FC1_DIR_DSTODS:
		IEEE80211_ADDR_COPY(eh.ether_dhost, wh->i_addr3);
		IEEE80211_ADDR_COPY(eh.ether_shost,
		    ((struct ieee80211_frame_addr4 *)wh)->i_addr4);
		break;
	}
	llc = (struct llc *)((caddr_t)wh + hdrlen);
	if (llc->llc_dsap == LLC_SNAP_LSAP &&
	    llc->llc_ssap == LLC_SNAP_LSAP &&
	    llc->llc_control == LLC_UI &&
	    llc->llc_snap.org_code[0] == 0 &&
	    llc->llc_snap.org_code[1] == 0 &&
	    llc->llc_snap.org_code[2] == 0) {
		eh.ether_type = llc->llc_snap.ether_type;
		m_adj(m, hdrlen + LLC_SNAPFRAMELEN - ETHER_HDR_LEN);
	} else {
		eh.ether_type = htons(m->m_pkthdr.len - hdrlen);
		m_adj(m, hdrlen - ETHER_HDR_LEN);
	}
	memcpy(mtod(m, caddr_t), &eh, ETHER_HDR_LEN);
#ifdef __STRICT_ALIGNMENT
	if (!ALIGNED_POINTER(mtod(m, caddr_t) + ETHER_HDR_LEN, u_int32_t)) {
		if ((m = ieee80211_align_mbuf(m)) == NULL) {
			ic->ic_stats.is_rx_decap++;
			return;
		}
	}
#endif
	ieee80211_deliver_data(ic, m, ni);
}

#ifndef IEEE80211_NO_HT
/*
 * Decapsulate an Aggregate MSDU (see 7.2.2.2).
 */
void
ieee80211_amsdu_decap(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni, int hdrlen)
{
	struct mbuf *n;
	struct ether_header *eh;
	struct llc *llc;
	int len, pad;

	/* strip 802.11 header */
	m_adj(m, hdrlen);

	for (;;) {
		/* process an A-MSDU subframe */
		if (m->m_len < ETHER_HDR_LEN + LLC_SNAPFRAMELEN) {
			m = m_pullup(m, ETHER_HDR_LEN + LLC_SNAPFRAMELEN);
			if (m == NULL) {
				ic->ic_stats.is_rx_decap++;
				break;
			}
		}
		eh = mtod(m, struct ether_header *);
		/* examine 802.3 header */
		len = ntohs(eh->ether_type);
		if (len < LLC_SNAPFRAMELEN) {
			DPRINTF(("A-MSDU subframe too short (%d)\n", len));
			/* stop processing A-MSDU subframes */
			ic->ic_stats.is_rx_decap++;
			m_freem(m);
			break;
		}
		llc = (struct llc *)&eh[1];
		/* examine 802.2 LLC header */
		if (llc->llc_dsap == LLC_SNAP_LSAP &&
		    llc->llc_ssap == LLC_SNAP_LSAP &&
		    llc->llc_control == LLC_UI &&
		    llc->llc_snap.org_code[0] == 0 &&
		    llc->llc_snap.org_code[1] == 0 &&
		    llc->llc_snap.org_code[2] == 0) {
			/* convert to Ethernet II header */
			eh->ether_type = llc->llc_snap.ether_type;
			/* strip LLC+SNAP headers */
			ovbcopy(eh, (u_int8_t *)eh + LLC_SNAPFRAMELEN,
			    ETHER_HDR_LEN);
			m_adj(m, LLC_SNAPFRAMELEN);
			len -= LLC_SNAPFRAMELEN;
		}
		len += ETHER_HDR_LEN;

		/* "detach" our A-MSDU subframe from the others */
		n = m_split(m, len, M_NOWAIT);
		if (n == NULL) {
			/* stop processing A-MSDU subframes */
			ic->ic_stats.is_rx_decap++;
			m_freem(m);
			break;
		}
		ieee80211_deliver_data(ic, m, ni);

		m = n;
		/* remove padding */
		pad = ((len + 3) & ~3) - len;
		m_adj(m, pad);
	}
}
#endif	/* !IEEE80211_NO_HT */

/*
 * Parse an EDCA Parameter Set element (see 7.3.2.27).
 */
int
ieee80211_parse_edca_params_body(struct ieee80211com *ic, const u_int8_t *frm)
{
	u_int updtcount;
	int aci;

	/*
	 * Check if EDCA parameters have changed XXX if we miss more than
	 * 15 consecutive beacons, we might not detect changes to EDCA
	 * parameters due to wraparound of the 4-bit Update Count field.
	 */
	updtcount = frm[0] & 0xf;
	if (updtcount == ic->ic_edca_updtcount)
		return 0;	/* no changes to EDCA parameters, ignore */
	ic->ic_edca_updtcount = updtcount;

	frm += 2;	/* skip QoS Info & Reserved fields */

	/* parse AC Parameter Records */
	for (aci = 0; aci < EDCA_NUM_AC; aci++) {
		struct ieee80211_edca_ac_params *ac = &ic->ic_edca_ac[aci];

		ac->ac_acm       = (frm[0] >> 4) & 0x1;
		ac->ac_aifsn     = frm[0] & 0xf;
		ac->ac_ecwmin    = frm[1] & 0xf;
		ac->ac_ecwmax    = frm[1] >> 4;
		ac->ac_txoplimit = LE_READ_2(frm + 2);
		frm += 4;
	}
	/* give drivers a chance to update their settings */
	if ((ic->ic_flags & IEEE80211_F_QOS) && ic->ic_updateedca != NULL)
		(*ic->ic_updateedca)(ic);

	return 0;
}

int
ieee80211_parse_edca_params(struct ieee80211com *ic, const u_int8_t *frm)
{
	if (frm[1] < 18) {
		ic->ic_stats.is_rx_elem_toosmall++;
		return IEEE80211_REASON_IE_INVALID;
	}
	return ieee80211_parse_edca_params_body(ic, frm + 2);
}

int
ieee80211_parse_wmm_params(struct ieee80211com *ic, const u_int8_t *frm)
{
	if (frm[1] < 24) {
		ic->ic_stats.is_rx_elem_toosmall++;
		return IEEE80211_REASON_IE_INVALID;
	}
	return ieee80211_parse_edca_params_body(ic, frm + 8);
}

enum ieee80211_cipher
ieee80211_parse_rsn_cipher(const u_int8_t selector[4])
{
	if (memcmp(selector, MICROSOFT_OUI, 3) == 0) {	/* WPA */
		switch (selector[3]) {
		case 0:	/* use group data cipher suite */
			return IEEE80211_CIPHER_USEGROUP;
		case 1:	/* WEP-40 */
			return IEEE80211_CIPHER_WEP40;
		case 2:	/* TKIP */
			return IEEE80211_CIPHER_TKIP;
		case 4:	/* CCMP (RSNA default) */
			return IEEE80211_CIPHER_CCMP;
		case 5:	/* WEP-104 */
			return IEEE80211_CIPHER_WEP104;
		}
	} else if (memcmp(selector, IEEE80211_OUI, 3) == 0) {	/* RSN */
		/* from IEEE Std 802.11 - Table 20da */
		switch (selector[3]) {
		case 0:	/* use group data cipher suite */
			return IEEE80211_CIPHER_USEGROUP;
		case 1:	/* WEP-40 */
			return IEEE80211_CIPHER_WEP40;
		case 2:	/* TKIP */
			return IEEE80211_CIPHER_TKIP;
		case 4:	/* CCMP (RSNA default) */
			return IEEE80211_CIPHER_CCMP;
		case 5:	/* WEP-104 */
			return IEEE80211_CIPHER_WEP104;
		case 6:	/* BIP */
			return IEEE80211_CIPHER_BIP;
		}
	}
	return IEEE80211_CIPHER_NONE;	/* ignore unknown ciphers */
}

enum ieee80211_akm
ieee80211_parse_rsn_akm(const u_int8_t selector[4])
{
	if (memcmp(selector, MICROSOFT_OUI, 3) == 0) {	/* WPA */
		switch (selector[3]) {
		case 1:	/* IEEE 802.1X (RSNA default) */
			return IEEE80211_AKM_8021X;
		case 2:	/* PSK */
			return IEEE80211_AKM_PSK;
		}
	} else if (memcmp(selector, IEEE80211_OUI, 3) == 0) {	/* RSN */
		/* from IEEE Std 802.11i-2004 - Table 20dc */
		switch (selector[3]) {
		case 1:	/* IEEE 802.1X (RSNA default) */
			return IEEE80211_AKM_8021X;
		case 2:	/* PSK */
			return IEEE80211_AKM_PSK;
		case 5:	/* IEEE 802.1X with SHA256 KDF */
			return IEEE80211_AKM_SHA256_8021X;
		case 6:	/* PSK with SHA256 KDF */
			return IEEE80211_AKM_SHA256_PSK;
		}
	}
	return IEEE80211_AKM_NONE;	/* ignore unknown AKMs */
}

/*
 * Parse an RSN element (see 7.3.2.25).
 */
int
ieee80211_parse_rsn_body(struct ieee80211com *ic, const u_int8_t *frm,
    u_int len, struct ieee80211_rsnparams *rsn)
{
	const u_int8_t *efrm;
	u_int16_t m, n, s;

	efrm = frm + len;

	/* check Version field */
	if (LE_READ_2(frm) != 1)
		return IEEE80211_STATUS_RSN_IE_VER_UNSUP;
	frm += 2;

	/* all fields after the Version field are optional */

	/* if Cipher Suite missing, default to CCMP */
	rsn->rsn_groupcipher = IEEE80211_CIPHER_CCMP;
	rsn->rsn_nciphers = 1;
	rsn->rsn_ciphers = IEEE80211_CIPHER_CCMP;
	/* if Group Management Cipher Suite missing, defaut to BIP */
	rsn->rsn_groupmgmtcipher = IEEE80211_CIPHER_BIP;
	/* if AKM Suite missing, default to 802.1X */
	rsn->rsn_nakms = 1;
	rsn->rsn_akms = IEEE80211_AKM_8021X;
	/* if RSN capabilities missing, default to 0 */
	rsn->rsn_caps = 0;
	rsn->rsn_npmkids = 0;

	/* read Group Data Cipher Suite field */
	if (frm + 4 > efrm)
		return 0;
	rsn->rsn_groupcipher = ieee80211_parse_rsn_cipher(frm);
	if (rsn->rsn_groupcipher == IEEE80211_CIPHER_USEGROUP)
		return IEEE80211_STATUS_BAD_GROUP_CIPHER;
	frm += 4;

	/* read Pairwise Cipher Suite Count field */
	if (frm + 2 > efrm)
		return 0;
	m = rsn->rsn_nciphers = LE_READ_2(frm);
	frm += 2;

	/* read Pairwise Cipher Suite List */
	if (frm + m * 4 > efrm)
		return IEEE80211_STATUS_IE_INVALID;
	rsn->rsn_ciphers = IEEE80211_CIPHER_NONE;
	while (m-- > 0) {
		rsn->rsn_ciphers |= ieee80211_parse_rsn_cipher(frm);
		frm += 4;
	}
	if (rsn->rsn_ciphers & IEEE80211_CIPHER_USEGROUP) {
		if (rsn->rsn_ciphers != IEEE80211_CIPHER_USEGROUP)
			return IEEE80211_STATUS_BAD_PAIRWISE_CIPHER;
		if (rsn->rsn_groupcipher == IEEE80211_CIPHER_CCMP)
			return IEEE80211_STATUS_BAD_PAIRWISE_CIPHER;
	}
>>>>>>> origin/master

	case IEEE80211_M_AHDEMO:
		/* should not come here */
		break;

	case IEEE80211_M_HOSTAP:
		if (ic->ic_state != IEEE80211_S_RUN ||
		    seq != IEEE80211_AUTH_OPEN_REQUEST) {
			IEEE80211_DPRINTF(("%s: discard auth from %s; "
			    "state %u, seq %u\n", __func__,
			    ether_sprintf(wh->i_addr2),
			    ic->ic_state, seq));
			ic->ic_stats.is_rx_bad_auth++;
			return;
		}
		if (ni == ic->ic_bss) {
			ni = ieee80211_alloc_node(ic, wh->i_addr2);
			if (ni == NULL) {
				ic->ic_stats.is_rx_nodealloc++;
				return;
			}
			IEEE80211_ADDR_COPY(ni->ni_bssid, ic->ic_bss->ni_bssid);
			ni->ni_rssi = rssi;
			ni->ni_rstamp = rstamp;
			ni->ni_chan = ic->ic_bss->ni_chan;
		}
		IEEE80211_SEND_MGMT(ic, ni,
			IEEE80211_FC0_SUBTYPE_AUTH, seq + 1);
		if (ifp->if_flags & IFF_DEBUG)
			printf("%s: station %s %s authenticated (open)\n",
			    ifp->if_xname, ether_sprintf(ni->ni_macaddr),
			    ni->ni_state != IEEE80211_STA_CACHE ?
			    "newly" : "already");
		ieee80211_node_newstate(ni, IEEE80211_STA_AUTH);
		break;

<<<<<<< HEAD
	case IEEE80211_M_STA:
		if (ic->ic_state != IEEE80211_S_AUTH ||
		    seq != IEEE80211_AUTH_OPEN_RESPONSE) {
			ic->ic_stats.is_rx_bad_auth++;
			IEEE80211_DPRINTF(("%s: discard auth from %s; "
			    "state %u, seq %u\n", __func__,
			    ether_sprintf(wh->i_addr2),
			    ic->ic_state, seq));
			return;
		}
		if (status != 0) {
			if (ifp->if_flags & IFF_DEBUG)
				printf("%s: open authentication failed "
				    "(reason %d) for %s\n", ifp->if_xname,
				    status, ether_sprintf(wh->i_addr3));
			if (ni != ic->ic_bss)
				ni->ni_fails++;
			ic->ic_stats.is_rx_auth_fail++;
			return;
		}
		ieee80211_new_state(ic, IEEE80211_S_ASSOC,
		    wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK);
		break;
	case IEEE80211_M_MONITOR:
		break;
	}
=======
	/* read PMKID Count field */
	if (frm + 2 > efrm)
		return 0;
	s = rsn->rsn_npmkids = LE_READ_2(frm);
	frm += 2;

	/* read PMKID List */
	if (frm + s * IEEE80211_PMKID_LEN > efrm)
		return IEEE80211_STATUS_IE_INVALID;
	if (s != 0) {
		rsn->rsn_pmkids = frm;
		frm += s * IEEE80211_PMKID_LEN;
	}

	/* read Group Management Cipher Suite field */
	if (frm + 4 > efrm)
		return 0;
	rsn->rsn_groupmgmtcipher = ieee80211_parse_rsn_cipher(frm);

	return IEEE80211_STATUS_SUCCESS;
>>>>>>> origin/master
}

#if 0
/* TBD send appropriate responses on error? */
static void
ieee80211_auth_shared(struct ieee80211com *ic, struct ieee80211_frame *wh,
    u_int8_t *frm, u_int8_t *efrm, struct ieee80211_node *ni, int rssi,
    u_int32_t rstamp, u_int16_t seq, u_int16_t status)
{
<<<<<<< HEAD
	struct ifnet *ifp = &ic->ic_if;
	u_int8_t *challenge = NULL;
	int i;

	if ((ic->ic_flags & IEEE80211_F_WEPON) == 0) {
		IEEE80211_DPRINTF(("%s: WEP is off\n", __func__));
		return;
=======
	if (frm[1] < 2) {
		ic->ic_stats.is_rx_elem_toosmall++;
		return IEEE80211_STATUS_IE_INVALID;
>>>>>>> origin/master
	}

<<<<<<< HEAD
	if (frm + 1 < efrm) {
		if (frm[1] + 2 > efrm - frm) {
			IEEE80211_DPRINTF(("elt %d %d bytes too long\n",
			    frm[0], (frm[1] + 2) - (int)(efrm - frm)));
			ic->ic_stats.is_rx_bad_auth++;
			return;
		}
		if (*frm == IEEE80211_ELEMID_CHALLENGE)
			challenge = frm;
		frm += frm[1] + 2;
	}
	switch (seq) {
	case IEEE80211_AUTH_SHARED_CHALLENGE:
	case IEEE80211_AUTH_SHARED_RESPONSE:
		if (challenge == NULL) {
			IEEE80211_DPRINTF(("%s: no challenge sent\n",
			    __func__));
			ic->ic_stats.is_rx_bad_auth++;
			return;
		}
		if (challenge[1] != IEEE80211_CHALLENGE_LEN) {
			IEEE80211_DPRINTF(("%s: bad challenge len %d\n",
			    __func__, challenge[1]));
			ic->ic_stats.is_rx_bad_auth++;
			return;
		}
	default:
		break;
=======
int
ieee80211_parse_wpa(struct ieee80211com *ic, const u_int8_t *frm,
    struct ieee80211_rsnparams *rsn)
{
	if (frm[1] < 6) {
		ic->ic_stats.is_rx_elem_toosmall++;
		return IEEE80211_STATUS_IE_INVALID;
>>>>>>> origin/master
	}
	switch (ic->ic_opmode) {
	case IEEE80211_M_MONITOR:
	case IEEE80211_M_AHDEMO:
	case IEEE80211_M_IBSS:
		IEEE80211_DPRINTF(("%s: unexpected operating mode\n",
		    __func__));
		return;
	case IEEE80211_M_HOSTAP:
		if (ic->ic_state != IEEE80211_S_RUN) {
			IEEE80211_DPRINTF(("%s: not running\n", __func__));
			return;
		}
		switch (seq) {
		case IEEE80211_AUTH_SHARED_REQUEST:
			if (ni == ic->ic_bss) {
				ni = ieee80211_alloc_node(ic, wh->i_addr2);
				if (ni == NULL) {
					ic->ic_stats.is_rx_nodealloc++;
					return;
				}
				IEEE80211_ADDR_COPY(ni->ni_bssid,
				    ic->ic_bss->ni_bssid);
				ni->ni_rssi = rssi;
				ni->ni_rstamp = rstamp;
				ni->ni_chan = ic->ic_bss->ni_chan;
			}
			if (ni->ni_challenge == NULL)
				ni->ni_challenge = (u_int32_t*)malloc(
				    IEEE80211_CHALLENGE_LEN, M_DEVBUF,
				    M_NOWAIT);
			if (ni->ni_challenge == NULL) {
				IEEE80211_DPRINTF(("%s: "
				    "challenge alloc failed\n", __func__));
				/* XXX statistic */
				return;
			}
			for (i = IEEE80211_CHALLENGE_LEN / sizeof(u_int32_t);
			     --i >= 0; )
				ni->ni_challenge[i] = arc4random();
			if (ifp->if_flags & IFF_DEBUG)
				printf("%s: station %s shared key "
				    "%sauthentication\n", ifp->if_xname,
				    ether_sprintf(ni->ni_macaddr),
				    ni->ni_state != IEEE80211_STA_CACHE ?
				    "" : "re");
			break;
		case IEEE80211_AUTH_SHARED_RESPONSE:
			if (ni == ic->ic_bss) {
				IEEE80211_DPRINTF(("%s: unknown STA\n",
				    __func__));
				return;
			}
			if (ni->ni_challenge == NULL) {
				IEEE80211_DPRINTF((
				    "%s: no challenge recorded\n", __func__));
				ic->ic_stats.is_rx_bad_auth++;
				return;
			}
			if (memcmp(ni->ni_challenge, &challenge[2],
			    challenge[1]) != 0) {
				IEEE80211_DPRINTF(("%s: challenge mismatch\n",
				    __func__));
				ic->ic_stats.is_rx_auth_fail++;
				return;
			}
			if (ifp->if_flags & IFF_DEBUG)
				printf("%s: station %s authenticated "
					"(shared key)\n", ifp->if_xname,
					ether_sprintf(ni->ni_macaddr));
			ieee80211_node_newstate(ni, IEEE80211_STA_AUTH);
			break;
		default:
			IEEE80211_DPRINTF(("%s: bad seq %d from %s\n",
			    __func__, seq, ether_sprintf(wh->i_addr2)));
			ic->ic_stats.is_rx_bad_auth++;
			return;
		}
		IEEE80211_SEND_MGMT(ic, ni,
			IEEE80211_FC0_SUBTYPE_AUTH, seq + 1);
		break;

	case IEEE80211_M_STA:
		if (ic->ic_state != IEEE80211_S_AUTH)
			return;
		switch (seq) {
		case IEEE80211_AUTH_SHARED_PASS:
			if (ni->ni_challenge != NULL) {
				FREE(ni->ni_challenge, M_DEVBUF);
				ni->ni_challenge = NULL;
			}
			if (status != 0) {
				printf("%s: %s: shared authentication failed "
				    "(reason %d) for %s\n", ifp->if_xname,
				    __func__, status,
				    ether_sprintf(wh->i_addr3));
				if (ni != ic->ic_bss)
					ni->ni_fails++;
				ic->ic_stats.is_rx_auth_fail++;
				return;
			}
			ieee80211_new_state(ic, IEEE80211_S_ASSOC,
			    wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK);
			break;
		case IEEE80211_AUTH_SHARED_CHALLENGE:
			if (ni->ni_challenge == NULL)
				ni->ni_challenge = (u_int32_t*)malloc(
				    challenge[1], M_DEVBUF, M_NOWAIT);
			if (ni->ni_challenge == NULL) {
				IEEE80211_DPRINTF((
				    "%s: challenge alloc failed\n", __func__));
				/* XXX statistic */
				return;
			}
			memcpy(ni->ni_challenge, &challenge[2], challenge[1]);
			IEEE80211_SEND_MGMT(ic, ni,
				IEEE80211_FC0_SUBTYPE_AUTH, seq + 1);
			break;
		default:
			IEEE80211_DPRINTF(("%s: bad seq %d from %s\n",
			    __func__, seq, ether_sprintf(wh->i_addr2)));
			ic->ic_stats.is_rx_bad_auth++;
			return;
		}
		break;
	}
}
#endif

<<<<<<< HEAD
void
ieee80211_recv_mgmt(struct ieee80211com *ic, struct mbuf *m0,
	struct ieee80211_node *ni,
	int subtype, int rssi, u_int32_t rstamp)
{
#define	ISPROBE(_st)	((_st) == IEEE80211_FC0_SUBTYPE_PROBE_RESP)
#define	ISREASSOC(_st)	((_st) == IEEE80211_FC0_SUBTYPE_REASSOC_RESP)
	struct ifnet *ifp = &ic->ic_if;
	struct ieee80211_frame *wh;
	u_int8_t *frm, *efrm;
	u_int8_t *ssid, *rates, *xrates;
	int is_new, reassoc, resp;

	wh = mtod(m0, struct ieee80211_frame *);
	frm = (u_int8_t *)&wh[1];
	efrm = mtod(m0, u_int8_t *) + m0->m_len;
	switch (subtype) {
	case IEEE80211_FC0_SUBTYPE_PROBE_RESP:
	case IEEE80211_FC0_SUBTYPE_BEACON: {
		u_int8_t *tstamp, *bintval, *capinfo, *country;
		u_int8_t chan, bchan, fhindex, erp;
		u_int16_t fhdwell;

		/*
		 * We process beacon/probe response frames for:
		 *    o station mode: to collect state
		 *      updates such as 802.11g slot time and for passive
		 *      scanning of APs
		 *    o adhoc mode: to discover neighbors
		 *    o hostap mode: for passive scanning of neighbor APs
		 *    o when scanning
		 * In other words, in all modes other than monitor (which
		 * does not process incoming packets) and adhoc-demo (which
		 * does not use management frames at all).
		 */
#ifdef DIAGNOSTIC
		if (ic->ic_opmode != IEEE80211_M_STA &&
		    ic->ic_opmode != IEEE80211_M_IBSS &&
		    ic->ic_opmode != IEEE80211_M_HOSTAP &&
		    ic->ic_state != IEEE80211_S_SCAN) {
			panic("%s: impossible operating mode", __func__);
		}
#endif

		/*
		 * beacon/probe response frame format
		 *	[8] time stamp
		 *	[2] beacon interval
		 *	[2] capability information
		 *	[tlv] ssid
		 *	[tlv] supported rates
		 *	[tlv] country information
		 *	[tlv] parameter set (FH/DS)
		 *	[tlv] erp information
		 *	[tlv] extended supported rates
		 */
		IEEE80211_VERIFY_LENGTH(efrm - frm, 12);
		tstamp  = frm;	frm += 8;
		bintval = frm;	frm += 2;
		capinfo = frm;	frm += 2;
		ssid = rates = xrates = country = NULL;
		bchan = ieee80211_chan2ieee(ic, ic->ic_bss->ni_chan);
		chan = bchan;
		fhdwell = 0;
		fhindex = 0;
		erp = 0;
		while (frm < efrm) {
			switch (*frm) {
			case IEEE80211_ELEMID_SSID:
				ssid = frm;
				break;
			case IEEE80211_ELEMID_RATES:
				rates = frm;
				break;
			case IEEE80211_ELEMID_COUNTRY:
				country = frm;
				break;
			case IEEE80211_ELEMID_FHPARMS:
				if (ic->ic_phytype == IEEE80211_T_FH) {
					fhdwell = (frm[3] << 8) | frm[2];
					chan = IEEE80211_FH_CHAN(frm[4],
					    frm[5]);
					fhindex = frm[6];
				}
=======
/*-
 * Beacon/Probe response frame format:
 * [8]   Timestamp
 * [2]   Beacon interval
 * [2]   Capability
 * [tlv] Service Set Identifier (SSID)
 * [tlv] Supported rates
 * [tlv] DS Parameter Set (802.11g)
 * [tlv] ERP Information (802.11g)
 * [tlv] Extended Supported Rates (802.11g)
 * [tlv] RSN (802.11i)
 * [tlv] EDCA Parameter Set (802.11e)
 * [tlv] QoS Capability (Beacon only, 802.11e)
 * [tlv] HT Capabilities (802.11n)
 * [tlv] HT Operation (802.11n)
 */
void
ieee80211_recv_probe_resp(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni, struct ieee80211_rxinfo *rxi, int isprobe)
{
	const struct ieee80211_frame *wh;
	const u_int8_t *frm, *efrm;
	const u_int8_t *tstamp, *ssid, *rates, *xrates, *edcaie, *wmmie;
	const u_int8_t *rsnie, *wpaie, *htcaps, *htop;
	u_int16_t capinfo, bintval;
	u_int8_t chan, bchan, erp;
	int is_new;

	/*
	 * We process beacon/probe response frames for:
	 *    o station mode: to collect state
	 *      updates such as 802.11g slot time and for passive
	 *      scanning of APs
	 *    o adhoc mode: to discover neighbors
	 *    o hostap mode: for passive scanning of neighbor APs
	 *    o when scanning
	 * In other words, in all modes other than monitor (which
	 * does not process incoming frames) and adhoc-demo (which
	 * does not use management frames at all).
	 */
#ifdef DIAGNOSTIC
	if (ic->ic_opmode != IEEE80211_M_STA &&
#ifndef IEEE80211_STA_ONLY
	    ic->ic_opmode != IEEE80211_M_IBSS &&
	    ic->ic_opmode != IEEE80211_M_HOSTAP &&
#endif
	    ic->ic_state != IEEE80211_S_SCAN) {
		panic("%s: impossible operating mode", __func__);
	}
#endif
	/* make sure all mandatory fixed fields are present */
	if (m->m_len < sizeof(*wh) + 12) {
		DPRINTF(("frame too short\n"));
		return;
	}
	wh = mtod(m, struct ieee80211_frame *);
	frm = (const u_int8_t *)&wh[1];
	efrm = mtod(m, u_int8_t *) + m->m_len;

	tstamp  = frm; frm += 8;
	bintval = LE_READ_2(frm); frm += 2;
	capinfo = LE_READ_2(frm); frm += 2;

	ssid = rates = xrates = edcaie = wmmie = rsnie = wpaie = NULL;
	htcaps = htop = NULL;
	bchan = ieee80211_chan2ieee(ic, ic->ic_bss->ni_chan);
	chan = bchan;
	erp = 0;
	while (frm + 2 <= efrm) {
		if (frm + 2 + frm[1] > efrm) {
			ic->ic_stats.is_rx_elem_toosmall++;
			break;
		}
		switch (frm[0]) {
		case IEEE80211_ELEMID_SSID:
			ssid = frm;
			break;
		case IEEE80211_ELEMID_RATES:
			rates = frm;
			break;
		case IEEE80211_ELEMID_DSPARMS:
			if (frm[1] < 1) {
				ic->ic_stats.is_rx_elem_toosmall++;
>>>>>>> origin/master
				break;
			case IEEE80211_ELEMID_DSPARMS:
				/*
				 * XXX hack this since depending on phytype
				 * is problematic for multi-mode devices.
				 */
				if (ic->ic_phytype != IEEE80211_T_FH)
					chan = frm[2];
				break;
<<<<<<< HEAD
			case IEEE80211_ELEMID_TIM:
				break;
			case IEEE80211_ELEMID_IBSSPARMS:
				break;
			case IEEE80211_ELEMID_XRATES:
				xrates = frm;
				break;
			case IEEE80211_ELEMID_ERP:
				if (frm[1] != 1) {
					IEEE80211_DPRINTF(("%s: invalid ERP "
					    "element; length %u, expecting "
					    "1\n", __func__, frm[1]));
					ic->ic_stats.is_rx_elem_toobig++;
					break;
				}
				erp = frm[2];
				break;
			default:
				IEEE80211_DPRINTF2(("%s: element id %u/len %u "
				    "ignored\n", __func__, *frm, frm[1]));
				ic->ic_stats.is_rx_elem_unknown++;
				break;
			}
			frm += frm[1] + 2;
		}
		IEEE80211_VERIFY_ELEMENT(rates, IEEE80211_RATE_MAXSIZE);
		IEEE80211_VERIFY_ELEMENT(ssid, IEEE80211_NWID_LEN);
		if (
=======
			}
			erp = frm[2];
			break;
		case IEEE80211_ELEMID_RSN:
			rsnie = frm;
			break;
		case IEEE80211_ELEMID_EDCAPARMS:
			edcaie = frm;
			break;
		case IEEE80211_ELEMID_QOS_CAP:
			break;
#ifndef IEEE80211_NO_HT
		case IEEE80211_ELEMID_HTCAPS:
			htcaps = frm;
			break;
		case IEEE80211_ELEMID_HTOP:
			htop = frm;
			break;
#endif
		case IEEE80211_ELEMID_VENDOR:
			if (frm[1] < 4) {
				ic->ic_stats.is_rx_elem_toosmall++;
				break;
			}
			if (memcmp(frm + 2, MICROSOFT_OUI, 3) == 0) {
				if (frm[5] == 1)
					wpaie = frm;
				else if (frm[1] >= 5 &&
				    frm[5] == 2 && frm[6] == 1)
					wmmie = frm;
			}
			break;
		default:
			DPRINTF(("element id %u/len %u ignored\n",
			    frm[0], frm[1]));
			ic->ic_stats.is_rx_elem_unknown++;
			break;
		}
		frm += 2 + frm[1];
	}
	/* supported rates element is mandatory */
	if (rates == NULL || rates[1] > IEEE80211_RATE_MAXSIZE) {
		DPRINTF(("invalid supported rates element\n"));
		return;
	}
	/* SSID element is mandatory */
	if (ssid == NULL || ssid[1] > IEEE80211_NWID_LEN) {
		DPRINTF(("invalid SSID element\n"));
		return;
	}

	if (
>>>>>>> origin/master
#if IEEE80211_CHAN_MAX < 255
		    chan > IEEE80211_CHAN_MAX ||
#endif
<<<<<<< HEAD
		    isclr(ic->ic_chan_active, chan)) {
			IEEE80211_DPRINTF(("%s: ignore %s with invalid channel "
			    "%u\n", __func__, ISPROBE(subtype) ?
			    "probe response" : "beacon", chan));
			ic->ic_stats.is_rx_badchan++;
			return;
		}
		if (!(ic->ic_caps & IEEE80211_C_SCANALL) &&
		    (chan != bchan && ic->ic_phytype != IEEE80211_T_FH)) {
			/*
			 * Frame was received on a channel different from the
			 * one indicated in the DS params element id;
			 * silently discard it.
			 *
			 * NB: this can happen due to signal leakage.
			 *     But we should take it for FH phy because
			 *     the rssi value should be correct even for
			 *     different hop pattern in FH.
			 */
			IEEE80211_DPRINTF(("%s: ignore %s on channel %u marked "
			    "for channel %u\n", __func__, ISPROBE(subtype) ?
			    "probe response" : "beacon", bchan, chan));
			ic->ic_stats.is_rx_chanmismatch++;
			return;
		}
		/*
		 * Use mac, channel and rssi so we collect only the
		 * best potential AP with the equal bssid while scanning.
		 * Collecting all potential APs may result in bloat of
		 * the node tree. This call will return NULL if the node
		 * for this APs does not exist or if the new node is the
		 * potential better one.
		 */
		if ((ni = ieee80211_find_node_for_beacon(ic, wh->i_addr2,
		    &ic->ic_channels[chan], ssid, rssi)) != NULL)
			break;
=======
	    isclr(ic->ic_chan_active, chan)) {
		DPRINTF(("ignore %s with invalid channel %u\n",
		    isprobe ? "probe response" : "beacon", chan));
		ic->ic_stats.is_rx_badchan++;
		return;
	}
	if ((ic->ic_state != IEEE80211_S_SCAN ||
	     !(ic->ic_caps & IEEE80211_C_SCANALL)) &&
	    chan != bchan) {
		/*
		 * Frame was received on a channel different from the
		 * one indicated in the DS params element id;
		 * silently discard it.
		 *
		 * NB: this can happen due to signal leakage.
		 */
		DPRINTF(("ignore %s on channel %u marked for channel %u\n",
		    isprobe ? "probe response" : "beacon", bchan, chan));
		ic->ic_stats.is_rx_chanmismatch++;
		return;
	}
	/*
	 * Use mac, channel and rssi so we collect only the
	 * best potential AP with the equal bssid while scanning.
	 * Collecting all potential APs may result in bloat of
	 * the node tree. This call will return NULL if the node
	 * for this APs does not exist or if the new node is the
	 * potential better one.
	 */
	if ((ni = ieee80211_find_node_for_beacon(ic, wh->i_addr2,
	    &ic->ic_channels[chan], ssid, rxi->rxi_rssi)) != NULL)
		return;
>>>>>>> origin/master

#ifdef IEEE80211_DEBUG
		if (ieee80211_debug &&
		    (ni == NULL || ic->ic_state == IEEE80211_S_SCAN)) {
			printf("%s: %s%s on chan %u (bss chan %u) ",
			    __func__, (ni == NULL ? "new " : ""),
			    ISPROBE(subtype) ? "probe response" : "beacon",
			    chan, bchan);
			ieee80211_print_essid(ssid + 2, ssid[1]);
			printf(" from %s\n", ether_sprintf(wh->i_addr2));
			printf("%s: caps 0x%x bintval %u erp 0x%x\n",
				__func__, letoh16(*(u_int16_t *)capinfo),
				letoh16(*(u_int16_t *)bintval), erp);
			if (country) {
				int i;
				printf("%s: country info", __func__);
				for (i = 0; i < country[1]; i++)
					printf(" %02x", country[i+2]);
				printf("\n");
			}
		}
#endif

		if ((ni = ieee80211_find_node(ic, wh->i_addr2)) == NULL) {
			ni = ieee80211_alloc_node(ic, wh->i_addr2);
			if (ni == NULL)
				return;
			is_new = 1;
		} else
			is_new = 0;

		/*
		 * When operating in station mode, check for state updates
		 * while we're associated. We consider only 11g stuff right
		 * now.
		 */
<<<<<<< HEAD
		if (ic->ic_opmode == IEEE80211_M_STA &&
		    ic->ic_state == IEEE80211_S_ASSOC &&
		    ni->ni_state == IEEE80211_STA_BSS) {
			/*
			 * Check if protection mode has changed since last
			 * beacon.
			 */
			if (ni->ni_erp != erp) {
				IEEE80211_DPRINTF((
				    "[%s] erp change: was 0x%x, now 0x%x\n",
				    ether_sprintf(wh->i_addr2), ni->ni_erp,
				    erp));
				if (ic->ic_curmode == IEEE80211_MODE_11G &&
				    (erp & IEEE80211_ERP_USE_PROTECTION))
					ic->ic_flags |= IEEE80211_F_USEPROT;
				else
					ic->ic_flags &= ~IEEE80211_F_USEPROT;
				ic->ic_bss->ni_erp = erp;
			}

			/*
			 * Check if AP short slot time setting has changed
			 * since last beacon and give the driver a chance to
			 * update the hardware.
			 */
			if ((ni->ni_capinfo ^ letoh16(*(u_int16_t *)capinfo)) &
			    IEEE80211_CAPINFO_SHORT_SLOTTIME) {
				ieee80211_set_shortslottime(ic,
				    ic->ic_curmode == IEEE80211_MODE_11A ||
				    (letoh16(*(u_int16_t *)capinfo) &
				     IEEE80211_CAPINFO_SHORT_SLOTTIME));
			}
=======
		if (ni->ni_erp != erp) {
			DPRINTF(("[%s] erp change: was 0x%x, now 0x%x\n",
			    ether_sprintf((u_int8_t *)wh->i_addr2),
			    ni->ni_erp, erp));
			if (ic->ic_curmode == IEEE80211_MODE_11G &&
			    (erp & IEEE80211_ERP_USE_PROTECTION))
				ic->ic_flags |= IEEE80211_F_USEPROT;
			else
				ic->ic_flags &= ~IEEE80211_F_USEPROT;
			ic->ic_bss->ni_erp = erp;
>>>>>>> origin/master
		}

		if (ssid[1] != 0 && ni->ni_esslen == 0) {
			/*
			 * Update ESSID at probe response to adopt hidden AP by
			 * Lucent/Cisco, which announces null ESSID in beacon.
			 */
			ni->ni_esslen = ssid[1];
			memset(ni->ni_essid, 0, sizeof(ni->ni_essid));
			memcpy(ni->ni_essid, ssid + 2, ssid[1]);
		}
		IEEE80211_ADDR_COPY(ni->ni_bssid, wh->i_addr3);
		ni->ni_rssi = rssi;
		ni->ni_rstamp = rstamp;
		memcpy(ni->ni_tstamp, tstamp, sizeof(ni->ni_tstamp));
		ni->ni_intval = letoh16(*(u_int16_t *)bintval);
		ni->ni_capinfo = letoh16(*(u_int16_t *)capinfo);
		/* XXX validate channel # */
		ni->ni_chan = &ic->ic_channels[chan];
		ni->ni_fhdwell = fhdwell;
		ni->ni_fhindex = fhindex;
		ni->ni_erp = erp;
		/* NB: must be after ni_chan is setup */
		ieee80211_setup_rates(ic, ni, rates, xrates,
		    IEEE80211_F_DOSORT);

<<<<<<< HEAD
=======
	if (ic->ic_state == IEEE80211_S_SCAN &&
#ifndef IEEE80211_STA_ONLY
	    ic->ic_opmode != IEEE80211_M_HOSTAP &&
#endif
	    (ic->ic_flags & IEEE80211_F_RSNON)) {
		struct ieee80211_rsnparams rsn;
		const u_int8_t *saveie = NULL;
>>>>>>> origin/master
		/*
		 * When scanning we record results (nodes) with a zero
		 * refcnt.  Otherwise we want to hold the reference for
		 * ibss neighbors so the nodes don't get released prematurely.
		 * Anything else can be discarded (XXX and should be handled
		 * above so we don't do so much work).
		 */
		if (ic->ic_opmode == IEEE80211_M_IBSS || (is_new &&
		    ISPROBE(subtype))) {
			/*
			 * Fake an association so the driver can setup it's
			 * private state.  The rate set has been setup above;
			 * there is no handshake as in ap/station operation.
			 */
			if (ic->ic_newassoc)
				(*ic->ic_newassoc)(ic, ni, 1);
		}
<<<<<<< HEAD
		break;
	}

	case IEEE80211_FC0_SUBTYPE_PROBE_REQ: {
		u_int8_t rate;

		if (ic->ic_opmode == IEEE80211_M_STA ||
		    ic->ic_state != IEEE80211_S_RUN)
			return;

=======
		if (saveie != NULL &&
		    ieee80211_save_ie(saveie, &ni->ni_rsnie) == 0) {
			ni->ni_rsnakms = rsn.rsn_akms;
			ni->ni_rsnciphers = rsn.rsn_ciphers;
			ni->ni_rsngroupcipher = rsn.rsn_groupcipher;
			ni->ni_rsngroupmgmtcipher = rsn.rsn_groupmgmtcipher;
			ni->ni_rsncaps = rsn.rsn_caps;
		} else
			ni->ni_rsnprotos = IEEE80211_PROTO_NONE;
	} else if (ic->ic_state == IEEE80211_S_SCAN)
		ni->ni_rsnprotos = IEEE80211_PROTO_NONE;

	if (ssid[1] != 0 && ni->ni_esslen == 0) {
		ni->ni_esslen = ssid[1];
		memset(ni->ni_essid, 0, sizeof(ni->ni_essid));
		/* we know that ssid[1] <= IEEE80211_NWID_LEN */
		memcpy(ni->ni_essid, &ssid[2], ssid[1]);
	}
	IEEE80211_ADDR_COPY(ni->ni_bssid, wh->i_addr3);
	ni->ni_rssi = rxi->rxi_rssi;
	ni->ni_rstamp = rxi->rxi_tstamp;
	memcpy(ni->ni_tstamp, tstamp, sizeof(ni->ni_tstamp));
	ni->ni_intval = bintval;
	ni->ni_capinfo = capinfo;
	/* XXX validate channel # */
	ni->ni_chan = &ic->ic_channels[chan];
	ni->ni_erp = erp;
	/* NB: must be after ni_chan is setup */
	ieee80211_setup_rates(ic, ni, rates, xrates, IEEE80211_F_DOSORT);

	/*
	 * When scanning we record results (nodes) with a zero
	 * refcnt.  Otherwise we want to hold the reference for
	 * ibss neighbors so the nodes don't get released prematurely.
	 * Anything else can be discarded (XXX and should be handled
	 * above so we don't do so much work).
	 */
	if (
#ifndef IEEE80211_STA_ONLY
	    ic->ic_opmode == IEEE80211_M_IBSS ||
#endif
	    (is_new && isprobe)) {
		/*
		 * Fake an association so the driver can setup it's
		 * private state.  The rate set has been setup above;
		 * there is no handshake as in ap/station operation.
		 */
		if (ic->ic_newassoc)
			(*ic->ic_newassoc)(ic, ni, 1);
	}
}

#ifndef IEEE80211_STA_ONLY
/*-
 * Probe request frame format:
 * [tlv] SSID
 * [tlv] Supported rates
 * [tlv] Extended Supported Rates (802.11g)
 * [tlv] HT Capabilities (802.11n)
 */
void
ieee80211_recv_probe_req(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni, struct ieee80211_rxinfo *rxi)
{
	const struct ieee80211_frame *wh;
	const u_int8_t *frm, *efrm;
	const u_int8_t *ssid, *rates, *xrates, *htcaps;
	u_int8_t rate;

	if (ic->ic_opmode == IEEE80211_M_STA ||
	    ic->ic_state != IEEE80211_S_RUN)
		return;

	wh = mtod(m, struct ieee80211_frame *);
	frm = (const u_int8_t *)&wh[1];
	efrm = mtod(m, u_int8_t *) + m->m_len;

	ssid = rates = xrates = htcaps = NULL;
	while (frm + 2 <= efrm) {
		if (frm + 2 + frm[1] > efrm) {
			ic->ic_stats.is_rx_elem_toosmall++;
			break;
		}
		switch (frm[0]) {
		case IEEE80211_ELEMID_SSID:
			ssid = frm;
			break;
		case IEEE80211_ELEMID_RATES:
			rates = frm;
			break;
		case IEEE80211_ELEMID_XRATES:
			xrates = frm;
			break;
#ifndef IEEE80211_NO_HT
		case IEEE80211_ELEMID_HTCAPS:
			htcaps = frm;
			break;
#endif
		}
		frm += 2 + frm[1];
	}
	/* supported rates element is mandatory */
	if (rates == NULL || rates[1] > IEEE80211_RATE_MAXSIZE) {
		DPRINTF(("invalid supported rates element\n"));
		return;
	}
	/* SSID element is mandatory */
	if (ssid == NULL || ssid[1] > IEEE80211_NWID_LEN) {
		DPRINTF(("invalid SSID element\n"));
		return;
	}
	/* check that the specified SSID (if not wildcard) matches ours */
	if (ssid[1] != 0 && (ssid[1] != ic->ic_bss->ni_esslen ||
	    memcmp(&ssid[2], ic->ic_bss->ni_essid, ic->ic_bss->ni_esslen))) {
		DPRINTF(("SSID mismatch\n"));
		ic->ic_stats.is_rx_ssidmismatch++;
		return;
	}
	/* refuse wildcard SSID if we're hiding our SSID in beacons */
	if (ssid[1] == 0 && (ic->ic_flags & IEEE80211_F_HIDENWID)) {
		DPRINTF(("wildcard SSID rejected"));
		ic->ic_stats.is_rx_ssidmismatch++;
		return;
	}

	if (ni == ic->ic_bss) {
		ni = ieee80211_dup_bss(ic, wh->i_addr2);
		if (ni == NULL)
			return;
		DPRINTF(("new probe req from %s\n",
		    ether_sprintf((u_int8_t *)wh->i_addr2)));
	}
	ni->ni_rssi = rxi->rxi_rssi;
	ni->ni_rstamp = rxi->rxi_tstamp;
	rate = ieee80211_setup_rates(ic, ni, rates, xrates,
	    IEEE80211_F_DOSORT | IEEE80211_F_DOFRATE | IEEE80211_F_DONEGO |
	    IEEE80211_F_DODEL);
	if (rate & IEEE80211_RATE_BASIC) {
		DPRINTF(("rate mismatch for %s\n",
		    ether_sprintf((u_int8_t *)wh->i_addr2)));
		return;
	}
	IEEE80211_SEND_MGMT(ic, ni, IEEE80211_FC0_SUBTYPE_PROBE_RESP, 0);
}
#endif	/* IEEE80211_STA_ONLY */

/*-
 * Authentication frame format:
 * [2] Authentication algorithm number
 * [2] Authentication transaction sequence number
 * [2] Status code
 */
void
ieee80211_recv_auth(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni, struct ieee80211_rxinfo *rxi)
{
	const struct ieee80211_frame *wh;
	const u_int8_t *frm;
	u_int16_t algo, seq, status;

	/* make sure all mandatory fixed fields are present */
	if (m->m_len < sizeof(*wh) + 6) {
		DPRINTF(("frame too short\n"));
		return;
	}
	wh = mtod(m, struct ieee80211_frame *);
	frm = (const u_int8_t *)&wh[1];

	algo   = LE_READ_2(frm); frm += 2;
	seq    = LE_READ_2(frm); frm += 2;
	status = LE_READ_2(frm); frm += 2;
	DPRINTF(("auth %d seq %d from %s\n", algo, seq,
	    ether_sprintf((u_int8_t *)wh->i_addr2)));

	/* only "open" auth mode is supported */
	if (algo != IEEE80211_AUTH_ALG_OPEN) {
		DPRINTF(("unsupported auth algorithm %d from %s\n",
		    algo, ether_sprintf((u_int8_t *)wh->i_addr2)));
		ic->ic_stats.is_rx_auth_unsupported++;
#ifndef IEEE80211_STA_ONLY
		if (ic->ic_opmode == IEEE80211_M_HOSTAP) {
			/* XXX hack to workaround calling convention */
			IEEE80211_SEND_MGMT(ic, ni,
			    IEEE80211_FC0_SUBTYPE_AUTH,
			    IEEE80211_STATUS_ALG << 16 | ((seq + 1) & 0xffff));
		}
#endif
		return;
	}
	ieee80211_auth_open(ic, wh, ni, rxi, seq, status);
}

#ifndef IEEE80211_STA_ONLY
/*-
 * (Re)Association request frame format:
 * [2]   Capability information
 * [2]   Listen interval
 * [6*]  Current AP address (Reassociation only)
 * [tlv] SSID
 * [tlv] Supported rates
 * [tlv] Extended Supported Rates (802.11g)
 * [tlv] RSN (802.11i)
 * [tlv] QoS Capability (802.11e)
 * [tlv] HT Capabilities (802.11n)
 */
void
ieee80211_recv_assoc_req(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni, struct ieee80211_rxinfo *rxi, int reassoc)
{
	const struct ieee80211_frame *wh;
	const u_int8_t *frm, *efrm;
	const u_int8_t *ssid, *rates, *xrates, *rsnie, *wpaie, *htcaps;
	u_int16_t capinfo, bintval;
	int resp, status = 0;
	struct ieee80211_rsnparams rsn;
	u_int8_t rate;

	if (ic->ic_opmode != IEEE80211_M_HOSTAP ||
	    ic->ic_state != IEEE80211_S_RUN)
		return;

	/* make sure all mandatory fixed fields are present */
	if (m->m_len < sizeof(*wh) + (reassoc ? 10 : 4)) {
		DPRINTF(("frame too short\n"));
		return;
	}
	wh = mtod(m, struct ieee80211_frame *);
	frm = (const u_int8_t *)&wh[1];
	efrm = mtod(m, u_int8_t *) + m->m_len;

	if (!IEEE80211_ADDR_EQ(wh->i_addr3, ic->ic_bss->ni_bssid)) {
		DPRINTF(("ignore other bss from %s\n",
		    ether_sprintf((u_int8_t *)wh->i_addr2)));
		ic->ic_stats.is_rx_assoc_bss++;
		return;
	}
	capinfo = LE_READ_2(frm); frm += 2;
	bintval = LE_READ_2(frm); frm += 2;
	if (reassoc) {
		frm += IEEE80211_ADDR_LEN;	/* skip current AP address */
		resp = IEEE80211_FC0_SUBTYPE_REASSOC_RESP;
	} else
		resp = IEEE80211_FC0_SUBTYPE_ASSOC_RESP;

	ssid = rates = xrates = rsnie = wpaie = htcaps = NULL;
	while (frm + 2 <= efrm) {
		if (frm + 2 + frm[1] > efrm) {
			ic->ic_stats.is_rx_elem_toosmall++;
			break;
		}
		switch (frm[0]) {
		case IEEE80211_ELEMID_SSID:
			ssid = frm;
			break;
		case IEEE80211_ELEMID_RATES:
			rates = frm;
			break;
		case IEEE80211_ELEMID_XRATES:
			xrates = frm;
			break;
		case IEEE80211_ELEMID_RSN:
			rsnie = frm;
			break;
		case IEEE80211_ELEMID_QOS_CAP:
			break;
#ifndef IEEE80211_NO_HT
		case IEEE80211_ELEMID_HTCAPS:
			htcaps = frm;
			break;
#endif
		case IEEE80211_ELEMID_VENDOR:
			if (frm[1] < 4) {
				ic->ic_stats.is_rx_elem_toosmall++;
				break;
			}
			if (memcmp(frm + 2, MICROSOFT_OUI, 3) == 0) {
				if (frm[5] == 1)
					wpaie = frm;
			}
			break;
		}
		frm += 2 + frm[1];
	}
	/* supported rates element is mandatory */
	if (rates == NULL || rates[1] > IEEE80211_RATE_MAXSIZE) {
		DPRINTF(("invalid supported rates element\n"));
		return;
	}
	/* SSID element is mandatory */
	if (ssid == NULL || ssid[1] > IEEE80211_NWID_LEN) {
		DPRINTF(("invalid SSID element\n"));
		return;
	}
	/* check that the specified SSID matches ours */
	if (ssid[1] != ic->ic_bss->ni_esslen ||
	    memcmp(&ssid[2], ic->ic_bss->ni_essid, ic->ic_bss->ni_esslen)) {
		DPRINTF(("SSID mismatch\n"));
		ic->ic_stats.is_rx_ssidmismatch++;
		return;
	}

	if (ni->ni_state != IEEE80211_STA_AUTH &&
	    ni->ni_state != IEEE80211_STA_ASSOC) {
		DPRINTF(("deny %sassoc from %s, not authenticated\n",
		    reassoc ? "re" : "",
		    ether_sprintf((u_int8_t *)wh->i_addr2)));
		ni = ieee80211_dup_bss(ic, wh->i_addr2);
		if (ni != NULL) {
			IEEE80211_SEND_MGMT(ic, ni,
			    IEEE80211_FC0_SUBTYPE_DEAUTH,
			    IEEE80211_REASON_ASSOC_NOT_AUTHED);
		}
		ic->ic_stats.is_rx_assoc_notauth++;
		return;
	}

	if (ni->ni_state == IEEE80211_STA_ASSOC &&
	    (ni->ni_flags & IEEE80211_NODE_MFP)) {
		if (ni->ni_flags & IEEE80211_NODE_SA_QUERY_FAILED) {
			/* send a protected Disassociate frame */
			IEEE80211_SEND_MGMT(ic, ni,
			    IEEE80211_FC0_SUBTYPE_DISASSOC,
			    IEEE80211_REASON_AUTH_EXPIRE);
			/* terminate the old SA */
			ieee80211_node_leave(ic, ni);
		} else {
			/* reject the (Re)Association Request temporarily */
			IEEE80211_SEND_MGMT(ic, ni, resp,
			    IEEE80211_STATUS_TRY_AGAIN_LATER);
			/* start SA Query procedure if not already engaged */
			if (!(ni->ni_flags & IEEE80211_NODE_SA_QUERY))
				ieee80211_sa_query_request(ic, ni);
			/* do not modify association state */
		}
		return;
	}

	if (!(capinfo & IEEE80211_CAPINFO_ESS)) {
		ic->ic_stats.is_rx_assoc_capmismatch++;
		status = IEEE80211_STATUS_CAPINFO;
		goto end;
	}
	rate = ieee80211_setup_rates(ic, ni, rates, xrates,
	    IEEE80211_F_DOSORT | IEEE80211_F_DOFRATE | IEEE80211_F_DONEGO |
	    IEEE80211_F_DODEL);
	if (rate & IEEE80211_RATE_BASIC) {
		ic->ic_stats.is_rx_assoc_norate++;
		status = IEEE80211_STATUS_BASIC_RATE;
		goto end;
	}

	if (ic->ic_flags & IEEE80211_F_RSNON) {
		const u_int8_t *saveie;
>>>>>>> origin/master
		/*
		 * prreq frame format
		 *	[tlv] ssid
		 *	[tlv] supported rates
		 *	[tlv] extended supported rates
		 */
<<<<<<< HEAD
		ssid = rates = xrates = NULL;
		while (frm < efrm) {
			switch (*frm) {
			case IEEE80211_ELEMID_SSID:
				ssid = frm;
				break;
			case IEEE80211_ELEMID_RATES:
				rates = frm;
				break;
			case IEEE80211_ELEMID_XRATES:
				xrates = frm;
=======
		if (rsnie != NULL &&
		    (ic->ic_rsnprotos & IEEE80211_PROTO_RSN)) {
			status = ieee80211_parse_rsn(ic, rsnie, &rsn);
			if (status != 0)
				goto end;
			ni->ni_rsnprotos = IEEE80211_PROTO_RSN;
			saveie = rsnie;
		} else if (wpaie != NULL &&
		    (ic->ic_rsnprotos & IEEE80211_PROTO_WPA)) {
			status = ieee80211_parse_wpa(ic, wpaie, &rsn);
			if (status != 0)
				goto end;
			ni->ni_rsnprotos = IEEE80211_PROTO_WPA;
			saveie = wpaie;
		} else {
			/*
			 * In an RSN, an AP shall not associate with STAs
			 * that fail to include the RSN IE in the
			 * (Re)Association Request.
			 */
			status = IEEE80211_STATUS_IE_INVALID;
			goto end;
		}
		/*
		 * The initiating STA's RSN IE shall include one authentication
		 * and pairwise cipher suite among those advertised by the
		 * targeted AP.  It shall also specify the group cipher suite
		 * specified by the targeted AP.
		 */
		if (rsn.rsn_nakms != 1 ||
		    !(rsn.rsn_akms & ic->ic_bss->ni_rsnakms)) {
			status = IEEE80211_STATUS_BAD_AKMP;
			goto end;
		}
		if (rsn.rsn_nciphers != 1 ||
		    !(rsn.rsn_ciphers & ic->ic_bss->ni_rsnciphers)) {
			status = IEEE80211_STATUS_BAD_PAIRWISE_CIPHER;
			goto end;
		}
		if (rsn.rsn_groupcipher != ic->ic_bss->ni_rsngroupcipher) {
			status = IEEE80211_STATUS_BAD_GROUP_CIPHER;
			goto end;
		}

		if ((ic->ic_bss->ni_rsncaps & IEEE80211_RSNCAP_MFPR) &&
		    !(rsn.rsn_caps & IEEE80211_RSNCAP_MFPC)) {
			status = IEEE80211_STATUS_MFP_POLICY;
			goto end;
		}
		if ((ic->ic_bss->ni_rsncaps & IEEE80211_RSNCAP_MFPC) &&
		    (rsn.rsn_caps & (IEEE80211_RSNCAP_MFPC |
		     IEEE80211_RSNCAP_MFPR)) == IEEE80211_RSNCAP_MFPR) {
			/* STA advertises an invalid setting */
			status = IEEE80211_STATUS_MFP_POLICY;
			goto end;
		}
		/*
		 * A STA that has associated with Management Frame Protection
		 * enabled shall not use cipher suite pairwise selector WEP40,
		 * WEP104, TKIP, or "Use Group cipher suite".
		 */
		if ((rsn.rsn_caps & IEEE80211_RSNCAP_MFPC) &&
		    (rsn.rsn_ciphers != IEEE80211_CIPHER_CCMP ||
		     rsn.rsn_groupmgmtcipher !=
		     ic->ic_bss->ni_rsngroupmgmtcipher)) {
			status = IEEE80211_STATUS_MFP_POLICY;
			goto end;
		}

		/*
		 * Disallow new associations using TKIP if countermeasures
		 * are active.
		 */
		if ((ic->ic_flags & IEEE80211_F_COUNTERM) &&
		    (rsn.rsn_ciphers == IEEE80211_CIPHER_TKIP ||
		     rsn.rsn_groupcipher == IEEE80211_CIPHER_TKIP)) {
			status = IEEE80211_STATUS_CIPHER_REJ_POLICY;
			goto end;
		}

		/* everything looks fine, save IE and parameters */
		if (ieee80211_save_ie(saveie, &ni->ni_rsnie) != 0) {
			status = IEEE80211_STATUS_TOOMANY;
			goto end;
		}
		ni->ni_rsnakms = rsn.rsn_akms;
		ni->ni_rsnciphers = rsn.rsn_ciphers;
		ni->ni_rsngroupcipher = ic->ic_bss->ni_rsngroupcipher;
		ni->ni_rsngroupmgmtcipher = ic->ic_bss->ni_rsngroupmgmtcipher;
		ni->ni_rsncaps = rsn.rsn_caps;

		if (ieee80211_is_8021x_akm(ni->ni_rsnakms)) {
			struct ieee80211_pmk *pmk = NULL;
			const u_int8_t *pmkid = rsn.rsn_pmkids;
			/*
			 * Check if we have a cached PMK entry matching one
			 * of the PMKIDs specified in the RSN IE.
			 */
			while (rsn.rsn_npmkids-- > 0) {
				pmk = ieee80211_pmksa_find(ic, ni, pmkid);
				if (pmk != NULL)
					break;
				pmkid += IEEE80211_PMKID_LEN;
			}
			if (pmk != NULL) {
				memcpy(ni->ni_pmk, pmk->pmk_key,
				    IEEE80211_PMK_LEN);
				memcpy(ni->ni_pmkid, pmk->pmk_pmkid,
				    IEEE80211_PMKID_LEN);
				ni->ni_flags |= IEEE80211_NODE_PMK;
			}
		}
	} else
		ni->ni_rsnprotos = IEEE80211_PROTO_NONE;

	ni->ni_rssi = rxi->rxi_rssi;
	ni->ni_rstamp = rxi->rxi_tstamp;
	ni->ni_intval = bintval;
	ni->ni_capinfo = capinfo;
	ni->ni_chan = ic->ic_bss->ni_chan;
 end:
	if (status != 0) {
		IEEE80211_SEND_MGMT(ic, ni, resp, status);
		ieee80211_node_leave(ic, ni);
	} else
		ieee80211_node_join(ic, ni, resp);
}
#endif	/* IEEE80211_STA_ONLY */

/*-
 * (Re)Association response frame format:
 * [2]   Capability information
 * [2]   Status code
 * [2]   Association ID (AID)
 * [tlv] Supported rates
 * [tlv] Extended Supported Rates (802.11g)
 * [tlv] EDCA Parameter Set (802.11e)
 * [tlv] HT Capabilities (802.11n)
 * [tlv] HT Operation (802.11n)
 */
void
ieee80211_recv_assoc_resp(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni, int reassoc)
{
	struct ifnet *ifp = &ic->ic_if;
	const struct ieee80211_frame *wh;
	const u_int8_t *frm, *efrm;
	const u_int8_t *rates, *xrates, *edcaie, *wmmie, *htcaps, *htop;
	u_int16_t capinfo, status, associd;
	u_int8_t rate;

	if (ic->ic_opmode != IEEE80211_M_STA ||
	    ic->ic_state != IEEE80211_S_ASSOC) {
		ic->ic_stats.is_rx_mgtdiscard++;
		return;
	}

	/* make sure all mandatory fixed fields are present */
	if (m->m_len < sizeof(*wh) + 6) {
		DPRINTF(("frame too short\n"));
		return;
	}
	wh = mtod(m, struct ieee80211_frame *);
	frm = (const u_int8_t *)&wh[1];
	efrm = mtod(m, u_int8_t *) + m->m_len;

	capinfo = LE_READ_2(frm); frm += 2;
	status =  LE_READ_2(frm); frm += 2;
	if (status != IEEE80211_STATUS_SUCCESS) {
		if (ifp->if_flags & IFF_DEBUG)
			printf("%s: %sassociation failed (status %d)"
			    " for %s\n", ifp->if_xname,
			    reassoc ?  "re" : "",
			    status, ether_sprintf((u_int8_t *)wh->i_addr3));
		if (ni != ic->ic_bss)
			ni->ni_fails++;
		ic->ic_stats.is_rx_auth_fail++;
		return;
	}
	associd = LE_READ_2(frm); frm += 2;

	rates = xrates = edcaie = wmmie = htcaps = htop = NULL;
	while (frm + 2 <= efrm) {
		if (frm + 2 + frm[1] > efrm) {
			ic->ic_stats.is_rx_elem_toosmall++;
			break;
		}
		switch (frm[0]) {
		case IEEE80211_ELEMID_RATES:
			rates = frm;
			break;
		case IEEE80211_ELEMID_XRATES:
			xrates = frm;
			break;
		case IEEE80211_ELEMID_EDCAPARMS:
			edcaie = frm;
			break;
#ifndef IEEE80211_NO_HT
		case IEEE80211_ELEMID_HTCAPS:
			htcaps = frm;
			break;
		case IEEE80211_ELEMID_HTOP:
			htop = frm;
			break;
#endif
		case IEEE80211_ELEMID_VENDOR:
			if (frm[1] < 4) {
				ic->ic_stats.is_rx_elem_toosmall++;
>>>>>>> origin/master
				break;
			}
			frm += frm[1] + 2;
		}
<<<<<<< HEAD
		IEEE80211_VERIFY_ELEMENT(rates, IEEE80211_RATE_MAXSIZE);
		IEEE80211_VERIFY_ELEMENT(ssid, IEEE80211_NWID_LEN);
		IEEE80211_VERIFY_SSID(ic->ic_bss, ssid, "probe");
		if ((ic->ic_flags & IEEE80211_F_HIDENWID) && ssid[1] == 0) {
			IEEE80211_DPRINTF(("%s: no ssid "
			    "with ssid suppression enabled", __func__));
			ic->ic_stats.is_rx_ssidmismatch++;
			return;
		}

		if (ni == ic->ic_bss) {
			ni = ieee80211_dup_bss(ic, wh->i_addr2);
			if (ni == NULL)
				return;
			IEEE80211_DPRINTF(("%s: new probe req from %s\n",
			    __func__, ether_sprintf(wh->i_addr2)));
		}
		ni->ni_rssi = rssi;
		ni->ni_rstamp = rstamp;
		rate = ieee80211_setup_rates(ic, ni, rates, xrates,
				IEEE80211_F_DOSORT | IEEE80211_F_DOFRATE |
				IEEE80211_F_DONEGO | IEEE80211_F_DODEL);
		if (rate & IEEE80211_RATE_BASIC) {
			IEEE80211_DPRINTF(("%s: rate negotiation failed: %s\n",
			    __func__,ether_sprintf(wh->i_addr2)));
		} else {
			IEEE80211_SEND_MGMT(ic, ni,
				IEEE80211_FC0_SUBTYPE_PROBE_RESP, 0);
		}
		break;
	}

	case IEEE80211_FC0_SUBTYPE_AUTH: {
		u_int16_t algo, seq, status;
		/*
		 * auth frame format
		 *	[2] algorithm
		 *	[2] sequence
		 *	[2] status
		 *	[tlv*] challenge
		 */
		IEEE80211_VERIFY_LENGTH(efrm - frm, 6);
		algo   = letoh16(*(u_int16_t *)frm);
		seq    = letoh16(*(u_int16_t *)(frm + 2));
		status = letoh16(*(u_int16_t *)(frm + 4));
		IEEE80211_DPRINTF(("%s: auth %d seq %d from %s\n",
		    __func__, algo, seq, ether_sprintf(wh->i_addr2)));

		if (algo == IEEE80211_AUTH_ALG_OPEN)
			ieee80211_auth_open(ic, wh, ni, rssi, rstamp, seq,
			    status);
#if 0
		else if (algo == IEEE80211_AUTH_ALG_SHARED)
			ieee80211_auth_shared(ic, wh, frm + 6, efrm, ni, rssi,
			    rstamp, seq, status);
#endif
		else {
			IEEE80211_DPRINTF(("%s: unsupported authentication "
			    "algorithm %d from %s\n",
			    __func__, algo, ether_sprintf(wh->i_addr2)));
			ic->ic_stats.is_rx_auth_unsupported++;
			if (ic->ic_opmode == IEEE80211_M_HOSTAP) {
				/* XXX hack to workaround calling convention */
				IEEE80211_SEND_MGMT(ic, ni,
					IEEE80211_FC0_SUBTYPE_AUTH,
					(seq+1) | (IEEE80211_STATUS_ALG<<16));
			}
			return;
		}
		break;
=======
		frm += 2 + frm[1];
	}
	/* supported rates element is mandatory */
	if (rates == NULL || rates[1] > IEEE80211_RATE_MAXSIZE) {
		DPRINTF(("invalid supported rates element\n"));
		return;
	}
	rate = ieee80211_setup_rates(ic, ni, rates, xrates,
	    IEEE80211_F_DOSORT | IEEE80211_F_DOFRATE | IEEE80211_F_DONEGO |
	    IEEE80211_F_DODEL);
	if (rate & IEEE80211_RATE_BASIC) {
		DPRINTF(("rate mismatch for %s\n",
		    ether_sprintf((u_int8_t *)wh->i_addr2)));
		ic->ic_stats.is_rx_assoc_norate++;
		return;
	}
	ni->ni_capinfo = capinfo;
	ni->ni_associd = associd;
	if (edcaie != NULL || wmmie != NULL) {
		/* force update of EDCA parameters */
		ic->ic_edca_updtcount = -1;

		if ((edcaie != NULL &&
		     ieee80211_parse_edca_params(ic, edcaie) == 0) ||
		    (wmmie != NULL &&
		     ieee80211_parse_wmm_params(ic, wmmie) == 0))
			ni->ni_flags |= IEEE80211_NODE_QOS;
		else	/* for Reassociation */
			ni->ni_flags &= ~IEEE80211_NODE_QOS;
	}
	/*
	 * Configure state now that we are associated.
	 */
	if (ic->ic_curmode == IEEE80211_MODE_11A ||
	    (ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_PREAMBLE))
		ic->ic_flags |= IEEE80211_F_SHPREAMBLE;
	else
		ic->ic_flags &= ~IEEE80211_F_SHPREAMBLE;

	ieee80211_set_shortslottime(ic,
	    ic->ic_curmode == IEEE80211_MODE_11A ||
	    (ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_SLOTTIME));
	/*
	 * Honor ERP protection.
	 */
	if (ic->ic_curmode == IEEE80211_MODE_11G &&
	    (ni->ni_erp & IEEE80211_ERP_USE_PROTECTION))
		ic->ic_flags |= IEEE80211_F_USEPROT;
	else
		ic->ic_flags &= ~IEEE80211_F_USEPROT;
	/*
	 * If not an RSNA, mark the port as valid, otherwise wait for
	 * 802.1X authentication and 4-way handshake to complete..
	 */
	if (ic->ic_flags & IEEE80211_F_RSNON) {
		/* XXX ic->ic_mgt_timer = 5; */
	} else if (ic->ic_flags & IEEE80211_F_WEPON)
		ni->ni_flags |= IEEE80211_NODE_TXRXPROT;

	ieee80211_new_state(ic, IEEE80211_S_RUN,
	    IEEE80211_FC0_SUBTYPE_ASSOC_RESP);
}

/*-
 * Deauthentication frame format:
 * [2] Reason code
 */
void
ieee80211_recv_deauth(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni)
{
	const struct ieee80211_frame *wh;
	const u_int8_t *frm;
	u_int16_t reason;

	/* make sure all mandatory fixed fields are present */
	if (m->m_len < sizeof(*wh) + 2) {
		DPRINTF(("frame too short\n"));
		return;
	}
	wh = mtod(m, struct ieee80211_frame *);
	frm = (const u_int8_t *)&wh[1];

	reason = LE_READ_2(frm);

	ic->ic_stats.is_rx_deauth++;
	switch (ic->ic_opmode) {
	case IEEE80211_M_STA:
		ieee80211_new_state(ic, IEEE80211_S_AUTH,
		    IEEE80211_FC0_SUBTYPE_DEAUTH);
		break;
#ifndef IEEE80211_STA_ONLY
	case IEEE80211_M_HOSTAP:
		if (ni != ic->ic_bss) {
			if (ic->ic_if.if_flags & IFF_DEBUG)
				printf("%s: station %s deauthenticated "
				    "by peer (reason %d)\n",
				    ic->ic_if.if_xname,
				    ether_sprintf(ni->ni_macaddr),
				    reason);
			ieee80211_node_leave(ic, ni);
		}
		break;
#endif
	default:
		break;
	}
}

/*-
 * Disassociation frame format:
 * [2] Reason code
 */
void
ieee80211_recv_disassoc(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni)
{
	const struct ieee80211_frame *wh;
	const u_int8_t *frm;
	u_int16_t reason;

	/* make sure all mandatory fixed fields are present */
	if (m->m_len < sizeof(*wh) + 2) {
		DPRINTF(("frame too short\n"));
		return;
	}
	wh = mtod(m, struct ieee80211_frame *);
	frm = (const u_int8_t *)&wh[1];

	reason = LE_READ_2(frm);

	ic->ic_stats.is_rx_disassoc++;
	switch (ic->ic_opmode) {
	case IEEE80211_M_STA:
		ieee80211_new_state(ic, IEEE80211_S_ASSOC,
		    IEEE80211_FC0_SUBTYPE_DISASSOC);
		break;
#ifndef IEEE80211_STA_ONLY
	case IEEE80211_M_HOSTAP:
		if (ni != ic->ic_bss) {
			if (ic->ic_if.if_flags & IFF_DEBUG)
				printf("%s: station %s disassociated "
				    "by peer (reason %d)\n",
				    ic->ic_if.if_xname,
				    ether_sprintf(ni->ni_macaddr),
				    reason);
			ieee80211_node_leave(ic, ni);
		}
		break;
#endif
	default:
		break;
	}
}

#ifndef IEEE80211_NO_HT
/*-
 * ADDBA Request frame format:
 * [1] Category
 * [1] Action
 * [1] Dialog Token
 * [2] Block Ack Parameter Set
 * [2] Block Ack Timeout Value
 * [2] Block Ack Starting Sequence Control
 */
void
ieee80211_recv_addba_req(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni)
{
	const struct ieee80211_frame *wh;
	const u_int8_t *frm;
	struct ieee80211_rx_ba *ba;
	u_int16_t params, ssn, bufsz, timeout, status;
	u_int8_t token, tid;

	if (!(ni->ni_flags & IEEE80211_NODE_HT)) {
		DPRINTF(("received ADDBA req from non-HT STA %s\n",
		    ether_sprintf(ni->ni_macaddr)));
		return;
	}
	if (m->m_len < sizeof(*wh) + 9) {
		DPRINTF(("frame too short\n"));
		return;
	}
	/* MLME-ADDBA.indication */
	wh = mtod(m, struct ieee80211_frame *);
	frm = (const u_int8_t *)&wh[1];

	token = frm[2];
	params = LE_READ_2(&frm[3]);
	tid = (params >> 2) & 0xf;
	bufsz = (params >> 6) & 0x3ff;
	timeout = LE_READ_2(&frm[5]);
	ssn = LE_READ_2(&frm[7]) >> 4;

	ba = &ni->ni_rx_ba[tid];
	/* check if we already have a Block Ack agreement for this RA/TID */
	if (ba->ba_state == IEEE80211_BA_AGREED) {
		/* XXX should we update the timeout value? */
		/* reset Block Ack inactivity timer */
		timeout_add_usec(&ba->ba_to, ba->ba_timeout_val);

		/* check if it's a Protected Block Ack agreement */
		if (!(ni->ni_flags & IEEE80211_NODE_MFP) ||
		    !(ni->ni_rsncaps & IEEE80211_RSNCAP_PBAC))
			return;	/* not a PBAC, ignore */

		/* PBAC: treat the ADDBA Request like a BlockAckReq */
		if (SEQ_LT(ba->ba_winstart, ssn))
			ieee80211_ba_move_window(ic, ni, tid, ssn);
		return;
	}
	/* if PBAC required but RA does not support it, refuse request */
	if ((ic->ic_flags & IEEE80211_F_PBAR) &&
	    (!(ni->ni_flags & IEEE80211_NODE_MFP) ||
	     !(ni->ni_rsncaps & IEEE80211_RSNCAP_PBAC))) {
		status = IEEE80211_STATUS_REFUSED;
		goto resp;
	}
	/*
	 * If the TID for which the Block Ack agreement is requested is
	 * configured with a no-ACK policy, refuse the agreement.
	 */
	if (ic->ic_tid_noack & (1 << tid)) {
		status = IEEE80211_STATUS_REFUSED;
		goto resp;
	}
	/* check that we support the requested Block Ack Policy */
	if (!(ic->ic_htcaps & IEEE80211_HTCAP_DELAYEDBA) &&
	    !(params & IEEE80211_BA_ACK_POLICY)) {
		status = IEEE80211_STATUS_INVALID_PARAM;
		goto resp;
	}

	/* setup Block Ack agreement */
	ba->ba_state = IEEE80211_BA_INIT;
	ba->ba_timeout_val = timeout * IEEE80211_DUR_TU;
	if (ba->ba_timeout_val < IEEE80211_BA_MIN_TIMEOUT)
		ba->ba_timeout_val = IEEE80211_BA_MIN_TIMEOUT;
	else if (ba->ba_timeout_val > IEEE80211_BA_MAX_TIMEOUT)
		ba->ba_timeout_val = IEEE80211_BA_MAX_TIMEOUT;
	timeout_set(&ba->ba_to, ieee80211_rx_ba_timeout, ba);
	ba->ba_winsize = bufsz;
	if (ba->ba_winsize == 0 || ba->ba_winsize > IEEE80211_BA_MAX_WINSZ)
		ba->ba_winsize = IEEE80211_BA_MAX_WINSZ;
	ba->ba_winstart = ssn;
	ba->ba_winend = (ba->ba_winstart + ba->ba_winsize - 1) & 0xfff;
	/* allocate and setup our reordering buffer */
	ba->ba_buf = malloc(IEEE80211_BA_MAX_WINSZ * sizeof(*ba->ba_buf),
	    M_DEVBUF, M_NOWAIT | M_ZERO);
	if (ba->ba_buf == NULL) {
		status = IEEE80211_STATUS_REFUSED;
		goto resp;
	}
	ba->ba_head = 0;

	/* notify drivers of this new Block Ack agreement */
	if (ic->ic_ampdu_rx_start != NULL &&
	    ic->ic_ampdu_rx_start(ic, ni, tid) != 0) {
		/* driver failed to setup, rollback */
		free(ba->ba_buf, M_DEVBUF);
		ba->ba_buf = NULL;
		status = IEEE80211_STATUS_REFUSED;
		goto resp;
	}
	ba->ba_state = IEEE80211_BA_AGREED;
	/* start Block Ack inactivity timer */
	timeout_add_usec(&ba->ba_to, ba->ba_timeout_val);
	status = IEEE80211_STATUS_SUCCESS;
 resp:
	/* MLME-ADDBA.response */
	IEEE80211_SEND_ACTION(ic, ni, IEEE80211_CATEG_BA,
	    IEEE80211_ACTION_ADDBA_RESP, status << 16 | token << 8 | tid);
}

/*-
 * ADDBA Response frame format:
 * [1] Category
 * [1] Action
 * [1] Dialog Token
 * [2] Status Code
 * [2] Block Ack Parameter Set
 * [2] Block Ack Timeout Value
 */
void
ieee80211_recv_addba_resp(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni)
{
	const struct ieee80211_frame *wh;
	const u_int8_t *frm;
	struct ieee80211_tx_ba *ba;
	u_int16_t status, params, bufsz, timeout;
	u_int8_t token, tid;

	if (m->m_len < sizeof(*wh) + 9) {
		DPRINTF(("frame too short\n"));
		return;
	}
	wh = mtod(m, struct ieee80211_frame *);
	frm = (const u_int8_t *)&wh[1];

	token = frm[2];
	status = LE_READ_2(&frm[3]);
	params = LE_READ_2(&frm[5]);
	tid = (params >> 2) & 0xf;
	bufsz = (params >> 6) & 0x3ff;
	timeout = LE_READ_2(&frm[7]);

	DPRINTF(("received ADDBA resp from %s, TID %d, status %d\n",
	    ether_sprintf(ni->ni_macaddr), tid, status));

	/*
	 * Ignore if no ADDBA request has been sent for this RA/TID or
	 * if we already have a Block Ack agreement.
	 */
	ba = &ni->ni_tx_ba[tid];
	if (ba->ba_state != IEEE80211_BA_REQUESTED) {
		DPRINTF(("no matching ADDBA req found\n"));
		return;
	}
	if (token != ba->ba_token) {
		DPRINTF(("ignoring ADDBA resp from %s: token %x!=%x\n",
		    ether_sprintf(ni->ni_macaddr), token, ba->ba_token));
		return;
	}
	/* we got an ADDBA Response matching our request, stop timeout */
	timeout_del(&ba->ba_to);

	if (status != IEEE80211_STATUS_SUCCESS) {
		/* MLME-ADDBA.confirm(Failure) */
		ba->ba_state = IEEE80211_BA_INIT;
		return;
>>>>>>> origin/master
	}
	/* MLME-ADDBA.confirm(Success) */
	ba->ba_state = IEEE80211_BA_AGREED;

<<<<<<< HEAD
	case IEEE80211_FC0_SUBTYPE_ASSOC_REQ:
	case IEEE80211_FC0_SUBTYPE_REASSOC_REQ: {
		u_int16_t capinfo, bintval;

		if (ic->ic_opmode != IEEE80211_M_HOSTAP ||
		    ic->ic_state != IEEE80211_S_RUN)
			return;

		if (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ) {
			reassoc = 1;
			resp = IEEE80211_FC0_SUBTYPE_REASSOC_RESP;
		} else {
			reassoc = 0;
			resp = IEEE80211_FC0_SUBTYPE_ASSOC_RESP;
		}
		/*
		 * asreq frame format
		 *	[2] capability information
		 *	[2] listen interval
		 *	[6*] current AP address (reassoc only)
		 *	[tlv] ssid
		 *	[tlv] supported rates
		 *	[tlv] extended supported rates
		 */
		IEEE80211_VERIFY_LENGTH(efrm - frm, (reassoc ? 10 : 4));
		if (!IEEE80211_ADDR_EQ(wh->i_addr3, ic->ic_bss->ni_bssid)) {
			IEEE80211_DPRINTF(("%s: ignore other bss from %s\n",
			    __func__, ether_sprintf(wh->i_addr2)));
			ic->ic_stats.is_rx_assoc_bss++;
			return;
		}
		capinfo = letoh16(*(u_int16_t *)frm);	frm += 2;
		bintval = letoh16(*(u_int16_t *)frm);	frm += 2;
		if (reassoc)
			frm += 6;	/* ignore current AP info */
		ssid = rates = xrates = NULL;
		while (frm < efrm) {
			switch (*frm) {
			case IEEE80211_ELEMID_SSID:
				ssid = frm;
				break;
			case IEEE80211_ELEMID_RATES:
				rates = frm;
				break;
			case IEEE80211_ELEMID_XRATES:
				xrates = frm;
				break;
			}
			frm += frm[1] + 2;
		}
		IEEE80211_VERIFY_ELEMENT(rates, IEEE80211_RATE_MAXSIZE);
		IEEE80211_VERIFY_ELEMENT(ssid, IEEE80211_NWID_LEN);
		IEEE80211_VERIFY_SSID(ic->ic_bss, ssid,
			reassoc ? "reassoc" : "assoc");

		if (ni->ni_state != IEEE80211_STA_AUTH &&
		    ni->ni_state != IEEE80211_STA_ASSOC) {
			IEEE80211_DPRINTF(
			    ("%s: deny %sassoc from %s, not authenticated\n",
			    __func__, reassoc ? "re" : "",
			    ether_sprintf(wh->i_addr2)));
			ni = ieee80211_dup_bss(ic, wh->i_addr2);
			if (ni != NULL) {
				IEEE80211_SEND_MGMT(ic, ni,
				    IEEE80211_FC0_SUBTYPE_DEAUTH,
				    IEEE80211_REASON_ASSOC_NOT_AUTHED);
			}
			ic->ic_stats.is_rx_assoc_notauth++;
			return;
		}
		/* discard challenge after association */
		if (ni->ni_challenge != NULL) {
			FREE(ni->ni_challenge, M_DEVBUF);
			ni->ni_challenge = NULL;
		}
		/* XXX per-node cipher suite */
		/* XXX some stations use the privacy bit for handling APs
		       that suport both encrypted and unencrypted traffic */
		if ((capinfo & IEEE80211_CAPINFO_ESS) == 0 ||
		    (capinfo & IEEE80211_CAPINFO_PRIVACY) !=
		    ((ic->ic_flags & IEEE80211_F_WEPON) ?
		     IEEE80211_CAPINFO_PRIVACY : 0)) {
			IEEE80211_DPRINTF(("%s: rate mismatch for %s\n",
			    __func__, ether_sprintf(wh->i_addr2)));
			/* XXX what rate will we send this at? */
			IEEE80211_SEND_MGMT(ic, ni, resp,
			    IEEE80211_STATUS_BASIC_RATE);
			ieee80211_node_leave(ic, ni);
			ic->ic_stats.is_rx_assoc_capmismatch++;
			return;
		}
		ieee80211_setup_rates(ic, ni, rates, xrates,
				IEEE80211_F_DOSORT | IEEE80211_F_DOFRATE |
				IEEE80211_F_DONEGO | IEEE80211_F_DODEL);
		if (ni->ni_rates.rs_nrates == 0) {
			IEEE80211_DPRINTF(("%s: rate mismatch for %s\n",
			    __func__, ether_sprintf(wh->i_addr2)));
			IEEE80211_AID_CLR(ni->ni_associd, ic->ic_aid_bitmap);
			ni->ni_associd = 0;
			IEEE80211_SEND_MGMT(ic, ni, resp,
				IEEE80211_STATUS_BASIC_RATE);
			ic->ic_stats.is_rx_assoc_norate++;
			return;
		}
		ni->ni_rssi = rssi;
		ni->ni_rstamp = rstamp;
		ni->ni_intval = bintval;
		ni->ni_capinfo = capinfo;
		ni->ni_chan = ic->ic_bss->ni_chan;
		ni->ni_fhdwell = ic->ic_bss->ni_fhdwell;
		ni->ni_fhindex = ic->ic_bss->ni_fhindex;

		ieee80211_node_join(ic, ni, resp);
		break;
	}

	case IEEE80211_FC0_SUBTYPE_ASSOC_RESP:
	case IEEE80211_FC0_SUBTYPE_REASSOC_RESP: {
		u_int16_t status;

		if (ic->ic_opmode != IEEE80211_M_STA ||
		    ic->ic_state != IEEE80211_S_ASSOC) {
			ic->ic_stats.is_rx_mgtdiscard++;
			return;
		}

		/*
		 * asresp frame format
		 *	[2] capability information
		 *	[2] status
		 *	[2] association ID
		 *	[tlv] supported rates
		 *	[tlv] extended supported rates
		 */
		IEEE80211_VERIFY_LENGTH(efrm - frm, 6);
		ni = ic->ic_bss;
		ni->ni_capinfo = letoh16(*(u_int16_t *)frm);
		frm += 2;

		status = letoh16(*(u_int16_t *)frm);
		frm += 2;
		if (status != 0) {
			if (ifp->if_flags & IFF_DEBUG)
				printf("%s: %sassociation failed (reason %d)"
				    " for %s\n", ifp->if_xname,
				    ISREASSOC(subtype) ?  "re" : "",
				    status, ether_sprintf(wh->i_addr3));
			if (ni != ic->ic_bss)
				ni->ni_fails++;
			ic->ic_stats.is_rx_auth_fail++;
			return;
		}
		ni->ni_associd = letoh16(*(u_int16_t *)frm);
		frm += 2;

		rates = xrates = NULL;
		while (frm < efrm) {
			switch (*frm) {
			case IEEE80211_ELEMID_RATES:
				rates = frm;
				break;
			case IEEE80211_ELEMID_XRATES:
				xrates = frm;
				break;
			}
			frm += frm[1] + 2;
		}

		IEEE80211_VERIFY_ELEMENT(rates, IEEE80211_RATE_MAXSIZE);
		ieee80211_setup_rates(ic, ni, rates, xrates,
				IEEE80211_F_DOSORT | IEEE80211_F_DOFRATE |
				IEEE80211_F_DONEGO | IEEE80211_F_DODEL);
		if (ni->ni_rates.rs_nrates == 0)
			break;

		/*
		 * Configure state now that we are associated.
		 */
		if (ic->ic_curmode == IEEE80211_MODE_11A ||
		    (ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_PREAMBLE))
			ic->ic_flags |= IEEE80211_F_SHPREAMBLE;
		else
			ic->ic_flags &= ~IEEE80211_F_SHPREAMBLE;

		ieee80211_set_shortslottime(ic,
		    ic->ic_curmode == IEEE80211_MODE_11A ||
		    (ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_SLOTTIME));
		/*
		 * Honor ERP protection.
		 */
		if (ic->ic_curmode == IEEE80211_MODE_11G &&
		    (ni->ni_erp & IEEE80211_ERP_USE_PROTECTION))
			ic->ic_flags |= IEEE80211_F_USEPROT;
		else
			ic->ic_flags &= ~IEEE80211_F_USEPROT;

		ieee80211_new_state(ic, IEEE80211_S_RUN,
			wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK);
		break;
	}

	case IEEE80211_FC0_SUBTYPE_DEAUTH: {
		u_int16_t reason;
		/*
		 * deauth frame format
		 *	[2] reason
		 */
		IEEE80211_VERIFY_LENGTH(efrm - frm, 2);
		reason = letoh16(*(u_int16_t *)frm);
		ic->ic_stats.is_rx_deauth++;
		switch (ic->ic_opmode) {
		case IEEE80211_M_STA:
			ieee80211_new_state(ic, IEEE80211_S_AUTH,
			    wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK);
			break;
		case IEEE80211_M_HOSTAP:
			if (ni != ic->ic_bss) {
				if (ifp->if_flags & IFF_DEBUG)
					printf("%s: station %s deauthenticated "
					    "by peer (reason %d)\n",
					    ifp->if_xname,
					    ether_sprintf(ni->ni_macaddr),
					    reason);
				ieee80211_node_leave(ic, ni);
			}
			break;
		default:
			break;
		}
		break;
	}

	case IEEE80211_FC0_SUBTYPE_DISASSOC: {
		u_int16_t reason;
		/*
		 * disassoc frame format
		 *	[2] reason
		 */
		IEEE80211_VERIFY_LENGTH(efrm - frm, 2);
		reason = letoh16(*(u_int16_t *)frm);
		ic->ic_stats.is_rx_disassoc++;
		switch (ic->ic_opmode) {
		case IEEE80211_M_STA:
			ieee80211_new_state(ic, IEEE80211_S_ASSOC,
			    wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK);
			break;
		case IEEE80211_M_HOSTAP:
			if (ni != ic->ic_bss) {
				if (ifp->if_flags & IFF_DEBUG)
					printf("%s: station %s disassociated "
					    "by peer (reason %d)\n",
					    ifp->if_xname,
					    ether_sprintf(ni->ni_macaddr),
					    reason);
				ieee80211_node_leave(ic, ni);
			}
			break;
		default:
			break;
		}
		break;
	}
	default:
		IEEE80211_DPRINTF(("%s: mgmt frame with subtype 0x%x not "
		    "handled\n", __func__, subtype));
		ic->ic_stats.is_rx_badsubtype++;
		break;
=======
	/* notify drivers of this new Block Ack agreement */
	if (ic->ic_ampdu_tx_start != NULL)
		(void)ic->ic_ampdu_tx_start(ic, ni, tid);

	/* start Block Ack inactivity timeout */
	if (ba->ba_timeout_val != 0)
		timeout_add_usec(&ba->ba_to, ba->ba_timeout_val);
}

/*-
 * DELBA frame format:
 * [1] Category
 * [1] Action
 * [2] DELBA Parameter Set
 * [2] Reason Code
 */
void
ieee80211_recv_delba(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni)
{
	const struct ieee80211_frame *wh;
	const u_int8_t *frm;
	u_int16_t params, reason;
	u_int8_t tid;
	int i;

	if (m->m_len < sizeof(*wh) + 6) {
		DPRINTF(("frame too short\n"));
		return;
	}
	wh = mtod(m, struct ieee80211_frame *);
	frm = (const u_int8_t *)&wh[1];

	params = LE_READ_2(&frm[2]);
	reason = LE_READ_2(&frm[4]);
	tid = params >> 12;

	DPRINTF(("received DELBA from %s, TID %d, reason %d\n",
	    ether_sprintf(ni->ni_macaddr), tid, reason));

	if (params & IEEE80211_DELBA_INITIATOR) {
		/* MLME-DELBA.indication(Originator) */
		struct ieee80211_rx_ba *ba = &ni->ni_rx_ba[tid];

		if (ba->ba_state != IEEE80211_BA_AGREED) {
			DPRINTF(("no matching Block Ack agreement\n"));
			return;
		}
		/* notify drivers of the end of the Block Ack agreement */
		if (ic->ic_ampdu_rx_stop != NULL)
			ic->ic_ampdu_rx_stop(ic, ni, tid);

		ba->ba_state = IEEE80211_BA_INIT;
		/* stop Block Ack inactivity timer */
		timeout_del(&ba->ba_to);

		if (ba->ba_buf != NULL) {
			/* free all MSDUs stored in reordering buffer */
			for (i = 0; i < IEEE80211_BA_MAX_WINSZ; i++)
				if (ba->ba_buf[i].m != NULL)
					m_freem(ba->ba_buf[i].m);
			/* free reordering buffer */
			free(ba->ba_buf, M_DEVBUF);
			ba->ba_buf = NULL;
		}
	} else {
		/* MLME-DELBA.indication(Recipient) */
		struct ieee80211_tx_ba *ba = &ni->ni_tx_ba[tid];

		if (ba->ba_state != IEEE80211_BA_AGREED) {
			DPRINTF(("no matching Block Ack agreement\n"));
			return;
		}
		/* notify drivers of the end of the Block Ack agreement */
		if (ic->ic_ampdu_tx_stop != NULL)
			ic->ic_ampdu_tx_stop(ic, ni, tid);

		ba->ba_state = IEEE80211_BA_INIT;
		/* stop Block Ack inactivity timer */
		timeout_del(&ba->ba_to);
	}
}
#endif	/* !IEEE80211_NO_HT */

/*-
 * SA Query Request frame format:
 * [1] Category
 * [1] Action
 * [2] Transaction Identifier
 */
void
ieee80211_recv_sa_query_req(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni)
{
	const struct ieee80211_frame *wh;
	const u_int8_t *frm;

	if (ic->ic_opmode != IEEE80211_M_STA ||
	    !(ni->ni_flags & IEEE80211_NODE_MFP)) {
		DPRINTF(("unexpected SA Query req from %s\n",
		    ether_sprintf(ni->ni_macaddr)));
		return;
	}
	if (m->m_len < sizeof(*wh) + 4) {
		DPRINTF(("frame too short\n"));
		return;
	}
	wh = mtod(m, struct ieee80211_frame *);
	frm = (const u_int8_t *)&wh[1];

	/* MLME-SAQuery.indication */

	/* save Transaction Identifier for SA Query Response */
	ni->ni_sa_query_trid = LE_READ_2(&frm[2]);

	/* MLME-SAQuery.response */
	IEEE80211_SEND_ACTION(ic, ni, IEEE80211_CATEG_SA_QUERY,
	    IEEE80211_ACTION_SA_QUERY_RESP, 0);
}

#ifndef IEEE80211_STA_ONLY
/*-
 * SA Query Response frame format:
 * [1] Category
 * [1] Action
 * [2] Transaction Identifier
 */
void
ieee80211_recv_sa_query_resp(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni)
{
	const struct ieee80211_frame *wh;
	const u_int8_t *frm;

	/* ignore if we're not engaged in an SA Query with that STA */
	if (!(ni->ni_flags & IEEE80211_NODE_SA_QUERY)) {
		DPRINTF(("unexpected SA Query resp from %s\n",
		    ether_sprintf(ni->ni_macaddr)));
		return;
	}
	if (m->m_len < sizeof(*wh) + 4) {
		DPRINTF(("frame too short\n"));
		return;
	}
	wh = mtod(m, struct ieee80211_frame *);
	frm = (const u_int8_t *)&wh[1];

	/* check that Transaction Identifier matches */
	if (ni->ni_sa_query_trid != LE_READ_2(&frm[2])) {
		DPRINTF(("transaction identifier does not match\n"));
		return;
	}
	/* MLME-SAQuery.confirm */
	timeout_del(&ni->ni_sa_query_to);
	ni->ni_flags &= ~IEEE80211_NODE_SA_QUERY;
}
#endif

/*-
 * Action frame format:
 * [1] Category
 * [1] Action
 */
void
ieee80211_recv_action(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni)
{
	const struct ieee80211_frame *wh;
	const u_int8_t *frm;

	if (m->m_len < sizeof(*wh) + 2) {
		DPRINTF(("frame too short\n"));
		return;
	}
	wh = mtod(m, struct ieee80211_frame *);
	frm = (const u_int8_t *)&wh[1];

	switch (frm[0]) {
#ifndef IEEE80211_NO_HT
	case IEEE80211_CATEG_BA:
		switch (frm[1]) {
		case IEEE80211_ACTION_ADDBA_REQ:
			ieee80211_recv_addba_req(ic, m, ni);
			break;
		case IEEE80211_ACTION_ADDBA_RESP:
			ieee80211_recv_addba_resp(ic, m, ni);
			break;
		case IEEE80211_ACTION_DELBA:
			ieee80211_recv_delba(ic, m, ni);
			break;
		}
		break;
#endif
	case IEEE80211_CATEG_SA_QUERY:
		switch (frm[1]) {
		case IEEE80211_ACTION_SA_QUERY_REQ:
			ieee80211_recv_sa_query_req(ic, m, ni);
			break;
#ifndef IEEE80211_STA_ONLY
		case IEEE80211_ACTION_SA_QUERY_RESP:
			ieee80211_recv_sa_query_resp(ic, m, ni);
			break;
#endif
		}
		break;
	default:
		DPRINTF(("action frame category %d not handled\n", frm[0]));
		break;
	}
}

void
ieee80211_recv_mgmt(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni, struct ieee80211_rxinfo *rxi, int subtype)
{
	switch (subtype) {
	case IEEE80211_FC0_SUBTYPE_BEACON:
		ieee80211_recv_probe_resp(ic, m, ni, rxi, 0);
		break;
	case IEEE80211_FC0_SUBTYPE_PROBE_RESP:
		ieee80211_recv_probe_resp(ic, m, ni, rxi, 1);
		break;
#ifndef IEEE80211_STA_ONLY
	case IEEE80211_FC0_SUBTYPE_PROBE_REQ:
		ieee80211_recv_probe_req(ic, m, ni, rxi);
		break;
#endif
	case IEEE80211_FC0_SUBTYPE_AUTH:
		ieee80211_recv_auth(ic, m, ni, rxi);
		break;
#ifndef IEEE80211_STA_ONLY
	case IEEE80211_FC0_SUBTYPE_ASSOC_REQ:
		ieee80211_recv_assoc_req(ic, m, ni, rxi, 0);
		break;
	case IEEE80211_FC0_SUBTYPE_REASSOC_REQ:
		ieee80211_recv_assoc_req(ic, m, ni, rxi, 1);
		break;
#endif
	case IEEE80211_FC0_SUBTYPE_ASSOC_RESP:
		ieee80211_recv_assoc_resp(ic, m, ni, 0);
		break;
	case IEEE80211_FC0_SUBTYPE_REASSOC_RESP:
		ieee80211_recv_assoc_resp(ic, m, ni, 1);
		break;
	case IEEE80211_FC0_SUBTYPE_DEAUTH:
		ieee80211_recv_deauth(ic, m, ni);
		break;
	case IEEE80211_FC0_SUBTYPE_DISASSOC:
		ieee80211_recv_disassoc(ic, m, ni);
		break;
	case IEEE80211_FC0_SUBTYPE_ACTION:
		ieee80211_recv_action(ic, m, ni);
		break;
	default:
		DPRINTF(("mgmt frame with subtype 0x%x not handled\n",
		    subtype));
		ic->ic_stats.is_rx_badsubtype++;
		break;
	}
}

#ifndef IEEE80211_STA_ONLY
/*
 * Process an incoming PS-Poll control frame (see 11.2).
 */
void
ieee80211_recv_pspoll(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni)
{
	struct ifnet *ifp = &ic->ic_if;
	struct ieee80211_frame_pspoll *psp;
	struct ieee80211_frame *wh;
	u_int16_t aid;

	if (ic->ic_opmode != IEEE80211_M_HOSTAP ||
	    !(ic->ic_caps & IEEE80211_C_APPMGT) ||
	    ni->ni_state != IEEE80211_STA_ASSOC)
		return;

	if (m->m_len < sizeof(*psp)) {
		DPRINTF(("frame too short, len %u\n", m->m_len));
		ic->ic_stats.is_rx_tooshort++;
		return;
	}
	psp = mtod(m, struct ieee80211_frame_pspoll *);
	if (!IEEE80211_ADDR_EQ(psp->i_bssid, ic->ic_bss->ni_bssid)) {
		DPRINTF(("discard pspoll frame to BSS %s\n",
		    ether_sprintf(psp->i_bssid)));
		ic->ic_stats.is_rx_wrongbss++;
		return;
	}
	aid = letoh16(*(u_int16_t *)psp->i_aid);
	if (aid != ni->ni_associd) {
		DPRINTF(("invalid pspoll aid %x from %s\n", aid,
		    ether_sprintf(psp->i_ta)));
		return;
	}

	/* take the first queued frame and put it out.. */
	IF_DEQUEUE(&ni->ni_savedq, m);
	if (m == NULL)
		return;
	if (IF_IS_EMPTY(&ni->ni_savedq)) {
		/* last queued frame, turn off the TIM bit */
		(*ic->ic_set_tim)(ic, ni->ni_associd, 0);
	} else {
		/* more queued frames, set the more data bit */
		wh = mtod(m, struct ieee80211_frame *);
		wh->i_fc[1] |= IEEE80211_FC1_MORE_DATA;
	}
	IF_ENQUEUE(&ic->ic_pwrsaveq, m);
	(*ifp->if_start)(ifp);
}
#endif	/* IEEE80211_STA_ONLY */

#ifndef IEEE80211_NO_HT
/*
 * Process an incoming BlockAckReq control frame (see 7.2.1.7).
 */
void
ieee80211_recv_bar(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni)
{
	const struct ieee80211_frame_min *wh;
	const u_int8_t *frm;
	u_int16_t ctl, ssn;
	u_int8_t tid, ntids;

	if (!(ni->ni_flags & IEEE80211_NODE_HT)) {
		DPRINTF(("received BlockAckReq from non-HT STA %s\n",
		    ether_sprintf(ni->ni_macaddr)));
		return;
	}
	if (m->m_len < sizeof(*wh) + 4) {
		DPRINTF(("frame too short\n"));
		return;
	}
	wh = mtod(m, struct ieee80211_frame_min *);
	frm = (const u_int8_t *)&wh[1];

	/* read BlockAckReq Control field */
	ctl = LE_READ_2(&frm[0]);
	tid = ctl >> 12;

	/* determine BlockAckReq frame variant */
	if (ctl & IEEE80211_BA_MULTI_TID) {
		/* Multi-TID BlockAckReq variant (PSMP only) */
		ntids = tid + 1;

		if (m->m_len < sizeof(*wh) + 2 + 4 * ntids) {
			DPRINTF(("MTBAR frame too short\n"));
			return;
		}
		frm += 2;	/* skip BlockAckReq Control field */
		while (ntids-- > 0) {
			/* read MTBAR Information field */
			tid = LE_READ_2(&frm[0]) >> 12;
			ssn = LE_READ_2(&frm[2]) >> 4;
			ieee80211_bar_tid(ic, ni, tid, ssn);
			frm += 4;
		}
	} else {
		/* Basic or Compressed BlockAckReq variants */
		ssn = LE_READ_2(&frm[2]) >> 4;
		ieee80211_bar_tid(ic, ni, tid, ssn);
>>>>>>> origin/master
	}
}
#undef IEEE80211_VERIFY_LENGTH
#undef IEEE80211_VERIFY_ELEMENT
#undef IEEE80211_VERIFY_SSID

<<<<<<< HEAD
static void
ieee80211_recv_pspoll(struct ieee80211com *ic, struct mbuf *m0, int rssi,
    u_int32_t rstamp)
{
	struct ifnet *ifp = &ic->ic_if;
	struct ieee80211_frame *wh;
	struct ieee80211_node *ni;
	struct mbuf *m;
	u_int16_t aid;

	if (ic->ic_set_tim == NULL)  /* No powersaving functionality */
		return;

	wh = mtod(m0, struct ieee80211_frame *);

	if ((ni = ieee80211_find_node(ic, wh->i_addr2)) == NULL) {
		if (ifp->if_flags & IFF_DEBUG)
			printf("%s: station %s sent bogus power save poll\n",
			    ifp->if_xname, ether_sprintf(wh->i_addr2));
		return;
	}

	memcpy(&aid, wh->i_dur, sizeof(wh->i_dur));
	if ((aid & 0xc000) != 0xc000) {
		if (ifp->if_flags & IFF_DEBUG)
			printf("%s: station %s sent bogus aid %x\n",
			    ifp->if_xname, ether_sprintf(wh->i_addr2), aid);
		return;
	}

	if (aid != ni->ni_associd) {
		if (ifp->if_flags & IFF_DEBUG)
			printf("%s: station %s aid %x doesn't match pspoll "
			    "aid %x\n", ifp->if_xname,
			    ether_sprintf(wh->i_addr2), ni->ni_associd, aid);
		return;
	}

	/* Okay, take the first queued packet and put it out... */

	IF_DEQUEUE(&ni->ni_savedq, m);
	if (m == NULL) {
		if (ifp->if_flags & IFF_DEBUG)
			printf("%s: station %s sent pspoll, "
			    "but no packets are saved\n",
			    ifp->if_xname, ether_sprintf(wh->i_addr2));
		return;
	}
	wh = mtod(m, struct ieee80211_frame *);

	/*
	 * If this is the last packet, turn off the TIM fields.
	 * If there are more packets, set the more packets bit.
	 */

	if (IF_IS_EMPTY(&ni->ni_savedq)) {
		if (ic->ic_set_tim)
			ic->ic_set_tim(ic, ni->ni_associd, 0);
	} else {
		wh->i_fc[1] |= IEEE80211_FC1_MORE_DATA;
	}

	if (ifp->if_flags & IFF_DEBUG)
		printf("%s: enqueued power saving packet for station %s\n",
		    ifp->if_xname, ether_sprintf(ni->ni_macaddr));

	IF_ENQUEUE(&ic->ic_pwrsaveq, m);
	(*ifp->if_start)(ifp);
}

static int
ieee80211_do_slow_print(struct ieee80211com *ic, int *did_print)
{
	if ((ic->ic_if.if_flags & IFF_LINK0) == 0)
		return 0;
	if (!*did_print && (ic->ic_if.if_flags & IFF_DEBUG) == 0 &&
	    !ratecheck(&ic->ic_last_merge_print, &ieee80211_merge_print_intvl))
		return 0;

	*did_print = 1;
	return 1;
}

/* ieee80211_ibss_merge helps merge 802.11 ad hoc networks.  The
 * convention, set by the Wireless Ethernet Compatibility Alliance
 * (WECA), is that an 802.11 station will change its BSSID to match
 * the "oldest" 802.11 ad hoc network, on the same channel, that
 * has the station's desired SSID.  The "oldest" 802.11 network
 * sends beacons with the greatest TSF timestamp.
 *
 * Return ENETRESET if the BSSID changed, 0 otherwise.
 *
 * XXX Perhaps we should compensate for the time that elapses
 * between the MAC receiving the beacon and the host processing it
 * in ieee80211_ibss_merge.
 */
int
ieee80211_ibss_merge(struct ieee80211com *ic, struct ieee80211_node *ni,
    uint64_t local_tsft)
{
	uint64_t beacon_tsft;
	int did_print = 0, sign;
	union {
		uint64_t	word;
		uint8_t		tstamp[8];
	} u;

	/* ensure alignment */
	(void)memcpy(&u, &ni->ni_tstamp[0], sizeof(u));
	beacon_tsft = letoh64(u.word);

	/* we are faster, let the other guy catch up */
	if (beacon_tsft < local_tsft)
		sign = -1;
	else
		sign = 1;

	if (IEEE80211_ADDR_EQ(ni->ni_bssid, ic->ic_bss->ni_bssid)) {
		if (!ieee80211_do_slow_print(ic, &did_print))
			return 0;
		printf("%s: tsft offset %s%llu\n", ic->ic_if.if_xname,
		    (sign < 0) ? "-" : "",
		    (sign < 0)
			? (local_tsft - beacon_tsft)
			: (beacon_tsft - local_tsft));
		return 0;
	}

	if (sign < 0)
		return 0;

	if (ieee80211_match_bss(ic, ni) != 0)
		return 0;

	if (ieee80211_do_slow_print(ic, &did_print)) {
		printf("%s: ieee80211_ibss_merge: bssid mismatch %s\n",
		    ic->ic_if.if_xname, ether_sprintf(ni->ni_bssid));
		printf("%s: my tsft %llu beacon tsft %llu\n",
		    ic->ic_if.if_xname, local_tsft, beacon_tsft);
		printf("%s: sync TSF with %s\n",
		    ic->ic_if.if_xname, ether_sprintf(ni->ni_macaddr));
	}

	ic->ic_flags &= ~IEEE80211_F_SIBSS;

	/* negotiate rates with new IBSS */
	ieee80211_fix_rate(ic, ni, IEEE80211_F_DOFRATE |
	    IEEE80211_F_DONEGO | IEEE80211_F_DODEL);
	if (ni->ni_rates.rs_nrates == 0) {
		if (ieee80211_do_slow_print(ic, &did_print)) {
			printf("%s: rates mismatch, BSSID %s\n",
			    ic->ic_if.if_xname, ether_sprintf(ni->ni_bssid));
		}
		return 0;
	}

	if (ieee80211_do_slow_print(ic, &did_print)) {
		printf("%s: sync BSSID %s -> ",
		    ic->ic_if.if_xname, ether_sprintf(ic->ic_bss->ni_bssid));
		printf("%s ", ether_sprintf(ni->ni_bssid));
		printf("(from %s)\n", ether_sprintf(ni->ni_macaddr));
	}

	ieee80211_node_newstate(ni, IEEE80211_STA_BSS);
	(*ic->ic_node_copy)(ic, ic->ic_bss, ni);

	return ENETRESET;
}
=======
/*
 * Process a BlockAckReq for a specific TID (see 9.10.7.6.3).
 * This is the common back-end for all BlockAckReq frame variants.
 */
void
ieee80211_bar_tid(struct ieee80211com *ic, struct ieee80211_node *ni,
    u_int8_t tid, u_int16_t ssn)
{
	struct ieee80211_rx_ba *ba = &ni->ni_rx_ba[tid];

	/* check if we have a Block Ack agreement for RA/TID */
	if (ba->ba_state != IEEE80211_BA_AGREED) {
		/* XXX not sure in PBAC case */
		/* send a DELBA with reason code UNKNOWN-BA */
		IEEE80211_SEND_ACTION(ic, ni, IEEE80211_CATEG_BA,
		    IEEE80211_ACTION_DELBA,
		    IEEE80211_REASON_SETUP_REQUIRED << 16 | tid);
		return;
	}
	/* check if it is a Protected Block Ack agreement */
	if ((ni->ni_flags & IEEE80211_NODE_MFP) &&
	    (ni->ni_rsncaps & IEEE80211_RSNCAP_PBAC)) {
		/* ADDBA Requests must be used in PBAC case */
		if (SEQ_LT(ssn, ba->ba_winstart) ||
		    SEQ_LT(ba->ba_winend, ssn))
			ic->ic_stats.is_pbac_errs++;
		return;	/* PBAC, do not move window */
	}
	/* reset Block Ack inactivity timer */
	timeout_add_usec(&ba->ba_to, ba->ba_timeout_val);

	if (SEQ_LT(ba->ba_winstart, ssn))
		ieee80211_ba_move_window(ic, ni, tid, ssn);
}
#endif	/* !IEEE80211_NO_HT */
>>>>>>> origin/master
