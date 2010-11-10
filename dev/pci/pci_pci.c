/*-
 * Copyright (c) 1994,1995 Stefan Esser, Wolfgang StanglMeier
 * Copyright (c) 2000 Michael Smith <msmith@freebsd.org>
 * Copyright (c) 2000 BSDi
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/dev/pci/pci_pci.c,v 1.61 2009/12/10 01:01:53 jkim Exp $");

/*
 * PCI:PCI bridge support.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <machine/bus.h>
#include <sys/rman.h>
#include <sys/sysctl.h>

#include <machine/resource.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcib_private.h>

#include "pcib_if.h"

#ifdef __HAVE_ACPI
#include <contrib/dev/acpica/include/acpi.h>
#include "acpi_if.h"
#else
#define	ACPI_PWR_FOR_SLEEP(x, y, z)
#endif

extern int		pci_do_power_resume;

static int		pcib_probe(device_t dev);
static int		pcib_suspend(device_t dev);
static int		pcib_resume(device_t dev);

static device_method_t pcib_methods[] = {
    /* Device interface */
    DEVMETHOD(device_probe,		pcib_probe),
    DEVMETHOD(device_attach,		pcib_attach),
    DEVMETHOD(device_detach,		bus_generic_detach),
    DEVMETHOD(device_shutdown,		bus_generic_shutdown),
    DEVMETHOD(device_suspend,		pcib_suspend),
    DEVMETHOD(device_resume,		pcib_resume),

    /* Bus interface */
    DEVMETHOD(bus_print_child,		bus_generic_print_child),
    DEVMETHOD(bus_read_ivar,		pcib_read_ivar),
    DEVMETHOD(bus_write_ivar,		pcib_write_ivar),
    DEVMETHOD(bus_alloc_resource,	pcib_alloc_resource),
    DEVMETHOD(bus_release_resource,	bus_generic_release_resource),
    DEVMETHOD(bus_activate_resource,	bus_generic_activate_resource),
    DEVMETHOD(bus_deactivate_resource,	bus_generic_deactivate_resource),
    DEVMETHOD(bus_setup_intr,		bus_generic_setup_intr),
    DEVMETHOD(bus_teardown_intr,	bus_generic_teardown_intr),

    /* pcib interface */
    DEVMETHOD(pcib_maxslots,		pcib_maxslots),
    DEVMETHOD(pcib_read_config,		pcib_read_config),
    DEVMETHOD(pcib_write_config,	pcib_write_config),
    DEVMETHOD(pcib_route_interrupt,	pcib_route_interrupt),
    DEVMETHOD(pcib_alloc_msi,		pcib_alloc_msi),
    DEVMETHOD(pcib_release_msi,		pcib_release_msi),
    DEVMETHOD(pcib_alloc_msix,		pcib_alloc_msix),
    DEVMETHOD(pcib_release_msix,	pcib_release_msix),
    DEVMETHOD(pcib_map_msi,		pcib_map_msi),

    { 0, 0 }
};

static devclass_t pcib_devclass;

DEFINE_CLASS_0(pcib, pcib_driver, pcib_methods, sizeof(struct pcib_softc));
DRIVER_MODULE(pcib, pci, pcib_driver, pcib_devclass, 0, 0);

/*
 * Is the prefetch window open (eg, can we allocate memory in it?)
 */
static int
pcib_is_prefetch_open(struct pcib_softc *sc)
{
	return (sc->pmembase > 0 && sc->pmembase < sc->pmemlimit);
}

/*
 * Is the nonprefetch window open (eg, can we allocate memory in it?)
 */
static int
pcib_is_nonprefetch_open(struct pcib_softc *sc)
{
	return (sc->membase > 0 && sc->membase < sc->memlimit);
}

/*
 * Is the io window open (eg, can we allocate ports in it?)
 */
static int
pcib_is_io_open(struct pcib_softc *sc)
{
	return (sc->iobase > 0 && sc->iobase < sc->iolimit);
}

/*
 * Get current I/O decode.
 */
