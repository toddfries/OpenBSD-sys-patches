/*	$OpenBSD: tumbler.c,v 1.6 2008/10/29 00:04:14 jakemsr Exp $	*/

/*-
 * Copyright (c) 2001,2003 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Datasheet is available from
 * http://focus.ti.com/docs/prod/folders/print/tas3001.html
 */

#include <sys/param.h>
#include <sys/audioio.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/audio_if.h>
#include <dev/ofw/openfirm.h>
#include <macppc/dev/dbdma.h>

#include <machine/autoconf.h>

#include <macppc/dev/i2svar.h>

#ifdef TUMBLER_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

/* XXX */
#define tumbler_softc i2s_softc

/* XXX */
int kiic_write(struct device *, int, int, const void *, int);
int kiic_writereg(struct device *, int, u_int);

void tumbler_init(struct tumbler_softc *);
int tumbler_getdev(void *, struct audio_device *);
int tumbler_match(struct device *, void *, void *);
void tumbler_attach(struct device *, struct device *, void *);
void tumbler_defer(struct device *);
void tumbler_set_volume(struct tumbler_softc *, int, int);
void tumbler_set_bass(struct tumbler_softc *, int);
void tumbler_set_treble(struct tumbler_softc *, int);
void tumbler_get_default_params(void *, int, struct audio_params *);

int tas3001_write(struct tumbler_softc *, u_int, const void *);
int tas3001_init(struct tumbler_softc *);

struct cfattach tumbler_ca = {
	sizeof(struct tumbler_softc), tumbler_match, tumbler_attach
};
struct cfdriver tumbler_cd = {
	NULL, "tumbler", DV_DULL
};

struct audio_hw_if tumbler_hw_if = {
	i2s_open,
	i2s_close,
	NULL,
	i2s_query_encoding,
	i2s_set_params,
	i2s_round_blocksize,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	i2s_halt_output,
	i2s_halt_input,
	NULL,
	tumbler_getdev,
	NULL,
	i2s_set_port,
	i2s_get_port,
	i2s_query_devinfo,
	i2s_allocm,		/* allocm */
	NULL,
	i2s_round_buffersize,
	i2s_mappage,
	i2s_get_props,
	i2s_trigger_output,
	i2s_trigger_input,
	tumbler_get_default_params
};

struct audio_device tumbler_device = {
	"TUMBLER",
	"",
	"tumbler"
};

const uint8_t tumbler_trebletab[] = {
	0x96,	/* -18dB */
	0x94,	/* -17dB */
	0x92,	/* -16dB */
	0x90,	/* -15dB */
	0x8e,	/* -14dB */
	0x8c,	/* -13dB */
	0x8a,	/* -12dB */
	0x88,	/* -11dB */
	0x86,	/* -10dB */
	0x84,	/* -9dB */
	0x82,	/* -8dB */
	0x80,	/* -7dB */
	0x7e,	/* -6dB */
	0x7c,	/* -5dB */
	0x7a,	/* -4dB */
	0x78,	/* -3dB */
	0x76,	/* -2dB */
	0x74,	/* -1dB */
	0x72,	/* 0dB */
	0x70,	/* 1dB */
	0x6d,	/* 2dB */
	0x6b,	/* 3dB */
	0x68,	/* 4dB */
	0x65,	/* 5dB */
	0x62,	/* 6dB */
	0x5e,	/* 7dB */
	0x59,	/* 8dB */
	0x5a,	/* 9dB */
	0x4f,	/* 10dB */
	0x49,	/* 11dB */
	0x42,	/* 12dB */
	0x3a,	/* 13dB */
	0x32,	/* 14dB */
	0x28,	/* 15dB */
	0x1c,	/* 16dB */
	0x10,	/* 17dB */
	0x01,	/* 18dB */
};

