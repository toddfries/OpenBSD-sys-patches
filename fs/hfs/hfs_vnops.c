/*	$NetBSD: hfs_vnops.c,v 1.11 2008/09/03 22:57:46 gmcgarry Exp $	*/

/*-
 * Copyright (c) 2005, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yevgeny Binder and Dieter Baron.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */                                     

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1986, 1989, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


/*
 * Apple HFS+ filesystem
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hfs_vnops.c,v 1.11 2008/09/03 22:57:46 gmcgarry Exp $");

#ifdef _KERNEL_OPT
#include "opt_ipsec.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/vmmeter.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/buf.h>
#include <sys/dirent.h>
#include <sys/msgbuf.h>

#include <miscfs/fifofs/fifo.h>
#include <miscfs/specfs/specdev.h>

#include <fs/hfs/hfs.h>
#include <fs/hfs/unicode.h>

#include <miscfs/genfs/genfs.h>

int	hfs_vop_lookup		(void *);
int	hfs_vop_open		(void *);
int	hfs_vop_close		(void *);
int	hfs_vop_access		(void *);
int	hfs_vop_getattr	(void *);
int	hfs_vop_setattr	(void *);
int	hfs_vop_bmap		(void *);
int	hfs_vop_read		(void *);
int	hfs_vop_readdir	(void *);
int	hfs_vop_readlink	(void *);
int	hfs_vop_reclaim	(void *);
int	hfs_vop_print		(void *);


int (**hfs_vnodeop_p) (void *);
const struct vnodeopv_entry_desc hfs_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, hfs_vop_lookup },		/* lookup */
	{ &vop_create_desc, genfs_eopnotsupp },		/* create */
	{ &vop_whiteout_desc, genfs_eopnotsupp },	/* whiteout */
	{ &vop_mknod_desc, genfs_eopnotsupp },		/* mknod */
	{ &vop_open_desc, hfs_vop_open },		/* open */
	{ &vop_close_desc, hfs_vop_close },		/* close */
	{ &vop_access_desc, hfs_vop_access },		/* access */
	{ &vop_getattr_desc, hfs_vop_getattr },	/* getattr */
	{ &vop_setattr_desc, hfs_vop_setattr },	/* setattr */
	{ &vop_read_desc, hfs_vop_read },		/* read */
	{ &vop_write_desc, genfs_eopnotsupp },		/* write */
	{ &vop_ioctl_desc, genfs_eopnotsupp },		/* ioctl */
	{ &vop_fcntl_desc, genfs_fcntl },		/* fcntl */
	{ &vop_poll_desc, genfs_eopnotsupp },		/* poll */
	{ &vop_kqfilter_desc, genfs_kqfilter },		/* kqfilter */
	{ &vop_revoke_desc, genfs_eopnotsupp },		/* revoke */
	{ &vop_mmap_desc, genfs_mmap },			/* mmap */
	{ &vop_fsync_desc, genfs_nullop },		/* fsync */
	{ &vop_seek_desc, genfs_seek },			/* seek */
	{ &vop_remove_desc, genfs_eopnotsupp },		/* remove */
	{ &vop_link_desc, genfs_eopnotsupp },		/* link */
	{ &vop_rename_desc, genfs_eopnotsupp },		/* rename */
	{ &vop_mkdir_desc, genfs_eopnotsupp },		/* mkdir */
	{ &vop_rmdir_desc, genfs_eopnotsupp },		/* rmdir */
	{ &vop_symlink_desc, genfs_eopnotsupp },	/* symlink */
	{ &vop_readdir_desc, hfs_vop_readdir },	/* readdir */
	{ &vop_readlink_desc, hfs_vop_readlink },	/* readlink */
	{ &vop_abortop_desc, genfs_abortop },		/* abortop */
	{ &vop_inactive_desc, genfs_eopnotsupp },	/* inactive */
	{ &vop_reclaim_desc, hfs_vop_reclaim },	/* reclaim */
	{ &vop_lock_desc, genfs_lock },			/* lock */
	{ &vop_unlock_desc, genfs_unlock },		/* unlock */
	{ &vop_bmap_desc, hfs_vop_bmap },		/* bmap */
	{ &vop_strategy_desc, genfs_eopnotsupp },	/* strategy */
	{ &vop_print_desc, hfs_vop_print },		/* print */
	{ &vop_islocked_desc, genfs_islocked },		/* islocked */
	{ &vop_pathconf_desc, genfs_eopnotsupp },	/* pathconf */
	{ &vop_advlock_desc, genfs_eopnotsupp },	/* advlock */
	{ &vop_bwrite_desc, genfs_eopnotsupp },		/* bwrite */
	{ &vop_getpages_desc, genfs_getpages },		/* getpages */
	{ &vop_putpages_desc, genfs_putpages },		/* putpages */
	{ &vop_openextattr_desc, genfs_eopnotsupp },	/* openextattr */
	{ &vop_closeextattr_desc, genfs_eopnotsupp },	/* closeextattr */
	{ &vop_getextattr_desc, genfs_eopnotsupp },	/* getextattr */
	{ &vop_setextattr_desc, genfs_eopnotsupp },	/* setextattr */
	{ &vop_listextattr_desc, genfs_eopnotsupp },	/* listextattr */
	{ &vop_deleteextattr_desc, genfs_eopnotsupp },	/* deleteextattr */
	{ NULL, NULL }
};
const struct vnodeopv_desc hfs_vnodeop_opv_desc =
	{ &hfs_vnodeop_p, hfs_vnodeop_entries };

