# $FreeBSD: src/sys/modules/cmx/Makefile,v 1.1 2008/03/06 08:09:45 rink Exp $
#
MAINTAINER=	daniel@roe.ch

.PATH:	${.CURDIR}/../../dev/cmx

KMOD=	cmx
SRCS=	cmx.c cmxvar.h cmxreg.h \
	cmx_pccard.c \
	pccarddevs.h device_if.h bus_if.h card_if.h

.include <bsd.kmod.mk>