const uint8_t tumbler_basstab[] = {
	0x86,	/* -18dB */
	0x7f,	/* -17dB */
	0x7a,	/* -16dB */
	0x76,	/* -15dB */
	0x72,	/* -14dB */
	0x6e,	/* -13dB */
	0x6b,	/* -12dB */
	0x66,	/* -11dB */
	0x61,	/* -10dB */
	0x5d,	/* -9dB */
	0x5a,	/* -8dB */
	0x58,	/* -7dB */
	0x55,	/* -6dB */
	0x53,	/* -5dB */
	0x4f,	/* -4dB */
	0x4b,	/* -3dB */
	0x46,	/* -2dB */
	0x42,	/* -1dB */
	0x3e,	/* 0dB */
	0x3b,	/* 1dB */
	0x38,	/* 2dB */
	0x35,	/* 3dB */
	0x31,	/* 4dB */
	0x2e,	/* 5dB */
	0x2b,	/* 6dB */
	0x28,	/* 7dB */
	0x25,	/* 8dB */
	0x21,	/* 9dB */
	0x1c,	/* 10dB */
	0x18,	/* 11dB */
	0x16,	/* 12dB */
	0x13,	/* 13dB */
	0x10,	/* 14dB */
	0x0d,	/* 15dB */
	0x0a,	/* 16dB */
	0x06,	/* 17dB */
	0x01,	/* 18dB */
};

