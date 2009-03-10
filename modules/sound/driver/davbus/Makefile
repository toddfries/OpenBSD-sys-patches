# $FreeBSD: src/sys/modules/sound/driver/davbus/Makefile,v 1.1 2009/01/25 18:20:15 nwhitehorn Exp $

.PATH: ${.CURDIR}/../../../../dev/sound/macio

KMOD=	snd_davbus
SRCS=	device_if.h bus_if.h ofw_bus_if.h
SRCS+=	channel_if.h feeder_if.h mixer_if.h
SRCS+=	aoa.c davbus.c

.include <bsd.kmod.mk>
