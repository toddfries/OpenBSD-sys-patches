# $FreeBSD: src/sys/modules/sound/driver/ai2s/Makefile,v 1.1 2009/01/25 18:20:15 nwhitehorn Exp $

.PATH: ${.CURDIR}/../../../../dev/sound/macio

KMOD=	snd_ai2s
SRCS=	device_if.h bus_if.h ofw_bus_if.h
SRCS+=	channel_if.h feeder_if.h mixer_if.h
SRCS+=	snapper.c tumbler.c aoa.c i2s.c

.include <bsd.kmod.mk>
