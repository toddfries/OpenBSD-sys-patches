/*
 * Copyright (c) 2012 Stefan Fritsch.
 * Copyright (c) 2010 Minoura Makoto.
 * All rights reserved.
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
 */

#include <sys/cdefs.h>
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/virtioreg.h>
#include <dev/pci/virtiovar.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_ether.h>
#endif

#include <net/bpf.h>

#if 0
#define DBGPRINT(fmt, args...) printf("%s: " fmt "\n", __func__, ## args)
#else
#define DBGPRINT(fmt, args...)
#endif



/*
 * if_vioifreg.h:
 */
/* Configuration registers */
#define VIRTIO_NET_CONFIG_MAC		0 /* 8bit x 6byte */
#define VIRTIO_NET_CONFIG_STATUS	6 /* 16bit */

/* Feature bits */
#define VIRTIO_NET_F_CSUM		(1<<0)
#define VIRTIO_NET_F_GUEST_CSUM		(1<<1)
#define VIRTIO_NET_F_MAC		(1<<5)
#define VIRTIO_NET_F_GSO		(1<<6)
#define VIRTIO_NET_F_GUEST_TSO4		(1<<7)
#define VIRTIO_NET_F_GUEST_TSO6		(1<<8)
#define VIRTIO_NET_F_GUEST_ECN		(1<<9)
#define VIRTIO_NET_F_GUEST_UFO		(1<<10)
#define VIRTIO_NET_F_HOST_TSO4		(1<<11)
#define VIRTIO_NET_F_HOST_TSO6		(1<<12)
#define VIRTIO_NET_F_HOST_ECN		(1<<13)
#define VIRTIO_NET_F_HOST_UFO		(1<<14)
#define VIRTIO_NET_F_MRG_RXBUF		(1<<15)
#define VIRTIO_NET_F_STATUS		(1<<16)
#define VIRTIO_NET_F_CTRL_VQ		(1<<17)
#define VIRTIO_NET_F_CTRL_RX		(1<<18)
#define VIRTIO_NET_F_CTRL_VLAN		(1<<19)
#define VIRTIO_NET_F_CTRL_RX_EXTRA	(1<<20)
#define VIRTIO_NET_F_GUEST_ANNOUNCE	(1<<21)

static const struct virtio_feature_name virtio_net_feature_names[] = {
	{ VIRTIO_NET_F_CSUM,		"CSum" },
	{ VIRTIO_NET_F_MAC,		"MAC" },
	{ VIRTIO_NET_F_GSO,		"GSO" },
	{ VIRTIO_NET_F_GUEST_TSO4,	"GuestTSO4" },
	{ VIRTIO_NET_F_GUEST_TSO6,	"GuestTSO6" },
	{ VIRTIO_NET_F_GUEST_ECN,	"GuestECN" },
	{ VIRTIO_NET_F_GUEST_UFO,	"GuestUFO" },
	{ VIRTIO_NET_F_HOST_TSO4,	"HostTSO4" },
	{ VIRTIO_NET_F_HOST_TSO6,	"HostTSO6" },
	{ VIRTIO_NET_F_HOST_ECN, 	"HostECN" },
	{ VIRTIO_NET_F_HOST_UFO, 	"HostUFO" },
	{ VIRTIO_NET_F_MRG_RXBUF,	"MrgRXBuf" },
	{ VIRTIO_NET_F_STATUS,		"Status" },
	{ VIRTIO_NET_F_CTRL_VQ,		"CtrlVQ" },
	{ VIRTIO_NET_F_CTRL_RX,		"CtrlRX" },
	{ VIRTIO_NET_F_CTRL_VLAN,	"CtrlVLAN" },
	{ VIRTIO_NET_F_CTRL_RX_EXTRA,	"CtrlRXExtra" },
	{ VIRTIO_NET_F_GUEST_ANNOUNCE,	"GuestAnnounce" },
	{ 0, 				NULL }
};

/* Status */
#define VIRTIO_NET_S_LINK_UP	1

/* Packet header structure */
struct virtio_net_hdr {
	uint8_t		flags;
	uint8_t		gso_type;
	uint16_t	hdr_len;
	uint16_t	gso_size;
	uint16_t	csum_start;
	uint16_t	csum_offset;
#if 0
	uint16_t	num_buffers; /* if VIRTIO_NET_F_MRG_RXBUF enabled */
#endif
} __packed;

#define VIRTIO_NET_HDR_F_NEEDS_CSUM	1 /* flags */
#define VIRTIO_NET_HDR_GSO_NONE		0 /* gso_type */
#define VIRTIO_NET_HDR_GSO_TCPV4	1 /* gso_type */
#define VIRTIO_NET_HDR_GSO_UDP		3 /* gso_type */
#define VIRTIO_NET_HDR_GSO_TCPV6	4 /* gso_type */
#define VIRTIO_NET_HDR_GSO_ECN		0x80 /* gso_type, |'ed */

#define VIRTIO_NET_MAX_GSO_LEN		(65536+ETHER_HDR_LEN)

/* Control virtqueue */
struct virtio_net_ctrl_cmd {
	uint8_t	class;
	uint8_t	command;
} __packed;
#define VIRTIO_NET_CTRL_RX		0
# define VIRTIO_NET_CTRL_RX_PROMISC	0
# define VIRTIO_NET_CTRL_RX_ALLMULTI	1

#define VIRTIO_NET_CTRL_MAC		1
# define VIRTIO_NET_CTRL_MAC_TABLE_SET	0

#define VIRTIO_NET_CTRL_VLAN		2
# define VIRTIO_NET_CTRL_VLAN_ADD	0
# define VIRTIO_NET_CTRL_VLAN_DEL	1

struct virtio_net_ctrl_status {
	uint8_t	ack;
} __packed;
#define VIRTIO_NET_OK			0
#define VIRTIO_NET_ERR			1

struct virtio_net_ctrl_rx {
	uint8_t	onoff;
} __packed;

struct virtio_net_ctrl_mac_tbl {
	uint32_t nentries;
	uint8_t macs[][ETHER_ADDR_LEN];
} __packed;

struct virtio_net_ctrl_vlan {
	uint16_t id;
} __packed;

/*
 * if_vioifvar.h:
 */
enum vioif_ctrl_state {
	FREE, INUSE, DONE
};

struct vioif_softc {
	struct device		sc_dev;

	struct virtio_softc	*sc_virtio;
	struct virtqueue	sc_vq[3];

	struct arpcom		sc_ac;
	struct ifmedia		sc_media;

	short			sc_ifflags;

