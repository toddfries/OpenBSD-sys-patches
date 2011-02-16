<<<<<<< HEAD
/*	$OpenBSD: if_pfsync.h,v 1.29 2006/05/28 02:04:15 mcbride Exp $	*/
=======
/*	$OpenBSD: if_pfsync.h,v 1.44 2010/11/29 05:31:38 dlg Exp $	*/
>>>>>>> origin/master

/*
 * Copyright (c) 2001 Michael Shalayeff
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 2008 David Gwynne <dlg@openbsd.org>
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

#ifndef _NET_IF_PFSYNC_H_
#define _NET_IF_PFSYNC_H_

#define PFSYNC_VERSION		6
#define PFSYNC_DFLTTL		255

#define PFSYNC_ACT_CLR		0	/* clear all states */
#define PFSYNC_ACT_OINS		1	/* old insert state */
#define PFSYNC_ACT_INS_ACK	2	/* ack of insterted state */
#define PFSYNC_ACT_OUPD		3	/* old update state */
#define PFSYNC_ACT_UPD_C	4	/* "compressed" update state */
#define PFSYNC_ACT_UPD_REQ	5	/* request "uncompressed" state */
#define PFSYNC_ACT_DEL		6	/* delete state */
#define PFSYNC_ACT_DEL_C	7	/* "compressed" delete state */
#define PFSYNC_ACT_INS_F	8	/* insert fragment */
#define PFSYNC_ACT_DEL_F	9	/* delete fragments */
#define PFSYNC_ACT_BUS		10	/* bulk update status */
#define PFSYNC_ACT_TDB		11	/* TDB replay counter update */
#define PFSYNC_ACT_EOF		12	/* end of frame - DEPRECATED */
#define PFSYNC_ACT_INS		13	/* insert state */
#define PFSYNC_ACT_UPD		14	/* update state */
#define PFSYNC_ACT_MAX		15

#define PFSYNC_ACTIONS		"CLR ST",		\
				"INS ST OLD",		\
				"INS ST ACK",		\
				"UPD ST OLD",		\
				"UPD ST COMP",		\
				"UPD ST REQ",		\
				"DEL ST",		\
				"DEL ST COMP",		\
				"INS FR",		\
				"DEL FR",		\
				"BULK UPD STAT",	\
				"TDB UPD",		\
				"EOF",			\
				"INS ST",		\
				"UPD ST"

<<<<<<< HEAD
struct pfsync_state_scrub {
	u_int16_t	pfss_flags;
	u_int8_t	pfss_ttl;	/* stashed TTL		*/
#define PFSYNC_SCRUB_FLAG_VALID 	0x01
	u_int8_t	scrub_flag;
	u_int32_t	pfss_ts_mod;	/* timestamp modulation	*/
} __packed;

struct pfsync_state_host {
	struct pf_addr	addr;
	u_int16_t	port;
	u_int16_t	pad[3];
} __packed;

struct pfsync_state_peer {
	struct pfsync_state_scrub scrub;	/* state is scrubbed	*/
	u_int32_t	seqlo;		/* Max sequence number sent	*/
	u_int32_t	seqhi;		/* Max the other end ACKd + win	*/
	u_int32_t	seqdiff;	/* Sequence number modulator	*/
	u_int16_t	max_win;	/* largest window (pre scaling)	*/
	u_int16_t	mss;		/* Maximum segment size option	*/
	u_int8_t	state;		/* active state level		*/
	u_int8_t	wscale;		/* window scaling factor	*/
	u_int8_t	pad[6];
} __packed;

