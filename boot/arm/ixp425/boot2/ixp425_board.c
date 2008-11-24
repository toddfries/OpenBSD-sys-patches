/*-
 * Copyright (c) 2008 John Hay.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/boot/arm/ixp425/boot2/ixp425_board.c,v 1.1 2008/10/06 19:38:10 jhay Exp $");
#include <sys/param.h>
#include <sys/ata.h>

#include <stdarg.h>

#include "lib.h"
#include "cf_ata.h"

#include <arm/xscale/ixp425/ixp425reg.h>
#include <dev/ic/ns16550.h>

static u_int8_t *ubase;

#define BOARD_AVILA		0
#define BOARD_PRONGHORN		1
static int board;

static u_int8_t uart_getreg(u_int8_t *, int);
static void uart_setreg(u_int8_t *, int, u_int8_t);

static void cf_init(void);
static void cf_clr(void);

#ifdef DEBUG
#define	DPRINTF(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define	DPRINTF(fmt, ...)
#endif

const char *
board_init(void)
{
	volatile u_int32_t *cs;
	const char *bt = NULL;

	/*
	 * Redboot only configure the chip selects that are needed, so
	 * use that to figure out if it is an Avila or ADI board. The
	 * Avila boards use CS2 and ADI does not.
	 */
	cs = (u_int32_t *)(IXP425_EXP_HWBASE + EXP_TIMING_CS2_OFFSET);
	if (*cs != 0) {
		board = BOARD_AVILA;
		bt = "Avila";
	} else {
		board = BOARD_PRONGHORN;
		bt = "Pronghorn Metro";
	}

	/* Config the serial port. RedBoot should do the rest. */
	if (board == BOARD_AVILA)
		ubase = (u_int8_t *)(IXP425_UART0_HWBASE);
	else
		ubase = (u_int8_t *)(IXP425_UART1_HWBASE);

	cf_init();

	return bt;
}

/*
 * This should be called just before starting the kernel. This is so
 * that one can undo incompatable hardware settings.
 */
void
clr_board(void)
{
	cf_clr();
}

/*
 * General support functions.
 */

/*
 * DELAY should delay for the number of microseconds.
 * The idea is that the inner loop should take 1us, so val is the
 * number of usecs to delay.
 */
void
DELAY(int val)
{
	volatile int sub;
	volatile int subsub;

	sub = val;
	while(sub) {
		subsub = 3;
		while(subsub)
			subsub--;
		sub--;
	}
}

u_int32_t
swap32(u_int32_t a)
{
	return (((a & 0xff) << 24) | ((a & 0xff00) << 8) |
	    ((a & 0xff0000) >> 8) | ((a & 0xff000000) >> 24));
}

u_int16_t
swap16(u_int16_t val)
{
	return (val << 8) | (val >> 8);
}

/*
 * uart related funcs
 */
static u_int8_t
uart_getreg(u_int8_t *bas, int off)
{
	return *((volatile u_int32_t *)(bas + (off << 2))) & 0xff;
}

static void
uart_setreg(u_int8_t *bas, int off, u_int8_t val)
{
	*((volatile u_int32_t *)(bas + (off << 2))) = (u_int32_t)val;
}

int
getc(int seconds)
{
	int c, delay, limit;

	c = 0;
	delay = 10000;
	limit = seconds * 1000000/10000;
	while ((uart_getreg(ubase, REG_LSR) & LSR_RXRDY) == 0 && --limit)
		DELAY(delay);

	if ((uart_getreg(ubase, REG_LSR) & LSR_RXRDY) == LSR_RXRDY)
		c = uart_getreg(ubase, REG_DATA);

	return c;
}

void
putchar(int ch)
{
	int delay, limit;

	delay = 500;
	limit = 20;
	while ((uart_getreg(ubase, REG_LSR) & LSR_THRE) == 0 && --limit)
		DELAY(delay);
	uart_setreg(ubase, REG_DATA, ch);

	limit = 40;
	while ((uart_getreg(ubase, REG_LSR) & LSR_TEMT) == 0 && --limit)
		DELAY(delay);
}

