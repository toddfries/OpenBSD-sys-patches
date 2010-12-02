/*
 * Copyright (c) 2010 Owain G. Ainsworth <oga@openbsd.org>
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

#include <net/netisr.h>

#include <machine/intr.h>

void	 netintr(void *);

int	 netisr;
void	*netisr_intr;

void
netintr(void *unused) /* ARGSUSED */
{
	int n;
	while ((n = netisr) != 0) {
		atomic_clearbits_int(&netisr, n);
#define DONETISR(bit, fn)			\
		do {				\
			if (n & 1 << (bit))	\
				fn();		\
		} while ( /* CONSTCOND */ 0)
#include <net/netisr_dispatch.h>

#undef DONETISR
	}
}

void
netisr_init(void)
{
	netisr_intr = softintr_establish(IPL_SOFTNET, netintr, NULL);
	if (netisr_intr == NULL)
		panic("can't establish softnet handler");
}