struct pfsync_state {
	u_int32_t	 id[2];
	char		 ifname[IFNAMSIZ];
	struct pfsync_state_host lan;
	struct pfsync_state_host gwy;
	struct pfsync_state_host ext;
	struct pfsync_state_peer src;
	struct pfsync_state_peer dst;
	struct pf_addr	 rt_addr;
	u_int32_t	 rule;
	u_int32_t	 anchor;
	u_int32_t	 nat_rule;
	u_int32_t	 creation;
	u_int32_t	 expire;
	u_int32_t	 packets[2][2];
	u_int32_t	 bytes[2][2];
	u_int32_t	 creatorid;
	sa_family_t	 af;
	u_int8_t	 proto;
	u_int8_t	 direction;
	u_int8_t	 log;
	u_int8_t	 allow_opts;
	u_int8_t	 timeout;
	u_int8_t	 sync_flags;
	u_int8_t	 updates;
} __packed;

#define PFSYNC_FLAG_COMPRESS 	0x01
#define PFSYNC_FLAG_STALE	0x02

struct pfsync_tdb {
	u_int32_t	spi;
	union sockaddr_union dst;
	u_int32_t	rpl;
	u_int64_t	cur_bytes;
	u_int8_t	sproto;
	u_int8_t	updates;
	u_int8_t	pad[2];
} __packed;
=======
/*
 * A pfsync frame is built from a header followed by several sections which
 * are all prefixed with their own subheaders.
 *
 * | ...			|
 * | IP header			|
 * +============================+
 * | pfsync_header		|
 * +----------------------------+
 * | pfsync_subheader		|
 * +----------------------------+
 * | first action fields	|
 * | ...			|
 * +----------------------------+
 * | pfsync_subheader		|
 * +----------------------------+
 * | second action fields	|
 * | ...			|
 * +============================+
 */
>>>>>>> origin/master

/*
 * Frame header
 */

struct pfsync_header {
	u_int8_t			version;
	u_int8_t			_pad;
	u_int16_t			len; /* in bytes */
	u_int8_t			pfcksum[PF_MD5_DIGEST_LENGTH];
} __packed;

/*
 * Frame region subheader
 */

struct pfsync_subheader {
	u_int8_t			action;
	u_int8_t			len; /* in dwords */
	u_int16_t			count;
} __packed;

/*
 * CLR
 */

struct pfsync_clr {
	char				ifname[IFNAMSIZ];
	u_int32_t			creatorid;
} __packed;

/*
 * OINS, OUPD
 */

/* these messages are deprecated */

/*
 * INS, UPD, DEL
 */

/* these use struct pfsync_state in pfvar.h */

/*
 * INS_ACK
 */

struct pfsync_ins_ack {
	u_int64_t			id;
	u_int32_t			creatorid;
} __packed;

/*
 * UPD_C
 */

struct pfsync_upd_c {
	u_int64_t			id;
	struct pfsync_state_peer	src;
	struct pfsync_state_peer	dst;
	u_int32_t			creatorid;
	u_int32_t			expire;
	u_int8_t			timeout;
	u_int8_t			state_flags;
	u_int8_t			_pad[2];
} __packed;

<<<<<<< HEAD
#ifdef _KERNEL
=======
/*
 * UPD_REQ
 */

struct pfsync_upd_req {
	u_int64_t			id;
	u_int32_t			creatorid;
} __packed;

/*
 * DEL_C
 */
>>>>>>> origin/master

struct pfsync_del_c {
	u_int64_t			id;
	u_int32_t			creatorid;
} __packed;

/* 
 * INS_F, DEL_F
 */

/* not implemented (yet) */

/*
 * BUS
 */

struct pfsync_bus {
	u_int32_t			creatorid;
	u_int32_t			endtime;
	u_int8_t			status;
#define PFSYNC_BUS_START			1
#define PFSYNC_BUS_END				2
	u_int8_t			_pad[3];
} __packed;

/*
 * TDB
 */

struct pfsync_tdb {
	u_int32_t			spi;
	union sockaddr_union		dst;
	u_int32_t			rpl;
	u_int64_t			cur_bytes;
	u_int8_t			sproto;
	u_int8_t			updates;
	u_int16_t			rdomain;
} __packed;

