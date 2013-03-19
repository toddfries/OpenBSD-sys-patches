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
#include <sys/malloc.h>
#include <sys/vnode.h>

#include "fuse_kernel.h"
#include "fuse_node.h"
#include "fusefs.h"

void
fuse_make_in(struct mount *mp, struct fuse_in_header *hdr, int len, 
	     enum fuse_opcode op, ino_t ino, struct proc *p)
{
	struct fuse_mnt *fmp;
	
	fmp = VFSTOFUSEFS(mp);
	
	fmp->unique++;

	hdr->len = sizeof(*hdr) + len;
	hdr->opcode = op;
	hdr->nodeid = ino;
	hdr->unique = fmp->unique;
#ifdef FUSE_DEBUG_MSG
	printf("creat unique %i\n", hdr->unique);
#endif
	
	if (!p) {
	  hdr->pid = curproc->p_pid;
	  hdr->uid = 0;
	  hdr->gid = 0;
	} else {
	  hdr->pid = p->p_pid;
	  hdr->uid = p->p_cred->p_ruid;
	  hdr->gid = p->p_cred->p_rgid;
	}
}

void
fuse_init_resp(struct fuse_msg *msg, struct fuse_out_header *hdr, void *data)
{
	struct fuse_init_out *out = data;

#ifdef FUSE_DEBUG_MSG
	printf("async init unique %i\n", msg->hdr->unique);
	printf("init_out flags %i\n", out->flags);
	printf("init_out major %i\n", out->major);
	printf("init_out minor %i\n", out->minor);
	printf("init_out max_readahead %i\n", out->max_readahead);
	printf("init_out max_write %i\n", out->max_write);
	printf("init_out unused %i\n", out->unused);
#endif

	msg->fmp->sess_init = 1;
	msg->fmp->max_write = out->max_readahead;
}

void
fuse_sync_resp(struct fuse_msg *msg, struct fuse_out_header *hdr, void *data)
{
	size_t len;

#ifdef FUSE_DEBUG_MSG
	printf("buf unique %i\n", msg->hdr->unique);
#endif

	if (msg->type != msg_buff)
		printf("bad msg type\n");

	if (data != NULL && msg->rep.buff.len != 0) {
		len = hdr->len - sizeof(*hdr);
		if (msg->rep.buff.len != len) {
			printf("fusefs: packet size error on opcode %i\n", msg->hdr->opcode);
		}

		if (msg->rep.buff.len > len)
			printf("buff unused byte : 0x%x\n", msg->rep.buff.len - len);

		msg->rep.buff.data_rcv = malloc(msg->rep.buff.len,  M_FUSEFS, M_WAITOK | M_ZERO);
		memcpy(msg->rep.buff.data_rcv, data, msg->rep.buff.len);

		wakeup(msg);
		
	} else if (data != NULL) {
		len = hdr->len - sizeof(*hdr);
		msg->rep.buff.data_rcv = malloc(len,  M_FUSEFS, M_WAITOK | M_ZERO);
		memcpy(msg->rep.buff.data_rcv, data, len);
		msg->rep.buff.len = len;

		wakeup(msg);
	} else if (hdr->error) {
		msg->error = hdr->error;
#ifdef FUSE_DEBUG_MSG
		printf("error %i\n", msg->error);
#endif
		wakeup(msg);
	} else {
		msg->error = -1;
#ifdef FUSE_DEBUG_MSG
		printf("ack for msg\n");
#endif
		wakeup(msg);
	}
}

void
fuse_sync_it(struct fuse_msg *msg, struct fuse_out_header *hdr, void *data)
{
#ifdef FUSE_DEBUG_MSG
	printf("it unique %i\n", msg->hdr->unique);
#endif

	if (msg->type != msg_intr)
		printf("bad msg type\n");

	if (data != NULL)
		printf("normally data should be Null\n");

	msg->rep.it_res = hdr->error;
#ifdef FUSE_DEBUG_MSG
	printf("errno = %d\n");
#endif

	wakeup(msg);
}
