/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@carlstedt.se) at
 * Carlstedt Research & Technology.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/dev/usb2/controller/uhci2_pci.c,v 1.2 2008/11/10 20:54:31 thompsa Exp $");

/* Universal Host Controller Interface
 *
 * UHCI spec: http://www.intel.com/
 */

/* The low level controller code for UHCI has been split into
 * PCI probes and UHCI specific code. This was done to facilitate the
 * sharing of code between *BSD's
 */

#include <dev/usb2/include/usb2_mfunc.h>
#include <dev/usb2/include/usb2_defs.h>
#include <dev/usb2/include/usb2_standard.h>

#include <dev/usb2/core/usb2_core.h>
#include <dev/usb2/core/usb2_busdma.h>
#include <dev/usb2/core/usb2_process.h>
#include <dev/usb2/core/usb2_config_td.h>
#include <dev/usb2/core/usb2_sw_transfer.h>
#include <dev/usb2/core/usb2_util.h>
#include <dev/usb2/core/usb2_debug.h>

#include <dev/usb2/controller/usb2_controller.h>
#include <dev/usb2/controller/usb2_bus.h>
#include <dev/usb2/controller/usb2_pci.h>
#include <dev/usb2/controller/uhci2.h>

#define	PCI_UHCI_VENDORID_INTEL		0x8086
#define	PCI_UHCI_VENDORID_VIA		0x1106

/* PIIX4E has no separate stepping */

#define	PCI_UHCI_BASE_REG               0x20

static device_probe_t uhci_pci_probe;
static device_attach_t uhci_pci_attach;
static device_detach_t uhci_pci_detach;
static device_suspend_t uhci_pci_suspend;
static device_resume_t uhci_pci_resume;

static int
uhci_pci_suspend(device_t self)
{
	uhci_softc_t *sc = device_get_softc(self);
	int err;

	err = bus_generic_suspend(self);
	if (err) {
		return (err);
	}
	uhci_suspend(sc);
	return (0);
}

static int
uhci_pci_resume(device_t self)
{
	uhci_softc_t *sc = device_get_softc(self);

	pci_write_config(self, PCI_LEGSUP, PCI_LEGSUP_USBPIRQDEN, 2);

	uhci_resume(sc);

	bus_generic_resume(self);
	return (0);
}

static const char *
uhci_pci_match(device_t self)
{
	uint32_t device_id = pci_get_devid(self);

	switch (device_id) {
	case 0x26888086:
		return ("Intel 631XESB/632XESB/3100 USB controller USB-1");

	case 0x26898086:
		return ("Intel 631XESB/632XESB/3100 USB controller USB-2");

	case 0x268a8086:
		return ("Intel 631XESB/632XESB/3100 USB controller USB-3");

	case 0x268b8086:
		return ("Intel 631XESB/632XESB/3100 USB controller USB-4");

	case 0x70208086:
		return ("Intel 82371SB (PIIX3) USB controller");

	case 0x71128086:
		return ("Intel 82371AB/EB (PIIX4) USB controller");

	case 0x24128086:
		return ("Intel 82801AA (ICH) USB controller");

	case 0x24228086:
		return ("Intel 82801AB (ICH0) USB controller");

	case 0x24428086:
		return ("Intel 82801BA/BAM (ICH2) USB controller USB-A");

	case 0x24448086:
		return ("Intel 82801BA/BAM (ICH2) USB controller USB-B");

	case 0x24828086:
		return ("Intel 82801CA/CAM (ICH3) USB controller USB-A");

	case 0x24848086:
		return ("Intel 82801CA/CAM (ICH3) USB controller USB-B");

	case 0x24878086:
		return ("Intel 82801CA/CAM (ICH3) USB controller USB-C");

	case 0x24c28086:
		return ("Intel 82801DB (ICH4) USB controller USB-A");

	case 0x24c48086:
		return ("Intel 82801DB (ICH4) USB controller USB-B");

	case 0x24c78086:
		return ("Intel 82801DB (ICH4) USB controller USB-C");

	case 0x24d28086:
		return ("Intel 82801EB (ICH5) USB controller USB-A");

	case 0x24d48086:
		return ("Intel 82801EB (ICH5) USB controller USB-B");

	case 0x24d78086:
		return ("Intel 82801EB (ICH5) USB controller USB-C");

	case 0x24de8086:
		return ("Intel 82801EB (ICH5) USB controller USB-D");

	case 0x26588086:
		return ("Intel 82801FB/FR/FW/FRW (ICH6) USB controller USB-A");

	case 0x26598086:
		return ("Intel 82801FB/FR/FW/FRW (ICH6) USB controller USB-B");

	case 0x265a8086:
		return ("Intel 82801FB/FR/FW/FRW (ICH6) USB controller USB-C");

	case 0x265b8086:
		return ("Intel 82801FB/FR/FW/FRW (ICH6) USB controller USB-D");

	case 0x28308086:
		return ("Intel 82801H (ICH8) USB controller USB-A");
	case 0x28318086:
		return ("Intel 82801H (ICH8) USB controller USB-B");
	case 0x28328086:
		return ("Intel 82801H (ICH8) USB controller USB-C");
	case 0x28348086:
		return ("Intel 82801H (ICH8) USB controller USB-D");
	case 0x28358086:
		return ("Intel 82801H (ICH8) USB controller USB-E");
	case 0x29348086:
		return ("Intel 82801I (ICH9) USB controller");
	case 0x29358086:
		return ("Intel 82801I (ICH9) USB controller");
	case 0x29368086:
		return ("Intel 82801I (ICH9) USB controller");
	case 0x29378086:
		return ("Intel 82801I (ICH9) USB controller");
	case 0x29388086:
		return ("Intel 82801I (ICH9) USB controller");
	case 0x29398086:
		return ("Intel 82801I (ICH9) USB controller");

	case 0x719a8086:
		return ("Intel 82443MX USB controller");

	case 0x76028086:
		return ("Intel 82372FB/82468GX USB controller");

	case 0x30381106:
		return ("VIA 83C572 USB controller");

	default:
		break;
	}

	if ((pci_get_class(self) == PCIC_SERIALBUS) &&
	    (pci_get_subclass(self) == PCIS_SERIALBUS_USB) &&
	    (pci_get_progif(self) == PCI_INTERFACE_UHCI)) {
		return ("UHCI (generic) USB controller");
	}
	return (NULL);
}

