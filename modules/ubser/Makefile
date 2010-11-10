# $FreeBSD: src/sys/modules/ubser/Makefile,v 1.4 2009/10/13 18:07:56 jhb Exp $

S=	${.CURDIR}/../..
.PATH: $S/dev/usb/serial

KMOD=	ubser
SRCS=	ubser.c opt_usb.h device_if.h bus_if.h usbdevs.h

.include <bsd.kmod.mk>
