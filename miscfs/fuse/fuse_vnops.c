/* $OpenBSD$ */
/*
 * Copyright (c) 2012-2013 Sylvestre Gallon <ccna.syl@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/dirent.h>
#include <sys/specdev.h>
#include <sys/poll.h>
#include <sys/lockf.h>
#include <sys/namei.h>

#include "fuse_kernel.h"
#include "fuse_node.h"
#include "fusefs.h"

#ifdef	FUSE_DEBUG_VNOP
#define	DPRINTF(fmt, arg...)	printf("fuse vnop: " fmt, ##arg)
#else
#define	DPRINTF(fmt, arg...)
#endif


/*
 * Prototypes for fusefs vnode ops
 */
int	fusefs_lookup(void *);
int	fusefs_open(void *);
int	fusefs_close(void *);
int	fusefs_access(void *);
int	fusefs_getattr(void *);
int	fusefs_setattr(void *);
int	fusefs_ioctl(void *);
int	fusefs_link(void *);
int	fusefs_symlink(void *);
int	fusefs_readdir(void *);
int	fusefs_readlink(void *);
int	fusefs_inactive(void *);
int	fusefs_reclaim(void *);
int	fusefs_print(void *);
int	fusefs_create(void *);
int	fusefs_mknod(void *);
int	fusefs_read(void *);
int	fusefs_write(void *);
int	fusefs_poll(void *);
int	fusefs_fsync(void *);
int	fusefs_remove(void *);
int	fusefs_rename(void *);
int	fusefs_mkdir(void *);
int	fusefs_rmdir(void *);
int	fusefs_strategy(void *);
int	fusefs_lock(void *);
int	fusefs_unlock(void *);
int	fusefs_islocked(void *);
int	fusefs_advlock(void *);
int	readdir_process_data(void *buff, int len, struct uio *uio);

struct vops fusefs_vops = {
	.vop_lookup	= fusefs_lookup,
	.vop_create	= fusefs_create,
	.vop_mknod	= fusefs_mknod,
	.vop_open	= fusefs_open,
	.vop_close	= fusefs_close,
	.vop_access	= fusefs_access,
	.vop_getattr	= fusefs_getattr,
	.vop_setattr	= fusefs_setattr,
	.vop_read	= fusefs_read,
	.vop_write	= fusefs_write,
	.vop_ioctl	= fusefs_ioctl,
	.vop_poll	= fusefs_poll,
	.vop_fsync	= fusefs_fsync,
	.vop_remove	= fusefs_remove,
	.vop_link	= fusefs_link,
	.vop_rename	= fusefs_rename,
	.vop_mkdir	= fusefs_mkdir,
	.vop_rmdir	= fusefs_rmdir,
	.vop_symlink	= fusefs_symlink,
	.vop_readdir	= fusefs_readdir,
	.vop_readlink	= fusefs_readlink,
	.vop_abortop	= vop_generic_abortop,
	.vop_inactive	= fusefs_inactive,
	.vop_reclaim	= fusefs_reclaim,
	.vop_lock	= fusefs_lock,
	.vop_unlock	= fusefs_unlock,
	.vop_bmap	= vop_generic_bmap,
	.vop_strategy	= fusefs_strategy,
	.vop_print	= fusefs_print,
	.vop_islocked	= fusefs_islocked,
	.vop_pathconf	= spec_pathconf,
	.vop_advlock	= fusefs_advlock,
};

void
fuse_internal_attr_fat2vat(struct mount *mp,
                           struct fuse_attr *fat,
                           struct vattr *vap)
{
	vap->va_fsid = mp->mnt_stat.f_fsid.val[0];
	vap->va_fileid = fat->ino;/* XXX cast from 64 bits to 32 */
	vap->va_mode = fat->mode & ~S_IFMT;
	vap->va_nlink = fat->nlink;
	vap->va_uid = fat->uid;
	vap->va_gid = fat->gid;
	vap->va_rdev = fat->rdev;
	vap->va_size = fat->size;
	vap->va_atime.tv_sec = fat->atime;
	vap->va_atime.tv_nsec = fat->atimensec;
	vap->va_mtime.tv_sec = fat->mtime;
	vap->va_mtime.tv_nsec = fat->mtimensec;
	vap->va_ctime.tv_sec = fat->ctime;
	vap->va_ctime.tv_nsec = fat->ctimensec;
	vap->va_blocksize = fat->blksize;
	vap->va_type = IFTOVT(fat->mode);

#if (S_BLKSIZE == 512)
	/* Optimize this case */
	vap->va_bytes = fat->blocks << 9;
#else
	vap->va_bytes = fat->blocks * S_BLKSIZE;
#endif

}

