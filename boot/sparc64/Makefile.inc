# $FreeBSD: src/sys/boot/sparc64/Makefile.inc,v 1.2 2008/06/25 21:33:28 ru Exp $

BINDIR?=	/boot
CFLAGS+=	-ffreestanding
LDFLAGS+=	-nostdlib

.include "../Makefile.inc"
