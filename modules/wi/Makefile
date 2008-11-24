# $FreeBSD: src/sys/modules/wi/Makefile,v 1.19 2008/08/27 05:43:46 imp Exp $

.PATH: ${.CURDIR}/../../dev/wi

KMOD=	if_wi
SRCS=	if_wi.c if_wi_pccard.c if_wi_pci.c \
	card_if.h device_if.h bus_if.h pci_if.h pccarddevs.h

.include <bsd.kmod.mk>
