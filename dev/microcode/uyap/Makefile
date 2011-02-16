<<<<<<< HEAD
# $OpenBSD: Makefile,v 1.4 2006/11/07 23:52:45 deraadt Exp $
=======
# $OpenBSD: Makefile,v 1.7 2010/01/30 20:48:21 miod Exp $
>>>>>>> origin/master

NOPROG=
NOMAN=
CC=${HOSTCC}

# USB capable systems only
<<<<<<< HEAD
.if (${MACHINE} == "i386") || (${MACHINE} == "amd64") || \
    (${MACHINE} == "alpha") || (${MACHINE} == "sparc64") || \
    (${MACHINE_ARCH} == "powerpc") || (${MACHINE} == "cats") || \
    (${MACHINE} == "hppa") || (${MACHINE} == "hppa64") || \
    (${MACHINE} == "sgi") || (${MACHINE} == "landisk")
=======
.if (${MACHINE_ARCH} == "powerpc" || \
    ${MACHINE} == "alpha" || ${MACHINE} == "amd64" || \
    ${MACHINE} == "armish" || ${MACHINE} == "hppa" || \
    ${MACHINE} == "hppa64" || ${MACHINE} == "i386" || \
    ${MACHINE} == "landisk" || ${MACHINE} == "loongson" || \
    ${MACHINE} == "palm" || ${MACHINE} == "sgi" || \
    ${MACHINE} == "sparc64" || ${MACHINE} == "zaurus")
>>>>>>> origin/master

FIRM= 	uyap

PROG=	build

CLEANFILES+= ${FIRM} ${PROG}

all: ${FIRM}

${FIRM}: build
	${.OBJDIR}/build

realinstall:

afterinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 644 \
	    ${FIRM} ${DESTDIR}/etc/firmware
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 644 \
	    ${.CURDIR}/uyap-license ${DESTDIR}/etc/firmware
.endif

.include <bsd.prog.mk>
