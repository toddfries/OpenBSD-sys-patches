# $FreeBSD: src/sys/modules/wb/Makefile,v 1.14 2008/08/14 21:26:29 imp Exp $

.PATH: ${.CURDIR}/../../dev/wb

KMOD=	if_wb
SRCS=	if_wb.c device_if.h
SRCS+=	bus_if.h miibus_if.h pci_if.h

.include <bsd.kmod.mk>
