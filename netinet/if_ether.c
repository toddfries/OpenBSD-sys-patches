/*-
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)if_ether.c	8.1 (Berkeley) 6/10/93
 */

/*
 * Ethernet address resolution protocol.
 * TODO:
 *	add "inuse/lock" bit (or ref. count) along with valid bit
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/netinet/if_ether.c,v 1.192 2009/03/09 17:53:05 bms Exp $");

#include "opt_inet.h"
#include "opt_route.h"
#include "opt_mac.h"
#include "opt_carp.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/vimage.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/netisr.h>
#include <net/if_llc.h>
#include <net/ethernet.h>
#include <net/vnet.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <net/if_llatbl.h>
#include <netinet/if_ether.h>
#include <netinet/vinet.h>

#include <net/if_arc.h>
#include <net/iso88025.h>

#ifdef DEV_CARP
#include <netinet/ip_carp.h>
#endif

#include <security/mac/mac_framework.h>

#define SIN(s) ((struct sockaddr_in *)s)
#define SDL(s) ((struct sockaddr_dl *)s)
#define LLTABLE(ifp)	\
	((struct in_ifinfo *)(ifp)->if_afdata[AF_INET])->ii_llt

SYSCTL_DECL(_net_link_ether);
SYSCTL_NODE(_net_link_ether, PF_INET, inet, CTLFLAG_RW, 0, "");

/* timer values */
#ifdef VIMAGE_GLOBALS
static int	arpt_keep; /* once resolved, good for 20 more minutes */
static int	arp_maxtries;
int	useloopback; /* use loopback interface for local traffic */
static int	arp_proxyall;
#endif

SYSCTL_V_INT(V_NET, vnet_inet, _net_link_ether_inet, OID_AUTO, max_age,
    CTLFLAG_RW, arpt_keep, 0, "ARP entry lifetime in seconds");

static struct	ifqueue arpintrq;

SYSCTL_V_INT(V_NET, vnet_inet, _net_link_ether_inet, OID_AUTO, maxtries,
	CTLFLAG_RW, arp_maxtries, 0,
	"ARP resolution attempts before returning error");
SYSCTL_V_INT(V_NET, vnet_inet, _net_link_ether_inet, OID_AUTO, useloopback,
	CTLFLAG_RW, useloopback, 0,
	"Use the loopback interface for local traffic");
SYSCTL_V_INT(V_NET, vnet_inet, _net_link_ether_inet, OID_AUTO, proxyall,
	CTLFLAG_RW, arp_proxyall, 0,
	"Enable proxy ARP for all suitable requests");

static void	arp_init(void);
void		arprequest(struct ifnet *,
			struct in_addr *, struct in_addr *, u_char *);
static void	arpintr(struct mbuf *);
static void	arptimer(void *);
#ifdef INET
static void	in_arpinput(struct mbuf *);
#endif

#ifdef AF_INET
void arp_ifscrub(struct ifnet *ifp, uint32_t addr);

/*
 * called by in_ifscrub to remove entry from the table when
 * the interface goes away
 */
void
arp_ifscrub(struct ifnet *ifp, uint32_t addr)
{
	struct sockaddr_in addr4;

	bzero((void *)&addr4, sizeof(addr4));
	addr4.sin_len    = sizeof(addr4);
	addr4.sin_family = AF_INET;
	addr4.sin_addr.s_addr = addr;
	IF_AFDATA_LOCK(ifp);
	lla_lookup(LLTABLE(ifp), (LLE_DELETE | LLE_IFADDR),
	    (struct sockaddr *)&addr4);
	IF_AFDATA_UNLOCK(ifp);
}
#endif

/*
 * Timeout routine.  Age arp_tab entries periodically.
 */
static void
arptimer(void *arg)
{
	struct ifnet *ifp;
	struct llentry   *lle = (struct llentry *)arg;

	if (lle == NULL) {
		panic("%s: NULL entry!\n", __func__);
		return;
	}
	ifp = lle->lle_tbl->llt_ifp;
	IF_AFDATA_LOCK(ifp);
	LLE_WLOCK(lle);
	if (((lle->la_flags & LLE_DELETED)
		|| (time_second >= lle->la_expire))
	    && (!callout_pending(&lle->la_timer) &&
		callout_active(&lle->la_timer)))
		(void) llentry_free(lle);
	else {
		/*
		 * Still valid, just drop our reference
		 */
		LLE_FREE_LOCKED(lle);
	}
	IF_AFDATA_UNLOCK(ifp);
}

