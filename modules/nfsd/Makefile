# $FreeBSD: src/sys/modules/nfsd/Makefile,v 1.1 2009/05/28 19:45:11 rmacklem Exp $

.PATH: ${.CURDIR}/../../fs/nfsserver
KMOD=	nfsd
SRCS=	vnode_if.h \
	nfs_nfsdserv.c \
	nfs_nfsdcache.c \
	nfs_nfsdkrpc.c \
	nfs_nfsdsocket.c \
	nfs_nfsdstate.c \
	nfs_nfsdsubs.c \
	nfs_nfsdport.c \
	opt_ufs.h \
	opt_nfs.h \
	opt_inet6.h \
	opt_kgssapi.h

.include <bsd.kmod.mk>
