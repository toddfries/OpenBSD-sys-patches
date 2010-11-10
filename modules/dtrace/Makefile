# $FreeBSD: src/sys/modules/dtrace/Makefile,v 1.6 2009/05/26 20:28:22 sson Exp $

.include <bsd.own.mk>
.include "Makefile.inc"

SUBDIR=		dtmalloc	\
		dtnfsclient	\
		dtrace		\
		dtraceall	\
		dtrace_test	\
		lockstat	\
		profile		\
		prototype	\
		sdt		\
		systrace

.if ${MACHINE_ARCH} == "amd64"
SUBDIR+=	fbt
.elif ${MACHINE_ARCH} == "i386"
SUBDIR+=	fbt
.endif

.include <bsd.subdir.mk>
