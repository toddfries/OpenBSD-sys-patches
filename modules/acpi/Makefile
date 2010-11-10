# $FreeBSD: src/sys/modules/acpi/Makefile,v 1.54 2009/06/23 13:17:25 rpaulo Exp $

.if ${MACHINE} == "i386"
SUBDIR=		acpi
.endif

SUBDIR+=	acpi_aiboost acpi_asus acpi_fujitsu acpi_hp acpi_ibm	\
		acpi_panasonic acpi_sony acpi_toshiba acpi_video	\
		acpi_dock acpi_wmi 

.include <bsd.subdir.mk>
