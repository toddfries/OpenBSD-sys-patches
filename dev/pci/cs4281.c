/*	$OpenBSD: cs4281.c,v 1.22 2008/10/25 22:30:43 jakemsr Exp $ */
/*	$Tera: cs4281.c,v 1.18 2000/12/27 14:24:45 tacha Exp $	*/

/*
 * Copyright (c) 2000 Tatoku Ogaito.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Tatoku Ogaito
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 */

/*
 * Cirrus Logic CS4281 driver.
 * Data sheets can be found
 * http://www.cirrus.com/pubs/4281.pdf?DocumentID=30
 * ftp://ftp.alsa-project.org/pub/manuals/cirrus/cs4281tm.pdf
 *
 * TODO:
 *   1: midi and FM support
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/fcntl.h>
#include <sys/device.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/cs4281reg.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/midi_if.h>
#include <dev/mulaw.h>
#include <dev/auconv.h>

#include <dev/ic/ac97.h>

#include <machine/bus.h>

#define CSCC_PCI_BA0 0x10
#define CSCC_PCI_BA1 0x14

struct cs4281_dma {
	bus_dmamap_t map;
	caddr_t addr;		/* real dma buffer */
	caddr_t dum;		/* dummy buffer for audio driver */
	bus_dma_segment_t segs[1];
	int nsegs;
	size_t size;
	struct cs4281_dma *next;
};
#define DMAADDR(p) ((p)->map->dm_segs[0].ds_addr)
#define BUFADDR(p)  ((void *)((p)->dum))
#define KERNADDR(p) ((void *)((p)->addr))

/*
 * Software state
 */
struct cs4281_softc {
	struct device		sc_dev;

	pci_intr_handle_t	*sc_ih;

        /* I/O (BA0) */
	bus_space_tag_t		ba0t;
	bus_space_handle_t	ba0h;

	/* BA1 */
	bus_space_tag_t		ba1t;
	bus_space_handle_t	ba1h;

	/* DMA */
	bus_dma_tag_t		sc_dmatag;
	struct cs4281_dma	*sc_dmas;
	size_t dma_size;
	size_t dma_align;

	int	hw_blocksize;

        /* playback */
	void	(*sc_pintr)(void *);	/* dma completion intr handler */
	void	*sc_parg;		/* arg for sc_intr() */
	char	*sc_ps, *sc_pe, *sc_pn;
	int	sc_pcount;
	int	sc_pi;
	struct cs4281_dma *sc_pdma;
	char	*sc_pbuf;
	int	(*halt_output)(void *);
#ifdef DIAGNOSTIC
        char	sc_prun;
#endif

	/* capturing */
	void	(*sc_rintr)(void *);	/* dma completion intr handler */
	void	*sc_rarg;		/* arg for sc_intr() */
	char	*sc_rs, *sc_re, *sc_rn;
	int	sc_rcount;
	int	sc_ri;
	struct	cs4281_dma *sc_rdma;
	char	*sc_rbuf;
	int	sc_rparam;		/* record format */
	int	(*halt_input)(void *);
#ifdef DIAGNOSTIC
        char	sc_rrun;
#endif

#if NMIDI > 0
        void	(*sc_iintr)(void *, int);	/* midi input ready handler */
        void	(*sc_ointr)(void *);		/* midi output ready handler */
        void	*sc_arg;
#endif

	/* AC97 CODEC */
	struct ac97_codec_if *codec_if;
	struct ac97_host_if host_if;

        /* Power Management */
        char	sc_suspend;
        void	*sc_powerhook;		/* Power hook */
	u_int16_t ac97_reg[CS4281_SAVE_REG_MAX + 1];   /* Save ac97 registers */
};

#define BA0READ4(sc, r) bus_space_read_4((sc)->ba0t, (sc)->ba0h, (r))
#define BA0WRITE4(sc, r, x) bus_space_write_4((sc)->ba0t, (sc)->ba0h, (r), (x))

#if defined(ENABLE_SECONDARY_CODEC)
#define MAX_CHANNELS  (4)
#define MAX_FIFO_SIZE 32 /* 128/4 channels */
#else
#define MAX_CHANNELS  (2)
#define MAX_FIFO_SIZE 64 /* 128/2 channels */
#endif

int cs4281_match(struct device *, void *, void *);
void cs4281_attach(struct device *, struct device *, void *);
int cs4281_intr(void *);
int cs4281_query_encoding(void *, struct audio_encoding *);
int cs4281_set_params(void *, int, int, struct audio_params *,
				     struct audio_params *);
int cs4281_halt_output(void *);
int cs4281_halt_input(void *);
int cs4281_getdev(void *, struct audio_device *);
int cs4281_trigger_output(void *, void *, void *, int, void (*)(void *),
			  void *, struct audio_params *);
int cs4281_trigger_input(void *, void *, void *, int, void (*)(void *),
			 void *, struct audio_params *);
u_int8_t cs4281_sr2regval(int);
void cs4281_set_dac_rate(struct cs4281_softc *, int);
void cs4281_set_adc_rate(struct cs4281_softc *, int);
int cs4281_init(struct cs4281_softc *);

int cs4281_open(void *, int);
void cs4281_close(void *);
int cs4281_round_blocksize(void *, int);
int cs4281_get_props(void *);
int cs4281_attach_codec(void *, struct ac97_codec_if *);
int cs4281_read_codec(void *, u_int8_t , u_int16_t *);
int cs4281_write_codec(void *, u_int8_t, u_int16_t);
void cs4281_reset_codec(void *);

void cs4281_power(int, void *);

int cs4281_mixer_set_port(void *, mixer_ctrl_t *);
int cs4281_mixer_get_port(void *, mixer_ctrl_t *);
int cs4281_query_devinfo(void *, mixer_devinfo_t *);
void *cs4281_malloc(void *, int, size_t, int, int);
size_t cs4281_round_buffersize(void *, int, size_t);
void cs4281_free(void *, void *, int);
paddr_t cs4281_mappage(void *, void *, off_t, int);

int cs4281_allocmem(struct cs4281_softc *, size_t, int, int,
				     struct cs4281_dma *);
int cs4281_src_wait(struct cs4281_softc *);

#if defined(CS4281_DEBUG)
#undef DPRINTF
#undef DPRINTFN
#define DPRINTF(x)	    if (cs4281_debug) printf x
#define DPRINTFN(n,x)	    if (cs4281_debug>(n)) printf x
int cs4281_debug = 5;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

struct audio_hw_if cs4281_hw_if = {
	cs4281_open,
	cs4281_close,
	NULL,
	cs4281_query_encoding,
	cs4281_set_params,
	cs4281_round_blocksize,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	cs4281_halt_output,
	cs4281_halt_input,
	NULL,
	cs4281_getdev,
	NULL,
	cs4281_mixer_set_port,
	cs4281_mixer_get_port,
	cs4281_query_devinfo,
	cs4281_malloc,
	cs4281_free,
	cs4281_round_buffersize,
	NULL, /* cs4281_mappage, */
	cs4281_get_props,
	cs4281_trigger_output,
	cs4281_trigger_input,
	NULL
};

