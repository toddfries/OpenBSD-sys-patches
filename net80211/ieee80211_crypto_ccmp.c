/*	$OpenBSD: ieee80211_crypto_ccmp.c,v 1.6 2008/08/12 17:54:57 damien Exp $	*/

/*-
 * Copyright (c) 2008 Damien Bergamini <damien.bergamini@free.fr>
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
 * This code implements the CTR with CBC-MAC protocol (CCMP) defined in
 * IEEE Std 802.11-2007 section 8.3.3.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/endian.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_arp.h>
#include <net/if_llc.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_ether.h>
#endif

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_crypto.h>

#include <crypto/rijndael.h>

/* CCMP software crypto context */
struct ieee80211_ccmp_ctx {
	rijndael_ctx	rijndael;
};

/*
 * Initialize software crypto context.  This function can be overridden
 * by drivers doing hardware crypto.
 */
int
ieee80211_ccmp_set_key(struct ieee80211com *ic, struct ieee80211_key *k)
{
	struct ieee80211_ccmp_ctx *ctx;

	ctx = malloc(sizeof(*ctx), M_DEVBUF, M_NOWAIT | M_ZERO);
	if (ctx == NULL)
		return ENOMEM;
	rijndael_set_key_enc_only(&ctx->rijndael, k->k_key, 128);
	k->k_priv = ctx;
	return 0;
}

void
ieee80211_ccmp_delete_key(struct ieee80211com *ic, struct ieee80211_key *k)
{
	if (k->k_priv != NULL)
		free(k->k_priv, M_DEVBUF);
	k->k_priv = NULL;
}

/*-
 * Counter with CBC-MAC (CCM) - see RFC3610.
 * CCMP uses the following CCM parameters: M = 8, L = 2
 */
