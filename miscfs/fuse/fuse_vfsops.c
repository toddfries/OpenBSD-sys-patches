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
#include <sys/types.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/lock.h>

#include "fuse_kernel.h"
#include "fuse_node.h"
#include "fusefs.h"

#ifdef    FUSE_DEBUG_VFS
#define    DPRINTF(fmt, arg...)    printf("fuse vfsop: " fmt, ##arg)
#else
#define    DPRINTF(fmt, arg...)
#endif

int    fusefs_mount(struct mount *, const char *, void *, struct nameidata *,
        struct proc *);
int    fusefs_start(struct mount *, int, struct proc *);
int    fusefs_unmount(struct mount *, int, struct proc *);
int    fusefs_root(struct mount *, struct vnode **);
int    fusefs_quotactl(struct mount *, int, uid_t, caddr_t, struct proc *);
int    fusefs_statfs(struct mount *, struct statfs *, struct proc *);
int    fusefs_sync(struct mount *, int, struct ucred *, struct proc *);
int    fusefs_vget(struct mount *, ino_t, struct vnode **);
int    fusefs_fhtovp(struct mount *, struct fid *, struct vnode **);
int    fusefs_vptofh(struct vnode *, struct fid *);
int    fusefs_init(struct vfsconf *);
int    fusefs_sysctl(int *, u_int, void *, size_t *, void *, size_t,
        struct proc *);
int    fusefs_checkexp(struct mount *, struct mbuf *, int *,
        struct ucred **);

const struct vfsops fusefs_vfsops = {
    fusefs_mount,
    fusefs_start,
    fusefs_unmount,
    fusefs_root,
    fusefs_quotactl,
    fusefs_statfs,
    fusefs_sync,
    fusefs_vget,
    fusefs_fhtovp,
    fusefs_vptofh,
    fusefs_init,
    fusefs_sysctl,
    fusefs_checkexp
};

int
fusefs_mount(struct mount *mp, const char *path, void *data,
    struct nameidata *ndp, struct proc *p)
{
    struct fuse_mnt *fmp;
    struct fuse_msg *msg;
    struct fuse_init_in *init;
    struct fusefs_args args;
    int error;

    DPRINTF("mount\n");

    if (mp->mnt_flag & MNT_UPDATE)
        return (EOPNOTSUPP);

    error = copyin(data, &args, sizeof(struct fusefs_args));
    if (error)
        return (error);

    DPRINTF("fd = %d\n", args.fd);

    fmp = malloc(sizeof(*fmp), M_FUSEFS, M_WAITOK | M_ZERO);
    fmp->mp = mp;
    fmp->sess_init = 0;
    fmp->unique = 0;
    fmp->fd = args.fd;
    mp->mnt_data = (qaddr_t)fmp;

    mp->mnt_flag |= MNT_LOCAL;
    vfs_getnewfsid(mp);

    bzero(mp->mnt_stat.f_mntonname, MNAMELEN);
    strlcpy(mp->mnt_stat.f_mntonname, path, MNAMELEN);
    bzero(mp->mnt_stat.f_mntfromname, MNAMELEN);
    bcopy("fusefs", mp->mnt_stat.f_mntfromname, sizeof("fusefs"));

    msg = malloc(sizeof(struct fuse_msg), M_FUSEFS, M_WAITOK | M_ZERO);
    msg->hdr = malloc(sizeof(struct fuse_in_header), M_FUSEFS,
        M_WAITOK | M_ZERO);
    init = malloc(sizeof(struct fuse_init_in), M_FUSEFS, M_WAITOK | M_ZERO);

    init->major = FUSE_KERNEL_VERSION;
    init->minor = FUSE_KERNEL_MINOR_VERSION;
    init->max_readahead = 4096 * 16;
    init->flags = 0;
    msg->data = init;
    msg->len = sizeof(*init);
    msg->cb = &fuse_init_resp;
    msg->fmp = fmp;
    msg->type = msg_buff_async;

    fuse_make_in(mp, msg->hdr, msg->len, FUSE_INIT, 0, p);

