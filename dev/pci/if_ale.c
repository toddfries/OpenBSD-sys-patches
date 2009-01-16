/*	$OpenBSD$	*/

/*-
 * Copyright (c) 2008, Pyun YongHyeon <yongari@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/dev/ale/if_ale.c,v 1.3 2008/12/03 09:01:12 yongari Exp $
 */

/* Driver for Atheros AR8121/AR8113/AR8114 PCIe Ethernet. */

#include "bpfilter.h"
#include "vlan.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/endian.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/queue.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/timeout.h>
#include <sys/socket.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_llc.h>
#include <net/if_media.h>

#include <netinet/ip.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#endif

#include <net/if_types.h>
#include <net/if_vlan_var.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <dev/rndvar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/if_alereg.h>

int	ale_match(struct device *, void *, void *);
int	ale_attach(struct device *);
int	ale_detach(struct device *);
int	ale_shutdown(struct device *);
int	ale_suspend(struct device *);
int	ale_resume(struct device *);

int	ale_miibus_readreg(struct device *, int, int);
void	ale_miibus_writereg(struct device *, int, int, int);
void	ale_miibus_statchg(struct device *);

void	ale_init(void *);
void	ale_start(struct ifnet *);
int	ale_ioctl(struct ifnet *, u_long, caddr_t, struct ucred *);
void	ale_watchdog(struct ifnet *);
int	ale_mediachange(struct ifnet *);
void	ale_mediastatus(struct ifnet *, struct ifmediareq *);

void	ale_intr(void *);
int	ale_rxeof(struct ale_softc *sc);
void	ale_rx_update_page(struct ale_softc *, struct ale_rx_page **,
		    uint32_t, uint32_t *);
void	ale_rxcsum(struct ale_softc *, struct mbuf *, uint32_t);
void	ale_txeof(struct ale_softc *);

int	ale_dma_alloc(struct ale_softc *);
void	ale_dma_free(struct ale_softc *);
int	ale_check_boundary(struct ale_softc *);
void	ale_dmamap_cb(void *, bus_dma_segment_t *, int, int);
void	ale_dmamap_buf_cb(void *, bus_dma_segment_t *, int,
		    bus_size_t, int);
int	ale_encap(struct ale_softc *, struct mbuf **);
void	ale_init_rx_pages(struct ale_softc *);
void	ale_init_tx_ring(struct ale_softc *);

void	ale_stop(struct ale_softc *);
void	ale_tick(void *);
void	ale_get_macaddr(struct ale_softc *);
void	ale_mac_config(struct ale_softc *);
void	ale_phy_reset(struct ale_softc *);
void	ale_reset(struct ale_softc *);
void	ale_rxfilter(struct ale_softc *);
void	ale_rxvlan(struct ale_softc *);
void	ale_stats_clear(struct ale_softc *);
void	ale_stats_update(struct ale_softc *);
void	ale_stop_mac(struct ale_softc *);
#ifdef notyet
void	ale_setlinkspeed(struct ale_softc *);
void	ale_setwol(struct ale_softc *);
#endif

void	ale_sysctl_node(struct ale_softc *);
int	sysctl_hw_ale_int_mod(SYSCTL_HANDLER_ARGS);

const struct pci_matchid ale_devices[] = {
	{ PCI_VENDOR_ATTANSIC, PCI_PRODUCT_ATTANSIC_L1E }
};

struct cfattach ale_ca = {
	sizeof (struct ale_softc), ale_match, ale_attach
};

struct cfdriver ale_cd = {
	NULL, "ale", DV_IFNET
};

int aledebug = 0;
#define	DPRINTF(x)	do { if (aledebug) printf x; } while (0)

#define ALE_CSUM_FEATURES	(CSUM_TCP | CSUM_UDP)

int
ale_match(struct device *dev, void *match, void *aux)
{
	return pci_matchbyid((struct pci_attach_args *)aux, ale_devices,
		sizeof (ale_devices) / sizeof (ale_devices[0]));
}

int
ale_attach(struct device *parent, struct device *self, void *aux)
{
	struct ale_softc *sc = (struct ale_softc *)self;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *intrstr;
	struct ifnet *ifp;
	pcireg_t memtype;
	int error = 0;
	uint32_t rxf_len, txf_len;
	uint8_t pcie_ptr;

	/*
	 * Allocate IO memory
	 */
	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, ALE_PCIR_BAR);
	if (pci_mapreg_map(pa, ALE_PCIR_BAR, memtype, 0, &sc->sc_mem_bt,
	    &sc->sc_mem_bh, NULL, &sc->sc_mem_size, 0)) {
		printf": could not map mem space\n");
		return;
	}

	if (pci_intr_map(pa, &ih) != 0) {
		printf(": could not map interrupt\n");
		return;
	}

	/*
	 * Allocate IRQ
	 */
	intrstr = pci_intr_string(pc, ih);
	sc->sc_irq_handle = pci_intr_establish(pc, ih, IPL_NET, ale_intr, sc,
	    sc->sc_dev.dv_xname);
	if (sc->sc_irq_handle == NULL) {
		printf(": could not establish interrupt");
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf(": %s", intrstr);

	sc->sc_dmat = pa->pa_dmat;
	sc->sc_pct = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;

	/* Set PHY address. */
	sc->ale_phyaddr = ALE_PHY_ADDR;

	/* Reset PHY. */
	ale_phy_reset(sc);

	/* Reset the ethernet controller. */
	ale_reset(sc);

	/* Get PCI and chip id/revision. */
	sc->ale_rev = PCI_REVISION(pa->pa_class);
	if (sc->ale_rev >= 0xF0) {
		/* L2E Rev. B. AR8114 */
		sc->ale_flags |= ALE_FLAG_FASTETHER;
	} else {
		if ((CSR_READ_4(sc, ALE_PHY_STATUS) & PHY_STATUS_100M) != 0) {
			/* L1E AR8121 */
			sc->ale_flags |= ALE_FLAG_JUMBO;
		} else {
			/* L2E Rev. A. AR8113 */
			sc->ale_flags |= ALE_FLAG_FASTETHER;
		}
	}

	/*
	 * All known controllers seems to require 4 bytes alignment
	 * of Tx buffers to make Tx checksum offload with custom
	 * checksum generation method work.
	 */
	sc->ale_flags |= ALE_FLAG_TXCSUM_BUG;

	/*
	 * All known controllers seems to have issues on Rx checksum
	 * offload for fragmented IP datagrams.
	 */
	sc->ale_flags |= ALE_FLAG_RXCSUM_BUG;

	/*
	 * Don't use Tx CMB. It is known to cause RRS update failure
	 * under certain circumstances. Typical phenomenon of the
	 * issue would be unexpected sequence number encountered in
	 * Rx handler.
	 */
	sc->ale_flags |= ALE_FLAG_TXCMB_BUG;
	sc->ale_chip_rev = CSR_READ_4(sc, ALE_MASTER_CFG) >>
	    MASTER_CHIP_REV_SHIFT;
	if (aledebug) {
		printf("%s: PCI device revision : 0x%04x\n",
		    sc->sc_dev.dv_xname,
		    sc->ale_rev);
		printf("Chip id/revision : 0x%04x\n",
		    sc->sc_dev.dv_xname,
		    sc->ale_chip_rev);
	}

	/*
	 * Uninitialized hardware returns an invalid chip id/revision
	 * as well as 0xFFFFFFFF for Tx/Rx fifo length.
	 */
	txf_len = CSR_READ_4(sc, ALE_SRAM_TX_FIFO_LEN);
	rxf_len = CSR_READ_4(sc, ALE_SRAM_RX_FIFO_LEN);
	if (sc->ale_chip_rev == 0xFFFF || txf_len == 0xFFFFFFFF ||
	    rxf_len == 0xFFFFFFF) {
		printf("%s: chip revision : 0x%04x, %u Tx FIFO "
		    "%u Rx FIFO -- not initialized?\n", sc->sc_dev.dv_xname,
		    sc->ale_chip_rev, txf_len, rxf_len);
		error = ENXIO;
		goto fail;
	}
	if (aledebug) {
		printf("%s: %u Tx FIFO, %u Rx FIFO\n", sc->sc_dev.dv_xname,
			txf_len, rxf_len);
	}

	error = ale_dma_alloc(sc);
	if (error)
		goto fail;

	/* Load station address. */
	ale_get_macaddr(sc, sc->sc_arpcom.ac_enaddr);

	printf(", address %s\n", ether_sprintf(sc->sc_arpcom.ac_enaddr));

	ifp = &sc->sc_arpcom.ac_if;
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = ale_init;
	ifp->if_ioctl = ale_ioctl;
	ifp->if_start = ale_start;
	ifp->if_watchdog = ale_watchdog;
	IFQ_SET_MAXLEN(&ifp->if_snd, ALE_TX_RING_CNT - 1);
	IFQ_SET_READY(&ifp->if_snd);
	strlcpy(ifp->if_xname, sc->sc_dev.dv_xname, IFNAMSIZ);

	ifp->if_capabilities = IFCAP_VLAN_MTU;

#ifdef ALE_CHECKSUM
	/* XXX ifp->if_capabilities |= IFCAP_RXCSUM; */
#endif

#if NVLAN > 0
	ifp->if_capabilities |= IFCAP_VLAN_HWTAGGING;
#endif
#ifdef notyet
	ifp->if_capabilities |= IFCAP_TXCSUM;
	ifp->if_hwassist = ALE_CSUM_FEATURES;
#endif

	/* Set up MII bus. */
	sc->sc_miibus.mii_ifp = ifp;
	sc->sc_miibus.mii_readreg = ale_miibus_readreg;
	sc->sc_miibus.mii_writereg = ale_miibus_writereg;
	sc->sc_miibus.mii_statchg = ale_miibus_statchg;

	ifmedia_init(&sc->sc_miibus.mii_media, 0, ale_mediachange,
	    ale_mediastatus);
	mii_attach(self, &sc->sc_miibus, 0xffffffff, MII_PHY_ANY,
	    MII_OFFSET_ANY, 0);

	if (LIST_FIRST(&sc->sc_miibus.mii_phys) == NULL) {
		printf("%s: no PHY found!\n", sc->sc_dev.dv_xname);
		ifmedia_add(&sc->sc_miibus.mii_media, IFM_ETHER | IFM_MANUAL,
		    0, NULL);
		ifmedia_set(&sc->sc_miibus.mii_media, IFM_ETHER | IFM_MANUAL);
	} else
		ifmedia_set(&sc->sc_miibus.mii_media, IFM_ETHER | IFM_AUTO);

	if_attach(ifp);
	ether_ifattach(ifp);

	timeout_set(&sc->ale_tick_ch, ale_tick, sc);

fail:
	ale_detach(&sc->sc_dev, 0);
	return (error);
}

int
ale_detach(struct device *self, int flags)
{
	struct ale_softc *sc = (struct ale_softc *)self;
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	int s;

	s = splnet();
	ale_stop(sc);
	splx(s);

	mii_detach(&sc->sc_miibus, MII_PHY_ANY, MII_OFFSET_ANY);

	/* Delete all remaining media. */
	ifmedia_delete_instance(&sc->sc_miibus.mii_media, IFM_INST_ANY);

	ether_ifdetach(ifp);
	if_detach(ifp);
	ale_dma_free(sc);

	if (sc->sc_irq_handle != NULL) {
		pci_intr_disestablish(sc->sc_pct, sc->sc_irq_handle);
		sc->sc_irq_handle = NULL;
	}

	return (0);
}

/*
 *	Read a PHY register on the MII of the L1E.
 */
int
ale_miibus_readreg(struct device *dev, int phy, int reg)
{
	struct ale_softc *sc = (struct ale_softc *)dev;
	uint32_t v;
	int i;

	if (phy != sc->ale_phyaddr)
		return (0);

	CSR_WRITE_4(sc, ALE_MDIO, MDIO_OP_EXECUTE | MDIO_OP_READ |
	    MDIO_SUP_PREAMBLE | MDIO_CLK_25_4 | MDIO_REG_ADDR(reg));
	for (i = ALE_PHY_TIMEOUT; i > 0; i--) {
		DELAY(5);
		v = CSR_READ_4(sc, ALE_MDIO);
		if ((v & (MDIO_OP_EXECUTE | MDIO_OP_BUSY)) == 0)
			break;
	}

	if (i == 0) {
		printf("%s: phy read timeout : %d\n",
		    sc->sc_dev.dv_xname, reg);
		return (0);
	}

	return ((v & MDIO_DATA_MASK) >> MDIO_DATA_SHIFT);
}

/*
 *	Write a PHY register on the MII of the L1E.
 */