static void
pcib_get_io_decode(struct pcib_softc *sc)
{
	device_t	dev;
	uint32_t	iolow;

	dev = sc->dev;

	iolow = pci_read_config(dev, PCIR_IOBASEL_1, 1);
	if ((iolow & PCIM_BRIO_MASK) == PCIM_BRIO_32)
		sc->iobase = PCI_PPBIOBASE(
		    pci_read_config(dev, PCIR_IOBASEH_1, 2), iolow);
	else
		sc->iobase = PCI_PPBIOBASE(0, iolow);

	iolow = pci_read_config(dev, PCIR_IOLIMITL_1, 1);
	if ((iolow & PCIM_BRIO_MASK) == PCIM_BRIO_32)
		sc->iolimit = PCI_PPBIOLIMIT(
		    pci_read_config(dev, PCIR_IOLIMITH_1, 2), iolow);
	else
		sc->iolimit = PCI_PPBIOLIMIT(0, iolow);
}

/*
 * Get current memory decode.
 */
static void
pcib_get_mem_decode(struct pcib_softc *sc)
{
	device_t	dev;
	pci_addr_t	pmemlow;

	dev = sc->dev;

	sc->membase = PCI_PPBMEMBASE(0,
	    pci_read_config(dev, PCIR_MEMBASE_1, 2));
	sc->memlimit = PCI_PPBMEMLIMIT(0,
	    pci_read_config(dev, PCIR_MEMLIMIT_1, 2));

	pmemlow = pci_read_config(dev, PCIR_PMBASEL_1, 2);
	if ((pmemlow & PCIM_BRPM_MASK) == PCIM_BRPM_64)
		sc->pmembase = PCI_PPBMEMBASE(
		    pci_read_config(dev, PCIR_PMBASEH_1, 4), pmemlow);
	else
		sc->pmembase = PCI_PPBMEMBASE(0, pmemlow);

	pmemlow = pci_read_config(dev, PCIR_PMLIMITL_1, 2);
	if ((pmemlow & PCIM_BRPM_MASK) == PCIM_BRPM_64)	
		sc->pmemlimit = PCI_PPBMEMLIMIT(
		    pci_read_config(dev, PCIR_PMLIMITH_1, 4), pmemlow);
	else
		sc->pmemlimit = PCI_PPBMEMLIMIT(0, pmemlow);
}

/*
 * Restore previous I/O decode.
 */
static void
pcib_set_io_decode(struct pcib_softc *sc)
{
	device_t	dev;
	uint32_t	iohi;

	dev = sc->dev;

	iohi = sc->iobase >> 16;
	if (iohi > 0)
		pci_write_config(dev, PCIR_IOBASEH_1, iohi, 2);
	pci_write_config(dev, PCIR_IOBASEL_1, sc->iobase >> 8, 1);

	iohi = sc->iolimit >> 16;
	if (iohi > 0)
		pci_write_config(dev, PCIR_IOLIMITH_1, iohi, 2);
	pci_write_config(dev, PCIR_IOLIMITL_1, sc->iolimit >> 8, 1);
}

/*
 * Restore previous memory decode.
 */
static void
pcib_set_mem_decode(struct pcib_softc *sc)
{
	device_t	dev;
	pci_addr_t	pmemhi;

	dev = sc->dev;

	pci_write_config(dev, PCIR_MEMBASE_1, sc->membase >> 16, 2);
	pci_write_config(dev, PCIR_MEMLIMIT_1, sc->memlimit >> 16, 2);

	pmemhi = sc->pmembase >> 32;
	if (pmemhi > 0)
		pci_write_config(dev, PCIR_PMBASEH_1, pmemhi, 4);
	pci_write_config(dev, PCIR_PMBASEL_1, sc->pmembase >> 16, 2);

	pmemhi = sc->pmemlimit >> 32;
	if (pmemhi > 0)
		pci_write_config(dev, PCIR_PMLIMITH_1, pmemhi, 4);
	pci_write_config(dev, PCIR_PMLIMITL_1, sc->pmemlimit >> 16, 2);
}

/*
 * Get current bridge configuration.
 */
static void
pcib_cfg_save(struct pcib_softc *sc)
{
	device_t	dev;

	dev = sc->dev;

	sc->command = pci_read_config(dev, PCIR_COMMAND, 2);
	sc->pribus = pci_read_config(dev, PCIR_PRIBUS_1, 1);
	sc->secbus = pci_read_config(dev, PCIR_SECBUS_1, 1);
	sc->subbus = pci_read_config(dev, PCIR_SUBBUS_1, 1);
	sc->bridgectl = pci_read_config(dev, PCIR_BRIDGECTL_1, 2);
	sc->seclat = pci_read_config(dev, PCIR_SECLAT_1, 1);
	if (sc->command & PCIM_CMD_PORTEN)
		pcib_get_io_decode(sc);
	if (sc->command & PCIM_CMD_MEMEN)
		pcib_get_mem_decode(sc);
}

