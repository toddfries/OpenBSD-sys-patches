# $Id: Makefile,v 1.1 2003/04/27 00:22:12 max Exp $
# $FreeBSD: src/sys/modules/netgraph/bluetooth/ubtbcmfw/Makefile,v 1.8 2009/05/27 16:43:40 thompsa Exp $

.PATH: ${.CURDIR}/../../../../netgraph/bluetooth/drivers/ubtbcmfw

CFLAGS+=	-I${.CURDIR}/../../../../netgraph/bluetooth/include \
		-I${.CURDIR}/../../../../netgraph/bluetooth/drivers/ubtbcmfw

KMOD=		ubtbcmfw
SRCS=		ubtbcmfw.c opt_bus.h opt_usb.h device_if.h bus_if.h \
		usb_if.h vnode_if.h usbdevs.h

.include <bsd.kmod.mk>
