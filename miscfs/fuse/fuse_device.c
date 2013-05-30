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
#include <sys/proc.h>
#include <sys/poll.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/pool.h>

#include "fuse_kernel.h"
#include "fuse_node.h"
#include "fusefs.h"

#define	FUSEUNIT(dev)	(minor(dev))
#define DEVNAME(_s)	((_s)->sc_dev.dv_xname)

#ifdef	FUSE_DEV_DEBUG
#define	DPRINTF(fmt, arg...)	printf("%s: " fmt, DEVNAME(sc), ##arg)
#else
#define	DPRINTF(fmt, arg...)
#endif

struct fuse_softc {
	struct device	sc_dev;
	struct fuse_mnt	*sc_fmp;

	/* kq fields */
	struct selinfo	sc_rsel;

	int		sc_opened;
};

#define FUSE_OPEN 1
#define FUSE_CLOSE 0
#define FUSE_DONE 2

struct fuse_softc *fuse_softc;
int numfuse = 0;

void	fuseattach(int);
int	fuseopen(dev_t, int, int, struct proc *);
int	fuseclose(dev_t, int, int, struct proc *);
int	fuseioctl(dev_t, u_long, caddr_t, int, struct proc *);
int	fuseread(dev_t, struct uio *, int);
int	fusewrite(dev_t, struct uio *, int);
int	fusepoll(dev_t, int, struct proc *);
int	fusekqfilter(dev_t dev, struct knote *kn);
int	filt_fuse_read(struct knote *, long);
void	filt_fuse_rdetach(struct knote *);

struct filterops fuse_rd_filtops = {
	1,
	NULL,
	filt_fuse_rdetach,
	filt_fuse_read
};

struct filterops fuse_seltrue_filtops = {
	1,
	NULL,
	filt_fuse_rdetach,
	filt_seltrue
};

struct fuse_msg_head fmq_in;
struct fuse_msg_head fmq_wait;

#ifdef	FUSE_DEV_DEBUG
static void
dump_buff(char *buff, int len)
{
	char text[17];
	int i;

	bzero(text, 17);
	for (i = 0; i < len ; i++) {

		if (i != 0 && (i % 16) == 0) {
			printf(": %s\n", text);
			bzero(text, 17);
		}

		printf("%.2x ", buff[i] & 0xff);

		if (buff[i] > ' ' && buff[i] < '~')
			text[i%16] = buff[i] & 0xff;
		else
			text[i%16] = '.';
	}

	if ((i % 16) != 0) {
		while ((i % 16) != 0) {
			printf("   ");
			i++;
		}
	}
	printf(": %s\n", text);
}
#else
#define dump_buff(x, y)
#endif

void
fuse_device_cleanup(dev_t dev)
{
	int unit = FUSEUNIT(dev);
	struct fuse_softc *sc;

	if (unit >= numfuse)
		return;
	sc = &fuse_softc[unit];

	sc->sc_fmp = NULL;
}

void
fuse_device_kqwakeup(dev_t dev)
{
	int unit = FUSEUNIT(dev);
	struct fuse_softc *sc;

	if (unit >= numfuse)
		return;

	sc = &fuse_softc[unit];
	selwakeup(&sc->sc_rsel);
}

void
fuseattach(int num)
{
	char *mem;
	u_long size;
	int i;

	if (num <= 0)
		return;
	size = num * sizeof(struct fuse_softc);
	mem = malloc(size, M_FUSEFS, M_NOWAIT | M_ZERO);

	if (mem == NULL) {
		printf("WARNING: no memory for fuse device\n");
		return;
	}
	fuse_softc = (struct fuse_softc *)mem;
	for (i = 0; i < num; i++) {
		struct fuse_softc *sc = &fuse_softc[i];

		sc->sc_dev.dv_unit = i;
		snprintf(sc->sc_dev.dv_xname, sizeof(sc->sc_dev.dv_xname),
		    "fuse%d", i);
		device_ref(&sc->sc_dev);
	}
	numfuse = num;
}

int
fuseopen(dev_t dev, int flags, int fmt, struct proc * p)
{
	int unit = FUSEUNIT(dev);
	struct fuse_softc *sc;

	if (unit >= numfuse)
		return (ENXIO);
	sc = &fuse_softc[unit];

	if (sc->sc_opened != FUSE_CLOSE || sc->sc_fmp)
		return (EBUSY);

	sc->sc_opened = FUSE_OPEN;

	return (0);
}