void
xputchar(int ch)
{
	if (ch == '\n')
		putchar('\r');
	putchar(ch);
}

void
putstr(const char *str)
{
	while(*str)
		xputchar(*str++);
}

void
puthex8(u_int8_t ch)
{
	const char *hex = "0123456789abcdef";

	putchar(hex[ch >> 4]);
	putchar(hex[ch & 0xf]);
}

void
puthexlist(const u_int8_t *str, int length)
{
	while(length) {
		puthex8(*str);
		putchar(' ');
		str++;
		length--;
	}
}

/*
 *
 * CF/IDE functions.
 *
 */

struct {
	u_int64_t dsize;
	u_int64_t total_secs;
	u_int8_t heads;
	u_int8_t sectors;
	u_int32_t cylinders;

	u_int8_t *cs1;
	u_int8_t *cs2;

	u_int32_t use_lba;
	u_int32_t use_stream8;
	u_int32_t debug;

	u_int8_t status;
	u_int8_t error;
} dskinf;

static void cfenable16(void);
static void cfdisable16(void);
static u_int8_t cfread8(u_int32_t off);
static u_int16_t cfread16(u_int32_t off);
static void cfreadstream8(void *buf, int length);
static void cfreadstream16(void *buf, int length);
static void cfwrite8(u_int32_t off, u_int8_t val);
static u_int8_t cfaltread8(u_int32_t off);
static void cfaltwrite8(u_int32_t off, u_int8_t val);
static int cfwait(u_int8_t mask);
static int cfaltwait(u_int8_t mask);
static int cfcmd(u_int32_t cmd, u_int32_t cylinder, u_int32_t head,
    u_int32_t sector, u_int32_t count, u_int32_t feature);
static void cfreset(void);
#ifdef DEBUG
static int cfgetparams(void);
#endif
static void cfprintregs(void);

static void
cf_init(void)
{
	u_int8_t status;
#ifdef DEBUG
	int rval;
#endif
	volatile u_int32_t *cs;

	/* Setup the CF select timeing. Maybe already done by RedBoot? */
	if (board == BOARD_AVILA) {
		cs = (u_int32_t *)(IXP425_EXP_HWBASE + EXP_TIMING_CS1_OFFSET);
		*cs |= (EXP_BYTE_EN | EXP_WR_EN | EXP_BYTE_RD16 | EXP_CS_EN);
		DPRINTF("t1 %x, ", *cs);

		cs = (u_int32_t *)(IXP425_EXP_HWBASE + EXP_TIMING_CS2_OFFSET);
		*cs |= (EXP_BYTE_EN | EXP_WR_EN | EXP_BYTE_RD16 | EXP_CS_EN);
		DPRINTF("t2 %x\n", *cs);

		dskinf.cs1 = (u_int8_t *)IXP425_EXP_BUS_CS1_HWBASE;
		dskinf.cs2 = (u_int8_t *)IXP425_EXP_BUS_CS2_HWBASE;
	} else {
		cs = (u_int32_t *)(IXP425_EXP_HWBASE + EXP_TIMING_CS3_OFFSET);
		*cs |= (EXP_BYTE_EN | EXP_WR_EN | EXP_BYTE_RD16 | EXP_CS_EN);
		DPRINTF("t1 %x, ", *cs);

		cs = (u_int32_t *)(IXP425_EXP_HWBASE + EXP_TIMING_CS4_OFFSET);
		*cs |= (EXP_BYTE_EN | EXP_WR_EN | EXP_BYTE_RD16 | EXP_CS_EN);
		DPRINTF("t2 %x\n", *cs);

		dskinf.cs1 = (u_int8_t *)IXP425_EXP_BUS_CS3_HWBASE;
		dskinf.cs2 = (u_int8_t *)IXP425_EXP_BUS_CS4_HWBASE;
	}
	DPRINTF("cs1 %x, cs2 %x\n", dskinf.cs1, dskinf.cs2);

	dskinf.use_stream8 = 0;
	dskinf.use_lba = 0;
	dskinf.debug = 1;

	/* Detect if there is a disk. */
	cfwrite8(CF_DRV_HEAD, CF_D_IBM);
	DELAY(1000);
	status = cfread8(CF_STATUS);
	if (status != 0x50)
		printf("cf-ata0 %x\n", (u_int32_t)status);
	if (status == 0xff) {
		printf("cf_ata0: No disk!\n");
		return;
	}

	cfreset();

	if (dskinf.use_stream8) {
		DPRINTF("setting %d bit mode.\n", 8);
		cfwrite8(CF_FEATURE, 0x01); /* Enable 8 bit transfers */
		cfwrite8(CF_COMMAND, ATA_SETFEATURES);
		cfaltwait(CF_S_READY);
	}

#ifdef DEBUG
	rval = cfgetparams();
	if (rval)
		return;
#endif
	dskinf.use_lba = 1;
	dskinf.debug = 0;
}