	/* bus_dmamem */
	bus_dma_segment_t	sc_hdr_segs[1];
	struct virtio_net_hdr	*sc_hdrs;
#define sc_rx_hdrs	sc_hdrs
	struct virtio_net_hdr	*sc_tx_hdrs;
	struct virtio_net_ctrl_cmd *sc_ctrl_cmd;
	struct virtio_net_ctrl_status *sc_ctrl_status;
	struct virtio_net_ctrl_rx *sc_ctrl_rx;
	struct virtio_net_ctrl_mac_tbl *sc_ctrl_mac_tbl_uc;
	struct virtio_net_ctrl_mac_tbl *sc_ctrl_mac_tbl_mc;

	/* kmem */
	bus_dmamap_t		*sc_arrays;
#define sc_rxhdr_dmamaps sc_arrays
	bus_dmamap_t		*sc_txhdr_dmamaps;
	bus_dmamap_t		*sc_rx_dmamaps;
	bus_dmamap_t		*sc_tx_dmamaps;
	struct mbuf		**sc_rx_mbufs;
	struct mbuf		**sc_tx_mbufs;

	bus_dmamap_t		sc_ctrl_cmd_dmamap;
	bus_dmamap_t		sc_ctrl_status_dmamap;
	bus_dmamap_t		sc_ctrl_rx_dmamap;
	bus_dmamap_t		sc_ctrl_tbl_uc_dmamap;
	bus_dmamap_t		sc_ctrl_tbl_mc_dmamap;

	enum vioif_ctrl_state	sc_ctrl_inuse;
	struct mutex		sc_ctrl_lock;
};
#define VIRTIO_NET_TX_MAXNSEGS		(16) /* XXX */
#define VIRTIO_NET_CTRL_MAC_MAXENTRIES	(64) /* XXX */

/* cfattach interface functions */
int	vioif_match(struct device *, void *, void *);
void	vioif_attach(struct device *, struct device *, void *);
void	vioif_deferred_init(void *);

/* ifnet interface functions */
int	vioif_init(struct ifnet *);
void	vioif_stop(struct ifnet *, int);
void	vioif_start(struct ifnet *);
int	vioif_ioctl(struct ifnet *, u_long, caddr_t);
void	vioif_watchdog(struct ifnet *);
void	viof_get_lladr(struct arpcom *ac, struct virtio_softc *vsc);
void	viof_put_lladr(struct arpcom *ac, struct virtio_softc *vsc);

/* rx */
int	vioif_add_rx_mbuf(struct vioif_softc *, int);
void	vioif_free_rx_mbuf(struct vioif_softc *, int);
void	vioif_populate_rx_mbufs(struct vioif_softc *);
int	vioif_rx_deq(struct vioif_softc *);
int	vioif_rx_vq_done(struct virtqueue *);
void	vioif_rx_drain(struct vioif_softc *);

/* tx */
int	vioif_tx_vq_done(struct virtqueue *);
void	vioif_tx_drain(struct vioif_softc *);
int	vioif_tx_load(struct vioif_softc *, int, struct mbuf *, struct mbuf **);

/* other control */
int	vioif_link_state(struct ifnet *);
int	vioif_ctrl_rx(struct vioif_softc *, int, int);
int	vioif_set_promisc(struct vioif_softc *, int);
int	vioif_set_allmulti(struct vioif_softc *, int);
int	vioif_set_rx_filter(struct vioif_softc *);
int	vioif_iff(struct vioif_softc *);
int	vioif_media_change(struct ifnet *);
void	vioif_media_status(struct ifnet *, struct ifmediareq *);
int	vioif_ctrl_vq_done(struct virtqueue *);
void	vioif_wait_ctrl(struct vioif_softc *sc);
void	vioif_wait_ctrl_done(struct vioif_softc *sc);
void	vioif_ctrl_wakeup(struct vioif_softc *, enum vioif_ctrl_state);
int	vioif_alloc_mems(struct vioif_softc *);


int
vioif_match(struct device *parent, void *match, void *aux)
{
	struct virtio_softc *va = aux;

	if (va->sc_childdevid == PCI_PRODUCT_VIRTIO_NETWORK)
		return 1;

	return 0;
}

struct cfattach vioif_ca = {
	sizeof(struct vioif_softc),
	  vioif_match, vioif_attach, NULL
};

struct cfdriver vioif_cd = {
	NULL, "vioif", DV_IFNET
};


/* allocate memory */
/*
 * dma memory is used for:
 *   sc_rx_hdrs[slot]:	 metadata array for recieved frames (READ)
 *   sc_tx_hdrs[slot]:	 metadata array for frames to be sent (WRITE)
 *   sc_ctrl_cmd:	 command to be sent via ctrl vq (WRITE)
 *   sc_ctrl_status:	 return value for a command via ctrl vq (READ)
 *   sc_ctrl_rx:	 parameter for a VIRTIO_NET_CTRL_RX class command
 *			 (WRITE)
 *   sc_ctrl_mac_tbl_uc: unicast MAC address filter for a VIRTIO_NET_CTRL_MAC
 *			 class command (WRITE)
 *   sc_ctrl_mac_tbl_mc: multicast MAC address filter for a VIRTIO_NET_CTRL_MAC
 *			 class command (WRITE)
 * sc_ctrl_* structures are allocated only one each; they are protected by
 * sc_ctrl_lock mutex
 */
/*
 * dynamically allocated memory is used for:
 *   sc_rxhdr_dmamaps[slot]:	bus_dmamap_t array for sc_rx_hdrs[slot]
 *   sc_txhdr_dmamaps[slot]:	bus_dmamap_t array for sc_tx_hdrs[slot]
 *   sc_rx_dmamaps[slot]:	bus_dmamap_t array for recieved payload
 *   sc_tx_dmamaps[slot]:	bus_dmamap_t array for sent payload
 *   sc_rx_mbufs[slot]:		mbuf pointer array for recieved frames
 *   sc_tx_mbufs[slot]:		mbuf pointer array for sent frames
 */