int
fuseclose(dev_t dev, int flags, int fmt, struct proc *p)
{
	int unit = FUSEUNIT(dev);
	struct fuse_softc *sc;

	if (unit >= numfuse)
		return (ENXIO);
	sc = &fuse_softc[unit];

	if (sc->sc_fmp) {
		printf("libfuse close the device without umount\n");
		sc->sc_fmp->sess_init = 0;
		sc->sc_fmp = NULL;
	}

	sc->sc_opened = FUSE_CLOSE;
	return (0);
}

int
fuseioctl(dev_t dev, u_long cmd, caddr_t addr, int flags, struct proc *p)
{
	int unit = FUSEUNIT(dev);
	struct fuse_softc *sc;
	int error = 0;

	if (unit >= numfuse)
		return (ENXIO);
	sc = &fuse_softc[unit];

	switch (cmd) {
	default:
		DPRINTF("bad ioctl number %d\n", cmd);
		return (ENODEV);
	}

	return (error);
}

int
fuseread(dev_t dev, struct uio *uio, int ioflag)
{
	int unit = FUSEUNIT(dev);
	struct fuse_softc *sc;
	struct fuse_msg *msg;
	int error = 0;
	int remain;
	int size;
	int len;

	if (unit >= numfuse)
		return (ENXIO);
	sc = &fuse_softc[unit];

	DPRINTF("read dev %i size %x\n", unit, uio->uio_resid);

	if (sc->sc_opened != FUSE_OPEN) {
		return (ENODEV);
	}

again:
	if (SIMPLEQ_EMPTY(&fmq_in)) {

		if (ioflag & O_NONBLOCK) {
			DPRINTF("Return EAGAIN");
			return (EAGAIN);
		}

		error = tsleep(&fmq_in, PWAIT, "fuse read", 0);

		if (error)
			return (error);
	}
	if (SIMPLEQ_EMPTY(&fmq_in))
		goto again;

	if (!SIMPLEQ_EMPTY(&fmq_in)) {
		msg = SIMPLEQ_FIRST(&fmq_in);

		switch (msg->hdr.opcode) {
		case FUSE_INIT:
			sc->sc_fmp = msg->fmp;
			msg->fmp->dev = dev;
			break;
		case FUSE_DESTROY:
			sc->sc_fmp = NULL;
			break;
		default:
			break;
		}

		len =  sizeof(struct fuse_in_header);
		if (msg->resid < len) {
			size = len - msg->resid;
			msg->resid += MIN(size, uio->uio_resid);
			error = uiomove(&msg->hdr, size, uio);

			dump_buff((char *)&msg->hdr, len);
		}

		if (msg->len > 0 && uio->uio_resid) {
			size = msg->hdr.len - msg->resid;
			msg->resid += MIN(size, uio->uio_resid);
			error = uiomove(msg->data, size, uio);

			dump_buff(msg->data, msg->len);
		}

		remain = (msg->hdr.len - msg->resid);
		DPRINTF("size remaining : %i\n", remain);

		if (error) {
			printf("error: %i\n", error);
			return (error);
		}

		/*
		 * msg moves from a simpleq to another
		 */
		if (remain == 0) {
			SIMPLEQ_REMOVE_HEAD(&fmq_in, node);
			SIMPLEQ_INSERT_TAIL(&fmq_wait, msg, node);
		}
	}

	return (error);
}

