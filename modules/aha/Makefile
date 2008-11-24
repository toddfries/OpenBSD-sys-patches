# $FreeBSD: src/sys/modules/aha/Makefile,v 1.10 2008/09/01 23:59:00 imp Exp $

.PATH: ${.CURDIR}/../../dev/aha

KMOD=	aha
SRCS=	aha.c aha_isa.c ahareg.h opt_cam.h device_if.h bus_if.h \
	opt_scsi.h isa_if.h

.if !defined(KERNBUILDDIR)
opt_scsi.h:
	echo "#define SCSI_DELAY 15000" > ${.TARGET}
.endif

.include <bsd.kmod.mk>