    TAILQ_INSERT_TAIL(&fmq_in, msg, node);
    wakeup(&fmq_in);

    return (0);
}

int
fusefs_start(struct mount *mp, int flags, struct proc *p)
{
    DPRINTF("start\n");
    return (0);
}

int
fusefs_unmount(struct mount *mp, int mntflags, struct proc *p)
{
    struct fuse_in_header hdr;
    struct fuse_mnt *fmp;
    struct fuse_msg msg;
    extern int doforce;
    struct fuse_msg *m;
    int flags = 0;
    int error;

    fmp = VFSTOFUSEFS(mp);

    if (!fmp->sess_init)
        return (0);

    fmp->sess_init = 0;

    DPRINTF("unmount\n");

    bzero(&msg, sizeof(msg));
    msg.hdr = &hdr;
    msg.len = 0;
    msg.type = msg_intr;
    msg.data = NULL;
    msg.cb = &fuse_sync_it;

    fuse_make_in(fmp->mp, msg.hdr, msg.len, FUSE_DESTROY, 0, p);
    TAILQ_INSERT_TAIL(&fmq_in, &msg, node);
    wakeup(&fmq_in);

    error = tsleep(&msg, PWAIT, "fuse unmount", 0);

    if (error)
        return (error);

    error = msg.rep.it_res;

    if (error)
        DPRINTF("error from fuse\n");

    /* clear FIFO IN*/
    while ((m = TAILQ_FIRST(&fmq_in))) {
        DPRINTF("warning some msg where not processed....\n");
    }

    /* clear FIFO WAIT*/
    while ((m = TAILQ_FIRST(&fmq_wait))) {
        DPRINTF("warning some msg where not processed....\n");
    }

    if (mntflags & MNT_FORCE) {
        /* fusefs can never be rootfs so don't check for it */
        if (!doforce)
            return (EINVAL);
        flags |= FORCECLOSE;
    }

    if ((error = vflush(mp, 0, flags)))
        return (error);

    free(fmp, M_FUSEFS);

    return (error);
}

int
fusefs_root(struct mount *mp, struct vnode **vpp)
{
    struct vnode *nvp;
    struct fuse_node *ip;
    int error;

    DPRINTF("root\n");

    if ((error = VFS_VGET(mp, (ino_t)FUSE_ROOTINO, &nvp)) != 0)
        return (error);

    ip = VTOI(nvp);
    nvp->v_type = VDIR;
    ip->vtype = VDIR;

    *vpp = nvp;
    return (0);
}

int fusefs_quotactl(struct mount *mp, int cmds, uid_t uid, caddr_t arg,
    struct proc *p)
{
    DPRINTF("quotactl\n");
    return (0);
}

