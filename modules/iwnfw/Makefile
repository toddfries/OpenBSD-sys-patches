# $FreeBSD: src/sys/modules/iwnfw/Makefile,v 1.1 2008/04/29 21:36:17 sam Exp $

.PATH: ${.CURDIR}/../../contrib/dev/iwn

KMOD=	iwnfw
FIRMWS=	iwlwifi-4965-4.44.17.fw:iwnfw:44417
FIRMWARE_LICENSE= intel_iwn

CLEANFILES=	iwlwifi-4965-4.44.17.fw

iwlwifi-4965-4.44.17.fw: ${.CURDIR}/../../contrib/dev/iwn/iwlwifi-4965-4.44.17.fw.uu
	uudecode -p ${.CURDIR}/../../contrib/dev/iwn/iwlwifi-4965-4.44.17.fw.uu > ${.TARGET}

.include <bsd.kmod.mk>
