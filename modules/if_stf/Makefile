# $FreeBSD: src/sys/modules/if_stf/Makefile,v 1.5 2009/06/06 17:01:44 rwatson Exp $

.PATH: ${.CURDIR}/../../net

KMOD=	if_stf
SRCS=	if_stf.c opt_inet.h opt_inet6.h

.if !defined(KERNBUILDDIR)
opt_inet.h:
	echo "#define INET 1" > ${.TARGET}

opt_inet6.h:
	echo "#define INET6 1" > ${.TARGET}
.endif

.include <bsd.kmod.mk>
