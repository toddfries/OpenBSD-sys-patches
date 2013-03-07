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

#include "fuse_kernel.h"
#include "fuse_node.h"
#include "fusefs.h"

#ifdef    FUSE_DEBUG_VNOP
#define    DPRINTF(fmt, arg...)    printf("fuse vnop: " fmt, ##arg)
#else
#define    DPRINTF(fmt, arg...)
#endif

extern int fusefs_lookup(void *);

int
fusefs_lookup(void *v)
{
    struct vop_lookup_args *ap = v;
    struct vnode *vdp;    /* vnode for directory being searched */
    struct fuse_node *dp;    /* inode for directory being searched */
    struct fuse_mnt *fmp;    /* file system that directory is in */
    int lockparent;        /* 1 => lockparent flag is set */
    struct vnode *tdp;    /* returned by VOP_VGET */
    struct fuse_in_header hdr;
    struct fuse_msg msg;
    struct vnode **vpp = ap->a_vpp;
    struct componentname *cnp = ap->a_cnp;
    struct proc *p = cnp->cn_proc;
    struct ucred *cred = cnp->cn_cred;
    struct fuse_entry_out *feo = NULL;
    int flags;
    int nameiop = cnp->cn_nameiop;
    /*struct proc *p = cnp->cn_proc;*/
    int error = 0;
    uint64_t nid;

    flags = cnp->cn_flags;
    *vpp = NULL;
    vdp = ap->a_dvp;
    dp = VTOI(vdp);
    fmp = dp->i_mnt;
    lockparent = flags & LOCKPARENT;

    DPRINTF("lookup path %s\n", cnp->cn_pnbuf);
    DPRINTF("lookup file %s\n", cnp->cn_nameptr);

    if ((error = VOP_ACCESS(vdp, VEXEC, cred, cnp->cn_proc)) != 0)
        return (error);

    if ((flags & ISLASTCN) && (vdp->v_mount->mnt_flag & MNT_RDONLY) &&
        (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME))
        return (EROFS);

    if ((error = cache_lookup(vdp, vpp, cnp)) >= 0)
        return (error);

    if (flags & ISDOTDOT) {
        /* got ".." */
        nid = dp->parent;
        if (nid == 0) {
            return (ENOENT);
        }
    } else if (cnp->cn_namelen == 1 && *(cnp->cn_nameptr) == '.') {
        /* got "." */
        nid = dp->i_number;
    } else {
        /* got a real entry */
        bzero(&msg, sizeof(msg));
        msg.hdr = &hdr;
        msg.len = cnp->cn_namelen + 1;

        msg.data = malloc(msg.len, M_FUSEFS, M_WAITOK | M_ZERO);
        memcpy(msg.data, cnp->cn_nameptr, cnp->cn_namelen);
        ((char *)msg.data)[cnp->cn_namelen] = '\0';

        msg.type = msg_buff;
        msg.rep.buff.len = 0;
        msg.rep.buff.data_rcv = NULL;
        msg.cb = &fuse_sync_resp;

        fuse_make_in(fmp->mp, msg.hdr, msg.len, FUSE_LOOKUP,
            dp->i_number, p);

        TAILQ_INSERT_TAIL(&fmq_in, &msg, node);
        wakeup(&fmq_in);

        error = tsleep(&msg, PWAIT, "fuse lookup", 0);

        if (error)
            return (error);

        if (msg.error) {
            if ((nameiop == CREATE || nameiop == RENAME) &&
                (flags & ISLASTCN) ) {
                if (vdp->v_mount->mnt_flag & MNT_RDONLY)
                    return (EROFS);

                cnp->cn_flags |= SAVENAME;

                if (!lockparent) {
                    VOP_UNLOCK(vdp, 0, p);
                    cnp->cn_flags |= PDIRUNLOCK;
                }

                error = EJUSTRETURN;
                goto out;
            }

            error = ENOENT;
            goto out;
        }

        feo = (struct fuse_entry_out *)msg.rep.buff.data_rcv;
        nid = feo->nodeid;
    }

    if (nameiop == DELETE && (flags & ISLASTCN)) {
        /*
         * Write access to directory required to delete files.
         */
        if ((error = VOP_ACCESS(vdp, VWRITE, cred, cnp->cn_proc)) != 0)
            return (error);

        cnp->cn_flags |= SAVENAME;
    }

    if (flags & ISDOTDOT) {
        DPRINTF("lookup for ..\n");
        VOP_UNLOCK(vdp, 0, p);    /* race to get the inode */
        cnp->cn_flags |= PDIRUNLOCK;

        error = VFS_VGET(fmp->mp, nid, &tdp);

        if (error) {
            if (vn_lock(vdp, LK_EXCLUSIVE | LK_RETRY, p) == 0)
                cnp->cn_flags &= ~PDIRUNLOCK;

            return (error);
        }

        if (lockparent && (flags & ISLASTCN)) {
            if ((error = vn_lock(vdp, LK_EXCLUSIVE, p))) {
                vput(tdp);
                return (error);
            }
            cnp->cn_flags &= ~PDIRUNLOCK;
        }
        *vpp = tdp;

    } else if (nid == dp->i_number) {
        vref(vdp);
        *vpp = vdp;
        error = 0;
    } else {
        error = VFS_VGET(fmp->mp, nid, &tdp);

        if (!error) {
            tdp->v_type = IFTOVT(feo->attr.mode);
            VTOI(tdp)->vtype = tdp->v_type;
        }

        fuse_internal_attr_fat2vat(fmp->mp, &feo->attr,
            &(VTOI(tdp)->cached_attrs));
        free(feo, M_FUSEFS);

        if (error)
            return (error);

        if (vdp != NULL && vdp->v_type == VDIR) {
            VTOI(tdp)->parent = dp->i_number;
        }
        if (!lockparent || !(flags & ISLASTCN)) {
            VOP_UNLOCK(vdp, 0, p);
            cnp->cn_flags |= PDIRUNLOCK;
        }

        *vpp = tdp;
    }

out:
    if ((cnp->cn_flags & MAKEENTRY) && nameiop != CREATE &&
        nameiop != DELETE )
        cache_enter(vdp, *vpp, cnp);

    return (error);
}
