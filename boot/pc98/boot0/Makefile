# $FreeBSD: src/sys/boot/pc98/boot0/Makefile,v 1.5 2010/06/03 17:42:32 ed Exp $

PROG=	${BOOT}.out
INTERNALPROG=
FILES=	${BOOT}
NO_MAN=
SRCS=	${BOOT}.s
CLEANFILES= ${BOOT}

BOOT=	boot0

# The base address that we the boot0 code to to run it.  Don't change this
# unless you are glutton for punishment.
BOOT_BOOT0_ORG?=	0x0000

LDFLAGS=-e start -Ttext ${BOOT_BOOT0_ORG} -Wl,-N

${BOOT}: ${BOOT}.out
	objcopy -S -O binary ${BOOT}.out ${.TARGET}

.include <bsd.prog.mk>