/*
 * Restore previous bridge configuration.
 */
static void
pcib_cfg_restore(struct pcib_softc *sc)
{
	device_t	dev;

	dev = sc->dev;

	pci_write_config(dev, PCIR_COMMAND, sc->command, 2);
	pci_write_config(dev, PCIR_PRIBUS_1, sc->pribus, 1);
	pci_write_config(dev, PCIR_SECBUS_1, sc->secbus, 1);
	pci_write_config(dev, PCIR_SUBBUS_1, sc->subbus, 1);
	pci_write_config(dev, PCIR_BRIDGECTL_1, sc->bridgectl, 2);
	pci_write_config(dev, PCIR_SECLAT_1, sc->seclat, 1);
	if (sc->command & PCIM_CMD_PORTEN)
		pcib_set_io_decode(sc);
	if (sc->command & PCIM_CMD_MEMEN)
		pcib_set_mem_decode(sc);
}

/*
 * Generic device interface
 */
static int
pcib_probe(device_t dev)
{
    if ((pci_get_class(dev) == PCIC_BRIDGE) &&
	(pci_get_subclass(dev) == PCIS_BRIDGE_PCI)) {
	device_set_desc(dev, "PCI-PCI bridge");
	return(-10000);
    }
    return(ENXIO);
}

void
pcib_attach_common(device_t dev)
{
    struct pcib_softc	*sc;
    struct sysctl_ctx_list *sctx;
    struct sysctl_oid	*soid;

    sc = device_get_softc(dev);
    sc->dev = dev;

    /*
     * Get current bridge configuration.
     */
    sc->domain = pci_get_domain(dev);
    sc->secstat = pci_read_config(dev, PCIR_SECSTAT_1, 2);
    pcib_cfg_save(sc);

    /*
     * Setup sysctl reporting nodes
     */
    sctx = device_get_sysctl_ctx(dev);
    soid = device_get_sysctl_tree(dev);
    SYSCTL_ADD_UINT(sctx, SYSCTL_CHILDREN(soid), OID_AUTO, "domain",
      CTLFLAG_RD, &sc->domain, 0, "Domain number");
    SYSCTL_ADD_UINT(sctx, SYSCTL_CHILDREN(soid), OID_AUTO, "pribus",
      CTLFLAG_RD, &sc->pribus, 0, "Primary bus number");
    SYSCTL_ADD_UINT(sctx, SYSCTL_CHILDREN(soid), OID_AUTO, "secbus",
      CTLFLAG_RD, &sc->secbus, 0, "Secondary bus number");
    SYSCTL_ADD_UINT(sctx, SYSCTL_CHILDREN(soid), OID_AUTO, "subbus",
      CTLFLAG_RD, &sc->subbus, 0, "Subordinate bus number");

    /*
     * Quirk handling.
     */
    switch (pci_get_devid(dev)) {
    case 0x12258086:		/* Intel 82454KX/GX (Orion) */
	{
	    uint8_t	supbus;

	    supbus = pci_read_config(dev, 0x41, 1);
	    if (supbus != 0xff) {
		sc->secbus = supbus + 1;
		sc->subbus = supbus + 1;
	    }
	    break;
	}

    /*
     * The i82380FB mobile docking controller is a PCI-PCI bridge,
     * and it is a subtractive bridge.  However, the ProgIf is wrong
     * so the normal setting of PCIB_SUBTRACTIVE bit doesn't
     * happen.  There's also a Toshiba bridge that behaves this
     * way.
     */
    case 0x124b8086:		/* Intel 82380FB Mobile */
    case 0x060513d7:		/* Toshiba ???? */
	sc->flags |= PCIB_SUBTRACTIVE;
	break;

    /* Compaq R3000 BIOS sets wrong subordinate bus number. */
    case 0x00dd10de:
	{
	    char *cp;

	    if ((cp = getenv("smbios.planar.maker")) == NULL)
		break;
	    if (strncmp(cp, "Compal", 6) != 0) {
		freeenv(cp);
		break;
	    }
	    freeenv(cp);
	    if ((cp = getenv("smbios.planar.product")) == NULL)
		break;
	    if (strncmp(cp, "08A0", 4) != 0) {
		freeenv(cp);
		break;
	    }
	    freeenv(cp);
	    if (sc->subbus < 0xa) {
		pci_write_config(dev, PCIR_SUBBUS_1, 0xa, 1);
		sc->subbus = pci_read_config(dev, PCIR_SUBBUS_1, 1);
	    }
	    break;
	}
    }

    if (pci_msi_device_blacklisted(dev))
	sc->flags |= PCIB_DISABLE_MSI;

    /*
     * Intel 815, 845 and other chipsets say they are PCI-PCI bridges,
     * but have a ProgIF of 0x80.  The 82801 family (AA, AB, BAM/CAM,
     * BA/CA/DB and E) PCI bridges are HUB-PCI bridges, in Intelese.
     * This means they act as if they were subtractively decoding
     * bridges and pass all transactions.  Mark them and real ProgIf 1
     * parts as subtractive.
     */
    if ((pci_get_devid(dev) & 0xff00ffff) == 0x24008086 ||
      pci_read_config(dev, PCIR_PROGIF, 1) == PCIP_BRIDGE_PCI_SUBTRACTIVE)
	sc->flags |= PCIB_SUBTRACTIVE;
	
    if (bootverbose) {
	device_printf(dev, "  domain            %d\n", sc->domain);
	device_printf(dev, "  secondary bus     %d\n", sc->secbus);
	device_printf(dev, "  subordinate bus   %d\n", sc->subbus);
	device_printf(dev, "  I/O decode        0x%x-0x%x\n", sc->iobase, sc->iolimit);
	if (pcib_is_nonprefetch_open(sc))
	    device_printf(dev, "  memory decode     0x%jx-0x%jx\n",
	      (uintmax_t)sc->membase, (uintmax_t)sc->memlimit);
	if (pcib_is_prefetch_open(sc))
	    device_printf(dev, "  prefetched decode 0x%jx-0x%jx\n",
	      (uintmax_t)sc->pmembase, (uintmax_t)sc->pmemlimit);
	else
	    device_printf(dev, "  no prefetched decode\n");
	if (sc->flags & PCIB_SUBTRACTIVE)
	    device_printf(dev, "  Subtractively decoded bridge.\n");
    }

    /*
     * XXX If the secondary bus number is zero, we should assign a bus number
     *     since the BIOS hasn't, then initialise the bridge.  A simple
     *     bus_alloc_resource with the a couple of busses seems like the right
     *     approach, but we don't know what busses the BIOS might have already
     *     assigned to other bridges on this bus that probe later than we do.
     *
     *     If the subordinate bus number is less than the secondary bus number,
     *     we should pick a better value.  One sensible alternative would be to
     *     pick 255; the only tradeoff here is that configuration transactions
     *     would be more widely routed than absolutely necessary.  We could
     *     then do a walk of the tree later and fix it.
     */
}

