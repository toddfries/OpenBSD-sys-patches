# $FreeBSD: src/sys/modules/uhci/Makefile,v 1.1 2008/10/09 19:22:00 n_hibma Exp $

S=	${.CURDIR}/../..

.PATH: $S/dev/usb $S/pci

KMOD=	uhci
SRCS=	bus_if.h device_if.h \
	opt_usb.h \
	uhci_pci.c uhci.c uhcireg.h uhcivar.h \
	pci_if.h

.include <bsd.kmod.mk>
