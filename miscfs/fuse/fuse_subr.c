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
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/pool.h>

#include "fuse_kernel.h"
#include "fuse_node.h"
#include "fusefs.h"

#ifdef	FUSE_DEBUG_MSG
#define	DPRINTF(fmt, arg...)	printf("fuse ipc: " fmt, ##arg)
#else
#define	DPRINTF(fmt, arg...)
#endif

struct fuse_msg *
fuse_alloc_in(struct fuse_mnt *fmp, int data_in_len, int data_out_len,
    fuse_cb cb, enum msg_type type)
{
	struct fuse_msg *msg;

	msg = pool_get(&fusefs_msgin_pool, PR_WAITOK | PR_ZERO);
	bzero(msg, sizeof(*msg));

	if (data_in_len == 0)
		msg->data = 0;
	else
		msg->data = malloc(data_in_len, M_FUSEFS, M_WAITOK | M_ZERO);
	msg->len = data_in_len;
	msg->cb = cb;
	msg->fmp = fmp;
	msg->type = type;

	if (type != msg_intr) {
		msg->buff.len = data_out_len;
		msg->buff.data_rcv = NULL;
	}

	return (msg);
}

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

	DPRINTF("create unique %i\n", hdr->unique);

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

int
fuse_send_in(struct fuse_mnt *fmp, struct fuse_msg *msg)
{
	int error = 0;
	struct fuse_msg *m;

	SIMPLEQ_INSERT_TAIL(&fmq_in, msg, node);
	wakeup(&fmq_in);
	fuse_device_kqwakeup(fmp->dev);

	if ((error = tsleep(msg, PWAIT, "fuse msg", TSLEEP_TIMEOUT * hz))) {
		/* look for msg inFIFO IN*/
		SIMPLEQ_FOREACH(m, &fmq_in, node) {
			if (m == msg) {
				DPRINTF("remove unread msg\n");
				SIMPLEQ_REMOVE_HEAD(&fmq_in, node);
				break;
			}
		}
		SIMPLEQ_FOREACH(m, &fmq_wait, node) {
			if (m == msg) {
				DPRINTF("remove msg with no response\n");
				SIMPLEQ_REMOVE_HEAD(&fmq_wait, node);
				break;
			}
		}
		return (error);
	}

	if (msg->error) {
		error = msg->error;
	}

	return (error);
}

void
fuse_clean_msg(struct fuse_msg *msg)
{
	if (msg->data) {
		free(msg->data, M_FUSEFS);
		msg->data = NULL;
	}

	if (msg->type != msg_intr && msg->buff.data_rcv) {
		free(msg->buff.data_rcv, M_FUSEFS);
		msg->buff.data_rcv = NULL;
	}

	pool_put(&fusefs_msgin_pool, msg);
}

void
fuse_init_resp(struct fuse_msg *msg, struct fuse_out_header *hdr, void *data)
{
	struct fuse_init_out *out = data;

	msg->fmp->sess_init = 1;
	msg->fmp->max_write = out->max_readahead;
}

void
fuse_sync_resp(struct fuse_msg *msg, struct fuse_out_header *hdr, void *data)
{
	size_t len;

	DPRINTF("unique %i\n", msg->hdr.unique);

	if (msg->type != msg_buff)
		DPRINTF("bad msg type\n");

	if (data != NULL && msg->buff.len != 0) {
		len = hdr->len - sizeof(*hdr);
		if (msg->buff.len != len) {
			printf("fusefs: packet size error on opcode %i\n",
			    msg->hdr.opcode);
		}

		if (msg->buff.len > len)
			printf("unused byte : 0x%x\n", msg->buff.len - len);

		msg->buff.data_rcv = malloc(msg->buff.len,  M_FUSEFS,
		    M_WAITOK | M_ZERO);
		memcpy(msg->buff.data_rcv, data, msg->buff.len);

		wakeup(msg);

	} else if (data != NULL) {
		len = hdr->len - sizeof(*hdr);
		msg->buff.data_rcv = malloc(len,  M_FUSEFS,
		    M_WAITOK | M_ZERO);
		memcpy(msg->buff.data_rcv, data, len);
		msg->buff.len = len;

		wakeup(msg);
	} else if (hdr->error) {
		msg->error = hdr->error;
		DPRINTF("error %i\n", msg->error);
		wakeup(msg);
	} else {
		msg->error = 0;
		DPRINTF("ack for msg\n");
		wakeup(msg);
	}
}

void
fuse_sync_it(struct fuse_msg *msg, struct fuse_out_header *hdr, void *data)
{
	DPRINTF("unique %i\n", msg->hdr.unique);

	if (msg->type != msg_intr)
		printf("bad msg type\n");

	if (data != NULL)
		printf("normally data should be Null\n");

	msg->error = hdr->error;
	DPRINTF("errno = %d\n", msg->error);
	wakeup(msg);
}