int
pcib_attach(device_t dev)
{
    struct pcib_softc	*sc;
    device_t		child;

    pcib_attach_common(dev);
    sc = device_get_softc(dev);
    if (sc->secbus != 0) {
	child = device_add_child(dev, "pci", sc->secbus);
	if (child != NULL)
	    return(bus_generic_attach(dev));
    }

    /* no secondary bus; we should have fixed this */
    return(0);
}

int
pcib_suspend(device_t dev)
{
	device_t	acpi_dev;
	int		dstate, error;

	pcib_cfg_save(device_get_softc(dev));
	error = bus_generic_suspend(dev);
	if (error == 0 && pci_do_power_resume) {
		acpi_dev = devclass_get_device(devclass_find("acpi"), 0);
		if (acpi_dev != NULL) {
			dstate = PCI_POWERSTATE_D3;
			ACPI_PWR_FOR_SLEEP(acpi_dev, dev, &dstate);
			pci_set_powerstate(dev, dstate);
		}
	}
	return (error);
}

int
pcib_resume(device_t dev)
{
	device_t	acpi_dev;

	if (pci_do_power_resume) {
		acpi_dev = devclass_get_device(devclass_find("acpi"), 0);
		if (acpi_dev != NULL) {
			ACPI_PWR_FOR_SLEEP(acpi_dev, dev, NULL);
			pci_set_powerstate(dev, PCI_POWERSTATE_D0);
		}
	}
	pcib_cfg_restore(device_get_softc(dev));
	return (bus_generic_resume(dev));
}

