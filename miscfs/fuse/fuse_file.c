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

int
fuse_file_open(struct fuse_mnt *fmp, struct fuse_node *ip,
	       enum fufh_type fufh_type, int flags, int isdir)
{
	struct fuse_open_out *open_out;
	struct fuse_open_in open_in;
	struct fuse_in_header hdr;
	struct fuse_msg msg;
	int error;

	bzero(&msg, sizeof(msg));
	msg.hdr = &hdr;
	msg.data = &open_in;
	msg.len = sizeof(open_in);
	msg.cb = &fuse_sync_resp;
	msg.fmp = fmp;
	msg.type = msg_buff;
	
	open_in.flags = flags;
	open_in.mode = 0;
	msg.rep.buff.data_rcv = NULL;
	msg.rep.buff.len = sizeof(*open_out);

	fuse_make_in(fmp->mp, msg.hdr, msg.len, ((isdir)?FUSE_OPENDIR:FUSE_OPEN), 
		     ip->i_number, curproc);

	TAILQ_INSERT_TAIL(&fmq_in, &msg, node);
	wakeup(&fmq_in);

	error = tsleep(&msg, PWAIT, "fuse open", 0);

	if (error)
		return error;

	open_out = (struct fuse_open_out *)msg.rep.buff.data_rcv;

	ip->fufh[fufh_type].fh_id = open_out->fh;
	ip->fufh[fufh_type].fh_type = fufh_type;

	free(open_out, M_FUSEFS);

	return 0;
}

int
fuse_file_close(struct fuse_mnt *fmp, struct fuse_node * ip,
		enum fufh_type  fufh_type, int flags, int isdir)
{
	struct fuse_release_in rel;
	struct fuse_in_header hdr;
	struct fuse_msg msg;
	int error;

	bzero(&msg, sizeof(msg));
	bzero(&rel, sizeof(rel));
	msg.hdr = &hdr;
	msg.data = &rel;
	msg.len = sizeof(rel);
	msg.cb = &fuse_sync_it;
	msg.fmp = fmp;
	msg.type = msg_intr;

	rel.fh  = ip->fufh[fufh_type].fh_id;
	rel.flags = flags;

	fuse_make_in(fmp->mp, msg.hdr, msg.len, ((isdir)?FUSE_RELEASEDIR:FUSE_RELEASE),
		     ip->i_number, curproc);

	TAILQ_INSERT_TAIL(&fmq_in, &msg, node);
	wakeup(&fmq_in);

	error = tsleep(&msg, PWAIT, "fuse close", 0);

	if (error)
		return error;

	error = msg.rep.it_res;
	if (error)
		printf("error %d\n", error);

	ip->fufh[fufh_type].fh_id = (uint64_t)-1;
	ip->fufh[fufh_type].fh_type = FUFH_INVALID;

	return (error);
}
