# $FreeBSD: src/sys/modules/dtrace/dtrace_test/Makefile,v 1.1 2008/05/17 02:31:17 jb Exp $

.PATH: ${.CURDIR}/../../../cddl/dev/dtrace

KMOD=		dtrace_test
SRCS=		dtrace_test.c

SRCS+=		vnode_if.h
		
CFLAGS+=	-I${.CURDIR}/../../..

CFLAGS+=	-D_KERNEL

.include <bsd.kmod.mk>