int (**hfs_specop_p) (void *);
const struct vnodeopv_entry_desc hfs_specop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, spec_lookup },		/* lookup */
	{ &vop_create_desc, spec_create },		/* create */
	{ &vop_mknod_desc, spec_mknod },		/* mknod */
	{ &vop_open_desc, spec_open },			/* open */
	{ &vop_close_desc, spec_close },		/* close */
	{ &vop_access_desc, hfs_vop_access },		/* access */
	{ &vop_getattr_desc, hfs_vop_getattr },	/* getattr */
	{ &vop_setattr_desc, hfs_vop_setattr },	/* setattr */
	{ &vop_read_desc, spec_read },			/* read */
	{ &vop_write_desc, spec_write },		/* write */
	{ &vop_ioctl_desc, spec_ioctl },		/* ioctl */
	{ &vop_fcntl_desc, genfs_fcntl },		/* fcntl */
	{ &vop_poll_desc, spec_poll },			/* poll */
	{ &vop_kqfilter_desc, spec_kqfilter },		/* kqfilter */
	{ &vop_revoke_desc, spec_revoke },		/* revoke */
	{ &vop_mmap_desc, spec_mmap },			/* mmap */
	{ &vop_fsync_desc, spec_fsync },		/* fsync */
	{ &vop_seek_desc, spec_seek },			/* seek */
	{ &vop_remove_desc, spec_remove },		/* remove */
	{ &vop_link_desc, spec_link },			/* link */
	{ &vop_rename_desc, spec_rename },		/* rename */
	{ &vop_mkdir_desc, spec_mkdir },		/* mkdir */
	{ &vop_rmdir_desc, spec_rmdir },		/* rmdir */
	{ &vop_symlink_desc, spec_symlink },		/* symlink */
	{ &vop_readdir_desc, spec_readdir },		/* readdir */
	{ &vop_readlink_desc, spec_readlink },		/* readlink */
	{ &vop_abortop_desc, spec_abortop },		/* abortop */
	{ &vop_inactive_desc, genfs_eopnotsupp },	/* inactive */
	{ &vop_reclaim_desc, hfs_vop_reclaim },	/* reclaim */
	{ &vop_lock_desc, genfs_lock },			/* lock */
	{ &vop_unlock_desc, genfs_unlock },		/* unlock */
	{ &vop_bmap_desc, spec_bmap },			/* bmap */
	{ &vop_strategy_desc, spec_strategy },		/* strategy */
	{ &vop_print_desc, hfs_vop_print },		/* print */
	{ &vop_islocked_desc, genfs_islocked },		/* islocked */
	{ &vop_pathconf_desc, spec_pathconf },		/* pathconf */
	{ &vop_advlock_desc, spec_advlock },		/* advlock */
	{ &vop_bwrite_desc, vn_bwrite },		/* bwrite */
	{ &vop_getpages_desc, spec_getpages },		/* getpages */
	{ &vop_putpages_desc, spec_putpages },		/* putpages */
#if 0
	{ &vop_openextattr_desc, ffs_openextattr },	/* openextattr */
	{ &vop_closeextattr_desc, ffs_closeextattr },	/* closeextattr */
	{ &vop_getextattr_desc, ffs_getextattr },	/* getextattr */
	{ &vop_setextattr_desc, ffs_setextattr },	/* setextattr */
	{ &vop_listextattr_desc, ffs_listextattr },	/* listextattr */
	{ &vop_deleteextattr_desc, ffs_deleteextattr },	/* deleteextattr */