struct {
	int high, mid, low;
} tumbler_volumetab[] = {
	{ 0x07, 0xF1, 0x7B }, /* 18.0 */
	{ 0x07, 0x7F, 0xBB }, /* 17.5 */
	{ 0x07, 0x14, 0x57 }, /* 17.0 */
	{ 0x06, 0xAE, 0xF6 }, /* 16.5 */
	{ 0x06, 0x4F, 0x40 }, /* 16.0 */
	{ 0x05, 0xF4, 0xE5 }, /* 15.5 */
	{ 0x05, 0x9F, 0x98 }, /* 15.0 */
	{ 0x05, 0x4F, 0x10 }, /* 14.5 */
	{ 0x05, 0x03, 0x0A }, /* 14.0 */
	{ 0x04, 0xBB, 0x44 }, /* 13.5 */
	{ 0x04, 0x77, 0x83 }, /* 13.0 */
	{ 0x04, 0x37, 0x8B }, /* 12.5 */
	{ 0x03, 0xFB, 0x28 }, /* 12.0 */
	{ 0x03, 0xC2, 0x25 }, /* 11.5 */
	{ 0x03, 0x8C, 0x53 }, /* 11.0 */
	{ 0x03, 0x59, 0x83 }, /* 10.5 */
	{ 0x03, 0x29, 0x8B }, /* 10.0 */
	{ 0x02, 0xFC, 0x42 }, /* 9.5 */
	{ 0x02, 0xD1, 0x82 }, /* 9.0 */
	{ 0x02, 0xA9, 0x25 }, /* 8.5 */
	{ 0x02, 0x83, 0x0B }, /* 8.0 */
	{ 0x02, 0x5F, 0x12 }, /* 7.5 */
	{ 0x02, 0x3D, 0x1D }, /* 7.0 */
	{ 0x02, 0x1D, 0x0E }, /* 6.5 */
	{ 0x01, 0xFE, 0xCA }, /* 6.0 */
	{ 0x01, 0xE2, 0x37 }, /* 5.5 */
	{ 0x01, 0xC7, 0x3D }, /* 5.0 */
	{ 0x01, 0xAD, 0xC6 }, /* 4.5 */
	{ 0x01, 0x95, 0xBC }, /* 4.0 */
	{ 0x01, 0x7F, 0x09 }, /* 3.5 */
	{ 0x01, 0x69, 0x9C }, /* 3.0 */
	{ 0x01, 0x55, 0x62 }, /* 2.5 */
	{ 0x01, 0x42, 0x49 }, /* 2.0 */
	{ 0x01, 0x30, 0x42 }, /* 1.5 */
	{ 0x01, 0x1F, 0x3D }, /* 1.0 */
	{ 0x01, 0x0F, 0x2B }, /* 0.5 */
	{ 0x01, 0x00, 0x00 }, /* 0.0 */
	{ 0x00, 0xF1, 0xAE }, /* -0.5 */
	{ 0x00, 0xE4, 0x29 }, /* -1.0 */
	{ 0x00, 0xD7, 0x66 }, /* -1.5 */
	{ 0x00, 0xCB, 0x59 }, /* -2.0 */
	{ 0x00, 0xBF, 0xF9 }, /* -2.5 */
	{ 0x00, 0xB5, 0x3C }, /* -3.0 */
	{ 0x00, 0xAB, 0x19 }, /* -3.5 */
	{ 0x00, 0xA1, 0x86 }, /* -4.0 */
	{ 0x00, 0x98, 0x7D }, /* -4.5 */
	{ 0x00, 0x8F, 0xF6 }, /* -5.0 */
	{ 0x00, 0x87, 0xE8 }, /* -5.5 */
	{ 0x00, 0x80, 0x4E }, /* -6.0 */
	{ 0x00, 0x79, 0x20 }, /* -6.5 */
	{ 0x00, 0x72, 0x5A }, /* -7.0 */
	{ 0x00, 0x6B, 0xF4 }, /* -7.5 */
	{ 0x00, 0x65, 0xEA }, /* -8.0 */
	{ 0x00, 0x60, 0x37 }, /* -8.5 */
	{ 0x00, 0x5A, 0xD5 }, /* -9.0 */
	{ 0x00, 0x55, 0xC0 }, /* -9.5 */
	{ 0x00, 0x50, 0xF4 }, /* -10.0 */
	{ 0x00, 0x4C, 0x6D }, /* -10.5 */
	{ 0x00, 0x48, 0x27 }, /* -11.0 */
	{ 0x00, 0x44, 0x1D }, /* -11.5 */
	{ 0x00, 0x40, 0x4E }, /* -12.0 */
	{ 0x00, 0x3C, 0xB5 }, /* -12.5 */
	{ 0x00, 0x39, 0x50 }, /* -13.0 */
	{ 0x00, 0x36, 0x1B }, /* -13.5 */
	{ 0x00, 0x33, 0x14 }, /* -14.0 */
	{ 0x00, 0x30, 0x39 }, /* -14.5 */
	{ 0x00, 0x2D, 0x86 }, /* -15.0 */
	{ 0x00, 0x2A, 0xFA }, /* -15.5 */
	{ 0x00, 0x28, 0x93 }, /* -16.0 */
	{ 0x00, 0x26, 0x4E }, /* -16.5 */
	{ 0x00, 0x24, 0x29 }, /* -17.0 */
	{ 0x00, 0x22, 0x23 }, /* -17.5 */
	{ 0x00, 0x20, 0x3A }, /* -18.0 */
	{ 0x00, 0x1E, 0x6D }, /* -18.5 */
	{ 0x00, 0x1C, 0xB9 }, /* -19.0 */
	{ 0x00, 0x1B, 0x1E }, /* -19.5 */
	{ 0x00, 0x19, 0x9A }, /* -20.0 */
	{ 0x00, 0x18, 0x2B }, /* -20.5 */
	{ 0x00, 0x16, 0xD1 }, /* -21.0 */
	{ 0x00, 0x15, 0x8A }, /* -21.5 */
	{ 0x00, 0x14, 0x56 }, /* -22.0 */
	{ 0x00, 0x13, 0x33 }, /* -22.5 */
	{ 0x00, 0x12, 0x20 }, /* -23.0 */
	{ 0x00, 0x11, 0x1C }, /* -23.5 */
	{ 0x00, 0x10, 0x27 }, /* -24.0 */
	{ 0x00, 0x0F, 0x40 }, /* -24.5 */
	{ 0x00, 0x0E, 0x65 }, /* -25.0 */
	{ 0x00, 0x0D, 0x97 }, /* -25.5 */
	{ 0x00, 0x0C, 0xD5 }, /* -26.0 */
	{ 0x00, 0x0C, 0x1D }, /* -26.5 */
	{ 0x00, 0x0B, 0x6F }, /* -27.0 */
	{ 0x00, 0x0A, 0xCC }, /* -27.5 */
	{ 0x00, 0x0A, 0x31 }, /* -28.0 */
	{ 0x00, 0x09, 0x9F }, /* -28.5 */
	{ 0x00, 0x09, 0x15 }, /* -29.0 */
	{ 0x00, 0x08, 0x93 }, /* -29.5 */
	{ 0x00, 0x08, 0x18 }, /* -30.0 */
	{ 0x00, 0x07, 0xA5 }, /* -30.5 */
	{ 0x00, 0x07, 0x37 }, /* -31.0 */
	{ 0x00, 0x06, 0xD0 }, /* -31.5 */
	{ 0x00, 0x06, 0x6E }, /* -32.0 */
	{ 0x00, 0x06, 0x12 }, /* -32.5 */
	{ 0x00, 0x05, 0xBB }, /* -33.0 */
	{ 0x00, 0x05, 0x69 }, /* -33.5 */
	{ 0x00, 0x05, 0x1C }, /* -34.0 */
	{ 0x00, 0x04, 0xD2 }, /* -34.5 */
	{ 0x00, 0x04, 0x8D }, /* -35.0 */
	{ 0x00, 0x04, 0x4C }, /* -35.5 */
	{ 0x00, 0x04, 0x0F }, /* -36.0 */
	{ 0x00, 0x03, 0xD5 }, /* -36.5 */
	{ 0x00, 0x03, 0x9E }, /* -37.0 */
	{ 0x00, 0x03, 0x6A }, /* -37.5 */
	{ 0x00, 0x03, 0x39 }, /* -38.0 */
	{ 0x00, 0x03, 0x0B }, /* -38.5 */
	{ 0x00, 0x02, 0xDF }, /* -39.0 */
	{ 0x00, 0x02, 0xB6 }, /* -39.5 */
	{ 0x00, 0x02, 0x8F }, /* -40.0 */
	{ 0x00, 0x02, 0x6B }, /* -40.5 */
	{ 0x00, 0x02, 0x48 }, /* -41.0 */
	{ 0x00, 0x02, 0x27 }, /* -41.5 */
	{ 0x00, 0x02, 0x09 }, /* -42.0 */
	{ 0x00, 0x01, 0xEB }, /* -42.5 */
	{ 0x00, 0x01, 0xD0 }, /* -43.0 */
	{ 0x00, 0x01, 0xB6 }, /* -43.5 */
	{ 0x00, 0x01, 0x9E }, /* -44.0 */
	{ 0x00, 0x01, 0x86 }, /* -44.5 */
	{ 0x00, 0x01, 0x71 }, /* -45.0 */
	{ 0x00, 0x01, 0x5C }, /* -45.5 */
	{ 0x00, 0x01, 0x48 }, /* -46.0 */
	{ 0x00, 0x01, 0x36 }, /* -46.5 */
	{ 0x00, 0x01, 0x25 }, /* -47.0 */
	{ 0x00, 0x01, 0x14 }, /* -47.5 */
	{ 0x00, 0x01, 0x05 }, /* -48.0 */
	{ 0x00, 0x00, 0xF6 }, /* -48.5 */
	{ 0x00, 0x00, 0xE9 }, /* -49.0 */
	{ 0x00, 0x00, 0xDC }, /* -49.5 */
	{ 0x00, 0x00, 0xCF }, /* -50.0 */
	{ 0x00, 0x00, 0xC4 }, /* -50.5 */
	{ 0x00, 0x00, 0xB9 }, /* -51.0 */
	{ 0x00, 0x00, 0xAE }, /* -51.5 */
	{ 0x00, 0x00, 0xA5 }, /* -52.0 */
	{ 0x00, 0x00, 0x9B }, /* -52.5 */
	{ 0x00, 0x00, 0x93 }, /* -53.0 */
	{ 0x00, 0x00, 0x8B }, /* -53.5 */
	{ 0x00, 0x00, 0x83 }, /* -54.0 */
	{ 0x00, 0x00, 0x7B }, /* -54.5 */
	{ 0x00, 0x00, 0x75 }, /* -55.0 */
	{ 0x00, 0x00, 0x6E }, /* -55.5 */
	{ 0x00, 0x00, 0x68 }, /* -56.0 */
	{ 0x00, 0x00, 0x62 }, /* -56.5 */
	{ 0x00, 0x00, 0x0 } /* Mute? */
};

