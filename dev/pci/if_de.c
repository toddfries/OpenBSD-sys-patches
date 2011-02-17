/*	$OpenBSD: if_de.c,v 1.106 2010/09/20 07:40:38 deraadt Exp $	*/
/*	$NetBSD: if_de.c,v 1.58 1998/01/12 09:39:58 thorpej Exp $	*/

/*-
 * Copyright (c) 1994-1997 Matt Thomas (matt@3am-software.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
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
 *
 * Id: if_de.c,v 1.89 1997/06/03 19:19:55 thomas Exp
 *
 */

/*
 * DEC 21040 PCI Ethernet Controller
 *
 * Written by Matt Thomas
 * BPF support code stolen directly from if_ec.c
 *
 *   This driver supports the DEC DE435 or any other PCI
 *   board which support 21040, 21041, or 21140 (mostly).
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/timeout.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <net/netisr.h>

#include "bpfilter.h"
#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#endif

#include <netinet/if_ether.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/ic/dc21040reg.h>

/*
 * Intel CPUs should use I/O mapped access.
 */
#if defined(__i386__)
#define	TULIP_IOMAPPED
#endif

#define	TULIP_HZ	10

#define TULIP_SIAGEN_WATCHDOG	0

#define TULIP_GPR_CMDBITS	(TULIP_CMD_PORTSELECT|TULIP_CMD_PCSFUNCTION|TULIP_CMD_SCRAMBLER|TULIP_CMD_TXTHRSHLDCTL)

#define EMIT	do { TULIP_CSR_WRITE(sc, csr_srom_mii, csr); tulip_delay_300ns(sc); } while (0)
#define MII_EMIT	do { TULIP_CSR_WRITE(sc, csr_srom_mii, csr); tulip_delay_300ns(sc); } while (0)

#define tulip_mchash(mca)	(ether_crc32_le(mca, 6) & 0x1FF)
#define tulip_srom_crcok(databuf)	( \
    ((ether_crc32_le(databuf, 126) & 0xFFFFU) ^ 0xFFFFU) == \
     ((databuf)[126] | ((databuf)[127] << 8)))

/*
 * This is the PCI configuration support.  Since the 21040 is available
 * on both EISA and PCI boards, one must be careful in how defines the
 * 21040 in the config file.
 */

#define PCI_CFID	0x00	/* Configuration ID */
#define PCI_CFCS	0x04	/* Configurtion Command/Status */
#define PCI_CFRV	0x08	/* Configuration Revision */
#define PCI_CFLT	0x0c	/* Configuration Latency Timer */
#define PCI_CBIO	0x10	/* Configuration Base IO Address */
#define PCI_CBMA	0x14	/* Configuration Base Memory Address */
#define PCI_CFIT	0x3c	/* Configuration Interrupt */
#define PCI_CFDA	0x40	/* Configuration Driver Area */

#define PCI_CONF_WRITE(r, v)	pci_conf_write(pa->pa_pc, pa->pa_tag, (r), (v))
#define PCI_CONF_READ(r)	pci_conf_read(pa->pa_pc, pa->pa_tag, (r))
#define PCI_GETBUSDEVINFO(sc)	do { \
	(sc)->tulip_pci_busno = parent; \
	(sc)->tulip_pci_devno = pa->pa_device; \
    } while (0)

#include <dev/pci/if_devar.h>
/*
 * This module supports
 *	the DEC 21040 PCI Ethernet Controller.
 *	the DEC 21041 PCI Ethernet Controller.
 *	the DEC 21140 PCI Fast Ethernet Controller.
 */
int tulip_probe(struct device *parent, void *match, void *aux);
void tulip_attach(struct device * const parent, struct device * const self, void * const aux);

struct cfattach de_ca = {
	sizeof(tulip_softc_t), tulip_probe, tulip_attach
};

struct cfdriver de_cd = {
	NULL, "de", DV_IFNET
};

