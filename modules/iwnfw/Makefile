# $FreeBSD: src/sys/modules/iwnfw/Makefile,v 1.2 2009/01/09 22:35:25 sam Exp $

.PATH: ${.CURDIR}/../../contrib/dev/iwn

KMOD=	iwnfw
FIRMWS=	iwlwifi-4965-4.44.17.fw:iwnfw:44417

CLEANFILES=	iwlwifi-4965-4.44.17.fw

iwlwifi-4965-4.44.17.fw: ${.CURDIR}/../../contrib/dev/iwn/iwlwifi-4965-4.44.17.fw.uu
	uudecode -p ${.CURDIR}/../../contrib/dev/iwn/iwlwifi-4965-4.44.17.fw.uu > ${.TARGET}

.include <bsd.kmod.mk>
