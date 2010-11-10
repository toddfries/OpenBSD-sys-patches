# $FreeBSD: src/sys/modules/cas/Makefile,v 1.2 2010/05/16 12:39:17 marius Exp $

.PATH: ${.CURDIR}/../../dev/cas

KMOD=	if_cas
SRCS=	bus_if.h device_if.h if_cas.c miibus_if.h pci_if.h ${ofw_bus_if}

.if ${MACHINE_ARCH} == "powerpc" || ${MACHINE_ARCH} == "sparc64"
ofw_bus_if=	ofw_bus_if.h
.endif

.include <bsd.kmod.mk>