void tulip_timeout_callback(void *arg);
void tulip_timeout(tulip_softc_t * const sc);
int tulip_txprobe(tulip_softc_t * const sc);
void tulip_media_set(tulip_softc_t * const sc, tulip_media_t media);
void tulip_linkup(tulip_softc_t * const sc, tulip_media_t media);
void tulip_media_print(tulip_softc_t * const sc);
tulip_link_status_t tulip_media_link_monitor(tulip_softc_t * const sc);
void tulip_media_poll(tulip_softc_t * const sc, tulip_mediapoll_event_t event);
void tulip_media_select(tulip_softc_t * const sc);

void tulip_21040_mediainfo_init(tulip_softc_t * const sc, tulip_media_t media);
void tulip_21040_media_probe(tulip_softc_t * const sc);
void tulip_21040_10baset_only_media_probe(tulip_softc_t * const sc);
void tulip_21040_10baset_only_media_select(tulip_softc_t * const sc);
void tulip_21040_auibnc_only_media_probe(tulip_softc_t * const sc);
void tulip_21040_auibnc_only_media_select(tulip_softc_t * const sc);

void tulip_21041_mediainfo_init(tulip_softc_t * const sc);
void tulip_21041_media_probe(tulip_softc_t * const sc);
void tulip_21041_media_poll(tulip_softc_t * const sc, const tulip_mediapoll_event_t event);

tulip_media_t tulip_mii_phy_readspecific(tulip_softc_t * const sc);
unsigned tulip_mii_get_phyaddr(tulip_softc_t * const sc, unsigned offset);
int tulip_mii_map_abilities(tulip_softc_t * const sc, unsigned abilities);
void tulip_mii_autonegotiate(tulip_softc_t * const sc, const unsigned phyaddr);

void tulip_2114x_media_preset(tulip_softc_t * const sc);

void tulip_null_media_poll(tulip_softc_t * const sc, tulip_mediapoll_event_t event);

void tulip_21140_mediainit(tulip_softc_t * const sc, tulip_media_info_t * const mip,
    tulip_media_t const media, unsigned gpdata, unsigned cmdmode);
void tulip_21140_evalboard_media_probe(tulip_softc_t * const sc);
void tulip_21140_accton_media_probe(tulip_softc_t * const sc);
void tulip_21140_smc9332_media_probe(tulip_softc_t * const sc);
void tulip_21140_cogent_em100_media_probe(tulip_softc_t * const sc);
void tulip_21140_znyx_zx34x_media_probe(tulip_softc_t * const sc);

void tulip_2114x_media_probe(tulip_softc_t * const sc);

void tulip_delay_300ns(tulip_softc_t * const sc);
void tulip_srom_idle(tulip_softc_t * const sc);
void tulip_srom_read(tulip_softc_t * const sc);
void tulip_mii_writebits(tulip_softc_t * const sc, unsigned data, unsigned bits);
void tulip_mii_turnaround(tulip_softc_t * const sc, unsigned cmd);
unsigned tulip_mii_readbits(tulip_softc_t * const sc);
unsigned tulip_mii_readreg(tulip_softc_t * const sc, unsigned devaddr, unsigned regno);
void tulip_mii_writereg(tulip_softc_t * const sc, unsigned devaddr, unsigned regno,
    unsigned data);

void tulip_identify_dec_nic(tulip_softc_t * const sc);
void tulip_identify_znyx_nic(tulip_softc_t * const sc);
void tulip_identify_smc_nic(tulip_softc_t * const sc);
void tulip_identify_cogent_nic(tulip_softc_t * const sc);
void tulip_identify_accton_nic(tulip_softc_t * const sc);
void tulip_identify_asante_nic(tulip_softc_t * const sc);
void tulip_identify_compex_nic(tulip_softc_t * const sc);

