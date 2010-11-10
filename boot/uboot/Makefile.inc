# $FreeBSD: src/sys/boot/uboot/Makefile.inc,v 1.2 2010/07/12 00:49:22 nwhitehorn Exp $

.if ${MACHINE_ARCH} == "powerpc64"
CFLAGS+=	-m32 -mcpu=powerpc
LDFLAGS+=	-m elf32ppc
.endif

.include "../Makefile.inc"
