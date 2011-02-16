<<<<<<< HEAD
/*	$OpenBSD: bluetooth.h,v 1.2 2005/01/17 18:12:49 mickey Exp $	*/
=======
/*	$OpenBSD: bluetooth.h,v 1.6 2008/11/22 04:42:58 uwe Exp $	*/
/*	$NetBSD: bluetooth.h,v 1.8 2008/09/08 23:36:55 gmcgarry Exp $	*/
>>>>>>> origin/master

/*
 * bluetooth.h
 *
 * Copyright (c) 2001-2002 Maksim Yevmenkin <m_evmenkin@yahoo.com>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/netgraph/bluetooth/include/ng_bluetooth.h,v 1.3 2003/11/14 03:45:29 emax Exp $
 */

#ifndef _NETGRAPH_BLUETOOTH_H_
#define _NETGRAPH_BLUETOOTH_H_

/*
 * XXX: This file only contains redundant mbuf wrapppers and must be
 * removed later.
 */

/*
 * XXX: dirty temporary hacks.
 */
#undef KASSERT
#define KASSERT(a, b)
#undef mtx_init
#undef mtx_lock
#undef mtx_unlock
#undef mtx_assert
#undef mtx_destroy
#define mtx_init(a, b, c, d)
#define mtx_lock(a)
#define mtx_unlock(a)
#define mtx_assert(a, b)
#define mtx_destroy(a)
#define ACCEPT_LOCK()
#define SOCK_LOCK(a)

#define NG_FREE_M(m)							\
	do {								\
		if ((m)) {						\
			m_freem((m));					\
			(m) = NULL;					\
		}							\
	} while (0)

/*
 * Version of the stack
 */
<<<<<<< HEAD

#define NG_BLUETOOTH_VERSION	1
=======
typedef struct {
	uint8_t	b[BLUETOOTH_BDADDR_SIZE];
} __packed bdaddr_t;
>>>>>>> origin/master

/*
 * Declare the base of the Bluetooth sysctl hierarchy, 
 * but only if this file cares about sysctl's
 */

#ifdef SYSCTL_DECL
SYSCTL_DECL(_net_bluetooth);
SYSCTL_DECL(_net_bluetooth_hci);
SYSCTL_DECL(_net_bluetooth_l2cap);
SYSCTL_DECL(_net_bluetooth_rfcomm);
#endif /* SYSCTL_DECL */

/*
 * Mbuf queue and useful mbufq macros. We do not use ifqueue because we
 * do not need mutex and other locking stuff
 */

struct mbuf;

struct ng_bt_mbufq {
	struct mbuf	*head;   /* first item in the queue */
	struct mbuf	*tail;   /* last item in the queue */
	u_int32_t	 len;    /* number of items in the queue */
	u_int32_t	 maxlen; /* maximal number of items in the queue */
	u_int32_t	 drops;	 /* number if dropped items */
};
typedef struct ng_bt_mbufq	ng_bt_mbufq_t;
typedef struct ng_bt_mbufq *	ng_bt_mbufq_p;

#define NG_BT_MBUFQ_INIT(q, _maxlen)			\
	do {						\
		(q)->head = NULL;			\
		(q)->tail = NULL;			\
		(q)->len = 0;				\
		(q)->maxlen = (_maxlen);		\
		(q)->drops = 0;				\
	} while (0)

#define NG_BT_MBUFQ_DESTROY(q)				\
	do {						\
		NG_BT_MBUFQ_DRAIN((q));			\
	} while (0)

#define NG_BT_MBUFQ_FIRST(q)	(q)->head

#define NG_BT_MBUFQ_LEN(q)	(q)->len

#define NG_BT_MBUFQ_FULL(q)	((q)->len >= (q)->maxlen)

#define NG_BT_MBUFQ_DROP(q)	(q)->drops ++

#define NG_BT_MBUFQ_ENQUEUE(q, i)			\
	do {						\
		(i)->m_nextpkt = NULL;			\
							\
		if ((q)->tail == NULL)			\
			(q)->head = (i);		\
		else					\
			(q)->tail->m_nextpkt = (i);	\
							\
		(q)->tail = (i);			\
		(q)->len ++;				\
	} while (0)

#define NG_BT_MBUFQ_DEQUEUE(q, i)			\
	do {						\
		(i) = (q)->head;			\
		if ((i) != NULL) {			\
			(q)->head = (q)->head->m_nextpkt; \
			if ((q)->head == NULL)		\
				(q)->tail = NULL;	\
							\
			(q)->len --;			\
			(i)->m_nextpkt = NULL;		\
		} 					\
	} while (0)

#define NG_BT_MBUFQ_PREPEND(q, i)			\
	do {						\
		(i)->m_nextpkt = (q)->head;		\
		if ((q)->tail == NULL)			\
			(q)->tail = (i);		\
							\
		(q)->head = (i);			\
		(q)->len ++;				\
	} while (0)

#define NG_BT_MBUFQ_DRAIN(q)				\
	do { 						\
        	struct mbuf	*m = NULL;		\
							\
		for (;;) { 				\
			NG_BT_MBUFQ_DEQUEUE((q), m);	\
			if (m == NULL) 			\
				break; 			\
							\
			NG_FREE_M(m);	 		\
		} 					\
	} while (0)