int
vioif_alloc_mems(struct vioif_softc *sc)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	int allocsize, allocsize2, r, rsegs, i;
	void *vaddr;
	char *p;
	int rxqsize, txqsize;

	rxqsize = vsc->sc_vqs[0].vq_num;
	txqsize = vsc->sc_vqs[1].vq_num;

	allocsize = sizeof(struct virtio_net_hdr) * rxqsize;
	allocsize += sizeof(struct virtio_net_hdr) * txqsize;
	if (vsc->sc_nvqs == 3) {
		allocsize += sizeof(struct virtio_net_ctrl_cmd) * 1;
		allocsize += sizeof(struct virtio_net_ctrl_status) * 1;
		allocsize += sizeof(struct virtio_net_ctrl_rx) * 1;
		allocsize += sizeof(struct virtio_net_ctrl_mac_tbl)
			+ sizeof(struct virtio_net_ctrl_mac_tbl)
			+ ETHER_ADDR_LEN * VIRTIO_NET_CTRL_MAC_MAXENTRIES;
	}
	r = bus_dmamem_alloc(vsc->sc_dmat, allocsize, 0, 0,
			     &sc->sc_hdr_segs[0], 1, &rsegs, BUS_DMA_NOWAIT);
	if (r != 0) {
		printf("DMA memory allocation failed, size %d, error %d\n",
		       allocsize, r);
		goto err_none;
	}
	r = bus_dmamem_map(vsc->sc_dmat,
			   &sc->sc_hdr_segs[0], 1, allocsize,
			   (caddr_t*)&vaddr, BUS_DMA_NOWAIT);
	if (r != 0) {
		printf("DMA memory map failed, error %d\n", r);
		goto err_dmamem_alloc;
	}
	sc->sc_hdrs = vaddr;
	memset(vaddr, 0, allocsize);
	p = (char *)vaddr;
	p += sizeof(struct virtio_net_hdr) * rxqsize;
#define P(name,size)	do { sc->sc_ ##name = (void*) p;	\
			     p += size; } while (0)
	P(tx_hdrs, sizeof(struct virtio_net_hdr) * txqsize);
	if (vsc->sc_nvqs == 3) {
		P(ctrl_cmd, sizeof(struct virtio_net_ctrl_cmd));
		P(ctrl_status, sizeof(struct virtio_net_ctrl_status));
		P(ctrl_rx, sizeof(struct virtio_net_ctrl_rx));
		P(ctrl_mac_tbl_uc, sizeof(struct virtio_net_ctrl_mac_tbl));
		P(ctrl_mac_tbl_mc,
		  (sizeof(struct virtio_net_ctrl_mac_tbl)
		   + ETHER_ADDR_LEN * VIRTIO_NET_CTRL_MAC_MAXENTRIES));
	}
#undef P

	allocsize2 = sizeof(bus_dmamap_t) * (rxqsize + txqsize);
	allocsize2 += sizeof(bus_dmamap_t) * (rxqsize + txqsize);
	allocsize2 += sizeof(struct mbuf*) * (rxqsize + txqsize);
	sc->sc_arrays = malloc(allocsize2, M_DEVBUF, M_WAITOK|M_CANFAIL|M_ZERO);
	if (sc->sc_arrays == NULL)
		goto err_dmamem_map;
	sc->sc_txhdr_dmamaps = sc->sc_arrays + rxqsize;
	sc->sc_rx_dmamaps = sc->sc_txhdr_dmamaps + txqsize;
	sc->sc_tx_dmamaps = sc->sc_rx_dmamaps + rxqsize;
	sc->sc_rx_mbufs = (void*) (sc->sc_tx_dmamaps + txqsize);
	sc->sc_tx_mbufs = sc->sc_rx_mbufs + rxqsize;