int
pcib_read_ivar(device_t dev, device_t child, int which, uintptr_t *result)
{
    struct pcib_softc	*sc = device_get_softc(dev);
    
    switch (which) {
    case PCIB_IVAR_DOMAIN:
	*result = sc->domain;
	return(0);
    case PCIB_IVAR_BUS:
	*result = sc->secbus;
	return(0);
    }
    return(ENOENT);
}

int
pcib_write_ivar(device_t dev, device_t child, int which, uintptr_t value)
{
    struct pcib_softc	*sc = device_get_softc(dev);

    switch (which) {
    case PCIB_IVAR_DOMAIN:
	return(EINVAL);
    case PCIB_IVAR_BUS:
	sc->secbus = value;
	return(0);
    }
    return(ENOENT);
}

/*
 * We have to trap resource allocation requests and ensure that the bridge
 * is set up to, or capable of handling them.
 */
struct resource *
pcib_alloc_resource(device_t dev, device_t child, int type, int *rid, 
    u_long start, u_long end, u_long count, u_int flags)
{
	struct pcib_softc	*sc = device_get_softc(dev);
	const char *name, *suffix;
	int ok;

	/*
	 * Fail the allocation for this range if it's not supported.
	 */
	name = device_get_nameunit(child);
	if (name == NULL) {
		name = "";
		suffix = "";
	} else
		suffix = " ";
	switch (type) {
	case SYS_RES_IOPORT:
		ok = 0;
		if (!pcib_is_io_open(sc))
			break;
		ok = (start >= sc->iobase && end <= sc->iolimit);

		/*
		 * Make sure we allow access to VGA I/O addresses when the
		 * bridge has the "VGA Enable" bit set.
		 */
		if (!ok && pci_is_vga_ioport_range(start, end))
			ok = (sc->bridgectl & PCIB_BCR_VGA_ENABLE) ? 1 : 0;

		if ((sc->flags & PCIB_SUBTRACTIVE) == 0) {
			if (!ok) {
				if (start < sc->iobase)
					start = sc->iobase;
				if (end > sc->iolimit)
					end = sc->iolimit;
				if (start < end)
					ok = 1;
			}
		} else {
			ok = 1;
#if 0
			/*
			 * If we overlap with the subtractive range, then
			 * pick the upper range to use.
			 */
			if (start < sc->iolimit && end > sc->iobase)
				start = sc->iolimit + 1;
#endif
		}
		if (end < start) {
			device_printf(dev, "ioport: end (%lx) < start (%lx)\n",
			    end, start);
			start = 0;
			end = 0;
			ok = 0;
		}
		if (!ok) {
			device_printf(dev, "%s%srequested unsupported I/O "
			    "range 0x%lx-0x%lx (decoding 0x%x-0x%x)\n",
			    name, suffix, start, end, sc->iobase, sc->iolimit);
			return (NULL);
		}
		if (bootverbose)
			device_printf(dev,
			    "%s%srequested I/O range 0x%lx-0x%lx: in range\n",
			    name, suffix, start, end);
		break;

	case SYS_RES_MEMORY:
		ok = 0;
		if (pcib_is_nonprefetch_open(sc))
			ok = ok || (start >= sc->membase && end <= sc->memlimit);
		if (pcib_is_prefetch_open(sc))
			ok = ok || (start >= sc->pmembase && end <= sc->pmemlimit);

		/*
		 * Make sure we allow access to VGA memory addresses when the
		 * bridge has the "VGA Enable" bit set.
		 */
		if (!ok && pci_is_vga_memory_range(start, end))
			ok = (sc->bridgectl & PCIB_BCR_VGA_ENABLE) ? 1 : 0;

		if ((sc->flags & PCIB_SUBTRACTIVE) == 0) {
			if (!ok) {
				ok = 1;
				if (flags & RF_PREFETCHABLE) {
					if (pcib_is_prefetch_open(sc)) {
						if (start < sc->pmembase)
							start = sc->pmembase;
						if (end > sc->pmemlimit)
							end = sc->pmemlimit;
					} else {
						ok = 0;
					}
				} else {	/* non-prefetchable */
					if (pcib_is_nonprefetch_open(sc)) {
						if (start < sc->membase)
							start = sc->membase;
						if (end > sc->memlimit)
							end = sc->memlimit;
					} else {
						ok = 0;
					}
				}
			}
		} else if (!ok) {
			ok = 1;	/* subtractive bridge: always ok */
#if 0
			if (pcib_is_nonprefetch_open(sc)) {
				if (start < sc->memlimit && end > sc->membase)
					start = sc->memlimit + 1;
			}
			if (pcib_is_prefetch_open(sc)) {
				if (start < sc->pmemlimit && end > sc->pmembase)
					start = sc->pmemlimit + 1;
			}
#endif
		}
		if (end < start) {
			device_printf(dev, "memory: end (%lx) < start (%lx)\n",
			    end, start);
			start = 0;
			end = 0;
			ok = 0;
		}
		if (!ok && bootverbose)
			device_printf(dev,
			    "%s%srequested unsupported memory range %#lx-%#lx "
			    "(decoding %#jx-%#jx, %#jx-%#jx)\n",
			    name, suffix, start, end,
			    (uintmax_t)sc->membase, (uintmax_t)sc->memlimit,
			    (uintmax_t)sc->pmembase, (uintmax_t)sc->pmemlimit);
		if (!ok)
			return (NULL);
		if (bootverbose)
			device_printf(dev,"%s%srequested memory range "
			    "0x%lx-0x%lx: good\n",
			    name, suffix, start, end);
		break;

	default:
		break;
	}
	/*
	 * Bridge is OK decoding this resource, so pass it up.
	 */
	return (bus_generic_alloc_resource(dev, child, type, rid, start, end,
	    count, flags));
}

