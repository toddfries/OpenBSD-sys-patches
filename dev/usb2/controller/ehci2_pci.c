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
__FBSDID("$FreeBSD: src/sys/dev/usb2/controller/ehci2_pci.c,v 1.2 2008/11/10 20:54:31 thompsa Exp $");

/*
 * USB Enhanced Host Controller Driver, a.k.a. USB 2.0 controller.
 *
 * The EHCI 1.0 spec can be found at
 * http://developer.intel.com/technology/usb/download/ehci-r10.pdf
 * and the USB 2.0 spec at
 * http://www.usb.org/developers/docs/usb_20.zip
 */

/* The low level controller code for EHCI has been split into
 * PCI probes and EHCI specific code. This was done to facilitate the
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

#include <dev/usb2/controller/usb2_controller.h>
#include <dev/usb2/controller/usb2_bus.h>
#include <dev/usb2/controller/usb2_pci.h>
#include <dev/usb2/controller/ehci2.h>

#define	PCI_EHCI_VENDORID_ACERLABS	0x10b9
#define	PCI_EHCI_VENDORID_AMD		0x1022
#define	PCI_EHCI_VENDORID_APPLE		0x106b
#define	PCI_EHCI_VENDORID_ATI		0x1002
#define	PCI_EHCI_VENDORID_CMDTECH	0x1095
#define	PCI_EHCI_VENDORID_INTEL		0x8086
#define	PCI_EHCI_VENDORID_NEC		0x1033
#define	PCI_EHCI_VENDORID_OPTI		0x1045
#define	PCI_EHCI_VENDORID_PHILIPS	0x1131
#define	PCI_EHCI_VENDORID_SIS		0x1039
#define	PCI_EHCI_VENDORID_NVIDIA	0x12D2
#define	PCI_EHCI_VENDORID_NVIDIA2	0x10DE
#define	PCI_EHCI_VENDORID_VIA		0x1106

#define	PCI_EHCI_BASE_REG	0x10

static void ehci_pci_takecontroller(device_t self);

static device_probe_t ehci_pci_probe;
static device_attach_t ehci_pci_attach;
static device_detach_t ehci_pci_detach;
static device_suspend_t ehci_pci_suspend;
static device_resume_t ehci_pci_resume;
static device_shutdown_t ehci_pci_shutdown;

static int
ehci_pci_suspend(device_t self)
{
	ehci_softc_t *sc = device_get_softc(self);
	int err;

	err = bus_generic_suspend(self);
	if (err)
		return (err);
	ehci_suspend(sc);
	return (0);
}

static int
ehci_pci_resume(device_t self)
{
	ehci_softc_t *sc = device_get_softc(self);

	ehci_pci_takecontroller(self);
	ehci_resume(sc);

	bus_generic_resume(self);

	return (0);
}

static int
ehci_pci_shutdown(device_t self)
{
	ehci_softc_t *sc = device_get_softc(self);
	int err;

	err = bus_generic_shutdown(self);
	if (err)
		return (err);
	ehci_shutdown(sc);

	return (0);
}

static const char *
ehci_pci_match(device_t self)
{
	uint32_t device_id = pci_get_devid(self);

	switch (device_id) {
	case 0x268c8086:
		return ("Intel 63XXESB USB 2.0 controller");

	case 0x523910b9:
		return "ALi M5239 USB 2.0 controller";

	case 0x10227463:
		return "AMD 8111 USB 2.0 controller";

	case 0x20951022:
		return ("AMD CS5536 (Geode) USB 2.0 controller");

	case 0x43451002:
		return "ATI SB200 USB 2.0 controller";
	case 0x43731002:
		return "ATI SB400 USB 2.0 controller";

	case 0x25ad8086:
		return "Intel 6300ESB USB 2.0 controller";
	case 0x24cd8086:
		return "Intel 82801DB/L/M (ICH4) USB 2.0 controller";
	case 0x24dd8086:
		return "Intel 82801EB/R (ICH5) USB 2.0 controller";
	case 0x265c8086:
		return "Intel 82801FB (ICH6) USB 2.0 controller";
	case 0x27cc8086:
		return "Intel 82801GB/R (ICH7) USB 2.0 controller";

	case 0x28368086:
		return "Intel 82801H (ICH8) USB 2.0 controller USB2-A";
	case 0x283a8086:
		return "Intel 82801H (ICH8) USB 2.0 controller USB2-B";
	case 0x293a8086:
		return "Intel 82801I (ICH9) USB 2.0 controller";
	case 0x293c8086:
		return "Intel 82801I (ICH9) USB 2.0 controller";

	case 0x00e01033:
		return ("NEC uPD 720100 USB 2.0 controller");

	case 0x006810de:
		return "NVIDIA nForce2 USB 2.0 controller";
	case 0x008810de:
		return "NVIDIA nForce2 Ultra 400 USB 2.0 controller";
	case 0x00d810de:
		return "NVIDIA nForce3 USB 2.0 controller";
	case 0x00e810de:
		return "NVIDIA nForce3 250 USB 2.0 controller";
	case 0x005b10de:
		return "NVIDIA nForce4 USB 2.0 controller";

	case 0x15621131:
		return "Philips ISP156x USB 2.0 controller";

	case 0x31041106:
		return ("VIA VT6202 USB 2.0 controller");

	default:
		break;
	}

	if ((pci_get_class(self) == PCIC_SERIALBUS)
	    && (pci_get_subclass(self) == PCIS_SERIALBUS_USB)
	    && (pci_get_progif(self) == PCI_INTERFACE_EHCI)) {
		return ("EHCI (generic) USB 2.0 controller");
	}
	return (NULL);			/* dunno */
}