int
fusefs_open(void *v)
{
	struct vop_open_args *ap;
	struct fuse_node *ip;
	struct fuse_mnt *fmp;
	enum fufh_type fufh_type = FUFH_RDONLY;
	int flags = O_RDONLY;
	int error;
	int isdir;

	DPRINTF("fusefs_open\n");

	ap = v;
	ip = VTOI(ap->a_vp);
	fmp = (struct fuse_mnt *)ip->ufs_ino.i_ump;

	if (!fmp->sess_init) {
		return (0);
	}

	DPRINTF("inode = %i mode=0x%x\n", ip->ufs_ino.i_number, ap->a_mode);

	isdir = 0;
	if (ip->vtype == VDIR) {
		isdir = 1;
	} else {
		if ((ap->a_mode & FREAD) && (ap->a_mode & FWRITE)) {
			fufh_type = FUFH_RDWR;
			flags = O_RDWR;
		} else if (ap->a_mode  & (FWRITE)) {
			fufh_type = FUFH_WRONLY;
			flags = O_WRONLY;
		}
	}

	/* already open i think all is ok */
	if (ip->fufh[fufh_type].fh_type != FUFH_INVALID)
		return (0);

	error = fuse_file_open(fmp, ip, fufh_type, flags, isdir, ap->a_p);

	if (error)
		return (error);

	DPRINTF("file open fd : %i\n", ip->fufh[fufh_type].fh_id);

	return (error);
}

int
fusefs_close(void *v)
{
	struct vop_close_args *ap;
	struct fuse_node *ip;
	struct fuse_mnt *fmp;
	enum fufh_type fufh_type = FUFH_RDONLY;
	int isdir, i;

	DPRINTF("fusefs_close\n");

	ap = v;
	ip = VTOI(ap->a_vp);
	fmp = (struct fuse_mnt *)ip->ufs_ino.i_ump;

	if (!fmp->sess_init) {
		return (0);
	}

	if (ip->vtype == VDIR) {
		isdir = 1;

		if (ip->fufh[fufh_type].fh_type != FUFH_INVALID)
			return (fuse_file_close(fmp, ip, fufh_type, O_RDONLY,
			    isdir, ap->a_p));
	} else {
		if (ap->a_fflag & IO_NDELAY) {
			return (0);
		}

		if ((ap->a_fflag & FREAD) && (ap->a_fflag & FWRITE)) {
			fufh_type = FUFH_RDWR;
		} else if (ap->a_fflag  & (FWRITE)) {
			fufh_type = FUFH_WRONLY;
		}
	}

	/*
	 * if fh not valid lookup for another valid fh in vnode.
	 * panic if there's not fh valid
	 */
	if (ip->fufh[fufh_type].fh_type != FUFH_INVALID) {
		for (i = 0; i < FUFH_MAXTYPE; i++)
			if (ip->fufh[fufh_type].fh_type != FUFH_INVALID)
				break;
		return (0);
	}

	return (0);
}

int
fusefs_access(void *v)
{
	struct fuse_access_in *access;
	struct vop_access_args *ap;
	struct fuse_node *ip;
	struct fuse_mnt *fmp;
	struct fuse_msg *msg;
	struct proc *p;
	uint32_t mask = 0;
	int error = 0;

	DPRINTF("fusefs_access\n");

	ap = v;
	p = ap->a_p;
	ip = VTOI(ap->a_vp);
	fmp = (struct fuse_mnt *)ip->ufs_ino.i_ump;

	if (!fmp->sess_init || (fmp->undef_op & UNDEF_ACCESS))
		goto system_check;

	if (ap->a_vp->v_type == VLNK)
		goto system_check;

	if (ap->a_vp->v_type == VREG && (ap->a_mode & VWRITE & VEXEC))
		goto system_check;

	if ((ap->a_mode & VWRITE) && (fmp->mp->mnt_flag & MNT_RDONLY))
		return (EACCES);

	if ((ap->a_mode & VWRITE) != 0) {
		mask |= 0x2;
	}
	if ((ap->a_mode & VREAD) != 0) {
		mask |= 0x4;
	}
	if ((ap->a_mode & VEXEC) != 0) {
		mask |= 0x1;
	}

	msg = fuse_alloc_in(fmp, sizeof(*access), 0, &fuse_sync_it, msg_intr);

	access = (struct fuse_access_in *)msg->data;
	access->mask = mask;

	fuse_make_in(fmp->mp, &msg->hdr, msg->len, FUSE_ACCESS,
	    ip->ufs_ino.i_number, p);

	error = fuse_send_in(fmp, msg);

	if (error) {
		if (error == ENOSYS) {
			fmp->undef_op |= UNDEF_ACCESS;
			fuse_clean_msg(msg);
			goto system_check;
		}

		DPRINTF("access error %i\n", error);
		fuse_clean_msg(msg);
		return (error);
	}

	fuse_clean_msg(msg);
	return (error);

system_check:
	return (vaccess(ap->a_vp->v_type, ip->cached_attrs.va_mode & ALLPERMS,
	    ip->cached_attrs.va_uid, ip->cached_attrs.va_gid, ap->a_mode,
	    ap->a_cred));
}

