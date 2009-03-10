/*-
 * Copyright (c) 1998 - 2008 S�ren Schmidt <sos@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification, immediately at the beginning of the file.
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
__FBSDID("$FreeBSD: src/sys/dev/ata/chipsets/ata-netcell.c,v 1.3 2009/02/19 00:32:55 mav Exp $");

#include "opt_ata.h"
#include <sys/param.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/ata.h>
#include <sys/bus.h>
#include <sys/endian.h>
#include <sys/malloc.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/sema.h>
#include <sys/taskqueue.h>
#include <vm/uma.h>
#include <machine/stdarg.h>
#include <machine/resource.h>
#include <machine/bus.h>
#include <sys/rman.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/ata/ata-all.h>
#include <dev/ata/ata-pci.h>
#include <ata_if.h>

/* local prototypes */
static int ata_netcell_chipinit(device_t dev);
static int ata_netcell_ch_attach(device_t dev);
static void ata_netcell_setmode(device_t dev, int mode);


/*
 * NetCell chipset support functions
 */
static int
ata_netcell_probe(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);

    if (pci_get_devid(dev) == ATA_NETCELL_SR) {
	device_set_desc(dev, "Netcell SyncRAID SR3000/5000 RAID Controller");
	ctlr->chipinit = ata_netcell_chipinit;
	return 0;
    }
    return ENXIO;
}

static int
ata_netcell_chipinit(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);

    if (ata_setup_interrupt(dev, ata_generic_intr))
        return ENXIO;

    ctlr->ch_attach = ata_netcell_ch_attach;
    ctlr->ch_detach = ata_pci_ch_detach;
    ctlr->setmode = ata_netcell_setmode;
    return 0;
}

static int
ata_netcell_ch_attach(device_t dev)
{
    struct ata_channel *ch = device_get_softc(dev);
 
    /* setup the usual register normal pci style */
    if (ata_pci_ch_attach(dev))
	return ENXIO;
 
    /* the NetCell only supports 16 bit PIO transfers */
    ch->flags |= ATA_USE_16BIT;

    return 0;
}

static void
ata_netcell_setmode(device_t dev, int mode)
{
    struct ata_device *atadev = device_get_softc(dev);

    mode = ata_limit_mode(dev, mode, ATA_UDMA2);
    mode = ata_check_80pin(dev, mode);
    if (!ata_controlcmd(dev, ATA_SETFEATURES, ATA_SF_SETXFER, 0, mode))
        atadev->mode = mode;
}

ATA_DECLARE_DRIVER(ata_netcell);