static void
cf_clr(void)
{
	cfwrite8(CF_DRV_HEAD, CF_D_IBM);
	cfaltwait(CF_S_READY);
	cfwrite8(CF_FEATURE, 0x81); /* Enable 8 bit transfers */
	cfwrite8(CF_COMMAND, ATA_SETFEATURES);
	cfaltwait(CF_S_READY);
}

static void
cfenable16(void)
{
	u_int32_t val;
	volatile u_int32_t *cs;

	if (board == BOARD_AVILA)
		cs = (u_int32_t *)(IXP425_EXP_HWBASE + EXP_TIMING_CS1_OFFSET);
	else
		cs = (u_int32_t *)(IXP425_EXP_HWBASE + EXP_TIMING_CS3_OFFSET);
	val = *cs;
	*cs = val & (~1);
	DPRINTF("cfenable16: cs1 timing reg %x\n", *cs);
}

static void
cfdisable16(void)
{
	u_int32_t val;
	volatile u_int32_t *cs;

	if (board == BOARD_AVILA)
		cs = (u_int32_t *)(IXP425_EXP_HWBASE + EXP_TIMING_CS1_OFFSET);
	else
		cs = (u_int32_t *)(IXP425_EXP_HWBASE + EXP_TIMING_CS3_OFFSET);
	val = *cs;
	*cs = val | 1;
}

static u_int8_t
cfread8(u_int32_t off)
{
	volatile u_int8_t *vp;

	vp = (volatile u_int8_t *)(dskinf.cs1 + off);
	return *vp;
}

static void
cfreadstream8(void *buf, int length)
{
	u_int8_t *lbuf;
	u_int8_t tmp;

	lbuf = buf;
	while (length) {
		tmp = cfread8(CF_DATA);
		*lbuf = tmp;
#ifdef DEBUG
		if (dskinf.debug && (length > (512 - 32))) {
			if ((length % 16) == 0)
				xputchar('\n');
			puthex8(tmp);
			putchar(' ');
		}
#endif
		lbuf++;
		length--;
	}
#ifdef DEBUG
	if (dskinf.debug)
		xputchar('\n');
#endif
}

static u_int16_t
cfread16(u_int32_t off)
{
	volatile u_int16_t *vp;

	vp = (volatile u_int16_t *)(dskinf.cs1 + off);
	return swap16(*vp);
}

static void
cfreadstream16(void *buf, int length)
{
	u_int16_t *lbuf;

	length = length / 2;
	cfenable16();
	lbuf = buf;
	while (length--) {
		*lbuf = cfread16(CF_DATA);
		lbuf++;
	}
	cfdisable16();
}

static void
cfwrite8(u_int32_t off, u_int8_t val)
{
	volatile u_int8_t *vp;

	vp = (volatile u_int8_t *)(dskinf.cs1 + off);
	*vp = val;
}

#if 0
static void
cfwrite16(u_int32_t off, u_int16_t val)
{
	volatile u_int16_t *vp;

	vp = (volatile u_int16_t *)(dskinf.cs1 + off);
	*vp = val;
}
#endif

