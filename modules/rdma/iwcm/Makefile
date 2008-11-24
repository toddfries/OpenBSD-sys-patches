# $FreeBSD: src/sys/modules/rdma/iwcm/Makefile,v 1.1 2008/05/05 20:19:33 kmacy Exp $

RDMA=	${.CURDIR}/../../../contrib/rdma
.PATH: ${RDMA}

KMOD=  rdma_iwcm
SRCS=  device_if.h bus_if.h pci_if.h pcib_if.h opt_sched.h
SRCS+= rdma_iwcm.c 

.include <bsd.kmod.mk>
