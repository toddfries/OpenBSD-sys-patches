# $FreeBSD: src/sys/modules/rl/Makefile,v 1.13 2008/11/02 16:50:57 imp Exp $

.PATH: ${.CURDIR}/../../pci

KMOD=	if_rl
SRCS=	if_rl.c device_if.h bus_if.h pci_if.h
SRCS+=	miibus_if.h

.include <bsd.kmod.mk>