static void
ieee80211_ccmp_phase1(rijndael_ctx *ctx, const struct ieee80211_frame *wh,
    u_int64_t pn, int lm, u_int8_t b[16], u_int8_t a[16], u_int8_t s0[16])
{
	u_int8_t auth[32], nonce[13];
	u_int8_t *aad;
	u_int8_t tid = 0;
	int la, i;

	/* construct AAD (additional authenticated data) */
	aad = &auth[2];	/* skip l(a), will be filled later */
	*aad = wh->i_fc[0];
	/* 11w: conditionnally mask subtype field */
	if ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) ==
	    IEEE80211_FC0_TYPE_DATA)
		*aad &= ~IEEE80211_FC0_SUBTYPE_MASK;
	aad++;
	/* protected bit is already set in wh */
	*aad = wh->i_fc[1];
	*aad &= ~(IEEE80211_FC1_RETRY | IEEE80211_FC1_PWR_MGT |
	    IEEE80211_FC1_MORE_DATA);
	/* 11n: conditionnally mask order bit */
	if ((wh->i_fc[0] &
	    (IEEE80211_FC0_TYPE_MASK | IEEE80211_FC0_SUBTYPE_QOS)) ==
	    (IEEE80211_FC0_TYPE_DATA | IEEE80211_FC0_SUBTYPE_QOS))
		*aad &= ~IEEE80211_FC1_ORDER;
	aad++;
	IEEE80211_ADDR_COPY(aad, wh->i_addr1); aad += IEEE80211_ADDR_LEN;
	IEEE80211_ADDR_COPY(aad, wh->i_addr2); aad += IEEE80211_ADDR_LEN;
	IEEE80211_ADDR_COPY(aad, wh->i_addr3); aad += IEEE80211_ADDR_LEN;
	*aad++ = wh->i_seq[0] & ~0xf0;
	*aad++ = 0;
	if ((wh->i_fc[1] & IEEE80211_FC1_DIR_MASK) ==
	    IEEE80211_FC1_DIR_DSTODS) {
		const struct ieee80211_frame_addr4 *wh4 =
		    (const struct ieee80211_frame_addr4 *)wh;
		IEEE80211_ADDR_COPY(aad, wh4->i_addr4);
		aad += IEEE80211_ADDR_LEN;
		if ((wh->i_fc[0] &
		    (IEEE80211_FC0_TYPE_MASK | IEEE80211_FC0_SUBTYPE_QOS)) ==
		    (IEEE80211_FC0_TYPE_DATA | IEEE80211_FC0_SUBTYPE_QOS)) {
			const struct ieee80211_qosframe_addr4 *qwh4 =
			    (const struct ieee80211_qosframe_addr4 *)wh;
			*aad++ = tid = qwh4->i_qos[0] & 0x0f;
			*aad++ = 0;
		}
	} else if ((wh->i_fc[0] &
	    (IEEE80211_FC0_TYPE_MASK | IEEE80211_FC0_SUBTYPE_QOS)) ==
	    (IEEE80211_FC0_TYPE_DATA | IEEE80211_FC0_SUBTYPE_QOS)) {
		const struct ieee80211_qosframe *qwh =
		    (const struct ieee80211_qosframe *)wh;
		*aad++ = tid = qwh->i_qos[0] & 0x0f;
		*aad++ = 0;
	}

	/* construct CCM nonce */
	nonce[ 0] = tid;
	if ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) ==
	    IEEE80211_FC0_TYPE_MGT)
		nonce[0] |= 1 << 4;	/* 11w: set management bit */
	IEEE80211_ADDR_COPY(&nonce[1], wh->i_addr2);
	nonce[ 7] = pn >> 40;	/* PN5 */
	nonce[ 8] = pn >> 32;	/* PN4 */
	nonce[ 9] = pn >> 24;	/* PN3 */
	nonce[10] = pn >> 16;	/* PN2 */
	nonce[11] = pn >> 8;	/* PN1 */
	nonce[12] = pn;		/* PN0 */

	/* add 2 authentication blocks (including l(a) and padded AAD) */
	la = aad - &auth[2];		/* fill l(a) */
	auth[0] = la >> 8;
	auth[1] = la & 0xff;
	memset(aad, 0, 30 - la);	/* pad AAD with zeros */

	/* construct first block B_0 */
	b[ 0] = 89;	/* Flags = 64*Adata + 8*((M-2)/2) + (L-1) */
	memcpy(&b[1], nonce, 13);
	b[14] = lm >> 8;
	b[15] = lm & 0xff;
	rijndael_encrypt(ctx, b, b);

	for (i = 0; i < 16; i++)
		b[i] ^= auth[i];
	rijndael_encrypt(ctx, b, b);
	for (i = 0; i < 16; i++)
		b[i] ^= auth[16 + i];
	rijndael_encrypt(ctx, b, b);

	/* construct S_0 */
	a[ 0] = 1;	/* Flags = L' = (L-1) */
	memcpy(&a[1], nonce, 13);
	a[14] = a[15] = 0;
	rijndael_encrypt(ctx, a, s0);
}

struct mbuf *
ieee80211_ccmp_encrypt(struct ieee80211com *ic, struct mbuf *m0,
    struct ieee80211_key *k)
{
	struct ieee80211_ccmp_ctx *ctx = k->k_priv;
	const struct ieee80211_frame *wh;
	const u_int8_t *src;
	u_int8_t *ivp, *mic, *dst;
	u_int8_t a[16], b[16], s0[16], s[16];
	struct mbuf *n0, *m, *n;
	int hdrlen, left, moff, noff, len;
	u_int16_t ctr;
	int i, j;

	MGET(n0, M_DONTWAIT, m0->m_type);
	if (n0 == NULL)
		goto nospace;
	M_DUP_PKTHDR(n0, m0);
	n0->m_pkthdr.len += IEEE80211_CCMP_HDRLEN;
	n0->m_len = MHLEN;
	if (n0->m_pkthdr.len >= MINCLSIZE - IEEE80211_CCMP_MICLEN) {
		MCLGET(n0, M_DONTWAIT);
		if (n0->m_flags & M_EXT)
			n0->m_len = n0->m_ext.ext_size;
	}
	if (n0->m_len > n0->m_pkthdr.len)
		n0->m_len = n0->m_pkthdr.len;