/*
 * Broadcast an ARP request. Caller specifies:
 *	- arp header source ip address
 *	- arp header target ip address
 *	- arp header source ethernet address
 */
void
arprequest(struct ifnet *ifp, struct in_addr *sip, struct in_addr  *tip,
    u_char *enaddr)
{
	struct mbuf *m;
	struct arphdr *ah;
	struct sockaddr sa;

	if (sip == NULL) {
		/* XXX don't believe this can happen (or explain why) */
		/*
		 * The caller did not supply a source address, try to find
		 * a compatible one among those assigned to this interface.
		 */
		struct ifaddr *ifa;

		TAILQ_FOREACH(ifa, &ifp->if_addrhead, ifa_link) {
			if (!ifa->ifa_addr ||
			    ifa->ifa_addr->sa_family != AF_INET)
				continue;
			sip = &SIN(ifa->ifa_addr)->sin_addr;
			if (0 == ((sip->s_addr ^ tip->s_addr) &
			    SIN(ifa->ifa_netmask)->sin_addr.s_addr) )
				break;  /* found it. */
		}
		if (sip == NULL) {  
			printf("%s: cannot find matching address\n", __func__);
			return;
		}
	}

	if ((m = m_gethdr(M_DONTWAIT, MT_DATA)) == NULL)
		return;
	m->m_len = sizeof(*ah) + 2*sizeof(struct in_addr) +
		2*ifp->if_data.ifi_addrlen;
	m->m_pkthdr.len = m->m_len;
	MH_ALIGN(m, m->m_len);
	ah = mtod(m, struct arphdr *);
	bzero((caddr_t)ah, m->m_len);
#ifdef MAC
	mac_netinet_arp_send(ifp, m);
#endif
	ah->ar_pro = htons(ETHERTYPE_IP);
	ah->ar_hln = ifp->if_addrlen;		/* hardware address length */
	ah->ar_pln = sizeof(struct in_addr);	/* protocol address length */
	ah->ar_op = htons(ARPOP_REQUEST);
	bcopy((caddr_t)enaddr, (caddr_t)ar_sha(ah), ah->ar_hln);
	bcopy((caddr_t)sip, (caddr_t)ar_spa(ah), ah->ar_pln);
	bcopy((caddr_t)tip, (caddr_t)ar_tpa(ah), ah->ar_pln);
	sa.sa_family = AF_ARP;
	sa.sa_len = 2;
	m->m_flags |= M_BCAST;
	(*ifp->if_output)(ifp, m, &sa, (struct rtentry *)0);
}

/*
 * Resolve an IP address into an ethernet address.
 * On input:
 *    ifp is the interface we use
 *    rt0 is the route to the final destination (possibly useless)
 *    m is the mbuf. May be NULL if we don't have a packet.
 *    dst is the next hop,
 *    desten is where we want the address.
 *
 * On success, desten is filled in and the function returns 0;
 * If the packet must be held pending resolution, we return EWOULDBLOCK
 * On other errors, we return the corresponding error code.
 * Note that m_freem() handles NULL.
 */
int
arpresolve(struct ifnet *ifp, struct rtentry *rt0, struct mbuf *m,
	struct sockaddr *dst, u_char *desten, struct llentry **lle)
{
	INIT_VNET_INET(ifp->if_vnet);
	struct llentry *la = 0;
	u_int flags = 0;
	int error, renew;

