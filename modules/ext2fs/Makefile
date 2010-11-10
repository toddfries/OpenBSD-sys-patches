# $FreeBSD: src/sys/modules/ext2fs/Makefile,v 1.7 2010/01/14 14:30:54 lulf Exp $

.PATH:	${.CURDIR}/../../fs/ext2fs
KMOD=	ext2fs
SRCS=	opt_ddb.h opt_quota.h opt_suiddir.h vnode_if.h \
	ext2_alloc.c ext2_balloc.c ext2_bmap.c ext2_inode.c \
	ext2_inode_cnv.c ext2_lookup.c ext2_subr.c ext2_vfsops.c \
	ext2_vnops.c

.include <bsd.kmod.mk>
