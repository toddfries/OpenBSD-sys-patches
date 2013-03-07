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
#include <sys/poll.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>

#include "fuse_kernel.h"
#include "fuse_node.h"
#include "fusefs.h"

#ifdef    FUSE_DEV_DEBUG
#define    DPRINTF(fmt, arg...)    printf("fuse device: " fmt, ##arg)
#else
#define    DPRINTF(fmt, arg...)
#endif

struct fuse_dev {
    int opened;
    int end;
};

#define FUSE_OPEN 1
#define FUSE_CLOSE 0
#define FUSE_DONE 2

static struct fuse_dev *fuse_devs[MAX_FUSE_DEV];

void    fuseattach(int);
int    fuseopen(dev_t, int, int, struct proc *);
int    fuseclose(dev_t, int, int, struct proc *);
int    fuseioctl(dev_t, u_long, caddr_t, int, struct proc *);
int    fuseread(dev_t, struct uio *, int);
int    fusewrite(dev_t, struct uio *, int);
int    fusepoll(dev_t, int, struct proc *);

struct fuse_msg_head fmq_in;
struct fuse_msg_head fmq_wait;

#ifdef    FUSE_DEV_DEBUG
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
fuseattach(int num)
{
    DPRINTF("fuse attach\n");
}

int
fuseopen(dev_t dev, int flags, int fmt, struct proc * p)
{
    if (minor(dev) >= MAX_FUSE_DEV &&
        fuse_devs[minor(dev)]->opened != FUSE_CLOSE)
        return (ENXIO);

    DPRINTF("open dev %i\n", minor(dev));

    fuse_devs[minor(dev)] = malloc(sizeof(*fuse_devs[minor(dev)]),
        M_FUSEFS, M_WAITOK | M_ZERO);
    fuse_devs[minor(dev)]->opened = FUSE_OPEN;

    return (0);
}

int
fuseclose(dev_t dev, int flags, int fmt, struct proc *p)
{
    if (minor(dev) >= MAX_FUSE_DEV)
        return (ENXIO);

    DPRINTF("close dev %i\n", minor(dev));

    fuse_devs[minor(dev)]->opened = FUSE_CLOSE;
    free(fuse_devs[minor(dev)], M_FUSEFS);
    fuse_devs[minor(dev)] = NULL;
    return (0);
}

int
fuseioctl(dev_t dev, u_long cmd, caddr_t addr, int flags, struct proc *p)
{
    int error = 0;

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
    int error = 0;
    struct fuse_msg *msg;

    DPRINTF("read 0x%x\n", dev);


    if (fuse_devs[minor(dev)]->opened != FUSE_OPEN) {
        return (ENODEV);
    }

again:
    if (TAILQ_EMPTY(&fmq_in)) {

        if (ioflag & O_NONBLOCK) {
            return (EAGAIN);
        }

        error = tsleep(&fmq_in, PWAIT, "fuse read", 0);

        if (error)
            return (error);
    }
    if (TAILQ_EMPTY(&fmq_in))
        goto again;

    if (!TAILQ_EMPTY(&fmq_in)) {
        msg = TAILQ_FIRST(&fmq_in);

        if (msg->hdr->opcode == FUSE_DESTROY) {
            DPRINTF("catch done\n");
            fuse_devs[minor(dev)]->opened = FUSE_DONE;
        }

        error = uiomove(msg->hdr, sizeof(struct fuse_in_header), uio);

        DPRINTF("hdr r:\n");
        dump_buff((char *)msg->hdr, sizeof(struct fuse_in_header));

        if (msg->len > 0) {
            error = uiomove(msg->data, msg->len, uio);
            DPRINTF("data r:\n");
            dump_buff(msg->data, msg->len);
        }

        DPRINTF("msg send : %i\n", msg->len);

        if (error) {
            DPRINTF("error: %i\n", error);
            return (error);
        }

        /*
          * msg moves from a tailq to another
          */
        TAILQ_REMOVE(&fmq_in, msg, node);
        TAILQ_INSERT_TAIL(&fmq_wait, msg, node);
    }

    return (error);
}

int
fusewrite(dev_t dev, struct uio *uio, int ioflag)
{
    struct fuse_out_header hdr;
    struct fuse_msg *msg;
    int error = 0;
    int caught = 0;
    int len;
    void *data;

    DPRINTF("write %x bytes\n", uio->uio_resid);

    if (uio->uio_resid < sizeof(struct fuse_out_header)) {
        DPRINTF("uio goes wrong\n");
        return (EINVAL);
    }

    /*
     * get out header
     */

    if ((error = uiomove(&hdr, sizeof(struct fuse_out_header), uio)) != 0) {
        DPRINTF("uiomove failed\n");
        return (error);
    }
    DPRINTF("hdr w:\n");
    dump_buff((char *)&hdr, sizeof(struct fuse_out_header));

    /*
     * check header validity
     */
    if (uio->uio_resid + sizeof(struct fuse_out_header) != hdr.len ||
        (uio->uio_resid && hdr.error) || TAILQ_EMPTY(&fmq_wait) ) {
        DPRINTF("corrupted fuse header or queue empty\n");
        return (EINVAL);
    }

    /* fuse errno are negative */
    if (hdr.error)
        hdr.error = -(hdr.error);

    TAILQ_FOREACH(msg, &fmq_wait, node) {
        if (msg->hdr->unique == hdr.unique) {
            DPRINTF("catch unique %i\n", msg->hdr->unique);
            caught = 1;
            break;
        }
    }

    if (caught) {
        if (uio->uio_resid > 0) {
            len = uio->uio_resid;
            data = malloc(len, M_FUSEFS, M_WAITOK);
            error = uiomove(data, len, uio);

            DPRINTF("data w:\n");
            dump_buff(data, len);
        } else {
            data = NULL;
        }

        DPRINTF("call callback\n");

        if (!error)
            msg->cb(msg, &hdr, data);

        TAILQ_REMOVE(&fmq_wait, msg, node);

        if (msg->type == msg_buff_async) {
            free(msg->hdr, M_FUSEFS);
            free(msg, M_FUSEFS);

            if (data)
                free(data, M_FUSEFS);
        }

    } else {
        error = EINVAL;
    }

    return (error);
}

int
fusepoll(dev_t dev, int events, struct proc *p)
{
    int revents = 0;

    DPRINTF("fuse poll\n");

    if (events & (POLLIN | POLLRDNORM)) {
        if (!TAILQ_EMPTY(&fmq_in))
            revents |= events & (POLLIN | POLLRDNORM);
    }

    if (events & (POLLOUT | POLLWRNORM))
        revents |= events & (POLLOUT | POLLWRNORM);

    return (revents);
}