#if NMIDI > 0
/* Midi Interface */
void cs4281_midi_close(void *);
void cs4281_midi_getinfo(void *, struct midi_info *);
int cs4281_midi_open(void *, int, void (*)(void *, int),
		     void (*)(void *), void *);
int cs4281_midi_output(void *, int);

struct midi_hw_if cs4281_midi_hw_if = {
	cs4281_midi_open,
	cs4281_midi_close,
	cs4281_midi_output,
	cs4281_midi_getinfo,
	0,
};
#endif

struct cfattach clct_ca = {
	sizeof(struct cs4281_softc), cs4281_match, cs4281_attach
};

struct cfdriver clct_cd = {
	NULL, "clct", DV_DULL
};

struct audio_device cs4281_device = {
	"CS4281",
	"",
	"cs4281"
};


int
cs4281_match(parent, match, aux)
	struct device *parent;
	void *match;
	void *aux;
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	
	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_CIRRUS ||
	    PCI_PRODUCT(pa->pa_id) != PCI_PRODUCT_CIRRUS_CS4281)
		return (0);

	return (1);
}

void
cs4281_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct cs4281_softc *sc = (struct cs4281_softc *)self;
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	char const *intrstr;
	pci_intr_handle_t ih;

	/* Map I/O register */
	if (pci_mapreg_map(pa, CSCC_PCI_BA0,
	    PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT, 0, &sc->ba0t,
	    &sc->ba0h, NULL, NULL, 0)) {
		printf("%s: can't map BA0 space\n", sc->sc_dev.dv_xname);
		return;
	}
	if (pci_mapreg_map(pa, CSCC_PCI_BA1,
	    PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT, 0, &sc->ba1t,
	    &sc->ba1h, NULL, NULL, 0)) {
		printf("%s: can't map BA1 space\n", sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_dmatag = pa->pa_dmat;

	/*
	 * Set Power State D0.
	 * Without doing this, 0xffffffff is read from all registers after
	 * using Windows and rebooting into OpenBSD.
	 * On my IBM ThinkPad X20, it is set to D3 after using Windows2000.
	 */
	pci_set_powerstate(pc, pa->pa_tag, PCI_PMCSR_STATE_D0);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);

	sc->sc_ih = pci_intr_establish(pc, ih, IPL_AUDIO, cs4281_intr, sc,
	    sc->sc_dev.dv_xname);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf(" %s\n", intrstr);

	/*
	 * Sound System start-up
	 */
	if (cs4281_init(sc) != 0)
		return;

	sc->halt_input  = cs4281_halt_input;
	sc->halt_output = cs4281_halt_output;

	sc->dma_size     = CS4281_BUFFER_SIZE / MAX_CHANNELS;
	sc->dma_align    = 0x10;
	sc->hw_blocksize = sc->dma_size / 2;
	
	/* AC 97 attachment */
	sc->host_if.arg = sc;
	sc->host_if.attach = cs4281_attach_codec;
	sc->host_if.read   = cs4281_read_codec;
	sc->host_if.write  = cs4281_write_codec;
	sc->host_if.reset  = cs4281_reset_codec;
	if (ac97_attach(&sc->host_if) != 0) {
		printf("%s: ac97_attach failed\n", sc->sc_dev.dv_xname);
		return;
	}
	audio_attach_mi(&cs4281_hw_if, sc, &sc->sc_dev);

#if NMIDI > 0
	midi_attach_mi(&cs4281_midi_hw_if, sc, &sc->sc_dev);
#endif

	sc->sc_suspend = PWR_RESUME;
	sc->sc_powerhook = powerhook_establish(cs4281_power, sc);
}


int
cs4281_intr(p)
	void *p;
{
	struct cs4281_softc *sc = p;
	u_int32_t intr, val;
	char *empty_dma;
	
	intr = BA0READ4(sc, CS4281_HISR);
	if (!(intr & (HISR_DMA0 | HISR_DMA1 | HISR_MIDI))) {
		BA0WRITE4(sc, CS4281_HICR, HICR_IEV | HICR_CHGM);
		return (0);
	}
	DPRINTF(("cs4281_intr:"));

	if (intr & HISR_DMA0)
		val = BA0READ4(sc, CS4281_HDSR0); /* clear intr condition */
	if (intr & HISR_DMA1)
		val = BA0READ4(sc, CS4281_HDSR1); /* clear intr condition */
	BA0WRITE4(sc, CS4281_HICR, HICR_IEV | HICR_CHGM);

	/* Playback Interrupt */
	if (intr & HISR_DMA0) {
		DPRINTF((" PB DMA 0x%x(%d)", (int)BA0READ4(sc, CS4281_DCA0),
			 (int)BA0READ4(sc, CS4281_DCC0)));
		if (sc->sc_pintr) {
			if ((sc->sc_pi%sc->sc_pcount) == 0)
				sc->sc_pintr(sc->sc_parg);
		} else {
			printf("unexpected play intr\n");
		}
		/* copy buffer */
		++sc->sc_pi;
		empty_dma = sc->sc_pdma->addr;
		if (sc->sc_pi&1)
			empty_dma += sc->hw_blocksize;
		memcpy(empty_dma, sc->sc_pn, sc->hw_blocksize);
		sc->sc_pn += sc->hw_blocksize;
		if (sc->sc_pn >= sc->sc_pe)
			sc->sc_pn = sc->sc_ps;
	}
	if (intr & HISR_DMA1) {
		val = BA0READ4(sc, CS4281_HDSR1);
		/* copy from dma */
		DPRINTF((" CP DMA 0x%x(%d)", (int)BA0READ4(sc, CS4281_DCA1),
			 (int)BA0READ4(sc, CS4281_DCC1)));
		++sc->sc_ri;
		empty_dma = sc->sc_rdma->addr;
		if ((sc->sc_ri & 1) == 0)
			empty_dma += sc->hw_blocksize;
		memcpy(sc->sc_rn, empty_dma, sc->hw_blocksize);
		if (sc->sc_rn >= sc->sc_re)
			sc->sc_rn = sc->sc_rs;
		if (sc->sc_rintr) {
			if ((sc->sc_ri % sc->sc_rcount) == 0)
				sc->sc_rintr(sc->sc_rarg);
		} else {
			printf("unexpected record intr\n");
		}
	}
	DPRINTF(("\n"));
	return (1);
}