	*lle = NULL;
	if (m != NULL) {
		if (m->m_flags & M_BCAST) {
			/* broadcast */
			(void)memcpy(desten,
			    ifp->if_broadcastaddr, ifp->if_addrlen);
			return (0);
		}
		if (m->m_flags & M_MCAST && ifp->if_type != IFT_ARCNET) {
			/* multicast */
			ETHER_MAP_IP_MULTICAST(&SIN(dst)->sin_addr, desten);
			return (0);
		}
	}
	/* XXXXX
	 */
retry:
	IF_AFDATA_RLOCK(ifp);	
	la = lla_lookup(LLTABLE(ifp), flags, dst);
	IF_AFDATA_RUNLOCK(ifp);	
	if ((la == NULL) && ((flags & LLE_EXCLUSIVE) == 0)
	    && ((ifp->if_flags & (IFF_NOARP | IFF_STATICARP)) == 0)) {		
		flags |= (LLE_CREATE | LLE_EXCLUSIVE);
		IF_AFDATA_WLOCK(ifp);	
		la = lla_lookup(LLTABLE(ifp), flags, dst);
		IF_AFDATA_WUNLOCK(ifp);	
	}
	if (la == NULL) {
		if (flags & LLE_CREATE)
			log(LOG_DEBUG,
			    "arpresolve: can't allocate llinfo for %s\n",
			    inet_ntoa(SIN(dst)->sin_addr));
		m_freem(m);
		return (EINVAL);
	} 

	if ((la->la_flags & LLE_VALID) &&
	    ((la->la_flags & LLE_STATIC) || la->la_expire > time_uptime)) {
		bcopy(&la->ll_addr, desten, ifp->if_addrlen);
		/*
		 * If entry has an expiry time and it is approaching,
		 * see if we need to send an ARP request within this
		 * arpt_down interval.
		 */
		if (!(la->la_flags & LLE_STATIC) &&
		    time_uptime + la->la_preempt > la->la_expire) {
			arprequest(ifp, NULL,
			    &SIN(dst)->sin_addr, IF_LLADDR(ifp));

			la->la_preempt--;
		}
		
		*lle = la;
		error = 0;
		goto done;
	} 
			    
	if (la->la_flags & LLE_STATIC) {   /* should not happen! */
		log(LOG_DEBUG, "arpresolve: ouch, empty static llinfo for %s\n",
		    inet_ntoa(SIN(dst)->sin_addr));
		m_freem(m);
		error = EINVAL;
		goto done;
	}

	renew = (la->la_asked == 0 || la->la_expire != time_uptime);
	if ((renew || m != NULL) && (flags & LLE_EXCLUSIVE) == 0) {
		flags |= LLE_EXCLUSIVE;
		LLE_RUNLOCK(la);
		goto retry;
	}
	/*
	 * There is an arptab entry, but no ethernet address
	 * response yet.  Replace the held mbuf with this
	 * latest one.
	 */
	if (m != NULL) {
		if (la->la_hold != NULL)
			m_freem(la->la_hold);
		la->la_hold = m;
		if (renew == 0 && (flags & LLE_EXCLUSIVE)) {
			flags &= ~LLE_EXCLUSIVE;
			LLE_DOWNGRADE(la);
		}
		
	}
	/*
	 * Return EWOULDBLOCK if we have tried less than arp_maxtries. It
	 * will be masked by ether_output(). Return EHOSTDOWN/EHOSTUNREACH
	 * if we have already sent arp_maxtries ARP requests. Retransmit the
	 * ARP request, but not faster than one request per second.
	 */
	if (la->la_asked < V_arp_maxtries)
		error = EWOULDBLOCK;	/* First request. */
	else
		error =
		    (rt0->rt_flags & RTF_GATEWAY) ? EHOSTDOWN : EHOSTUNREACH;

	if (renew) {
		LLE_ADDREF(la);
		la->la_expire = time_uptime;
		callout_reset(&la->la_timer, hz, arptimer, la);
		la->la_asked++;
		LLE_WUNLOCK(la);
		arprequest(ifp, NULL, &SIN(dst)->sin_addr,
		    IF_LLADDR(ifp));
		return (error);
	}
done:
	if (flags & LLE_EXCLUSIVE)
		LLE_WUNLOCK(la);
	else
		LLE_RUNLOCK(la);
	return (error);
}

/*
 * Common length and type checks are done here,
 * then the protocol-specific routine is called.
 */
