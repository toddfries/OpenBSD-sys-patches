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

#ifndef __FUSE_NODE_H__
#define __FUSE_NODE_H__

enum fufh_type {
    FUFH_INVALID = -1,
    FUFH_RDONLY  = 0,
    FUFH_WRONLY  = 1,
    FUFH_RDWR    = 2,
    FUFH_MAXTYPE = 3,
};

struct fuse_filehandle {
    uint64_t fh_id;
    enum fufh_type fh_type;
};

struct fuse_node {
    LIST_ENTRY(fuse_node) i_hash;    /* Hash chain */
    struct vnode *i_vnode;        /* vnode associated with this inode */
    struct lockf *i_lockf;        /* Head of byte-level lock list. */
    struct lock i_lock;        /* node lock */
    ino_t i_number;            /* the identity of the inode */
    int i_fd;            /* fd of fuse session */

    struct fuse_mnt *i_mnt;        /* fs associated with this inode */
    uint64_t parent;

    /** I/O **/
    struct     fuse_filehandle fufh[FUFH_MAXTYPE];

    /** flags **/
    uint32_t   flag;

    /** meta **/
    struct vattr      cached_attrs;
    off_t             filesize;
    uint64_t          nlookup;
    enum vtype        vtype;
};

extern struct fuse_node **fusehashtbl;
extern u_long fusehash;

void fusefs_ihashinit(void);
struct vnode *fusefs_ihashget(int, ino_t);
int fusefs_ihashins(struct fuse_node *);
void fusefs_ihashrem(struct fuse_node *);
#define ITOV(ip) ((ip)->i_vnode)
#define VTOI(vp) ((struct fuse_node *)(vp)->v_data)

#endif /* __FUSE_NODE_H__ */