int tulip_srom_decode(tulip_softc_t * const sc);
int tulip_read_macaddr(tulip_softc_t * const sc);
void tulip_ifmedia_add(tulip_softc_t * const sc);
int tulip_ifmedia_change(struct ifnet * const ifp);
void tulip_ifmedia_status(struct ifnet * const ifp, struct ifmediareq *req);
void tulip_addr_filter(tulip_softc_t * const sc);
void tulip_reset(tulip_softc_t * const sc);
void tulip_init(tulip_softc_t * const sc);
void tulip_rx_intr(tulip_softc_t * const sc);
int tulip_tx_intr(tulip_softc_t * const sc);
void tulip_print_abnormal_interrupt(tulip_softc_t * const sc, u_int32_t csr);
void tulip_intr_handler(tulip_softc_t * const sc, int *progress_p);
int tulip_intr_shared(void *arg);
int tulip_intr_normal(void *arg);
struct mbuf *tulip_mbuf_compress(struct mbuf *m);
struct mbuf *tulip_txput(tulip_softc_t * const sc, struct mbuf *m);
void tulip_txput_setup(tulip_softc_t * const sc);
int tulip_ifioctl(struct ifnet * ifp, u_long cmd, caddr_t data);
void tulip_ifstart(struct ifnet *ifp);
void tulip_ifstart_one(struct ifnet *ifp);
void tulip_ifwatchdog(struct ifnet *ifp);
int tulip_busdma_allocmem(tulip_softc_t * const sc, size_t size,
    bus_dmamap_t *map_p, tulip_desc_t **desc_p);
int tulip_busdma_init(tulip_softc_t * const sc);
void tulip_initcsrs(tulip_softc_t * const sc, bus_addr_t csr_base, size_t csr_size);
void tulip_initring(tulip_softc_t * const sc, tulip_ringinfo_t * const ri,
    tulip_desc_t *descs, int ndescs);

bus_dmamap_t tulip_alloc_rxmap(tulip_softc_t *);
void tulip_free_rxmap(tulip_softc_t *, bus_dmamap_t);
bus_dmamap_t tulip_alloc_txmap(tulip_softc_t *);
void tulip_free_txmap(tulip_softc_t *, bus_dmamap_t);

void
tulip_timeout_callback(void *arg)
{
    tulip_softc_t * const sc = arg;
    int s;

    s = splnet();

    TULIP_PERFSTART(timeout)

    sc->tulip_flags &= ~TULIP_TIMEOUTPENDING;
    sc->tulip_probe_timeout -= 1000 / TULIP_HZ;
    (sc->tulip_boardsw->bd_media_poll)(sc, TULIP_MEDIAPOLL_TIMER);

    TULIP_PERFEND(timeout);
    splx(s);
}

void
tulip_timeout(tulip_softc_t * const sc)
{
    if (sc->tulip_flags & TULIP_TIMEOUTPENDING)
	return;
    sc->tulip_flags |= TULIP_TIMEOUTPENDING;
    timeout_add(&sc->tulip_stmo, (hz + TULIP_HZ / 2) / TULIP_HZ);
}

int
tulip_txprobe(tulip_softc_t * const sc)
{
    struct mbuf *m;

    /*
     * Before we are sure this is the right media we need
     * to send a small packet to make sure there's carrier.
     * Strangely, BNC and AUI will "see" receive data if
     * either is connected so the transmit is the only way
     * to verify the connectivity.
     */
    MGETHDR(m, M_DONTWAIT, MT_DATA);
    if (m == NULL)
	return (0);
    /*
     * Construct a LLC TEST message which will point to ourselves.
     */
    bcopy(sc->tulip_enaddr, mtod(m, struct ether_header *)->ether_dhost,
       ETHER_ADDR_LEN);
    bcopy(sc->tulip_enaddr, mtod(m, struct ether_header *)->ether_shost,
       ETHER_ADDR_LEN);
    mtod(m, struct ether_header *)->ether_type = htons(3);
    mtod(m, unsigned char *)[14] = 0;
    mtod(m, unsigned char *)[15] = 0;
    mtod(m, unsigned char *)[16] = 0xE3;	/* LLC Class1 TEST (no poll) */
    m->m_len = m->m_pkthdr.len = sizeof(struct ether_header) + 3;
    /*
     * send it!
     */
    sc->tulip_cmdmode |= TULIP_CMD_TXRUN;
    sc->tulip_intrmask |= TULIP_STS_TXINTR;
    sc->tulip_flags |= TULIP_TXPROBE_ACTIVE;
    TULIP_CSR_WRITE(sc, csr_command, sc->tulip_cmdmode);
    TULIP_CSR_WRITE(sc, csr_intr, sc->tulip_intrmask);
    if ((m = tulip_txput(sc, m)) != NULL)
	m_freem(m);
    sc->tulip_probe.probe_txprobes++;
    return (1);
}