int
cs4281_query_encoding(addr, fp)
	void *addr;
	struct audio_encoding *fp;
{
	switch (fp->index) {
	case 0:
		strlcpy(fp->name, AudioEulinear, sizeof fp->name);
		fp->encoding = AUDIO_ENCODING_ULINEAR;
		fp->precision = 8;
		fp->flags = 0;
		break;
	case 1:
		strlcpy(fp->name, AudioEmulaw, sizeof fp->name);
		fp->encoding = AUDIO_ENCODING_ULAW;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;
	case 2:
		strlcpy(fp->name, AudioEalaw, sizeof fp->name);
		fp->encoding = AUDIO_ENCODING_ALAW;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;
	case 3:
		strlcpy(fp->name, AudioEslinear, sizeof fp->name);
		fp->encoding = AUDIO_ENCODING_SLINEAR;
		fp->precision = 8;
		fp->flags = 0;
		break;
	case 4:
		strlcpy(fp->name, AudioEslinear_le, sizeof fp->name);
		fp->encoding = AUDIO_ENCODING_SLINEAR_LE;
		fp->precision = 16;
		fp->flags = 0;
		break;
	case 5:
		strlcpy(fp->name, AudioEulinear_le, sizeof fp->name);
		fp->encoding = AUDIO_ENCODING_ULINEAR_LE;
		fp->precision = 16;
		fp->flags = 0;
		break;
	case 6:
		strlcpy(fp->name, AudioEslinear_be, sizeof fp->name);
		fp->encoding = AUDIO_ENCODING_SLINEAR_BE;
		fp->precision = 16;
		fp->flags = 0;
		break;
	case 7:
		strlcpy(fp->name, AudioEulinear_be, sizeof fp->name);
		fp->encoding = AUDIO_ENCODING_ULINEAR_BE;
		fp->precision = 16;
		fp->flags = 0;
		break;
	default:
		return EINVAL;
	}
	return (0);
}

int
cs4281_set_params(addr, setmode, usemode, play, rec)
	void *addr;
	int setmode, usemode;
	struct audio_params *play, *rec;
{
	struct cs4281_softc *sc = addr;
	struct audio_params *p;
	int mode;

	for (mode = AUMODE_RECORD; mode != -1;
	    mode = mode == AUMODE_RECORD ? AUMODE_PLAY : -1) {
		if ((setmode & mode) == 0)
			continue;
		
		p = mode == AUMODE_PLAY ? play : rec;
		
		if (p == play) {
			DPRINTFN(5,("play: samp=%ld precision=%d channels=%d\n",
				p->sample_rate, p->precision, p->channels));
		} else {
			DPRINTFN(5,("rec: samp=%ld precision=%d channels=%d\n",
				p->sample_rate, p->precision, p->channels));
		}
		if (p->sample_rate < 6023)
			p->sample_rate = 6023;
		if (p->sample_rate > 48000)
			p->sample_rate = 48000;
		if (p->precision > 16)
			p->precision = 16;
		if (p->channels > 2)
			p->channels = 2;
		p->factor = 1;
		p->sw_code = 0;

		switch (p->encoding) {
		case AUDIO_ENCODING_SLINEAR_BE:
			break;
		case AUDIO_ENCODING_SLINEAR_LE:
			break;
		case AUDIO_ENCODING_ULINEAR_BE:
			break;
		case AUDIO_ENCODING_ULINEAR_LE:
			break;
		case AUDIO_ENCODING_ULAW:
			if (mode == AUMODE_PLAY) {
				p->sw_code = mulaw_to_slinear8;
			} else {
				p->sw_code = slinear8_to_mulaw;
			}
			break;
		case AUDIO_ENCODING_ALAW:
			if (mode == AUMODE_PLAY) {
				p->sw_code = alaw_to_slinear8;
			} else {
				p->sw_code = slinear8_to_alaw;
			}
			break;
		default:
			return (EINVAL);
		}
	}

	/* set sample rate */
	cs4281_set_dac_rate(sc, play->sample_rate);
	cs4281_set_adc_rate(sc, rec->sample_rate);
	return (0);
}

int
cs4281_halt_output(addr)
	void *addr;
{
	struct cs4281_softc *sc = addr;
	
	BA0WRITE4(sc, CS4281_DCR0, BA0READ4(sc, CS4281_DCR0) | DCRn_MSK);
#ifdef DIAGNOSTIC
	sc->sc_prun = 0;
#endif
	return (0);
}

int
cs4281_halt_input(addr)
	void *addr;
{
	struct cs4281_softc *sc = addr;

	BA0WRITE4(sc, CS4281_DCR1, BA0READ4(sc, CS4281_DCR1) | DCRn_MSK);
#ifdef DIAGNOSTIC
	sc->sc_rrun = 0;
#endif
	return (0);
}

/* trivial */
int
cs4281_getdev(addr, retp)
     void *addr;
     struct audio_device *retp;
{
	*retp = cs4281_device;
	return (0);
}


int
cs4281_trigger_output(addr, start, end, blksize, intr, arg, param)
	void *addr;
	void *start, *end;
	int blksize;
	void (*intr)(void *);
	void *arg;
	struct audio_params *param;
{
	struct cs4281_softc *sc = addr;
	u_int32_t fmt=0;
	struct cs4281_dma *p;
	int dma_count;

#ifdef DIAGNOSTIC
	if (sc->sc_prun)
		printf("cs4281_trigger_output: already running\n");
	sc->sc_prun = 1;
#endif

	DPRINTF(("cs4281_trigger_output: sc=%p start=%p end=%p "
		 "blksize=%d intr=%p(%p)\n", addr, start, end, blksize, intr, arg));
	sc->sc_pintr = intr;
	sc->sc_parg  = arg;

	/* stop playback DMA */
	BA0WRITE4(sc, CS4281_DCR0, BA0READ4(sc, CS4281_DCR0) | DCRn_MSK);

	DPRINTF(("param: precision=%d  factor=%d channels=%d encoding=%d\n",
	       param->precision, param->factor, param->channels,
	       param->encoding));
	for (p = sc->sc_dmas; p != NULL && BUFADDR(p) != start; p = p->next)
		;
	if (p == NULL) {
		printf("cs4281_trigger_output: bad addr %p\n", start);
		return (EINVAL);
	}

	sc->sc_pcount = blksize / sc->hw_blocksize;
	sc->sc_ps = (char *)start;
	sc->sc_pe = (char *)end;
	sc->sc_pdma = p;
	sc->sc_pbuf = KERNADDR(p);
	sc->sc_pi = 0;
	sc->sc_pn = sc->sc_ps;
	if (blksize >= sc->dma_size) {
		sc->sc_pn = sc->sc_ps + sc->dma_size;
		memcpy(sc->sc_pbuf, start, sc->dma_size);
		++sc->sc_pi;
	} else {
		sc->sc_pn = sc->sc_ps + sc->hw_blocksize;
		memcpy(sc->sc_pbuf, start, sc->hw_blocksize);
	}

	dma_count = sc->dma_size;
	if (param->precision * param->factor != 8)
		dma_count /= 2;   /* 16 bit */
	if (param->channels > 1)
		dma_count /= 2;   /* Stereo */