/*
 * PCIB interface.
 */
int
pcib_maxslots(device_t dev)
{
    return(PCI_SLOTMAX);
}

/*
 * Since we are a child of a PCI bus, its parent must support the pcib interface.
 */
uint32_t
pcib_read_config(device_t dev, u_int b, u_int s, u_int f, u_int reg, int width)
{
    return(PCIB_READ_CONFIG(device_get_parent(device_get_parent(dev)), b, s, f, reg, width));
}

void
pcib_write_config(device_t dev, u_int b, u_int s, u_int f, u_int reg, uint32_t val, int width)
{
    PCIB_WRITE_CONFIG(device_get_parent(device_get_parent(dev)), b, s, f, reg, val, width);
}

/*
 * Route an interrupt across a PCI bridge.
 */
int
pcib_route_interrupt(device_t pcib, device_t dev, int pin)
{
    device_t	bus;
    int		parent_intpin;
    int		intnum;

    /*	
     *
     * The PCI standard defines a swizzle of the child-side device/intpin to
     * the parent-side intpin as follows.
     *
     * device = device on child bus
     * child_intpin = intpin on child bus slot (0-3)
     * parent_intpin = intpin on parent bus slot (0-3)
     *
     * parent_intpin = (device + child_intpin) % 4
     */
    parent_intpin = (pci_get_slot(dev) + (pin - 1)) % 4;

    /*
     * Our parent is a PCI bus.  Its parent must export the pcib interface
     * which includes the ability to route interrupts.
     */
    bus = device_get_parent(pcib);
    intnum = PCIB_ROUTE_INTERRUPT(device_get_parent(bus), pcib, parent_intpin + 1);
    if (PCI_INTERRUPT_VALID(intnum) && bootverbose) {
	device_printf(pcib, "slot %d INT%c is routed to irq %d\n",
	    pci_get_slot(dev), 'A' + pin - 1, intnum);
    }
    return(intnum);
}

/* Pass request to alloc MSI/MSI-X messages up to the parent bridge. */
int
pcib_alloc_msi(device_t pcib, device_t dev, int count, int maxcount, int *irqs)
{
	struct pcib_softc *sc = device_get_softc(pcib);
	device_t bus;

	if (sc->flags & PCIB_DISABLE_MSI)
		return (ENXIO);
	bus = device_get_parent(pcib);
	return (PCIB_ALLOC_MSI(device_get_parent(bus), dev, count, maxcount,
	    irqs));
}

