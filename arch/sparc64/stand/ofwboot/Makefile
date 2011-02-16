<<<<<<< HEAD
#	$OpenBSD: Makefile,v 1.7 2003/06/10 02:42:58 brad Exp $
=======
#	$OpenBSD: Makefile,v 1.16 2010/08/25 12:53:38 jsing Exp $
>>>>>>> origin/master
#	$NetBSD: Makefile,v 1.2 2001/03/04 14:50:05 mrg Exp $

CURDIR=	${.CURDIR}
S=	${CURDIR}/../../../..

#
# Override normal settings
#

WARNS=		0

PROG?=		ofwboot
NOMAN=		ja, man!
<<<<<<< HEAD
SRCS=		srt0.s Locore.c boot.c ofdev.c alloc.c net.c netif_of.c vers.c
.PATH:		${S}/arch/sparc64/sparc64
=======

.PATH:		${S}/arch/sparc64/sparc64
SRCS=		srt0.s Locore.c alloc.c boot.c elf64_exec.c net.c netif_of.c \
		ofdev.c vers.c

.PATH:		${S}/lib/libkern/arch/sparc64 ${S}/lib/libkern
SRCS+=		strlcpy.c strcmp.c strlcat.c strlen.c
>>>>>>> origin/master

CWARNFLAGS+=	-Wno-main
AFLAGS+=	-Wa,-Av9a
AFLAGS+=	-x assembler-with-cpp -D_LOCORE -D__ELF__
CFLAGS+=	${COPTS}
CPPFLAGS+=	-D_STANDALONE -DSUN4U -nostdinc
#CPPFLAGS+=	-DNETIF_DEBUG 

BINMODE=	444

NEWVERSWHAT=	"OpenFirmware Boot"

#
# ELF64 defaults to 1MB
#
RELOC=		100000

ENTRY=		_start

CLEANFILES+=	vers.c sparc machine

CPPFLAGS+=	-I${CURDIR}/../../.. -I${CURDIR}/../../../.. -I${CURDIR} -I.
CPPFLAGS+=	-DRELOC=0x${RELOC}

#
# XXXXX FIXME
#
CPPFLAGS+=	-DSPARC_BOOT_UFS
#CPPFLAGS+=	-DSPARC_BOOT_HSFS

.if !make(libdep) && !make(sadep) && !make(salibdir) && !make(kernlibdir) && !make(obj)
.BEGIN:
	@([ -h machine ] || ln -s ${.CURDIR}/../../include machine)
.endif

vers.c: version
	sh ${S}/conf/newvers.sh ${CURDIR}/version "sparc64" ${NEWVERSWHAT}

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -N -Ttext ${RELOC} -e ${ENTRY} -o ${PROG} \
	    ${OBJS} -L${LIBSADIR} ${LIBSA} -L${LIBKERNDIR} ${LIBKERN} \
	    -L${LIBZDIR} ${LIBZ}

NORMAL_S=	${CC} ${AFLAGS} ${CPPFLAGS} -c $<
srt0.o: srt0.s
	${NORMAL_S}

.include <bsd.prog.mk>