void
ale_miibus_writereg(struct device *dev, int phy, int reg, int val)
{
	struct ale_softc *sc = (struct ale_softc *)dev;
	uint32_t v;
	int i;

	if (phy != sc->ale_phyaddr)
		return;

	CSR_WRITE_4(sc, ALE_MDIO, MDIO_OP_EXECUTE | MDIO_OP_WRITE |
	    (val & MDIO_DATA_MASK) << MDIO_DATA_SHIFT |
	    MDIO_SUP_PREAMBLE | MDIO_CLK_25_4 | MDIO_REG_ADDR(reg));

	for (i = ALE_PHY_TIMEOUT; i > 0; i--) {
		DELAY(5);
		v = CSR_READ_4(sc, ALE_MDIO);
		if ((v & (MDIO_OP_EXECUTE | MDIO_OP_BUSY)) == 0)
			break;
	}

	if (i == 0) {
		printf("%s: phy write timeout : %d\n",
		    sc->sc_dev.dv_xname, reg);
	}

	return;
}

void
ale_miibus_statchg(struct device *dev)
{
	struct ale_softc *sc = (struct ale_softc *)dev;
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	struct mii_data *mii;
	uint32_t reg;

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return;

	mii = &sc->sc_miibus;

	sc->ale_flags &= ~ALE_FLAG_LINK;
	if ((mii->mii_media_status & (IFM_ACTIVE | IFM_AVALID)) ==
	    (IFM_ACTIVE | IFM_AVALID)) {
		switch (IFM_SUBTYPE(mii->mii_media_active)) {
		case IFM_10_T:
		case IFM_100_TX:
			sc->ale_flags |= ALE_FLAG_LINK;
			break;

		case IFM_1000_T:
			if ((sc->ale_flags & ALE_FLAG_FASTETHER) == 0)
				sc->ale_flags |= ALE_FLAG_LINK;
			break;

		default:
			break;
		}
	}

	/* Stop Rx/Tx MACs. */
	ale_stop_mac(sc);

	/* Program MACs with resolved speed/duplex/flow-control. */
	if ((sc->ale_flags & ALE_FLAG_LINK) != 0) {
		ale_mac_config(sc);
		/* Reenable Tx/Rx MACs. */
		reg = CSR_READ_4(sc, ALE_MAC_CFG);
		reg |= MAC_CFG_TX_ENB | MAC_CFG_RX_ENB;
		CSR_WRITE_4(sc, ALE_MAC_CFG, reg);
	}
}

void
ale_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct ale_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->sc_miibus;

	mii_pollstat(mii);
	ifmr->ifm_status = mii->mii_media_status;
	ifmr->ifm_active = mii->mii_media_active;
}

int
ale_mediachange(struct ifnet *ifp)
{
	struct ale_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->sc_miibus;
	int error;

	if (mii->mii_instance != 0) {
		struct mii_softc *miisc;

		LIST_FOREACH(miisc, &mii->mii_phys, mii_list)
			mii_phy_reset(miisc);
	}
	error = mii_mediachg(mii);

	return (error);
}

void
ale_get_macaddr(struct ale_softc *sc)
{
	uint32_t ea[2], reg;
	int i, vpdc;

	reg = CSR_READ_4(sc, ALE_SPI_CTRL);
	if ((reg & SPI_VPD_ENB) != 0) {
		reg &= ~SPI_VPD_ENB;
		CSR_WRITE_4(sc, ALE_SPI_CTRL, reg);
	}

	vpdc = pci_get_vpdcap_ptr(sc->ale_dev);
	if (vpdc) {
		/*
		 * PCI VPD capability found, let TWSI reload EEPROM.
		 * This will set ethernet address of controller.
		 */
		CSR_WRITE_4(sc, ALE_TWSI_CTRL, CSR_READ_4(sc, ALE_TWSI_CTRL) |
		    TWSI_CTRL_SW_LD_START);
		for (i = 100; i > 0; i--) {
			DELAY(1000);
			reg = CSR_READ_4(sc, ALE_TWSI_CTRL);
			if ((reg & TWSI_CTRL_SW_LD_START) == 0)
				break;
		}
		if (i == 0)
			printf("%s: reloading EEPROM timeout!\n",
			    sc->sc_dev.dv_xname);
	} else {
		if (aledebug)
			printf("%s: PCI VPD capability not found!\n",
			    sc->sc_dev.dv_xname);
	}

	ea[0] = CSR_READ_4(sc, ALE_PAR0);
	ea[1] = CSR_READ_4(sc, ALE_PAR1);
	sc->ale_eaddr[0] = (ea[1] >> 8) & 0xFF;
	sc->ale_eaddr[1] = (ea[1] >> 0) & 0xFF;
	sc->ale_eaddr[2] = (ea[0] >> 24) & 0xFF;
	sc->ale_eaddr[3] = (ea[0] >> 16) & 0xFF;
	sc->ale_eaddr[4] = (ea[0] >> 8) & 0xFF;
	sc->ale_eaddr[5] = (ea[0] >> 0) & 0xFF;
}

void
ale_phy_reset(struct ale_softc *sc)
{
	/* Reset magic from Linux. */
	CSR_WRITE_2(sc, ALE_GPHY_CTRL,
	    GPHY_CTRL_HIB_EN | GPHY_CTRL_HIB_PULSE | GPHY_CTRL_SEL_ANA_RESET |
	    GPHY_CTRL_PHY_PLL_ON);
	DELAY(1000);
	CSR_WRITE_2(sc, ALE_GPHY_CTRL,
	    GPHY_CTRL_EXT_RESET | GPHY_CTRL_HIB_EN | GPHY_CTRL_HIB_PULSE |
	    GPHY_CTRL_SEL_ANA_RESET | GPHY_CTRL_PHY_PLL_ON);
	DELAY(1000);

#define	ATPHY_DBG_ADDR		0x1D
#define	ATPHY_DBG_DATA		0x1E

	/* Enable hibernation mode. */
	ale_miibus_writereg(sc->ale_dev, sc->ale_phyaddr,
	    ATPHY_DBG_ADDR, 0x0B);
	ale_miibus_writereg(sc->ale_dev, sc->ale_phyaddr,
	    ATPHY_DBG_DATA, 0xBC00);
	/* Set Class A/B for all modes. */
	ale_miibus_writereg(sc->ale_dev, sc->ale_phyaddr,
	    ATPHY_DBG_ADDR, 0x00);
	ale_miibus_writereg(sc->ale_dev, sc->ale_phyaddr,
	    ATPHY_DBG_DATA, 0x02EF);
	/* Enable 10BT power saving. */
	ale_miibus_writereg(sc->ale_dev, sc->ale_phyaddr,
	    ATPHY_DBG_ADDR, 0x12);
	ale_miibus_writereg(sc->ale_dev, sc->ale_phyaddr,
	    ATPHY_DBG_DATA, 0x4C04);
	/* Adjust 1000T power. */
	ale_miibus_writereg(sc->ale_dev, sc->ale_phyaddr,
	    ATPHY_DBG_ADDR, 0x04);
	ale_miibus_writereg(sc->ale_dev, sc->ale_phyaddr,
	    ATPHY_DBG_ADDR, 0x8BBB);
	/* 10BT center tap voltage. */
	ale_miibus_writereg(sc->ale_dev, sc->ale_phyaddr,
	    ATPHY_DBG_ADDR, 0x05);
	ale_miibus_writereg(sc->ale_dev, sc->ale_phyaddr,
	    ATPHY_DBG_ADDR, 0x2C46);

#undef	ATPHY_DBG_ADDR
#undef	ATPHY_DBG_DATA
	DELAY(1000);
}

struct ale_dmamap_arg {
	bus_addr_t	ale_busaddr;
};

void
ale_dmamap_cb(void *arg, bus_dma_segment_t *segs, int nsegs, int error)
{
	struct ale_dmamap_arg *ctx;

	if (error != 0)
		return;

	KASSERT(nsegs == 1, ("%s: %d segments returned!", __func__, nsegs));

	ctx = (struct ale_dmamap_arg *)arg;
	ctx->ale_busaddr = segs[0].ds_addr;
}

/*
 * Tx descriptors/RXF0/CMB DMA blocks share ALE_DESC_ADDR_HI register
 * which specifies high address region of DMA blocks. Therefore these
 * blocks should have the same high address of given 4GB address
 * space(i.e. crossing 4GB boundary is not allowed).
 */
int
ale_check_boundary(struct ale_softc *sc)
{
	bus_addr_t rx_cmb_end[ALE_RX_PAGES], tx_cmb_end;
	bus_addr_t rx_page_end[ALE_RX_PAGES], tx_ring_end;

	rx_page_end[0] = sc->ale_cdata.ale_rx_page[0].page_paddr +
	    sc->ale_pagesize;
	rx_page_end[1] = sc->ale_cdata.ale_rx_page[1].page_paddr +
	    sc->ale_pagesize;
	tx_ring_end = sc->ale_cdata.ale_tx_ring_paddr + ALE_TX_RING_SZ;
	tx_cmb_end = sc->ale_cdata.ale_tx_cmb_paddr + ALE_TX_CMB_SZ;
	rx_cmb_end[0] = sc->ale_cdata.ale_rx_page[0].cmb_paddr + ALE_RX_CMB_SZ;
	rx_cmb_end[1] = sc->ale_cdata.ale_rx_page[1].cmb_paddr + ALE_RX_CMB_SZ;

	if ((ALE_ADDR_HI(tx_ring_end) !=
	    ALE_ADDR_HI(sc->ale_cdata.ale_tx_ring_paddr)) ||
	    (ALE_ADDR_HI(rx_page_end[0]) !=
	    ALE_ADDR_HI(sc->ale_cdata.ale_rx_page[0].page_paddr)) ||
	    (ALE_ADDR_HI(rx_page_end[1]) !=
	    ALE_ADDR_HI(sc->ale_cdata.ale_rx_page[1].page_paddr)) ||
	    (ALE_ADDR_HI(tx_cmb_end) !=
	    ALE_ADDR_HI(sc->ale_cdata.ale_tx_cmb_paddr)) ||
	    (ALE_ADDR_HI(rx_cmb_end[0]) !=
	    ALE_ADDR_HI(sc->ale_cdata.ale_rx_page[0].cmb_paddr)) ||
	    (ALE_ADDR_HI(rx_cmb_end[1]) !=
	    ALE_ADDR_HI(sc->ale_cdata.ale_rx_page[1].cmb_paddr)))
		return (EFBIG);

	if ((ALE_ADDR_HI(tx_ring_end) != ALE_ADDR_HI(rx_page_end[0])) ||
	    (ALE_ADDR_HI(tx_ring_end) != ALE_ADDR_HI(rx_page_end[1])) ||
	    (ALE_ADDR_HI(tx_ring_end) != ALE_ADDR_HI(rx_cmb_end[0])) ||
	    (ALE_ADDR_HI(tx_ring_end) != ALE_ADDR_HI(rx_cmb_end[1])) ||
	    (ALE_ADDR_HI(tx_ring_end) != ALE_ADDR_HI(tx_cmb_end)))
		return (EFBIG);

	return (0);
}

