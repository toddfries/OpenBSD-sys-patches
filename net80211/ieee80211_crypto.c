<<<<<<< HEAD
/*	$OpenBSD: ieee80211_crypto.c,v 1.8 2006/06/18 18:39:41 damien Exp $	*/
/*	$NetBSD: ieee80211_crypto.c,v 1.5 2003/12/14 09:56:53 dyoung Exp $	*/
=======
/*	$OpenBSD: ieee80211_crypto.c,v 1.60 2011/01/11 15:42:05 deraadt Exp $	*/
>>>>>>> origin/master

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
#include <net80211/ieee80211_priv.h>

#include <dev/rndvar.h>
#include <crypto/arc4.h>
<<<<<<< HEAD
#define	arc4_ctxlen()			sizeof (struct rc4_ctx)
#define	arc4_setkey(_c,_k,_l)		rc4_keysetup(_c,_k,_l)
#define	arc4_encrypt(_c,_d,_s,_l)	rc4_crypt(_c,_s,_d,_l)

static	void ieee80211_crc_init(void);
static	u_int32_t ieee80211_crc_update(u_int32_t crc, u_int8_t *buf, int len);
=======
#include <crypto/md5.h>
#include <crypto/sha1.h>
#include <crypto/sha2.h>
#include <crypto/hmac.h>
#include <crypto/rijndael.h>
#include <crypto/cmac.h>
#include <crypto/key_wrap.h>

void	ieee80211_prf(const u_int8_t *, size_t, const u_int8_t *, size_t,
	    const u_int8_t *, size_t, u_int8_t *, size_t);
void	ieee80211_kdf(const u_int8_t *, size_t, const u_int8_t *, size_t,
	    const u_int8_t *, size_t, u_int8_t *, size_t);
void	ieee80211_derive_pmkid(enum ieee80211_akm, const u_int8_t *,
	    const u_int8_t *, const u_int8_t *, u_int8_t *);
>>>>>>> origin/master

void
ieee80211_crypto_attach(struct ifnet *ifp)
{
<<<<<<< HEAD
	/*
	 * Setup crypto support.
	 */
	ieee80211_crc_init();
=======
	struct ieee80211com *ic = (void *)ifp;

	TAILQ_INIT(&ic->ic_pmksa);
	if (ic->ic_caps & IEEE80211_C_RSN) {
		ic->ic_rsnprotos = IEEE80211_PROTO_WPA | IEEE80211_PROTO_RSN;
		ic->ic_rsnakms = IEEE80211_AKM_PSK;
		ic->ic_rsnciphers = IEEE80211_CIPHER_TKIP |
		    IEEE80211_CIPHER_CCMP;
		ic->ic_rsngroupcipher = IEEE80211_CIPHER_TKIP;
		ic->ic_rsngroupmgmtcipher = IEEE80211_CIPHER_BIP;
	}
	ic->ic_set_key = ieee80211_set_key;
	ic->ic_delete_key = ieee80211_delete_key;
>>>>>>> origin/master
}

