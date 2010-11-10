# $FreeBSD: src/sys/modules/runfw/Makefile,v 1.2 2010/01/29 03:35:01 thompsa Exp $

KMOD=	runfw
FIRMWS=	runfw:runfw:1

CLEANFILES=	runfw

runfw: ${.CURDIR}/../../contrib/dev/run/rt2870.fw.uu
	uudecode -p ${.CURDIR}/../../contrib/dev/run/rt2870.fw.uu > ${.TARGET}

.include <bsd.kmod.mk>
