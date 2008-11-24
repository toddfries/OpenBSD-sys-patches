# $FreeBSD: src/sys/modules/tl/Makefile,v 1.17 2008/08/14 20:02:34 imp Exp $

.PATH: ${.CURDIR}/../../dev/tl

KMOD=	if_tl
SRCS=	if_tl.c device_if.h bus_if.h pci_if.h
SRCS+=	miibus_if.h

.include <bsd.kmod.mk>