	/* copy 802.11 header */
	wh = mtod(m0, struct ieee80211_frame *);
	hdrlen = ieee80211_get_hdrlen(wh);
	memcpy(mtod(n0, caddr_t), wh, hdrlen);

	k->k_tsc++;	/* increment the 48-bit PN */

	/* construct CCMP header */
	ivp = mtod(n0, u_int8_t *) + hdrlen;
	ivp[0] = k->k_tsc;		/* PN0 */
	ivp[1] = k->k_tsc >> 8;		/* PN1 */
	ivp[2] = 0;			/* Rsvd */
	ivp[3] = k->k_id << 6 | IEEE80211_WEP_EXTIV;	/* KeyID | ExtIV */
	ivp[4] = k->k_tsc >> 16;	/* PN2 */
	ivp[5] = k->k_tsc >> 24;	/* PN3 */
	ivp[6] = k->k_tsc >> 32;	/* PN4 */
	ivp[7] = k->k_tsc >> 40;	/* PN5 */

	/* construct initial B, A and S_0 blocks */
	ieee80211_ccmp_phase1(&ctx->rijndael, wh, k->k_tsc,
	    m0->m_pkthdr.len - hdrlen, b, a, s0);

	/* construct S_1 */
	ctr = 1;
	a[14] = ctr >> 8;
	a[15] = ctr & 0xff;
	rijndael_encrypt(&ctx->rijndael, a, s);

	/* encrypt frame body and compute MIC */
	j = 0;
	m = m0;
	n = n0;
	moff = hdrlen;
	noff = hdrlen + IEEE80211_CCMP_HDRLEN;
	left = m0->m_pkthdr.len - moff;
	while (left > 0) {
		if (moff == m->m_len) {
			/* nothing left to copy from m */
			m = m->m_next;
			moff = 0;
		}
		if (noff == n->m_len) {
			/* n is full and there's more data to copy */
			MGET(n->m_next, M_DONTWAIT, n->m_type);
			if (n->m_next == NULL)
				goto nospace;
			n = n->m_next;
			n->m_len = MLEN;
			if (left >= MINCLSIZE - IEEE80211_CCMP_MICLEN) {
				MCLGET(n, M_DONTWAIT);
				if (n->m_flags & M_EXT)
					n->m_len = n->m_ext.ext_size;
			}
			if (n->m_len > left)
				n->m_len = left;
			noff = 0;
		}
		len = min(m->m_len - moff, n->m_len - noff);

		src = mtod(m, u_int8_t *) + moff;
		dst = mtod(n, u_int8_t *) + noff;
		for (i = 0; i < len; i++) {
			/* update MIC with clear text */
			b[j] ^= src[i];
			/* encrypt message */
			dst[i] = src[i] ^ s[j];
			if (++j < 16)
				continue;
			/* we have a full block, encrypt MIC */
			rijndael_encrypt(&ctx->rijndael, b, b);
			/* construct a new S_ctr block */
			ctr++;
			a[14] = ctr >> 8;
			a[15] = ctr & 0xff;
			rijndael_encrypt(&ctx->rijndael, a, s);
			j = 0;
		}

		moff += len;
		noff += len;
		left -= len;
	}
	if (j != 0)	/* partial block, encrypt MIC */
		rijndael_encrypt(&ctx->rijndael, b, b);

