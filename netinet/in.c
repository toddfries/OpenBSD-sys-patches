/*-
 * Copyright (c) 1982, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (C) 2001 WIDE Project.  All rights reserved.
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
 *	@(#)in.c	8.4 (Berkeley) 1/9/95
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/netinet/in.c,v 1.118 2009/01/09 21:57:49 bz Exp $");

#include "opt_carp.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/malloc.h>
#include <sys/priv.h>
#include <sys/socket.h>
#include <sys/jail.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/vimage.h>

#include <net/if.h>
#include <net/if_llatbl.h>
#include <net/if_types.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/vinet.h>

static int in_mask2len(struct in_addr *);
static void in_len2mask(struct in_addr *, int);
static int in_lifaddr_ioctl(struct socket *, u_long, caddr_t,
	struct ifnet *, struct thread *);

static int	in_addprefix(struct in_ifaddr *, int);
static int	in_scrubprefix(struct in_ifaddr *);
static void	in_socktrim(struct sockaddr_in *);
static int	in_ifinit(struct ifnet *,
	    struct in_ifaddr *, struct sockaddr_in *, int);
static void	in_purgemaddrs(struct ifnet *);

#ifdef VIMAGE_GLOBALS
static int subnetsarelocal;
static int sameprefixcarponly;
extern struct inpcbinfo ripcbinfo;
#endif

SYSCTL_V_INT(V_NET, vnet_inet, _net_inet_ip, OID_AUTO, subnets_are_local,
	CTLFLAG_RW, subnetsarelocal, 0,
	"Treat all subnets as directly connected");
SYSCTL_V_INT(V_NET, vnet_inet, _net_inet_ip, OID_AUTO, same_prefix_carp_only,
	CTLFLAG_RW, sameprefixcarponly, 0,
	"Refuse to create same prefixes on different interfaces");

/*
 * Return 1 if an internet address is for a ``local'' host
 * (one to which we have a connection).  If subnetsarelocal
 * is true, this includes other subnets of the local net.
 * Otherwise, it includes only the directly-connected (sub)nets.
 */
int
in_localaddr(struct in_addr in)
{
	INIT_VNET_INET(curvnet);
	register u_long i = ntohl(in.s_addr);
	register struct in_ifaddr *ia;

	if (V_subnetsarelocal) {
		TAILQ_FOREACH(ia, &V_in_ifaddrhead, ia_link)
			if ((i & ia->ia_netmask) == ia->ia_net)
				return (1);
	} else {
		TAILQ_FOREACH(ia, &V_in_ifaddrhead, ia_link)
			if ((i & ia->ia_subnetmask) == ia->ia_subnet)
				return (1);
	}
	return (0);
}

/*
 * Return 1 if an internet address is for the local host and configured
 * on one of its interfaces.
 */
int
in_localip(struct in_addr in)
{
	INIT_VNET_INET(curvnet);
	struct in_ifaddr *ia;

	LIST_FOREACH(ia, INADDR_HASH(in.s_addr), ia_hash) {
		if (IA_SIN(ia)->sin_addr.s_addr == in.s_addr)
			return (1);
	}
	return (0);
}

/*
 * Determine whether an IP address is in a reserved set of addresses
 * that may not be forwarded, or whether datagrams to that destination
 * may be forwarded.
 */
int
in_canforward(struct in_addr in)
{
	register u_long i = ntohl(in.s_addr);
	register u_long net;

	if (IN_EXPERIMENTAL(i) || IN_MULTICAST(i) || IN_LINKLOCAL(i))
		return (0);
	if (IN_CLASSA(i)) {
		net = i & IN_CLASSA_NET;
		if (net == 0 || net == (IN_LOOPBACKNET << IN_CLASSA_NSHIFT))
			return (0);
	}
	return (1);
}

/*
 * Trim a mask in a sockaddr
 */
static void
in_socktrim(struct sockaddr_in *ap)
{
    register char *cplim = (char *) &ap->sin_addr;
    register char *cp = (char *) (&ap->sin_addr + 1);

    ap->sin_len = 0;
    while (--cp >= cplim)
	if (*cp) {
	    (ap)->sin_len = cp - (char *) (ap) + 1;
	    break;
	}
}

static int
in_mask2len(mask)
	struct in_addr *mask;
{
	int x, y;
	u_char *p;

	p = (u_char *)mask;
	for (x = 0; x < sizeof(*mask); x++) {
		if (p[x] != 0xff)
			break;
	}
	y = 0;
	if (x < sizeof(*mask)) {
		for (y = 0; y < 8; y++) {
			if ((p[x] & (0x80 >> y)) == 0)
				break;
		}
	}
	return (x * 8 + y);
}

static void
in_len2mask(struct in_addr *mask, int len)
{
	int i;
	u_char *p;

	p = (u_char *)mask;
	bzero(mask, sizeof(*mask));
	for (i = 0; i < len / 8; i++)
		p[i] = 0xff;
	if (len % 8)
		p[i] = (0xff00 >> (len % 8)) & 0xff;
}

/*
 * Generic internet control operations (ioctl's).
 * Ifp is 0 if not an interface-specific ioctl.
 */
/* ARGSUSED */
int
in_control(struct socket *so, u_long cmd, caddr_t data, struct ifnet *ifp,
    struct thread *td)
{
	INIT_VNET_INET(curvnet); /* both so and ifp can be NULL here! */
	register struct ifreq *ifr = (struct ifreq *)data;
	register struct in_ifaddr *ia, *iap;
	register struct ifaddr *ifa;
	struct in_addr allhosts_addr;
	struct in_addr dst;
	struct in_ifaddr *oia;
	struct in_aliasreq *ifra = (struct in_aliasreq *)data;
	struct sockaddr_in oldaddr;
	int error, hostIsNew, iaIsNew, maskIsNew, s;
	int iaIsFirst;

