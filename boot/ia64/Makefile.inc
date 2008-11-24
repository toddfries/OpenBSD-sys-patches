# $FreeBSD: src/sys/boot/ia64/Makefile.inc,v 1.4 2008/06/25 21:33:28 ru Exp $

BINDIR?=	/boot

# Options used when building standalone components
CFLAGS+=	-ffreestanding -fshort-wchar -Wformat
LDFLAGS+=	-nostdlib

.include "../Makefile.inc"