static int
ehci_pci_probe(device_t self)
{
	const char *desc = ehci_pci_match(self);

	if (desc) {
		device_set_desc(self, desc);
		return (0);
	} else {
		return (ENXIO);
	}
}

static int
ehci_pci_attach(device_t self)
{
	ehci_softc_t *sc = device_get_softc(self);
	int err;
	int rid;

	if (sc == NULL) {
		device_printf(self, "Could not allocate sc\n");
		return (ENXIO);
	}
	/* get all DMA memory */

	if (usb2_bus_mem_alloc_all(&sc->sc_bus,
	    USB_GET_DMA_TAG(self), &ehci_iterate_hw_softc)) {
		return ENOMEM;
	}
	sc->sc_dev = self;

	pci_enable_busmaster(self);

	switch (pci_read_config(self, PCI_USBREV, 1) & PCI_USB_REV_MASK) {
	case PCI_USB_REV_PRE_1_0:
	case PCI_USB_REV_1_0:
	case PCI_USB_REV_1_1:
		/*
		 * NOTE: some EHCI USB controllers have the wrong USB
		 * revision number. It appears those controllers are
		 * fully compliant so we just ignore this value in
		 * some common cases.
		 */
		device_printf(self, "pre-2.0 USB revision (ignored)\n");
		/* fallthrough */
	case PCI_USB_REV_2_0:
		sc->sc_bus.usbrev = USB_REV_2_0;
		break;
	default:
		sc->sc_bus.usbrev = USB_REV_UNKNOWN;
		break;
	}

	rid = PCI_CBMEM;
	sc->sc_io_res = bus_alloc_resource_any(self, SYS_RES_MEMORY, &rid,
	    RF_ACTIVE);
	if (!sc->sc_io_res) {
		device_printf(self, "Could not map memory\n");
		goto error;
	}
	sc->sc_io_tag = rman_get_bustag(sc->sc_io_res);
	sc->sc_io_hdl = rman_get_bushandle(sc->sc_io_res);
	sc->sc_io_size = rman_get_size(sc->sc_io_res);

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
	 * ehci_pci_match will never return NULL if ehci_pci_probe
	 * succeeded
	 */
	device_set_desc(sc->sc_bus.bdev, ehci_pci_match(self));
	switch (pci_get_vendor(self)) {
	case PCI_EHCI_VENDORID_ACERLABS:
		sprintf(sc->sc_vendor, "AcerLabs");
		break;
	case PCI_EHCI_VENDORID_AMD:
		sprintf(sc->sc_vendor, "AMD");
		break;
	case PCI_EHCI_VENDORID_APPLE:
		sprintf(sc->sc_vendor, "Apple");
		break;
	case PCI_EHCI_VENDORID_ATI:
		sprintf(sc->sc_vendor, "ATI");
		break;
	case PCI_EHCI_VENDORID_CMDTECH:
		sprintf(sc->sc_vendor, "CMDTECH");
		break;
	case PCI_EHCI_VENDORID_INTEL:
		sprintf(sc->sc_vendor, "Intel");
		break;
	case PCI_EHCI_VENDORID_NEC:
		sprintf(sc->sc_vendor, "NEC");
		break;
	case PCI_EHCI_VENDORID_OPTI:
		sprintf(sc->sc_vendor, "OPTi");
		break;
	case PCI_EHCI_VENDORID_PHILIPS:
		sprintf(sc->sc_vendor, "Philips");
		break;
	case PCI_EHCI_VENDORID_SIS:
		sprintf(sc->sc_vendor, "SiS");
		break;
	case PCI_EHCI_VENDORID_NVIDIA:
	case PCI_EHCI_VENDORID_NVIDIA2:
		sprintf(sc->sc_vendor, "nVidia");
		break;
	case PCI_EHCI_VENDORID_VIA:
		sprintf(sc->sc_vendor, "VIA");
		break;
	default:
		if (bootverbose)
			device_printf(self, "(New EHCI DeviceId=0x%08x)\n",
			    pci_get_devid(self));
		sprintf(sc->sc_vendor, "(0x%04x)", pci_get_vendor(self));
	}

	err = usb2_config_td_setup(&sc->sc_config_td, sc, &sc->sc_bus.bus_mtx,
	    NULL, 0, 4);
	if (err) {
		device_printf(self, "could not setup config thread!\n");
		goto error;
	}
#if (__FreeBSD_version >= 700031)
	err = bus_setup_intr(self, sc->sc_irq_res, INTR_TYPE_BIO | INTR_MPSAFE,
	    NULL, (void *)(void *)ehci_interrupt, sc, &sc->sc_intr_hdl);
#else
	err = bus_setup_intr(self, sc->sc_irq_res, INTR_TYPE_BIO | INTR_MPSAFE,
	    (void *)(void *)ehci_interrupt, sc, &sc->sc_intr_hdl);
#endif
	if (err) {
		device_printf(self, "Could not setup irq, %d\n", err);
		sc->sc_intr_hdl = NULL;
		goto error;
	}
	ehci_pci_takecontroller(self);
	err = ehci_init(sc);
	if (!err) {
		err = device_probe_and_attach(sc->sc_bus.bdev);
	}
	if (err) {
		device_printf(self, "USB init failed err=%d\n", err);
		goto error;
	}
	return (0);

error:
	ehci_pci_detach(self);
	return (ENXIO);
}