	/* reserve trailing space for MIC */
	if (M_TRAILINGSPACE(n) < IEEE80211_CCMP_MICLEN) {
		MGET(n->m_next, M_DONTWAIT, n->m_type);
		if (n->m_next == NULL)
			goto nospace;
		n = n->m_next;
		n->m_len = 0;
	}
	/* finalize MIC, U := T XOR first-M-bytes( S_O ) */
	mic = mtod(n, u_int8_t *) + n->m_len;
	for (i = 0; i < IEEE80211_CCMP_MICLEN; i++)
		mic[i] = b[i] ^ s0[i];
	n->m_len += IEEE80211_CCMP_MICLEN;
	n0->m_pkthdr.len += IEEE80211_CCMP_MICLEN;

	m_freem(m0);
	return n0;
 nospace:
	ic->ic_stats.is_tx_nombuf++;
	m_freem(m0);
	if (n0 != NULL)
		m_freem(n0);
	return NULL;
}

struct mbuf *
ieee80211_ccmp_decrypt(struct ieee80211com *ic, struct mbuf *m0,
    struct ieee80211_key *k)
{
	struct ieee80211_ccmp_ctx *ctx = k->k_priv;
	struct ieee80211_frame *wh;
	u_int64_t pn, *prsc;
	const u_int8_t *ivp, *src;
	u_int8_t *dst;
	u_int8_t mic0[IEEE80211_CCMP_MICLEN];
	u_int8_t a[16], b[16], s0[16], s[16];
	struct mbuf *n0, *m, *n;
	int hdrlen, left, moff, noff, len;
	u_int16_t ctr;
	int i, j;

	wh = mtod(m0, struct ieee80211_frame *);
	hdrlen = ieee80211_get_hdrlen(wh);
	ivp = (u_int8_t *)wh + hdrlen;

	if (m0->m_pkthdr.len < hdrlen + IEEE80211_CCMP_HDRLEN +
	    IEEE80211_CCMP_MICLEN) {
		m_freem(m0);
		return NULL;
	}
	/* check that ExtIV bit is be set */
	if (!(ivp[3] & IEEE80211_WEP_EXTIV)) {
		m_freem(m0);
		return NULL;
	}

