<<<<<<< HEAD
# $OpenBSD: Makefile,v 1.4 2006/11/13 02:52:46 jsg Exp $
=======
# $OpenBSD: Makefile,v 1.9 2010/01/30 20:48:21 miod Exp $
>>>>>>> origin/master

NOPROG=
NOMAN=
CC=${HOSTCC}

# USB capable systems only
<<<<<<< HEAD
.if (${MACHINE_ARCH} == "powerpc") || (${MACHINE} == "alpha") || \
    (${MACHINE} == "amd64") || (${MACHINE} == "armish") || \
    (${MACHINE} == "cats") || (${MACHINE} == "hppa") || \
    (${MACHINE} == "hppa64") || (${MACHINE} == "i386") || \
    (${MACHINE} == "sgi") || (${MACHINE} == "sparc64") || \
    (${MACHINE} == "zaurus") || (${MACHINE} == "landisk")
=======
.if (${MACHINE_ARCH} == "powerpc" || \
    ${MACHINE} == "alpha" || ${MACHINE} == "amd64" || \
    ${MACHINE} == "armish" || ${MACHINE} == "hppa" || \
    ${MACHINE} == "hppa64" || ${MACHINE} == "i386" || \
    ${MACHINE} == "landisk" || ${MACHINE} == "loongson" || \
    ${MACHINE} == "palm" || ${MACHINE} == "sgi" || \
    ${MACHINE} == "sparc64" || ${MACHINE} == "zaurus")
>>>>>>> origin/master

FIRM=	zd1211 zd1211b

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
	    ${.CURDIR}/zd1211-licence ${DESTDIR}/etc/firmware
.endif

.include <bsd.prog.mk>
