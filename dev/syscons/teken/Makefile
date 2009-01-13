# $FreeBSD: src/sys/dev/syscons/teken/Makefile,v 1.1 2009/01/01 13:26:53 ed Exp $

PROG=		teken_demo
SRCS=		teken_demo.c teken.c teken_state.h
CLEANFILES=	teken_state.h teken.log
LDADD=		-lncurses -lutil
NO_MAN=
WARNS?=		6

teken_state.h: gensequences sequences
	awk -f gensequences sequences > ${.TARGET}

.include <bsd.prog.mk>