	/* retrieve last seen packet number for this frame type/TID */
	if ((wh->i_fc[0] &
	    (IEEE80211_FC0_TYPE_MASK | IEEE80211_FC0_SUBTYPE_QOS)) ==
	    (IEEE80211_FC0_TYPE_DATA | IEEE80211_FC0_SUBTYPE_QOS)) {
		u_int8_t tid;
		if ((wh->i_fc[1] & IEEE80211_FC1_DIR_MASK) ==
		    IEEE80211_FC1_DIR_DSTODS) {
			struct ieee80211_qosframe_addr4 *qwh4 =
			    (struct ieee80211_qosframe_addr4 *)wh;
			tid = qwh4->i_qos[0] & 0x0f;
		} else {
			struct ieee80211_qosframe *qwh =
			    (struct ieee80211_qosframe *)wh;
			tid = qwh->i_qos[0] & 0x0f;
		}
		prsc = &k->k_rsc[tid];
	} else if ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) !=
	    IEEE80211_FC0_TYPE_MGT) {
		prsc = &k->k_rsc[0];
	} else	/* 11w: management frames have their own counters */
		prsc = &k->k_mgmt_rsc;

	/* extract the 48-bit PN from the CCMP header */
	pn = (u_int64_t)ivp[0]       |
	     (u_int64_t)ivp[1] <<  8 |
	     (u_int64_t)ivp[4] << 16 |
	     (u_int64_t)ivp[5] << 24 |
	     (u_int64_t)ivp[6] << 32 |
	     (u_int64_t)ivp[7] << 40;
	if (pn <= *prsc) {
		/* replayed frame, discard */
		ic->ic_stats.is_ccmp_replays++;
		m_freem(m0);
		return NULL;
	}

	MGET(n0, M_DONTWAIT, m0->m_type);
	if (n0 == NULL)
		goto nospace;
	M_DUP_PKTHDR(n0, m0);
	n0->m_pkthdr.len -= IEEE80211_CCMP_HDRLEN + IEEE80211_CCMP_MICLEN;
	n0->m_len = MHLEN;
	if (n0->m_pkthdr.len >= MINCLSIZE) {
		MCLGET(n0, M_DONTWAIT);
		if (n0->m_flags & M_EXT)
			n0->m_len = n0->m_ext.ext_size;
	}
	if (n0->m_len > n0->m_pkthdr.len)
		n0->m_len = n0->m_pkthdr.len;

	/* construct initial B, A and S_0 blocks */
	ieee80211_ccmp_phase1(&ctx->rijndael, wh, pn,
	    n0->m_pkthdr.len - hdrlen, b, a, s0);

	/* copy 802.11 header and clear protected bit */
	memcpy(mtod(n0, caddr_t), wh, hdrlen);
	wh = mtod(n0, struct ieee80211_frame *);
	wh->i_fc[1] &= ~IEEE80211_FC1_PROTECTED;

	/* construct S_1 */
	ctr = 1;
	a[14] = ctr >> 8;
	a[15] = ctr & 0xff;
	rijndael_encrypt(&ctx->rijndael, a, s);

	/* decrypt frame body and compute MIC */
	j = 0;
	m = m0;
	n = n0;
	moff = hdrlen + IEEE80211_CCMP_HDRLEN;
	noff = hdrlen;
	left = n0->m_pkthdr.len - noff;
	while (left > 0) {
		if (moff == m->m_len) {
			/* nothing left to copy from m */
			m = m->m_next;
			moff = 0;
		}
		if (noff == n->m_len) {
			/* n is full and there's more data to copy */
			MGET(n->m_next, M_DONTWAIT, n->m_type);
			if (n->m_next == NULL)
				goto nospace;
			n = n->m_next;
			n->m_len = MLEN;
			if (left >= MINCLSIZE) {
				MCLGET(n, M_DONTWAIT);
				if (n->m_flags & M_EXT)
					n->m_len = n->m_ext.ext_size;
			}
			if (n->m_len > left)
				n->m_len = left;
			noff = 0;
		}
		len = min(m->m_len - moff, n->m_len - noff);

		src = mtod(m, u_int8_t *) + moff;
		dst = mtod(n, u_int8_t *) + noff;
		for (i = 0; i < len; i++) {
			/* decrypt message */
			dst[i] = src[i] ^ s[j];
			/* update MIC with clear text */
			b[j] ^= dst[i];
			if (++j < 16)
				continue;
			/* we have a full block, encrypt MIC */
			rijndael_encrypt(&ctx->rijndael, b, b);
			/* construct a new S_ctr block */
			ctr++;
			a[14] = ctr >> 8;
			a[15] = ctr & 0xff;
			rijndael_encrypt(&ctx->rijndael, a, s);
			j = 0;
		}

		moff += len;
		noff += len;
		left -= len;
	}
	if (j != 0)	/* partial block, encrypt MIC */
		rijndael_encrypt(&ctx->rijndael, b, b);

	/* finalize MIC, U := T XOR first-M-bytes( S_O ) */
	for (i = 0; i < IEEE80211_CCMP_MICLEN; i++)
		b[i] ^= s0[i];

	/* check that it matches the MIC in received frame */
	m_copydata(m, moff, IEEE80211_CCMP_MICLEN, mic0);
	if (memcmp(mic0, b, IEEE80211_CCMP_MICLEN) != 0) {
		ic->ic_stats.is_ccmp_dec_errs++;
		m_freem(m0);
		m_freem(n0);
		return NULL;
	}

	/* update last seen packet number (MIC is validated) */
	*prsc = pn;

	m_freem(m0);
	return n0;
 nospace:
	ic->ic_stats.is_rx_nombuf++;
	m_freem(m0);
	if (n0 != NULL)
		m_freem(n0);
	return NULL;
}