#endif
	{ NULL, NULL }
};
const struct vnodeopv_desc hfs_specop_opv_desc =
	{ &hfs_specop_p, hfs_specop_entries };

int (**hfs_fifoop_p) (void *);
const struct vnodeopv_entry_desc hfs_fifoop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, fifo_lookup },		/* lookup */
	{ &vop_create_desc, fifo_create },		/* create */
	{ &vop_mknod_desc, fifo_mknod },		/* mknod */
	{ &vop_open_desc, fifo_open },			/* open */
	{ &vop_close_desc, fifo_close },		/* close */
	{ &vop_access_desc, hfs_vop_access },		/* access */
	{ &vop_getattr_desc, hfs_vop_getattr },	/* getattr */
	{ &vop_setattr_desc, hfs_vop_setattr },	/* setattr */
	{ &vop_read_desc, fifo_read },			/* read */
	{ &vop_write_desc, fifo_write },		/* write */
	{ &vop_ioctl_desc, fifo_ioctl },		/* ioctl */
	{ &vop_fcntl_desc, genfs_fcntl },		/* fcntl */
	{ &vop_poll_desc, fifo_poll },			/* poll */
	{ &vop_kqfilter_desc, fifo_kqfilter },		/* kqfilter */
	{ &vop_revoke_desc, fifo_revoke },		/* revoke */
	{ &vop_mmap_desc, fifo_mmap },			/* mmap */
	{ &vop_fsync_desc, fifo_fsync },		/* fsync */
	{ &vop_seek_desc, fifo_seek },			/* seek */
	{ &vop_remove_desc, fifo_remove },		/* remove */
	{ &vop_link_desc, fifo_link },			/* link */
	{ &vop_rename_desc, fifo_rename },		/* rename */
	{ &vop_mkdir_desc, fifo_mkdir },		/* mkdir */
	{ &vop_rmdir_desc, fifo_rmdir },		/* rmdir */
	{ &vop_symlink_desc, fifo_symlink },		/* symlink */
	{ &vop_readdir_desc, fifo_readdir },		/* readdir */
	{ &vop_readlink_desc, fifo_readlink },		/* readlink */
	{ &vop_abortop_desc, fifo_abortop },		/* abortop */
	{ &vop_inactive_desc, genfs_eopnotsupp },	/* inactive */
	{ &vop_reclaim_desc, hfs_vop_reclaim },	/* reclaim */
	{ &vop_lock_desc, genfs_lock },			/* lock */
	{ &vop_unlock_desc, genfs_unlock },		/* unlock */
	{ &vop_bmap_desc, fifo_bmap },			/* bmap */
	{ &vop_strategy_desc, fifo_strategy },		/* strategy */
	{ &vop_print_desc, hfs_vop_print },		/* print */
	{ &vop_islocked_desc, genfs_islocked },		/* islocked */
	{ &vop_pathconf_desc, fifo_pathconf },		/* pathconf */
	{ &vop_advlock_desc, fifo_advlock },		/* advlock */
	{ &vop_bwrite_desc, vn_bwrite },		/* bwrite */
	{ &vop_putpages_desc, fifo_putpages }, 		/* putpages */
#if 0
	{ &vop_openextattr_desc, ffs_openextattr },	/* openextattr */
	{ &vop_closeextattr_desc, ffs_closeextattr },	/* closeextattr */
	{ &vop_getextattr_desc, ffs_getextattr },	/* getextattr */
	{ &vop_setextattr_desc, ffs_setextattr },	/* setextattr */
	{ &vop_listextattr_desc, ffs_listextattr },	/* listextattr */
	{ &vop_deleteextattr_desc, ffs_deleteextattr },	/* deleteextattr */
#endif
	{ NULL, NULL }
};
const struct vnodeopv_desc hfs_fifoop_opv_desc =
	{ &hfs_fifoop_p, hfs_fifoop_entries };