int fusefs_statfs(struct mount *mp, struct statfs *sbp, struct proc *p)
{
    struct fuse_statfs_out *stat;
    struct fuse_in_header hdr;
    struct fuse_mnt *fmp;
    struct fuse_msg msg;
    int error;

    DPRINTF("statfs\n");

    fmp = VFSTOFUSEFS(mp);

    if (fmp->sess_init) {
        bzero(&msg, sizeof(msg));
        msg.hdr = &hdr;
        msg.len = 0;
        msg.data = NULL;
        msg.rep.buff.data_rcv = NULL;
        msg.rep.buff.len = sizeof(*stat);
        msg.cb = &fuse_sync_resp;
        msg.type = msg_buff;

        fuse_make_in(mp, msg.hdr, msg.len, FUSE_STATFS, FUSE_ROOT_ID,
            NULL);

        TAILQ_INSERT_TAIL(&fmq_in, &msg, node);
        wakeup(&fmq_in);

        error = tsleep(&msg, PWAIT, "fuse stat", 0);
        if (error) {
            if (msg.rep.buff.data_rcv)
                free(msg.rep.buff.data_rcv, M_FUSEFS);
            return (error);
        }

        stat = (struct fuse_statfs_out *)msg.rep.buff.data_rcv;

        DPRINTF("statfs a: %i\n", stat->st.bavail);
        DPRINTF("statfs a: %i\n", stat->st.bfree);
        DPRINTF("statfs a: %i\n", stat->st.blocks);
        DPRINTF("statfs a: %i\n", stat->st.bsize);
        DPRINTF("statfs a: %i\n", stat->st.ffree);
        DPRINTF("statfs a: %i\n", stat->st.files);
        DPRINTF("statfs a: %i\n", stat->st.frsize);
        DPRINTF("statfs a: %i\n", stat->st.namelen);
        DPRINTF("statfs a: %i\n", stat->st.padding);

        sbp->f_bavail = stat->st.bavail;
        sbp->f_bfree = stat->st.bfree;
        sbp->f_blocks = stat->st.blocks;
        sbp->f_ffree = stat->st.ffree;
        sbp->f_files = stat->st.files;
        sbp->f_bsize = stat->st.frsize;
        sbp->f_namemax = stat->st.namelen;

        free(stat, M_FUSEFS);
    } else {
        sbp->f_bavail = 0;
        sbp->f_bfree = 0;
        sbp->f_blocks = 0;
        sbp->f_ffree = 0;
        sbp->f_files = 0;
        sbp->f_bsize = 0;
        sbp->f_namemax = 0;
    }


    return (0);
}

int fusefs_sync(struct mount *mp, int waitfor, struct ucred *cred,
    struct proc *p)
{
    DPRINTF("sync\n");
    return (0);
}

int fusefs_vget(struct mount *mp, ino_t ino, struct vnode **vpp)
{
    struct fuse_mnt *fmp;
    struct fuse_node *ip;
    struct vnode *nvp;
    int i;
    int error;

    DPRINTF("vget\n");

retry:
    fmp = VFSTOFUSEFS(mp);
    /*
     * check if vnode is in hash.
     */
    if ((*vpp = fusefs_ihashget(fmp->fd, ino)) != NULLVP)
        return (0);

    /*
     * if not create it
     */
    if ((error = getnewvnode(VT_FUSEFS, mp, &fusefs_vops, &nvp)) != 0) {
        DPRINTF("getnewvnode error\n");
        *vpp = NULLVP;
        return (error);
    }

    ip = malloc(sizeof(*ip), M_FUSEFS, M_WAITOK | M_ZERO);
    lockinit(&ip->i_lock, PINOD, "fuseinode", 0, 0);
    nvp->v_data = ip;
    ip->i_vnode = nvp;
    ip->i_fd = fmp->fd;
    ip->i_number = ino;
    ip->parent = 0;

    for (i = 0; i < FUFH_MAXTYPE; i++)
        ip->fufh[i].fh_type = FUFH_INVALID;

    error = fusefs_ihashins(ip);
    if (error) {
        vrele(nvp);

        if (error == EEXIST)
            goto retry;

        return (error);
    }

    ip->i_mnt = fmp;

    if (ino == FUSE_ROOTINO)
        nvp->v_flag |= VROOT;

    *vpp = nvp;

    return (0);
}

int fusefs_fhtovp(struct mount *mp, struct fid *fhp, struct vnode **vpp)
{
    DPRINTF("fhtovp\n");
    return (0);
}

int fusefs_vptofh(struct vnode *vp, struct fid *fhp)
{
    DPRINTF("vptofh\n");
    return (0);
}

int fusefs_init(struct vfsconf *vfc)
{
    DPRINTF("init\n");

    TAILQ_INIT(&fmq_in);
    TAILQ_INIT(&fmq_wait);

    fusefs_ihashinit();
    return (0);
}

int fusefs_sysctl(int *name, u_int namelen, void *oldp, size_t *oldplen,
    void *newp, size_t newlen, struct proc *p)
{
    DPRINTF("sysctl\n");
    return (0);
}

int fusefs_checkexp(struct mount *mp, struct mbuf *nam, int *extflagsp,
    struct ucred **credanonp)
{
    DPRINTF("checkexp\n");
    return (0);
}
