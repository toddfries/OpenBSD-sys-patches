# $FreeBSD: src/sys/boot/efi/Makefile.inc,v 1.8 2008/06/25 21:33:28 ru Exp $

BINDIR?=	/boot

# Options used when building app-specific efi components
CFLAGS+=	-ffreestanding -fshort-wchar -Wformat
LDFLAGS+=	-nostdlib

.include "../Makefile.inc"
