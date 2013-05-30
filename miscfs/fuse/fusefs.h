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

#ifndef __FUSEFS_H__
#define __FUSEFS_H__

struct fuse_msg;

struct fuse_mnt {
	struct mount *mp;
	uint32_t undef_op;
	uint32_t max_write;
	int sess_init;
	int unique;
	dev_t dev;
};

#define	UNDEF_ACCESS	1<<0
#define	UNDEF_MKDIR	1<<1
#define	UNDEF_CREATE	1<<2
#define	UNDEF_LINK	1<<3
#define UNDEF_READLINK	1<<4
#define UNDEF_RMDIR	1<<5
#define UNDEF_REMOVE	1<<6
#define UNDEF_SETATTR	1<<7

typedef void (*fuse_cb)(struct fuse_msg *,struct fuse_out_header *, void *);

struct rcv_buf {
	void *data_rcv;
	size_t len;
};

enum msg_type {
	msg_intr,
	msg_buff,
	msg_buff_async,
};

struct fuse_msg {
	struct fuse_in_header hdr;
	struct fuse_mnt *fmp;

	void *data;
	int len;
	int error;
	int resid; /*residual io*/

	enum msg_type type;
	struct rcv_buf buff;

	fuse_cb cb;
	SIMPLEQ_ENTRY(fuse_msg) node;
};

extern struct vops fusefs_vops;

/*
 * In and Out fifo for fuse communication
 */
SIMPLEQ_HEAD(fuse_msg_head, fuse_msg);

extern struct fuse_msg_head fmq_in;
extern struct fuse_msg_head fmq_wait;

extern struct pool fusefs_msgin_pool;
extern struct pool fusefs_msgout_pool;

/*
 * fuse helpers
 */
#define TSLEEP_TIMEOUT 5
struct fuse_msg * fuse_alloc_in(struct fuse_mnt *, int, int, fuse_cb,
    enum msg_type);
void fuse_make_in(struct mount *, struct fuse_in_header *, int,
    enum fuse_opcode, ino_t, struct proc *);
int fuse_send_in(struct fuse_mnt *, struct fuse_msg *);
void fuse_clean_msg(struct fuse_msg *);
void fuse_init_resp(struct fuse_msg *, struct fuse_out_header *, void *);
void fuse_sync_resp(struct fuse_msg *, struct fuse_out_header *, void *);
void fuse_sync_it(struct fuse_msg *, struct fuse_out_header *, void *);

/*
 * files helpers.
 */

int fuse_file_open(struct fuse_mnt *, struct fuse_node *, enum fufh_type, int,
    int, struct proc *);
int fuse_file_close(struct fuse_mnt *, struct fuse_node *, enum fufh_type, int,
    int, struct proc *);
void fuse_internal_attr_fat2vat(struct mount *, struct fuse_attr *,
    struct vattr *);

/*
 * device helpers.
 */
void fuse_device_cleanup(dev_t dev);
void fuse_device_kqwakeup(dev_t dev);

/*
 * The root inode is the root of the file system.  Inode 0 can't be used for
 * normal purposes.
 */
#define	FUSE_ROOTINO ((ino_t)1)
#define VFSTOFUSEFS(mp)	((struct fuse_mnt *)((mp)->mnt_data))

/*#define FUSE_DEBUG_VNOP
#define FUSE_DEBUG_VFS
#define FUSE_DEV_DEBUG
#define FUSE_DEBUG_MSG*/

#endif /* __FUSEFS_H__ */
