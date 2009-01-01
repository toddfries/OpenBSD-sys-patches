/*
 * Private include for xenbus communications.
 * 
 * Copyright (C) 2005 Rusty Russell, IBM Corporation
 *
 * This file may be distributed separately from the Linux kernel, or
 * incorporated into other software packages, subject to the following license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * $FreeBSD: src/sys/xen/xenbus/xenbus_comms.h,v 1.4 2008/12/29 06:31:03 kmacy Exp $
 */

#ifndef _XENBUS_COMMS_H
#define _XENBUS_COMMS_H

struct sx;
extern int xen_store_evtchn;
extern char *xen_store;

int xs_init(void);
int xb_init_comms(void);

/* Low level routines. */
int xb_write(const void *data, unsigned len, struct lock_object *);
int xb_read(void *data, unsigned len, struct lock_object *);
extern int xenbus_running;

char *kasprintf(const char *fmt, ...);


#endif /* _XENBUS_COMMS_H */