/* 
 * Netgraph item queue and useful itemq macros
 */

struct ng_item;

struct ng_bt_itemq {
	struct ng_item	*head;   /* first item in the queue */
	struct ng_item	*tail;   /* last item in the queue */
	u_int32_t	 len;    /* number of items in the queue */
	u_int32_t	 maxlen; /* maximal number of items in the queue */
	u_int32_t	 drops;  /* number if dropped items */
};
typedef struct ng_bt_itemq	ng_bt_itemq_t;
typedef struct ng_bt_itemq *	ng_bt_itemq_p;

#define NG_BT_ITEMQ_INIT(q, _maxlen)	NG_BT_MBUFQ_INIT((q), (_maxlen))

#define NG_BT_ITEMQ_DESTROY(q)				\
	do {						\
		NG_BT_ITEMQ_DRAIN((q));			\
	} while (0)

#define NG_BT_ITEMQ_FIRST(q)	NG_BT_MBUFQ_FIRST((q))

#define NG_BT_ITEMQ_LEN(q)	NG_BT_MBUFQ_LEN((q))

#define NG_BT_ITEMQ_FULL(q)	NG_BT_MBUFQ_FULL((q))

#define NG_BT_ITEMQ_DROP(q)	NG_BT_MBUFQ_DROP((q))

#define NG_BT_ITEMQ_ENQUEUE(q, i)			\
	do {						\
		(i)->el_next = NULL;			\
							\
		if ((q)->tail == NULL)			\
			(q)->head = (i);		\
		else					\
			(q)->tail->el_next = (i);	\
							\
		(q)->tail = (i);			\
		(q)->len ++;				\
	} while (0)

#define NG_BT_ITEMQ_DEQUEUE(q, i)			\
	do {						\
		(i) = (q)->head;			\
		if ((i) != NULL) {			\
			(q)->head = (q)->head->el_next;	\
			if ((q)->head == NULL)		\
				(q)->tail = NULL;	\
							\
			(q)->len --;			\
			(i)->el_next = NULL;		\
		} 					\
	} while (0)

#define NG_BT_ITEMQ_PREPEND(q, i)			\
	do {						\
		(i)->el_next = (q)->head;		\
		if ((q)->tail == NULL)			\
			(q)->tail = (i);		\
							\
		(q)->head = (i);			\
		(q)->len ++;				\
	} while (0)

#define NG_BT_ITEMQ_DRAIN(q)				\
	do { 						\
        	struct ng_item	*i = NULL;		\
							\
		for (;;) { 				\
			NG_BT_ITEMQ_DEQUEUE((q), i);	\
			if (i == NULL) 			\
				break; 			\
							\
			NG_FREE_ITEM(i); 		\
		} 					\
	} while (0)

/*
 * Get Bluetooth stack sysctl globals
 */

<<<<<<< HEAD
u_int32_t	bluetooth_hci_command_timeout	(void);
u_int32_t	bluetooth_hci_connect_timeout	(void);
u_int32_t	bluetooth_hci_max_neighbor_age	(void);
u_int32_t	bluetooth_l2cap_rtx_timeout	(void);
u_int32_t	bluetooth_l2cap_ertx_timeout	(void);

#endif /* _NETGRAPH_BLUETOOTH_H_ */
=======
#define BLUETOOTH_DEBUG
#ifdef BLUETOOTH_DEBUG
extern int bluetooth_debug;
# define DPRINTF(fmt, args...)	do {			\
	if (bluetooth_debug)				\
		printf("%s: "fmt, __func__ , ##args);	\
} while (/* CONSTCOND */0)

# define DPRINTFN(n, fmt, args...)	do {		\
	if (bluetooth_debug > (n))			\
		printf("%s: "fmt, __func__ , ##args);	\
} while (/* CONSTCOND */0)

# define UNKNOWN(value)			\
		printf("%s: %s = %d unknown!\n", __func__, #value, (value));
#else
# define DPRINTF(...) ((void)0)
# define DPRINTFN(...) ((void)0)
# define UNKNOWN(x) ((void)0)
#endif	/* BLUETOOTH_DEBUG */

extern struct mutex bt_lock;

/* XXX NetBSD compatibility goo, abused for debugging */
#ifdef BLUETOOTH_DEBUG
#define mutex_enter(mtx) do {						\
	DPRINTFN(1, "mtx_enter(" __STRING(mtx) ") in %d\n",		\
	    curproc ? curproc->p_pid : 0);				\
	mtx_enter((mtx));						\
} while (/*CONSTCOND*/0)
#define mutex_exit(mtx) do {						\
	DPRINTFN(1, "mtx_leave(" __STRING(mtx) ") in %d\n",		\
	    curproc ? curproc->p_pid : 0);				\
	mtx_leave((mtx));						\
} while (/*CONSTCOND*/0)
#else
#define mutex_enter		mtx_enter
#define mutex_exit		mtx_leave
#endif

#endif	/* _KERNEL */

#endif	/* _NETBT_BLUETOOTH_H_ */
>>>>>>> origin/master