	DPRINTF(("cs4281_trigger_output: DMAADDR(p)=0x%x count=%d\n",
		 (int)DMAADDR(p), dma_count));
	BA0WRITE4(sc, CS4281_DBA0, DMAADDR(p));
	BA0WRITE4(sc, CS4281_DBC0, dma_count-1);

	/* set playback format */
	fmt = BA0READ4(sc, CS4281_DMR0) & ~DMRn_FMTMSK;
	if (param->precision * param->factor == 8)
		fmt |= DMRn_SIZE8;
	if (param->channels == 1)
		fmt |= DMRn_MONO;
	if (param->encoding == AUDIO_ENCODING_ULINEAR_BE ||
	    param->encoding == AUDIO_ENCODING_SLINEAR_BE)
		fmt |= DMRn_BEND;
	if (param->encoding == AUDIO_ENCODING_ULINEAR_BE ||
	    param->encoding == AUDIO_ENCODING_ULINEAR_LE)
		fmt |= DMRn_USIGN;
	BA0WRITE4(sc, CS4281_DMR0, fmt);

	/* set sample rate */
	cs4281_set_dac_rate(sc, param->sample_rate);

	/* start DMA */
	BA0WRITE4(sc, CS4281_DCR0, BA0READ4(sc, CS4281_DCR0) & ~DCRn_MSK);
	/* Enable interrupts */
	BA0WRITE4(sc, CS4281_HICR, HICR_IEV | HICR_CHGM);

	BA0WRITE4(sc, CS4281_PPRVC, 7);
	BA0WRITE4(sc, CS4281_PPLVC, 7);

	DPRINTF(("HICR =0x%08x(expected 0x00000001)\n", BA0READ4(sc, CS4281_HICR)));
	DPRINTF(("HIMR =0x%08x(expected 0x00f0fc3f)\n", BA0READ4(sc, CS4281_HIMR)));
	DPRINTF(("DMR0 =0x%08x(expected 0x2???0018)\n", BA0READ4(sc, CS4281_DMR0)));
	DPRINTF(("DCR0 =0x%08x(expected 0x00030000)\n", BA0READ4(sc, CS4281_DCR0)));
	DPRINTF(("FCR0 =0x%08x(expected 0x81000f00)\n", BA0READ4(sc, CS4281_FCR0)));
	DPRINTF(("DACSR=0x%08x(expected 1 for 44kHz 5 for 8kHz)\n",
		 BA0READ4(sc, CS4281_DACSR)));
	DPRINTF(("SRCSA=0x%08x(expected 0x0b0a0100)\n", BA0READ4(sc, CS4281_SRCSA)));
	DPRINTF(("SSPM&SSPM_PSRCEN =0x%08x(expected 0x00000010)\n",
		 BA0READ4(sc, CS4281_SSPM) & SSPM_PSRCEN));

	return (0);
}

int
cs4281_trigger_input(addr, start, end, blksize, intr, arg, param)
	void *addr;
	void *start, *end;
	int blksize;
	void (*intr)(void *);
	void *arg;
	struct audio_params *param;
{
	struct cs4281_softc *sc = addr;
	struct cs4281_dma *p;
	u_int32_t fmt=0;
	int dma_count;

	printf("cs4281_trigger_input: not implemented yet\n");
#ifdef DIAGNOSTIC
	if (sc->sc_rrun)
		printf("cs4281_trigger_input: already running\n");
	sc->sc_rrun = 1;
#endif
	DPRINTF(("cs4281_trigger_input: sc=%p start=%p end=%p "
	    "blksize=%d intr=%p(%p)\n", addr, start, end, blksize, intr, arg));
	sc->sc_rintr = intr;
	sc->sc_rarg  = arg;

	/* stop recording DMA */
	BA0WRITE4(sc, CS4281_DCR1, BA0READ4(sc, CS4281_DCR1) | DCRn_MSK);

	for (p = sc->sc_dmas; p && BUFADDR(p) != start; p = p->next)
		;
	if (!p) {
		printf("cs4281_trigger_input: bad addr %p\n", start);
		return (EINVAL);
	}

	sc->sc_rcount = blksize / sc->hw_blocksize;
	sc->sc_rs = (char *)start;
	sc->sc_re = (char *)end;
	sc->sc_rdma = p;
	sc->sc_rbuf = KERNADDR(p);
	sc->sc_ri = 0;
	sc->sc_rn = sc->sc_rs;

	dma_count = sc->dma_size;
	if (param->precision * param->factor == 8)
		dma_count /= 2;
	if (param->channels > 1)
		dma_count /= 2;

	DPRINTF(("cs4281_trigger_input: DMAADDR(p)=0x%x count=%d\n",
		 (int)DMAADDR(p), dma_count));
	BA0WRITE4(sc, CS4281_DBA1, DMAADDR(p));
	BA0WRITE4(sc, CS4281_DBC1, dma_count-1);

	/* set recording format */
	fmt = BA0READ4(sc, CS4281_DMR1) & ~DMRn_FMTMSK;
	if (param->precision * param->factor == 8)
		fmt |= DMRn_SIZE8;
	if (param->channels == 1)
		fmt |= DMRn_MONO;
	if (param->encoding == AUDIO_ENCODING_ULINEAR_BE ||
	    param->encoding == AUDIO_ENCODING_SLINEAR_BE)
		fmt |= DMRn_BEND;
	if (param->encoding == AUDIO_ENCODING_ULINEAR_BE ||
	    param->encoding == AUDIO_ENCODING_ULINEAR_LE)
		fmt |= DMRn_USIGN;
	BA0WRITE4(sc, CS4281_DMR1, fmt);

	/* set sample rate */
	cs4281_set_adc_rate(sc, param->sample_rate);

	/* Start DMA */
	BA0WRITE4(sc, CS4281_DCR1, BA0READ4(sc, CS4281_DCR1) & ~DCRn_MSK);
	/* Enable interrupts */
	BA0WRITE4(sc, CS4281_HICR, HICR_IEV | HICR_CHGM);

	DPRINTF(("HICR=0x%08x\n", BA0READ4(sc, CS4281_HICR)));
	DPRINTF(("HIMR=0x%08x\n", BA0READ4(sc, CS4281_HIMR)));
	DPRINTF(("DMR1=0x%08x\n", BA0READ4(sc, CS4281_DMR1)));
	DPRINTF(("DCR1=0x%08x\n", BA0READ4(sc, CS4281_DCR1)));

	return (0);
}