#define C(map, buf, size, nsegs, rw, usage)				\
	do {								\
		r = bus_dmamap_create(vsc->sc_dmat, size, nsegs, size, 0, \
				      BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW,	\
				      &sc->sc_ ##map);			\
		if (r != 0) {						\
			printf(usage " dmamap creation failed, error "	\
			       "%d\n", r);				\
			goto err_reqs;					\
		}							\
	} while (0)
#define C_L1(map, buf, size, nsegs, rw, usage)				\
	C(map, buf, size, nsegs, rw, usage);				\
	do {								\
		r = bus_dmamap_load(vsc->sc_dmat, sc->sc_ ##map,	\
				    &sc->sc_ ##buf, size, NULL,		\
				    BUS_DMA_ ##rw | BUS_DMA_NOWAIT);	\
		if (r != 0) {						\
			printf(usage " dmamap load failed, error %d\n", \
			       r);					\
			goto err_reqs;					\
		}							\
	} while (0)
#define C_L2(map, buf, size, nsegs, rw, usage)				\
	C(map, buf, size, nsegs, rw, usage);				\
	do {								\
		r = bus_dmamap_load(vsc->sc_dmat, sc->sc_ ##map,	\
				    sc->sc_ ##buf, size, NULL,		\
				    BUS_DMA_ ##rw | BUS_DMA_NOWAIT);	\
		if (r != 0) {						\
			printf(usage " dmamap load failed, error %d\n",	\
			       r);					\
			goto err_reqs;					\
		}							\
	} while (0)
	for (i = 0; i < rxqsize; i++) {
		C_L1(rxhdr_dmamaps[i], rx_hdrs[i],
		    sizeof(struct virtio_net_hdr), 1,
		    READ, "rx header");
		C(rx_dmamaps[i], NULL, MCLBYTES, 1, 0, "rx payload");
	}

	for (i = 0; i < txqsize; i++) {
		C_L1(txhdr_dmamaps[i], rx_hdrs[i],
		    sizeof(struct virtio_net_hdr), 1,
		    WRITE, "tx header");
		C(tx_dmamaps[i], NULL, ETHER_MAX_LEN, 256 /* XXX */, 0,
		  "tx payload");
	}

	if (vsc->sc_nvqs == 3) {
		/* control vq class & command */
		C_L2(ctrl_cmd_dmamap, ctrl_cmd,
		    sizeof(struct virtio_net_ctrl_cmd), 1, WRITE,
		    "control command");

		/* control vq status */
		C_L2(ctrl_status_dmamap, ctrl_status,
		    sizeof(struct virtio_net_ctrl_status), 1, READ,
		    "control status");

		/* control vq rx mode command parameter */
		C_L2(ctrl_rx_dmamap, ctrl_rx,
		    sizeof(struct virtio_net_ctrl_rx), 1, WRITE,
		    "rx mode control command");

		/* control vq MAC filter table for unicast */
		/* do not load now since its length is variable */
		C(ctrl_tbl_uc_dmamap, NULL,
		  sizeof(struct virtio_net_ctrl_mac_tbl) + 0, 1, WRITE,
		  "unicast MAC address filter command");

		/* control vq MAC filter table for multicast */
		C(ctrl_tbl_mc_dmamap, NULL,
		  (sizeof(struct virtio_net_ctrl_mac_tbl)
		   + ETHER_ADDR_LEN * VIRTIO_NET_CTRL_MAC_MAXENTRIES),
		  1, WRITE, "multicast MAC address filter command");
	}
#undef C_L2
#undef C_L1
#undef C

	return 0;

err_reqs:
#define D(map)								\
	do {								\
		if (sc->sc_ ##map) {					\
			bus_dmamap_destroy(vsc->sc_dmat, sc->sc_ ##map); \
			sc->sc_ ##map = NULL;				\
		}							\
	} while (0)
	D(ctrl_tbl_mc_dmamap);
	D(ctrl_tbl_uc_dmamap);
	D(ctrl_rx_dmamap);
	D(ctrl_status_dmamap);
	D(ctrl_cmd_dmamap);
	for (i = 0; i < txqsize; i++) {
		D(tx_dmamaps[i]);
		D(txhdr_dmamaps[i]);
	}
	for (i = 0; i < rxqsize; i++) {
		D(rx_dmamaps[i]);
		D(rxhdr_dmamaps[i]);
	}
#undef D
	if (sc->sc_arrays) {
		free(sc->sc_arrays, M_DEVBUF);
		sc->sc_arrays = 0;
	}
err_dmamem_map:
	bus_dmamem_unmap(vsc->sc_dmat, (caddr_t)sc->sc_hdrs, allocsize);
err_dmamem_alloc:
	bus_dmamem_free(vsc->sc_dmat, &sc->sc_hdr_segs[0], 1);
err_none:
	return -1;
}

void
viof_get_lladr(struct arpcom *ac, struct virtio_softc *vsc)
{
	int i;
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		ac->ac_enaddr[i] =
		    virtio_read_device_config_1(vsc, VIRTIO_NET_CONFIG_MAC + i);
	}
}

void
viof_put_lladr(struct arpcom *ac, struct virtio_softc *vsc)
{
	int i;
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		virtio_write_device_config_1(vsc, VIRTIO_NET_CONFIG_MAC + i,
		     ac->ac_enaddr[i]);
	}
}

void
vioif_attach(struct device *parent, struct device *self, void *aux)
{
	struct vioif_softc *sc = (struct vioif_softc *)self;
	struct virtio_softc *vsc = (struct virtio_softc *)parent;
	uint32_t features;
	struct ifnet *ifp = &sc->sc_ac.ac_if;

	if (vsc->sc_child != NULL) {
		printf("child already attached for %s; something wrong...\n",
		       parent->dv_xname);
		return;
	}

	sc->sc_virtio = vsc;

	vsc->sc_child = self;
	vsc->sc_ipl = IPL_NET;
	vsc->sc_vqs = &sc->sc_vq[0];
	vsc->sc_config_change = 0;
	vsc->sc_intrhand = virtio_vq_intr;

	features = virtio_negotiate_features(vsc,
					     (VIRTIO_NET_F_MAC |
					      VIRTIO_NET_F_STATUS |
					      VIRTIO_NET_F_CTRL_VQ |
					      VIRTIO_NET_F_CTRL_RX |
					      VIRTIO_F_NOTIFY_ON_EMPTY),
					     virtio_net_feature_names);
	if (features & VIRTIO_NET_F_MAC) {
		viof_get_lladr(&sc->sc_ac, vsc);
	} else {
		ether_fakeaddr(ifp);
		viof_put_lladr(&sc->sc_ac, vsc);
	}
	printf(" %s\n", ether_sprintf(sc->sc_ac.ac_enaddr));

	if (virtio_alloc_vq(vsc, &sc->sc_vq[0], 0,
			    MCLBYTES+sizeof(struct virtio_net_hdr), 2,
			    "rx") != 0) {
		goto err;
	}
	vsc->sc_nvqs = 1;
	sc->sc_vq[0].vq_done = vioif_rx_vq_done;
	if (virtio_alloc_vq(vsc, &sc->sc_vq[1], 1,
			    (sizeof(struct virtio_net_hdr)
			     + (ETHER_MAX_LEN - ETHER_HDR_LEN)),
			    VIRTIO_NET_TX_MAXNSEGS + 1,
			    "tx") != 0) {
		goto err;
	}
	vsc->sc_nvqs = 2;
	sc->sc_vq[1].vq_done = vioif_tx_vq_done;
	virtio_start_vq_intr(vsc, &sc->sc_vq[0]);
	virtio_stop_vq_intr(vsc, &sc->sc_vq[1]); /* not urgent; do it later */
	if ((features & VIRTIO_NET_F_CTRL_VQ)
	    && (features & VIRTIO_NET_F_CTRL_RX)) {
		if (virtio_alloc_vq(vsc, &sc->sc_vq[2], 2,
				    NBPG, 1, "control") == 0) {
			mtx_init(&sc->sc_ctrl_lock, IPL_NET);
			sc->sc_vq[2].vq_done = vioif_ctrl_vq_done;
			virtio_start_vq_intr(vsc, &sc->sc_vq[2]);
			vsc->sc_nvqs = 3;
		}
	}

	if (vioif_alloc_mems(sc) < 0)
		goto err;
	if (vsc->sc_nvqs == 3)
		startuphook_establish(vioif_deferred_init, self);

	strlcpy(ifp->if_xname, self->dv_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_start = vioif_start;
	ifp->if_ioctl = vioif_ioctl;
	ifp->if_capabilities = 0;
	ifp->if_watchdog = vioif_watchdog;
	ifmedia_init(&sc->sc_media, 0, vioif_media_change, vioif_media_status);
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_AUTO, 0, NULL);
	ifmedia_set(&sc->sc_media, IFM_ETHER | IFM_AUTO);

	if_attach(ifp);
	ether_ifattach(ifp);

	return;

err:
	if (vsc->sc_nvqs == 3) {
		virtio_free_vq(vsc, &sc->sc_vq[2]);
		vsc->sc_nvqs = 2;
	}
	if (vsc->sc_nvqs == 2) {
		virtio_free_vq(vsc, &sc->sc_vq[1]);
		vsc->sc_nvqs = 1;
	}
	if (vsc->sc_nvqs == 1) {
		virtio_free_vq(vsc, &sc->sc_vq[0]);
		vsc->sc_nvqs = 0;
	}
	vsc->sc_child = VIRTIO_CHILD_ERROR;
	return;
}

/* check link status */
int
vioif_link_state(struct ifnet *ifp)
{
	struct vioif_softc *sc = ifp->if_softc;
	struct virtio_softc *vsc = sc->sc_virtio;
	int link_state = LINK_STATE_FULL_DUPLEX;

	if (vsc->sc_features & VIRTIO_NET_F_STATUS) {
		int status = virtio_read_device_config_2(vsc, VIRTIO_NET_CONFIG_STATUS);
		if (!(status & VIRTIO_NET_S_LINK_UP))
			link_state = LINK_STATE_DOWN;
	}
	if (ifp->if_link_state != link_state) {
		ifp->if_link_state = link_state;
		if_link_state_change(ifp);
	}
	return 0;
}

int
vioif_media_change(struct ifnet *ifp)
{
	/* Ignore */
	return (0);
}

void
vioif_media_status(struct ifnet *ifp, struct ifmediareq *imr)
{
	imr->ifm_active = IFM_ETHER | IFM_AUTO;
	imr->ifm_status = IFM_AVALID;

	vioif_link_state(ifp);
	if (LINK_STATE_IS_UP(ifp->if_link_state) &&
	    ifp->if_flags & IFF_UP)
		imr->ifm_status |= IFM_ACTIVE|IFM_FDX;
}

/* we need interrupts to make promiscuous mode off */
void
vioif_deferred_init(void *self)
{
	struct vioif_softc *sc = (struct vioif_softc *)self;
	struct ifnet *ifp = &sc->sc_ac.ac_if;
	int r;

	r =  vioif_set_promisc(sc, 0);
	if (r != 0)
		printf("resetting promisc mode failed, errror %d\n", r);
	else
		ifp->if_flags &= ~IFF_PROMISC;
}

/*
 * Interface functions for ifnet
 */
int
vioif_init(struct ifnet *ifp)
{
	struct vioif_softc *sc = ifp->if_softc;

	vioif_stop(ifp, 0);
	vioif_populate_rx_mbufs(sc);
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	vioif_iff(sc);
	vioif_link_state(ifp);
	return 0;
}

void
vioif_stop(struct ifnet *ifp, int disable)
{
	struct vioif_softc *sc = ifp->if_softc;
	struct virtio_softc *vsc = sc->sc_virtio;

	ifp->if_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	/* only way to stop I/O and DMA is resetting... */
	virtio_reset(vsc);
	vioif_rx_deq(sc);
	vioif_tx_drain(sc);

	if (disable)
		vioif_rx_drain(sc);

	virtio_reinit_start(vsc);
	virtio_negotiate_features(vsc, vsc->sc_features, NULL);
	virtio_start_vq_intr(vsc, &sc->sc_vq[0]);
	virtio_stop_vq_intr(vsc, &sc->sc_vq[1]);
	if (vsc->sc_nvqs >= 3)
		virtio_start_vq_intr(vsc, &sc->sc_vq[2]);
	virtio_reinit_end(vsc);
}

void
vioif_start(struct ifnet *ifp)
{
	struct vioif_softc *sc = ifp->if_softc;
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = &sc->sc_vq[1]; /* tx vq */
	struct mbuf *m;
	int queued = 0, retry = 0;

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (;;) {
		int slot, r;

		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			break;

		r = virtio_enqueue_prep(vq, &slot);
		if (r == EAGAIN) {
			ifp->if_flags |= IFF_OACTIVE;
			if (vioif_tx_vq_done(vq) && retry++ == 0)
				continue;
			else
				break;
		}
		if (r != 0)
			panic("enqueue_prep for a tx buffer: %d", r);
		r = vioif_tx_load(sc, slot, m, &sc->sc_tx_mbufs[slot]);
		if (r != 0) {
			virtio_enqueue_abort(vq, slot);
			break;
		}
		IFQ_DEQUEUE(&ifp->if_snd, m);
		r = virtio_enqueue_reserve(vq, slot,
					sc->sc_tx_dmamaps[slot]->dm_nsegs + 1);
		if (r != 0) {
			bus_dmamap_unload(vsc->sc_dmat,
					  sc->sc_tx_dmamaps[slot]);
			m_freem(sc->sc_tx_mbufs[slot]);
			sc->sc_tx_mbufs[slot] = NULL;
			ifp->if_oerrors++;
			ifp->if_flags |= IFF_OACTIVE;
			if (vioif_tx_vq_done(vq) && retry++ == 0)
				continue;
			else
				break;
		}

		memset(&sc->sc_tx_hdrs[slot], 0, sizeof(struct virtio_net_hdr));
		bus_dmamap_sync(vsc->sc_dmat, sc->sc_tx_dmamaps[slot],
				0, sc->sc_tx_dmamaps[slot]->dm_mapsize,
				BUS_DMASYNC_PREWRITE);
		bus_dmamap_sync(vsc->sc_dmat, sc->sc_txhdr_dmamaps[slot],
				0, sc->sc_txhdr_dmamaps[slot]->dm_mapsize,
				BUS_DMASYNC_PREWRITE);
		virtio_enqueue(vq, slot, sc->sc_txhdr_dmamaps[slot], 1);
		virtio_enqueue(vq, slot, sc->sc_tx_dmamaps[slot], 1);
		virtio_enqueue_commit(vsc, vq, slot, 0);
		queued++;
#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m, BPF_DIRECTION_OUT);
#endif
	}

	if (queued > 0) {
		virtio_notify(vsc, vq);
		ifp->if_timer = 5;
	}
}

int
vioif_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct vioif_softc *sc = ifp->if_softc;
	int s, r = 0;
	struct ifaddr *ifa = (struct ifaddr *)data;

	s = splnet();
	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		if (!(ifp->if_flags & IFF_RUNNING))
			vioif_init(ifp);
#ifdef INET
		if (ifa->ifa_addr->sa_family == AF_INET)
			arp_ifinit(&sc->sc_ac, ifa);
#endif
		break;
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			if (ifp->if_flags & IFF_RUNNING)
				r = ENETRESET;
			else
				vioif_init(ifp);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				vioif_stop(ifp, 1);
		}
		break;
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		r = ifmedia_ioctl(ifp, (struct ifreq *)data, &sc->sc_media, cmd);
		break;
	default:
		r = ether_ioctl(ifp, &sc->sc_ac, cmd, data);
	}

	if (r == ENETRESET) {
		if (ifp->if_flags & IFF_RUNNING)
			vioif_iff(sc);
		r = 0;
	}
	splx(s);
	return r;
}

