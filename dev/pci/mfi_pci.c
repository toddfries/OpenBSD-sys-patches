<<<<<<< HEAD
/* $OpenBSD: mfi_pci.c,v 1.10 2006/08/06 03:58:36 brad Exp $ */
=======
/* $OpenBSD: mfi_pci.c,v 1.23 2010/04/01 23:19:39 jsg Exp $ */
>>>>>>> origin/master
/*
 * Copyright (c) 2006 Marco Peereboom <marco@peereboom.us>
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

#include "bio.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/rwlock.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>

#include <machine/bus.h>

#include <scsi/scsi_all.h>
#include <scsi/scsi_disk.h>
#include <scsi/scsiconf.h>

#include <dev/ic/mfireg.h>
#include <dev/ic/mfivar.h>

#define	MFI_BAR		0x10
#define	MFI_BAR_GEN2	0x14
#define	MFI_PCI_MEMSIZE	0x2000 /* 8k */

int	mfi_pci_find_device(void *);
int	mfi_pci_match(struct device *, void *, void *);
void	mfi_pci_attach(struct device *, struct device *, void *);

struct cfattach mfi_pci_ca = {
	sizeof(struct mfi_softc), mfi_pci_match, mfi_pci_attach
};

<<<<<<< HEAD
=======
struct mfi_pci_subtype {
	pcireg_t			st_id;
	const char			*st_string;
};

static const struct mfi_pci_subtype mfi_1078_subtypes[] = {
	{ 0x10061000,	"SAS 8888ELP" },
	{ 0x100a1000,	"SAS 8708ELP" },
	{ 0x100f1000,	"SAS 8708E" },
	{ 0x10121000,	"SAS 8704ELP" },
	{ 0x10131000,	"SAS 8708EM2" },
	{ 0x10161000,	"SAS 8880EM2" },
	{ 0x1f0a1028,	"Dell PERC 6/e" },
	{ 0x1f0b1028,	"Dell PERC 6/i" },
	{ 0x1f0d1028,	"Dell CERC 6/i" },
	{ 0x1f0c1028,	"Dell PERC 6/i integrated" },
	{ 0x1f111028,	"Dell CERC 6/i integrated" },
	{ 0x0,		"" }
};

static const struct mfi_pci_subtype mfi_perc5_subtypes[] = {
	{ 0x1f011028,	"Dell PERC 5/e" },
	{ 0x1f021028,	"Dell PERC 5/i" },
	{ 0x0,		"" }
};

static const struct mfi_pci_subtype mfi_gen2_subtypes[] = {
	{ 0x1f151028,	"Dell PERC H800 Adapter" },
	{ 0x1f161028,	"Dell PERC H700 Adapter" },
	{ 0x1f171028,	"Dell PERC H700 Integrated" },
	{ 0x1f181028,	"Dell PERC H700 Modular" },
	{ 0x1f191028,	"Dell PERC H700" },
	{ 0x1f1a1028,	"Dell PERC H800 Proto Adapter" },
	{ 0x1f1b1028,	"Dell PERC H800" },
	{ 0x0,		"" }
};

>>>>>>> origin/master
static const
struct	mfi_pci_device {
	pcireg_t	mpd_vendor;
	pcireg_t	mpd_product;
	pcireg_t	mpd_subvendor;
	pcireg_t	mpd_subproduct;
	char		*mpd_model;
	uint32_t	mpd_flags;
} mfi_pci_devices[] = {
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_MEGARAID_SAS,
	  0,			0,		"",			0 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_MEGARAID_VERDE_ZCR,
<<<<<<< HEAD
	  0,			0,		"",			0 },
	{ PCI_VENDOR_DELL,	PCI_PRODUCT_DELL_PERC5,
	  PCI_VENDOR_DELL,	0x1f01,		"Dell PERC 5/e",	0 },
	{ PCI_VENDOR_DELL,	PCI_PRODUCT_DELL_PERC5,
	  PCI_VENDOR_DELL,	0x1f02,		"Dell PERC 5/i",	0 },
	{ 0 }
};

