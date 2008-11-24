# $FreeBSD: src/sys/modules/ata/atapci/Makefile,v 1.2 2008/10/09 12:56:57 sos Exp $

SUBDIR += chipsets

.PATH: ${.CURDIR}/../../../dev/ata

KMOD=	atapci
SRCS=	ata-pci.c ata-dma.c ata-sata.c
SRCS+=  opt_ata.h ata_if.h device_if.h bus_if.h pci_if.h

.include <bsd.kmod.mk>