static int
ehci_pci_detach(device_t self)
{
	ehci_softc_t *sc = device_get_softc(self);
	device_t bdev;

	usb2_config_td_drain(&sc->sc_config_td);

	if (sc->sc_bus.bdev) {
		bdev = sc->sc_bus.bdev;
		device_detach(bdev);
		device_delete_child(self, bdev);
	}
	/* during module unload there are lots of children leftover */
	device_delete_all_children(self);

	pci_disable_busmaster(self);

	/*
	 * disable interrupts that might have been switched on in ehci_init
	 */
	if (sc->sc_io_res) {
		EWRITE4(sc, EHCI_USBINTR, 0);
	}
	if (sc->sc_irq_res && sc->sc_intr_hdl) {
		/*
		 * only call ehci_detach() after ehci_init()
		 */
		ehci_detach(sc);

		int err = bus_teardown_intr(self, sc->sc_irq_res, sc->sc_intr_hdl);

		if (err)
			/* XXX or should we panic? */
			device_printf(self, "Could not tear down irq, %d\n",
			    err);
		sc->sc_intr_hdl = NULL;
	}
	if (sc->sc_irq_res) {
		bus_release_resource(self, SYS_RES_IRQ, 0, sc->sc_irq_res);
		sc->sc_irq_res = NULL;
	}
	if (sc->sc_io_res) {
		bus_release_resource(self, SYS_RES_MEMORY, PCI_CBMEM,
		    sc->sc_io_res);
		sc->sc_io_res = NULL;
	}
	usb2_config_td_unsetup(&sc->sc_config_td);

	usb2_bus_mem_free_all(&sc->sc_bus, &ehci_iterate_hw_softc);

	return (0);
}

static void
ehci_pci_takecontroller(device_t self)
{
	ehci_softc_t *sc = device_get_softc(self);
	uint32_t cparams;
	uint32_t eec;
	uint16_t to;
	uint8_t eecp;
	uint8_t bios_sem;

	cparams = EREAD4(sc, EHCI_HCCPARAMS);

	/* Synchronise with the BIOS if it owns the controller. */
	for (eecp = EHCI_HCC_EECP(cparams); eecp != 0;
	    eecp = EHCI_EECP_NEXT(eec)) {
		eec = pci_read_config(self, eecp, 4);
		if (EHCI_EECP_ID(eec) != EHCI_EC_LEGSUP) {
			continue;
		}
		bios_sem = pci_read_config(self, eecp +
		    EHCI_LEGSUP_BIOS_SEM, 1);
		if (bios_sem == 0) {
			continue;
		}
		device_printf(sc->sc_bus.bdev, "waiting for BIOS "
		    "to give up control\n");
		pci_write_config(self, eecp +
		    EHCI_LEGSUP_OS_SEM, 1, 1);
		to = 500;
		while (1) {
			bios_sem = pci_read_config(self, eecp +
			    EHCI_LEGSUP_BIOS_SEM, 1);
			if (bios_sem == 0)
				break;

			if (--to == 0) {
				device_printf(sc->sc_bus.bdev,
				    "timed out waiting for BIOS\n");
				break;
			}
			usb2_pause_mtx(NULL, 10);	/* wait 10ms */
		}
	}
	return;
}

static driver_t ehci_driver =
{
	.name = "ehci",
	.methods = (device_method_t[]){
		/* device interface */
		DEVMETHOD(device_probe, ehci_pci_probe),
		DEVMETHOD(device_attach, ehci_pci_attach),
		DEVMETHOD(device_detach, ehci_pci_detach),
		DEVMETHOD(device_suspend, ehci_pci_suspend),
		DEVMETHOD(device_resume, ehci_pci_resume),
		DEVMETHOD(device_shutdown, ehci_pci_shutdown),
		/* bus interface */
		DEVMETHOD(bus_print_child, bus_generic_print_child),

		{0, 0}
	},
	.size = sizeof(struct ehci_softc),
};

static devclass_t ehci_devclass;

DRIVER_MODULE(ehci, pci, ehci_driver, ehci_devclass, 0, 0);
DRIVER_MODULE(ehci, cardbus, ehci_driver, ehci_devclass, 0, 0);
MODULE_DEPEND(ehci, usb2_controller, 1, 1, 1);
MODULE_DEPEND(ehci, usb2_core, 1, 1, 1);