/* convert sample rate to register value */
u_int8_t
cs4281_sr2regval(rate)
     int rate;
{
	u_int8_t retval;

	/* We don't have to change here. but anyway ... */
	if (rate > 48000)
		rate = 48000;
	if (rate < 6023)
		rate = 6023;

	switch (rate) {
	case 8000:
		retval = 5;
		break;
	case 11025:
		retval = 4;
		break;
	case 16000:
		retval = 3;
		break;
	case 22050:
		retval = 2;
		break;
	case 44100:
		retval = 1;
		break;
	case 48000:
		retval = 0;
		break;
	default:
		retval = 1536000/rate; /* == 24576000/(rate*16) */
	}
	return (retval);
}

	
void
cs4281_set_dac_rate(sc, rate)
	struct cs4281_softc *sc;
	int rate;
{
	BA0WRITE4(sc, CS4281_DACSR, cs4281_sr2regval(rate));
}

void
cs4281_set_adc_rate(sc, rate)
	struct cs4281_softc *sc;
	int rate;
{
	BA0WRITE4(sc, CS4281_ADCSR, cs4281_sr2regval(rate));
}

int
cs4281_init(sc)
     struct cs4281_softc *sc;
{
	int n;
	u_int16_t data;
	u_int32_t dat32;

	/* set "Configuration Write Protect" register to
	 * 0x4281 to allow to write */
	BA0WRITE4(sc, CS4281_CWPR, 0x4281);

	/*
	 * Unset "Full Power-Down bit of Extended PCI Power Management
	 * Control" register to release the reset state.
	 */
	dat32 = BA0READ4(sc, CS4281_EPPMC);
	if (dat32 & EPPMC_FPDN)
		BA0WRITE4(sc, CS4281_EPPMC, dat32 & ~EPPMC_FPDN);

	/* Start PLL out in known state */
	BA0WRITE4(sc, CS4281_CLKCR1, 0);
	/* Start serial ports out in known state */
	BA0WRITE4(sc, CS4281_SERMC, 0);
	
	/* Reset codec */
	BA0WRITE4(sc, CS4281_ACCTL, 0);
	delay(50);	/* delay 50us */

	BA0WRITE4(sc, CS4281_SPMC, 0);
	delay(100);	/* delay 100us */
	BA0WRITE4(sc, CS4281_SPMC, SPMC_RSTN);
#if defined(ENABLE_SECONDARY_CODEC)
	BA0WRITE4(sc, CS4281_SPMC, SPMC_RSTN | SPCM_ASDIN2E);
	BA0WRITE4(sc, CS4281_SERMC, SERMC_TCID);
#endif
	delay(50000);   /* XXX: delay 50ms */

	/* Turn on Sound System clocks based on ABITCLK */
	BA0WRITE4(sc, CS4281_CLKCR1, CLKCR1_DLLP);
	delay(50000);   /* XXX: delay 50ms */
	BA0WRITE4(sc, CS4281_CLKCR1, CLKCR1_SWCE | CLKCR1_DLLP);

	/* Set enables for sections that are needed in the SSPM registers */
	BA0WRITE4(sc, CS4281_SSPM,
		  SSPM_MIXEN |		/* Mixer */
		  SSPM_CSRCEN |		/* Capture SRC */
		  SSPM_PSRCEN |		/* Playback SRC */
		  SSPM_JSEN |		/* Joystick */
		  SSPM_ACLEN |		/* AC LINK */
		  SSPM_FMEN		/* FM */
		  );

	/* Wait for clock stabilization */
	n = 0;
	while ((BA0READ4(sc, CS4281_CLKCR1)& (CLKCR1_DLLRDY | CLKCR1_CLKON))
	    != (CLKCR1_DLLRDY | CLKCR1_CLKON)) {
		delay(100);
		if (++n > 1000)
			return (-1);
	}

	/* Enable ASYNC generation */
	BA0WRITE4(sc, CS4281_ACCTL, ACCTL_ESYN);

	/* Wait for Codec ready. Linux driver wait 50ms here */
	n = 0;
	while((BA0READ4(sc, CS4281_ACSTS) & ACSTS_CRDY) == 0) {
		delay(100);
		if (++n > 1000)
			return (-1);
	}

#if defined(ENABLE_SECONDARY_CODEC)
	/* secondary codec ready*/
	n = 0;
	while((BA0READ4(sc, CS4281_ACSTS2) & ACSTS2_CRDY2) == 0) {
		delay(100);
		if (++n > 1000)
			return (-1);
	}
#endif

	/* Set the serial timing configuration */
	/* XXX: undocumented but the Linux driver do this */
	BA0WRITE4(sc, CS4281_SERMC, SERMC_PTCAC97);
	
	/* Wait for Codec ready signal */
	n = 0;
	do {
		delay(1000);
		if (++n > 1000) {
			printf("%s: Timeout waiting for Codec ready\n",
			       sc->sc_dev.dv_xname);
			return -1;
		}
		dat32 = BA0READ4(sc, CS4281_ACSTS) & ACSTS_CRDY;
	} while (dat32 == 0);

	/* Enable Valid Frame output on ASDOUT */
	BA0WRITE4(sc, CS4281_ACCTL, ACCTL_ESYN | ACCTL_VFRM);
	
	/* Wait until Codec Calibration is finished. Codec register 26h */
	n = 0;
	do {
		delay(1);
		if (++n > 1000) {
			printf("%s: Timeout waiting for Codec calibration\n",
			       sc->sc_dev.dv_xname);
			return -1;
		}
		cs4281_read_codec(sc, AC97_REG_POWER, &data);
	} while ((data & 0x0f) != 0x0f);

	/* Set the serial timing configuration again */
	/* XXX: undocumented but the Linux driver do this */
	BA0WRITE4(sc, CS4281_SERMC, SERMC_PTCAC97);

	/* Wait until we've sampled input slots 3 & 4 as valid */
	n = 0;
	do {
		delay(1000);
		if (++n > 1000) {
			printf("%s: Timeout waiting for sampled input slots as valid\n",
			       sc->sc_dev.dv_xname);
			return -1;
		}
		dat32 = BA0READ4(sc, CS4281_ACISV) & (ACISV_ISV3 | ACISV_ISV4);
	} while (dat32 != (ACISV_ISV3 | ACISV_ISV4));
	
	/* Start digital data transfer of audio data to the codec */
	BA0WRITE4(sc, CS4281_ACOSV, (ACOSV_SLV3 | ACOSV_SLV4));
	
	cs4281_write_codec(sc, AC97_REG_HEADPHONE_VOLUME, 0);
	cs4281_write_codec(sc, AC97_REG_MASTER_VOLUME, 0);
	
	/* Power on the DAC */
	cs4281_read_codec(sc, AC97_REG_POWER, &data);
	cs4281_write_codec(sc, AC97_REG_POWER, data &= 0xfdff);

	/* Wait until we sample a DAC ready state.
	 * Not documented, but Linux driver does.
	 */
	for (n = 0; n < 32; ++n) {
		delay(1000);
		cs4281_read_codec(sc, AC97_REG_POWER, &data);
		if (data & 0x02)
			break;
	}
	
