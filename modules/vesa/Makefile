# $FreeBSD: src/sys/modules/vesa/Makefile,v 1.16 2009/10/19 20:58:10 jkim Exp $

KMOD=	vesa
SRCS=	opt_vga.h opt_vesa.h
SRCS+=	bus_if.h device_if.h pci_if.h

.PATH:	${.CURDIR}/../../dev/fb
SRCS+=	vesa.c

.PATH:	${.CURDIR}/../../dev/syscons
SRCS+=	scvesactl.c

.include <bsd.kmod.mk>
