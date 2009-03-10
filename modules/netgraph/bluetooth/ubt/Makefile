# $Id: Makefile,v 1.2 2003/03/22 23:44:34 max Exp $
# $FreeBSD: src/sys/modules/netgraph/bluetooth/ubt/Makefile,v 1.9 2009/02/23 18:32:59 thompsa Exp $

S=	${.CURDIR}/../../../..

.PATH: $S/dev/usb/bluetooth

KMOD=	ng_ubt
SRCS=	opt_bus.h opt_usb.h device_if.h bus_if.h usb_if.h usbdevs.h \
	ng_ubt.c

.include <bsd.kmod.mk>