static void
arpintr(struct mbuf *m)
{
	struct arphdr *ar;

	if (m->m_len < sizeof(struct arphdr) &&
	    ((m = m_pullup(m, sizeof(struct arphdr))) == NULL)) {
		log(LOG_ERR, "arp: runt packet -- m_pullup failed\n");
		return;
	}
	ar = mtod(m, struct arphdr *);

	if (ntohs(ar->ar_hrd) != ARPHRD_ETHER &&
	    ntohs(ar->ar_hrd) != ARPHRD_IEEE802 &&
	    ntohs(ar->ar_hrd) != ARPHRD_ARCNET &&
	    ntohs(ar->ar_hrd) != ARPHRD_IEEE1394) {
		log(LOG_ERR, "arp: unknown hardware address format (0x%2D)\n",
		    (unsigned char *)&ar->ar_hrd, "");
		m_freem(m);
		return;
	}

	if (m->m_len < arphdr_len(ar)) {
		if ((m = m_pullup(m, arphdr_len(ar))) == NULL) {
			log(LOG_ERR, "arp: runt packet\n");
			m_freem(m);
			return;
		}
		ar = mtod(m, struct arphdr *);
	}

	switch (ntohs(ar->ar_pro)) {
#ifdef INET
	case ETHERTYPE_IP:
		in_arpinput(m);
		return;
#endif
	}
	m_freem(m);
}

#ifdef INET
/*
 * ARP for Internet protocols on 10 Mb/s Ethernet.
 * Algorithm is that given in RFC 826.
 * In addition, a sanity check is performed on the sender
 * protocol address, to catch impersonators.
 * We no longer handle negotiations for use of trailer protocol:
 * Formerly, ARP replied for protocol type ETHERTYPE_TRAIL sent
 * along with IP replies if we wanted trailers sent to us,
 * and also sent them in response to IP replies.
 * This allowed either end to announce the desire to receive
 * trailer packets.
 * We no longer reply to requests for ETHERTYPE_TRAIL protocol either,
 * but formerly didn't normally send requests.
 */
static int log_arp_wrong_iface = 1;
static int log_arp_movements = 1;
static int log_arp_permanent_modify = 1;

SYSCTL_INT(_net_link_ether_inet, OID_AUTO, log_arp_wrong_iface, CTLFLAG_RW,
	&log_arp_wrong_iface, 0,
	"log arp packets arriving on the wrong interface");
SYSCTL_INT(_net_link_ether_inet, OID_AUTO, log_arp_movements, CTLFLAG_RW,
        &log_arp_movements, 0,
        "log arp replies from MACs different than the one in the cache");
SYSCTL_INT(_net_link_ether_inet, OID_AUTO, log_arp_permanent_modify, CTLFLAG_RW,
        &log_arp_permanent_modify, 0,
        "log arp replies from MACs different than the one in the permanent arp entry");


