# $FreeBSD: src/sys/modules/ste/Makefile,v 1.13 2008/08/14 20:09:58 imp Exp $

.PATH: ${.CURDIR}/../../dev/ste

KMOD=	if_ste
SRCS=	if_ste.c device_if.h bus_if.h pci_if.h
SRCS+=	miibus_if.h

.include <bsd.kmod.mk>
