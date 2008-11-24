# $FreeBSD: src/sys/modules/ohci/Makefile,v 1.1 2008/10/09 19:22:00 n_hibma Exp $

S=	${.CURDIR}/../..

.PATH: $S/dev/usb $S/pci

KMOD=	ohci
SRCS=	bus_if.h device_if.h \
	opt_usb.h \
	ohci_pci.c ohci.c ohcireg.h ohcivar.h \
	pci_if.h

.include <bsd.kmod.mk>