int
hfs_vop_lookup(void *v)
{
	struct vop_lookup_args /* {
		struct vnode * a_dvp;
		struct vnode ** a_vpp;
		struct componentname * a_cnp;
	} */ *ap = v;
	struct buf *bp;			/* a buffer of directory entries */
	struct componentname *cnp;
	struct hfsnode *dp;	/* hfsnode for directory being searched */
	kauth_cred_t cred;
	struct vnode **vpp;		/* resultant vnode */
	struct vnode *pdp;		/* saved dp during symlink work */
	struct vnode *tdp;		/* returned by VFS_VGET */
	struct vnode *vdp;		/* vnode for directory being searched */
	hfs_catalog_key_t key;	/* hfs+ catalog search key for requested child */
	hfs_catalog_keyed_record_t rec; /* catalog record of requested child */
	unichar_t* unicn;		/* name of component, in Unicode */
	const char *pname;
	int error;
	int flags;
	int result;				/* result of libhfs operations */

#ifdef HFS_DEBUG
	printf("VOP = hfs_vop_lookup()\n");
#endif /* HFS_DEBUG */

	bp = NULL;
	cnp = ap->a_cnp;
	cred = cnp->cn_cred;
	vdp = ap->a_dvp;
	dp = VTOH(vdp);
	error = 0;
	pname = cnp->cn_nameptr;
	result = 0;
	unicn = NULL;
	vpp = ap->a_vpp;
	*vpp = NULL;

	flags = cnp->cn_flags;


	/*
	 * Check accessiblity of directory.
	 */
	if ((error = VOP_ACCESS(vdp, VEXEC, cred)) != 0)
		return error;

	if ((flags & ISLASTCN) && (vdp->v_mount->mnt_flag & MNT_RDONLY) &&
	    (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME))
		return EROFS;

	/*
	 * We now have a segment name to search for, and a directory to search.
	 *
	 * Before tediously performing a linear scan of the directory,
	 * check the name cache to see if the directory/name pair
	 * we are looking for is known already.
	 */
/* XXX Cache disabled until we can make sure it works. */
/*	if ((error = cache_lookup(vdp, vpp, cnp)) >= 0)
		return error; */


/*	if (cnp->cn_namelen == 1 && *pname == '.') {
		*vpp = vdp;
		VREF(vdp);
		return (0);
	}*/
	
	pdp = vdp;
	if (flags & ISDOTDOT) {
/*printf("DOTDOT ");*/
		VOP_UNLOCK(pdp, 0);	/* race to get the inode */
		error = VFS_VGET(vdp->v_mount, dp->h_parent, &tdp);
		vn_lock(pdp, LK_EXCLUSIVE | LK_RETRY);
		if (error != 0)
			goto error;
		*vpp = tdp;
/*	} else if (dp->h_rec.u.cnid == rec.file.u.cnid) {*/
	} else if (cnp->cn_namelen == 1 && pname[0] == '.') {
/*printf("DOT ");*/
		VREF(vdp);	/* we want ourself, ie "." */
		*vpp = vdp;
	} else {
		hfs_callback_args cbargs;
		uint8_t len;

		hfslib_init_cbargs(&cbargs);

		/* XXX: when decomposing, string could grow
		   and we have to handle overflow */
		unicn = malloc(cnp->cn_namelen*sizeof(unicn[0]), M_TEMP, M_WAITOK);
		len = utf8_to_utf16(unicn, cnp->cn_namelen,
				    cnp->cn_nameptr, cnp->cn_namelen, 0, NULL);
		/* XXX: check conversion errors? */
		if (hfslib_make_catalog_key(VTOH(vdp)->h_rec.u.cnid, len, unicn,
			&key) == 0) {
/*printf("ERROR in hfslib_make_catalog_key\n");*/
			error = EINVAL;
			goto error;
		}
			
		result = hfslib_find_catalog_record_with_key(&dp->h_hmp->hm_vol, &key,
			&rec, &cbargs);
		if (result > 0) {
			error = EINVAL;
			goto error;
		}
		if (result < 0) {
			if (cnp->cn_nameiop == CREATE)
				error = EROFS;
			else
				error = ENOENT;
			goto error;
		}

		if (rec.file.user_info.file_type == HFS_HARD_LINK_FILE_TYPE
		    && rec.file.user_info.file_creator
		    == HFS_HFSLUS_CREATOR) {
			if (hfslib_get_hardlink(&dp->h_hmp->hm_vol,
					 rec.file.bsd.special.inode_num,
					 &rec, &cbargs) != 0) {
				error = EINVAL;
				goto error;
			}
		}

		if (rec.type == HFS_REC_FILE
		    && strcmp(cnp->cn_nameptr+cnp->cn_namelen, "/rsrc") == 0
		    && rec.file.rsrc_fork.logical_size > 0) {
		    /* advance namei next pointer to end of stirng */
		    cnp->cn_consume = 5;
		    cnp->cn_flags &= ~REQUIREDIR; /* XXX: needed? */
		    error = hfs_vget_internal(vdp->v_mount, rec.file.cnid,
			HFS_RSRCFORK, &tdp);
		}
		else
			error = VFS_VGET(vdp->v_mount, rec.file.cnid, &tdp);
		if (error != 0)
			goto error;
		*vpp = tdp;
	}
/*printf("\n");*/
	/*
	 * Insert name into cache if appropriate.
	 */
/* XXX Cache disabled until we can make sure it works. */
/*	if (cnp->cn_flags & MAKEENTRY)
		cache_enter(vdp, *vpp, cnp);*/
	
	error = 0;

	/* FALLTHROUGH */
error:
	if (unicn != NULL)
		free(unicn, M_TEMP);

	return error;
}