	/* Power on the ADC */
	cs4281_read_codec(sc, AC97_REG_POWER, &data);
	cs4281_write_codec(sc, AC97_REG_POWER, data &= 0xfeff);

	/* Wait until we sample ADC ready state.
	 * Not documented, but Linux driver does.
	 */
	for (n = 0; n < 32; ++n) {
		delay(1000);
		cs4281_read_codec(sc, AC97_REG_POWER, &data);
		if (data & 0x01)
			break;
	}
	
#if 0
	/* Initialize SSCR register features */
	/* XXX: hardware volume setting */
	BA0WRITE4(sc, CS4281_SSCR, ~SSCR_HVC); /* disable HW volume setting */
#endif

	/* disable Sound Blaster Pro emulation */
	/* XXX:
	 * Cannot set since the documents does not describe which bit is
	 * correspond to SSCR_SB. Since the reset value of SSCR is 0,
	 * we can ignore it.*/
#if 0
	BA0WRITE4(sc, CS4281_SSCR, SSCR_SB);
#endif

	/* map AC97 PCM playback to DMA Channel 0 */
	/* Reset FEN bit to setup first */
	BA0WRITE4(sc, CS4281_FCR0, (BA0READ4(sc,CS4281_FCR0) & ~FCRn_FEN));
	/*
	 *| RS[4:0]/|        |
	 *| LS[4:0] |  AC97  | Slot Function
	 *|---------+--------+--------------------
	 *|     0   |    3   | Left PCM Playback
	 *|     1   |    4   | Right PCM Playback
	 *|     2   |    5   | Phone Line 1 DAC
	 *|     3   |    6   | Center PCM Playback
	 *....
	 *  quoted from Table 29(p109)
	 */
	dat32 = 0x01 << 24 |   /* RS[4:0] =  1 see above */
		0x00 << 16 |   /* LS[4:0] =  0 see above */
		0x0f <<  8 |   /* SZ[6:0] = 15 size of buffer */
		0x00 <<  0 ;   /* OF[6:0] =  0 offset */
	BA0WRITE4(sc, CS4281_FCR0, dat32);
	BA0WRITE4(sc, CS4281_FCR0, dat32 | FCRn_FEN);

	/* map AC97 PCM record to DMA Channel 1 */
	/* Reset FEN bit to setup first */
	BA0WRITE4(sc, CS4281_FCR1, (BA0READ4(sc,CS4281_FCR1) & ~FCRn_FEN));
	/*
	 *| RS[4:0]/|
	 *| LS[4:0] | AC97 | Slot Function
	 *|---------+------+-------------------
	 *|   10    |   3  | Left PCM Record
	 *|   11    |   4  | Right PCM Record
	 *|   12    |   5  | Phone Line 1 ADC
	 *|   13    |   6  | Mic ADC
	 *....
	 * quoted from Table 30(p109)
	 */
	dat32 = 0x0b << 24 |    /* RS[4:0] = 11 See above */
		0x0a << 16 |    /* LS[4:0] = 10 See above */
		0x0f <<  8 |    /* SZ[6:0] = 15 Size of buffer */
		0x10 <<  0 ;    /* OF[6:0] = 16 offset */

	/* XXX: I cannot understand why FCRn_PSH is needed here. */
	BA0WRITE4(sc, CS4281_FCR1, dat32 | FCRn_PSH);
	BA0WRITE4(sc, CS4281_FCR1, dat32 | FCRn_FEN);

#if 0
	/* Disable DMA Channel 2, 3 */
	BA0WRITE4(sc, CS4281_FCR2, (BA0READ4(sc,CS4281_FCR2) & ~FCRn_FEN));
	BA0WRITE4(sc, CS4281_FCR3, (BA0READ4(sc,CS4281_FCR3) & ~FCRn_FEN));
#endif

	/* Set the SRC Slot Assignment accordingly */
	/*| PLSS[4:0]/
	 *| PRSS[4:0] | AC97 | Slot Function
	 *|-----------+------+----------------
	 *|     0     |  3   | Left PCM Playback
	 *|     1     |  4   | Right PCM Playback
	 *|     2     |  5   | phone line 1 DAC
	 *|     3     |  6   | Center PCM Playback
	 *|     4     |  7   | Left Surround PCM Playback
	 *|     5     |  8   | Right Surround PCM Playback
	 *......
	 *
	 *| CLSS[4:0]/
	 *| CRSS[4:0] | AC97 | Codec |Slot Function
	 *|-----------+------+-------+-----------------
	 *|    10     |   3  |Primary| Left PCM Record
	 *|    11     |   4  |Primary| Right PCM Record
	 *|    12     |   5  |Primary| Phone Line 1 ADC
	 *|    13     |   6  |Primary| Mic ADC
	 *|.....
	 *|    20     |   3  |  Sec. | Left PCM Record
	 *|    21     |   4  |  Sec. | Right PCM Record
	 *|    22     |   5  |  Sec. | Phone Line 1 ADC
	 *|    23     |   6  |  Sec. | Mic ADC
	 */
	dat32 = 0x0b << 24 |   /* CRSS[4:0] Right PCM Record(primary) */
		0x0a << 16 |   /* CLSS[4:0] Left  PCM Record(primary) */
		0x01 <<  8 |   /* PRSS[4:0] Right PCM Playback */
		0x00 <<  0;    /* PLSS[4:0] Left  PCM Playback */
	BA0WRITE4(sc, CS4281_SRCSA, dat32);
	
	/* Set interrupt to occurred at Half and Full terminal
	 * count interrupt enable for DMA channel 0 and 1.
	 * To keep DMA stop, set MSK.
	 */
	dat32 = DCRn_HTCIE | DCRn_TCIE | DCRn_MSK;
	BA0WRITE4(sc, CS4281_DCR0, dat32);
	BA0WRITE4(sc, CS4281_DCR1, dat32);
	
	/* Set Auto-Initialize Control enable */
	BA0WRITE4(sc, CS4281_DMR0,
		  DMRn_DMA | DMRn_AUTO | DMRn_TR_READ);
	BA0WRITE4(sc, CS4281_DMR1,
		  DMRn_DMA | DMRn_AUTO | DMRn_TR_WRITE);

	/* Clear DMA Mask in HIMR */
	dat32 = BA0READ4(sc, CS4281_HIMR) & 0xfffbfcff;
	BA0WRITE4(sc, CS4281_HIMR, dat32);
	return (0);
}