void
vioif_watchdog(struct ifnet *ifp)
{
	struct vioif_softc *sc = ifp->if_softc;
	int s;

	if (ifp->if_flags & IFF_RUNNING) {
		s = splnet();
		vioif_tx_vq_done(&sc->sc_vq[1]);
		splx(s);
	}
}


/*
 * Recieve implementation
 */
/* allocate and initialize a mbuf for recieve */
int
vioif_add_rx_mbuf(struct vioif_softc *sc, int i)
{
	struct mbuf *m;
	int r;

	m = MCLGETI(NULL, M_DONTWAIT, &sc->sc_ac.ac_if, MCLBYTES);
	if (m == NULL)
		return ENOBUFS;
	sc->sc_rx_mbufs[i] = m;
	m->m_len = m->m_pkthdr.len = m->m_ext.ext_size;
	r = bus_dmamap_load_mbuf(sc->sc_virtio->sc_dmat,
				 sc->sc_rx_dmamaps[i],
				 m, BUS_DMA_READ|BUS_DMA_NOWAIT);
	if (r) {
		m_freem(m);
		sc->sc_rx_mbufs[i] = 0;
		return r;
	}

	return 0;
}

/* free a mbuf for recieve */
void
vioif_free_rx_mbuf(struct vioif_softc *sc, int i)
{
	bus_dmamap_unload(sc->sc_virtio->sc_dmat, sc->sc_rx_dmamaps[i]);
	m_freem(sc->sc_rx_mbufs[i]);
	sc->sc_rx_mbufs[i] = NULL;
}

