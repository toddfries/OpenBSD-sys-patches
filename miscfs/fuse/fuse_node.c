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
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/malloc.h>

#include "fuse_kernel.h"
#include "fuse_node.h"
#include "fusefs.h"

LIST_HEAD(ihashhead, fuse_node) *fhashtbl;
u_long    fhash;    /* size of hash table - 1 */
#define    INOHASH(fd, inum)    (&fhashtbl[((fd) + (inum)) & fhash])

void
fusefs_ihashinit(void)
{
    fhashtbl = hashinit(desiredvnodes, M_FUSEFS, M_WAITOK, &fhash);
}

/*
 * Use the fd/inum pair to find the incore inode, and return a pointer
 * to it. If it is in core, but locked, wait for it.
 */
struct vnode *
fusefs_ihashget(int fd, ino_t inum)
{
    struct proc *p = curproc;
    struct fuse_node *ip;
    struct vnode *vp;
loop:
    /* XXXLOCKING lock hash list */
    LIST_FOREACH(ip, INOHASH(fd, inum), i_hash) {
        if (inum == ip->i_number && fd == ip->i_fd) {
            vp = ITOV(ip);
            /* XXXLOCKING unlock hash list? */
            if (vget(vp, LK_EXCLUSIVE, p))
                goto loop;
            return (vp);
        }
    }
    /* XXXLOCKING unlock hash list? */
    return (NULL);
}

int
fusefs_ihashins(struct fuse_node *ip)
{
    struct fuse_node *curip;
    struct ihashhead *ipp;
    int fd = ip->i_fd;
    ino_t inum = ip->i_number;

    /* lock the inode, then put it on the appropriate hash list */
    lockmgr(&ip->i_lock, LK_EXCLUSIVE, NULL);

    /* XXXLOCKING lock hash list */

    LIST_FOREACH(curip, INOHASH(fd, inum), i_hash) {
        if (inum == curip->i_number && fd == curip->i_fd) {
            /* XXXLOCKING unlock hash list? */
            lockmgr(&ip->i_lock, LK_RELEASE, NULL);
            return (EEXIST);
        }
    }

    ipp = INOHASH(fd, inum);
    LIST_INSERT_HEAD(ipp, ip, i_hash);
    /* XXXLOCKING unlock hash list? */

    return (0);
}

/*
 * Remove the inode from the hash table.
 */
void
fusefs_ihashrem(struct fuse_node *ip)
{
    /* XXXLOCKING lock hash list */

    if (ip->i_hash.le_prev == NULL)
        return;

    LIST_REMOVE(ip, i_hash);
#ifdef DIAGNOSTIC
    ip->i_hash.le_next = NULL;
    ip->i_hash.le_prev = NULL;
#endif
    /* XXXLOCKING unlock hash list? */
}