int
fusefs_getattr(void *v)
{
	struct vop_getattr_args *ap = v;
	struct vnode *vp = ap->a_vp;
	struct fuse_mnt *fmp;
	struct vattr *vap = ap->a_vap;
	struct proc *p = ap->a_p;
	struct fuse_attr_out *fat;
	struct fuse_node *ip;
	struct fuse_msg *msg;
	int error = 0;

	DPRINTF("fusefs_getattr\n");

	ip = VTOI(vp);
	fmp = (struct fuse_mnt *)ip->ufs_ino.i_ump;

	if (!fmp->sess_init) {
		DPRINTF("Goto fake\n");
		goto fake;
	}

	msg = fuse_alloc_in(fmp, 0, sizeof(*fat), &fuse_sync_resp, msg_buff);

	fuse_make_in(fmp->mp, &msg->hdr, msg->len, FUSE_GETATTR,
	    ip->ufs_ino.i_number, p);

	error = fuse_send_in(fmp, msg);

	if (error) {
		DPRINTF("getattr error\n");
		fuse_clean_msg(msg);
		return (error);
	}

	if (msg->buff.data_rcv == NULL) {
		DPRINTF("getattr ack ==> Wrong!\n");
		fuse_clean_msg(msg);
		goto fake;
	}

	fat = (struct fuse_attr_out *)msg->buff.data_rcv;
	fuse_internal_attr_fat2vat(fmp->mp, &fat->attr, vap);

	memcpy(&ip->cached_attrs, vap, sizeof(*vap));
	fuse_clean_msg(msg);
	DPRINTF("end of fusefs_getattr\n");
	return (error);
fake:
	bzero(vap, sizeof(*vap));
	vap->va_type = vp->v_type;
	DPRINTF("end of fusefs_getattr fake\n");
	return (0);
}

int
fusefs_setattr(void *v)
{
	struct vop_setattr_args *ap = v;
	struct vattr *vap = ap->a_vap;
	struct vnode *vp = ap->a_vp;
	struct fuse_node *ip = VTOI(vp);
	struct proc *p = ap->a_p;
	struct fuse_setattr_in *fsi;
	struct fuse_attr_out *fao;
	struct fuse_mnt *fmp;
	struct fuse_msg *msg;
	int error = 0;

	DPRINTF("fusefs_setattr\n");
	fmp = (struct fuse_mnt *)ip->ufs_ino.i_ump;
	/*
	 * Check for unsettable attributes.
	 */
	if ((vap->va_type != VNON) || (vap->va_nlink != VNOVAL) ||
		(vap->va_fsid != VNOVAL) || (vap->va_fileid != VNOVAL) ||
		(vap->va_blocksize != VNOVAL) || (vap->va_rdev != VNOVAL) ||
		((int)vap->va_bytes != VNOVAL) || (vap->va_gen != VNOVAL)) {
		return (EINVAL);
	}

	if (!fmp->sess_init || (fmp->undef_op & UNDEF_SETATTR))
		return (ENXIO);

	msg = fuse_alloc_in(fmp, sizeof(*fsi), 0, &fuse_sync_resp, msg_buff);
	fsi = (struct fuse_setattr_in *)msg->data;
	fsi->valid = 0;

	if (vap->va_uid != (uid_t)VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY) {
			error = EROFS;
			goto out;
		}
		fsi->uid = vap->va_uid;
		fsi->valid |= FATTR_UID;
	}

	if (vap->va_gid != (gid_t)VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY) {
			error = EROFS;
			goto out;
		}
		fsi->gid = vap->va_gid;
		fsi->valid |= FATTR_GID;
	}

	if (vap->va_size != VNOVAL) {
		switch (vp->v_type) {
		case VDIR:
			error = EISDIR;
			goto out;
		case VLNK:
		case VREG:
			if (vp->v_mount->mnt_flag & MNT_RDONLY) {
				error = EROFS;
				goto out;
			}
		default:
			break;
		}

		/*XXX to finish*/
	}

	if (vap->va_atime.tv_sec != VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY) {
			error = EROFS;
			goto out;
		}
		fsi->atime = vap->va_atime.tv_sec;
		fsi->atimensec = vap->va_atime.tv_nsec;
		fsi->valid |= FATTR_ATIME;
	}

	if (vap->va_mtime.tv_sec != VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY) {
			error = EROFS;
			goto out;
		}
		fsi->mtime = vap->va_mtime.tv_sec;
		fsi->mtimensec = vap->va_mtime.tv_nsec;
		fsi->valid |= FATTR_MTIME;
	}

	if (vap->va_mode != (mode_t)VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY) {
			error = EROFS;
			goto out;
		}
		fsi->mode = vap->va_mode & ALLPERMS;
		fsi->valid |= FATTR_MODE;
	}

	if (!fsi->valid) {
		goto out;
	}

	if (fsi->valid & FATTR_SIZE && vp->v_type == VDIR) {
		error = EISDIR;
		goto out;
	}

	fuse_make_in(fmp->mp, &msg->hdr, msg->len, FUSE_SETATTR,
	    ip->ufs_ino.i_number, p);

	error = fuse_send_in(fmp, msg);

	if (error) {
		if (error == ENOSYS)
			fmp->undef_op |= UNDEF_SETATTR;
		goto out;
	}

	fao = (struct fuse_attr_out *)msg->buff.data_rcv;
	fuse_internal_attr_fat2vat(fmp->mp, &fao->attr, vap);

out:
	fuse_clean_msg(msg);
	return (error);
}