int
fusewrite(dev_t dev, struct uio *uio, int ioflag)
{
	struct fuse_out_header *hdr;
	struct fuse_msg *lastmsg;
	int unit = FUSEUNIT(dev);
	struct fuse_softc *sc;
	struct fuse_msg *msg;
	int error = 0;
	int caught = 0;
	int len;
	void *data;

	if (unit >= numfuse)
		return (ENXIO);
	sc = &fuse_softc[unit];

	DPRINTF("write %x bytes\n", uio->uio_resid);
	hdr = pool_get(&fusefs_msgout_pool, PR_WAITOK | PR_ZERO);
	bzero(hdr, sizeof(*hdr));

	if (uio->uio_resid < sizeof(struct fuse_out_header)) {
		printf("uio goes wrong\n");
		pool_put(&fusefs_msgout_pool, hdr);
		return (EINVAL);
	}

	/*
	 * get out header
	 */

	if ((error = uiomove(hdr, sizeof(struct fuse_out_header), uio)) != 0) {
		printf("uiomove failed\n");
		pool_put(&fusefs_msgout_pool, hdr);
		return (error);
	}

	dump_buff((char *)hdr, sizeof(struct fuse_out_header));

	/*
	 * check header validity
	 */
	if (uio->uio_resid + sizeof(struct fuse_out_header) != hdr->len ||
	    (uio->uio_resid && hdr->error) || SIMPLEQ_EMPTY(&fmq_wait) ) {
		printf("corrupted fuse header or queue empty\n");
		return (EINVAL);
	}

	/* fuse errno are negative */
	if (hdr->error)
		hdr->error = -(hdr->error);

	lastmsg = NULL;
	msg = SIMPLEQ_FIRST(&fmq_wait);

	for (; msg != NULL; msg = SIMPLEQ_NEXT(msg, node)) {

		if (msg->hdr.unique == hdr->unique) {
			DPRINTF("catch unique %i\n", msg->hdr.unique);
			caught = 1;
			break;
		}

		lastmsg = msg;
	}

	if (caught) {
		if (uio->uio_resid > 0) {
			len = uio->uio_resid;
			data = malloc(len, M_FUSEFS, M_WAITOK);
			error = uiomove(data, len, uio);

			dump_buff(data, len);
		} else {
			data = NULL;
		}

		if (!error)
			msg->cb(msg, hdr, data);

		/* the msg could not be the HEAD msg */
		if (lastmsg)
			SIMPLEQ_REMOVE_AFTER(&fmq_wait, lastmsg, node);
		else
			SIMPLEQ_REMOVE_HEAD(&fmq_wait, node);

		if (msg->type == msg_buff_async) {
			fuse_clean_msg(msg);

			if (data)
				free(data, M_FUSEFS);
		}

	} else {
		error = EINVAL;
	}

	pool_put(&fusefs_msgout_pool, hdr);
	return (error);
}

int
fusepoll(dev_t dev, int events, struct proc *p)
{
	int unit = FUSEUNIT(dev);
	struct fuse_softc *sc;
	int revents = 0;

	if (unit >= numfuse)
		return (ENXIO);
	sc = &fuse_softc[unit];

	if (events & (POLLIN | POLLRDNORM)) {
		if (!SIMPLEQ_EMPTY(&fmq_in))
			revents |= events & (POLLIN | POLLRDNORM);
	}

	if (events & (POLLOUT | POLLWRNORM))
		revents |= events & (POLLOUT | POLLWRNORM);

	if (revents == 0) {
		if (events & (POLLIN | POLLRDNORM))
			selrecord(p, &sc->sc_rsel);
	}

	return (revents);
}

int
fusekqfilter(dev_t dev, struct knote *kn)
{
	int unit = FUSEUNIT(dev);
	struct fuse_softc *sc;
	struct klist *klist;

	if (unit >= numfuse)
		return (ENXIO);
	sc = &fuse_softc[unit];

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &sc->sc_rsel.si_note;
		kn->kn_fop = &fuse_rd_filtops;
		break;
	case EVFILT_WRITE:
		klist = &sc->sc_rsel.si_note;
		kn->kn_fop = &fuse_seltrue_filtops;
		break;
	default:
		return (EINVAL);
	}

	kn->kn_hook = (caddr_t)sc;

	SLIST_INSERT_HEAD(klist, kn, kn_selnext);

	return (0);
}

void
filt_fuse_rdetach(struct knote *kn)
{
	struct fuse_softc *sc = (struct fuse_softc *)kn->kn_hook;
	struct klist *klist = &sc->sc_rsel.si_note;

	SLIST_REMOVE(klist, kn, knote, kn_selnext);
}

int
filt_fuse_read(struct knote *kn, long hint)
{
	int event = 0;

	if (!SIMPLEQ_EMPTY(&fmq_in)) {
		event = 1;
	}

	return (event);
}
