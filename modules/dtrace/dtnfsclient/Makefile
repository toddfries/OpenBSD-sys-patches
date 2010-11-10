# $FreeBSD: src/sys/modules/dtrace/dtnfsclient/Makefile,v 1.2 2009/03/25 17:47:22 rwatson Exp $

.PATH: ${.CURDIR}/../../../nfsclient

KMOD=		dtnfsclient
SRCS=		nfs_kdtrace.c
SRCS+=		vnode_if.h

CFLAGS+=	-I${.CURDIR}/../../../cddl/compat/opensolaris \
		-I${.CURDIR}/../../../cddl/contrib/opensolaris/uts/common \
		-I${.CURDIR}/../../..

.include <bsd.kmod.mk>