int
mfi_pci_find_device(void *aux) {
	struct pci_attach_args	*pa = aux;
	int			i;
=======
	  MFI_IOP_XSCALE,	NULL },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS1078,
	  MFI_IOP_PPC,		mfi_1078_subtypes },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS1078DE,
	  MFI_IOP_PPC,		mfi_1078_subtypes },
	{ PCI_VENDOR_DELL,	PCI_PRODUCT_DELL_PERC5,
	  MFI_IOP_XSCALE,	mfi_perc5_subtypes },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2108_1,
	  MFI_IOP_GEN2,		mfi_gen2_subtypes },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2108_2,
	  MFI_IOP_GEN2,		mfi_gen2_subtypes },
};

const struct mfi_pci_device *mfi_pci_find_device(struct pci_attach_args *);

const struct mfi_pci_device *
mfi_pci_find_device(struct pci_attach_args *pa)
{
	const struct mfi_pci_device *mpd;
	int i;

	for (i = 0; i < nitems(mfi_pci_devices); i++) {
		mpd = &mfi_pci_devices[i];
>>>>>>> origin/master

	for (i = 0; mfi_pci_devices[i].mpd_vendor; i++) {
		if (mfi_pci_devices[i].mpd_vendor == PCI_VENDOR(pa->pa_id) &&
		    mfi_pci_devices[i].mpd_product == PCI_PRODUCT(pa->pa_id)) {
		    	DNPRINTF(MFI_D_MISC, "mfi_pci_find_device: %i\n", i);
			return (i);
		}
	}

	return (-1);
}

int
mfi_pci_match(struct device *parent, void *match, void *aux)
{
	int			i;

	if ((i = mfi_pci_find_device(aux)) != -1) {
		DNPRINTF(MFI_D_MISC,
		    "mfi_pci_match: vendor: %04x  product: %04x\n",
		    mfi_pci_devices[i].mpd_vendor,
		    mfi_pci_devices[i].mpd_product);

		return (1);
	}

	return (0);
}

void
mfi_pci_attach(struct device *parent, struct device *self, void *aux)
{
	struct mfi_softc	*sc = (struct mfi_softc *)self;
	struct pci_attach_args	*pa = aux;
	const char		*intrstr;
	pci_intr_handle_t	ih;
	bus_size_t		size;
<<<<<<< HEAD
	pcireg_t		csr;
	uint32_t		subsysid, i;

	subsysid = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);
	for (i = 0; mfi_pci_devices[i].mpd_vendor; i++)
		if (mfi_pci_devices[i].mpd_subvendor == PCI_VENDOR(subsysid) &&
		    mfi_pci_devices[i].mpd_subproduct == PCI_PRODUCT(subsysid)){
				printf(", %s", mfi_pci_devices[i].mpd_model);
				break;
		}

	csr = pci_mapreg_type(pa->pa_pc, pa->pa_tag, MFI_BAR);
	csr |= PCI_MAPREG_MEM_TYPE_32BIT;
	if (pci_mapreg_map(pa, MFI_BAR, csr, 0,
=======
	pcireg_t		reg;
	int			regbar;
	const char		*subtype = NULL;
	char			subid[32];

	mpd = mfi_pci_find_device(pa);
	if (mpd == NULL) {
		printf(": can't find matching pci device\n");
		return;
	}

	if (mpd->mpd_iop == MFI_IOP_GEN2)
		regbar = MFI_BAR_GEN2;
	else
		regbar = MFI_BAR;

	reg = pci_mapreg_type(pa->pa_pc, pa->pa_tag, regbar);
	if (pci_mapreg_map(pa, regbar, reg, 0,
>>>>>>> origin/master
	    &sc->sc_iot, &sc->sc_ioh, NULL, &size, MFI_PCI_MEMSIZE)) {
		printf(": can't map controller pci space\n");
		return;
	}

	sc->sc_dmat = pa->pa_dmat;

	if (pci_intr_map(pa, &ih)) {
		printf(": can't map interrupt\n");
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, size);
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih);
	sc->sc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_BIO, mfi_intr, sc,
	    sc->sc_dev.dv_xname);
	if (!sc->sc_ih) {
		printf(": can't establish interrupt");
		if (intrstr)
			printf(" at %s", intrstr);
		printf("\n");
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, size);
		return;
	}

	printf(": %s\n", intrstr);

	if (mfi_attach(sc)) {
		printf("%s: can't attach", DEVNAME(sc));
		pci_intr_disestablish(pa->pa_pc, sc->sc_ih);
		sc->sc_ih = NULL;
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, size);
	}
}