static void
in_arpinput(struct mbuf *m)
{
	struct arphdr *ah;
	struct ifnet *ifp = m->m_pkthdr.rcvif;
	struct llentry *la = NULL;
	struct rtentry *rt;
	struct ifaddr *ifa;
	struct in_ifaddr *ia;
	struct sockaddr sa;
	struct in_addr isaddr, itaddr, myaddr;
	u_int8_t *enaddr = NULL;
	int op, flags;
	struct mbuf *m0;
	int req_len;
	int bridged = 0, is_bridge = 0;
#ifdef DEV_CARP
	int carp_match = 0;
#endif
	struct sockaddr_in sin;
	sin.sin_len = sizeof(struct sockaddr_in);
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	INIT_VNET_INET(ifp->if_vnet);

	if (ifp->if_bridge)
		bridged = 1;
	if (ifp->if_type == IFT_BRIDGE)
		is_bridge = 1;

	req_len = arphdr_len2(ifp->if_addrlen, sizeof(struct in_addr));
	if (m->m_len < req_len && (m = m_pullup(m, req_len)) == NULL) {
		log(LOG_ERR, "in_arp: runt packet -- m_pullup failed\n");
		return;
	}

	ah = mtod(m, struct arphdr *);
	op = ntohs(ah->ar_op);
	(void)memcpy(&isaddr, ar_spa(ah), sizeof (isaddr));
	(void)memcpy(&itaddr, ar_tpa(ah), sizeof (itaddr));

	/*
	 * For a bridge, we want to check the address irrespective
	 * of the receive interface. (This will change slightly
	 * when we have clusters of interfaces).
	 * If the interface does not match, but the recieving interface
	 * is part of carp, we call carp_iamatch to see if this is a
	 * request for the virtual host ip.
	 * XXX: This is really ugly!
	 */
	LIST_FOREACH(ia, INADDR_HASH(itaddr.s_addr), ia_hash) {
		if (((bridged && ia->ia_ifp->if_bridge != NULL) ||
		    ia->ia_ifp == ifp) &&
		    itaddr.s_addr == ia->ia_addr.sin_addr.s_addr)
			goto match;
#ifdef DEV_CARP
		if (ifp->if_carp != NULL &&
		    carp_iamatch(ifp->if_carp, ia, &isaddr, &enaddr) &&
		    itaddr.s_addr == ia->ia_addr.sin_addr.s_addr) {
			carp_match = 1;
			goto match;
		}
#endif
	}
	LIST_FOREACH(ia, INADDR_HASH(isaddr.s_addr), ia_hash)
		if (((bridged && ia->ia_ifp->if_bridge != NULL) ||
		    ia->ia_ifp == ifp) &&
		    isaddr.s_addr == ia->ia_addr.sin_addr.s_addr)
			goto match;

#define BDG_MEMBER_MATCHES_ARP(addr, ifp, ia)				\
  (ia->ia_ifp->if_bridge == ifp->if_softc &&				\
  !bcmp(IF_LLADDR(ia->ia_ifp), IF_LLADDR(ifp), ifp->if_addrlen) &&	\
  addr == ia->ia_addr.sin_addr.s_addr)
	/*
	 * Check the case when bridge shares its MAC address with
	 * some of its children, so packets are claimed by bridge
	 * itself (bridge_input() does it first), but they are really
	 * meant to be destined to the bridge member.
	 */
	if (is_bridge) {
		LIST_FOREACH(ia, INADDR_HASH(itaddr.s_addr), ia_hash) {
			if (BDG_MEMBER_MATCHES_ARP(itaddr.s_addr, ifp, ia)) {
				ifp = ia->ia_ifp;
				goto match;
			}
		}
	}
#undef BDG_MEMBER_MATCHES_ARP

	/*
	 * No match, use the first inet address on the receive interface
	 * as a dummy address for the rest of the function.
	 */
	TAILQ_FOREACH(ifa, &ifp->if_addrhead, ifa_link)
		if (ifa->ifa_addr->sa_family == AF_INET) {
			ia = ifatoia(ifa);
			goto match;
		}
	/*
	 * If bridging, fall back to using any inet address.
	 */
	if (!bridged || (ia = TAILQ_FIRST(&V_in_ifaddrhead)) == NULL)
		goto drop;
match:
	if (!enaddr)
		enaddr = (u_int8_t *)IF_LLADDR(ifp);
	myaddr = ia->ia_addr.sin_addr;
	if (!bcmp(ar_sha(ah), enaddr, ifp->if_addrlen))
		goto drop;	/* it's from me, ignore it. */
	if (!bcmp(ar_sha(ah), ifp->if_broadcastaddr, ifp->if_addrlen)) {
		log(LOG_ERR,
		    "arp: link address is broadcast for IP address %s!\n",
		    inet_ntoa(isaddr));
		goto drop;
	}
	/*
	 * Warn if another host is using the same IP address, but only if the
	 * IP address isn't 0.0.0.0, which is used for DHCP only, in which
	 * case we suppress the warning to avoid false positive complaints of
	 * potential misconfiguration.
	 */
	if (!bridged && isaddr.s_addr == myaddr.s_addr && myaddr.s_addr != 0) {
		log(LOG_ERR,
		   "arp: %*D is using my IP address %s on %s!\n",
		   ifp->if_addrlen, (u_char *)ar_sha(ah), ":",
		   inet_ntoa(isaddr), ifp->if_xname);
		itaddr = myaddr;
		goto reply;
	}
	if (ifp->if_flags & IFF_STATICARP)
		goto reply;

	bzero(&sin, sizeof(sin));
	sin.sin_len = sizeof(struct sockaddr_in);
	sin.sin_family = AF_INET;
	sin.sin_addr = isaddr;
	flags = (itaddr.s_addr == myaddr.s_addr) ? LLE_CREATE : 0;
	flags |= LLE_EXCLUSIVE;
	IF_AFDATA_LOCK(ifp); 
	la = lla_lookup(LLTABLE(ifp), flags, (struct sockaddr *)&sin);
	IF_AFDATA_UNLOCK(ifp);
	if (la != NULL) {
		/* the following is not an error when doing bridging */
		if (!bridged && la->lle_tbl->llt_ifp != ifp
#ifdef DEV_CARP
		    && (ifp->if_type != IFT_CARP || !carp_match)
#endif
			) {
			if (log_arp_wrong_iface)
				log(LOG_ERR, "arp: %s is on %s "
				    "but got reply from %*D on %s\n",
				    inet_ntoa(isaddr),
				    la->lle_tbl->llt_ifp->if_xname,
				    ifp->if_addrlen, (u_char *)ar_sha(ah), ":",
				    ifp->if_xname);
			goto reply;
		}
		if ((la->la_flags & LLE_VALID) &&
		    bcmp(ar_sha(ah), &la->ll_addr, ifp->if_addrlen)) {
			if (la->la_flags & LLE_STATIC) {
				log(LOG_ERR,
				    "arp: %*D attempts to modify permanent "
				    "entry for %s on %s\n",
				    ifp->if_addrlen, (u_char *)ar_sha(ah), ":",
				    inet_ntoa(isaddr), ifp->if_xname);
				goto reply;
			}
			if (log_arp_movements) {
			        log(LOG_INFO, "arp: %s moved from %*D "
				    "to %*D on %s\n",
				    inet_ntoa(isaddr),
				    ifp->if_addrlen,
				    (u_char *)&la->ll_addr, ":",
				    ifp->if_addrlen, (u_char *)ar_sha(ah), ":",
				    ifp->if_xname);
			}
		}
		    
		if (ifp->if_addrlen != ah->ar_hln) {
			log(LOG_WARNING,
			    "arp from %*D: addr len: new %d, i/f %d (ignored)",
			    ifp->if_addrlen, (u_char *) ar_sha(ah), ":",
			    ah->ar_hln, ifp->if_addrlen);
			goto reply;
		}
		(void)memcpy(&la->ll_addr, ar_sha(ah), ifp->if_addrlen);
		la->la_flags |= LLE_VALID;

		if (!(la->la_flags & LLE_STATIC)) {
			la->la_expire = time_uptime + V_arpt_keep;
			callout_reset(&la->la_timer, hz * V_arpt_keep,
			    arptimer, la);
		}
		la->la_asked = 0;
		la->la_preempt = V_arp_maxtries;
		if (la->la_hold != NULL) {
			m0 = la->la_hold;
			la->la_hold = 0;
			memcpy(&sa, L3_ADDR(la), sizeof(sa));
			LLE_WUNLOCK(la);
			
			(*ifp->if_output)(ifp, m0, &sa, NULL);
			return;
		}
	}
reply:
	if (op != ARPOP_REQUEST)
		goto drop;

	if (itaddr.s_addr == myaddr.s_addr) {
		/* Shortcut.. the receiving interface is the target. */
		(void)memcpy(ar_tha(ah), ar_sha(ah), ah->ar_hln);
		(void)memcpy(ar_sha(ah), enaddr, ah->ar_hln);
	} else {
		struct llentry *lle = NULL;

		if (!V_arp_proxyall)
			goto drop;

		sin.sin_addr = itaddr;
		/* XXX MRT use table 0 for arp reply  */
		rt = in_rtalloc1((struct sockaddr *)&sin, 0, 0UL, 0);
		if (!rt)
			goto drop;

		/*
		 * Don't send proxies for nodes on the same interface
		 * as this one came out of, or we'll get into a fight
		 * over who claims what Ether address.
		 */
		if (!rt->rt_ifp || rt->rt_ifp == ifp) {
			RTFREE_LOCKED(rt);
			goto drop;
		}
		IF_AFDATA_LOCK(rt->rt_ifp); 
		lle = lla_lookup(LLTABLE(rt->rt_ifp), 0, (struct sockaddr *)&sin);
		IF_AFDATA_UNLOCK(rt->rt_ifp);
		RTFREE_LOCKED(rt);

		if (lle != NULL) {
			(void)memcpy(ar_tha(ah), ar_sha(ah), ah->ar_hln);
			(void)memcpy(ar_sha(ah), &lle->ll_addr, ah->ar_hln);
			LLE_RUNLOCK(lle);
		} else
			goto drop;

		/*
		 * Also check that the node which sent the ARP packet
		 * is on the the interface we expect it to be on. This
		 * avoids ARP chaos if an interface is connected to the
		 * wrong network.
		 */
		sin.sin_addr = isaddr;

		/* XXX MRT use table 0 for arp checks */
		rt = in_rtalloc1((struct sockaddr *)&sin, 0, 0UL, 0);
		if (!rt)
			goto drop;
		if (rt->rt_ifp != ifp) {
			log(LOG_INFO, "arp_proxy: ignoring request"
			    " from %s via %s, expecting %s\n",
			    inet_ntoa(isaddr), ifp->if_xname,
			    rt->rt_ifp->if_xname);
			RTFREE_LOCKED(rt);
			goto drop;
		}
		RTFREE_LOCKED(rt);

#ifdef DEBUG_PROXY
		printf("arp: proxying for %s\n",
		       inet_ntoa(itaddr));
#endif
	}

	if (la != NULL)
		LLE_WUNLOCK(la);
	if (itaddr.s_addr == myaddr.s_addr &&
	    IN_LINKLOCAL(ntohl(itaddr.s_addr))) {
		/* RFC 3927 link-local IPv4; always reply by broadcast. */
#ifdef DEBUG_LINKLOCAL
		printf("arp: sending reply for link-local addr %s\n",
		    inet_ntoa(itaddr));
#endif
		m->m_flags |= M_BCAST;
		m->m_flags &= ~M_MCAST;
	} else {
		/* default behaviour; never reply by broadcast. */
		m->m_flags &= ~(M_BCAST|M_MCAST);
	}
	(void)memcpy(ar_tpa(ah), ar_spa(ah), ah->ar_pln);
	(void)memcpy(ar_spa(ah), &itaddr, ah->ar_pln);
	ah->ar_op = htons(ARPOP_REPLY);
	ah->ar_pro = htons(ETHERTYPE_IP); /* let's be sure! */
	m->m_len = sizeof(*ah) + (2 * ah->ar_pln) + (2 * ah->ar_hln);   
	m->m_pkthdr.len = m->m_len;   
	sa.sa_family = AF_ARP;
	sa.sa_len = 2;
	(*ifp->if_output)(ifp, m, &sa, (struct rtentry *)0);
	return;

drop:
	if (la != NULL)
		LLE_WUNLOCK(la);
	m_freem(m);
}
#endif