/* TAS3001 registers */
#define DEQ_MCR		0x01	/* Main Control Register (1byte) */
#define DEQ_DRC		0x02	/* Dynamic Range Compression (2bytes) */
#define DEQ_VOLUME	0x04	/* Volume (6bytes) */
#define DEQ_TREBLE	0x05	/* Treble Control Register (1byte) */
#define DEQ_BASS	0x06	/* Bass Control Register (1byte) */
#define DEQ_MIXER1	0x07	/* Mixer 1 (3bytes) */
#define DEQ_MIXER2	0x08	/* Mixer 2 (3bytes) */
#define DEQ_LB0		0x0a	/* Left Biquad 0 (15bytes) */
#define DEQ_LB1		0x0b	/* Left Biquad 1 (15bytes) */
#define DEQ_LB2		0x0c	/* Left Biquad 2 (15bytes) */
#define DEQ_LB3		0x0d	/* Left Biquad 3 (15bytes) */
#define DEQ_LB4		0x0e	/* Left Biquad 4 (15bytes) */
#define DEQ_LB5		0x0f	/* Left Biquad 5 (15bytes) */
#define DEQ_RB0		0x13	/* Right Biquad 0 (15bytes) */
#define DEQ_RB1		0x14	/* Right Biquad 1 (15bytes) */
#define DEQ_RB2		0x15	/* Right Biquad 2 (15bytes) */
#define DEQ_RB3		0x16	/* Right Biquad 3 (15bytes) */
#define DEQ_RB4		0x17	/* Right Biquad 4 (15bytes) */
#define DEQ_RB5		0x18	/* Right Biquad 5 (15bytes) */