void
tulip_media_set(tulip_softc_t * const sc, tulip_media_t media)
{
    const tulip_media_info_t *mi = sc->tulip_mediums[media];

    if (mi == NULL)
	return;

    /* Reset the SIA first
     */
    if (mi->mi_type == TULIP_MEDIAINFO_SIA || (sc->tulip_features & TULIP_HAVE_SIANWAY))
	TULIP_CSR_WRITE(sc, csr_sia_connectivity, TULIP_SIACONN_RESET);

    /* Next, set full duplex if needed.
     */
    if (sc->tulip_flags & TULIP_FULLDUPLEX) {
#ifdef TULIP_DEBUG
	if (TULIP_CSR_READ(sc, csr_command) & (TULIP_CMD_RXRUN|TULIP_CMD_TXRUN))
	    printf(TULIP_PRINTF_FMT ": warning: board is running (FD).\n", TULIP_PRINTF_ARGS);
	if ((TULIP_CSR_READ(sc, csr_command) & TULIP_CMD_FULLDUPLEX) == 0)
	    printf(TULIP_PRINTF_FMT ": setting full duplex.\n", TULIP_PRINTF_ARGS);
#endif
	sc->tulip_cmdmode |= TULIP_CMD_FULLDUPLEX;
	TULIP_CSR_WRITE(sc, csr_command, sc->tulip_cmdmode & ~(TULIP_CMD_RXRUN|TULIP_CMD_TXRUN));
    }

    /* Now setup the media.
     *
     * If we are switching media, make sure we don't think there's
     * any stale RX activity
     */
    sc->tulip_flags &= ~TULIP_RXACT;
    if (mi->mi_type == TULIP_MEDIAINFO_SIA) {
	TULIP_CSR_WRITE(sc, csr_sia_tx_rx,        mi->mi_sia_tx_rx);
	if (sc->tulip_features & TULIP_HAVE_SIAGP) {
	    TULIP_CSR_WRITE(sc, csr_sia_general,  mi->mi_sia_gp_control|mi->mi_sia_general|TULIP_SIAGEN_WATCHDOG);
	    DELAY(50);
	    TULIP_CSR_WRITE(sc, csr_sia_general,  mi->mi_sia_gp_data|mi->mi_sia_general|TULIP_SIAGEN_WATCHDOG);
	} else
	    TULIP_CSR_WRITE(sc, csr_sia_general,  mi->mi_sia_general|TULIP_SIAGEN_WATCHDOG);
	TULIP_CSR_WRITE(sc, csr_sia_connectivity, mi->mi_sia_connectivity);
    } else if (mi->mi_type == TULIP_MEDIAINFO_GPR) {
	/*
	 * If the cmdmode bits don't match the currently operating mode,
	 * set the cmdmode appropriately and reset the chip.
	 */
	if (((mi->mi_cmdmode ^ TULIP_CSR_READ(sc, csr_command)) & TULIP_GPR_CMDBITS) != 0) {
	    sc->tulip_cmdmode &= ~TULIP_GPR_CMDBITS;
	    sc->tulip_cmdmode |= mi->mi_cmdmode;
	    tulip_reset(sc);
	}
	TULIP_CSR_WRITE(sc, csr_gp, TULIP_GP_PINSET|sc->tulip_gpinit);
	DELAY(10);
	TULIP_CSR_WRITE(sc, csr_gp, (u_int8_t) mi->mi_gpdata);
    } else if (mi->mi_type == TULIP_MEDIAINFO_SYM) {
	/*
	 * If the cmdmode bits don't match the currently operating mode,
	 * set the cmdmode appropriately and reset the chip.
	 */
	if (((mi->mi_cmdmode ^ TULIP_CSR_READ(sc, csr_command)) & TULIP_GPR_CMDBITS) != 0) {
	    sc->tulip_cmdmode &= ~TULIP_GPR_CMDBITS;
	    sc->tulip_cmdmode |= mi->mi_cmdmode;
	    tulip_reset(sc);
	}
	TULIP_CSR_WRITE(sc, csr_sia_general, mi->mi_gpcontrol);
	TULIP_CSR_WRITE(sc, csr_sia_general, mi->mi_gpdata);
    } else if (mi->mi_type == TULIP_MEDIAINFO_MII
	       && sc->tulip_probe_state != TULIP_PROBE_INACTIVE) {
	int idx;
	if (sc->tulip_features & TULIP_HAVE_SIAGP) {
	    const u_int8_t *dp;
	    dp = &sc->tulip_rombuf[mi->mi_reset_offset];
	    for (idx = 0; idx < mi->mi_reset_length; idx++, dp += 2) {
		DELAY(10);
		TULIP_CSR_WRITE(sc, csr_sia_general, (dp[0] + 256 * dp[1]) << 16);
	    }
	    sc->tulip_phyaddr = mi->mi_phyaddr;
	    dp = &sc->tulip_rombuf[mi->mi_gpr_offset];
	    for (idx = 0; idx < mi->mi_gpr_length; idx++, dp += 2) {
		DELAY(10);
		TULIP_CSR_WRITE(sc, csr_sia_general, (dp[0] + 256 * dp[1]) << 16);
	    }
	} else {
	    for (idx = 0; idx < mi->mi_reset_length; idx++) {
		DELAY(10);
		TULIP_CSR_WRITE(sc, csr_gp, sc->tulip_rombuf[mi->mi_reset_offset + idx]);
	    }
	    sc->tulip_phyaddr = mi->mi_phyaddr;
	    for (idx = 0; idx < mi->mi_gpr_length; idx++) {
		DELAY(10);
		TULIP_CSR_WRITE(sc, csr_gp, sc->tulip_rombuf[mi->mi_gpr_offset + idx]);
	    }
	}

	if (sc->tulip_features & TULIP_HAVE_SIANWAY) {
	    /* Set the SIA port into MII mode */
	    TULIP_CSR_WRITE(sc, csr_sia_general, 1);
	    TULIP_CSR_WRITE(sc, csr_sia_tx_rx, 0);
	    TULIP_CSR_WRITE(sc, csr_sia_status, 0);
	}

	if (sc->tulip_flags & TULIP_TRYNWAY)
	    tulip_mii_autonegotiate(sc, sc->tulip_phyaddr);
	else if ((sc->tulip_flags & TULIP_DIDNWAY) == 0) {
	    u_int32_t data = tulip_mii_readreg(sc, sc->tulip_phyaddr, PHYREG_CONTROL);
	    data &= ~(PHYCTL_SELECT_100MB|PHYCTL_FULL_DUPLEX|PHYCTL_AUTONEG_ENABLE);
	    sc->tulip_flags &= ~TULIP_DIDNWAY;
	    if (TULIP_IS_MEDIA_FD(media))
		data |= PHYCTL_FULL_DUPLEX;
	    if (TULIP_IS_MEDIA_100MB(media))
		data |= PHYCTL_SELECT_100MB;
	    tulip_mii_writereg(sc, sc->tulip_phyaddr, PHYREG_CONTROL, data);
	}
    }
}

void
tulip_linkup(tulip_softc_t * const sc, tulip_media_t media)
{
    if ((sc->tulip_flags & TULIP_LINKUP) == 0)
	sc->tulip_flags |= TULIP_PRINTLINKUP;
    sc->tulip_flags |= TULIP_LINKUP;
    sc->tulip_if.if_flags &= ~IFF_OACTIVE;
    if (sc->tulip_media != media) {
#ifdef TULIP_DEBUG
	sc->tulip_dbg.dbg_last_media = sc->tulip_media;
#endif
	sc->tulip_media = media;
	sc->tulip_flags |= TULIP_PRINTMEDIA;
	if (TULIP_IS_MEDIA_FD(sc->tulip_media))
	    sc->tulip_flags |= TULIP_FULLDUPLEX;
	else if (sc->tulip_chipid != TULIP_21041 || (sc->tulip_flags & TULIP_DIDNWAY) == 0)
	    sc->tulip_flags &= ~TULIP_FULLDUPLEX;
    }
    /*
     * We could set probe_timeout to 0 but setting to 3000 puts this
     * in one central place and the only matters is tulip_link is
     * followed b