int
hfs_vop_open(void *v)
{
#if 0
	struct vop_open_args /* {
		struct vnode *a_vp;
		int a_mode;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct hfsnode *hn = VTOH(ap->a_vp);
#endif
#ifdef HFS_DEBUG
	printf("VOP = hfs_vop_open()\n");
#endif /* HFS_DEBUG */

	/*
	 * XXX This is a good place to read and cache the file's extents to avoid
	 * XXX doing it upon every read/write. Must however keep the cache in sync
	 * XXX when the file grows/shrinks. (So would that go in vop_truncate?)
	 */
	
	return 0;
}

int
hfs_vop_close(void *v)
{
#if 0
	struct vop_close_args /* {
		struct vnode *a_vp;
		int a_fflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct hfsnode *hn = VTOH(ap->a_vp);
#endif
#ifdef HFS_DEBUG
	printf("VOP = hfs_vop_close()\n");
#endif /* HFS_DEBUG */

	/* Release extents cache here. */
	
	return 0;
}

int
hfs_vop_access(void *v)
{
	struct vop_access_args /* {
		struct vnode *a_vp;
		int a_mode;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct vattr va;
	int error;

#ifdef HFS_DEBUG
	printf("VOP = hfs_vop_access()\n");
#endif /* HFS_DEBUG */

	/*
	 * Disallow writes on files, directories, and symlinks
	 * since we have no write support yet.
	 */

	if (ap->a_mode & VWRITE) {
		switch (ap->a_vp->v_type) {
		case VDIR:
		case VLNK:
		case VREG:
			return EROFS;
		default:
			break;
		}
	}

	if ((error = VOP_GETATTR(ap->a_vp, &va, ap->a_cred)) != 0)
		return error;

	return vaccess(va.va_type, va.va_mode, va.va_uid, va.va_gid,
	    ap->a_mode, ap->a_cred);
}

int
hfs_vop_getattr(void *v)
{
	struct vop_getattr_args /* {
		struct vnode	*a_vp;
		struct vattr	*a_vap;
		struct ucred	*a_cred;
	} */ *ap = v;
	struct vnode	*vp;
	struct hfsnode	*hp;
	struct vattr	*vap;
	hfs_bsd_data_t *bsd;
	hfs_fork_t     *fork;

#ifdef HFS_DEBUG
	printf("VOP = hfs_vop_getattr()\n");
#endif /* HFS_DEBUG */

	vp = ap->a_vp;
	hp = VTOH(vp);
	vap = ap->a_vap;
	
	vattr_null(vap);

	/*
	 * XXX Cannot trust permissions/modes/flags stored in an HFS+ catalog record
	 * XXX record those values are not set on files created under Mac OS 9.
	 */
	vap->va_type = ap->a_vp->v_type;
	if (hp->h_rec.u.rec_type == HFS_REC_FILE) {
		if (hp->h_fork == HFS_RSRCFORK)
			fork = &hp->h_rec.file.rsrc_fork;
		else
			fork = &hp->h_rec.file.data_fork;
		vap->va_fileid = hp->h_rec.file.cnid;
		bsd = &hp->h_rec.file.bsd;
		vap->va_bytes = fork->total_blocks * HFS_BLOCKSIZE(vp);
		vap->va_size = fork->logical_size;
		hfs_time_to_timespec(hp->h_rec.file.date_created, &vap->va_ctime);
		hfs_time_to_timespec(hp->h_rec.file.date_content_mod, &vap->va_mtime);
		hfs_time_to_timespec(hp->h_rec.file.date_accessed, &vap->va_atime);
		vap->va_nlink = 1;
	}
	else if (hp->h_rec.u.rec_type == HFS_REC_FLDR) {
		vap->va_fileid = hp->h_rec.folder.cnid;
		bsd = &hp->h_rec.folder.bsd;
		vap->va_size = 512; /* XXX Temporary */
		vap->va_bytes = 512; /* XXX Temporary */
		hfs_time_to_timespec(hp->h_rec.folder.date_created, &vap->va_ctime);
		hfs_time_to_timespec(hp->h_rec.folder.date_content_mod,&vap->va_mtime);
		hfs_time_to_timespec(hp->h_rec.folder.date_accessed, &vap->va_atime);
		vap->va_nlink = 2; /* XXX */
	}
	else {
		printf("hfslus: hfs_vop_getattr(): invalid record type %i",
			hp->h_rec.u.rec_type);
		return EINVAL;
	}

	if ((bsd->file_mode & S_IFMT) == 0) {
		/* no bsd permissions recorded, use default values */
		if (hp->h_rec.u.rec_type == HFS_REC_FILE)
			vap->va_mode = (S_IFREG | HFS_DEFAULT_FILE_MODE);
		else
			vap->va_mode = (S_IFDIR | HFS_DEFAULT_DIR_MODE);
		vap->va_uid = HFS_DEFAULT_UID;
		vap->va_gid = HFS_DEFAULT_GID;
	}
	else {
		vap->va_mode = bsd->file_mode;
		vap->va_uid = bsd->owner_id;
		vap->va_gid = bsd->group_id;
		if ((vap->va_mode & S_IFMT) == S_IFCHR
		    || (vap->va_mode & S_IFMT) == S_IFBLK) {
			vap->va_rdev
			    = HFS_CONVERT_RDEV(bsd->special.raw_device);
		}
		else if (bsd->special.link_count != 0) {
		    /* XXX: only if in metadata directory */
		    vap->va_nlink = bsd->special.link_count;
		}
	}

	vap->va_fsid = hp->h_dev;
	vap->va_blocksize = hp->h_hmp->hm_vol.vh.block_size;
	vap->va_gen = 1;
	vap->va_flags = 0;
	
	return 0;
}