/*
 * EOF
 */

/* this message is deprecated */


#define PFSYNC_HDRLEN		sizeof(struct pfsync_header)


/*
 * Names for PFSYNC sysctl objects
 */
#define	PFSYNCCTL_STATS		1	/* PFSYNC stats */
#define	PFSYNCCTL_MAXID		2

#define	PFSYNCCTL_NAMES { \
	{ 0, 0 }, \
	{ "stats", CTLTYPE_STRUCT }, \
}

struct pfsyncstats {
	u_int64_t	pfsyncs_ipackets;	/* total input packets, IPv4 */
	u_int64_t	pfsyncs_ipackets6;	/* total input packets, IPv6 */
	u_int64_t	pfsyncs_badif;		/* not the right interface */
	u_int64_t	pfsyncs_badttl;		/* TTL is not PFSYNC_DFLTTL */
	u_int64_t	pfsyncs_hdrops;		/* packets shorter than hdr */
	u_int64_t	pfsyncs_badver;		/* bad (incl unsupp) version */
	u_int64_t	pfsyncs_badact;		/* bad action */
	u_int64_t	pfsyncs_badlen;		/* data length does not match */
	u_int64_t	pfsyncs_badauth;	/* bad authentication */
	u_int64_t	pfsyncs_stale;		/* stale state */
	u_int64_t	pfsyncs_badval;		/* bad values */
	u_int64_t	pfsyncs_badstate;	/* insert/lookup failed */

	u_int64_t	pfsyncs_opackets;	/* total output packets, IPv4 */
	u_int64_t	pfsyncs_opackets6;	/* total output packets, IPv6 */
	u_int64_t	pfsyncs_onomem;		/* no memory for an mbuf */
	u_int64_t	pfsyncs_oerrors;	/* ip output error */
};

/*
 * Configuration structure for SIOCSETPFSYNC SIOCGETPFSYNC
 */
struct pfsyncreq {
	char		 pfsyncr_syncdev[IFNAMSIZ];
	struct in_addr	 pfsyncr_syncpeer;
	int		 pfsyncr_maxupdates;
	int		 pfsyncr_defer;
};

<<<<<<< HEAD

#define pf_state_peer_hton(s,d) do {		\
	(d)->seqlo = htonl((s)->seqlo);		\
	(d)->seqhi = htonl((s)->seqhi);		\
	(d)->seqdiff = htonl((s)->seqdiff);	\
	(d)->max_win = htons((s)->max_win);	\
	(d)->mss = htons((s)->mss);		\
	(d)->state = (s)->state;		\
	(d)->wscale = (s)->wscale;		\
	if ((s)->scrub) {						\
		(d)->scrub.pfss_flags = 				\
		    htons((s)->scrub->pfss_flags & PFSS_TIMESTAMP);	\
		(d)->scrub.pfss_ttl = (s)->scrub->pfss_ttl;		\
		(d)->scrub.pfss_ts_mod = htonl((s)->scrub->pfss_ts_mod);\
		(d)->scrub.scrub_flag = PFSYNC_SCRUB_FLAG_VALID;	\
	}								\
} while (0)

#define pf_state_peer_ntoh(s,d) do {		\
	(d)->seqlo = ntohl((s)->seqlo);		\
	(d)->seqhi = ntohl((s)->seqhi);		\
	(d)->seqdiff = ntohl((s)->seqdiff);	\
	(d)->max_win = ntohs((s)->max_win);	\
	(d)->mss = ntohs((s)->mss);		\
	(d)->state = (s)->state;		\
	(d)->wscale = (s)->wscale;		\
	if ((s)->scrub.scrub_flag == PFSYNC_SCRUB_FLAG_VALID && 	\
	    (d)->scrub != NULL) {					\
		(d)->scrub->pfss_flags =				\
		    ntohs((s)->scrub.pfss_flags) & PFSS_TIMESTAMP;	\
		(d)->scrub->pfss_ttl = (s)->scrub.pfss_ttl;		\
		(d)->scrub->pfss_ts_mod = ntohl((s)->scrub.pfss_ts_mod);\
	}								\
} while (0)