	ia = NULL;
	iaIsFirst = 0;
	iaIsNew = 0;
	allhosts_addr.s_addr = htonl(INADDR_ALLHOSTS_GROUP);

	switch (cmd) {
	case SIOCALIFADDR:
		if (td != NULL) {
			error = priv_check(td, PRIV_NET_ADDIFADDR);
			if (error)
				return (error);
		}
		if (ifp == NULL)
			return (EINVAL);
		return in_lifaddr_ioctl(so, cmd, data, ifp, td);

	case SIOCDLIFADDR:
		if (td != NULL) {
			error = priv_check(td, PRIV_NET_DELIFADDR);
			if (error)
				return (error);
		}
		if (ifp == NULL)
			return (EINVAL);
		return in_lifaddr_ioctl(so, cmd, data, ifp, td);

	case SIOCGLIFADDR:
		if (ifp == NULL)
			return (EINVAL);
		return in_lifaddr_ioctl(so, cmd, data, ifp, td);
	}

	/*
	 * Find address for this interface, if it exists.
	 *
	 * If an alias address was specified, find that one instead of
	 * the first one on the interface, if possible.
	 */
	if (ifp != NULL) {
		dst = ((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr;
		LIST_FOREACH(iap, INADDR_HASH(dst.s_addr), ia_hash)
			if (iap->ia_ifp == ifp &&
			    iap->ia_addr.sin_addr.s_addr == dst.s_addr) {
				if (td == NULL || prison_check_ip4(
				    td->td_ucred, &dst))
					ia = iap;
				break;
			}
		if (ia == NULL)
			TAILQ_FOREACH(ifa, &ifp->if_addrhead, ifa_link) {
				iap = ifatoia(ifa);
				if (iap->ia_addr.sin_family == AF_INET) {
					if (td != NULL &&
					    !prison_check_ip4(td->td_ucred,
					    &iap->ia_addr.sin_addr))
						continue;
					ia = iap;
					break;
				}
			}
		if (ia == NULL)
			iaIsFirst = 1;
	}

	switch (cmd) {

	case SIOCAIFADDR:
	case SIOCDIFADDR:
		if (ifp == NULL)
			return (EADDRNOTAVAIL);
		if (ifra->ifra_addr.sin_family == AF_INET) {
			for (oia = ia; ia; ia = TAILQ_NEXT(ia, ia_link)) {
				if (ia->ia_ifp == ifp  &&
				    ia->ia_addr.sin_addr.s_addr ==
				    ifra->ifra_addr.sin_addr.s_addr)
					break;
			}
			if ((ifp->if_flags & IFF_POINTOPOINT)
			    && (cmd == SIOCAIFADDR)
			    && (ifra->ifra_dstaddr.sin_addr.s_addr
				== INADDR_ANY)) {
				return (EDESTADDRREQ);
			}
		}
		if (cmd == SIOCDIFADDR && ia == NULL)
			return (EADDRNOTAVAIL);
		/* FALLTHROUGH */
	case SIOCSIFADDR:
	case SIOCSIFNETMASK:
	case SIOCSIFDSTADDR:
		if (td != NULL) {
			error = priv_check(td, (cmd == SIOCDIFADDR) ? 
			    PRIV_NET_DELIFADDR : PRIV_NET_ADDIFADDR);
			if (error)
				return (error);
		}

		if (ifp == NULL)
			return (EADDRNOTAVAIL);
		if (ia == NULL) {
			ia = (struct in_ifaddr *)
				malloc(sizeof *ia, M_IFADDR, M_WAITOK | M_ZERO);
			if (ia == NULL)
				return (ENOBUFS);
			/*
			 * Protect from ipintr() traversing address list
			 * while we're modifying it.
			 */
			s = splnet();
			ifa = &ia->ia_ifa;
			IFA_LOCK_INIT(ifa);
			ifa->ifa_addr = (struct sockaddr *)&ia->ia_addr;
			ifa->ifa_dstaddr = (struct sockaddr *)&ia->ia_dstaddr;
			ifa->ifa_netmask = (struct sockaddr *)&ia->ia_sockmask;
			ifa->ifa_refcnt = 1;
			TAILQ_INSERT_TAIL(&ifp->if_addrhead, ifa, ifa_link);

			ia->ia_sockmask.sin_len = 8;
			ia->ia_sockmask.sin_family = AF_INET;
			if (ifp->if_flags & IFF_BROADCAST) {
				ia->ia_broadaddr.sin_len = sizeof(ia->ia_addr);
				ia->ia_broadaddr.sin_family = AF_INET;
			}
			ia->ia_ifp = ifp;

			TAILQ_INSERT_TAIL(&V_in_ifaddrhead, ia, ia_link);
			splx(s);
			iaIsNew = 1;
		}
		break;

	case SIOCSIFBRDADDR:
		if (td != NULL) {
			error = priv_check(td, PRIV_NET_ADDIFADDR);
			if (error)
				return (error);
		}
		/* FALLTHROUGH */

	case SIOCGIFADDR:
	case SIOCGIFNETMASK:
	case SIOCGIFDSTADDR:
	case SIOCGIFBRDADDR:
		if (ia == NULL)
			return (EADDRNOTAVAIL);
		break;
	}
	switch (cmd) {

	case SIOCGIFADDR:
		*((struct sockaddr_in *)&ifr->ifr_addr) = ia->ia_addr;
		return (0);

	case SIOCGIFBRDADDR:
		if ((ifp->if_flags & IFF_BROADCAST) == 0)
			return (EINVAL);
		*((struct sockaddr_in *)&ifr->ifr_dstaddr) = ia->ia_broadaddr;
		return (0);

	case SIOCGIFDSTADDR:
		if ((ifp->if_flags & IFF_POINTOPOINT) == 0)
			return (EINVAL);
		*((struct sockaddr_in *)&ifr->ifr_dstaddr) = ia->ia_dstaddr;
		return (0);

	case SIOCGIFNETMASK:
		*((struct sockaddr_in *)&ifr->ifr_addr) = ia->ia_sockmask;
		return (0);

	case SIOCSIFDSTADDR:
		if ((ifp->if_flags & IFF_POINTOPOINT) == 0)
			return (EINVAL);
		oldaddr = ia->ia_dstaddr;
		ia->ia_dstaddr = *(struct sockaddr_in *)&ifr->ifr_dstaddr;
		if (ifp->if_ioctl != NULL) {
			IFF_LOCKGIANT(ifp);
			error = (*ifp->if_ioctl)(ifp, SIOCSIFDSTADDR,
			    (caddr_t)ia);
			IFF_UNLOCKGIANT(ifp);
			if (error) {
				ia->ia_dstaddr = oldaddr;
				return (error);
			}
		}
		if (ia->ia_flags & IFA_ROUTE) {
			ia->ia_ifa.ifa_dstaddr = (struct sockaddr *)&oldaddr;
			rtinit(&(ia->ia_ifa), (int)RTM_DELETE, RTF_HOST);
			ia->ia_ifa.ifa_dstaddr =
					(struct sockaddr *)&ia->ia_dstaddr;
			rtinit(&(ia->ia_ifa), (int)RTM_ADD, RTF_HOST|RTF_UP);
		}
		return (0);

	case SIOCSIFBRDADDR:
		if ((ifp->if_flags & IFF_BROADCAST) == 0)
			return (EINVAL);
		ia->ia_broadaddr = *(struct sockaddr_in *)&ifr->ifr_broadaddr;
		return (0);

	case SIOCSIFADDR:
		error = in_ifinit(ifp, ia,
		    (struct sockaddr_in *) &ifr->ifr_addr, 1);
		if (error != 0 && iaIsNew)
			break;
		if (error == 0) {
			if (iaIsFirst && (ifp->if_flags & IFF_MULTICAST) != 0)
				in_addmulti(&allhosts_addr, ifp);
			EVENTHANDLER_INVOKE(ifaddr_event, ifp);
		}
		return (0);

	case SIOCSIFNETMASK:
		ia->ia_sockmask.sin_addr = ifra->ifra_addr.sin_addr;
		ia->ia_subnetmask = ntohl(ia->ia_sockmask.sin_addr.s_addr);
		return (0);

	case SIOCAIFADDR:
		maskIsNew = 0;
		hostIsNew = 1;
		error = 0;
		if (ia->ia_addr.sin_family == AF_INET) {
			if (ifra->ifra_addr.sin_len == 0) {
				ifra->ifra_addr = ia->ia_addr;
				hostIsNew = 0;
			} else if (ifra->ifra_addr.sin_addr.s_addr ==
					       ia->ia_addr.sin_addr.s_addr)
				hostIsNew = 0;
		}
		if (ifra->ifra_mask.sin_len) {
			in_ifscrub(ifp, ia);
			ia->ia_sockmask = ifra->ifra_mask;
			ia->ia_sockmask.sin_family = AF_INET;
			ia->ia_subnetmask =
			     ntohl(ia->ia_sockmask.sin_addr.s_addr);
			maskIsNew = 1;
		}
		if ((ifp->if_flags & IFF_POINTOPOINT) &&
		    (ifra->ifra_dstaddr.sin_family == AF_INET)) {
			in_ifscrub(ifp, ia);
			ia->ia_dstaddr = ifra->ifra_dstaddr;
			maskIsNew  = 1; /* We lie; but the effect's the same */
		}
		if (ifra->ifra_addr.sin_family == AF_INET &&
		    (hostIsNew || maskIsNew))
			error = in_ifinit(ifp, ia, &ifra->ifra_addr, 0);
		if (error != 0 && iaIsNew)
			break;

		if ((ifp->if_flags & IFF_BROADCAST) &&
		    (ifra->ifra_broadaddr.sin_family == AF_INET))
			ia->ia_broadaddr = ifra->ifra_broadaddr;
		if (error == 0) {
			if (iaIsFirst && (ifp->if_flags & IFF_MULTICAST) != 0)
				in_addmulti(&allhosts_addr, ifp);
			EVENTHANDLER_INVOKE(ifaddr_event, ifp);
		}
		return (error);

	case SIOCDIFADDR:
		/*
		 * in_ifscrub kills the interface route.
		 */
		in_ifscrub(ifp, ia);
		/*
		 * in_ifadown gets rid of all the rest of
		 * the routes.  This is not quite the right
		 * thing to do, but at least if we are running
		 * a routing process they will come back.
		 */
		in_ifadown(&ia->ia_ifa, 1);
		EVENTHANDLER_INVOKE(ifaddr_event, ifp);
		error = 0;
		break;

	default:
		if (ifp == NULL || ifp->if_ioctl == NULL)
			return (EOPNOTSUPP);
		IFF_LOCKGIANT(ifp);
		error = (*ifp->if_ioctl)(ifp, cmd, data);
		IFF_UNLOCKGIANT(ifp);
		return (error);
	}

	/*
	 * Protect from ipintr() traversing address list while we're modifying
	 * it.
	 */
	s = splnet();
	TAILQ_REMOVE(&ifp->if_addrhead, &ia->ia_ifa, ifa_link);
	TAILQ_REMOVE(&V_in_ifaddrhead, ia, ia_link);
	if (ia->ia_addr.sin_family == AF_INET) {
		LIST_REMOVE(ia, ia_hash);
		/*
		 * If this is the last IPv4 address configured on this
		 * interface, leave the all-hosts group.
		 * XXX: This is quite ugly because of locking and structure.
		 */
		oia = NULL;
		IFP_TO_IA(ifp, oia);
		if (oia == NULL) {
			struct in_multi *inm;

			IFF_LOCKGIANT(ifp);
			IN_MULTI_LOCK();
			IN_LOOKUP_MULTI(allhosts_addr, ifp, inm);
			if (inm != NULL)
				in_delmulti_locked(inm);
			IN_MULTI_UNLOCK();
			IFF_UNLOCKGIANT(ifp);
		}
	}
	IFAFREE(&ia->ia_ifa);
	splx(s);

	return (error);
}

/*
 * SIOC[GAD]LIFADDR.
 *	SIOCGLIFADDR: get first address. (?!?)
 *	SIOCGLIFADDR with IFLR_PREFIX:
 *		get first address that matches the specified prefix.
 *	SIOCALIFADDR: add the specified address.
 *	SIOCALIFADDR with IFLR_PREFIX:
 *		EINVAL since we can't deduce hostid part of the address.
 *	SIOCDLIFADDR: delete the specified address.
 *	SIOCDLIFADDR with IFLR_PREFIX:
 *		delete the first address that matches the specified prefix.
 * return values:
 *	EINVAL on invalid parameters
 *	EADDRNOTAVAIL on prefix match failed/specified address not found
 *	other values may be returned from in_ioctl()
 */
static int
in_lifaddr_ioctl(struct socket *so, u_long cmd, caddr_t data,
    struct ifnet *ifp, struct thread *td)
{
	struct if_laddrreq *iflr = (struct if_laddrreq *)data;
	struct ifaddr *ifa;

	/* sanity checks */
	if (data == NULL || ifp == NULL) {
		panic("invalid argument to in_lifaddr_ioctl");
		/*NOTRECHED*/
	}

	switch (cmd) {
	case SIOCGLIFADDR:
		/* address must be specified on GET with IFLR_PREFIX */
		if ((iflr->flags & IFLR_PREFIX) == 0)
			break;
		/*FALLTHROUGH*/
	case SIOCALIFADDR:
	case SIOCDLIFADDR:
		/* address must be specified on ADD and DELETE */
		if (iflr->addr.ss_family != AF_INET)
			return (EINVAL);
		if (iflr->addr.ss_len != sizeof(struct sockaddr_in))
			return (EINVAL);
		/* XXX need improvement */
		if (iflr->dstaddr.ss_family
		 && iflr->dstaddr.ss_family != AF_INET)
			return (EINVAL);
		if (iflr->dstaddr.ss_family
		 && iflr->dstaddr.ss_len != sizeof(struct sockaddr_in))
			return (EINVAL);
		break;
	default: /*shouldn't happen*/
		return (EOPNOTSUPP);
	}
	if (sizeof(struct in_addr) * 8 < iflr->prefixlen)
		return (EINVAL);

	switch (cmd) {
	case SIOCALIFADDR:
	    {
		struct in_aliasreq ifra;

		if (iflr->flags & IFLR_PREFIX)
			return (EINVAL);

		/* copy args to in_aliasreq, perform ioctl(SIOCAIFADDR_IN6). */
		bzero(&ifra, sizeof(ifra));
		bcopy(iflr->iflr_name, ifra.ifra_name,
			sizeof(ifra.ifra_name));

		bcopy(&iflr->addr, &ifra.ifra_addr, iflr->addr.ss_len);

		if (iflr->dstaddr.ss_family) {	/*XXX*/
			bcopy(&iflr->dstaddr, &ifra.ifra_dstaddr,
				iflr->dstaddr.ss_len);
		}

		ifra.ifra_mask.sin_family = AF_INET;
		ifra.ifra_mask.sin_len = sizeof(struct sockaddr_in);
		in_len2mask(&ifra.ifra_mask.sin_addr, iflr->prefixlen);

		return (in_control(so, SIOCAIFADDR, (caddr_t)&ifra, ifp, td));
	    }
	case SIOCGLIFADDR:
	case SIOCDLIFADDR:
	    {
		struct in_ifaddr *ia;
		struct in_addr mask, candidate, match;
		struct sockaddr_in *sin;

		bzero(&mask, sizeof(mask));
		bzero(&match, sizeof(match));
		if (iflr->flags & IFLR_PREFIX) {
			/* lookup a prefix rather than address. */
			in_len2mask(&mask, iflr->prefixlen);

			sin = (struct sockaddr_in *)&iflr->addr;
			match.s_addr = sin->sin_addr.s_addr;
			match.s_addr &= mask.s_addr;

			/* if you set extra bits, that's wrong */
			if (match.s_addr != sin->sin_addr.s_addr)
				return (EINVAL);

		} else {
			/* on getting an address, take the 1st match */
			/* on deleting an address, do exact match */
			if (cmd != SIOCGLIFADDR) {
				in_len2mask(&mask, 32);
				sin = (struct sockaddr_in *)&iflr->addr;
				match.s_addr = sin->sin_addr.s_addr;
			}
		}

		TAILQ_FOREACH(ifa, &ifp->if_addrhead, ifa_link)	{
			if (ifa->ifa_addr->sa_family != AF_INET6)
				continue;
			if (match.s_addr == 0)
				break;
			candidate.s_addr = ((struct sockaddr_in *)&ifa->ifa_addr)->sin_addr.s_addr;
			candidate.s_addr &= mask.s_addr;
			if (candidate.s_addr == match.s_addr)
				break;
		}
		if (ifa == NULL)
			return (EADDRNOTAVAIL);
		ia = (struct in_ifaddr *)ifa;

		if (cmd == SIOCGLIFADDR) {
			/* fill in the if_laddrreq structure */
			bcopy(&ia->ia_addr, &iflr->addr, ia->ia_addr.sin_len);

			if ((ifp->if_flags & IFF_POINTOPOINT) != 0) {
				bcopy(&ia->ia_dstaddr, &iflr->dstaddr,
					ia->ia_dstaddr.sin_len);
			} else
				bzero(&iflr->dstaddr, sizeof(iflr->dstaddr));

			iflr->prefixlen =
				in_mask2len(&ia->ia_sockmask.sin_addr);

			iflr->flags = 0;	/*XXX*/

			return (0);
		} else {
			struct in_aliasreq ifra;

			/* fill in_aliasreq and do ioctl(SIOCDIFADDR_IN6) */
			bzero(&ifra, sizeof(ifra));
			bcopy(iflr->iflr_name, ifra.ifra_name,
				sizeof(ifra.ifra_name));

			bcopy(&ia->ia_addr, &ifra.ifra_addr,
				ia->ia_addr.sin_len);
			if ((ifp->if_flags & IFF_POINTOPOINT) != 0) {
				bcopy(&ia->ia_dstaddr, &ifra.ifra_dstaddr,
					ia->ia_dstaddr.sin_len);
			}
			bcopy(&ia->ia_sockmask, &ifra.ifra_dstaddr,
				ia->ia_sockmask.sin_len);

			return (in_control(so, SIOCDIFADDR, (caddr_t)&ifra,
			    ifp, td));
		}
	    }
	}

	return (EOPNOTSUPP);	/*just for safety*/
}

/*
 * Delete any existing route for an interface.
 */
void
in_ifscrub(struct ifnet *ifp, struct in_ifaddr *ia)
{

	in_scrubprefix(ia);
}

/*
 * Initialize an interface's internet address
 * and routing table entry.
 */
static int
in_ifinit(struct ifnet *ifp, struct in_ifaddr *ia, struct sockaddr_in *sin,
    int scrub)
{
	INIT_VNET_INET(ifp->if_vnet);
	register u_long i = ntohl(sin->sin_addr.s_addr);
	struct sockaddr_in oldaddr;
	int s = splimp(), flags = RTF_UP, error = 0;

	oldaddr = ia->ia_addr;
	if (oldaddr.sin_family == AF_INET)
		LIST_REMOVE(ia, ia_hash);
	ia->ia_addr = *sin;
	if (ia->ia_addr.sin_family == AF_INET)
		LIST_INSERT_HEAD(INADDR_HASH(ia->ia_addr.sin_addr.s_addr),
		    ia, ia_hash);
	/*
	 * Give the interface a chance to initialize
	 * if this is its first address,
	 * and to validate the address if necessary.
	 */
	if (ifp->if_ioctl != NULL) {
		IFF_LOCKGIANT(ifp);
		error = (*ifp->if_ioctl)(ifp, SIOCSIFADDR, (caddr_t)ia);
		IFF_UNLOCKGIANT(ifp);
		if (error) {
			splx(s);
			/* LIST_REMOVE(ia, ia_hash) is done in in_control */
			ia->ia_addr = oldaddr;
			if (ia->ia_addr.sin_family == AF_INET)
				LIST_INSERT_HEAD(INADDR_HASH(
				    ia->ia_addr.sin_addr.s_addr), ia, ia_hash);
			else 
				/* 
				 * If oldaddr family is not AF_INET (e.g. 
				 * interface has been just created) in_control 
				 * does not call LIST_REMOVE, and we end up 
				 * with bogus ia entries in hash
				 */
				LIST_REMOVE(ia, ia_hash);
			return (error);
		}
	}
	splx(s);
	if (scrub) {
		ia->ia_ifa.ifa_addr = (struct sockaddr *)&oldaddr;
		in_ifscrub(ifp, ia);
		ia->ia_ifa.ifa_addr = (struct sockaddr *)&ia->ia_addr;
	}
	if (IN_CLASSA(i))
		ia->ia_netmask = IN_CLASSA_NET;
	else if (IN_CLASSB(i))
		ia->ia_netmask = IN_CLASSB_NET;
	else
		ia->ia_netmask = IN_CLASSC_NET;
	/*
	 * The subnet mask usually includes at least the standard network part,
	 * but may may be smaller in the case of supernetting.
	 * If it is set, we believe it.
	 */
	if (ia->ia_subnetmask == 0) {
		ia->ia_subnetmask = ia->ia_netmask;
		ia->ia_sockmask.sin_addr.s_addr = htonl(ia->ia_subnetmask);
	} else
		ia->ia_netmask &= ia->ia_subnetmask;
	ia->ia_net = i & ia->ia_netmask;
	ia->ia_subnet = i & ia->ia_subnetmask;
	in_socktrim(&ia->ia_sockmask);
#ifdef DEV_CARP
	/*
	 * XXX: carp(4) does not have interface route
	 */
	if (ifp->if_type == IFT_CARP)
		return (0);
#endif
	/*
	 * Add route for the network.
	 */
	ia->ia_ifa.ifa_metric = ifp->if_metric;
	if (ifp->if_flags & IFF_BROADCAST) {
		ia->ia_broadaddr.sin_addr.s_addr =
			htonl(ia->ia_subnet | ~ia->ia_subnetmask);
		ia->ia_netbroadcast.s_addr =
			htonl(ia->ia_net | ~ ia->ia_netmask);
	} else if (ifp->if_flags & IFF_LOOPBACK) {
		ia->ia_dstaddr = ia->ia_addr;
		flags |= RTF_HOST;
	} else if (ifp->if_flags & IFF_POINTOPOINT) {
		if (ia->ia_dstaddr.sin_family != AF_INET)
			return (0);
		flags |= RTF_HOST;
	}
	if ((error = in_addprefix(ia, flags)) != 0)
		return (error);

	return (error);
}

#define rtinitflags(x) \
	((((x)->ia_ifp->if_flags & (IFF_LOOPBACK | IFF_POINTOPOINT)) != 0) \
	    ? RTF_HOST : 0)
/*
 * Check if we have a route for the given prefix already or add one accordingly.
 */
static int
in_addprefix(struct in_ifaddr *target, int flags)
{
	INIT_VNET_INET(curvnet);
	struct in_ifaddr *ia;
	struct in_addr prefix, mask, p, m;
	int error;

	if ((flags & RTF_HOST) != 0) {
		prefix = target->ia_dstaddr.sin_addr;
		mask.s_addr = 0;
	} else {
		prefix = target->ia_addr.sin_addr;
		mask = target->ia_sockmask.sin_addr;
		prefix.s_addr &= mask.s_addr;
	}

	TAILQ_FOREACH(ia, &V_in_ifaddrhead, ia_link) {
		if (rtinitflags(ia)) {
			p = ia->ia_addr.sin_addr;

			if (prefix.s_addr != p.s_addr)
				continue;
		} else {
			p = ia->ia_addr.sin_addr;
			m = ia->ia_sockmask.sin_addr;
			p.s_addr &= m.s_addr;

			if (prefix.s_addr != p.s_addr ||
			    mask.s_addr != m.s_addr)
				continue;
		}

		/*
		 * If we got a matching prefix route inserted by other
		 * interface address, we are done here.
		 */
		if (ia->ia_flags & IFA_ROUTE) {
			if (V_sameprefixcarponly &&
			    target->ia_ifp->if_type != IFT_CARP &&
			    ia->ia_ifp->if_type != IFT_CARP)
				return (EEXIST);
			else
				return (0);
		}
	}

	/*
	 * No-one seem to have this prefix route, so we try to insert it.
	 */
	error = rtinit(&target->ia_ifa, (int)RTM_ADD, flags);
	if (!error)
		target->ia_flags |= IFA_ROUTE;
	return (error);
}

extern void arp_ifscrub(struct ifnet *ifp, uint32_t addr);

/*
 * If there is no other address in the system that can serve a route to the
 * same prefix, remove the route.  Hand over the route to the new address
 * otherwise.
 */
static int
in_scrubprefix(struct in_ifaddr *target)
{
	INIT_VNET_INET(curvnet);
	struct in_ifaddr *ia;
	struct in_addr prefix, mask, p;
	int error;

	if ((target->ia_flags & IFA_ROUTE) == 0)
		return (0);

	if (rtinitflags(target))
		prefix = target->ia_dstaddr.sin_addr;
	else {
		prefix = target->ia_addr.sin_addr;
		mask = target->ia_sockmask.sin_addr;
		prefix.s_addr &= mask.s_addr;
		/* remove arp cache */
		arp_ifscrub(target->ia_ifp, IA_SIN(target)->sin_addr.s_addr);
	}

	TAILQ_FOREACH(ia, &V_in_ifaddrhead, ia_link) {
		if (rtinitflags(ia))
			p = ia->ia_dstaddr.sin_addr;
		else {
			p = ia->ia_addr.sin_addr;
			p.s_addr &= ia->ia_sockmask.sin_addr.s_addr;
		}

		if (prefix.s_addr != p.s_addr)
			continue;

		/*
		 * If we got a matching prefix address, move IFA_ROUTE and
		 * the route itself to it.  Make sure that routing daemons
		 * get a heads-up.
		 *
		 * XXX: a special case for carp(4) interface
		 */
		if ((ia->ia_flags & IFA_ROUTE) == 0
#ifdef DEV_CARP
		    && (ia->ia_ifp->if_type != IFT_CARP)
#endif
							) {
			rtinit(&(target->ia_ifa), (int)RTM_DELETE,
			    rtinitflags(target));
			target->ia_flags &= ~IFA_ROUTE;

			error = rtinit(&ia->ia_ifa, (int)RTM_ADD,
			    rtinitflags(ia) | RTF_UP);
			if (error == 0)
				ia->ia_flags |= IFA_ROUTE;
			return (error);
		}
	}

	/*
	 * As no-one seem to have this prefix, we can remove the route.
	 */
	rtinit(&(target->ia_ifa), (int)RTM_DELETE, rtinitflags(target));
	target->ia_flags &= ~IFA_ROUTE;
	return (0);
}

#undef rtinitflags

/*
 * Return 1 if the address might be a local broadcast address.
 */
int
in_broadcast(struct in_addr in, struct ifnet *ifp)
{
	register struct ifaddr *ifa;
	u_long t;

	if (in.s_addr == INADDR_BROADCAST ||
	    in.s_addr == INADDR_ANY)
		return (1);
	if ((ifp->if_flags & IFF_BROADCAST) == 0)
		return (0);
	t = ntohl(in.s_addr);
	/*
	 * Look through the list of addresses for a match
	 * with a broadcast address.
	 */
#define ia ((struct in_ifaddr *)ifa)
	TAILQ_FOREACH(ifa, &ifp->if_addrhead, ifa_link)
		if (ifa->ifa_addr->sa_family == AF_INET &&
		    (in.s_addr == ia->ia_broadaddr.sin_addr.s_addr ||
		     in.s_addr == ia->ia_netbroadcast.s_addr ||
		     /*
		      * Check for old-style (host 0) broadcast.
		      */
		     t == ia->ia_subnet || t == ia->ia_net) &&
		     /*
		      * Check for an all one subnetmask. These
		      * only exist when an interface gets a secondary
		      * address.
		      */
		     ia->ia_subnetmask != (u_long)0xffffffff)
			    return (1);
	return (0);
#undef ia
}

/*
 * Delete all IPv4 multicast address records, and associated link-layer
 * multicast address records, associated with ifp.
 */
static void
in_purgemaddrs(struct ifnet *ifp)
{
	INIT_VNET_INET(ifp->if_vnet);
	struct in_multi *inm;
	struct in_multi *oinm;

#ifdef DIAGNOSTIC
	printf("%s: purging ifp %p\n", __func__, ifp);
#endif
	IFF_LOCKGIANT(ifp);
	IN_MULTI_LOCK();
	LIST_FOREACH_SAFE(inm, &V_in_multihead, inm_link, oinm) {
		if (inm->inm_ifp == ifp)
			in_delmulti_locked(inm);
	}
	IN_MULTI_UNLOCK();
	IFF_UNLOCKGIANT(ifp);
}

/*
 * On interface removal, clean up IPv4 data structures hung off of the ifnet.
 */
void
in_ifdetach(struct ifnet *ifp)
{
	INIT_VNET_INET(ifp->if_vnet);

	in_pcbpurgeif0(&V_ripcbinfo, ifp);
	in_pcbpurgeif0(&V_udbinfo, ifp);
	in_purgemaddrs(ifp);
}

#include <sys/syslog.h>
#include <net/if_dl.h>
#include <netinet/if_ether.h>

struct in_llentry {
	struct llentry		base;
	struct sockaddr_in	l3_addr4;
};

static struct llentry *
in_lltable_new(const struct sockaddr *l3addr, u_int flags)
{
	struct in_llentry *lle;

	lle = malloc(sizeof(struct in_llentry), M_LLTABLE, M_DONTWAIT | M_ZERO);
	if (lle == NULL)		/* NB: caller generates msg */
		return NULL;

	callout_init(&lle->base.la_timer, CALLOUT_MPSAFE);
	/*
	 * For IPv4 this will trigger "arpresolve" to generate
	 * an ARP request.
	 */
	lle->base.la_expire = time_second; /* mark expired */
	lle->l3_addr4 = *(const struct sockaddr_in *)l3addr;
	lle->base.lle_refcnt = 1;
	LLE_LOCK_INIT(&lle->base);
	return &lle->base;
}

/*
 * Deletes an address from the address table.
 * This function is called by the timer functions
 * such as arptimer() and nd6_llinfo_timer(), and
 * the caller does the locking.
 */
static void
in_lltable_free(struct lltable *llt, struct llentry *lle)
{
	LLE_WUNLOCK(lle);
	LLE_LOCK_DESTROY(lle);
	free(lle, M_LLTABLE);
}

static int
in_lltable_rtcheck(struct ifnet *ifp, const struct sockaddr *l3addr)
{
	struct rtentry *rt;

	KASSERT(l3addr->sa_family == AF_INET,
	    ("sin_family %d", l3addr->sa_family));

	/* XXX rtalloc1 should take a const param */
	rt = rtalloc1(__DECONST(struct sockaddr *, l3addr), 0, 0);
	if (rt == NULL || (rt->rt_flags & RTF_GATEWAY) || rt->rt_ifp != ifp) {
		log(LOG_INFO, "IPv4 address: \"%s\" is not on the network\n",
		    inet_ntoa(((const struct sockaddr_in *)l3addr)->sin_addr));
		if (rt != NULL)
			RTFREE_LOCKED(rt);
		return (EINVAL);
	}
	RTFREE_LOCKED(rt);
	return 0;
}

/*
 * Return NULL if not found or marked for deletion.
 * If found return lle read locked.
 */
static struct llentry *
in_lltable_lookup(struct lltable *llt, u_int flags, const struct sockaddr *l3addr)
{
	const struct sockaddr_in *sin = (const struct sockaddr_in *)l3addr;
	struct ifnet *ifp = llt->llt_ifp;
	struct llentry *lle;
	struct llentries *lleh;
	u_int hashkey;

	IF_AFDATA_LOCK_ASSERT(ifp);
	KASSERT(l3addr->sa_family == AF_INET,
	    ("sin_family %d", l3addr->sa_family));

	hashkey = sin->sin_addr.s_addr;
	lleh = &llt->lle_head[LLATBL_HASH(hashkey, LLTBL_HASHMASK)];
	LIST_FOREACH(lle, lleh, lle_next) {
		struct sockaddr_in *sa2 = (struct sockaddr_in *)L3_ADDR(lle);
		if (lle->la_flags & LLE_DELETED)
			continue;
		if (sa2->sin_addr.s_addr == sin->sin_addr.s_addr)
			break;
	}
	if (lle == NULL) {
#ifdef DIAGNOSTICS
		if (flags & LLE_DELETE)
			log(LOG_INFO, "interface address is missing from cache = %p  in delete\n", lle);	
#endif
		if (!(flags & LLE_CREATE))
			return (NULL);
		/*
		 * A route that covers the given address must have
		 * been installed 1st because we are doing a resolution,
		 * verify this.
		 */
		if (!(flags & LLE_IFADDR) &&
		    in_lltable_rtcheck(ifp, l3addr) != 0)
			goto done;

		lle = in_lltable_new(l3addr, flags);
		if (lle == NULL) {
			log(LOG_INFO, "lla_lookup: new lle malloc failed\n");
			goto done;
		}
		lle->la_flags = flags & ~LLE_CREATE;
		if ((flags & (LLE_CREATE | LLE_IFADDR)) == (LLE_CREATE | LLE_IFADDR)) {
			bcopy(IF_LLADDR(ifp), &lle->ll_addr, ifp->if_addrlen);
			lle->la_flags |= (LLE_VALID | LLE_STATIC);
		}

		lle->lle_tbl  = llt;
		lle->lle_head = lleh;
		LIST_INSERT_HEAD(lleh, lle, lle_next);
	} else if (flags & LLE_DELETE) {
		if (!(lle->la_flags & LLE_IFADDR) || (flags & LLE_IFADDR)) {
			LLE_WLOCK(lle);
			lle->la_flags = LLE_DELETED;
			LLE_WUNLOCK(lle);
#ifdef DIAGNOSTICS
			log(LOG_INFO, "ifaddr cache = %p  is deleted\n", lle);	
#endif
		}
		lle = (void *)-1;
		
	}
	if (LLE_IS_VALID(lle)) {
		if (flags & LLE_EXCLUSIVE)
			LLE_WLOCK(lle);
		else
			LLE_RLOCK(lle);
	}
done:
	return (lle);
}

static int
in_lltable_dump(struct lltable *llt, struct sysctl_req *wr)
{
#define	SIN(lle)	((struct sockaddr_in *) L3_ADDR(lle))
	struct ifnet *ifp = llt->llt_ifp;
	struct llentry *lle;
	/* XXX stack use */
	struct {
		struct rt_msghdr	rtm;
		struct sockaddr_inarp	sin;
		struct sockaddr_dl	sdl;
	} arpc;
	int error, i;

	/* XXXXX
	 * current IFNET_RLOCK() is mapped to IFNET_WLOCK()
	 * so it is okay to use this ASSERT, change it when
	 * IFNET lock is finalized
	 */
	IFNET_WLOCK_ASSERT();

	error = 0;
	for (i = 0; i < LLTBL_HASHTBL_SIZE; i++) {
		LIST_FOREACH(lle, &llt->lle_head[i], lle_next) {
			struct sockaddr_dl *sdl;
			
			/* skip deleted entries */
			if ((lle->la_flags & (LLE_DELETED|LLE_VALID)) != LLE_VALID)
				continue;
			/* Skip if jailed and not a valid IP of the prison. */
			if (jailed(wr->td->td_ucred) &&
			    !prison_if(wr->td->td_ucred, L3_ADDR(lle)))
				continue;
			/*
			 * produce a msg made of:
			 *  struct rt_msghdr;
			 *  struct sockaddr_inarp; (IPv4)
			 *  struct sockaddr_dl;
			 */
			bzero(&arpc, sizeof(arpc));
			arpc.rtm.rtm_msglen = sizeof(arpc);
			arpc.rtm.rtm_version = RTM_VERSION;
			arpc.rtm.rtm_type = RTM_GET;
			arpc.rtm.rtm_flags = RTF_UP;
			arpc.rtm.rtm_addrs = RTA_DST | RTA_GATEWAY;
			arpc.sin.sin_family = AF_INET;
			arpc.sin.sin_len = sizeof(arpc.sin);
			arpc.sin.sin_addr.s_addr = SIN(lle)->sin_addr.s_addr;

			/* publish */
			if (lle->la_flags & LLE_PUB) {
				arpc.rtm.rtm_flags |= RTF_ANNOUNCE;
				/* proxy only */
				if (lle->la_flags & LLE_PROXY)
					arpc.sin.sin_other = SIN_PROXY;
			}

			sdl = &arpc.sdl;
			sdl->sdl_family = AF_LINK;
			sdl->sdl_len = sizeof(*sdl);
			sdl->sdl_alen = ifp->if_addrlen;
			sdl->sdl_index = ifp->if_index;
			sdl->sdl_type = ifp->if_type;
			bcopy(&lle->ll_addr, LLADDR(sdl), ifp->if_addrlen);

			arpc.rtm.rtm_rmx.rmx_expire =
			    lle->la_flags & LLE_STATIC ? 0 : lle->la_expire;
			arpc.rtm.rtm_flags |= (RTF_HOST | RTF_LLDATA);
			if (lle->la_flags & LLE_STATIC)
				arpc.rtm.rtm_flags |= RTF_STATIC;
			arpc.rtm.rtm_index = ifp->if_index;
			error = SYSCTL_OUT(wr, &arpc, sizeof(arpc));
			if (error)
				break;
		}
	}
	return error;
#undef SIN
}

void *
in_domifattach(struct ifnet *ifp)
{   
	struct lltable *llt = lltable_init(ifp, AF_INET);
 
	if (llt != NULL) {
		llt->llt_new = in_lltable_new;
		llt->llt_free = in_lltable_free;
		llt->llt_rtcheck = in_lltable_rtcheck;
		llt->llt_lookup = in_lltable_lookup;
		llt->llt_dump = in_lltable_dump;
	}
	return (llt);
}

void
in_domifdetach(struct ifnet *ifp __unused, void *aux)
{
	struct lltable *llt = (struct lltable *)aux;

	lltable_free(llt);
}
