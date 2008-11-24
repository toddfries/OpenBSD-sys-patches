# $FreeBSD: src/sys/modules/xl/Makefile,v 1.15 2008/08/10 09:45:52 imp Exp $

.PATH: ${.CURDIR}/../../dev/xl

KMOD=	if_xl
SRCS=	if_xl.c device_if.h bus_if.h pci_if.h
SRCS+=	miibus_if.h

.include <bsd.kmod.mk>
