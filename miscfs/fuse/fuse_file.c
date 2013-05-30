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

#include "fuse_kernel.h"
#include "fuse_node.h"
#include "fusefs.h"

#ifdef	FUSE_DEV_DEBUG
#define	DPRINTF(fmt, arg...)	printf("fuse vnop: " fmt, ##arg)
#else
#define	DPRINTF(fmt, arg...)
#endif

int
fuse_file_open(struct fuse_mnt *fmp, struct fuse_node *ip,
    enum fufh_type fufh_type, int flags, int isdir, struct proc *p)
{
	struct fuse_open_out *open_out;
	struct fuse_open_in *open_in;
	struct fuse_msg *msg;
	int error = 0;

	msg = fuse_alloc_in(fmp, sizeof(*open_in), sizeof(*open_out),
	    &fuse_sync_resp, msg_buff);

	open_in = (struct fuse_open_in *)msg->data;
	open_in->flags = flags;
	open_in->mode = 0;

	fuse_make_in(fmp->mp, &msg->hdr, msg->len,
	    ((isdir) ? FUSE_OPENDIR : FUSE_OPEN), ip->ufs_ino.i_number, p);

	error = fuse_send_in(fmp, msg);
	if (error) {
		fuse_clean_msg(msg);
		return (error);
	}

	open_out = (struct fuse_open_out *)msg->buff.data_rcv;

	ip->fufh[fufh_type].fh_id = open_out->fh;
	ip->fufh[fufh_type].fh_type = fufh_type;

	fuse_clean_msg(msg);
	return (0);
}

int
fuse_file_close(struct fuse_mnt *fmp, struct fuse_node * ip,
    enum fufh_type  fufh_type, int flags, int isdir, struct proc *p)
{
	struct fuse_release_in *rel;
	struct fuse_msg *msg;
	int error = 0;

	msg = fuse_alloc_in(fmp, sizeof(*rel), 0, &fuse_sync_it, msg_intr);

	rel = (struct fuse_release_in *)msg->data;
	rel->fh  = ip->fufh[fufh_type].fh_id;
	rel->flags = flags;

	fuse_make_in(fmp->mp, &msg->hdr, msg->len,
	    ((isdir) ? FUSE_RELEASEDIR : FUSE_RELEASE), ip->ufs_ino.i_number,
	    p);

	error = fuse_send_in(fmp, msg);

	if (error)
		printf("fuse file error %d\n", error);

	ip->fufh[fufh_type].fh_id = (uint64_t)-1;
	ip->fufh[fufh_type].fh_type = FUFH_INVALID;

	fuse_clean_msg(msg);
	return (error);
}

uint64_t
fuse_fd_get(struct fuse_node *ip, enum fufh_type type)
{
	if (ip->fufh[type].fh_type == FUFH_INVALID) {
		type = FUFH_RDWR;
	}

	return (ip->fufh[type].fh_id);
}