int
hfs_vop_setattr(void *v)
{
	struct vop_setattr_args /* {
		struct vnode	*a_vp;
		struct vattr	*a_vap;
		kauth_cred_t	a_cred;
	} */ *ap = v;
	struct vattr	*vap;
	struct vnode	*vp;

	vap = ap->a_vap;
	vp = ap->a_vp;

	/*
	 * Check for unsettable attributes.
	 */
	if ((vap->va_type != VNON) || (vap->va_nlink != VNOVAL) ||
	    (vap->va_fsid != VNOVAL) || (vap->va_fileid != VNOVAL) ||
	    (vap->va_blocksize != VNOVAL) || (vap->va_rdev != VNOVAL) ||
	    ((int)vap->va_bytes != VNOVAL) || (vap->va_gen != VNOVAL)) {
		return EINVAL;
	}

	/* XXX: needs revisiting for write support */
	if (vap->va_flags != VNOVAL
	    || vap->va_uid != (uid_t)VNOVAL || vap->va_gid != (gid_t)VNOVAL
	    || vap->va_atime.tv_sec != VNOVAL || vap->va_mtime.tv_sec != VNOVAL
	    || vap->va_birthtime.tv_sec != VNOVAL) {
		return EROFS;
	}

	if (vap->va_size != VNOVAL) {
		/*
		 * Disallow write attempts on read-only file systems;
		 * unless the file is a socket, fifo, or a block or
		 * character device resident on the file system.
		 */
		switch (vp->v_type) {
		case VDIR:
			return EISDIR;
		case VCHR:
		case VBLK:
		case VFIFO:
			break;
		case VREG:
			 return EROFS;
		default:
			return EOPNOTSUPP;
		}
	}

	return 0;
}