static u_int8_t
cfaltread8(u_int32_t off)
{
	volatile u_int8_t *vp;

	off &= 0x0f;
	vp = (volatile u_int8_t *)(dskinf.cs2 + off);
	return *vp;
}

static void
cfaltwrite8(u_int32_t off, u_int8_t val)
{
	volatile u_int8_t *vp;

	/*
	 * This is documented in the Intel appnote 302456.
	 */
	off &= 0x0f;
	vp = (volatile u_int8_t *)(dskinf.cs2 + off);
	*vp = val;
}

static int
cfwait(u_int8_t mask)
{
	u_int8_t status;
	u_int32_t tout;

	tout = 0;
	while (tout <= 5000000) {
		status = cfread8(CF_STATUS);
		if (status == 0xff) {
			printf("cfwait: master: no status, reselecting\n");
			cfwrite8(CF_DRV_HEAD, CF_D_IBM);
			DELAY(1);
			status = cfread8(CF_STATUS);
		}
		if (status == 0xff)
			return -1;
		dskinf.status = status;
		if (!(status & CF_S_BUSY)) {
			if (status & CF_S_ERROR)
				dskinf.error = cfread8(CF_ERROR);
			if ((status & mask) == mask) {
				DPRINTF("cfwait: tout %u\n", tout);
				return (status & CF_S_ERROR);
			}
		}
		if (tout > 1000) {
			tout += 1000;
			DELAY(1000);
		} else {
			tout += 10;
			DELAY(10);
		}
	}
	return -1;
}

static int
cfaltwait(u_int8_t mask)
{
	u_int8_t status;
	u_int32_t tout;

	tout = 0;
	while (tout <= 5000000) {
		status = cfaltread8(CF_ALT_STATUS);
		if (status == 0xff) {
			printf("cfaltwait: master: no status, reselectin\n");
			cfwrite8(CF_DRV_HEAD, CF_D_IBM);
			DELAY(1);
			status = cfread8(CF_STATUS);
		}
		if (status == 0xff)
			return -1;
		dskinf.status = status;
		if (!(status & CF_S_BUSY)) {
			if (status & CF_S_ERROR)
				dskinf.error = cfread8(CF_ERROR);
			if ((status & mask) == mask) {
				DPRINTF("cfaltwait: tout %u\n", tout);
				return (status & CF_S_ERROR);
			}
		}
		if (tout > 1000) {
			tout += 1000;
			DELAY(1000);
		} else {
			tout += 10;
			DELAY(10);
		}
	}
	return -1;
}

static int
cfcmd(u_int32_t cmd, u_int32_t cylinder, u_int32_t head, u_int32_t sector,
    u_int32_t count, u_int32_t feature)
{
	if (cfwait(0) < 0) {
		printf("cfcmd: timeout\n");
		return -1;
	}
	cfwrite8(CF_FEATURE, feature);
	cfwrite8(CF_CYL_L, cylinder);
	cfwrite8(CF_CYL_H, cylinder >> 8);
	if (dskinf.use_lba)
		cfwrite8(CF_DRV_HEAD, CF_D_IBM | CF_D_LBA | head);
	else
		cfwrite8(CF_DRV_HEAD, CF_D_IBM | head);
	cfwrite8(CF_SECT_NUM, sector);
	cfwrite8(CF_SECT_CNT, count);
	cfwrite8(CF_COMMAND, cmd);
	return 0;
}

