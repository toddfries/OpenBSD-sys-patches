/*
 * Copyright (c) 2008,2009 Thordur I. Bjornsson <thib@openbsd.org>
 * Copyright (c) 2004 Ted Unangst <tedu@openbsd.org>
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
#ifndef _SYS_BUFQ_H_
#define _SYS_BUFQ_H_

#ifdef _KERNEL

#define BUFQ_DISKSORT	1
#define BUFQ_DEFAULT	BUFQ_DISKSORT

struct bufq {
	struct buf	*(*bufq_get)(struct bufq *, int);
	void		 (*bufq_add)(struct bufq *, struct buf *);
	void		  *bufq_data;
	int		   bufq_type;
};

TAILQ_HEAD(bufq_tailq, buf);

#define	BUFQ_ADD(_bufq, _bp)	(_bufq)->bufq_add(_bufq, _bp)
#define	BUFQ_GET(_bufq)		(_bufq)->bufq_get(_bufq, 0)
#define	BUFQ_PEEK(_bufq)	(_bufq)->bufq_get(_bufq, 1)

struct bufq	*bufq_init(int);
void		 bufq_destroy(struct bufq *);
void		 bufq_drain(struct bufq *);

#endif /* _KERNEL */

#endif /* !_SYS_BUFQ_H_ */
