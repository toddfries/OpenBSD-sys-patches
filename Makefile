#	$OpenBSD: Makefile,v 1.33 2010/02/17 18:09:49 miod Exp $
#	$NetBSD: Makefile,v 1.5 1995/09/15 21:05:21 pk Exp $

SUBDIR=	dev/microcode \
	arch/alpha arch/amd64 arch/armish arch/aviion arch/hp300 \
	arch/hppa arch/hppa64 arch/i386 arch/landisk arch/loongson \
	arch/luna88k arch/m68k arch/mac68k arch/macppc arch/mvme68k \
	arch/mvme88k arch/mvmeppc arch/palm arch/sgi arch/socppc \
	arch/solbourne arch/sparc arch/sparc64 arch/vax arch/zaurus

.include <bsd.subdir.mk>