int
fusefs_ioctl(void *v)
{
	DPRINTF("fusefs_ioctl\n");
	return (ENOTTY);
}

int
fusefs_link(void *v)
{
	struct vop_link_args *ap = v;
	struct vnode *dvp = ap->a_dvp;
	struct vnode *vp = ap->a_vp;
	struct componentname *cnp = ap->a_cnp;
	struct proc *p = cnp->cn_proc;
	struct fuse_entry_out *feo;
	struct fuse_link_in *fli;
	struct fuse_mnt *fmp;
	struct fuse_node *ip;
	struct fuse_node *dip;
	struct fuse_msg *msg;
	int error = 0;

	DPRINTF("fusefs_link\n");

	if (vp->v_type == VDIR) {
		VOP_ABORTOP(dvp, cnp);
		error = EISDIR;
		goto out2;
	}
	if (dvp->v_mount != vp->v_mount) {
		VOP_ABORTOP(dvp, cnp);
		error = EXDEV;
		goto out2;
	}
	if (dvp != vp && (error = vn_lock(vp, LK_EXCLUSIVE, p))) {
		VOP_ABORTOP(dvp, cnp);
		goto out2;
	}

	ip = VTOI(vp);
	dip = VTOI(dvp);
	fmp = (struct fuse_mnt *)ip->ufs_ino.i_ump;

	if (!fmp->sess_init || (fmp->undef_op & UNDEF_LINK))
		goto out1;

	msg = fuse_alloc_in(fmp, sizeof(*fli) + cnp->cn_namelen + 1,
	    sizeof(*feo), &fuse_sync_resp, msg_buff);

	fli = (struct fuse_link_in *)msg->data;
	fli->oldnodeid = ip->ufs_ino.i_number;
	memcpy((char *)msg->data + sizeof(*fli), cnp->cn_nameptr,
	    cnp->cn_namelen);
	((char *)msg->data)[sizeof(*fli) + cnp->cn_namelen] = '\0';

	fuse_make_in(fmp->mp, &msg->hdr, msg->len, FUSE_LINK,
	    dip->ufs_ino.i_number, p);
	error = fuse_send_in(fmp, msg);

	if (error) {
		if (error == ENOSYS)
			fmp->undef_op |= UNDEF_LINK;

		fuse_clean_msg(msg);
		goto out1;
	}

	/*feo = (struct fuse_entry_out *)msg->buff.data_rcv;*/
	fuse_clean_msg(msg);

out1:
	if (dvp != vp)
		VOP_UNLOCK(vp, 0, p);
out2:
	vput(dvp);
	return (error);
}

int
fusefs_symlink(void *v)
{
	DPRINTF("fusefs_symlink\n");
	return (0);
}

#define GENERIC_DIRSIZ(dp) \
    ((sizeof (struct dirent) - (MAXNAMLEN+1)) + (((dp)->d_namlen+1 + 3) &~ 3))


int
readdir_process_data(void *buff, int len, struct uio *uio)
{
	struct fuse_dirent *fdir;
	struct dirent dir;
	int bytes;
	int flen, error = 0;

	if (len < FUSE_NAME_OFFSET) {
		return (0);
	}

	while ( len > 0) {
		if ( len < sizeof(*fdir)) {
			error = 0;
			break;
		}

		fdir = (struct fuse_dirent *)buff;
		flen = FUSE_DIRENT_SIZE(fdir);

		if (!fdir->namelen || fdir->namelen > MAXNAMLEN || len < flen) {
			DPRINTF("fuse: readdir return EINVAL\n");
			error = EINVAL;
			break;
		}

		bytes = GENERIC_DIRSIZ((struct pseudo_dirent *) &fdir->namelen);
		if (bytes > uio->uio_resid) {
			printf("fuse: entry too big\n");
			error = 0;
			break;
		}

		bzero(&dir, sizeof(dir));
		dir.d_fileno = fdir->ino;
		dir.d_reclen = bytes;
		dir.d_type = fdir->type;
		dir.d_namlen = fdir->namelen;
		bcopy(fdir->name, dir.d_name, fdir->namelen);

		uiomove(&dir, bytes , uio);
		len -= flen;
		/* ugly pointer arithmetic must find something else ...*/
		buff = (void *)(((char *) buff) + flen);
		uio->uio_offset = fdir->off;
	}

	return (error);
}

