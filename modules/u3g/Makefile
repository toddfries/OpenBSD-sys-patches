# $FreeBSD: src/sys/modules/u3g/Makefile,v 1.2 2008/10/24 07:19:09 n_hibma Exp $

.PATH: ${.CURDIR}/../../dev/usb

KMOD=	u3g
SRCS=	opt_usb.h opt_u3g.h \
	device_if.h bus_if.h \
	usbdevs.h \
	ucomvar.h \
	u3g.c 

.include <bsd.kmod.mk>
