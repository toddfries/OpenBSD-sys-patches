# $FreeBSD: src/sys/modules/ata/Makefile,v 1.3 2008/10/09 12:56:57 sos Exp $

SUBDIR = atacore
SUBDIR += atacard
.if ${MACHINE} == "pc98"
SUBDIR += atacbus
.else
SUBDIR += ataisa
.endif
SUBDIR += atapci 
SUBDIR += atadisk atapicd atapifd atapist ataraid atapicam #atacam

.include <bsd.subdir.mk>
