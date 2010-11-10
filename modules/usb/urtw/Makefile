# $FreeBSD: src/sys/modules/usb/urtw/Makefile,v 1.1 2009/05/27 03:57:38 weongyo Exp $

.PATH: ${.CURDIR}/../../../dev/usb/wlan

KMOD    = if_urtw
SRCS    = if_urtw.c if_urtwreg.h if_urtwvar.h	\
	  bus_if.h device_if.h \
	  opt_bus.h opt_usb.h usb_if.h usbdevs.h

.include <bsd.kmod.mk>
