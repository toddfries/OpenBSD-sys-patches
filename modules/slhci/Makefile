# $FreeBSD: src/sys/modules/slhci/Makefile,v 1.3 2008/10/09 19:22:00 n_hibma Exp $

.PATH:	${.CURDIR}/../../dev/usb

KMOD=	slhci
SRCS=	sl811hs.c slhci_pccard.c

SRCS+=	bus_if.h device_if.h card_if.h power_if.h \
	usbdevs.h pccarddevs.h \
	opt_slhci.h opt_usb.h \

.include <bsd.kmod.mk>