int
ale_dma_alloc(struct ale_softc *sc)
{
	struct ale_txdesc *txd;
	bus_addr_t lowaddr;
	struct ale_dmamap_arg ctx;
	int error, guard_size, i;

	if ((sc->ale_flags & ALE_FLAG_JUMBO) != 0)
		guard_size = ALE_JUMBO_FRAMELEN;
	else
		guard_size = ALE_MAX_FRAMELEN;
	sc->ale_pagesize = roundup(guard_size + ALE_RX_PAGE_SZ,
	    ALE_RX_PAGE_ALIGN);
	lowaddr = BUS_SPACE_MAXADDR;
again:
	/* Create parent DMA tag. */
	error = bus_dma_tag_create(
	    NULL,			/* parent */
	    1, 0,			/* alignment, boundary */
	    lowaddr,			/* lowaddr */
	    BUS_SPACE_MAXADDR,		/* highaddr */
	    NULL, NULL,			/* filter, filterarg */
	    BUS_SPACE_MAXSIZE_32BIT,	/* maxsize */
	    0,				/* nsegments */
	    BUS_SPACE_MAXSIZE_32BIT,	/* maxsegsize */
	    0,				/* flags */
	    &sc->ale_cdata.ale_parent_tag);
	if (error != 0) {
		printf("%s: could not create parent DMA tag.\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	/* Create DMA tag for Tx descriptor ring. */
	error = bus_dma_tag_create(
	    sc->ale_cdata.ale_parent_tag, /* parent */
	    ALE_TX_RING_ALIGN, 0,	/* alignment, boundary */
	    BUS_SPACE_MAXADDR,		/* lowaddr */
	    BUS_SPACE_MAXADDR,		/* highaddr */
	    NULL, NULL,			/* filter, filterarg */
	    ALE_TX_RING_SZ,		/* maxsize */
	    1,				/* nsegments */
	    ALE_TX_RING_SZ,		/* maxsegsize */
	    0,				/* flags */
	    &sc->ale_cdata.ale_tx_ring_tag);
	if (error != 0) {
		printf("%s: could not create Tx ring DMA tag.\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	/* Create DMA tag for Rx pages. */
	for (i = 0; i < ALE_RX_PAGES; i++) {
		error = bus_dma_tag_create(
		    sc->ale_cdata.ale_parent_tag, /* parent */
		    ALE_RX_PAGE_ALIGN, 0,	/* alignment, boundary */
		    BUS_SPACE_MAXADDR,		/* lowaddr */
		    BUS_SPACE_MAXADDR,		/* highaddr */
		    NULL, NULL,			/* filter, filterarg */
		    sc->ale_pagesize,		/* maxsize */
		    1,				/* nsegments */
		    sc->ale_pagesize,		/* maxsegsize */
		    0,				/* flags */
		    &sc->ale_cdata.ale_rx_page[i].page_tag);
		if (error != 0) {
			printf("%s: could not create Rx page %d DMA tag.\n",
			    sc->sc_dev.dv_xname, i);
			goto fail;
		}
	}

	/* Create DMA tag for Tx coalescing message block. */
	error = bus_dma_tag_create(
	    sc->ale_cdata.ale_parent_tag, /* parent */
	    ALE_CMB_ALIGN, 0,		/* alignment, boundary */
	    BUS_SPACE_MAXADDR,		/* lowaddr */
	    BUS_SPACE_MAXADDR,		/* highaddr */
	    NULL, NULL,			/* filter, filterarg */
	    ALE_TX_CMB_SZ,		/* maxsize */
	    1,				/* nsegments */
	    ALE_TX_CMB_SZ,		/* maxsegsize */
	    0,				/* flags */
	    &sc->ale_cdata.ale_tx_cmb_tag);
	if (error != 0) {
		printf("%s: could not create Tx CMB DMA tag.\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	/* Create DMA tag for Rx coalescing message block. */
	for (i = 0; i < ALE_RX_PAGES; i++) {
		error = bus_dma_tag_create(
		    sc->ale_cdata.ale_parent_tag, /* parent */
		    ALE_CMB_ALIGN, 0,		/* alignment, boundary */
		    BUS_SPACE_MAXADDR,		/* lowaddr */
		    BUS_SPACE_MAXADDR,		/* highaddr */
		    NULL, NULL,			/* filter, filterarg */
		    ALE_RX_CMB_SZ,		/* maxsize */
		    1,				/* nsegments */
		    ALE_RX_CMB_SZ,		/* maxsegsize */
		    0,				/* flags */
		    &sc->ale_cdata.ale_rx_page[i].cmb_tag);
		if (error != 0) {
			printf("could not create Rx page %d CMB DMA tag.\n",
			    sc->sc_dev.dv_xname, i);
			goto fail;
		}
	}

	/* Allocate DMA'able memory and load the DMA map for Tx ring. */
	error = bus_dmamem_alloc(sc->ale_cdata.ale_tx_ring_tag,
	    (void **)&sc->ale_cdata.ale_tx_ring,
	    BUS_DMA_WAITOK | BUS_DMA_ZERO,
	    &sc->ale_cdata.ale_tx_ring_map);
	if (error != 0) {
		printf("%s: could not allocate DMA'able memory for Tx ring.\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}
	ctx.ale_busaddr = 0;
	error = bus_dmamap_load(sc->ale_cdata.ale_tx_ring_tag,
	    sc->ale_cdata.ale_tx_ring_map, sc->ale_cdata.ale_tx_ring,
	    ALE_TX_RING_SZ, ale_dmamap_cb, &ctx, 0);
	if (error != 0 || ctx.ale_busaddr == 0) {
		printf("%s: could not load DMA'able memory for Tx ring.\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}
	sc->ale_cdata.ale_tx_ring_paddr = ctx.ale_busaddr;

	/* Rx pages. */
	for (i = 0; i < ALE_RX_PAGES; i++) {
		error = bus_dmamem_alloc(sc->ale_cdata.ale_rx_page[i].page_tag,
		    (void **)&sc->ale_cdata.ale_rx_page[i].page_addr,
		    BUS_DMA_WAITOK | BUS_DMA_ZERO,
		    &sc->ale_cdata.ale_rx_page[i].page_map);
		if (error != 0) {
			printf("could not allocate DMA'able memory for "
			    "Rx page %d.\n", sc->sc_dev.dv_xname, i);
			goto fail;
		}
		ctx.ale_busaddr = 0;
		error = bus_dmamap_load(sc->ale_cdata.ale_rx_page[i].page_tag,
		    sc->ale_cdata.ale_rx_page[i].page_map,
		    sc->ale_cdata.ale_rx_page[i].page_addr,
		    sc->ale_pagesize, ale_dmamap_cb, &ctx, 0);
		if (error != 0 || ctx.ale_busaddr == 0) {
			printf("could not load DMA'able memory for "
			    "Rx page %d.\n", sc->sc_dev.dv_xname, i);
			goto fail;
		}
		sc->ale_cdata.ale_rx_page[i].page_paddr = ctx.ale_busaddr;
	}

	/* Tx CMB. */
	error = bus_dmamem_alloc(sc->ale_cdata.ale_tx_cmb_tag,
	    (void **)&sc->ale_cdata.ale_tx_cmb,
	    BUS_DMA_WAITOK | BUS_DMA_ZERO,
	    &sc->ale_cdata.ale_tx_cmb_map);
	if (error != 0) {
		printf("%s: could not allocate DMA'able memory for Tx CMB.\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}
	ctx.ale_busaddr = 0;
	error = bus_dmamap_load(sc->ale_cdata.ale_tx_cmb_tag,
	    sc->ale_cdata.ale_tx_cmb_map, sc->ale_cdata.ale_tx_cmb,
	    ALE_TX_CMB_SZ, ale_dmamap_cb, &ctx, 0);
	if (error != 0 || ctx.ale_busaddr == 0) {
		printf("%s: could not load DMA'able memory for Tx CMB.\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}
	sc->ale_cdata.ale_tx_cmb_paddr = ctx.ale_busaddr;

	/* Rx CMB. */
	for (i = 0; i < ALE_RX_PAGES; i++) {
		error = bus_dmamem_alloc(sc->ale_cdata.ale_rx_page[i].cmb_tag,
		    (void **)&sc->ale_cdata.ale_rx_page[i].cmb_addr,
		    BUS_DMA_WAITOK | BUS_DMA_ZERO,
		    &sc->ale_cdata.ale_rx_page[i].cmb_map);
		if (error != 0) {
			printf("%s: could not allocate DMA'able memory for "
			    "Rx page %d CMB.\n", sc->sc_dev.dv_xname, i);
			goto fail;
		}
		ctx.ale_busaddr = 0;
		error = bus_dmamap_load(sc->ale_cdata.ale_rx_page[i].cmb_tag,
		    sc->ale_cdata.ale_rx_page[i].cmb_map,
		    sc->ale_cdata.ale_rx_page[i].cmb_addr,
		    ALE_RX_CMB_SZ, ale_dmamap_cb, &ctx, 0);
		if (error != 0 || ctx.ale_busaddr == 0) {
			printf("%s: could not load DMA'able memory for Rx "
			    "page %d CMB.\n", sc->sc_dev.dv_xname, i);
			goto fail;
		}
		sc->ale_cdata.ale_rx_page[i].cmb_paddr = ctx.ale_busaddr;
	}

	/*
	 * Tx descriptors/RXF0/CMB DMA blocks share the same
	 * high address region of 64bit DMA address space.
	 */
	if (lowaddr != BUS_SPACE_MAXADDR_32BIT &&
	    (error = ale_check_boundary(sc)) != 0) {
		printf("4GB boundary crossed, switching to 32bit DMA "
		    "addressing mode.\n",sc->sc_dev.dv_xname);
		ale_dma_free(sc);
		/*
		 * Limit max allowable DMA address space to 32bit
		 * and try again.
		 */
		lowaddr = BUS_SPACE_MAXADDR_32BIT;
		goto again;
	}

	/*
	 * Create Tx buffer parent tag.
	 * AR81xx allows 64bit DMA addressing of Tx buffers so it
	 * needs separate parent DMA tag as parent DMA address space
	 * could be restricted to be within 32bit address space by
	 * 4GB boundary crossing.
	 */
	error = bus_dma_tag_create(
	    NULL,			/* parent */
	    1, 0,			/* alignment, boundary */
	    BUS_SPACE_MAXADDR,		/* lowaddr */
	    BUS_SPACE_MAXADDR,		/* highaddr */
	    NULL, NULL,			/* filter, filterarg */
	    BUS_SPACE_MAXSIZE_32BIT,	/* maxsize */
	    0,				/* nsegments */
	    BUS_SPACE_MAXSIZE_32BIT,	/* maxsegsize */
	    0,				/* flags */
	    &sc->ale_cdata.ale_buffer_tag);
	if (error != 0) {
		printf("%s: could not create parent buffer DMA tag.\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	/* Create DMA tag for Tx buffers. */
	error = bus_dma_tag_create(
	    sc->ale_cdata.ale_buffer_tag, /* parent */
	    1, 0,			/* alignment, boundary */
	    BUS_SPACE_MAXADDR,		/* lowaddr */
	    BUS_SPACE_MAXADDR,		/* highaddr */
	    NULL, NULL,			/* filter, filterarg */
	    ALE_TSO_MAXSIZE,		/* maxsize */
	    ALE_MAXTXSEGS,		/* nsegments */
	    ALE_TSO_MAXSEGSIZE,		/* maxsegsize */
	    0,				/* flags */
	    &sc->ale_cdata.ale_tx_tag);
	if (error != 0) {
		printf("%s: could not create Tx DMA tag.\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	/* Create DMA maps for Tx buffers. */
	for (i = 0; i < ALE_TX_RING_CNT; i++) {
		txd = &sc->ale_cdata.ale_txdesc[i];
		txd->tx_m = NULL;
		txd->tx_dmamap = NULL;
		error = bus_dmamap_create(sc->ale_cdata.ale_tx_tag, 0,
		    &txd->tx_dmamap);
		if (error != 0) {
			printf("%s: could not create Tx dmamap.\n",
				sc->sc_dev.dv_xname);
			goto fail;
		}
	}
fail:
	return (error);
}

void
ale_dma_free(struct ale_softc *sc)
{
	struct ale_txdesc *txd;
	int i;

	/* Tx buffers. */
	if (sc->ale_cdata.ale_tx_tag != NULL) {
		for (i = 0; i < ALE_TX_RING_CNT; i++) {
			txd = &sc->ale_cdata.ale_txdesc[i];
			if (txd->tx_dmamap != NULL) {
				bus_dmamap_destroy(sc->ale_cdata.ale_tx_tag,
				    txd->tx_dmamap);
				txd->tx_dmamap = NULL;
			}
		}
		bus_dma_tag_destroy(sc->ale_cdata.ale_tx_tag);
		sc->ale_cdata.ale_tx_tag = NULL;
	}
	/* Tx descriptor ring. */
	if (sc->ale_cdata.ale_tx_ring_tag != NULL) {
		if (sc->ale_cdata.ale_tx_ring_map != NULL)
			bus_dmamap_unload(sc->ale_cdata.ale_tx_ring_tag,
			    sc->ale_cdata.ale_tx_ring_map);
		if (sc->ale_cdata.ale_tx_ring_map != NULL &&
		    sc->ale_cdata.ale_tx_ring != NULL)
			bus_dmamem_free(sc->ale_cdata.ale_tx_ring_tag,
			    sc->ale_cdata.ale_tx_ring,
			    sc->ale_cdata.ale_tx_ring_map);
		sc->ale_cdata.ale_tx_ring = NULL;
		sc->ale_cdata.ale_tx_ring_map = NULL;
		bus_dma_tag_destroy(sc->ale_cdata.ale_tx_ring_tag);
		sc->ale_cdata.ale_tx_ring_tag = NULL;
	}
	/* Rx page block. */
	for (i = 0; i < ALE_RX_PAGES; i++) {
		if (sc->ale_cdata.ale_rx_page[i].page_tag != NULL) {
			if (sc->ale_cdata.ale_rx_page[i].page_map != NULL)
				bus_dmamap_unload(
				    sc->ale_cdata.ale_rx_page[i].page_tag,
				    sc->ale_cdata.ale_rx_page[i].page_map);
			if (sc->ale_cdata.ale_rx_page[i].page_map != NULL &&
			    sc->ale_cdata.ale_rx_page[i].page_addr != NULL)
				bus_dmamem_free(
				    sc->ale_cdata.ale_rx_page[i].page_tag,
				    sc->ale_cdata.ale_rx_page[i].page_addr,
				    sc->ale_cdata.ale_rx_page[i].page_map);
			sc->ale_cdata.ale_rx_page[i].page_addr = NULL;
			sc->ale_cdata.ale_rx_page[i].page_map = NULL;
			bus_dma_tag_destroy(
			    sc->ale_cdata.ale_rx_page[i].page_tag);
			sc->ale_cdata.ale_rx_page[i].page_tag = NULL;
		}
	}
	/* Rx CMB. */
	for (i = 0; i < ALE_RX_PAGES; i++) {
		if (sc->ale_cdata.ale_rx_page[i].cmb_tag != NULL) {
			if (sc->ale_cdata.ale_rx_page[i].cmb_map != NULL)
				bus_dmamap_unload(
				    sc->ale_cdata.ale_rx_page[i].cmb_tag,
				    sc->ale_cdata.ale_rx_page[i].cmb_map);
			if (sc->ale_cdata.ale_rx_page[i].cmb_map != NULL &&
			    sc->ale_cdata.ale_rx_page[i].cmb_addr != NULL)
				bus_dmamem_free(
				    sc->ale_cdata.ale_rx_page[i].cmb_tag,
				    sc->ale_cdata.ale_rx_page[i].cmb_addr,
				    sc->ale_cdata.ale_rx_page[i].cmb_map);
			sc->ale_cdata.ale_rx_page[i].cmb_addr = NULL;
			sc->ale_cdata.ale_rx_page[i].cmb_map = NULL;
			bus_dma_tag_destroy(
			    sc->ale_cdata.ale_rx_page[i].cmb_tag);
			sc->ale_cdata.ale_rx_page[i].cmb_tag = NULL;
		}
	}
	/* Tx CMB. */
	if (sc->ale_cdata.ale_tx_cmb_tag != NULL) {
		if (sc->ale_cdata.ale_tx_cmb_map != NULL)
			bus_dmamap_unload(sc->ale_cdata.ale_tx_cmb_tag,
			    sc->ale_cdata.ale_tx_cmb_map);
		if (sc->ale_cdata.ale_tx_cmb_map != NULL &&
		    sc->ale_cdata.ale_tx_cmb != NULL)
			bus_dmamem_free(sc->ale_cdata.ale_tx_cmb_tag,
			    sc->ale_cdata.ale_tx_cmb,
			    sc->ale_cdata.ale_tx_cmb_map);
		sc->ale_cdata.ale_tx_cmb = NULL;
		sc->ale_cdata.ale_tx_cmb_map = NULL;
		bus_dma_tag_destroy(sc->ale_cdata.ale_tx_cmb_tag);
		sc->ale_cdata.ale_tx_cmb_tag = NULL;
	}
	if (sc->ale_cdata.ale_buffer_tag != NULL) {
		bus_dma_tag_destroy(sc->ale_cdata.ale_buffer_tag);
		sc->ale_cdata.ale_buffer_tag = NULL;
	}
	if (sc->ale_cdata.ale_parent_tag != NULL) {
		bus_dma_tag_destroy(sc->ale_cdata.ale_parent_tag);
		sc->ale_cdata.ale_parent_tag = NULL;
	}
}

int
ale_shutdown(struct device *dev)
{
	return (ale_suspend(dev));
}

#ifdef notyet

/*
 * Note, this driver resets the link speed to 10/100Mbps by
 * restarting auto-negotiation in suspend/shutdown phase but we
 * don't know whether that auto-negotiation would succeed or not
 * as driver has no control after powering off/suspend operation.
 * If the renegotiation fail WOL may not work. Running at 1Gbps
 * will draw more power than 375mA at 3.3V which is specified in
 * PCI specification and that would result in complete
 * shutdowning power to ethernet controller.
 *
 * TODO
 * Save current negotiated media speed/duplex/flow-control to
 * softc and restore the same link again after resuming. PHY
 * handling such as power down/resetting to 100Mbps may be better
 * handled in suspend method in phy driver.
 */
void
ale_setlinkspeed(struct ale_softc *sc)
{
	struct mii_data *mii;
	int aneg, i;

	mii = &sc->sc_miibus;
	mii_pollstat(mii);
	aneg = 0;
	if ((mii->mii_media_status & (IFM_ACTIVE | IFM_AVALID)) ==
	    (IFM_ACTIVE | IFM_AVALID)) {
		switch IFM_SUBTYPE(mii->mii_media_active) {
		case IFM_10_T:
		case IFM_100_TX:
			return;
		case IFM_1000_T:
			aneg++;
			break;
		default:
			break;
		}
	}
	ale_miibus_writereg(sc->ale_dev, sc->ale_phyaddr, MII_100T2CR, 0);
	ale_miibus_writereg(sc->ale_dev, sc->ale_phyaddr,
	    MII_ANAR, ANAR_TX_FD | ANAR_TX | ANAR_10_FD | ANAR_10 | ANAR_CSMA);
	ale_miibus_writereg(sc->ale_dev, sc->ale_phyaddr,
	    MII_BMCR, BMCR_RESET | BMCR_AUTOEN | BMCR_STARTNEG);
	DELAY(1000);
	if (aneg != 0) {
		/*
		 * Poll link state until ale(4) get a 10/100Mbps link.
		 */
		for (i = 0; i < MII_ANEGTICKS_GIGE; i++) {
			mii_pollstat(mii);
			if ((mii->mii_media_status & (IFM_ACTIVE | IFM_AVALID))
			    == (IFM_ACTIVE | IFM_AVALID)) {
				switch (IFM_SUBTYPE(
				    mii->mii_media_active)) {
				case IFM_10_T:
				case IFM_100_TX:
					ale_mac_config(sc);
					return;
				default:
					break;
				}
			}
			ALE_UNLOCK(sc);
			pause("alelnk", hz);
			ALE_LOCK(sc);
		}
		if (i == MII_ANEGTICKS_GIGE)
			printf("%s: establishing a link failed, WOL may not "
			    "work!", sc->sc_dev.dv_xname);
	}
	/*
	 * No link, force MAC to have 100Mbps, full-duplex link.
	 * This is the last resort and may/may not work.
	 */
	mii->mii_media_status = IFM_AVALID | IFM_ACTIVE;
	mii->mii_media_active = IFM_ETHER | IFM_100_TX | IFM_FDX;
	ale_mac_config(sc);
}

void
ale_setwol(struct ale_softc *sc)
{
	struct ifnet *ifp;
	uint32_t reg, pmcs;
	uint16_t pmstat;
	int pmc;

	ALE_LOCK_ASSERT(sc);

	if (pci_find_extcap(sc->ale_dev, PCIY_PMG, &pmc) != 0) {
		/* Disable WOL. */
		CSR_WRITE_4(sc, ALE_WOL_CFG, 0);
		reg = CSR_READ_4(sc, ALE_PCIE_PHYMISC);
		reg |= PCIE_PHYMISC_FORCE_RCV_DET;
		CSR_WRITE_4(sc, ALE_PCIE_PHYMISC, reg);
		/* Force PHY power down. */
		CSR_WRITE_2(sc, ALE_GPHY_CTRL,
		    GPHY_CTRL_EXT_RESET | GPHY_CTRL_HIB_EN |
		    GPHY_CTRL_HIB_PULSE | GPHY_CTRL_PHY_PLL_ON |
		    GPHY_CTRL_SEL_ANA_RESET | GPHY_CTRL_PHY_IDDQ |
		    GPHY_CTRL_PCLK_SEL_DIS | GPHY_CTRL_PWDOWN_HW);
		return;
	}

	ifp = sc->ale_ifp;
	if ((ifp->if_capenable & IFCAP_WOL) != 0) {
		if ((sc->ale_flags & ALE_FLAG_FASTETHER) == 0)
			ale_setlinkspeed(sc);
	}

	pmcs = 0;
	if ((ifp->if_capenable & IFCAP_WOL_MAGIC) != 0)
		pmcs |= WOL_CFG_MAGIC | WOL_CFG_MAGIC_ENB;
	CSR_WRITE_4(sc, ALE_WOL_CFG, pmcs);
	reg = CSR_READ_4(sc, ALE_MAC_CFG);
	reg &= ~(MAC_CFG_DBG | MAC_CFG_PROMISC | MAC_CFG_ALLMULTI |
	    MAC_CFG_BCAST);
	if ((ifp->if_capenable & IFCAP_WOL_MCAST) != 0)
		reg |= MAC_CFG_ALLMULTI | MAC_CFG_BCAST;
	if ((ifp->if_capenable & IFCAP_WOL) != 0)
		reg |= MAC_CFG_RX_ENB;
	CSR_WRITE_4(sc, ALE_MAC_CFG, reg);

	if ((ifp->if_capenable & IFCAP_WOL) == 0) {
		/* WOL disabled, PHY power down. */
		reg = CSR_READ_4(sc, ALE_PCIE_PHYMISC);
		reg |= PCIE_PHYMISC_FORCE_RCV_DET;
		CSR_WRITE_4(sc, ALE_PCIE_PHYMISC, reg);
		CSR_WRITE_2(sc, ALE_GPHY_CTRL,
		    GPHY_CTRL_EXT_RESET | GPHY_CTRL_HIB_EN |
		    GPHY_CTRL_HIB_PULSE | GPHY_CTRL_SEL_ANA_RESET |
		    GPHY_CTRL_PHY_IDDQ | GPHY_CTRL_PCLK_SEL_DIS |
		    GPHY_CTRL_PWDOWN_HW);
	}
	/* Request PME. */
	pmstat = pci_conf_read(sc->sc_pct, sc->sc_pcitag,
	    pmc + PCIR_POWER_STATUS)>>16;
	pmstat &= ~(PCIM_PSTAT_PME | PCIM_PSTAT_PMEENABLE);
	if ((ifp->if_capenable & IFCAP_WOL) != 0)
		pmstat |= PCIM_PSTAT_PME | PCIM_PSTAT_PMEENABLE;
	pci_conf_write(sc->sc_pct, sc->sc_pcitag, pmc + PCIR_POWER_STATUS, pmstat << 16);
}

#endif	/* notyet */

int
ale_suspend(struct device *dev)
{
	struct ale_softc *sc = device_get_softc(dev);
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;

	lwkt_serialize_enter(ifp->if_serializer);
	ale_stop(sc);
#ifdef notyet
	ale_setwol(sc);
#endif
	lwkt_serialize_exit(ifp->if_serializer);
	return (0);
}

int
ale_resume(struct device *dev)
{
	struct ale_softc *sc = device_get_softc(dev);
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	uint16_t cmd;

	lwkt_serialize_enter(ifp->if_serializer);

	/*
	 * Clear INTx emulation disable for hardwares that
	 * is set in resume event. From Linux.
	 */
	cmd = pci_conf_read(sc->sc_pct, sc->sc_pcitag, PCIR_COMMAND) >> 16;
	if ((cmd & 0x0400) != 0) {
		cmd &= ~0x0400;
		pci_conf_write(sc->sc_pct, sc->sc_pcitag, PCIR_COMMAND,
		    cmd << 16);
	}

#ifdef notyet
	if (pci_find_extcap(sc->ale_dev, PCIY_PMG, &pmc) == 0) {
		uint16_t pmstat;
		int pmc;

		/* Disable PME and clear PME status. */
		pmstat = pci_conf_read(sc->sc_pct, sc->sc_pcitag,
		    pmc + PCIR_POWER_STATUS) >> 16;
		if ((pmstat & PCIM_PSTAT_PMEENABLE) != 0) {
			pmstat &= ~PCIM_PSTAT_PMEENABLE;
			pci_conf_write(sc->sc_pct, sc->sc_pcitag,
			    pmc + PCIR_POWER_STATUS, pmstat << 16);
		}
	}
#endif

	/* Reset PHY. */
	ale_phy_reset(sc);
	if ((ifp->if_flags & IFF_UP) != 0)
		ale_init(sc);

	lwkt_serialize_exit(ifp->if_serializer);
	return (0);
}

int
ale_encap(struct ale_softc *sc, struct mbuf **m_head)
{
	struct ale_txdesc *txd, *txd_last;
	struct tx_desc *desc;
	struct mbuf *m;
	bus_dma_segment_t txsegs[ALE_MAXTXSEGS];
	struct ale_dmamap_ctx ctx;
	bus_dmamap_t map;
	uint32_t cflags, poff, vtag;
	int error, i, nsegs, prod, si;

	M_ASSERTPKTHDR((*m_head));

	m = *m_head;
	cflags = vtag = 0;
	poff = 0;

	si = prod = sc->ale_cdata.ale_tx_prod;
	txd = &sc->ale_cdata.ale_txdesc[prod];
	txd_last = txd;
	map = txd->tx_dmamap;

	ctx.nsegs = ALE_MAXTXSEGS;
	ctx.segs = txsegs;
	error =  bus_dmamap_load_mbuf(sc->ale_cdata.ale_tx_tag, map,
				      *m_head, ale_dmamap_buf_cb, &ctx,
				      BUS_DMA_NOWAIT);
	if (error == EFBIG) {
		m = m_defrag(*m_head, MB_DONTWAIT);
		if (m == NULL) {
			m_freem(*m_head);
			*m_head = NULL;
			return (ENOMEM);
		}
		*m_head = m;

		ctx.nsegs = ALE_MAXTXSEGS;
		ctx.segs = txsegs;
		error =  bus_dmamap_load_mbuf(sc->ale_cdata.ale_tx_tag, map,
					      *m_head, ale_dmamap_buf_cb, &ctx,
					      BUS_DMA_NOWAIT);
		if (error != 0) {
			m_freem(*m_head);
			*m_head = NULL;
			return (error);
		}
	} else if (error != 0) {
		return (error);
	}
	nsegs = ctx.nsegs;

	if (nsegs == 0) {
		m_freem(*m_head);
		*m_head = NULL;
		return (EIO);
	}

	/* Check descriptor overrun. */
	if (sc->ale_cdata.ale_tx_cnt + nsegs >= ALE_TX_RING_CNT - 2) {
		bus_dmamap_unload(sc->ale_cdata.ale_tx_tag, map);
		return (ENOBUFS);
	}
	bus_dmamap_sync(sc->dmat, sc->ale_cdata.ale_tx_tag, map,
	    BUS_DMASYNC_PREWRITE);

	m = *m_head;
	/* Configure Tx checksum offload. */
	if ((m->m_pkthdr.csum_flags & ALE_CSUM_FEATURES) != 0) {
		/*
		 * AR81xx supports Tx custom checksum offload feature
		 * that offloads single 16bit checksum computation.
		 * So you can choose one among IP, TCP and UDP.
		 * Normally driver sets checksum start/insertion
		 * position from the information of TCP/UDP frame as
		 * TCP/UDP checksum takes more time than that of IP.
		 * However it seems that custom checksum offload
		 * requires 4 bytes aligned Tx buffers due to hardware
		 * bug.
		 * AR81xx also supports explicit Tx checksum computation
		 * if it is told that the size of IP header and TCP
		 * header(for UDP, the header size does not matter
		 * because it's fixed length). However with this scheme
		 * TSO does not work so you have to choose one either
		 * TSO or explicit Tx checksum offload. I chosen TSO
		 * plus custom checksum offload with work-around which
		 * will cover most common usage for this consumer
		 * ethernet controller. The work-around takes a lot of
		 * CPU cycles if Tx buffer is not aligned on 4 bytes
		 * boundary, though.
		 */
		cflags |= ALE_TD_CXSUM;
		/* Set checksum start offset. */
		cflags |= (poff << ALE_TD_CSUM_PLOADOFFSET_SHIFT);
		/* Set checksum insertion position of TCP/UDP. */
		cflags |= ((poff + m->m_pkthdr.csum_data) <<
		    ALE_TD_CSUM_XSUMOFFSET_SHIFT);
	}

	/* Configure VLAN hardware tag insertion. */
	if ((m->m_flags & M_VLANTAG) != 0) {
		vtag = ALE_TX_VLAN_TAG(m->m_pkthdr.ether_vlantag);
		vtag = ((vtag << ALE_TD_VLAN_SHIFT) & ALE_TD_VLAN_MASK);
		cflags |= ALE_TD_INSERT_VLAN_TAG;
	}

	desc = NULL;
	for (i = 0; i < nsegs; i++) {
		desc = &sc->ale_cdata.ale_tx_ring[prod];
		desc->addr = htole64(txsegs[i].ds_addr);
		desc->len = htole32(ALE_TX_BYTES(txsegs[i].ds_len) | vtag);
		desc->flags = htole32(cflags);
		sc->ale_cdata.ale_tx_cnt++;
		ALE_DESC_INC(prod, ALE_TX_RING_CNT);
	}
	/* Update producer index. */
	sc->ale_cdata.ale_tx_prod = prod;

	/* Finally set EOP on the last descriptor. */
	prod = (prod + ALE_TX_RING_CNT - 1) % ALE_TX_RING_CNT;
	desc = &sc->ale_cdata.ale_tx_ring[prod];
	desc->flags |= htole32(ALE_TD_EOP);

	/* Swap dmamap of the first and the last. */
	txd = &sc->ale_cdata.ale_txdesc[prod];
	map = txd_last->tx_dmamap;
	txd_last->tx_dmamap = txd->tx_dmamap;
	txd->tx_dmamap = map;
	txd->tx_m = m;

	/* Sync descriptors. */
	bus_dmamap_sync(sc->sc_dmat, sc->ale_cdata.ale_tx_ring_tag,
	    sc->ale_cdata.ale_tx_ring_map, BUS_DMASYNC_PREWRITE);

	return (0);
}

void
ale_start(struct ifnet *ifp)
{
        struct ale_softc *sc = ifp->if_softc;
	struct mbuf *m_head;
	int enq;

	if ((sc->ale_flags & ALE_FLAG_LINK) == 0) {
		ifq_purge(&ifp->if_snd);
		return;
	}

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	/* Reclaim transmitted frames. */
	if (sc->ale_cdata.ale_tx_cnt >= ALE_TX_DESC_HIWAT)
		ale_txeof(sc);

	enq = 0;
	while (!ifq_is_empty(&ifp->if_snd)) {
		m_head = ifq_dequeue(&ifp->if_snd, NULL);
		if (m_head == NULL)
			break;

		/*
		 * Pack the data into the transmit ring. If we
		 * don't have room, set the OACTIVE flag and wait
		 * for the NIC to drain the ring.
		 */
		if (ale_encap(sc, &m_head)) {
			if (m_head == NULL)
				break;
			ifq_prepend(&ifp->if_snd, m_head);
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}
		enq = 1;

		/*
		 * If there's a BPF listener, bounce a copy of this frame
		 * to him.
		 */
		ETHER_BPF_MTAP(ifp, m_head);
	}

	if (enq) {
		/* Kick. */
		CSR_WRITE_4(sc, ALE_MBOX_TPD_PROD_IDX,
		    sc->ale_cdata.ale_tx_prod);

		/* Set a timeout in case the chip goes out to lunch. */
		ifp->if_timer = ALE_TX_TIMEOUT;
	}
}

void
ale_watchdog(struct ifnet *ifp)
{
	struct ale_softc *sc = ifp->if_softc;

	if ((sc->ale_flags & ALE_FLAG_LINK) == 0) {
		printf("%s: watchdog timeout (lost link)\n",
		    sc->sc_dev.dv_xname);
		ifp->if_oerrors++;
		ale_init(sc);
		return;
	}

	printf("%s: watchdog timeout -- resetting\n",sc->sc_dev.dv_xname);
	ifp->if_oerrors++;
	ale_init(sc);

	if (!ifq_is_empty(&ifp->if_snd))
		if_devstart(ifp);
}

int
ale_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct ale_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->sc_miibus;
	struct *ifa = (struct ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		if (!(ifp->if_flags & IFF_RUNNING))
			ale_init(ifp);
#ifdef INET
		if (ifa->ifa_addr->sa_family == AF_INET)
			arp_ifinit(&sc->sc_arpcom, ifa);
#endif
		break;

	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			if (ifp->if_flags & IFF_RUNNING)
				ale_rxfilter(sc);
			else
				ale_init(ifp);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				ale_stop(sc);
		}
		sc->ale_if_flags = ifp->if_flags;
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		error = (cmd = SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_arpcom) :
		    ether_delmulti(ifr, &sc->sc_arpcom);
		if (error == ENETRESET) {
			if ((ifp->if_flags & IFF_RUNNING) != 0)
				ale_rxfilter(sc);
			error = 0;
		}
		break;

	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &mii->mii_media, cmd);
		break;

	default:
		error = ether_ioctl(ifp, &sc->sc_arpcom, cmd, data);
		break;
	}

	if (error == ENETRESET) {
		if (ifp->if_flags & IFF_RUNNING)
			ale_rxfilter(sc);
		error = 0;
	}
	return (error);
}

void
ale_mac_config(struct ale_softc *sc)
{
	struct mii_data *mii;
	uint32_t reg;

	mii = &sc->sc_miibus;
	reg = CSR_READ_4(sc, ALE_MAC_CFG);
	reg &= ~(MAC_CFG_FULL_DUPLEX | MAC_CFG_TX_FC | MAC_CFG_RX_FC |
	    MAC_CFG_SPEED_MASK);
	/* Reprogram MAC with resolved speed/duplex. */
	switch (IFM_SUBTYPE(mii->mii_media_active)) {
	case IFM_10_T:
	case IFM_100_TX:
		reg |= MAC_CFG_SPEED_10_100;
		break;
	case IFM_1000_T:
		reg |= MAC_CFG_SPEED_1000;
		break;
	}
	if ((IFM_OPTIONS(mii->mii_media_active) & IFM_FDX) != 0) {
		reg |= MAC_CFG_FULL_DUPLEX;
#ifdef notyet
		if ((IFM_OPTIONS(mii->mii_media_active) & IFM_ETH_TXPAUSE) != 0)
			reg |= MAC_CFG_TX_FC;
		if ((IFM_OPTIONS(mii->mii_media_active) & IFM_ETH_RXPAUSE) != 0)
			reg |= MAC_CFG_RX_FC;
#endif
	}

	CSR_WRITE_4(sc, ALE_MAC_CFG, reg);
}

void
ale_stats_clear(struct ale_softc *sc)
{
	struct smb sb;
	uint32_t *reg;
	int i;

	for (reg = &sb.rx_frames, i = 0; reg <= &sb.rx_pkts_filtered; reg++) {
		CSR_READ_4(sc, ALE_RX_MIB_BASE + i);
		i += sizeof(uint32_t);
	}
	/* Read Tx statistics. */
	for (reg = &sb.tx_frames, i = 0; reg <= &sb.tx_mcast_bytes; reg++) {
		CSR_READ_4(sc, ALE_TX_MIB_BASE + i);
		i += sizeof(uint32_t);
	}
}

void
ale_stats_update(struct ale_softc *sc)
{
	struct ale_hw_stats *stat;
	struct smb sb, *smb;
	struct ifnet *ifp;
	uint32_t *reg;
	int i;

	ifp = &sc->sc_arpcom.ac_if;
	stat = &sc->ale_stats;
	smb = &sb;

	/* Read Rx statistics. */
	for (reg = &sb.rx_frames, i = 0; reg <= &sb.rx_pkts_filtered; reg++) {
		*reg = CSR_READ_4(sc, ALE_RX_MIB_BASE + i);
		i += sizeof(uint32_t);
	}
	/* Read Tx statistics. */
	for (reg = &sb.tx_frames, i = 0; reg <= &sb.tx_mcast_bytes; reg++) {
		*reg = CSR_READ_4(sc, ALE_TX_MIB_BASE + i);
		i += sizeof(uint32_t);
	}

	/* Rx stats. */
	stat->rx_frames += smb->rx_frames;
	stat->rx_bcast_frames += smb->rx_bcast_frames;
	stat->rx_mcast_frames += smb->rx_mcast_frames;
	stat->rx_pause_frames += smb->rx_pause_frames;
	stat->rx_control_frames += smb->rx_control_frames;
	stat->rx_crcerrs += smb->rx_crcerrs;
	stat->rx_lenerrs += smb->rx_lenerrs;
	stat->rx_bytes += smb->rx_bytes;
	stat->rx_runts += smb->rx_runts;
	stat->rx_fragments += smb->rx_fragments;
	stat->rx_pkts_64 += smb->rx_pkts_64;
	stat->rx_pkts_65_127 += smb->rx_pkts_65_127;
	stat->rx_pkts_128_255 += smb->rx_pkts_128_255;
	stat->rx_pkts_256_511 += smb->rx_pkts_256_511;
	stat->rx_pkts_512_1023 += smb->rx_pkts_512_1023;
	stat->rx_pkts_1024_1518 += smb->rx_pkts_1024_1518;
	stat->rx_pkts_1519_max += smb->rx_pkts_1519_max;
	stat->rx_pkts_truncated += smb->rx_pkts_truncated;
	stat->rx_fifo_oflows += smb->rx_fifo_oflows;
	stat->rx_rrs_errs += smb->rx_rrs_errs;
	stat->rx_alignerrs += smb->rx_alignerrs;
	stat->rx_bcast_bytes += smb->rx_bcast_bytes;
	stat->rx_mcast_bytes += smb->rx_mcast_bytes;
	stat->rx_pkts_filtered += smb->rx_pkts_filtered;

	/* Tx stats. */
	stat->tx_frames += smb->tx_frames;
	stat->tx_bcast_frames += smb->tx_bcast_frames;
	stat->tx_mcast_frames += smb->tx_mcast_frames;
	stat->tx_pause_frames += smb->tx_pause_frames;
	stat->tx_excess_defer += smb->tx_excess_defer;
	stat->tx_control_frames += smb->tx_control_frames;
	stat->tx_deferred += smb->tx_deferred;
	stat->tx_bytes += smb->tx_bytes;
	stat->tx_pkts_64 += smb->tx_pkts_64;
	stat->tx_pkts_65_127 += smb->tx_pkts_65_127;
	stat->tx_pkts_128_255 += smb->tx_pkts_128_255;
	stat->tx_pkts_256_511 += smb->tx_pkts_256_511;
	stat->tx_pkts_512_1023 += smb->tx_pkts_512_1023;
	stat->tx_pkts_1024_1518 += smb->tx_pkts_1024_1518;
	stat->tx_pkts_1519_max += smb->tx_pkts_1519_max;
	stat->tx_single_colls += smb->tx_single_colls;
	stat->tx_multi_colls += smb->tx_multi_colls;
	stat->tx_late_colls += smb->tx_late_colls;
	stat->tx_excess_colls += smb->tx_excess_colls;
	stat->tx_abort += smb->tx_abort;
	stat->tx_underrun += smb->tx_underrun;
	stat->tx_desc_underrun += smb->tx_desc_underrun;
	stat->tx_lenerrs += smb->tx_lenerrs;
	stat->tx_pkts_truncated += smb->tx_pkts_truncated;
	stat->tx_bcast_bytes += smb->tx_bcast_bytes;
	stat->tx_mcast_bytes += smb->tx_mcast_bytes;

	/* Update counters in ifnet. */
	ifp->if_opackets += smb->tx_frames;

	ifp->if_collisions += smb->tx_single_colls +
	    smb->tx_multi_colls * 2 + smb->tx_late_colls +
	    smb->tx_abort * HDPX_CFG_RETRY_DEFAULT;

	/*
	 * XXX
	 * tx_pkts_truncated counter looks suspicious. It constantly
	 * increments with no sign of Tx errors. This may indicate
	 * the counter name is not correct one so I've removed the
	 * counter in output errors.
	 */
	ifp->if_oerrors += smb->tx_abort + smb->tx_late_colls +
	    smb->tx_underrun;

	ifp->if_ipackets += smb->rx_frames;

	ifp->if_ierrors += smb->rx_crcerrs + smb->rx_lenerrs +
	    smb->rx_runts + smb->rx_pkts_truncated +
	    smb->rx_fifo_oflows + smb->rx_rrs_errs +
	    smb->rx_alignerrs;
}

void
ale_intr(void *xsc)
{
	struct ale_softc *sc = xsc;
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	uint32_t status;

	status = CSR_READ_4(sc, ALE_INTR_STATUS);
	if ((status & ALE_INTRS) == 0)
		return;

	/* Acknowledge and disable interrupts. */
	CSR_WRITE_4(sc, ALE_INTR_STATUS, status | INTR_DIS_INT);

	if ((ifp->if_flags & IFF_RUNNING) != 0) {
		int error;

		error = ale_rxeof(sc);
		if (error) {
			sc->ale_stats.reset_brk_seq++;
			ale_init(sc);
			return;
		}

		if ((status & (INTR_DMA_RD_TO_RST | INTR_DMA_WR_TO_RST)) != 0) {
			if ((status & INTR_DMA_RD_TO_RST) != 0)
				printf("%s: DMA read error! -- resetting\n",
				    sc->sc_dev.dv_xname);
			if ((status & INTR_DMA_WR_TO_RST) != 0)
				printf("DMA write error! -- resetting\n",
				    sc->sc_dev.dv-xname);
			ale_init(sc);
			return;
		}

		ale_txeof(sc);
		if (!ifq_is_empty(&ifp->if_snd))
			if_devstart(ifp);
	}

	/* Re-enable interrupts. */
	CSR_WRITE_4(sc, ALE_INTR_STATUS, 0x7FFFFFFF);
}

void
ale_txeof(struct ale_softc *sc)
{
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	struct ale_txdesc *txd;
	uint32_t cons, prod;
	int prog;

	if (sc->ale_cdata.ale_tx_cnt == 0)
		return;

	bus_dmamap_sync(sc->sc_dmat, sc->ale_cdata.ale_tx_ring_tag,
	    sc->ale_cdata.ale_tx_ring_map, BUS_DMASYNC_POSTREAD);
	if ((sc->ale_flags & ALE_FLAG_TXCMB_BUG) == 0) {
		bus_dmamap_sync(sc->sc_dmat, sc->ale_cdata.ale_tx_cmb_tag,
		    sc->ale_cdata.ale_tx_cmb_map, BUS_DMASYNC_POSTREAD);
		prod = *sc->ale_cdata.ale_tx_cmb & TPD_CNT_MASK;
	} else
		prod = CSR_READ_2(sc, ALE_TPD_CONS_IDX);
	cons = sc->ale_cdata.ale_tx_cons;
	/*
	 * Go through our Tx list and free mbufs for those
	 * frames which have been transmitted.
	 */
	for (prog = 0; cons != prod; prog++,
	     ALE_DESC_INC(cons, ALE_TX_RING_CNT)) {
		if (sc->ale_cdata.ale_tx_cnt <= 0)
			break;
		prog++;
		ifp->if_flags &= ~IFF_OACTIVE;
		sc->ale_cdata.ale_tx_cnt--;
		txd = &sc->ale_cdata.ale_txdesc[cons];
		if (txd->tx_m != NULL) {
			/* Reclaim transmitted mbufs. */
			bus_dmamap_unload(sc->ale_cdata.ale_tx_tag,
			    txd->tx_dmamap);
			m_freem(txd->tx_m);
			txd->tx_m = NULL;
		}
	}

	if (prog > 0) {
		sc->ale_cdata.ale_tx_cons = cons;
		/*
		 * Unarm watchdog timer only when there is no pending
		 * Tx descriptors in queue.
		 */
		if (sc->ale_cdata.ale_tx_cnt == 0)
			ifp->if_timer = 0;
	}
}

void
ale_rx_update_page(struct ale_softc *sc, struct ale_rx_page **page,
    uint32_t length, uint32_t *prod)
{
	struct ale_rx_page *rx_page;

	rx_page = *page;
	/* Update consumer position. */
	rx_page->cons += roundup(length + sizeof(struct rx_rs),
	    ALE_RX_PAGE_ALIGN);
	if (rx_page->cons >= ALE_RX_PAGE_SZ) {
		/*
		 * End of Rx page reached, let hardware reuse
		 * this page.
		 */
		rx_page->cons = 0;
		*rx_page->cmb_addr = 0;
		bus_dmamap_sync(rx_page->cmb_tag, rx_page->cmb_map,
				BUS_DMASYNC_PREWRITE);
		CSR_WRITE_1(sc, ALE_RXF0_PAGE0 + sc->ale_cdata.ale_rx_curp,
		    RXF_VALID);
		/* Switch to alternate Rx page. */
		sc->ale_cdata.ale_rx_curp ^= 1;
		rx_page = *page =
		    &sc->ale_cdata.ale_rx_page[sc->ale_cdata.ale_rx_curp];
		/* Page flipped, sync CMB and Rx page. */
		bus_dmamap_sync(rx_page->page_tag, rx_page->page_map,
		    BUS_DMASYNC_POSTREAD);
		bus_dmamap_sync(rx_page->cmb_tag, rx_page->cmb_map,
		    BUS_DMASYNC_POSTREAD);
		/* Sync completed, cache updated producer index. */
		*prod = *rx_page->cmb_addr;
	}
}


/*
 * It seems that AR81xx controller can compute partial checksum.
 * The partial checksum value can be used to accelerate checksum
 * computation for fragmented TCP/UDP packets. Upper network stack
 * already takes advantage of the partial checksum value in IP
 * reassembly stage. But I'm not sure the correctness of the
 * partial hardware checksum assistance due to lack of data sheet.
 * In addition, the Rx feature of controller that requires copying
 * for every frames effectively nullifies one of most nice offload
 * capability of controller.
 */
void
ale_rxcsum(struct ale_softc *sc, struct mbuf *m, uint32_t status)
{
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	struct ip *ip;
	char *p;

	m->m_pkthdr.csum_flags |= CSUM_IP_CHECKED;
	if ((status & ALE_RD_IPCSUM_NOK) == 0)
		m->m_pkthdr.csum_flags |= CSUM_IP_VALID;

	if ((sc->ale_flags & ALE_FLAG_RXCSUM_BUG) == 0) {
		if (((status & ALE_RD_IPV4_FRAG) == 0) &&
		    ((status & (ALE_RD_TCP | ALE_RD_UDP)) != 0) &&
		    ((status & ALE_RD_TCP_UDPCSUM_NOK) == 0)) {
			m->m_pkthdr.csum_flags |=
			    CSUM_DATA_VALID | CSUM_PSEUDO_HDR;
			m->m_pkthdr.csum_data = 0xffff;
		}
	} else {
		if ((status & (ALE_RD_TCP | ALE_RD_UDP)) != 0 &&
		    (status & ALE_RD_TCP_UDPCSUM_NOK) == 0) {
			p = mtod(m, char *);
			p += ETHER_HDR_LEN;
			if ((status & ALE_RD_802_3) != 0)
				p += LLC_SNAPFRAMELEN;
			if ((ifp->if_capenable & IFCAP_VLAN_HWTAGGING) == 0 &&
			    (status & ALE_RD_VLAN) != 0)
				p += EVL_ENCAPLEN;
			ip = (struct ip *)p;
			if (ip->ip_off != 0 && (status & ALE_RD_IPV4_DF) == 0)
				return;
			m->m_pkthdr.csum_flags |= CSUM_DATA_VALID |
			    CSUM_PSEUDO_HDR;
			m->m_pkthdr.csum_data = 0xffff;
		}
	}
	/*
	 * Don't mark bad checksum for TCP/UDP frames
	 * as fragmented frames may always have set
	 * bad checksummed bit of frame status.
	 */
}

/* Process received frames. */
int
ale_rxeof(struct ale_softc *sc)
{
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	struct ale_rx_page *rx_page;
	struct rx_rs *rs;
	struct mbuf *m;
	uint32_t length, prod, seqno, status, vtags;
	int prog;

	rx_page = &sc->ale_cdata.ale_rx_page[sc->ale_cdata.ale_rx_curp];
	bus_dmamap_sync(sc->sc_dmat, rx_page->cmb_tag, rx_page->cmb_map,
			BUS_DMASYNC_POSTREAD);
	bus_dmamap_sync(sc->sc_dmat, rx_page->page_tag, rx_page->page_map,
			BUS_DMASYNC_POSTREAD);
	/*
	 * Don't directly access producer index as hardware may
	 * update it while Rx handler is in progress. It would
	 * be even better if there is a way to let hardware
	 * know how far driver processed its received frames.
	 * Alternatively, hardware could provide a way to disable
	 * CMB updates until driver acknowledges the end of CMB
	 * access.
	 */
	prod = *rx_page->cmb_addr;
	for (prog = 0; ; prog++) {
		if (rx_page->cons >= prod)
			break;
		rs = (struct rx_rs *)(rx_page->page_addr + rx_page->cons);
		seqno = ALE_RX_SEQNO(letoh32(rs->seqno));
		if (sc->ale_cdata.ale_rx_seqno != seqno) {
			/*
			 * Normally I believe this should not happen unless
			 * severe driver bug or corrupted memory. However
			 * it seems to happen under certain conditions which
			 * is triggered by abrupt Rx events such as initiation
			 * of bulk transfer of remote host. It's not easy to
			 * reproduce this and I doubt it could be related
			 * with FIFO overflow of hardware or activity of Tx
			 * CMB updates. I also remember similar behaviour
			 * seen on RealTek 8139 which uses resembling Rx
			 * scheme.
			 */
			if (aledebug)
				printf("%s: garbled seq: %u, expected: %u -- "
				    "resetting!\n", sc->sc_dev.dv_xname, seqno,
				    sc->ale_cdata.ale_rx_seqno);
			return (EIO);
		}
		/* Frame received. */
		sc->ale_cdata.ale_rx_seqno++;
		length = ALE_RX_BYTES(letoh32(rs->length));
		status = letoh32(rs->flags);
		if ((status & ALE_RD_ERROR) != 0) {
			/*
			 * We want to pass the following frames to upper
			 * layer regardless of error status of Rx return
			 * status.
			 *
			 *  o IP/TCP/UDP checksum is bad.
			 *  o frame length and protocol specific length
			 *     does not match.
			 */
			if ((status & (ALE_RD_CRC | ALE_RD_CODE |
			    ALE_RD_DRIBBLE | ALE_RD_RUNT | ALE_RD_OFLOW |
			    ALE_RD_TRUNC)) != 0) {
				ale_rx_update_page(sc, &rx_page, length, &prod);
				continue;
			}
		}
		/*
		 * m_devget(9) is major bottle-neck of ale(4)(It comes
		 * from hardware limitation). For jumbo frames we could
		 * get a slightly better performance if driver use
		 * m_getjcl(9) with proper buffer size argument. However
		 * that would make code more complicated and I don't
		 * think users would expect good Rx performance numbers
		 * on these low-end consumer ethernet controller.
		 */
		m = m_devget((char *)(rs + 1), length - ETHER_CRC_LEN,
		    ETHER_ALIGN, ifp, NULL);
		if (m == NULL) {
			ifp->if_iqdrops++;
			ale_rx_update_page(sc, &rx_page, length, &prod);
			continue;
		}
#if 0
		if ((ifp->if_capenable & IFCAP_RXCSUM) != 0 &&
		    (status & ALE_RD_IPV4) != 0)
			ale_rxcsum(sc, m, status);
#endif
#if NVLAN > 0
		if (status & ALE_RRD_VLAN) {
			vtags = ALE_RX_VLAN(letoh32(rs->vtags));
			m->m_pkthdr.ether_vtag = ALE_RX_VLAN_TAG(vtags);
			m->m_flags |= M_VLANTAG;
		}
#endif

#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap_ether(ifp->if_bpf, m, BPF_DIRECTION_IN);
#endif

		/* Pass it to upper layer. */
		ether_input_mbuf(ifp, m);

		ale_rx_update_page(sc, &rx_page, length, &prod);
	}
	return 0;
}

void
ale_tick(void *xsc)
{
	struct ale_softc *sc = xsc;
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	struct mii_data *mii;

	lwkt_serialize_enter(ifp->if_serializer);

	mii = &sc->sc_miibus;
	mii_tick(mii);
	ale_stats_update(sc);

	callout_reset(&sc->ale_tick_ch, hz, ale_tick, sc);

	lwkt_serialize_exit(ifp->if_serializer);
}

void
ale_reset(struct ale_softc *sc)
{
	uint32_t reg;
	int i;

	/* Initialize PCIe module. From Linux. */
	CSR_WRITE_4(sc, 0x1008, CSR_READ_4(sc, 0x1008) | 0x8000);

	CSR_WRITE_4(sc, ALE_MASTER_CFG, MASTER_RESET);
	for (i = ALE_RESET_TIMEOUT; i > 0; i--) {
		DELAY(10);
		if ((CSR_READ_4(sc, ALE_MASTER_CFG) & MASTER_RESET) == 0)
			break;
	}
	if (i == 0)
		printf("%s: master reset timeout!\n", sc->sc_dev.dv_xname);

	for (i = ALE_RESET_TIMEOUT; i > 0; i--) {
		if ((reg = CSR_READ_4(sc, ALE_IDLE_STATUS)) == 0)
			break;
		DELAY(10);
	}

	if (i == 0)
		printf("%s: reset timeout(0x%08x)!\n", sc->sc_dev.dv_xname,
		    reg);
}

void
ale_init(void *xsc)
{
	struct ale_softc *sc = xsc;
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	struct mii_data *mii = &sc->sc_miibus;
	uint8_t eaddr[ETHER_ADDR_LEN];
	bus_addr_t paddr;
	uint32_t reg, rxf_hi, rxf_lo;

	/*
	 * Cancel any pending I/O.
	 */
	ale_stop(sc);

	/*
	 * Reset the chip to a known state.
	 */
	ale_reset(sc);

	/* Initialize Tx descriptors, DMA memory blocks. */
	ale_init_rx_pages(sc);
	ale_init_tx_ring(sc);

	/* Reprogram the station address. */
	bcopy(IF_LLADDR(ifp), eaddr, ETHER_ADDR_LEN);
	CSR_WRITE_4(sc, ALE_PAR0,
	    eaddr[2] << 24 | eaddr[3] << 16 | eaddr[4] << 8 | eaddr[5]);
	CSR_WRITE_4(sc, ALE_PAR1, eaddr[0] << 8 | eaddr[1]);

	/*
	 * Clear WOL status and disable all WOL feature as WOL
	 * would interfere Rx operation under normal environments.
	 */
	CSR_READ_4(sc, ALE_WOL_CFG);
	CSR_WRITE_4(sc, ALE_WOL_CFG, 0);

	/*
	 * Set Tx descriptor/RXF0/CMB base addresses. They share
	 * the same high address part of DMAable region.
	 */
	paddr = sc->ale_cdata.ale_tx_ring_paddr;
	CSR_WRITE_4(sc, ALE_TPD_ADDR_HI, ALE_ADDR_HI(paddr));
	CSR_WRITE_4(sc, ALE_TPD_ADDR_LO, ALE_ADDR_LO(paddr));
	CSR_WRITE_4(sc, ALE_TPD_CNT,
	    (ALE_TX_RING_CNT << TPD_CNT_SHIFT) & TPD_CNT_MASK);

	/* Set Rx page base address, note we use single queue. */
	paddr = sc->ale_cdata.ale_rx_page[0].page_paddr;
	CSR_WRITE_4(sc, ALE_RXF0_PAGE0_ADDR_LO, ALE_ADDR_LO(paddr));
	paddr = sc->ale_cdata.ale_rx_page[1].page_paddr;
	CSR_WRITE_4(sc, ALE_RXF0_PAGE1_ADDR_LO, ALE_ADDR_LO(paddr));

	/* Set Tx/Rx CMB addresses. */
	paddr = sc->ale_cdata.ale_tx_cmb_paddr;
	CSR_WRITE_4(sc, ALE_TX_CMB_ADDR_LO, ALE_ADDR_LO(paddr));
	paddr = sc->ale_cdata.ale_rx_page[0].cmb_paddr;
	CSR_WRITE_4(sc, ALE_RXF0_CMB0_ADDR_LO, ALE_ADDR_LO(paddr));
	paddr = sc->ale_cdata.ale_rx_page[1].cmb_paddr;
	CSR_WRITE_4(sc, ALE_RXF0_CMB1_ADDR_LO, ALE_ADDR_LO(paddr));

	/* Mark RXF0 is valid. */
	CSR_WRITE_1(sc, ALE_RXF0_PAGE0, RXF_VALID);
	CSR_WRITE_1(sc, ALE_RXF0_PAGE1, RXF_VALID);
	/*
	 * No need to initialize RFX1/RXF2/RXF3. We don't use
	 * multi-queue yet.
	 */

	/* Set Rx page size, excluding guard frame size. */
	CSR_WRITE_4(sc, ALE_RXF_PAGE_SIZE, ALE_RX_PAGE_SZ);

	/* Tell hardware that we're ready to load DMA blocks. */
	CSR_WRITE_4(sc, ALE_DMA_BLOCK, DMA_BLOCK_LOAD);

	/* Set Rx/Tx interrupt trigger threshold. */
	CSR_WRITE_4(sc, ALE_INT_TRIG_THRESH, (1 << INT_TRIG_RX_THRESH_SHIFT) |
	    (4 << INT_TRIG_TX_THRESH_SHIFT));
	/*
	 * XXX
	 * Set interrupt trigger timer, its purpose and relation
	 * with interrupt moderation mechanism is not clear yet.
	 */
	CSR_WRITE_4(sc, ALE_INT_TRIG_TIMER,
	    ((ALE_USECS(10) << INT_TRIG_RX_TIMER_SHIFT) |
	    (ALE_USECS(1000) << INT_TRIG_TX_TIMER_SHIFT)));

	/* Configure interrupt moderation timer. */
	reg = ALE_USECS(sc->ale_int_rx_mod) << IM_TIMER_RX_SHIFT;
	reg |= ALE_USECS(sc->ale_int_tx_mod) << IM_TIMER_TX_SHIFT;
	CSR_WRITE_4(sc, ALE_IM_TIMER, reg);
	reg = CSR_READ_4(sc, ALE_MASTER_CFG);
	reg &= ~(MASTER_CHIP_REV_MASK | MASTER_CHIP_ID_MASK);
	reg &= ~(MASTER_IM_RX_TIMER_ENB | MASTER_IM_TX_TIMER_ENB);
	if (ALE_USECS(sc->ale_int_rx_mod) != 0)
		reg |= MASTER_IM_RX_TIMER_ENB;
	if (ALE_USECS(sc->ale_int_tx_mod) != 0)
		reg |= MASTER_IM_TX_TIMER_ENB;
	CSR_WRITE_4(sc, ALE_MASTER_CFG, reg);
	CSR_WRITE_2(sc, ALE_INTR_CLR_TIMER, ALE_USECS(1000));

	/* Set Maximum frame size of controller. */
	if (ifp->if_mtu < ETHERMTU)
		sc->ale_max_frame_size = ETHERMTU;
	else
		sc->ale_max_frame_size = ifp->if_mtu;
	sc->ale_max_frame_size += ETHER_HDR_LEN + EVL_ENCAPLEN + ETHER_CRC_LEN;
	CSR_WRITE_4(sc, ALE_FRAME_SIZE, sc->ale_max_frame_size);

	/* Configure IPG/IFG parameters. */
	CSR_WRITE_4(sc, ALE_IPG_IFG_CFG,
	    ((IPG_IFG_IPGT_DEFAULT << IPG_IFG_IPGT_SHIFT) & IPG_IFG_IPGT_MASK) |
	    ((IPG_IFG_MIFG_DEFAULT << IPG_IFG_MIFG_SHIFT) & IPG_IFG_MIFG_MASK) |
	    ((IPG_IFG_IPG1_DEFAULT << IPG_IFG_IPG1_SHIFT) & IPG_IFG_IPG1_MASK) |
	    ((IPG_IFG_IPG2_DEFAULT << IPG_IFG_IPG2_SHIFT) & IPG_IFG_IPG2_MASK));

	/* Set parameters for half-duplex media. */
	CSR_WRITE_4(sc, ALE_HDPX_CFG,
	    ((HDPX_CFG_LCOL_DEFAULT << HDPX_CFG_LCOL_SHIFT) &
	    HDPX_CFG_LCOL_MASK) |
	    ((HDPX_CFG_RETRY_DEFAULT << HDPX_CFG_RETRY_SHIFT) &
	    HDPX_CFG_RETRY_MASK) | HDPX_CFG_EXC_DEF_EN |
	    ((HDPX_CFG_ABEBT_DEFAULT << HDPX_CFG_ABEBT_SHIFT) &
	    HDPX_CFG_ABEBT_MASK) |
	    ((HDPX_CFG_JAMIPG_DEFAULT << HDPX_CFG_JAMIPG_SHIFT) &
	    HDPX_CFG_JAMIPG_MASK));

	/* Configure Tx jumbo frame parameters. */
	if ((sc->ale_flags & ALE_FLAG_JUMBO) != 0) {
		if (ifp->if_mtu < ETHERMTU)
			reg = sc->ale_max_frame_size;
		else if (ifp->if_mtu < 6 * 1024)
			reg = (sc->ale_max_frame_size * 2) / 3;
		else
			reg = sc->ale_max_frame_size / 2;
		CSR_WRITE_4(sc, ALE_TX_JUMBO_THRESH,
		    roundup(reg, TX_JUMBO_THRESH_UNIT) >>
		    TX_JUMBO_THRESH_UNIT_SHIFT);
	}

	/* Configure TxQ. */
	reg = (128 << (sc->ale_dma_rd_burst >> DMA_CFG_RD_BURST_SHIFT))
	    << TXQ_CFG_TX_FIFO_BURST_SHIFT;
	reg |= (TXQ_CFG_TPD_BURST_DEFAULT << TXQ_CFG_TPD_BURST_SHIFT) &
	    TXQ_CFG_TPD_BURST_MASK;
	CSR_WRITE_4(sc, ALE_TXQ_CFG, reg | TXQ_CFG_ENHANCED_MODE | TXQ_CFG_ENB);

	/* Configure Rx jumbo frame & flow control parameters. */
	if ((sc->ale_flags & ALE_FLAG_JUMBO) != 0) {
		reg = roundup(sc->ale_max_frame_size, RX_JUMBO_THRESH_UNIT);
		CSR_WRITE_4(sc, ALE_RX_JUMBO_THRESH,
		    (((reg >> RX_JUMBO_THRESH_UNIT_SHIFT) <<
		    RX_JUMBO_THRESH_MASK_SHIFT) & RX_JUMBO_THRESH_MASK) |
		    ((RX_JUMBO_LKAH_DEFAULT << RX_JUMBO_LKAH_SHIFT) &
		    RX_JUMBO_LKAH_MASK));
		reg = CSR_READ_4(sc, ALE_SRAM_RX_FIFO_LEN);
		rxf_hi = (reg * 7) / 10;
		rxf_lo = (reg * 3)/ 10;
		CSR_WRITE_4(sc, ALE_RX_FIFO_PAUSE_THRESH,
		    ((rxf_lo << RX_FIFO_PAUSE_THRESH_LO_SHIFT) &
		    RX_FIFO_PAUSE_THRESH_LO_MASK) |
		    ((rxf_hi << RX_FIFO_PAUSE_THRESH_HI_SHIFT) &
		     RX_FIFO_PAUSE_THRESH_HI_MASK));
	}

	/* Disable RSS. */
	CSR_WRITE_4(sc, ALE_RSS_IDT_TABLE0, 0);
	CSR_WRITE_4(sc, ALE_RSS_CPU, 0);

	/* Configure RxQ. */
	CSR_WRITE_4(sc, ALE_RXQ_CFG,
	    RXQ_CFG_ALIGN_32 | RXQ_CFG_CUT_THROUGH_ENB | RXQ_CFG_ENB);

	/* Configure DMA parameters. */
	reg = 0;
	if ((sc->ale_flags & ALE_FLAG_TXCMB_BUG) == 0)
		reg |= DMA_CFG_TXCMB_ENB;
	CSR_WRITE_4(sc, ALE_DMA_CFG,
	    DMA_CFG_OUT_ORDER | DMA_CFG_RD_REQ_PRI | DMA_CFG_RCB_64 |
	    sc->ale_dma_rd_burst | reg |
	    sc->ale_dma_wr_burst | DMA_CFG_RXCMB_ENB |
	    ((DMA_CFG_RD_DELAY_CNT_DEFAULT << DMA_CFG_RD_DELAY_CNT_SHIFT) &
	    DMA_CFG_RD_DELAY_CNT_MASK) |
	    ((DMA_CFG_WR_DELAY_CNT_DEFAULT << DMA_CFG_WR_DELAY_CNT_SHIFT) &
	    DMA_CFG_WR_DELAY_CNT_MASK));

	/*
	 * Hardware can be configured to issue SMB interrupt based
	 * on programmed interval. Since there is a callout that is
	 * invoked for every hz in driver we use that instead of
	 * relying on periodic SMB interrupt.
	 */
	CSR_WRITE_4(sc, ALE_SMB_STAT_TIMER, ALE_USECS(0));

	/* Clear MAC statistics. */
	ale_stats_clear(sc);

	/*
	 * Configure Tx/Rx MACs.
	 *  - Auto-padding for short frames.
	 *  - Enable CRC generation.
	 *  Actual reconfiguration of MAC for resolved speed/duplex
	 *  is followed after detection of link establishment.
	 *  AR81xx always does checksum computation regardless of
	 *  MAC_CFG_RXCSUM_ENB bit. In fact, setting the bit will
	 *  cause Rx handling issue for fragmented IP datagrams due
	 *  to silicon bug.
	 */
	reg = MAC_CFG_TX_CRC_ENB | MAC_CFG_TX_AUTO_PAD | MAC_CFG_FULL_DUPLEX |
	    ((MAC_CFG_PREAMBLE_DEFAULT << MAC_CFG_PREAMBLE_SHIFT) &
	    MAC_CFG_PREAMBLE_MASK);
	if ((sc->ale_flags & ALE_FLAG_FASTETHER) != 0)
		reg |= MAC_CFG_SPEED_10_100;
	else
		reg |= MAC_CFG_SPEED_1000;
	CSR_WRITE_4(sc, ALE_MAC_CFG, reg);

	/* Set up the receive filter. */
	ale_rxfilter(sc);
	ale_rxvlan(sc);

	/* Acknowledge all pending interrupts and clear it. */
	CSR_WRITE_4(sc, ALE_INTR_MASK, ALE_INTRS);
	CSR_WRITE_4(sc, ALE_INTR_STATUS, 0xFFFFFFFF);
	CSR_WRITE_4(sc, ALE_INTR_STATUS, 0);

	sc->ale_flags &= ~ALE_FLAG_LINK;

	/* Switch to the current media. */
	mii_mediachg(mii);

	callout_reset(&sc->ale_tick_ch, hz, ale_tick, sc);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
}

void
ale_stop(struct ale_softc *sc)
{
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	struct ale_txdesc *txd;
	uint32_t reg;
	int i;

	/*
	 * Mark the interface down and cancel the watchdog timer.
	 */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;

	callout_stop(&sc->ale_tick_ch);
	sc->ale_flags &= ~ALE_FLAG_LINK;

	ale_stats_update(sc);

	/* Disable interrupts. */
	CSR_WRITE_4(sc, ALE_INTR_MASK, 0);
	CSR_WRITE_4(sc, ALE_INTR_STATUS, 0xFFFFFFFF);

	/* Disable queue processing and DMA. */
	reg = CSR_READ_4(sc, ALE_TXQ_CFG);
	reg &= ~TXQ_CFG_ENB;
	CSR_WRITE_4(sc, ALE_TXQ_CFG, reg);
	reg = CSR_READ_4(sc, ALE_RXQ_CFG);
	reg &= ~RXQ_CFG_ENB;
	CSR_WRITE_4(sc, ALE_RXQ_CFG, reg);
	reg = CSR_READ_4(sc, ALE_DMA_CFG);
	reg &= ~(DMA_CFG_TXCMB_ENB | DMA_CFG_RXCMB_ENB);
	CSR_WRITE_4(sc, ALE_DMA_CFG, reg);
	DELAY(1000);

	/* Stop Rx/Tx MACs. */
	ale_stop_mac(sc);

	/* Disable interrupts again? XXX */
	CSR_WRITE_4(sc, ALE_INTR_STATUS, 0xFFFFFFFF);

	/*
	 * Free TX mbufs still in the queues.
	 */
	for (i = 0; i < ALE_TX_RING_CNT; i++) {
		txd = &sc->ale_cdata.ale_txdesc[i];
		if (txd->tx_m != NULL) {
			bus_dmamap_unload(sc->ale_cdata.ale_tx_tag,
			    txd->tx_dmamap);
			m_freem(txd->tx_m);
			txd->tx_m = NULL;
		}
        }
}

void
ale_stop_mac(struct ale_softc *sc)
{
	uint32_t reg;
	int i;

	reg = CSR_READ_4(sc, ALE_MAC_CFG);
	if ((reg & (MAC_CFG_TX_ENB | MAC_CFG_RX_ENB)) != 0) {
		reg &= ~MAC_CFG_TX_ENB | MAC_CFG_RX_ENB;
		CSR_WRITE_4(sc, ALE_MAC_CFG, reg);
	}

	for (i = ALE_TIMEOUT; i > 0; i--) {
		reg = CSR_READ_4(sc, ALE_IDLE_STATUS);
		if (reg == 0)
			break;
		DELAY(10);
	}
	if (i == 0)
		printf("%s: could not disable Tx/Rx MAC(0x%08x)!\n",
		    sc->sc_dev.dv_xname, reg);
}

void
ale_init_tx_ring(struct ale_softc *sc)
{
	struct ale_txdesc *txd;
	int i;

	sc->ale_cdata.ale_tx_prod = 0;
	sc->ale_cdata.ale_tx_cons = 0;
	sc->ale_cdata.ale_tx_cnt = 0;

	bzero(sc->ale_cdata.ale_tx_ring, ALE_TX_RING_SZ);
	bzero(sc->ale_cdata.ale_tx_cmb, ALE_TX_CMB_SZ);
	for (i = 0; i < ALE_TX_RING_CNT; i++) {
		txd = &sc->ale_cdata.ale_txdesc[i];
		txd->tx_m = NULL;
	}
	*sc->ale_cdata.ale_tx_cmb = 0;
	bus_dmamap_sync(sc->sc_dmat, sc->ale_cdata.ale_tx_cmb_tag,
	    sc->ale_cdata.ale_tx_cmb_map,
	    BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(sc->sc_dmat, sc->ale_cdata.ale_tx_ring_tag,
	    sc->ale_cdata.ale_tx_ring_map,
	    BUS_DMASYNC_PREWRITE);
}

void
ale_init_rx_pages(struct ale_softc *sc)
{
	struct ale_rx_page *rx_page;
	int i;

	sc->ale_cdata.ale_rx_seqno = 0;
	sc->ale_cdata.ale_rx_curp = 0;

	for (i = 0; i < ALE_RX_PAGES; i++) {
		rx_page = &sc->ale_cdata.ale_rx_page[i];
		bzero(rx_page->page_addr, sc->ale_pagesize);
		bzero(rx_page->cmb_addr, ALE_RX_CMB_SZ);
		rx_page->cons = 0;
		*rx_page->cmb_addr = 0;
		bus_dmamap_sync(sc->sc_dmat, rx_page->page_tag,
		    rx_page->page_map, BUS_DMASYNC_PREWRITE);
		bus_dmamap_sync(sc->sc_dmat, rx_page->cmb_tag,
		    rx_page->cmb_map, BUS_DMASYNC_PREWRITE);
	}
}

void
ale_rxvlan(struct ale_softc *sc)
{
	struct ifnet *ifp;
	uint32_t reg;

	ifp = &sc->sc_arpcom.ac_if;
	reg = CSR_READ_4(sc, ALE_MAC_CFG);
	reg &= ~MAC_CFG_VLAN_TAG_STRIP;
	if ((ifp->if_capenable & IFCAP_VLAN_HWTAGGING) != 0)
		reg |= MAC_CFG_VLAN_TAG_STRIP;
	CSR_WRITE_4(sc, ALE_MAC_CFG, reg);
}

void
ale_rxfilter(struct ale_softc *sc)
{
	struct ifnet *ifp;
	struct ifmultiaddr *ifma;
	uint32_t crc;
	uint32_t mchash[2];
	uint32_t rxcfg;

	ifp = &sc->sc_arpcom.ac_if;

	rxcfg = CSR_READ_4(sc, ALE_MAC_CFG);
	rxcfg &= ~(MAC_CFG_ALLMULTI | MAC_CFG_BCAST | MAC_CFG_PROMISC);
	if ((ifp->if_flags & IFF_BROADCAST) != 0)
		rxcfg |= MAC_CFG_BCAST;
	if ((ifp->if_flags & (IFF_PROMISC | IFF_ALLMULTI)) != 0) {
		if ((ifp->if_flags & IFF_PROMISC) != 0)
			rxcfg |= MAC_CFG_PROMISC;
		if ((ifp->if_flags & IFF_ALLMULTI) != 0)
			rxcfg |= MAC_CFG_ALLMULTI;
		CSR_WRITE_4(sc, ALE_MAR0, 0xFFFFFFFF);
		CSR_WRITE_4(sc, ALE_MAR1, 0xFFFFFFFF);
		CSR_WRITE_4(sc, ALE_MAC_CFG, rxcfg);
		return;
	}

	/* Program new filter. */
	bzero(mchash, sizeof(mchash));

	LIST_FOREACH(ifma, &ifp->if_multiaddrs, ifma_link) {
		if (ifma->ifma_addr->sa_family != AF_LINK)
			continue;
		crc = ether_crc32_le(LLADDR((struct sockaddr_dl *)
		    ifma->ifma_addr), ETHER_ADDR_LEN);
		mchash[crc >> 31] |= 1 << ((crc >> 26) & 0x1f);
	}

	CSR_WRITE_4(sc, ALE_MAR0, mchash[0]);
	CSR_WRITE_4(sc, ALE_MAR1, mchash[1]);
	CSR_WRITE_4(sc, ALE_MAC_CFG, rxcfg);
}

int
sysctl_hw_ale_int_mod(SYSCTL_HANDLER_ARGS)
{
	return (sysctl_int_range(oidp, arg1, arg2, req,
	    ALE_IM_TIMER_MIN, ALE_IM_TIMER_MAX));
}

void
ale_dmamap_buf_cb(void *xctx, bus_dma_segment_t *segs, int nsegs,
		  bus_size_t mapsz __unused, int error)
{
	struct ale_dmamap_ctx *ctx = xctx;
	int i;

	if (error)
		return;

	if (nsegs > ctx->nsegs) {
		ctx->nsegs = 0;
		return;
	}

	ctx->nsegs = nsegs;
	for (i = 0; i < nsegs; ++i)
		ctx->segs[i] = segs[i];
}
