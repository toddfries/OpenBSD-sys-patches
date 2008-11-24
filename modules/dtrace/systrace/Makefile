# $FreeBSD: src/sys/modules/dtrace/systrace/Makefile,v 1.1 2008/05/17 02:31:19 jb Exp $

.PATH: ${.CURDIR}/../../../cddl/dev/systrace

KMOD=		systrace
SRCS=		systrace.c
SRCS+=		vnode_if.h

CFLAGS+=	-I${.CURDIR}/../../../cddl/compat/opensolaris \
		-I${.CURDIR}/../../../cddl/contrib/opensolaris/uts/common \
		-I${.CURDIR}/../../..

.include <bsd.kmod.mk>
