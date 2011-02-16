<<<<<<< HEAD
# $OpenBSD: Makefile,v 1.1 2006/09/20 22:16:04 deraadt Exp $
=======
# $OpenBSD: Makefile,v 1.4 2009/07/03 04:34:51 dlg Exp $
>>>>>>> origin/master

NOPROG=
NOMAN=
CC=${HOSTCC}

# PCI capable systems only
.if (${MACHINE} == "i386") || (${MACHINE} == "amd64") || \
    (${MACHINE} == "alpha") || (${MACHINE} == "sparc64") || \
    (${MACHINE_ARCH} == "powerpc") || (${MACHINE} == "cats") || \
    (${MACHINE} == "hppa") || (${MACHINE} == "hppa64") || \
    (${MACHINE} == "sgi")

FIRM=	bnx-b06 bnx-b09 bnx-rv2p bnx-xi-rv2p bnx-xi90-rv2p

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
	    ${.CURDIR}/bnx-license ${DESTDIR}/etc/firmware

.endif

.include <bsd.prog.mk>