void
arp_ifinit(struct ifnet *ifp, struct ifaddr *ifa)
{
	struct llentry *lle;

	if (ntohl(IA_SIN(ifa)->sin_addr.s_addr) != INADDR_ANY) {
		arprequest(ifp, &IA_SIN(ifa)->sin_addr,
				&IA_SIN(ifa)->sin_addr, IF_LLADDR(ifp));
		/* 
		 * interface address is considered static entry
		 * because the output of the arp utility shows
		 * that L2 entry as permanent
		 */
		IF_AFDATA_LOCK(ifp);
		lle = lla_lookup(LLTABLE(ifp), (LLE_CREATE | LLE_IFADDR | LLE_STATIC),
				 (struct sockaddr *)IA_SIN(ifa));
		IF_AFDATA_UNLOCK(ifp);
		if (lle == NULL)
			log(LOG_INFO, "arp_ifinit: cannot create arp "
			    "entry for interface address\n");
		else
			LLE_RUNLOCK(lle);
	}
	ifa->ifa_rtrequest = NULL;
}

void
arp_ifinit2(struct ifnet *ifp, struct ifaddr *ifa, u_char *enaddr)
{
	if (ntohl(IA_SIN(ifa)->sin_addr.s_addr) != INADDR_ANY)
		arprequest(ifp, &IA_SIN(ifa)->sin_addr,
				&IA_SIN(ifa)->sin_addr, enaddr);
	ifa->ifa_rtrequest = NULL;
}

static void
arp_init(void)
{
	INIT_VNET_INET(curvnet);

	V_arpt_keep = (20*60); /* once resolved, good for 20 more minutes */
	V_arp_maxtries = 5;
	V_useloopback = 1; /* use loopback interface for local traffic */
	V_arp_proxyall = 0;

	arpintrq.ifq_maxlen = 50;
	mtx_init(&arpintrq.ifq_mtx, "arp_inq", NULL, MTX_DEF);
	netisr_register(NETISR_ARP, arpintr, &arpintrq, 0);
}
SYSINIT(arp, SI_SUB_PROTO_DOMAIN, SI_ORDER_ANY, arp_init, 0);