void
ieee80211_crypto_detach(struct ifnet *ifp)
{
	struct ieee80211com *ic = (void *)ifp;
<<<<<<< HEAD

	if (ic->ic_wep_ctx != NULL) {
		free(ic->ic_wep_ctx, M_DEVBUF);
		ic->ic_wep_ctx = NULL;
=======
	struct ieee80211_pmk *pmk;
	int i;

	/* purge the PMKSA cache */
	while ((pmk = TAILQ_FIRST(&ic->ic_pmksa)) != NULL) {
		TAILQ_REMOVE(&ic->ic_pmksa, pmk, pmk_next);
		explicit_bzero(pmk, sizeof(*pmk));
		free(pmk, M_DEVBUF);
	}

	/* clear all group keys from memory */
	for (i = 0; i < IEEE80211_GROUP_NKID; i++) {
		struct ieee80211_key *k = &ic->ic_nw_keys[i];
		if (k->k_cipher != IEEE80211_CIPHER_NONE)
			(*ic->ic_delete_key)(ic, NULL, k);
		explicit_bzero(k, sizeof(*k));
	}

	/* clear pre-shared key from memory */
	explicit_bzero(ic->ic_psk, IEEE80211_PMK_LEN);
}

/*
 * Return the length in bytes of a cipher suite key (see Table 60).
 */
int
ieee80211_cipher_keylen(enum ieee80211_cipher cipher)
{
	switch (cipher) {
	case IEEE80211_CIPHER_WEP40:
		return 5;
	case IEEE80211_CIPHER_TKIP:
		return 32;
	case IEEE80211_CIPHER_CCMP:
		return 16;
	case IEEE80211_CIPHER_WEP104:
		return 13;
	case IEEE80211_CIPHER_BIP:
		return 16;
	default:	/* unknown cipher */
		return 0;
	}
}

int
ieee80211_set_key(struct ieee80211com *ic, struct ieee80211_node *ni,
    struct ieee80211_key *k)
{
	int error;

	switch (k->k_cipher) {
	case IEEE80211_CIPHER_WEP40:
	case IEEE80211_CIPHER_WEP104:
		error = ieee80211_wep_set_key(ic, k);
		break;
	case IEEE80211_CIPHER_TKIP:
		error = ieee80211_tkip_set_key(ic, k);
		break;
	case IEEE80211_CIPHER_CCMP:
		error = ieee80211_ccmp_set_key(ic, k);
		break;
	case IEEE80211_CIPHER_BIP:
		error = ieee80211_bip_set_key(ic, k);
		break;
	default:
		/* should not get there */
		error = EINVAL;
>>>>>>> origin/master
	}
}

<<<<<<< HEAD
/* Round up to a multiple of IEEE80211_WEP_KEYLEN + IEEE80211_WEP_IVLEN */
#define klen_round(x)							\
	(((x) + (IEEE80211_WEP_KEYLEN + IEEE80211_WEP_IVLEN - 1)) &	\
	~(IEEE80211_WEP_KEYLEN + IEEE80211_WEP_IVLEN - 1))

struct mbuf *
ieee80211_wep_crypt(struct ifnet *ifp, struct mbuf *m0, int txflag)
=======
void
ieee80211_delete_key(struct ieee80211com *ic, struct ieee80211_node *ni,
    struct ieee80211_key *k)
{
	switch (k->k_cipher) {
	case IEEE80211_CIPHER_WEP40:
	case IEEE80211_CIPHER_WEP104:
		ieee80211_wep_delete_key(ic, k);
		break;
	case IEEE80211_CIPHER_TKIP:
		ieee80211_tkip_delete_key(ic, k);
		break;
	case IEEE80211_CIPHER_CCMP:
		ieee80211_ccmp_delete_key(ic, k);
		break;
	case IEEE80211_CIPHER_BIP:
		ieee80211_bip_delete_key(ic, k);
		break;
	default:
		/* should not get there */
		break;
	}
	explicit_bzero(k, sizeof(*k));
}

struct ieee80211_key *
ieee80211_get_txkey(struct ieee80211com *ic, const struct ieee80211_frame *wh,
    struct ieee80211_node *ni)
{
	int kid;

	if ((ic->ic_flags & IEEE80211_F_RSNON) &&
	    !IEEE80211_IS_MULTICAST(wh->i_addr1) &&
	    ni->ni_rsncipher != IEEE80211_CIPHER_USEGROUP)
		return &ni->ni_pairwise_key;

	if (!IEEE80211_IS_MULTICAST(wh->i_addr1) ||
	    (wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) !=
	    IEEE80211_FC0_TYPE_MGT)
		kid = ic->ic_def_txkey;
	else
		kid = ic->ic_igtk_kid;
	return &ic->ic_nw_keys[kid];
}

struct mbuf *
ieee80211_encrypt(struct ieee80211com *ic, struct mbuf *m0,
    struct ieee80211_key *k)
{
	switch (k->k_cipher) {
	case IEEE80211_CIPHER_WEP40:
	case IEEE80211_CIPHER_WEP104:
		m0 = ieee80211_wep_encrypt(ic, m0, k);
		break;
	case IEEE80211_CIPHER_TKIP:
		m0 = ieee80211_tkip_encrypt(ic, m0, k);
		break;
	case IEEE80211_CIPHER_CCMP:
		m0 = ieee80211_ccmp_encrypt(ic, m0, k);
		break;
	case IEEE80211_CIPHER_BIP:
		m0 = ieee80211_bip_encap(ic, m0, k);
		break;
	default:
		/* should not get there */
		m_freem(m0);
		m0 = NULL;
	}
	return m0;
}

struct mbuf *
ieee80211_decrypt(struct ieee80211com *ic, struct mbuf *m0,
    struct ieee80211_node *ni)
>>>>>>> origin/master
{
	struct ieee80211com *ic = (void *)ifp;
	struct mbuf *m, *n, *n0;
	struct ieee80211_frame *wh;
<<<<<<< HEAD
	int i, left, len, moff, noff, kid;
	u_int32_t iv, crc;
	u_int8_t *ivp;
	void *ctx;
	u_int8_t keybuf[klen_round(IEEE80211_WEP_IVLEN + IEEE80211_KEYBUF_SIZE)];
	u_int8_t crcbuf[IEEE80211_WEP_CRCLEN];

	n0 = NULL;
	if ((ctx = ic->ic_wep_ctx) == NULL) {
		ctx = malloc(arc4_ctxlen(), M_DEVBUF, M_NOWAIT);
		if (ctx == NULL) {
			ic->ic_stats.is_crypto_nomem++;
			goto fail;
		}
		ic->ic_wep_ctx = ctx;
	}
	m = m0;
	left = m->m_pkthdr.len;
	MGET(n, M_DONTWAIT, m->m_type);
	n0 = n;
	if (n == NULL) {
		if (txflag)
			ic->ic_stats.is_tx_nombuf++;
		else
			ic->ic_stats.is_rx_nombuf++;
		goto fail;
	}
	M_DUP_PKTHDR(n, m);
	len = IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN + IEEE80211_WEP_CRCLEN;
	if (txflag) {
		n->m_pkthdr.len += len;
	} else {
		n->m_pkthdr.len -= len;
		left -= len;
	}
	n->m_len = MHLEN;
	if (n->m_pkthdr.len >= MINCLSIZE) {
		MCLGET(n, M_DONTWAIT);
		if (n->m_flags & M_EXT)
			n->m_len = n->m_ext.ext_size;
	}
	len = sizeof(struct ieee80211_frame);
	memcpy(mtod(n, caddr_t), mtod(m, caddr_t), len);
	wh = mtod(n, struct ieee80211_frame *);
	left -= len;
	moff = len;
	noff = len;
	if (txflag) {
		kid = ic->ic_wep_txkey;
		wh->i_fc[1] |= IEEE80211_FC1_WEP;
		iv = ic->ic_iv ? ic->ic_iv : arc4random();
		/*
		 * Skip 'bad' IVs from Fluhrer/Mantin/Shamir:
		 * (B, 255, N) with 3 <= B < 8
		 */
		if (iv >= 0x03ff00 &&
		    (iv & 0xf8ff00) == 0x00ff00)
			iv += 0x000100;
		ic->ic_iv = iv + 1;
		/* put iv in little endian to prepare 802.11i */
		ivp = mtod(n, u_int8_t *) + noff;
		for (i = 0; i < IEEE80211_WEP_IVLEN; i++) {
			ivp[i] = iv & 0xff;
			iv >>= 8;
		}
		ivp[IEEE80211_WEP_IVLEN] = kid << 6;	/* pad and keyid */
		noff += IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN;
	} else {
		wh->i_fc[1] &= ~IEEE80211_FC1_WEP;
		ivp = mtod(m, u_int8_t *) + moff;
		kid = ivp[IEEE80211_WEP_IVLEN] >> 6;
		moff += IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN;
	}

	/*
	 * Copy the IV and the key material.  The input key has been padded
	 * with zeros by the ioctl.  The output key buffer length is rounded
	 * to a multiple of 64bit to allow variable length keys padded by
	 * zeros.
	 */
	bzero(&keybuf, sizeof(keybuf));
	memcpy(keybuf, ivp, IEEE80211_WEP_IVLEN);
	memcpy(keybuf + IEEE80211_WEP_IVLEN, ic->ic_nw_keys[kid].wk_key,
	    ic->ic_nw_keys[kid].wk_len);
	len = klen_round(IEEE80211_WEP_IVLEN + ic->ic_nw_keys[kid].wk_len);
	arc4_setkey(ctx, keybuf, len);

	/* encrypt with calculating CRC */
	crc = ~0;
	while (left > 0) {
		len = m->m_len - moff;
		if (len == 0) {
			m = m->m_next;
			moff = 0;
			continue;
		}
		if (len > n->m_len - noff) {
			len = n->m_len - noff;
			if (len == 0) {
				MGET(n->m_next, M_DONTWAIT, n->m_type);
				if (n->m_next == NULL) {
					if (txflag)
						ic->ic_stats.is_tx_nombuf++;
					else
						ic->ic_stats.is_rx_nombuf++;
					goto fail;
				}
				n = n->m_next;
				n->m_len = MLEN;
				if (left >= MINCLSIZE) {
					MCLGET(n, M_DONTWAIT);
					if (n->m_flags & M_EXT)
						n->m_len = n->m_ext.ext_size;
				}
				noff = 0;
				continue;
			}
		}
		if (len > left)
			len = left;
		arc4_encrypt(ctx, mtod(n, caddr_t) + noff,
		    mtod(m, caddr_t) + moff, len);
		if (txflag)
			crc = ieee80211_crc_update(crc,
			    mtod(m, u_int8_t *) + moff, len);
		else
			crc = ieee80211_crc_update(crc,
			    mtod(n, u_int8_t *) + noff, len);
		left -= len;
		moff += len;
		noff += len;
	}
	crc = ~crc;
	if (txflag) {
		*(u_int32_t *)crcbuf = htole32(crc);
		if (n->m_len >= noff + sizeof(crcbuf))
			n->m_len = noff + sizeof(crcbuf);
		else {
			n->m_len = noff;
			MGET(n->m_next, M_DONTWAIT, n->m_type);
			if (n->m_next == NULL) {
				ic->ic_stats.is_tx_nombuf++;
				goto fail;
			}
			n = n->m_next;
			n->m_len = sizeof(crcbuf);
			noff = 0;
		}
		arc4_encrypt(ctx, mtod(n, caddr_t) + noff, crcbuf,
		    sizeof(crcbuf));
	} else {
		n->m_len = noff;
		for (noff = 0; noff < sizeof(crcbuf); noff += len) {
			len = sizeof(crcbuf) - noff;
			if (len > m->m_len - moff)
				len = m->m_len - moff;
			if (len > 0)
				arc4_encrypt(ctx, crcbuf + noff,
				    mtod(m, caddr_t) + moff, len);
			m = m->m_next;
			moff = 0;
		}
		if (crc != letoh32(*(u_int32_t *)crcbuf)) {
#ifdef IEEE80211_DEBUG
			if (ieee80211_debug) {
				printf("%s: decrypt CRC error\n",
				    ifp->if_xname);
				if (ieee80211_debug > 1)
					ieee80211_dump_pkt(n0->m_data,
					    n0->m_len, -1, -1);
			}
#endif
			ic->ic_stats.is_rx_decryptcrc++;
			goto fail;
		}
	}
	m_freem(m0);
	return n0;

 fail:
	m_freem(m0);
	m_freem(n0);
	return NULL;
=======
	struct ieee80211_key *k;
	u_int8_t *ivp, *mmie;
	u_int16_t kid;
	int hdrlen;

	/* find key for decryption */
	wh = mtod(m0, struct ieee80211_frame *);
	if ((ic->ic_flags & IEEE80211_F_RSNON) &&
	    !IEEE80211_IS_MULTICAST(wh->i_addr1) &&
	    ni->ni_rsncipher != IEEE80211_CIPHER_USEGROUP) {
		k = &ni->ni_pairwise_key;

	} else if (!IEEE80211_IS_MULTICAST(wh->i_addr1) ||
	    (wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) !=
	    IEEE80211_FC0_TYPE_MGT) {
		/* retrieve group data key id from IV field */
		hdrlen = ieee80211_get_hdrlen(wh);
		/* check that IV field is present */
		if (m0->m_len < hdrlen + 4) {
			m_freem(m0);
			return NULL;
		}
		ivp = (u_int8_t *)wh + hdrlen;
		kid = ivp[3] >> 6;
		k = &ic->ic_nw_keys[kid];
	} else {
		/* retrieve integrity group key id from MMIE */
		if (m0->m_len < sizeof(*wh) + IEEE80211_MMIE_LEN) {
			m_freem(m0);
			return NULL;
		}
		/* it is assumed management frames are contiguous */
		mmie = (u_int8_t *)wh + m0->m_len - IEEE80211_MMIE_LEN;
		/* check that MMIE is valid */
		if (mmie[0] != IEEE80211_ELEMID_MMIE || mmie[1] != 16) {
			m_freem(m0);
			return NULL;
		}
		kid = LE_READ_2(&mmie[2]);
		if (kid != 4 && kid != 5) {
			m_freem(m0);
			return NULL;
		}
		k = &ic->ic_nw_keys[kid];
	}
	switch (k->k_cipher) {
	case IEEE80211_CIPHER_WEP40:
	case IEEE80211_CIPHER_WEP104:
		m0 = ieee80211_wep_decrypt(ic, m0, k);
		break;
	case IEEE80211_CIPHER_TKIP:
		m0 = ieee80211_tkip_decrypt(ic, m0, k);
		break;
	case IEEE80211_CIPHER_CCMP:
		m0 = ieee80211_ccmp_decrypt(ic, m0, k);
		break;
	case IEEE80211_CIPHER_BIP:
		m0 = ieee80211_bip_decap(ic, m0, k);
		break;
	default:
		/* key not defined */
		m_freem(m0);
		m0 = NULL;
	}
	return m0;
}

/*
 * SHA1-based Pseudo-Random Function (see 8.5.1.1).
 */
void
ieee80211_prf(const u_int8_t *key, size_t key_len, const u_int8_t *label,
    size_t label_len, const u_int8_t *context, size_t context_len,
    u_int8_t *output, size_t len)
{
	HMAC_SHA1_CTX ctx;
	u_int8_t digest[SHA1_DIGEST_LENGTH];
	u_int8_t count;

	for (count = 0; len != 0; count++) {
		HMAC_SHA1_Init(&ctx, key, key_len);
		HMAC_SHA1_Update(&ctx, label, label_len);
		HMAC_SHA1_Update(&ctx, context, context_len);
		HMAC_SHA1_Update(&ctx, &count, 1);
		if (len < SHA1_DIGEST_LENGTH) {
			HMAC_SHA1_Final(digest, &ctx);
			/* truncate HMAC-SHA1 to len bytes */
			memcpy(output, digest, len);
			break;
		}
		HMAC_SHA1_Final(output, &ctx);
		output += SHA1_DIGEST_LENGTH;
		len -= SHA1_DIGEST_LENGTH;
	}
}

/*
 * SHA256-based Key Derivation Function (see 8.5.1.5.2).
 */
void
ieee80211_kdf(const u_int8_t *key, size_t key_len, const u_int8_t *label,
    size_t label_len, const u_int8_t *context, size_t context_len,
    u_int8_t *output, size_t len)
{
	HMAC_SHA256_CTX ctx;
	u_int8_t digest[SHA256_DIGEST_LENGTH];
	u_int16_t i, iter, length;

	length = htole16(len * NBBY);
	for (i = 1; len != 0; i++) {
		HMAC_SHA256_Init(&ctx, key, key_len);
		iter = htole16(i);
		HMAC_SHA256_Update(&ctx, (u_int8_t *)&iter, sizeof iter);
		HMAC_SHA256_Update(&ctx, label, label_len);
		HMAC_SHA256_Update(&ctx, context, context_len);
		HMAC_SHA256_Update(&ctx, (u_int8_t *)&length, sizeof length);
		if (len < SHA256_DIGEST_LENGTH) {
			HMAC_SHA256_Final(digest, &ctx);
			/* truncate HMAC-SHA-256 to len bytes */
			memcpy(output, digest, len);
			break;
		}
		HMAC_SHA256_Final(output, &ctx);
		output += SHA256_DIGEST_LENGTH;
		len -= SHA256_DIGEST_LENGTH;
	}
}

/*
 * Derive Pairwise Transient Key (PTK) (see 8.5.1.2).
 */
void
ieee80211_derive_ptk(enum ieee80211_akm akm, const u_int8_t *pmk,
    const u_int8_t *aa, const u_int8_t *spa, const u_int8_t *anonce,
    const u_int8_t *snonce, struct ieee80211_ptk *ptk)
{
	void (*kdf)(const u_int8_t *, size_t, const u_int8_t *, size_t,
	    const u_int8_t *, size_t, u_int8_t *, size_t);
	u_int8_t buf[2 * IEEE80211_ADDR_LEN + 2 * EAPOL_KEY_NONCE_LEN];
	int ret;

	/* Min(AA,SPA) || Max(AA,SPA) */
	ret = memcmp(aa, spa, IEEE80211_ADDR_LEN) < 0;
	memcpy(&buf[ 0], ret ? aa : spa, IEEE80211_ADDR_LEN);
	memcpy(&buf[ 6], ret ? spa : aa, IEEE80211_ADDR_LEN);

	/* Min(ANonce,SNonce) || Max(ANonce,SNonce) */
	ret = memcmp(anonce, snonce, EAPOL_KEY_NONCE_LEN) < 0;
	memcpy(&buf[12], ret ? anonce : snonce, EAPOL_KEY_NONCE_LEN);
	memcpy(&buf[44], ret ? snonce : anonce, EAPOL_KEY_NONCE_LEN);

	kdf = ieee80211_is_sha256_akm(akm) ? ieee80211_kdf : ieee80211_prf;
	(*kdf)(pmk, IEEE80211_PMK_LEN, "Pairwise key expansion", 23,
	    buf, sizeof buf, (u_int8_t *)ptk, sizeof(*ptk));
}

static void
ieee80211_pmkid_sha1(const u_int8_t *pmk, const u_int8_t *aa,
    const u_int8_t *spa, u_int8_t *pmkid)
{
	HMAC_SHA1_CTX ctx;
	u_int8_t digest[SHA1_DIGEST_LENGTH];

	HMAC_SHA1_Init(&ctx, pmk, IEEE80211_PMK_LEN);
	HMAC_SHA1_Update(&ctx, "PMK Name", 8);
	HMAC_SHA1_Update(&ctx, aa, IEEE80211_ADDR_LEN);
	HMAC_SHA1_Update(&ctx, spa, IEEE80211_ADDR_LEN);
	HMAC_SHA1_Final(digest, &ctx);
	/* use the first 128 bits of HMAC-SHA1 */
	memcpy(pmkid, digest, IEEE80211_PMKID_LEN);
}

static void
ieee80211_pmkid_sha256(const u_int8_t *pmk, const u_int8_t *aa,
    const u_int8_t *spa, u_int8_t *pmkid)
{
	HMAC_SHA256_CTX ctx;
	u_int8_t digest[SHA256_DIGEST_LENGTH];

	HMAC_SHA256_Init(&ctx, pmk, IEEE80211_PMK_LEN);
	HMAC_SHA256_Update(&ctx, "PMK Name", 8);
	HMAC_SHA256_Update(&ctx, aa, IEEE80211_ADDR_LEN);
	HMAC_SHA256_Update(&ctx, spa, IEEE80211_ADDR_LEN);
	HMAC_SHA256_Final(digest, &ctx);
	/* use the first 128 bits of HMAC-SHA-256 */
	memcpy(pmkid, digest, IEEE80211_PMKID_LEN);
>>>>>>> origin/master
}

/*
 * CRC 32 -- routine from RFC 2083
 */
<<<<<<< HEAD

/* Table of CRCs of all 8-bit messages */
static u_int32_t ieee80211_crc_table[256];

/* Make the table for a fast CRC. */
static void
ieee80211_crc_init(void)
{
	u_int32_t c;
	int n, k;

	for (n = 0; n < 256; n++) {
		c = (u_int32_t)n;
		for (k = 0; k < 8; k++) {
			if (c & 1)
				c = 0xedb88320UL ^ (c >> 1);
			else
				c = c >> 1;
		}
		ieee80211_crc_table[n] = c;
=======
void
ieee80211_derive_pmkid(enum ieee80211_akm akm, const u_int8_t *pmk,
    const u_int8_t *aa, const u_int8_t *spa, u_int8_t *pmkid)
{
	if (ieee80211_is_sha256_akm(akm))
		ieee80211_pmkid_sha256(pmk, aa, spa, pmkid);
	else
		ieee80211_pmkid_sha1(pmk, aa, spa, pmkid);
}

typedef union _ANY_CTX {
	HMAC_MD5_CTX	md5;
	HMAC_SHA1_CTX	sha1;
	AES_CMAC_CTX	cmac;
} ANY_CTX;

/*
 * Compute the Key MIC field of an EAPOL-Key frame using the specified Key
 * Confirmation Key (KCK).  The hash function can be HMAC-MD5, HMAC-SHA1
 * or AES-128-CMAC depending on the EAPOL-Key Key Descriptor Version.
 */
void
ieee80211_eapol_key_mic(struct ieee80211_eapol_key *key, const u_int8_t *kck)
{
	u_int8_t digest[SHA1_DIGEST_LENGTH];
	ANY_CTX ctx;	/* XXX off stack? */
	u_int len;

	len = BE_READ_2(key->len) + 4;

	switch (BE_READ_2(key->info) & EAPOL_KEY_VERSION_MASK) {
	case EAPOL_KEY_DESC_V1:
		HMAC_MD5_Init(&ctx.md5, kck, 16);
		HMAC_MD5_Update(&ctx.md5, (u_int8_t *)key, len);
		HMAC_MD5_Final(key->mic, &ctx.md5);
		break;
	case EAPOL_KEY_DESC_V2:
		HMAC_SHA1_Init(&ctx.sha1, kck, 16);
		HMAC_SHA1_Update(&ctx.sha1, (u_int8_t *)key, len);
		HMAC_SHA1_Final(digest, &ctx.sha1);
		/* truncate HMAC-SHA1 to its 128 MSBs */
		memcpy(key->mic, digest, EAPOL_KEY_MIC_LEN);
		break;
	case EAPOL_KEY_DESC_V3:
		AES_CMAC_Init(&ctx.cmac);
		AES_CMAC_SetKey(&ctx.cmac, kck);
		AES_CMAC_Update(&ctx.cmac, (u_int8_t *)key, len);
		AES_CMAC_Final(key->mic, &ctx.cmac);
		break;
	}
}

/*
 * Check the MIC of a received EAPOL-Key frame using the specified Key
 * Confirmation Key (KCK).
 */
int
ieee80211_eapol_key_check_mic(struct ieee80211_eapol_key *key,
    const u_int8_t *kck)
{
	u_int8_t mic[EAPOL_KEY_MIC_LEN];

	memcpy(mic, key->mic, EAPOL_KEY_MIC_LEN);
	memset(key->mic, 0, EAPOL_KEY_MIC_LEN);
	ieee80211_eapol_key_mic(key, kck);

	return timingsafe_bcmp(key->mic, mic, EAPOL_KEY_MIC_LEN) != 0;
}

#ifndef IEEE80211_STA_ONLY
/*
 * Encrypt the Key Data field of an EAPOL-Key frame using the specified Key
 * Encryption Key (KEK).  The encryption algorithm can be either ARC4 or
 * AES Key Wrap depending on the EAPOL-Key Key Descriptor Version.
 */
void
ieee80211_eapol_key_encrypt(struct ieee80211com *ic,
    struct ieee80211_eapol_key *key, const u_int8_t *kek)
{
	union {
		struct rc4_ctx rc4;
		aes_key_wrap_ctx aes;
	} ctx;	/* XXX off stack? */
	u_int8_t keybuf[EAPOL_KEY_IV_LEN + 16];
	u_int16_t len, info;
	u_int8_t *data;
	int n;

	len  = BE_READ_2(key->paylen);
	info = BE_READ_2(key->info);
	data = (u_int8_t *)(key + 1);

	switch (info & EAPOL_KEY_VERSION_MASK) {
	case EAPOL_KEY_DESC_V1:
		/* set IV to the lower 16 octets of our global key counter */
		memcpy(key->iv, ic->ic_globalcnt + 16, 16);
		/* increment our global key counter (256-bit, big-endian) */
		for (n = 31; n >= 0 && ++ic->ic_globalcnt[n] == 0; n--);

		/* concatenate the EAPOL-Key IV field and the KEK */
		memcpy(keybuf, key->iv, EAPOL_KEY_IV_LEN);
		memcpy(keybuf + EAPOL_KEY_IV_LEN, kek, 16);

		rc4_keysetup(&ctx.rc4, keybuf, sizeof keybuf);
		/* discard the first 256 octets of the ARC4 key stream */
		rc4_skip(&ctx.rc4, RC4STATE);
		rc4_crypt(&ctx.rc4, data, data, len);
		break;
	case EAPOL_KEY_DESC_V2:
	case EAPOL_KEY_DESC_V3:
		if (len < 16 || (len & 7) != 0) {
			/* insert padding */
			n = (len < 16) ? 16 - len : 8 - (len & 7);
			data[len++] = IEEE80211_ELEMID_VENDOR;
			memset(&data[len], 0, n - 1);
			len += n - 1;
		}
		aes_key_wrap_set_key_wrap_only(&ctx.aes, kek, 16);
		aes_key_wrap(&ctx.aes, data, len / 8, data);
		len += 8;	/* AES Key Wrap adds 8 bytes */
		/* update key data length */
		BE_WRITE_2(key->paylen, len);
		/* update packet body length */
		BE_WRITE_2(key->len, sizeof(*key) + len - 4);
		break;
	}
}
#endif	/* IEEE80211_STA_ONLY */

/*
 * Decrypt the Key Data field of an EAPOL-Key frame using the specified Key
 * Encryption Key (KEK).  The encryption algorithm can be either ARC4 or
 * AES Key Wrap depending on the EAPOL-Key Key Descriptor Version.
 */
int
ieee80211_eapol_key_decrypt(struct ieee80211_eapol_key *key,
    const u_int8_t *kek)
{
	union {
		struct rc4_ctx rc4;
		aes_key_wrap_ctx aes;
	} ctx;	/* XXX off stack? */
	u_int8_t keybuf[EAPOL_KEY_IV_LEN + 16];
	u_int16_t len, info;
	u_int8_t *data;

	len  = BE_READ_2(key->paylen);
	info = BE_READ_2(key->info);
	data = (u_int8_t *)(key + 1);

	switch (info & EAPOL_KEY_VERSION_MASK) {
	case EAPOL_KEY_DESC_V1:
		/* concatenate the EAPOL-Key IV field and the KEK */
		memcpy(keybuf, key->iv, EAPOL_KEY_IV_LEN);
		memcpy(keybuf + EAPOL_KEY_IV_LEN, kek, 16);

		rc4_keysetup(&ctx.rc4, keybuf, sizeof keybuf);
		/* discard the first 256 octets of the ARC4 key stream */
		rc4_skip(&ctx.rc4, RC4STATE);
		rc4_crypt(&ctx.rc4, data, data, len);
		return 0;
	case EAPOL_KEY_DESC_V2:
	case EAPOL_KEY_DESC_V3:
		/* Key Data Length must be a multiple of 8 */
		if (len < 16 + 8 || (len & 7) != 0)
			return 1;
		len -= 8;	/* AES Key Wrap adds 8 bytes */
		aes_key_wrap_set_key(&ctx.aes, kek, 16);
		return aes_key_unwrap(&ctx.aes, data, data, len / 8);
>>>>>>> origin/master
	}
}

/*
<<<<<<< HEAD
 * Update a running CRC with the bytes buf[0..len-1]--the CRC
 * should be initialized to all 1's, and the transmitted value
 * is the 1's complement of the final running CRC
 */

static u_int32_t
ieee80211_crc_update(u_int32_t crc, u_int8_t *buf, int len)
{
	u_int8_t *endbuf;

	for (endbuf = buf + len; buf < endbuf; buf++)
		crc = ieee80211_crc_table[(crc ^ *buf) & 0xff] ^ (crc >> 8);
	return crc;
=======
 * Add a PMK entry to the PMKSA cache.
 */
struct ieee80211_pmk *
ieee80211_pmksa_add(struct ieee80211com *ic, enum ieee80211_akm akm,
    const u_int8_t *macaddr, const u_int8_t *key, u_int32_t lifetime)
{
	struct ieee80211_pmk *pmk;

	/* check if an entry already exists for this (STA,AKMP) */
	TAILQ_FOREACH(pmk, &ic->ic_pmksa, pmk_next) {
		if (pmk->pmk_akm == akm &&
		    IEEE80211_ADDR_EQ(pmk->pmk_macaddr, macaddr))
			break;
	}
	if (pmk == NULL) {
		/* allocate a new PMKSA entry */
		if ((pmk = malloc(sizeof(*pmk), M_DEVBUF, M_NOWAIT)) == NULL)
			return NULL;
		pmk->pmk_akm = akm;
		IEEE80211_ADDR_COPY(pmk->pmk_macaddr, macaddr);
		TAILQ_INSERT_TAIL(&ic->ic_pmksa, pmk, pmk_next);
	}
	memcpy(pmk->pmk_key, key, IEEE80211_PMK_LEN);
	pmk->pmk_lifetime = lifetime;	/* XXX not used yet */
#ifndef IEEE80211_STA_ONLY
	if (ic->ic_opmode == IEEE80211_M_HOSTAP) {
		ieee80211_derive_pmkid(pmk->pmk_akm, pmk->pmk_key,
		    ic->ic_myaddr, macaddr, pmk->pmk_pmkid);
	} else
#endif
	{
		ieee80211_derive_pmkid(pmk->pmk_akm, pmk->pmk_key,
		    macaddr, ic->ic_myaddr, pmk->pmk_pmkid);
	}
	return pmk;
}

/*
 * Check if we have a cached PMK entry for the specified node and PMKID.
 */
struct ieee80211_pmk *
ieee80211_pmksa_find(struct ieee80211com *ic, struct ieee80211_node *ni,
    const u_int8_t *pmkid)
{
	struct ieee80211_pmk *pmk;

	TAILQ_FOREACH(pmk, &ic->ic_pmksa, pmk_next) {
		if (pmk->pmk_akm == ni->ni_rsnakms &&
		    IEEE80211_ADDR_EQ(pmk->pmk_macaddr, ni->ni_macaddr) &&
		    (pmkid == NULL ||
		     memcmp(pmk->pmk_pmkid, pmkid, IEEE80211_PMKID_LEN) == 0))
			break;
	}
	return pmk;
>>>>>>> origin/master
}