int
fusefs_readdir(void *v)
{
	struct vop_readdir_args *ap = v;
	struct fuse_read_in *read;
	struct fuse_node *ip;
	struct fuse_mnt *fmp;
	struct fuse_msg *msg;
	struct vnode *vp;
	struct proc *p;
	struct uio *uio;
	int error = 0;

	vp = ap->a_vp;
	uio = ap->a_uio;
	p = uio->uio_procp;

	ip = VTOI(vp);
	fmp = (struct fuse_mnt *)ip->ufs_ino.i_ump;

	if (!fmp->sess_init) {
		return (0);
	}

	DPRINTF("fusefs_readdir\n");
	DPRINTF("uio resid 0x%x\n", uio->uio_resid);

	if (uio->uio_resid == 0)
		return (error);

	while (uio->uio_resid > 0) {
		msg = fuse_alloc_in(fmp, sizeof(*read), 0, &fuse_sync_resp,
		    msg_buff);

		if (ip->fufh[FUFH_RDONLY].fh_type == FUFH_INVALID) {
		      DPRINTF("dir not open\n");
		      /* TODO open the file */
		      fuse_clean_msg(msg);
		      return (error);
		}
		read = (struct fuse_read_in *)msg->data;
		read->fh = ip->fufh[FUFH_RDONLY].fh_id;
		read->offset = uio->uio_offset;
		read->size = MIN(uio->uio_resid, PAGE_SIZE);

		fuse_make_in(fmp->mp, &msg->hdr, msg->len, FUSE_READDIR,
		    ip->ufs_ino.i_number, p);

		error = fuse_send_in(fmp, msg);

		if (error) {
			fuse_clean_msg(msg);
			break;
		}

		/*ack end of readdir */
		if (msg->buff.len == 0) {
			fuse_clean_msg(msg);
			break;
		}

		if ((error = readdir_process_data(msg->buff.data_rcv,
		    msg->buff.len, uio))) {
			fuse_clean_msg(msg);
			break;
		}

		fuse_clean_msg(msg);
	}

	return (error);
}

int
fusefs_inactive(void *v)
{
	struct vop_inactive_args *ap = v;
	struct vnode *vp = ap->a_vp;
	struct proc *p = ap->a_p;
	struct fuse_node *ip = VTOI(vp);
	struct fuse_filehandle *fufh = NULL;
	struct fuse_mnt *fmp;
	int error = 0;
	int type;

	DPRINTF("fusefs_inactive\n");
	fmp = (struct fuse_mnt *)ip->ufs_ino.i_ump;

	for (type = 0; type < FUFH_MAXTYPE; type++) {
		fufh = &(ip->fufh[type]);
		if (fufh->fh_type != FUFH_INVALID)
			fuse_file_close(fmp, ip, fufh->fh_type, type,
			    (ip->vtype == VDIR), ap->a_p);
	}

	VOP_UNLOCK(vp, 0, p);

	/* not sure if it is ok to do like that ...*/
	if (ip->cached_attrs.va_mode == 0)
		vrecycle(vp, p);

	return (error);
}

int
fusefs_readlink(void *v)
{
	struct vop_readlink_args *ap = v;
	struct vnode *vp = ap->a_vp;
	struct fuse_node *ip;
	struct fuse_mnt *fmp;
	struct fuse_msg *msg;
	struct uio *uio;
	struct proc *p;
	int error = 0;

	DPRINTF("fusefs_readlink\n");

	ip = VTOI(vp);
	fmp = (struct fuse_mnt *)ip->ufs_ino.i_ump;
	uio = ap->a_uio;
	p = uio->uio_procp;

	if (!fmp->sess_init || (fmp->undef_op & UNDEF_READLINK)) {
		error = ENOSYS;
		goto out;
	}

	msg = fuse_alloc_in(fmp, 0, 0, &fuse_sync_resp, msg_buff);
	fuse_make_in(fmp->mp, &msg->hdr, msg->len, FUSE_READLINK,
	    ip->ufs_ino.i_number, p);

	error = fuse_send_in(fmp, msg);

	if (error) {
		if (error == ENOSYS)
			fmp->undef_op |= UNDEF_READLINK;

		fuse_clean_msg(msg);
		goto out;
	}

	error = uiomove(msg->buff.data_rcv, msg->buff.len, uio);
	fuse_clean_msg(msg);
out:
	return (error);
}

int
fusefs_reclaim(void *v)
{
	struct vop_reclaim_args *ap = v;
	struct vnode *vp = ap->a_vp;
	struct fuse_node *ip = VTOI(vp);
	struct fuse_filehandle *fufh = NULL;
	struct fuse_mnt *fmp;
	int type;

	DPRINTF("fusefs_reclaim\n");
	fmp = (struct fuse_mnt *)ip->ufs_ino.i_ump;

	/*close opened files*/
	for (type = 0; type < FUFH_MAXTYPE; type++) {
		fufh = &(ip->fufh[type]);
		if (fufh->fh_type != FUFH_INVALID) {
			printf("FUSE: vnode being reclaimed is valid");
			fuse_file_close(fmp, ip, fufh->fh_type, type,
			    (ip->vtype == VDIR), ap->a_p);
		}
	}
	/*
	 * Purge old data structures associated with the inode.
	 */
	ip->parent = 0;

	/*
	 * Remove the inode from its hash chain.
	 */
	ufs_ihashrem(&ip->ufs_ino);
	cache_purge(vp);

	/*close file if exist
	if (ip->ufs_ino.i_dev) {
		vrele(ip->i_devvp);
		ip->i_devvp = 0;
	}*/

	free(ip, M_FUSEFS);
	vp->v_data = NULL;
	return (0);
}

int
fusefs_print(void *v)
{
	struct vop_print_args *ap = v;
	struct vnode *vp = ap->a_vp;
	struct fuse_node *ip = VTOI(vp);

	/*
	 * Complete the information given by vprint().
	 */
	printf("tag VT_FUSE, hash id %u ", ip->ufs_ino.i_number);
	lockmgr_printinfo(&ip->ufs_ino.i_lock);
	printf("\n");
	return (0);
}