#define pf_state_host_hton(s,d) do {				\
	bcopy(&(s)->addr, &(d)->addr, sizeof((d)->addr));	\
	(d)->port = (s)->port;					\
} while (0)

#define pf_state_host_ntoh(s,d) do {				\
	bcopy(&(s)->addr, &(d)->addr, sizeof((d)->addr));	\
	(d)->port = (s)->port;					\
} while (0)

#define pf_state_counter_hton(s,d) do {				\
	d[0] = htonl((s>>32)&0xffffffff);			\
	d[1] = htonl(s&0xffffffff);				\
} while (0)

#define pf_state_counter_ntoh(s,d) do {				\
	d = ntohl(s[0]);					\
	d = d<<32;						\
	d += ntohl(s[1]);					\
} while (0)

#ifdef _KERNEL
void pfsync_input(struct mbuf *, ...);
int pfsync_clear_states(u_int32_t, char *);
int pfsync_pack_state(u_int8_t, struct pf_state *, int);
#define pfsync_insert_state(st)	do {				\
	if ((st->rule.ptr->rule_flag & PFRULE_NOSYNC) ||	\
	    (st->proto == IPPROTO_PFSYNC))			\
		st->sync_flags |= PFSTATE_NOSYNC;		\
	else if (!st->sync_flags)				\
		pfsync_pack_state(PFSYNC_ACT_INS, (st), 	\
		    PFSYNC_FLAG_COMPRESS);			\
	st->sync_flags &= ~PFSTATE_FROMSYNC;			\
} while (0)
#define pfsync_update_state(st) do {				\
	if (!st->sync_flags)					\
		pfsync_pack_state(PFSYNC_ACT_UPD, (st), 	\
		    PFSYNC_FLAG_COMPRESS);			\
	st->sync_flags &= ~PFSTATE_FROMSYNC;			\
} while (0)
#define pfsync_delete_state(st) do {				\
	if (!st->sync_flags)					\
		pfsync_pack_state(PFSYNC_ACT_DEL, (st),		\
		    PFSYNC_FLAG_COMPRESS);			\
} while (0)
int pfsync_update_tdb(struct tdb *, int);
=======
#ifdef _KERNEL

/*
 * this shows where a pf state is with respect to the syncing.
 */
#define PFSYNC_S_IACK	0x00
#define PFSYNC_S_UPD_C	0x01
#define PFSYNC_S_DEL	0x02
#define PFSYNC_S_INS	0x03
#define PFSYNC_S_UPD	0x04
#define PFSYNC_S_COUNT	0x05

#define PFSYNC_S_DEFER	0xfe
#define PFSYNC_S_NONE	0xff

void			pfsync_input(struct mbuf *, ...);
int			pfsync_sysctl(int *, u_int,  void *, size_t *,
			    void *, size_t);

#define	PFSYNC_SI_IOCTL		0x01
#define	PFSYNC_SI_CKSUM		0x02
#define	PFSYNC_SI_ACK		0x04
int			pfsync_state_import(struct pfsync_state *, int);
void			pfsync_state_export(struct pfsync_state *,
			    struct pf_state *);

void			pfsync_insert_state(struct pf_state *);
void			pfsync_update_state(struct pf_state *);
void			pfsync_delete_state(struct pf_state *);
void			pfsync_clear_states(u_int32_t, const char *);

void			pfsync_update_tdb(struct tdb *, int);
void			pfsync_delete_tdb(struct tdb *);

int			pfsync_defer(struct pf_state *, struct mbuf *);

int			pfsync_up(void);
int			pfsync_state_in_use(struct pf_state *);
>>>>>>> origin/master
#endif

#endif /* _NET_IF_PFSYNC_H_ */
