# $FreeBSD: src/sys/modules/sis/Makefile,v 1.12 2008/08/10 10:00:14 imp Exp $

.PATH: ${.CURDIR}/../../dev/sis

KMOD=	if_sis
SRCS=	if_sis.c device_if.h bus_if.h pci_if.h
SRCS+=	miibus_if.h

.include <bsd.kmod.mk>