#define DEQ_MCR_FL	0x80	/* Fast load */
#define DEQ_MCR_SC	0x40	/* SCLK frequency */
#define  DEQ_MCR_SC_32	0x00	/*  32fs */
#define  DEQ_MCR_SC_64	0x40	/*  64fs */
#define DEQ_MCR_OM	0x30	/* Output serial port mode */
#define  DEQ_MCR_OM_L	0x00	/*  Left justified */
#define  DEQ_MCR_OM_R	0x10	/*  Right justified */
#define  DEQ_MCR_OM_I2S	0x20	/*  I2S */
#define DEQ_MCR_IM	0x0c	/* Input serial port mode */
#define  DEQ_MCR_IM_L	0x00	/*  Left justified */
#define  DEQ_MCR_IM_R	0x04	/*  Right justified */
#define  DEQ_MCR_IM_I2S	0x08	/*  I2S */
#define DEQ_MCR_W	0x03	/* Serial port word length */
#define  DEQ_MCR_W_16	0x00	/*  16 bit */
#define  DEQ_MCR_W_18	0x01	/*  18 bit */
#define  DEQ_MCR_W_20	0x02	/*  20 bit */

#define DEQ_DRC_CR	0xc0	/* Compression ratio */
#define  DEQ_DRC_CR_31	0xc0	/*  3:1 */
#define DEQ_DRC_EN	0x01	/* Enable DRC */

#define DEQ_MCR_I2S	(DEQ_MCR_OM_I2S | DEQ_MCR_IM_I2S)

struct tas3001_reg {
	u_char MCR[1];
	u_char DRC[2];
	u_char VOLUME[6];
	u_char TREBLE[1];
	u_char BASS[1];
	u_char MIXER1[3];
	u_char MIXER2[3];
	u_char LB0[15];
	u_char LB1[15];
	u_char LB2[15];
	u_char LB3[15];
	u_char LB4[15];
	u_char LB5[15];
	u_char RB0[15];
	u_char RB1[15];
	u_char RB2[15];
	u_char RB3[15];
	u_char RB4[15];
	u_char RB5[15];
};

int
tumbler_match(struct device *parent, void *match, void *aux)
{
	struct confargs *ca = aux;
	int soundbus, soundchip;
	char compat[32];

	if (strcmp(ca->ca_name, "i2s") != 0)
		return (0);

	if ((soundbus = OF_child(ca->ca_node)) == 0 ||
	    (soundchip = OF_child(soundbus)) == 0)
		return (0);

	bzero(compat, sizeof compat);
	OF_getprop(soundchip, "compatible", compat, sizeof compat);

	if (strcmp(compat, "tumbler") != 0)
		return (0);

	return (1);
}

void
tumbler_attach(struct device *parent, struct device *self, void *aux)
{
	struct tumbler_softc *sc = (struct tumbler_softc *)self;

	sc->sc_setvolume = tumbler_set_volume;
	sc->sc_setbass = tumbler_set_bass;
	sc->sc_settreble = tumbler_set_treble;

	i2s_attach(parent, sc, aux);
	config_defer(self, tumbler_defer);
}