int
hfs_vop_bmap(void *v)
{
	struct vop_bmap_args /* {
		struct vnode *a_vp;
		daddr_t  a_bn;
		struct vnode **a_vpp;
		daddr_t *a_bnp;
		int *a_runp;
	} */ *ap = v;
	struct vnode *vp;
	struct hfsnode *hp;
	daddr_t lblkno;
	hfs_callback_args cbargs;
	hfs_libcb_argsread argsread;
	hfs_extent_descriptor_t *extents;
	uint16_t numextents, i;
	int bshift;

	vp = ap->a_vp;
	hp = VTOH(vp);
	lblkno = ap->a_bn;
	bshift = vp->v_mount->mnt_fs_bshift;

	/*
	 * Check for underlying vnode requests and ensure that logical
	 * to physical mapping is requested.
	 */
	if (ap->a_vpp != NULL)
		*ap->a_vpp = hp->h_devvp;
	if (ap->a_bnp == NULL)
		return (0);

	hfslib_init_cbargs(&cbargs);
	argsread.cred = NULL;
	argsread.l = NULL;
	cbargs.read = &argsread;

	numextents = hfslib_get_file_extents(&hp->h_hmp->hm_vol,
	    hp->h_rec.u.cnid, hp->h_fork, &extents, &cbargs);

	/* XXX: is this correct for 0-length files? */
	if (numextents == 0)
		return EBADF;

	for (i=0; i<numextents; i++) {
		if (lblkno < extents[i].block_count)
			break;
		lblkno -= extents[i].block_count;
	}

	if (i == numextents) {
		/* XXX: block number past EOF */
		i--;
		lblkno += extents[i].block_count;
	}

	*ap->a_bnp = ((extents[i].start_block + lblkno)
		      << (bshift-DEV_BSHIFT))
	    + (hp->h_hmp->hm_vol.offset >> DEV_BSHIFT);

	if (ap->a_runp) {
		int nblk;

		nblk = extents[i].block_count - lblkno - 1;
		if (nblk <= 0)
			*ap->a_runp = 0;
		else if (nblk > MAXBSIZE >> bshift)
			*ap->a_runp = (MAXBSIZE >> bshift) - 1;
		else
			*ap->a_runp = nblk;

	}

	free(extents, /*M_HFSMNT*/ M_TEMP);

	return 0;
}

int
hfs_vop_read(void *v)
{
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct vnode *vp;
	struct hfsnode *hp;
	struct uio *uio;
	uint64_t fsize; /* logical size of file */
	int advice;
        int error;

	vp = ap->a_vp;
	hp = VTOH(vp);
	uio = ap->a_uio;
	if (hp->h_fork == HFS_RSRCFORK)
		fsize = hp->h_rec.file.rsrc_fork.logical_size;
	else
		fsize = hp->h_rec.file.data_fork.logical_size;
	error = 0;
	advice = IO_ADV_DECODE(ap->a_ioflag);

        if (uio->uio_offset < 0)
                return EINVAL;

        if (uio->uio_resid == 0 || uio->uio_offset >= fsize)
                return 0;

        if (vp->v_type != VREG && vp->v_type != VLNK)
		return EINVAL;

	error = 0;
	while (uio->uio_resid > 0 && error == 0) {
		int flags;
		vsize_t len;
		void *win;

		len = MIN(uio->uio_resid, fsize - uio->uio_offset);
		if (len == 0)
			break;
		
		win = ubc_alloc(&vp->v_uobj, uio->uio_offset, &len,
				advice, UBC_READ);
		error = uiomove(win, len, uio);
		flags = UBC_WANT_UNMAP(vp) ? UBC_UNMAP : 0;
		ubc_release(win, flags);
	}

        return error;
}

int
hfs_vop_readdir(void *v)
{
struct vop_readdir_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		kauth_cred_t a_cred;
		int *a_eofflag;
		off_t **a_cookies;
		int a_*ncookies;
	} */ *ap = v;

#ifdef HFS_DEBUG
	printf("VOP = hfs_vop_readdir()\n");
#endif /* HFS_DEBUG */

	struct dirent curent; /* the dirent entry we're currently constructing */
	struct hfsnode *hp;
	hfs_catalog_keyed_record_t *children;
	hfs_unistr255_t *childnames;
	hfs_callback_args cbargs;
	hfs_libcb_argsread argsread;
	struct uio *uio;
	off_t bufoff; /* current position in buffer relative to start of dirents */
	uint32_t numchildren;
	uint32_t curchild; /* index of child we're currently stuffing into dirent */
	size_t namlen;
	int error;
	int i; /* dummy variable */
	
	bufoff = 0;
	children = NULL;
	error = 0;
	numchildren = 0;
	hp = VTOH(ap->a_vp);
	uio = ap->a_uio;
	
	if (uio->uio_offset < 0)
		return EINVAL;
	if (ap->a_eofflag != NULL)
		*ap->a_eofflag = 0;

/* XXX Inform that we don't support NFS, for now. */
/*	if(ap->a_eofflag != NULL || ap->a_cookies != NULL || ap->a_ncookies != NULL)
		return EOPNOTSUPP;*/
