# $FreeBSD: src/sys/modules/syscons/beastie/Makefile,v 1.3 2010/04/07 18:42:21 jkim Exp $

.PATH:	${.CURDIR}/../../../dev/syscons/logo

KMOD=	beastie_saver
SRCS=	beastie.c logo_saver.c

CFLAGS+=-DBEASTIE_LOGO

.include <bsd.kmod.mk>
