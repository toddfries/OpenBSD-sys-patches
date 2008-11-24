# $FreeBSD: src/sys/modules/mac_bsdextended/Makefile,v 1.2 2008/10/27 12:09:15 rwatson Exp $

.PATH: ${.CURDIR}/../../security/mac_bsdextended

KMOD=	mac_bsdextended
SRCS=	vnode_if.h \
	mac_bsdextended.h \
	mac_bsdextended.c \
	ugidfw_system.c \
	ugidfw_vnode.c

.include <bsd.kmod.mk>
