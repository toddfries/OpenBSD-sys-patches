<<<<<<< HEAD
#	$OpenBSD: Makefile,v 1.8 2002/07/15 21:29:16 mickey Exp $
#	$NetBSD: Makefile,v 1.4 1996/03/25 22:48:09 mrg Exp $

#
# Makefile for sparc links, tags file

S=${.CURDIR}/../..
.include "$S/kern/Make.tags.inc"

SUBDIR= stand

NOPROG=	noprog
NOMAN=	noman

TSPARC=	$S/arch/sparc/tags
DIRS=	conf dev fpu include rcons sbus sparc

links::
	-for i in ${DIRS}; do \
	    (cd $$i && { rm -f tags; ln -s ${TSPARC} tags; }) done

SPARC=	$S/arch/sparc/dev/*.[ch] $S/arch/sparc/fpu/*.[ch] \
	$S/arch/sparc/include/*.[ch] $S/arch/sparc/sparc/*.[ch]
ASPARC=	$S/arch/sparc/sparc/*.[sS] $S/lib/libkern/arch/sparc/*.[sS]

tags:
	-ctags -wdtf ${TSPARC} ${COMM} ${SPARC}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ASPARC} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
		>> ${TSPARC}
	sort -o ${TSPARC} ${TSPARC}

obj:   _SUBDIRUSE
=======
#	$OpenBSD: Makefile,v 1.12 2010/11/12 17:11:44 deraadt Exp $

S=	${.CURDIR}/../..
KFILE=	GENERIC
.if exists(conf/GENERIC.MP)
KFILE=	GENERIC.MP
.endif
TDIRS=	${_arch} include pci isa eisa
TAGS=	${.CURDIR}/tags

NOPROG=
NOMAN=
SUBDIR=	stand

# config the fattest kernel we can find into a temporary dir
# to create a Makefile.  Then use make to pull some variables
# out and push them into the sub-shell to expand the paths,
# and finally run ctags.
tags::
	TDIR=`mktemp -d /tmp/_tagXXXXXXXXXX` || exit 1; \
	eval "S=${S}" && \
	config -s ${S} -b $${TDIR} ${.CURDIR}/conf/${KFILE} && \
	eval "_arch=\"`make -V _arch -f $${TDIR}/Makefile`\"" && \
	eval "_mach=\"`make -V _mach -f $${TDIR}/Makefile`\"" && \
	eval "_machdir=\$S/arch/$${_mach}" && \
	eval "_archdir=\$S/arch/$${_arch}" && \
	eval "HFILES=\"`find $S \( -path $S/'arch' -o -path $S/stand -o -path $S/lib/libsa -o -path $S'/lib/libkern/arch' \) -prune -o -name '*.h'; find $${_machdir} $${_archdir} $S/lib/libkern/arch/$${_mach} \( -name boot -o -name stand \) -prune -o -name '*.h'`\"" && \
	eval "SFILES=\"`make -V SFILES -f $${TDIR}/Makefile`\"" && \
	eval "CFILES=\"`make -V CFILES -f $${TDIR}/Makefile`\"" && \
	eval "AFILES=\"`make -V AFILES -f $${TDIR}/Makefile`\"" && \
	ctags -wd -f ${TAGS} $${CFILES} $${HFILES} && \
	egrep "^[_A-Z]*ENTRY[_A-Z]*\(.*\)" $${SFILES} $${AFILES} | \
	    sed "s;\\([^:]*\\):\\([^(]*\\)(\\([^, )]*\\)\\(.*\\);\\3	\\1	/^\\2(\\3\\4$$/;" \
	    >> ${TAGS} && \
	sort -o ${TAGS} ${TAGS} && \
	rm -rf $${TDIR}

links:
	-for i in conf ${TDIRS}; do \
	    (cd $$i && rm -f tags; ln -s tags tags); done

obj:	_SUBDIRUSE
>>>>>>> origin/master

.include <bsd.prog.mk>
