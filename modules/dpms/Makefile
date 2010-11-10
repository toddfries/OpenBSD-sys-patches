# $FreeBSD: src/sys/modules/dpms/Makefile,v 1.3 2009/10/19 20:58:10 jkim Exp $

KMOD=	dpms
SRCS=	bus_if.h device_if.h pci_if.h

.PATH:	${.CURDIR}/../../dev/dpms
SRCS+=	dpms.c

.include <bsd.kmod.mk>