int
fusefs_create(void *v)
{
	struct vop_create_args *ap = v;
	struct componentname *cnp = ap->a_cnp;
	struct vnode **vpp = ap->a_vpp;
	struct vnode *dvp = ap->a_dvp;
	struct vattr *vap = ap->a_vap;
	struct proc *p = cnp->cn_proc;
	struct fuse_entry_out *feo;
	struct vnode *tdp = NULL;
	struct fuse_open_in *foi;
	struct fuse_mnt *fmp;
	struct fuse_node *ip;
	struct fuse_msg *msg;
	int error = 0;
	mode_t mode;

	DPRINTF("fusefs_create(cnp %08x, vap %08x\n", cnp, vap);

	ip = VTOI(dvp);
	fmp = (struct fuse_mnt *)ip->ufs_ino.i_ump;
	mode = MAKEIMODE(vap->va_type, vap->va_mode);

	if (!fmp->sess_init || (fmp->undef_op & UNDEF_CREATE)) {
		error = ENOSYS;
		goto out;
	}

	msg = fuse_alloc_in(fmp, sizeof(*foi) + cnp->cn_namelen + 1,
	    sizeof(struct fuse_entry_out) + sizeof(struct fuse_open_out),
	    &fuse_sync_resp, msg_buff);

	foi = (struct fuse_open_in *)msg->data;
	foi->mode = mode;
	foi->flags = O_CREAT | O_RDWR;

	memcpy((char *)msg->data + sizeof(*foi), cnp->cn_nameptr,
	    cnp->cn_namelen);
	((char *)msg->data)[sizeof(*foi) + cnp->cn_namelen] = '\0';

	fuse_make_in(fmp->mp, &msg->hdr, msg->len, FUSE_CREATE,
	    ip->ufs_ino.i_number, p);

	error = fuse_send_in(fmp, msg);
	if (error) {
		if (error == ENOSYS)
			fmp->undef_op |= UNDEF_CREATE;

		fuse_clean_msg(msg);
		goto out;
	}

	feo = msg->buff.data_rcv;
	if ((error = VFS_VGET(fmp->mp, feo->nodeid, &tdp))) {
		fuse_clean_msg(msg);
		goto out;
	}

	tdp->v_type = IFTOVT(feo->attr.mode);
	VTOI(tdp)->vtype = tdp->v_type;

	if (dvp != NULL && dvp->v_type == VDIR)
		VTOI(tdp)->parent = ip->ufs_ino.i_number;

	*vpp = tdp;
	VN_KNOTE(ap->a_dvp, NOTE_WRITE);
	fuse_clean_msg(msg);

out:
	vput(ap->a_dvp);
	return (error);
}

int
fusefs_mknod(void *v)
{
	struct vop_mknod_args *ap = v;

	VN_KNOTE(ap->a_dvp, NOTE_WRITE);
	vput(ap->a_dvp);
	return (EINVAL);
}

int
fusefs_read(void *v)
{
	struct vop_read_args *ap = v;
	struct vnode *vp = ap->a_vp;
	struct uio *uio = ap->a_uio;
	struct proc *p = uio->uio_procp;
	struct fuse_read_in *fri;
	struct fuse_node *ip;
	struct fuse_mnt *fmp;
	struct fuse_msg *msg = NULL;
	int error=0;

	DPRINTF("fusefs_read\n");

	ip = VTOI(vp);
	fmp = (struct fuse_mnt *)ip->ufs_ino.i_ump;

	DPRINTF("read inode=%i, offset=%llu, resid=%x\n",
	    ip->ufs_ino.i_number, uio->uio_offset, uio->uio_resid);

	if (uio->uio_resid == 0)
		return (error);
	if (uio->uio_offset < 0)
		return (EINVAL);

	while (uio->uio_resid > 0) {
		msg = fuse_alloc_in(fmp, sizeof(*fri), 0, &fuse_sync_resp,
		    msg_buff);

		fri = (struct fuse_read_in *)msg->data;
		fri->fh = fuse_fd_get(ip, FUFH_RDONLY);
		fri->offset = uio->uio_offset;
		fri->size = MIN(uio->uio_resid, fmp->max_write);

		fuse_make_in(fmp->mp, &msg->hdr, msg->len, FUSE_READ,
		    ip->ufs_ino.i_number, p);

		error = fuse_send_in(fmp, msg);

		if (error) {
			DPRINTF("read error %i\n", error);
			break;
		}

		if ((error = uiomove(msg->buff.data_rcv,
		    MIN(fri->size, msg->buff.len), uio)))
			break;

		if (msg->buff.len < fri->size)
			break;

		fuse_clean_msg(msg);
		msg = NULL;
	}

	if (msg)
		fuse_clean_msg(msg);

	return (error);
}