void
cs4281_power(why, v)
	int why;
	void *v;
{
	struct cs4281_softc *sc = (struct cs4281_softc *)v;
	int i;

	DPRINTF(("%s: cs4281_power why=%d\n", sc->sc_dev.dv_xname, why));
	if (why != PWR_RESUME) {
		sc->sc_suspend = why;

		cs4281_halt_output(sc);
		cs4281_halt_input(sc);
		/* Save AC97 registers */
		for (i = 1; i <= CS4281_SAVE_REG_MAX; i++) {
			if (i == 0x04)	/* AC97_REG_MASTER_TONE */
				continue;
			cs4281_read_codec(sc, 2*i, &sc->ac97_reg[i>>1]);
		}
		/* should I powerdown here ? */
		cs4281_write_codec(sc, AC97_REG_POWER, CS4281_POWER_DOWN_ALL);
	} else {
		if (sc->sc_suspend == PWR_RESUME) {
			printf("cs4281_power: odd, resume without suspend.\n");
			sc->sc_suspend = why;
			return;
		}
		sc->sc_suspend = why;
		cs4281_init(sc);
		cs4281_reset_codec(sc);

		/* restore ac97 registers */
		for (i = 1; i <= CS4281_SAVE_REG_MAX; i++) {
			if (i == 0x04)	/* AC97_REG_MASTER_TONE */
				continue;
			cs4281_write_codec(sc, 2*i, sc->ac97_reg[i>>1]);
		}
	}
}

void
cs4281_reset_codec(void *addr)
{
	struct cs4281_softc *sc;
	u_int16_t data;
	u_int32_t dat32;
	int n;

	sc = addr;

	DPRINTFN(3,("cs4281_reset_codec\n"));

	/* Reset codec */
	BA0WRITE4(sc, CS4281_ACCTL, 0);
	delay(50);    /* delay 50us */

	BA0WRITE4(sc, CS4281_SPMC, 0);
	delay(100);	/* delay 100us */
	BA0WRITE4(sc, CS4281_SPMC, SPMC_RSTN);
#if defined(ENABLE_SECONDARY_CODEC)
	BA0WRITE4(sc, CS4281_SPMC, SPMC_RSTN | SPCM_ASDIN2E);
	BA0WRITE4(sc, CS4281_SERMC, SERMC_TCID);
#endif
	delay(50000);   /* XXX: delay 50ms */

	/* Enable ASYNC generation */
	BA0WRITE4(sc, CS4281_ACCTL, ACCTL_ESYN);

	/* Wait for Codec ready. Linux driver wait 50ms here */
	n = 0;
	while((BA0READ4(sc, CS4281_ACSTS) & ACSTS_CRDY) == 0) {
		delay(100);
		if (++n > 1000) {
			printf("reset_codec: AC97 codec ready timeout\n");
			return;
		}
	}
#if defined(ENABLE_SECONDARY_CODEC)
	/* secondary codec ready*/
	n = 0;
	while((BA0READ4(sc, CS4281_ACSTS2) & ACSTS2_CRDY2) == 0) {
		delay(100);
		if (++n > 1000)
			return;
	}
#endif
	/* Set the serial timing configuration */
	/* XXX: undocumented but the Linux driver do this */
	BA0WRITE4(sc, CS4281_SERMC, SERMC_PTCAC97);
	
	/* Wait for Codec ready signal */
	n = 0;
	do {
		delay(1000);
		if (++n > 1000) {
			printf("%s: Timeout waiting for Codec ready\n",
			       sc->sc_dev.dv_xname);
			return;
		}
		dat32 = BA0READ4(sc, CS4281_ACSTS) & ACSTS_CRDY;
	} while (dat32 == 0);

	/* Enable Valid Frame output on ASDOUT */
	BA0WRITE4(sc, CS4281_ACCTL, ACCTL_ESYN | ACCTL_VFRM);
	
	/* Wait until Codec Calibration is finished. Codec register 26h */
	n = 0;
	do {
		delay(1);
		if (++n > 1000) {
			printf("%s: Timeout waiting for Codec calibration\n",
			       sc->sc_dev.dv_xname);
			return ;
		}
		cs4281_read_codec(sc, AC97_REG_POWER, &data);
	} while ((data & 0x0f) != 0x0f);

	/* Set the serial timing configuration again */
	/* XXX: undocumented but the Linux driver do this */
	BA0WRITE4(sc, CS4281_SERMC, SERMC_PTCAC97);

	/* Wait until we've sampled input slots 3 & 4 as valid */
	n = 0;
	do {
		delay(1000);
		if (++n > 1000) {
			printf("%s: Timeout waiting for sampled input slots as valid\n",
			       sc->sc_dev.dv_xname);
			return;
		}
		dat32 = BA0READ4(sc, CS4281_ACISV) & (ACISV_ISV3 | ACISV_ISV4) ;
	} while (dat32 != (ACISV_ISV3 | ACISV_ISV4));
	
	/* Start digital data transfer of audio data to the codec */
	BA0WRITE4(sc, CS4281_ACOSV, (ACOSV_SLV3 | ACOSV_SLV4));
}

int
cs4281_open(void *addr, int flags)
{
	return (0);
}

void
cs4281_close(void *addr)
{
	struct cs4281_softc *sc;

	sc = addr;

	(*sc->halt_output)(sc);
	(*sc->halt_input)(sc);
	
	sc->sc_pintr = 0;
	sc->sc_rintr = 0;
}

int
cs4281_round_blocksize(void *addr, int blk)
{
	struct cs4281_softc *sc;
	int retval;
	
	DPRINTFN(5,("cs4281_round_blocksize blk=%d -> ", blk));
	
	sc=addr;
	if (blk < sc->hw_blocksize)
		retval = sc->hw_blocksize;
	else
		retval = blk & -(sc->hw_blocksize);

	DPRINTFN(5,("%d\n", retval));

	return (retval);
}

int
cs4281_mixer_set_port(void *addr, mixer_ctrl_t *cp)
{
	struct cs4281_softc *sc;
	int val;

	sc = addr;
	val = sc->codec_if->vtbl->mixer_set_port(sc->codec_if, cp);
	DPRINTFN(3,("mixer_set_port: val=%d\n", val));
	return (val);
}

int
cs4281_mixer_get_port(void *addr, mixer_ctrl_t *cp)
{
	struct cs4281_softc *sc;

	sc = addr;
	return (sc->codec_if->vtbl->mixer_get_port(sc->codec_if, cp));
}


int
cs4281_query_devinfo(void *addr, mixer_devinfo_t *dip)
{
	struct cs4281_softc *sc;

	sc = addr;
	return (sc->codec_if->vtbl->query_devinfo(sc->codec_if, dip));
}

void *
cs4281_malloc(void *addr, int direction, size_t size, int pool, int flags)
{
	struct cs4281_softc *sc;
	struct cs4281_dma   *p;
	int error;

	sc = addr;

	p = malloc(sizeof(*p), pool, flags);
	if (!p)
		return (0);

	error = cs4281_allocmem(sc, size, pool, flags, p);

	if (error) {
		free(p, pool);
		return (0);
	}

	p->next = sc->sc_dmas;
	sc->sc_dmas = p;
	return (BUFADDR(p));
}



