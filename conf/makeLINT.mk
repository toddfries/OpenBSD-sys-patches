# $FreeBSD: src/sys/conf/makeLINT.mk,v 1.2 2010/01/08 19:00:20 bz Exp $

all:
	@echo "make LINT only"

clean:
	rm -f LINT
.if ${TARGET} == "amd64" || ${TARGET} == "i386"
	rm -f LINT-VIMAGE
.endif

NOTES=	../../conf/NOTES NOTES
LINT: ${NOTES} ../../conf/makeLINT.sed
	cat ${NOTES} | sed -E -n -f ../../conf/makeLINT.sed > ${.TARGET}
.if ${TARGET} == "amd64" || ${TARGET} == "i386"
	echo "include ${.TARGET}"	>  ${.TARGET}-VIMAGE
	echo "ident ${.TARGET}-VIMAGE"	>> ${.TARGET}-VIMAGE
	echo "options VIMAGE"		>> ${.TARGET}-VIMAGE
.endif