void
tumbler_defer(struct device *dev)
{
	struct tumbler_softc *sc = (struct tumbler_softc *)dev;
	struct device *dv;

	TAILQ_FOREACH(dv, &alldevs, dv_list)
		if (strncmp(dv->dv_xname, "kiic", 4) == 0 &&
		    strncmp(dv->dv_parent->dv_xname, "macobio", 7) == 0)
			sc->sc_i2c = dv;
	if (sc->sc_i2c == NULL) {
		printf("%s: unable to find i2c\n", sc->sc_dev.dv_xname);
		return;
	}

	/* XXX If i2c has failed to attach, what should we do? */

	audio_attach_mi(&tumbler_hw_if, sc, &sc->sc_dev);

	tumbler_init(sc);
}

void
tumbler_set_volume(struct tumbler_softc *sc, int left, int right)
{
	u_char vol[6];
	int nentries = sizeof(tumbler_volumetab) / sizeof(tumbler_volumetab[0];
	int l, r;

	sc->sc_vol_l = left;
	sc->sc_vol_r = right;

	l = nentries - (left * nentries / 256);
	r = nentries - (right * nentries / 256);

	DPRINTF(" left %d vol %d %d, right %d vol %d %d\n",
		left, l, nentries,
		right, r, nentries);
	if (l >= nentries)
		l = nentries - 1;
	if (r >= nentries)
		r = nentries - 1;

	vol[0] = tumbler_volumetab[l].high;
	vol[1] = tumbler_volumetab[l].mid;
	vol[2] = tumbler_volumetab[l].low;
	vol[3] = tumbler_volumetab[r].high;
	vol[4] = tumbler_volumetab[r].mid;
	vol[5] = tumbler_volumetab[r].low;

	tas3001_write(sc, DEQ_VOLUME, vol);
}

void
tumbler_set_treble(struct tumbler_softc *sc, int value)
{
	uint8_t reg;

	if ((value >= 0) && (value <= 255) && (value != sc->sc_treble)) {
		reg = tumbler_trebletab[(value >> 3) + 2];
		if (tas3001_write(sc, DEQ_TREBLE, &reg) < 0)
			return;
		sc->sc_treble = value;
	}
}

void
tumbler_set_bass(struct tumbler_softc *sc, int value)
{
	uint8_t reg;

	if ((value >= 0) && (value <= 255) && (value != sc->sc_bass)) {
		reg = tumbler_basstab[(value >> 3) + 2];
		if (tas3001_write(sc, DEQ_BASS, &reg) < 0)
			return;
		sc->sc_bass = value;
	}
}

const struct tas3001_reg tas3001_initdata = {
	{ DEQ_MCR_SC_64 | DEQ_MCR_I2S | DEQ_MCR_W_20 },		/* MCR */
	{ DEQ_DRC_CR_31, 0xa0 },				/* DRC */
	{ 0x00, 0xd7, 0x66, 0x00, 0xd7, 0x66 },			/* VOLUME */
	{ 0x72 },						/* TREBLE */
	{ 0x3e },						/* BASS */
	{ 0x10, 0x00, 0x00 },					/* MIXER1 */
	{ 0x00, 0x00, 0x00 },					/* MIXER2 */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
	{ 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* BIQUAD */
};

const char tas3001_regsize[] = {
	0,					/* 0x00 */
	sizeof tas3001_initdata.MCR,		/* 0x01 */
	sizeof tas3001_initdata.DRC,		/* 0x02 */
	0,					/* 0x03 */
	sizeof tas3001_initdata.VOLUME,		/* 0x04 */
	sizeof tas3001_initdata.TREBLE,		/* 0x05 */
	sizeof tas3001_initdata.BASS,		/* 0x06 */
	sizeof tas3001_initdata.MIXER1,		/* 0x07 */
	sizeof tas3001_initdata.MIXER2,		/* 0x08 */
	0,					/* 0x09 */
	sizeof tas3001_initdata.LB0,		/* 0x0a */
	sizeof tas3001_initdata.LB1,		/* 0x0b */
	sizeof tas3001_initdata.LB2,		/* 0x0c */
	sizeof tas3001_initdata.LB3,		/* 0x0d */
	sizeof tas3001_initdata.LB4,		/* 0x0e */
	sizeof tas3001_initdata.LB5,		/* 0x0f */
	0,					/* 0x10 */
	0,					/* 0x11 */
	0,					/* 0x12 */
	sizeof tas3001_initdata.RB0,		/* 0x13 */
	sizeof tas3001_initdata.RB1,		/* 0x14 */
	sizeof tas3001_initdata.RB2,		/* 0x15 */
	sizeof tas3001_initdata.RB3,		/* 0x16 */
	sizeof tas3001_initdata.RB4,		/* 0x17 */
	sizeof tas3001_initdata.RB5		/* 0x18 */
};

#define DEQaddr 0x68

int
tas3001_write(struct tumbler_softc *sc, u_int reg, const void *data)
{
	int size;

	KASSERT(reg < sizeof tas3001_regsize);
	size = tas3001_regsize[reg];
	KASSERT(size > 0);

	if (kiic_write(sc->sc_i2c, DEQaddr, reg, data, size))
		return (-1);

	return (0);
}

#define DEQ_WRITE(sc, reg, addr) \
	if (tas3001_write(sc, reg, addr)) goto err

int
tas3001_init(struct tumbler_softc *sc)
{
	deq_reset(sc);

	/* Initialize TAS3001 registers. */
	DEQ_WRITE(sc, DEQ_LB0, tas3001_initdata.LB0);
	DEQ_WRITE(sc, DEQ_LB1, tas3001_initdata.LB1);
	DEQ_WRITE(sc, DEQ_LB2, tas3001_initdata.LB2);
	DEQ_WRITE(sc, DEQ_LB3, tas3001_initdata.LB3);
	DEQ_WRITE(sc, DEQ_LB4, tas3001_initdata.LB4);
	DEQ_WRITE(sc, DEQ_LB5, tas3001_initdata.LB5);
	DEQ_WRITE(sc, DEQ_RB0, tas3001_initdata.RB0);
	DEQ_WRITE(sc, DEQ_RB1, tas3001_initdata.RB1);
	DEQ_WRITE(sc, DEQ_RB1, tas3001_initdata.RB1);
	DEQ_WRITE(sc, DEQ_RB2, tas3001_initdata.RB2);
	DEQ_WRITE(sc, DEQ_RB3, tas3001_initdata.RB3);
	DEQ_WRITE(sc, DEQ_RB4, tas3001_initdata.RB4);
	DEQ_WRITE(sc, DEQ_MCR, tas3001_initdata.MCR);
	DEQ_WRITE(sc, DEQ_DRC, tas3001_initdata.DRC);
	DEQ_WRITE(sc, DEQ_VOLUME, tas3001_initdata.VOLUME);
	DEQ_WRITE(sc, DEQ_TREBLE, tas3001_initdata.TREBLE);
	DEQ_WRITE(sc, DEQ_BASS, tas3001_initdata.BASS);
	DEQ_WRITE(sc, DEQ_MIXER1, tas3001_initdata.MIXER1);
	DEQ_WRITE(sc, DEQ_MIXER2, tas3001_initdata.MIXER2);

	return (0);
err:
	printf("%s: tas3001_init: error\n", sc->sc_dev.dv_xname);
	return (-1);
}

void
tumbler_init(struct tumbler_softc *sc)
{

	/* "sample-rates" (44100, 48000) */
	i2s_set_rate(sc, 44100);

#if 1
	/* Enable I2C interrupts. */
#define IER 4
#define I2C_INT_DATA 0x01
#define I2C_INT_ADDR 0x02
#define I2C_INT_STOP 0x04
	kiic_writereg(sc->sc_i2c, IER,I2C_INT_DATA|I2C_INT_ADDR|I2C_INT_STOP);
#endif

	if (tas3001_init(sc))
		return;

	tumbler_set_volume(sc, 190, 190);
	tumbler_set_treble(sc, 128); /* 0 dB */
	tumbler_set_bass(sc, 128); /* 0 dB */
}

int
tumbler_getdev(void *h, struct audio_device *retp)
{
	*retp = tumbler_device;
	return (0);
}

void
tumbler_get_default_params(void *addr, int mode, struct audio_params *params)
{
	i2s_get_default_params(params);
}