/* Pass request to release MSI/MSI-X messages up to the parent bridge. */
int
pcib_release_msi(device_t pcib, device_t dev, int count, int *irqs)
{
	device_t bus;

	bus = device_get_parent(pcib);
	return (PCIB_RELEASE_MSI(device_get_parent(bus), dev, count, irqs));
}

/* Pass request to alloc an MSI-X message up to the parent bridge. */
int
pcib_alloc_msix(device_t pcib, device_t dev, int *irq)
{
	struct pcib_softc *sc = device_get_softc(pcib);
	device_t bus;

	if (sc->flags & PCIB_DISABLE_MSI)
		return (ENXIO);
	bus = device_get_parent(pcib);
	return (PCIB_ALLOC_MSIX(device_get_parent(bus), dev, irq));
}

/* Pass request to release an MSI-X message up to the parent bridge. */
int
pcib_release_msix(device_t pcib, device_t dev, int irq)
{
	device_t bus;

	bus = device_get_parent(pcib);
	return (PCIB_RELEASE_MSIX(device_get_parent(bus), dev, irq));
}

/* Pass request to map MSI/MSI-X message up to parent bridge. */
int
pcib_map_msi(device_t pcib, device_t dev, int irq, uint64_t *addr,
    uint32_t *data)
{
	device_t bus;
	int error;

	bus = device_get_parent(pcib);
	error = PCIB_MAP_MSI(device_get_parent(bus), dev, irq, addr, data);
	if (error)
		return (error);

	pci_ht_map_msi(pcib, *addr);
	return (0);
}

/*
 * Try to read the bus number of a host-PCI bridge using appropriate config
 * registers.
 */
int
host_pcib_get_busno(pci_read_config_fn read_config, int bus, int slot, int func,
    uint8_t *busnum)
{
	uint32_t id;

	id = read_config(bus, slot, func, PCIR_DEVVENDOR, 4);
	if (id == 0xffffffff)
		return (0);

	switch (id) {
	case 0x12258086:
		/* Intel 824?? */
		/* XXX This is a guess */
		/* *busnum = read_config(bus, slot, func, 0x41, 1); */
		*busnum = bus;
		break;
	case 0x84c48086:
		/* Intel 82454KX/GX (Orion) */
		*busnum = read_config(bus, slot, func, 0x4a, 1);
		break;
	case 0x84ca8086:
		/*
		 * For the 450nx chipset, there is a whole bundle of
		 * things pretending to be host bridges. The MIOC will 
		 * be seen first and isn't really a pci bridge (the
		 * actual busses are attached to the PXB's). We need to 
		 * read the registers of the MIOC to figure out the
		 * bus numbers for the PXB channels.
		 *
		 * Since the MIOC doesn't have a pci bus attached, we
		 * pretend it wasn't there.
		 */
		return (0);
	case 0x84cb8086:
		switch (slot) {
		case 0x12:
			/* Intel 82454NX PXB#0, Bus#A */
			*busnum = read_config(bus, 0x10, func, 0xd0, 1);
			break;
		case 0x13:
			/* Intel 82454NX PXB#0, Bus#B */
			*busnum = read_config(bus, 0x10, func, 0xd1, 1) + 1;
			break;
		case 0x14:
			/* Intel 82454NX PXB#1, Bus#A */
			*busnum = read_config(bus, 0x10, func, 0xd3, 1);
			break;
		case 0x15:
			/* Intel 82454NX PXB#1, Bus#B */
			*busnum = read_config(bus, 0x10, func, 0xd4, 1) + 1;
			break;
		}
		break;

		/* ServerWorks -- vendor 0x1166 */
	case 0x00051166:
	case 0x00061166:
	case 0x00081166:
	case 0x00091166:
	case 0x00101166:
	case 0x00111166:
	case 0x00171166:
	case 0x01011166:
	case 0x010f1014:
	case 0x02011166:
	case 0x03021014:
		*busnum = read_config(bus, slot, func, 0x44, 1);
		break;

		/* Compaq/HP -- vendor 0x0e11 */
	case 0x60100e11:
		*busnum = read_config(bus, slot, func, 0xc8, 1);
		break;
	default:
		/* Don't know how to read bus number. */
		return 0;
	}

	return 1;
}