static void
cfreset(void)
{
	u_int8_t status;
	u_int32_t tout;

	cfwrite8(CF_DRV_HEAD, CF_D_IBM);
	DELAY(1);
#ifdef DEBUG
	cfprintregs();
#endif
	cfread8(CF_STATUS);
	cfaltwrite8(CF_ALT_DEV_CTR, CF_A_IDS | CF_A_RESET);
	DELAY(10000);
	cfaltwrite8(CF_ALT_DEV_CTR, CF_A_IDS);
	DELAY(10000);
	cfread8(CF_ERROR);
	DELAY(3000);

	for (tout = 0; tout < 310000; tout++) {
		cfwrite8(CF_DRV_HEAD, CF_D_IBM);
		DELAY(1);
		status = cfread8(CF_STATUS);
		if (!(status & CF_S_BUSY))
			break;
		DELAY(100);
	}
	DELAY(1);
	if (status & CF_S_BUSY) {
		cfprintregs();
		printf("cfreset: Status stayed busy after reset.\n");
	}
	DPRINTF("cfreset: finished, tout %u\n", tout);
}

#ifdef DEBUG
static int
cfgetparams(void)
{
	u_int8_t *buf;

	buf = (u_int8_t *)(0x170000);
	p_memset((char *)buf, 0, 1024);
	/* Select the drive. */
	cfwrite8(CF_DRV_HEAD, CF_D_IBM);
	DELAY(1);
	cfcmd(ATA_ATA_IDENTIFY, 0, 0, 0, 0, 0);
	if (cfaltwait(CF_S_READY | CF_S_DSC | CF_S_DRQ)) {
		printf("cfgetparams: ATA_IDENTIFY failed.\n");
		return -1;
	}
	if (dskinf.use_stream8)
		cfreadstream8(buf, 512);
	else
		cfreadstream16(buf, 512);
	if (dskinf.debug)
		cfprintregs();
#if 0
	memcpy(&dskinf.ata_params, buf, sizeof(struct ata_params));
	dskinf.cylinders = dskinf.ata_params.cylinders;
	dskinf.heads = dskinf.ata_params.heads;
	dskinf.sectors = dskinf.ata_params.sectors;
	printf("dsk0: sec %x, hd %x, cyl %x, stat %x, err %x\n",
	    (u_int32_t)dskinf.ata_params.sectors,
	    (u_int32_t)dskinf.ata_params.heads,
	    (u_int32_t)dskinf.ata_params.cylinders,
	    (u_int32_t)dskinf.status,
	    (u_int32_t)dskinf.error);
#endif
	dskinf.status = cfread8(CF_STATUS);
	if (dskinf.debug)
		printf("cfgetparams: ata_params * %x, stat %x\n",
		    (u_int32_t)buf, (u_int32_t)dskinf.status);
	return 0;
}
#endif /* DEBUG */

static void
cfprintregs(void)
{
	u_int8_t rv;

	putstr("cfprintregs: regs error ");
	rv = cfread8(CF_ERROR);
	puthex8(rv);
	putstr(", count ");
	rv = cfread8(CF_SECT_CNT);
	puthex8(rv);
	putstr(", sect ");
	rv = cfread8(CF_SECT_NUM);
	puthex8(rv);
	putstr(", cyl low ");
	rv = cfread8(CF_CYL_L);
	puthex8(rv);
	putstr(", cyl high ");
	rv = cfread8(CF_CYL_H);
	puthex8(rv);
	putstr(", drv head ");
	rv = cfread8(CF_DRV_HEAD);
	puthex8(rv);
	putstr(", status ");
	rv = cfread8(CF_STATUS);
	puthex8(rv);
	putstr("\n");
}

int
avila_read(char *dest, unsigned source, unsigned length)
{
	if (dskinf.use_lba == 0 && source == 0)
		source++;
	if (dskinf.debug)
		printf("avila_read: 0x%x, sect %d num secs %d\n",
		    (u_int32_t)dest, source, length);
	while (length) {
		cfwait(CF_S_READY);
		/* cmd, cyl, head, sect, count, feature */
		cfcmd(ATA_READ, (source >> 8) & 0xffff, source >> 24,
		    source & 0xff, 1, 0);

		cfwait(CF_S_READY | CF_S_DRQ | CF_S_DSC);
		if (dskinf.use_stream8)
			cfreadstream8(dest, 512);
		else
			cfreadstream16(dest, 512);
		length--;
		source++;
		dest += 512;
	}
	return 0;
}