/* add mbufs for all the empty recieve slots */
void
vioif_populate_rx_mbufs(struct vioif_softc *sc)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	int i, r, ndone = 0;
	struct virtqueue *vq = &sc->sc_vq[0]; /* rx vq */

	for (i = 0; i < vq->vq_num; i++) {
		int slot;
		r = virtio_enqueue_prep(vq, &slot);
		if (r == EAGAIN)
			break;
		if (r != 0)
			panic("enqueue_prep for rx buffers: %d", r);
		if (sc->sc_rx_mbufs[slot] == NULL) {
			r = vioif_add_rx_mbuf(sc, slot);
			if (r != 0) {
				break;
			}
		}
		r = virtio_enqueue_reserve(vq, slot,
					sc->sc_rx_dmamaps[slot]->dm_nsegs + 1);
		if (r != 0) {
			vioif_free_rx_mbuf(sc, slot);
			break;
		}
		bus_dmamap_sync(vsc->sc_dmat, sc->sc_rxhdr_dmamaps[slot],
			0, sizeof(struct virtio_net_hdr), BUS_DMASYNC_PREREAD);
		bus_dmamap_sync(vsc->sc_dmat, sc->sc_rx_dmamaps[slot],
			0, MCLBYTES, BUS_DMASYNC_PREREAD);
		virtio_enqueue(vq, slot, sc->sc_rxhdr_dmamaps[slot], 0);
		virtio_enqueue(vq, slot, sc->sc_rx_dmamaps[slot], 0);
		virtio_enqueue_commit(vsc, vq, slot, 0);
		ndone++;
	}
	if (ndone > 0)
		virtio_notify(vsc, vq);
}

/* dequeue recieved packets */
int
vioif_rx_deq(struct vioif_softc *sc)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = &sc->sc_vq[0];
	struct ifnet *ifp = &sc->sc_ac.ac_if;
	struct mbuf *m;
	int r = 0;
	int slot, len;

	while (virtio_dequeue(vsc, vq, &slot, &len) == 0) {
		len -= sizeof(struct virtio_net_hdr);
		r = 1;
		bus_dmamap_sync(vsc->sc_dmat, sc->sc_rxhdr_dmamaps[slot],
				0, sizeof(struct virtio_net_hdr),
				BUS_DMASYNC_POSTREAD);
		bus_dmamap_sync(vsc->sc_dmat, sc->sc_rx_dmamaps[slot],
				0, MCLBYTES,
				BUS_DMASYNC_POSTREAD);
		m = sc->sc_rx_mbufs[slot];
		KASSERT(m != NULL);
		bus_dmamap_unload(vsc->sc_dmat, sc->sc_rx_dmamaps[slot]);
		sc->sc_rx_mbufs[slot] = 0;
		virtio_dequeue_commit(vq, slot);
		m->m_pkthdr.rcvif = ifp;
		m->m_len = m->m_pkthdr.len = len;
		m->m_pkthdr.csum_flags = 0;
		ifp->if_ipackets++;
#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m, BPF_DIRECTION_IN);
#endif
		ether_input_mbuf(ifp, m);
	}
	return r;
}

/* rx interrupt; call _dequeue above and schedule a softint */
int
vioif_rx_vq_done(struct virtqueue *vq)
{
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_softc *sc = (struct vioif_softc *)vsc->sc_child;
	int r;

	r = vioif_rx_deq(sc);
	if (r)
		vioif_populate_rx_mbufs(sc);

	return r;
}

/* free all the mbufs; called from if_stop(disable) */
void
vioif_rx_drain(struct vioif_softc *sc)
{
	struct virtqueue *vq = &sc->sc_vq[0];
	int i;

	for (i = 0; i < vq->vq_num; i++) {
		if (sc->sc_rx_mbufs[i] == NULL)
			continue;
		vioif_free_rx_mbuf(sc, i);
	}
}

/*
 * Transmition implementation
 */
/* actual transmission is done in if_start */
/* tx interrupt; dequeue and free mbufs */
/*
 * tx interrupt is actually disabled; this should be called upon
 * tx vq full and watchdog
 */
int
vioif_tx_vq_done(struct virtqueue *vq)
{
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_softc *sc = (struct vioif_softc *)vsc->sc_child;
	struct ifnet *ifp = &sc->sc_ac.ac_if;
	struct mbuf *m;
	int r = 0;
	int slot, len;

	while (virtio_dequeue(vsc, vq, &slot, &len) == 0) {
		r++;
		bus_dmamap_sync(vsc->sc_dmat, sc->sc_txhdr_dmamaps[slot],
				0, sizeof(struct virtio_net_hdr),
				BUS_DMASYNC_POSTWRITE);
		bus_dmamap_sync(vsc->sc_dmat, sc->sc_tx_dmamaps[slot],
				0, sc->sc_tx_dmamaps[slot]->dm_mapsize,
				BUS_DMASYNC_POSTWRITE);
		m = sc->sc_tx_mbufs[slot];
		bus_dmamap_unload(vsc->sc_dmat, sc->sc_tx_dmamaps[slot]);
		sc->sc_tx_mbufs[slot] = 0;
		virtio_dequeue_commit(vq, slot);
		ifp->if_opackets++;
		m_freem(m);
	}

	if (r)
		ifp->if_flags &= ~IFF_OACTIVE;
	return r;
}

int
vioif_tx_load(struct vioif_softc *sc, int slot, struct mbuf *m,
	      struct mbuf **mnew)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	bus_dmamap_t	 dmap= sc->sc_tx_dmamaps[slot];
	struct mbuf	*m0 = NULL;
	int		 r;

	r = bus_dmamap_load_mbuf(vsc->sc_dmat, dmap, m,
	    BUS_DMA_WRITE|BUS_DMA_NOWAIT);
	if (r == 0) {
		*mnew = m;
		return r;
	}
	if (r != EFBIG)
		return r;
	/* EFBIG: mbuf chain is too fragmented */
	MGETHDR(m0, M_DONTWAIT, MT_DATA);
	if (m0 == NULL)
		return ENOBUFS;
	if (m->m_pkthdr.len > MHLEN) {
		MCLGETI(m0, M_DONTWAIT, NULL, m->m_pkthdr.len);
		if (!(m0->m_flags & M_EXT)) {
			m_freem(m0);
			return ENOBUFS;
		}
	}
	m_copydata(m, 0, m->m_pkthdr.len, mtod(m0, caddr_t));
	m0->m_pkthdr.len = m0->m_len = m->m_pkthdr.len;
	r = bus_dmamap_load_mbuf(vsc->sc_dmat, dmap, m0,
	    BUS_DMA_NOWAIT|BUS_DMA_WRITE);
	if (r != 0) {
		m_freem(m0);
		printf("%s: tx dmamap load error %d\n", sc->sc_dev.dv_xname, r);
		return ENOBUFS;
	}
	m_freem(m);
	*mnew = m0;
	return 0;
}