/*printf("READDIR uio: offset=%i, resid=%i\n",
(int)uio->uio_offset, (int)uio->uio_resid);*/
	hfslib_init_cbargs(&cbargs);
	argsread.cred = ap->a_cred;
	argsread.l = NULL;
	cbargs.read = &argsread;
	
	/* XXX Should we cache this? */
	if (hfslib_get_directory_contents(&hp->h_hmp->hm_vol, hp->h_rec.u.cnid,
		&children, &childnames, &numchildren, &cbargs) != 0) {
/*printf("NOENT\n");*/
		error = ENOENT;
		goto error;
	}

/*printf("numchildren = %i\n", numchildren);*/
	for (curchild = 0; curchild < numchildren && uio->uio_resid>0; curchild++) {
		namlen = utf16_to_utf8(curent.d_name, MAXNAMLEN, 
				       childnames[curchild].unicode,
				       childnames[curchild].length,
				       0, NULL);
		/* XXX: check conversion errors? */
		if (namlen > MAXNAMLEN) {
			/* XXX: how to handle name too long? */
			continue;
		}
		curent.d_namlen = namlen;
		curent.d_reclen = _DIRENT_SIZE(&curent);
		
		/* Skip to desired dirent. */
		if ((bufoff += curent.d_reclen) - curent.d_reclen < uio->uio_offset)
			continue;

		/* Make sure we don't return partial entries. */
		if (uio->uio_resid < curent.d_reclen) {
/*printf("PARTIAL ENTRY\n");*/
			if (ap->a_eofflag != NULL)
				*ap->a_eofflag = 1;
			break;
		}
			
		curent.d_fileno = children[curchild].file.cnid;
		switch (hfs_catalog_keyed_record_vtype(children+curchild)) {
		case VREG:
			curent.d_type = DT_REG;
			break;
		case VDIR:
			curent.d_type = DT_DIR;
			break;
		case VBLK:
			curent.d_type = DT_BLK;
			break;
		case VCHR:
			curent.d_type = DT_CHR;
			break;
		case VLNK:
			curent.d_type = DT_LNK;
			break;
		case VSOCK:
			curent.d_type = DT_SOCK;
			break;
		case VFIFO:
			curent.d_type = DT_FIFO;
			break;
		default:
			curent.d_type = DT_UNKNOWN;
			break;
		}
/*printf("curchildname = %s\t\t", curchildname);*/
		/* pad curent.d_name to aligned byte boundary */
                for (i=curent.d_namlen;
                     i<curent.d_reclen-_DIRENT_NAMEOFF(&curent); i++)
                        curent.d_name[i] = 0;
			
/*printf("curent.d_name = %s\n", curent.d_name);*/

		if ((error = uiomove(&curent, curent.d_reclen, uio)) != 0)
			goto error;
	}
	

	/* FALLTHROUGH */
	
error:
	if (numchildren > 0) {
		if (children != NULL)
			free(children, M_TEMP);
		if (childnames != NULL)
			free(childnames, M_TEMP);
	}

/*if (error)
printf("ERROR = %i\n", error);*/
	return error;
}

int
hfs_vop_readlink(void *v) {
	struct vop_readlink_args /* {
		struct vnode *a_vp;
        	struct uio *a_uio;
        	kauth_cred_t a_cred;
	} */ *ap = v;

	return VOP_READ(ap->a_vp, ap->a_uio, 0, ap->a_cred);
}

int
hfs_vop_reclaim(void *v)
{
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct vnode *vp;
	struct hfsnode *hp;
	struct hfsmount *hmp;
	
#ifdef HFS_DEBUG
	printf("VOP = hfs_vop_reclaim()\n");
#endif /* HFS_DEBUG */

	vp = ap->a_vp;
	hp = VTOH(vp);
	hmp = hp->h_hmp;

	/* Remove the hfsnode from its hash chain. */
	hfs_nhashremove(hp);

	/* Purge name lookup cache. */
	cache_purge(vp);
	
	/* Decrement the reference count to the volume's device. */
	if (hp->h_devvp) {
		vrele(hp->h_devvp);
		hp->h_devvp = 0;
	}
	
	genfs_node_destroy(vp);
	FREE(vp->v_data, M_TEMP);
	vp->v_data = 0;

	return 0;
}

int
hfs_vop_print(void *v)
{
	struct vop_print_args /* {
		struct vnode	*a_vp;
	} */ *ap = v;
	struct vnode	*vp;
	struct hfsnode	*hp;

#ifdef HFS_DEBUG
	printf("VOP = hfs_vop_print()\n");
#endif /* HFS_DEBUG */

	vp = ap->a_vp;
	hp = VTOH(vp);

	printf("dummy = %X\n", (unsigned)hp->dummy);
	printf("\n");

	return 0;
}
