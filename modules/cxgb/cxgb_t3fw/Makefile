# $FreeBSD: src/sys/modules/cxgb/cxgb_t3fw/Makefile,v 1.3 2008/09/23 03:16:54 kmacy Exp $

CXGB = ${.CURDIR}/../../../dev/cxgb
.PATH: ${CXGB} 

KMOD= cxgb_t3fw
SRCS+= cxgb_t3fw.c
CFLAGS+= -g -I${CXGB}

.include <bsd.kmod.mk>