/* free all the mbufs already put on vq; called from if_stop(disable) */
void
vioif_tx_drain(struct vioif_softc *sc)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = &sc->sc_vq[1];
	int i;

	for (i = 0; i < vq->vq_num; i++) {
		if (sc->sc_tx_mbufs[i] == NULL)
			continue;
		bus_dmamap_unload(vsc->sc_dmat, sc->sc_tx_dmamaps[i]);
		m_freem(sc->sc_tx_mbufs[i]);
		sc->sc_tx_mbufs[i] = NULL;
	}
}

/*
 * Control vq
 */
/* issue a VIRTIO_NET_CTRL_RX class command and wait for completion */
int
vioif_ctrl_rx(struct vioif_softc *sc, int cmd, int onoff)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = &sc->sc_vq[2];
	int r, slot;

	if (vsc->sc_nvqs < 3)
		return ENOTSUP;

	vioif_wait_ctrl(sc);

	sc->sc_ctrl_cmd->class = VIRTIO_NET_CTRL_RX;
	sc->sc_ctrl_cmd->command = cmd;
	sc->sc_ctrl_rx->onoff = onoff;

	bus_dmamap_sync(vsc->sc_dmat, sc->sc_ctrl_cmd_dmamap,
			0, sizeof(struct virtio_net_ctrl_cmd),
			BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(vsc->sc_dmat, sc->sc_ctrl_rx_dmamap,
			0, sizeof(struct virtio_net_ctrl_rx),
			BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(vsc->sc_dmat, sc->sc_ctrl_status_dmamap,
			0, sizeof(struct virtio_net_ctrl_status),
			BUS_DMASYNC_PREREAD);

	r = virtio_enqueue_prep(vq, &slot);
	if (r != 0)
		panic("%s: control vq busy!?", sc->sc_dev.dv_xname);
	r = virtio_enqueue_reserve(vq, slot, 3);
	if (r != 0)
		panic("%s: control vq busy!?", sc->sc_dev.dv_xname);
	virtio_enqueue(vq, slot, sc->sc_ctrl_cmd_dmamap, 1);
	virtio_enqueue(vq, slot, sc->sc_ctrl_rx_dmamap, 1);
	virtio_enqueue(vq, slot, sc->sc_ctrl_status_dmamap, 0);
	virtio_enqueue_commit(vsc, vq, slot, 1);

	vioif_wait_ctrl_done(sc);

	bus_dmamap_sync(vsc->sc_dmat, sc->sc_ctrl_cmd_dmamap, 0,
			sizeof(struct virtio_net_ctrl_cmd),
			BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(vsc->sc_dmat, sc->sc_ctrl_rx_dmamap, 0,
			sizeof(struct virtio_net_ctrl_rx),
			BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(vsc->sc_dmat, sc->sc_ctrl_status_dmamap, 0,
			sizeof(struct virtio_net_ctrl_status),
			BUS_DMASYNC_POSTREAD);

	if (sc->sc_ctrl_status->ack == VIRTIO_NET_OK)
		r = 0;
	else {
		printf("%s: failed setting rx mode\n", sc->sc_dev.dv_xname);
		r = EIO;
	}

	vioif_ctrl_wakeup(sc, FREE);
	DBGPRINT("cmd %d %d: %d", cmd, (int)onoff, r);
	return r;
}

int
vioif_set_promisc(struct vioif_softc *sc, int onoff)
{
	int r;

	r = vioif_ctrl_rx(sc, VIRTIO_NET_CTRL_RX_PROMISC, onoff);

	return r;
}

int
vioif_set_allmulti(struct vioif_softc *sc, int onoff)
{
	int r;

	r = vioif_ctrl_rx(sc, VIRTIO_NET_CTRL_RX_ALLMULTI, onoff);

	return r;
}

void
vioif_wait_ctrl(struct vioif_softc *sc)
{
	mtx_enter(&sc->sc_ctrl_lock);
	while (sc->sc_ctrl_inuse != FREE) {
		msleep(&sc->sc_ctrl_inuse, &sc->sc_ctrl_lock, IPL_NET,
				"vioif_wait", 0);
	}
	sc->sc_ctrl_inuse = INUSE;
	mtx_leave(&sc->sc_ctrl_lock);
}

void
vioif_wait_ctrl_done(struct vioif_softc *sc)
{
	mtx_enter(&sc->sc_ctrl_lock);
	while (sc->sc_ctrl_inuse != DONE) {
			msleep(&sc->sc_ctrl_inuse, &sc->sc_ctrl_lock, IPL_NET,
				"vioif_wait", 0);
	}
	mtx_leave(&sc->sc_ctrl_lock);
}

void
vioif_ctrl_wakeup(struct vioif_softc *sc, enum vioif_ctrl_state new)
{
	mtx_enter(&sc->sc_ctrl_lock);
	sc->sc_ctrl_inuse = new;
	mtx_leave(&sc->sc_ctrl_lock);
	wakeup(&sc->sc_ctrl_inuse);
}

int
vioif_ctrl_vq_done(struct virtqueue *vq)
{
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioif_softc *sc = (struct vioif_softc *)vsc->sc_child;
	int r, slot;

	r = virtio_dequeue(vsc, vq, &slot, NULL);
	if (r == ENOENT)
		return 0;
	virtio_dequeue_commit(vq, slot);
	vioif_ctrl_wakeup(sc, DONE);

	return 1;
}

/* issue VIRTIO_NET_CTRL_MAC_TABLE_SET command and wait for completion */
int
vioif_set_rx_filter(struct vioif_softc *sc)
{
	/* filter already set in sc_ctrl_mac_tbl */
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = &sc->sc_vq[2];
	int r, slot;

	if (vsc->sc_nvqs < 3)
		return ENOTSUP;

	vioif_wait_ctrl(sc);

	sc->sc_ctrl_cmd->class = VIRTIO_NET_CTRL_MAC;
	sc->sc_ctrl_cmd->command = VIRTIO_NET_CTRL_MAC_TABLE_SET;

	r = bus_dmamap_load(vsc->sc_dmat, sc->sc_ctrl_tbl_uc_dmamap,
			    sc->sc_ctrl_mac_tbl_uc,
			    (sizeof(struct virtio_net_ctrl_mac_tbl)
			  + ETHER_ADDR_LEN * sc->sc_ctrl_mac_tbl_uc->nentries),
			    NULL, BUS_DMA_WRITE|BUS_DMA_NOWAIT);
	if (r) {
		printf("%s: control command dmamap load failed, error %d\n",
		       sc->sc_dev.dv_xname, r);
		goto out;
	}
	r = bus_dmamap_load(vsc->sc_dmat, sc->sc_ctrl_tbl_mc_dmamap,
			    sc->sc_ctrl_mac_tbl_mc,
			    (sizeof(struct virtio_net_ctrl_mac_tbl)
			  + ETHER_ADDR_LEN * sc->sc_ctrl_mac_tbl_mc->nentries),
			    NULL, BUS_DMA_WRITE|BUS_DMA_NOWAIT);
	if (r) {
		printf("%s: control command dmamap load failed, error %d\n",
		       sc->sc_dev.dv_xname, r);
		bus_dmamap_unload(vsc->sc_dmat, sc->sc_ctrl_tbl_uc_dmamap);
		goto out;
	}

	bus_dmamap_sync(vsc->sc_dmat, sc->sc_ctrl_cmd_dmamap,
			0, sizeof(struct virtio_net_ctrl_cmd),
			BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(vsc->sc_dmat, sc->sc_ctrl_tbl_uc_dmamap, 0,
			(sizeof(struct virtio_net_ctrl_mac_tbl)
			 + ETHER_ADDR_LEN * sc->sc_ctrl_mac_tbl_uc->nentries),
			BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(vsc->sc_dmat, sc->sc_ctrl_tbl_mc_dmamap, 0,
			(sizeof(struct virtio_net_ctrl_mac_tbl)
			 + ETHER_ADDR_LEN * sc->sc_ctrl_mac_tbl_mc->nentries),
			BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(vsc->sc_dmat, sc->sc_ctrl_status_dmamap,
			0, sizeof(struct virtio_net_ctrl_status),
			BUS_DMASYNC_PREREAD);

	r = virtio_enqueue_prep(vq, &slot);
	if (r != 0)
		panic("%s: control vq busy!?", sc->sc_dev.dv_xname);
	r = virtio_enqueue_reserve(vq, slot, 4);
	if (r != 0)
		panic("%s: control vq busy!?", sc->sc_dev.dv_xname);
	virtio_enqueue(vq, slot, sc->sc_ctrl_cmd_dmamap, 1);
	virtio_enqueue(vq, slot, sc->sc_ctrl_tbl_uc_dmamap, 1);
	virtio_enqueue(vq, slot, sc->sc_ctrl_tbl_mc_dmamap, 1);
	virtio_enqueue(vq, slot, sc->sc_ctrl_status_dmamap, 0);
	virtio_enqueue_commit(vsc, vq, slot, 1);

	vioif_wait_ctrl_done(sc);

	bus_dmamap_sync(vsc->sc_dmat, sc->sc_ctrl_cmd_dmamap, 0,
			sizeof(struct virtio_net_ctrl_cmd),
			BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(vsc->sc_dmat, sc->sc_ctrl_tbl_uc_dmamap, 0,
			(sizeof(struct virtio_net_ctrl_mac_tbl)
			 + ETHER_ADDR_LEN * sc->sc_ctrl_mac_tbl_uc->nentries),
			BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(vsc->sc_dmat, sc->sc_ctrl_tbl_mc_dmamap, 0,
			(sizeof(struct virtio_net_ctrl_mac_tbl)
			 + ETHER_ADDR_LEN * sc->sc_ctrl_mac_tbl_mc->nentries),
			BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(vsc->sc_dmat, sc->sc_ctrl_status_dmamap, 0,
			sizeof(struct virtio_net_ctrl_status),
			BUS_DMASYNC_POSTREAD);
	bus_dmamap_unload(vsc->sc_dmat, sc->sc_ctrl_tbl_uc_dmamap);
	bus_dmamap_unload(vsc->sc_dmat, sc->sc_ctrl_tbl_mc_dmamap);

	if (sc->sc_ctrl_status->ack == VIRTIO_NET_OK)
		r = 0;
	else {
		printf("%s: failed setting rx filter\n", sc->sc_dev.dv_xname);
		r = EIO;
	}

out:
	vioif_ctrl_wakeup(sc, FREE);
	return r;
}

/*
 * If IFF_PROMISC requested,  set promiscuous
 * If multicast filter small enough (<=MAXENTRIES) set rx filter
 * If large multicast filter exist use ALLMULTI
 */
/*
 * If setting rx filter fails fall back to ALLMULTI
 * If ALLMULTI fails fall back to PROMISC
 */
int
vioif_iff(struct vioif_softc *sc)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct ifnet *ifp = &sc->sc_ac.ac_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	int nentries = 0;
	int promisc = 0, allmulti = 0, rxfilter = 0;
	int r;

	if (vsc->sc_nvqs < 3) {	/* no ctrl vq; always promisc */
		ifp->if_flags |= IFF_PROMISC;
		return 0;
	}

	if (ifp->if_flags & IFF_PROMISC) {
		promisc = 1;
		goto set;
	}

	ETHER_FIRST_MULTI(step, &sc->sc_ac, enm);
	while (enm != NULL) {
		if (nentries >= VIRTIO_NET_CTRL_MAC_MAXENTRIES) {
			allmulti = 1;
			goto set;
		}
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
			   ETHER_ADDR_LEN)) {
			allmulti = 1;
			goto set;
		}
		memcpy(sc->sc_ctrl_mac_tbl_mc->macs[nentries],
		       enm->enm_addrlo, ETHER_ADDR_LEN);
		ETHER_NEXT_MULTI(step, enm);
		nentries++;
	}
	rxfilter = 1;

set:
	if (rxfilter) {
		sc->sc_ctrl_mac_tbl_uc->nentries = 0;
		sc->sc_ctrl_mac_tbl_mc->nentries = nentries;
		r = vioif_set_rx_filter(sc);
		if (r != 0) {
			rxfilter = 0;
			allmulti = 1; /* fallback */
		}
	} else {
		/* remove rx filter */
		sc->sc_ctrl_mac_tbl_uc->nentries = 0;
		sc->sc_ctrl_mac_tbl_mc->nentries = 0;
		r = vioif_set_rx_filter(sc);
		/* what to do on failure? */
	}
	if (allmulti) {
		r = vioif_set_allmulti(sc, 1);
		if (r != 0) {
			allmulti = 0;
			promisc = 1; /* fallback */
		}
	} else {
		r = vioif_set_allmulti(sc, 0);
		/* what to do on failure? */
	}

	return vioif_set_promisc(sc, promisc);
}
