# $DragonFly: src/sys/dev/netif/et/Makefile,v 1.1 2007/10/12 14:12:42 sephe Exp $
# $FreeBSD: src/sys/modules/et/Makefile,v 1.2 2008/07/28 17:56:37 antoine Exp $

.PATH: ${.CURDIR}/../../dev/et

KMOD=	if_et
SRCS=	if_et.c
SRCS+=	bus_if.h pci_if.h device_if.h miibus_if.h

.include <bsd.kmod.mk>