static int
uhci_pci_probe(device_t self)
{
	const char *desc = uhci_pci_match(self);

	if (desc) {
		device_set_desc(self, desc);
		return (0);
	} else {
		return (ENXIO);
	}
}

static int
uhci_pci_attach(device_t self)
{
	uhci_softc_t *sc = device_get_softc(self);
	int rid;
	int err;

	if (sc == NULL) {
		device_printf(self, "Could not allocate sc\n");
		return (ENXIO);
	}
	/* get all DMA memory */

	if (usb2_bus_mem_alloc_all(&sc->sc_bus, USB_GET_DMA_TAG(self),
	    &uhci_iterate_hw_softc)) {
		return ENOMEM;
	}
	sc->sc_dev = self;

	pci_enable_busmaster(self);

	rid = PCI_UHCI_BASE_REG;
	sc->sc_io_res = bus_alloc_resource_any(self, SYS_RES_IOPORT, &rid,
	    RF_ACTIVE);
	if (!sc->sc_io_res) {
		device_printf(self, "Could not map ports\n");
		goto error;
	}
	sc->sc_io_tag = rman_get_bustag(sc->sc_io_res);
	sc->sc_io_hdl = rman_get_bushandle(sc->sc_io_res);
	sc->sc_io_size = rman_get_size(sc->sc_io_res);

	/* disable interrupts */
	bus_space_write_2(sc->sc_io_tag, sc->sc_io_hdl, UHCI_INTR, 0);

	rid = 0;
	sc->sc_irq_res = bus_alloc_resource_any(self, SYS_RES_IRQ, &rid,
	    RF_SHAREABLE | RF_ACTIVE);
	if (sc->sc_irq_res == NULL) {
		device_printf(self, "Could not allocate irq\n");
		goto error;
	}
	sc->sc_bus.bdev = device_add_child(self, "usbus", -1);
	if (!sc->sc_bus.bdev) {
		device_printf(self, "Could not add USB device\n");
		goto error;
	}
	device_set_ivars(sc->sc_bus.bdev, &sc->sc_bus);

	/*
	 * uhci_pci_match must never return NULL if uhci_pci_probe
	 * succeeded
	 */
	device_set_desc(sc->sc_bus.bdev, uhci_pci_match(self));
	switch (pci_get_vendor(self)) {
	case PCI_UHCI_VENDORID_INTEL:
		sprintf(sc->sc_vendor, "Intel");
		break;
	case PCI_UHCI_VENDORID_VIA:
		sprintf(sc->sc_vendor, "VIA");
		break;
	default:
		if (bootverbose) {
			device_printf(self, "(New UHCI DeviceId=0x%08x)\n",
			    pci_get_devid(self));
		}
		sprintf(sc->sc_vendor, "(0x%04x)", pci_get_vendor(self));
	}

	switch (pci_read_config(self, PCI_USBREV, 1) & PCI_USB_REV_MASK) {
	case PCI_USB_REV_PRE_1_0:
		sc->sc_bus.usbrev = USB_REV_PRE_1_0;
		break;
	case PCI_USB_REV_1_0:
		sc->sc_bus.usbrev = USB_REV_1_0;
		break;
	default:
		sc->sc_bus.usbrev = USB_REV_UNKNOWN;
		break;
	}

	err = usb2_config_td_setup(&sc->sc_config_td, sc, &sc->sc_bus.bus_mtx,
	    NULL, 0, 4);
	if (err) {
		device_printf(self, "could not setup config thread!\n");
		goto error;
	}
#if (__FreeBSD_version >= 700031)
	err = bus_setup_intr(self, sc->sc_irq_res, INTR_TYPE_BIO | INTR_MPSAFE,
	    NULL, (void *)(void *)uhci_interrupt, sc, &sc->sc_intr_hdl);
#else
	err = bus_setup_intr(self, sc->sc_irq_res, INTR_TYPE_BIO | INTR_MPSAFE,
	    (void *)(void *)uhci_interrupt, sc, &sc->sc_intr_hdl);
#endif

	if (err) {
		device_printf(self, "Could not setup irq, %d\n", err);
		sc->sc_intr_hdl = NULL;
		goto error;
	}
	/*
	 * Set the PIRQD enable bit and switch off all the others. We don't
	 * want legacy support to interfere with us XXX Does this also mean
	 * that the BIOS won't touch the keyboard anymore if it is connected
	 * to the ports of the root hub?
	 */
#if USB_DEBUG
	if (pci_read_config(self, PCI_LEGSUP, 2) != PCI_LEGSUP_USBPIRQDEN) {
		device_printf(self, "LegSup = 0x%04x\n",
		    pci_read_config(self, PCI_LEGSUP, 2));
	}
#endif
	pci_write_config(self, PCI_LEGSUP, PCI_LEGSUP_USBPIRQDEN, 2);

	err = uhci_init(sc);
	if (!err) {
		err = device_probe_and_attach(sc->sc_bus.bdev);
	}
	if (err) {
		device_printf(self, "USB init failed\n");
		goto error;
	}
	return (0);

error:
	uhci_pci_detach(self);
	return (ENXIO);
}