int
fusefs_write(void *v)
{
	struct vop_write_args *ap = v;
	struct vnode *vp = ap->a_vp;
	struct uio *uio = ap->a_uio;
	struct proc *p = uio->uio_procp;
	struct fuse_write_in *fwi;
	struct fuse_node *ip;
	struct fuse_mnt *fmp;
	struct fuse_msg *msg = NULL;
	size_t len, diff;
	int error=0;

	DPRINTF("fusefs_write\n");

	ip = VTOI(vp);
	fmp = (struct fuse_mnt *)ip->ufs_ino.i_ump;

	DPRINTF("write inode=%i, offset=%llu, resid=%x\n",
	    ip->ufs_ino.i_number, uio->uio_offset, uio->uio_resid);

	if (uio->uio_resid == 0)
		return (error);

	while (uio->uio_resid > 0) {
		len = MIN(uio->uio_resid, fmp->max_write);
		msg = fuse_alloc_in(fmp, sizeof(*fwi) + len,
		    sizeof(struct fuse_write_out), &fuse_sync_resp, msg_buff);

		fwi = (struct fuse_write_in *)msg->data;
		fwi->fh = fuse_fd_get(ip, FUFH_WRONLY);
		fwi->offset = uio->uio_offset;
		fwi->size = len;

		if ((error = uiomove((char *)msg->data + sizeof(*fwi), len,
		    uio))) {
			DPRINTF("uio error %i", error);
			break;
		}

		fuse_make_in(fmp->mp, &msg->hdr, msg->len, FUSE_WRITE,
		    ip->ufs_ino.i_number, p);

		error = fuse_send_in(fmp, msg);

		if (error) {
			DPRINTF("write error %i\n", error);
			break;
		}

		diff = len -
		    ((struct fuse_write_out *)msg->buff.data_rcv)->size;
		if (diff < 0) {
			error = EINVAL;
			break;
		}

		uio->uio_resid += diff;
		uio->uio_offset -= diff;

		fuse_clean_msg(msg);
		msg = NULL;
	}

	if (msg)
		fuse_clean_msg(msg);
	return (error);
}

int
fusefs_poll(void *v)
{
	struct vop_poll_args *ap = v;

	DPRINTF("fusefs_poll\n");

	/*
	 * We should really check to see if I/O is possible.
	 */
	return (ap->a_events & (POLLIN | POLLOUT | POLLRDNORM | POLLWRNORM));
}

int
fusefs_fsync(void *v)
{
	DPRINTF("fusefs_fsync\n");
	return (0);
}

int
fusefs_rename(void *v)
{
	DPRINTF("fusefs_rename\n");
	return (0);
}

int
fusefs_mkdir(void *v)
{
	struct vop_mkdir_args *ap = v;
	struct vnode *dvp = ap->a_dvp;
	struct vnode **vpp = ap->a_vpp;
	struct componentname *cnp = ap->a_cnp;
	struct vattr *vap = ap->a_vap;
	struct proc *p = cnp->cn_proc;
	struct fuse_mkdir_in *fmdi;
	struct vnode *tdp = NULL;
	struct fuse_entry_out *feo;
	struct fuse_node *ip;
	struct fuse_mnt *fmp;
	struct fuse_msg *msg;
	int error = 0;

	DPRINTF("fusefs_mkdir %s\n", cnp->cn_nameptr);

	ip = VTOI(dvp);
	fmp = (struct fuse_mnt *)ip->ufs_ino.i_ump;


	if (!fmp->sess_init || (fmp->undef_op & UNDEF_MKDIR)) {
		error = ENOSYS;
		goto out;
	}

	msg = fuse_alloc_in(fmp, sizeof(*fmdi) + cnp->cn_namelen + 1,
	    sizeof(*feo), &fuse_sync_resp, msg_buff);

	fmdi = (struct fuse_mkdir_in *)msg->data;
	fmdi->mode = MAKEIMODE(vap->va_type, vap->va_mode);
	memcpy((char *)msg->data + sizeof(*fmdi), cnp->cn_nameptr,
	    cnp->cn_namelen);
	((char *)msg->data)[sizeof(*fmdi) + cnp->cn_namelen] = '\0';

	fuse_make_in(fmp->mp, &msg->hdr, msg->len, FUSE_MKDIR,
	    ip->ufs_ino.i_number, p);

	error = fuse_send_in(fmp, msg);
	if (error) {
		if (error == ENOSYS) {
			fmp->undef_op |= UNDEF_MKDIR;
		}

		fuse_clean_msg(msg);
		goto out;
	}

	feo = (struct fuse_entry_out *)msg->buff.data_rcv;

	if ((error = VFS_VGET(fmp->mp, feo->nodeid, &tdp))) {
		fuse_clean_msg(msg);
		goto out;
	}

	tdp->v_type = IFTOVT(feo->attr.mode);
	VTOI(tdp)->vtype = tdp->v_type;

	if (dvp != NULL && dvp->v_type == VDIR)
		VTOI(tdp)->parent = ip->ufs_ino.i_number;

	*vpp = tdp;
	VN_KNOTE(ap->a_dvp, NOTE_WRITE | NOTE_LINK);
	fuse_clean_msg(msg);
out:
	vput(dvp);
	return (error);
}

