# $FreeBSD: src/sys/modules/vr/Makefile,v 1.14 2008/03/11 03:50:57 yongari Exp $

.PATH: ${.CURDIR}/../../dev/vr

KMOD=	if_vr
SRCS=	if_vr.c device_if.h bus_if.h pci_if.h
SRCS+=	miibus_if.h

.include <bsd.kmod.mk>