int
uhci_pci_detach(device_t self)
{
	uhci_softc_t *sc = device_get_softc(self);
	device_t bdev;

	usb2_config_td_drain(&sc->sc_config_td);

	if (sc->sc_bus.bdev) {
		bdev = sc->sc_bus.bdev;
		device_detach(bdev);
		device_delete_child(self, bdev);
	}
	/* during module unload there are lots of children leftover */
	device_delete_all_children(self);

	/*
	 * disable interrupts that might have been switched on in
	 * uhci_init.
	 */
	if (sc->sc_io_res) {
		USB_BUS_LOCK(&sc->sc_bus);

		/* stop the controller */
		uhci_reset(sc);

		USB_BUS_UNLOCK(&sc->sc_bus);
	}
	pci_disable_busmaster(self);

	if (sc->sc_irq_res && sc->sc_intr_hdl) {
		int err = bus_teardown_intr(self, sc->sc_irq_res, sc->sc_intr_hdl);

		if (err) {
			/* XXX or should we panic? */
			device_printf(self, "Could not tear down irq, %d\n",
			    err);
		}
		sc->sc_intr_hdl = NULL;
	}
	if (sc->sc_irq_res) {
		bus_release_resource(self, SYS_RES_IRQ, 0, sc->sc_irq_res);
		sc->sc_irq_res = NULL;
	}
	if (sc->sc_io_res) {
		bus_release_resource(self, SYS_RES_IOPORT, PCI_UHCI_BASE_REG,
		    sc->sc_io_res);
		sc->sc_io_res = NULL;
	}
	usb2_config_td_unsetup(&sc->sc_config_td);

	usb2_bus_mem_free_all(&sc->sc_bus, &uhci_iterate_hw_softc);

	return (0);
}

static driver_t uhci_driver =
{
	.name = "uhci",
	.methods = (device_method_t[]){
		/* device interface */
		DEVMETHOD(device_probe, uhci_pci_probe),
		DEVMETHOD(device_attach, uhci_pci_attach),
		DEVMETHOD(device_detach, uhci_pci_detach),

		DEVMETHOD(device_suspend, uhci_pci_suspend),
		DEVMETHOD(device_resume, uhci_pci_resume),
		DEVMETHOD(device_shutdown, bus_generic_shutdown),

		/* Bus interface */
		DEVMETHOD(bus_print_child, bus_generic_print_child),
		{0, 0}
	},
	.size = sizeof(struct uhci_softc),
};

static devclass_t uhci_devclass;

DRIVER_MODULE(uhci, pci, uhci_driver, uhci_devclass, 0, 0);
DRIVER_MODULE(uhci, cardbus, uhci_driver, uhci_devclass, 0, 0);
MODULE_DEPEND(uhci, usb2_controller, 1, 1, 1);
MODULE_DEPEND(uhci, usb2_core, 1, 1, 1);
