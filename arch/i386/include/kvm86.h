/* $OpenBSD: kvm86.h,v 1.2 2011/03/23 16:54:35 pirofti Exp $ */
/*
 * Copyright (c) 2006 Gordon Willem Klok <gwk@openbsd.org>
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
#ifndef _MACHINE_KVM86_H_
#define _MACHINE_KVM86_H_

struct kvm86regs {
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	uint32_t esi;
	uint32_t edi;
	uint32_t eflags;
	uint32_t es;
};

extern int kvm86_incall;

void kvm86_init(void);
void kvm86_gpfault(struct trapframe *);

void *kvm86_bios_addpage(uint32_t);
void kvm86_bios_delpage(uint32_t, void *);
size_t kvm86_bios_read(uint32_t, char *, size_t);

int kvm86_bioscall(int, struct trapframe *);
int kvm86_simplecall(int, struct kvm86regs *);

#endif