int
fusefs_rmdir(void *v)
{
	struct vop_rmdir_args *ap = v;
	struct vnode *vp = ap->a_vp;
	struct vnode *dvp = ap->a_dvp;
	struct componentname *cnp = ap->a_cnp;
	struct proc *p = cnp->cn_proc;
	struct fuse_node *ip, *dp;
	struct fuse_mnt *fmp;
	struct fuse_msg *msg;
	int error;

	DPRINTF("fusefs_rmdir\n");

	ip = VTOI(vp);
	dp = VTOI(dvp);
	fmp = (struct fuse_mnt *)ip->ufs_ino.i_ump;

	/*
	 * No rmdir "." please.
	 */
	if (dp == ip) {
		vrele(dvp);
		vput(vp);
		return (EINVAL);
	}

	if (!fmp->sess_init || (fmp->undef_op & UNDEF_RMDIR)) {
		error = ENOSYS;
		goto out;
	}

	VN_KNOTE(dvp, NOTE_WRITE | NOTE_LINK);

	msg = fuse_alloc_in(fmp, cnp->cn_namelen + 1, 0, &fuse_sync_it,
	    msg_intr);
	memcpy(msg->data, cnp->cn_nameptr, cnp->cn_namelen);
	((char *)msg->data)[cnp->cn_namelen] = '\0';

	fuse_make_in(fmp->mp, &msg->hdr, msg->len, FUSE_RMDIR,
	    dp->ufs_ino.i_number, p);

	error = fuse_send_in(fmp, msg);

	if (error) {
		if (error == ENOSYS)
			fmp->undef_op |= UNDEF_RMDIR;
		if (error != ENOTEMPTY)
			VN_KNOTE(dvp, NOTE_WRITE | NOTE_LINK);

		fuse_clean_msg(msg);
		goto out;
	}

	VN_KNOTE(dvp, NOTE_WRITE | NOTE_LINK);

	cache_purge(dvp);
	vput(dvp);
	dvp = NULL;

	cache_purge(ITOV(ip));
	fuse_clean_msg(msg);
out:
	if (dvp)
		vput(dvp);
	VN_KNOTE(vp, NOTE_DELETE);
	vput(vp);
	return (error);
}

int
fusefs_remove(void *v)
{
	struct vop_remove_args *ap = v;
	struct vnode *vp = ap->a_vp;
	struct vnode *dvp = ap->a_dvp;
	struct componentname *cnp = ap->a_cnp;
	struct proc *p = cnp->cn_proc;
	struct fuse_node *ip;
	struct fuse_node *dp;
	struct fuse_mnt *fmp;
	struct fuse_msg *msg;
	int error = 0;

	DPRINTF("fusefs_remove\n");

	ip = VTOI(vp);
	dp = VTOI(dvp);
	fmp = (struct fuse_mnt *)ip->ufs_ino.i_ump;

	if (!fmp->sess_init || (fmp->undef_op & UNDEF_REMOVE)) {
		error = ENOSYS;
		goto out;
	}

	msg = fuse_alloc_in(fmp, cnp->cn_namelen + 1, 0, &fuse_sync_it,
	    msg_intr);
	memcpy(msg->data, cnp->cn_nameptr, cnp->cn_namelen);
	((char *)msg->data)[cnp->cn_namelen] = '\0';

	fuse_make_in(fmp->mp, &msg->hdr, msg->len, FUSE_UNLINK,
	    dp->ufs_ino.i_number, p);

	error = fuse_send_in(fmp, msg);
	if (error) {
		if (error == ENOSYS)
			fmp->undef_op |= UNDEF_REMOVE;

		fuse_clean_msg(msg);
		goto out;
	}

	VN_KNOTE(vp, NOTE_DELETE);
	VN_KNOTE(dvp, NOTE_WRITE);
	fuse_clean_msg(msg);
out:
	if (dvp == vp)
		vrele(vp);
	else
		vput(vp);
	vput(dvp);
	return (error);
}

int
fusefs_strategy(void *v)
{
	DPRINTF("fusefs_strategy\n");
	return (0);
}

int
fusefs_lock(void *v)
{
	struct vop_lock_args *ap = v;
	struct vnode *vp = ap->a_vp;

	DPRINTF("fuse_lock\n");
	return (lockmgr(&VTOI(vp)->ufs_ino.i_lock, ap->a_flags, NULL));
}

int
fusefs_unlock(void *v)
{
	struct vop_unlock_args *ap = v;
	struct vnode *vp = ap->a_vp;

	DPRINTF("fuse_unlock\n");
	return (lockmgr(&VTOI(vp)->ufs_ino.i_lock, ap->a_flags | LK_RELEASE,
	    NULL));
}

int
fusefs_islocked(void *v)
{
	struct vop_islocked_args *ap = v;

	DPRINTF("fuse_islock\n");
	return (lockstatus(&VTOI(ap->a_vp)->ufs_ino.i_lock));
}

int
fusefs_advlock(void *v)
{
	struct vop_advlock_args *ap = v;
	struct fuse_node *ip = VTOI(ap->a_vp);

	DPRINTF("fuse_advlock\n");
	return (lf_advlock(&ip->ufs_ino.i_lockf, ip->filesize, ap->a_id,
	    ap->a_op, ap->a_fl, ap->a_flags));
}