void
cs4281_free(void *addr, void *ptr, int pool)
{
	struct cs4281_softc *sc;
	struct cs4281_dma **pp, *p;

	sc = addr;
	for (pp = &sc->sc_dmas; (p = *pp) != NULL; pp = &p->next) {
		if (BUFADDR(p) == ptr) {
			bus_dmamap_unload(sc->sc_dmatag, p->map);
			bus_dmamap_destroy(sc->sc_dmatag, p->map);
			bus_dmamem_unmap(sc->sc_dmatag, p->addr, p->size);
			bus_dmamem_free(sc->sc_dmatag, p->segs, p->nsegs);
			free(p->dum, pool);
			*pp = p->next;
			free(p, pool);
			return;
		}
	}
}

size_t
cs4281_round_buffersize(void *addr, int direction, size_t size)
{
	/* The real dma buffersize are 4KB for CS4280
	 * and 64kB/MAX_CHANNELS for CS4281.
	 * But they are too small for high quality audio,
	 * let the upper layer(audio) use a larger buffer.
	 * (originally suggested by Lennart Augustsson.)
	 */
	return (size);
}

paddr_t
cs4281_mappage(void *addr, void *mem, off_t off, int prot)
{
	struct cs4281_softc *sc;
	struct cs4281_dma *p;
	
	sc = addr;
	if (off < 0)
		return -1;

	for (p = sc->sc_dmas; p && BUFADDR(p) != mem; p = p->next)
		;

	if (!p) {
		DPRINTF(("cs4281_mappage: bad buffer address\n"));
		return (-1);
	}

	return (bus_dmamem_mmap(sc->sc_dmatag, p->segs, p->nsegs, off, prot,
	    BUS_DMA_WAITOK));
}


int
cs4281_get_props(void *addr)
{
	int retval;

	retval = AUDIO_PROP_INDEPENDENT | AUDIO_PROP_FULLDUPLEX;
#ifdef MMAP_READY
	retval |= AUDIO_PROP_MMAP;
#endif
	return (retval);
}

/* AC97 */
int
cs4281_attach_codec(void *addr, struct ac97_codec_if *codec_if)
{
	struct cs4281_softc *sc;

	DPRINTF(("cs4281_attach_codec:\n"));
	sc = addr;
	sc->codec_if = codec_if;
	return (0);
}


int
cs4281_read_codec(void *addr, u_int8_t ac97_addr, u_int16_t *ac97_data)
{
	struct cs4281_softc *sc;
	u_int32_t acctl;
	int n;

	sc = addr;

	DPRINTFN(5,("read_codec: add=0x%02x ", ac97_addr));
	/*
	 * Make sure that there is not data sitting around from a preivous
	 * uncompleted access.
	 */
	BA0READ4(sc, CS4281_ACSDA);

	/* Set up AC97 control registers. */
	BA0WRITE4(sc, CS4281_ACCAD, ac97_addr);
	BA0WRITE4(sc, CS4281_ACCDA, 0);

	acctl = ACCTL_ESYN | ACCTL_VFRM | ACCTL_CRW  | ACCTL_DCV;
	BA0WRITE4(sc, CS4281_ACCTL, acctl);

	if (cs4281_src_wait(sc) < 0) {
		printf("%s: AC97 read prob. (DCV!=0) for add=0x%0x\n",
		       sc->sc_dev.dv_xname, ac97_addr);
		return 1;
	}

	/* wait for valid status bit is active */
	n = 0;
	while ((BA0READ4(sc, CS4281_ACSTS) & ACSTS_VSTS) == 0) {
		delay(1);
		while (++n > 1000) {
			printf("%s: AC97 read fail (VSTS==0) for add=0x%0x\n",
			       sc->sc_dev.dv_xname, ac97_addr);
			return 1;
		}
	}
	*ac97_data = BA0READ4(sc, CS4281_ACSDA);
	DPRINTFN(5,("data=0x%04x\n", *ac97_data));
	return (0);
}

int
cs4281_write_codec(void *addr, u_int8_t ac97_addr, u_int16_t ac97_data)
{
	struct cs4281_softc *sc;
	u_int32_t acctl;

	sc = addr;

	DPRINTFN(5,("write_codec: add=0x%02x  data=0x%04x\n", ac97_addr, ac97_data));
	BA0WRITE4(sc, CS4281_ACCAD, ac97_addr);
	BA0WRITE4(sc, CS4281_ACCDA, ac97_data);

	acctl = ACCTL_ESYN | ACCTL_VFRM | ACCTL_DCV;
	BA0WRITE4(sc, CS4281_ACCTL, acctl);

	if (cs4281_src_wait(sc) < 0) {
		printf("%s: AC97 write fail (DCV!=0) for add=0x%02x data="
		       "0x%04x\n", sc->sc_dev.dv_xname, ac97_addr, ac97_data);
		return (1);
	}
	return (0);
}

int
cs4281_allocmem(struct cs4281_softc *sc, size_t size, int pool, int flags,
		struct cs4281_dma *p)
{
	int error;
	size_t align;
	
	align   = sc->dma_align;
	p->size = sc->dma_size;
	/* allocate memory for upper audio driver */
	p->dum  = malloc(size, pool, flags);
	if (!p->dum)
		return (1);
	error = bus_dmamem_alloc(sc->sc_dmatag, p->size, align, 0,
				 p->segs, sizeof(p->segs)/sizeof(p->segs[0]),
				 &p->nsegs, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: unable to allocate dma. error=%d\n",
		       sc->sc_dev.dv_xname, error);
		return (error);
	}

	error = bus_dmamem_map(sc->sc_dmatag, p->segs, p->nsegs, p->size,
			       &p->addr, BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (error) {
		printf("%s: unable to map dma, error=%d\n",
		       sc->sc_dev.dv_xname, error);
		goto free;
	}

	error = bus_dmamap_create(sc->sc_dmatag, p->size, 1, p->size,
				  0, BUS_DMA_NOWAIT, &p->map);
	if (error) {
		printf("%s: unable to create dma map, error=%d\n",
		       sc->sc_dev.dv_xname, error);
		goto unmap;
	}

	error = bus_dmamap_load(sc->sc_dmatag, p->map, p->addr, p->size, NULL,
				BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: unable to load dma map, error=%d\n",
		       sc->sc_dev.dv_xname, error);
		goto destroy;
	}
	return (0);

destroy:
	bus_dmamap_destroy(sc->sc_dmatag, p->map);
unmap:
	bus_dmamem_unmap(sc->sc_dmatag, p->addr, p->size);
free:
	bus_dmamem_free(sc->sc_dmatag, p->segs, p->nsegs);
	return (error);
}


int
cs4281_src_wait(sc)
	struct cs4281_softc *sc;
{
	int n;

	n = 0;
	while ((BA0READ4(sc, CS4281_ACCTL) & ACCTL_DCV)) {
		delay(1000);
		if (++n > 1000)
			return (-1);
	}
	return (0);
}
