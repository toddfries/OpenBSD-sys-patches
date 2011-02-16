<<<<<<< HEAD
/*	$OpenBSD: ips.c,v 1.15 2006/11/29 14:52:19 grange Exp $	*/

/*
 * Copyright (c) 2006 Alexander Yurchenko <grange@openbsd.org>
=======
/*	$OpenBSD: ips.c,v 1.104 2010/10/12 00:53:32 krw Exp $	*/

/*
 * Copyright (c) 2006, 2007, 2009 Alexander Yurchenko <grange@openbsd.org>
>>>>>>> origin/master
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

/*
<<<<<<< HEAD
 * IBM ServeRAID controller driver.
=======
 * IBM (Adaptec) ServeRAID controllers driver.
>>>>>>> origin/master
 */

#include "bio.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
<<<<<<< HEAD
#include <sys/endian.h>
=======
#include <sys/ioctl.h>
>>>>>>> origin/master
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/sensors.h>
#include <sys/timeout.h>
#include <sys/queue.h>

#include <machine/bus.h>

#include <scsi/scsi_all.h>
#include <scsi/scsi_disk.h>
#include <scsi/scsiconf.h>

#include <dev/biovar.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

<<<<<<< HEAD
#define IPS_DEBUG	/* XXX: remove when the driver becomes stable */

=======
>>>>>>> origin/master
/* Debug levels */
#define IPS_D_ERR	0x0001
#define IPS_D_INFO	0x0002
#define IPS_D_XFER	0x0004
#define IPS_D_INTR	0x0008

#ifdef IPS_DEBUG
#define DPRINTF(a, b)	if (ips_debug & (a)) printf b
int ips_debug = IPS_D_ERR;
#else
#define DPRINTF(a, b)
#endif

/*
 * Register definitions.
 */
#define IPS_BAR0		0x10	/* I/O space base address */
#define IPS_BAR1		0x14	/* I/O space base address */

#define IPS_MORPHEUS_OISR	0x0030	/* outbound IRQ status */
#define	IPS_MORPHEUS_OISR_CMD		(1 << 3)
#define IPS_MORPHEUS_OIMR	0x0034	/* outbound IRQ mask */
#define IPS_MORPHEUS_IQPR	0x0040	/* inbound queue port */
#define IPS_MORPHEUS_OQPR	0x0044	/* outbound queue port */
#define IPS_MORPHEUS_OQPR_ID(x)		(((x) >> 8) & 0xff)
#define IPS_MORPHEUS_OQPR_ST(x)		(((x) >> 16) & 0xff)
#define IPS_MORPHEUS_OQPR_GSC(x)	(((x) >> 16) & 0x0f)
#define IPS_MORPHEUS_OQPR_EST(x)	(((x) >> 24) & 0xff)
#define IPS_MORPHEUS_GSC_NOERR		0x0
#define IPS_MORPHEUS_GSC_RECOV		0x1

/* Commands */
#define IPS_CMD_READ		0x02
#define IPS_CMD_WRITE		0x03
#define IPS_CMD_ADAPTERINFO	0x05
#define IPS_CMD_FLUSHCACHE	0x0a
#define IPS_CMD_READ_SG		0x82
#define IPS_CMD_WRITE_SG	0x83
#define IPS_CMD_DRIVEINFO	0x19

#define IPS_MAXCMDSZ		256	/* XXX: for now */
#define IPS_MAXDATASZ		64 * 1024
#define IPS_MAXSEGS		32

#define IPS_MAXDRIVES		8
#define IPS_MAXCHANS		4
<<<<<<< HEAD
#define IPS_MAXTARGETS		15
#define IPS_MAXCMDS		32

#define IPS_MAXFER		(64 * 1024)
#define IPS_MAXSGS		32

/* Command frames */
struct ips_cmd_adapterinfo {
	u_int8_t	command;
	u_int8_t	id;
	u_int8_t	reserve1;
	u_int8_t	commandtype;
	u_int32_t	reserve2;
	u_int32_t	buffaddr;
	u_int32_t	reserve3;
} __packed;

struct ips_cmd_driveinfo {
	u_int8_t	command;
=======
#define IPS_MAXTARGETS		16
#define IPS_MAXCHUNKS		16
#define IPS_MAXCMDS		128

#define IPS_MAXFER		(64 * 1024)
#define IPS_MAXSGS		16
#define IPS_MAXCDB		12

#define IPS_SECSZ		512
#define IPS_NVRAMPGSZ		128
#define IPS_SQSZ		(IPS_MAXCMDS * sizeof(u_int32_t))

#define	IPS_TIMEOUT		60000	/* ms */

/* Command codes */
#define IPS_CMD_READ		0x02
#define IPS_CMD_WRITE		0x03
#define IPS_CMD_DCDB		0x04
#define IPS_CMD_GETADAPTERINFO	0x05
#define IPS_CMD_FLUSH		0x0a
#define IPS_CMD_REBUILDSTATUS	0x0c
#define IPS_CMD_SETSTATE	0x10
#define IPS_CMD_REBUILD		0x16
#define IPS_CMD_ERRORTABLE	0x17
#define IPS_CMD_GETDRIVEINFO	0x19
#define IPS_CMD_RESETCHAN	0x1a
#define IPS_CMD_DOWNLOAD	0x20
#define IPS_CMD_RWBIOSFW	0x22
#define IPS_CMD_READCONF	0x38
#define IPS_CMD_GETSUBSYS	0x40
#define IPS_CMD_CONFIGSYNC	0x58
#define IPS_CMD_READ_SG		0x82
#define IPS_CMD_WRITE_SG	0x83
#define IPS_CMD_DCDB_SG		0x84
#define IPS_CMD_EDCDB		0x95
#define IPS_CMD_EDCDB_SG	0x96
#define IPS_CMD_RWNVRAMPAGE	0xbc
#define IPS_CMD_GETVERINFO	0xc6
#define IPS_CMD_FFDC		0xd7
#define IPS_CMD_SG		0x80
#define IPS_CMD_RWNVRAM		0xbc

/* DCDB attributes */
#define IPS_DCDB_DATAIN		0x01	/* data input */
#define IPS_DCDB_DATAOUT	0x02	/* data output */
#define IPS_DCDB_XFER64K	0x08	/* 64K transfer */
#define IPS_DCDB_TIMO10		0x10	/* 10 secs timeout */
#define IPS_DCDB_TIMO60		0x20	/* 60 secs timeout */
#define IPS_DCDB_TIMO20M	0x30	/* 20 mins timeout */
#define IPS_DCDB_NOAUTOREQSEN	0x40	/* no auto request sense */
#define IPS_DCDB_DISCON		0x80	/* disconnect allowed */

/* Register definitions */
#define IPS_REG_HIS		0x08	/* host interrupt status */
#define IPS_REG_HIS_SCE			0x01	/* status channel enqueue */
#define IPS_REG_HIS_EN			0x80	/* enable interrupts */
#define IPS_REG_CCSA		0x10	/* command channel system address */
#define IPS_REG_CCC		0x14	/* command channel control */
#define IPS_REG_CCC_SEM			0x0008	/* semaphore */
#define IPS_REG_CCC_START		0x101a	/* start command */
#define IPS_REG_SQH		0x20	/* status queue head */
#define IPS_REG_SQT		0x24	/* status queue tail */
#define IPS_REG_SQE		0x28	/* status queue end */
#define IPS_REG_SQS		0x2c	/* status queue start */

#define IPS_REG_OIS		0x30	/* outbound interrupt status */
#define IPS_REG_OIS_PEND		0x0008	/* interrupt is pending */
#define IPS_REG_OIM		0x34	/* outbound interrupt mask */
#define IPS_REG_OIM_DS			0x0008	/* disable interrupts */
#define IPS_REG_IQP		0x40	/* inbound queue port */
#define IPS_REG_OQP		0x44	/* outbound queue port */

/* Status word fields */
#define IPS_STAT_ID(x)		(((x) >> 8) & 0xff)	/* command id */
#define IPS_STAT_BASIC(x)	(((x) >> 16) & 0xff)	/* basic status */
#define IPS_STAT_EXT(x)		(((x) >> 24) & 0xff)	/* ext status */
#define IPS_STAT_GSC(x)		((x) & 0x0f)

/* Basic status codes */
#define IPS_STAT_OK		0x00	/* success */
#define IPS_STAT_RECOV		0x01	/* recovered error */
#define IPS_STAT_INVOP		0x03	/* invalid opcode */
#define IPS_STAT_INVCMD		0x04	/* invalid command block */
#define IPS_STAT_INVPARM	0x05	/* invalid parameters block */
#define IPS_STAT_BUSY		0x08	/* busy */
#define IPS_STAT_CMPLERR	0x0c	/* completed with error */
#define IPS_STAT_LDERR		0x0d	/* logical drive error */
#define IPS_STAT_TIMO		0x0e	/* timeout */
#define IPS_STAT_PDRVERR	0x0f	/* physical drive error */

/* Extended status codes */
#define IPS_ESTAT_SELTIMO	0xf0	/* select timeout */
#define IPS_ESTAT_OURUN		0xf2	/* over/underrun */
#define IPS_ESTAT_HOSTRST	0xf7	/* host reset */
#define IPS_ESTAT_DEVRST	0xf8	/* device reset */
#define IPS_ESTAT_RECOV		0xfc	/* recovered error */
#define IPS_ESTAT_CKCOND	0xff	/* check condition */

#define IPS_IOSIZE		128	/* max space size to map */

/* Command frame */
struct ips_cmd {
	u_int8_t	code;
>>>>>>> origin/master
	u_int8_t	id;
	u_int8_t	drivenum;
	u_int8_t	reserve1;
	u_int32_t	reserve2;
	u_int32_t	buffaddr;
	u_int32_t	reserve3;
} __packed;

struct ips_cmd_generic {
	u_int8_t	command;
	u_int8_t	id;
	u_int8_t	drivenum;
	u_int8_t	reserve2;
	u_int32_t	lba;
	u_int32_t	buffaddr;
	u_int32_t	reserve3;
} __packed;

<<<<<<< HEAD
struct ips_cmd_io {
	u_int8_t	command;
	u_int8_t	id;
	u_int8_t	drivenum;
	u_int8_t	segnum;
	u_int32_t	lba;
	u_int32_t	buffaddr;
	u_int16_t	length;
	u_int16_t	reserve1;
} __packed;
=======
/* Direct CDB (SCSI pass-through) frame */
struct ips_dcdb {
	u_int8_t	device;
	u_int8_t	attr;
	u_int16_t	datalen;
	u_int32_t	sgaddr;
	u_int8_t	cdblen;
	u_int8_t	senselen;
	u_int8_t	sgcnt;
	u_int8_t	__reserved1;
	u_int8_t	cdb[IPS_MAXCDB];
	u_int8_t	sense[64];
	u_int8_t	status;
	u_int8_t	__reserved2[3];
};

/* Scatter-gather array element */
struct ips_sg {
	u_int32_t	addr;
	u_int32_t	size;
};
>>>>>>> origin/master

/* Command block */
struct ips_cmdb {
	struct ips_cmd	cmd;
	struct ips_dcdb	dcdb;
	struct ips_sg	sg[IPS_MAXSGS];
};

/* Data frames */
struct ips_adapterinfo {
<<<<<<< HEAD
	u_int8_t	drivecount;
	u_int8_t	miscflags;
	u_int8_t	SLTflags;
	u_int8_t	BSTflags;
	u_int8_t	pwr_chg_count;
	u_int8_t	wrong_addr_count;
	u_int8_t	unident_count;
	u_int8_t	nvram_dev_chg_count;
	u_int8_t	codeblock_version[8];
	u_int8_t	bootblock_version[8];
	u_int32_t	drive_sector_count[IPS_MAXDRIVES];
	u_int8_t	max_concurrent_cmds;
	u_int8_t	max_phys_devices;
	u_int16_t	flash_prog_count;
	u_int8_t	defunct_disks;
	u_int8_t	rebuildflags;
	u_int8_t	offline_drivecount;
	u_int8_t	critical_drivecount;
	u_int16_t	config_update_count;
	u_int8_t	blockedflags;
	u_int8_t	psdn_error;
	u_int16_t	addr_dead_disk[IPS_MAXCHANS][IPS_MAXTARGETS];
} __packed;

struct ips_drive {
	u_int8_t	drivenum;
	u_int8_t	merge_id;
	u_int8_t	raid_lvl;
	u_int8_t	state;
	u_int32_t	sector_count;
} __packed;

struct ips_driveinfo {
	u_int8_t	drivecount;
	u_int8_t	reserve1;
	u_int16_t	reserve2;
	struct ips_drive drives[IPS_MAXDRIVES];
} __packed;

/* I/O access helper macros */
#define IPS_READ_4(s, r) \
	letoh32(bus_space_read_4((s)->sc_iot, (s)->sc_ioh, (r)))
#define IPS_WRITE_4(s, r, v) \
	bus_space_write_4((s)->sc_iot, (s)->sc_ioh, (r), htole32((v)))

struct ccb {
	int			c_id;
	int			c_flags;
#define CCB_F_RUN	0x0001

	bus_dmamap_t		c_dmam;
	struct scsi_xfer *	c_xfer;

	TAILQ_ENTRY(ccb)	c_link;
};

TAILQ_HEAD(ccbq, ccb);
=======
	u_int8_t	drivecnt;
	u_int8_t	miscflag;
	u_int8_t	sltflag;
	u_int8_t	bstflag;
	u_int8_t	pwrchgcnt;
	u_int8_t	wrongaddrcnt;
	u_int8_t	unidentcnt;
	u_int8_t	nvramdevchgcnt;
	u_int8_t	firmware[8];
	u_int8_t	bios[8];
	u_int32_t	drivesize[IPS_MAXDRIVES];
	u_int8_t	cmdcnt;
	u_int8_t	maxphysdevs;
	u_int16_t	flashrepgmcnt;
	u_int8_t	defunctdiskcnt;
	u_int8_t	rebuildflag;
	u_int8_t	offdrivecnt;
	u_int8_t	critdrivecnt;
	u_int16_t	confupdcnt;
	u_int8_t	blkflag;
	u_int8_t	__reserved;
	u_int16_t	deaddisk[IPS_MAXCHANS][IPS_MAXTARGETS];
};

struct ips_driveinfo {
	u_int8_t	drivecnt;
	u_int8_t	__reserved[3];
	struct ips_drive {
		u_int8_t	id;
		u_int8_t	__reserved;
		u_int8_t	raid;
		u_int8_t	state;
#define IPS_DS_FREE	0x00
#define IPS_DS_OFFLINE	0x02
#define IPS_DS_ONLINE	0x03
#define IPS_DS_DEGRADED	0x04
#define IPS_DS_SYS	0x06
#define IPS_DS_CRS	0x24

		u_int32_t	seccnt;
	}		drive[IPS_MAXDRIVES];
};

struct ips_conf {
	u_int8_t	ldcnt;
	u_int8_t	day;
	u_int8_t	month;
	u_int8_t	year;
	u_int8_t	initid[4];
	u_int8_t	hostid[12];
	u_int8_t	time[8];
	u_int32_t	useropt;
	u_int16_t	userfield;
	u_int8_t	rebuildrate;
	u_int8_t	__reserved1;

	struct ips_hw {
		u_int8_t	board[8];
		u_int8_t	cpu[8];
		u_int8_t	nchantype;
		u_int8_t	nhostinttype;
		u_int8_t	compression;
		u_int8_t	nvramtype;
		u_int32_t	nvramsize;
	}		hw;

	struct ips_ld {
		u_int16_t	userfield;
		u_int8_t	state;
		u_int8_t	raidcacheparam;
		u_int8_t	chunkcnt;
		u_int8_t	stripesize;
		u_int8_t	params;
		u_int8_t	__reserved;
		u_int32_t	size;

		struct ips_chunk {
			u_int8_t	channel;
			u_int8_t	target;
			u_int16_t	__reserved;
			u_int32_t	startsec;
			u_int32_t	seccnt;
		}		chunk[IPS_MAXCHUNKS];
	}		ld[IPS_MAXDRIVES];

	struct ips_dev {
		u_int8_t	initiator;
		u_int8_t	params;
		u_int8_t	miscflag;
		u_int8_t	state;
#define IPS_DVS_STANDBY	0x01
#define IPS_DVS_REBUILD	0x02
#define IPS_DVS_SPARE	0x04
#define IPS_DVS_MEMBER	0x08
#define IPS_DVS_ONLINE	0x80
#define IPS_DVS_READY	(IPS_DVS_STANDBY | IPS_DVS_ONLINE)

		u_int32_t	seccnt;
		u_int8_t	devid[28];
	}		dev[IPS_MAXCHANS][IPS_MAXTARGETS];

	u_int8_t	reserved[512];
};

struct ips_rblstat {
	u_int8_t	__unknown[20];
	struct {
		u_int8_t	__unknown[4];
		u_int32_t	total;
		u_int32_t	remain;
	}		ld[IPS_MAXDRIVES];
};

struct ips_pg5 {
	u_int32_t	signature;
	u_int8_t	__reserved1;
	u_int8_t	slot;
	u_int16_t	type;
	u_int8_t	bioshi[4];
	u_int8_t	bioslo[4];
	u_int16_t	__reserved2;
	u_int8_t	__reserved3;
	u_int8_t	os;
	u_int8_t	driverhi[4];
	u_int8_t	driverlo[4];
	u_int8_t	__reserved4[100];
};

struct ips_info {
	struct ips_adapterinfo	adapter;
	struct ips_driveinfo	drive;
	struct ips_conf		conf;
	struct ips_rblstat	rblstat;
	struct ips_pg5		pg5;
};

/* Command control block */
struct ips_softc;
struct ips_ccb {
	struct ips_softc *	c_sc;		/* driver softc */
	int			c_id;		/* command id */
	int			c_flags;	/* SCSI_* flags */
	enum {
		IPS_CCB_FREE,
		IPS_CCB_QUEUED,
		IPS_CCB_DONE
	}			c_state;	/* command state */

	void *			c_cmdbva;	/* command block virt addr */
	paddr_t			c_cmdbpa;	/* command block phys addr */
	bus_dmamap_t		c_dmam;		/* data buffer DMA map */

	struct scsi_xfer *	c_xfer;		/* corresponding SCSI xfer */

	u_int8_t		c_stat;		/* status byte copy */
	u_int8_t		c_estat;	/* ext status byte copy */
	int			c_error;	/* completion error */

	void			(*c_done)(struct ips_softc *,	/* cmd done */
				    struct ips_ccb *);		/* callback */

	SLIST_ENTRY(ips_ccb)	c_link;		/* queue link */
};

/* CCB queue */
SLIST_HEAD(ips_ccbq, ips_ccb);
>>>>>>> origin/master

struct dmamem {
	bus_dma_tag_t		dm_tag;
	bus_dmamap_t		dm_map;
	bus_dma_segment_t	dm_seg;
	bus_size_t		dm_size;
	void *			dm_kva;
};

struct ips_softc {
	struct device		sc_dev;

	struct scsi_link	sc_scsi_link;
<<<<<<< HEAD
	struct scsibus_softc *	sc_scsi_bus;

	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_tag;
=======
	struct scsibus_softc *	sc_scsibus;

	struct ips_pt {
		struct ips_softc *	pt_sc;
		int			pt_chan;

		struct scsi_link	pt_link;

		int			pt_proctgt;
		char			pt_procdev[16];
	}			sc_pt[IPS_MAXCHANS];

	struct ksensordev	sc_sensordev;
	struct ksensor *	sc_sensors;
>>>>>>> origin/master

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_dma_tag_t		sc_dmat;

	struct dmamem *		sc_cmdm;

<<<<<<< HEAD
	struct ccb *		sc_ccb;
	struct ccbq		sc_ccbq;

	void *			sc_ih;

	void			(*sc_exec)(struct ips_softc *);
	void			(*sc_inten)(struct ips_softc *);
	int			(*sc_intr)(void *);

	struct ips_adapterinfo	sc_ai;
	struct ips_driveinfo	sc_di;
=======
	struct ips_info *	sc_info;
	struct dmamem		sc_infom;

	int			sc_nunits;

	struct dmamem		sc_cmdbm;

	struct ips_ccb *	sc_ccb;
	int			sc_nccbs;
	struct ips_ccbq		sc_ccbq_free;

	struct dmamem		sc_sqm;
	paddr_t			sc_sqtail;
	u_int32_t *		sc_sqbuf;
	int			sc_sqidx;
>>>>>>> origin/master
};

int	ips_match(struct device *, void *, void *);
void	ips_attach(struct device *, struct device *, void *);

<<<<<<< HEAD
int	ips_scsi_cmd(struct scsi_xfer *);
int	ips_scsi_io(struct scsi_xfer *);
int	ips_scsi_ioctl(struct scsi_link *, u_long, caddr_t, int,
	    struct proc *);
void	ips_scsi_minphys(struct buf *);

void	ips_xfer_timeout(void *);

void	ips_flushcache(struct ips_softc *);
int	ips_getadapterinfo(struct ips_softc *, struct ips_adapterinfo *);
int	ips_getdriveinfo(struct ips_softc *, struct ips_driveinfo *);

void	ips_copperhead_exec(struct ips_softc *);
void	ips_copperhead_inten(struct ips_softc *);
int	ips_copperhead_intr(void *);

void	ips_morpheus_exec(struct ips_softc *);
void	ips_morpheus_inten(struct ips_softc *);
int	ips_morpheus_intr(void *);

struct ccb *	ips_ccb_alloc(bus_dma_tag_t, int);
void		ips_ccb_free(struct ccb *, bus_dma_tag_t, int);

struct dmamem *	ips_dmamem_alloc(bus_dma_tag_t, bus_size_t);
void		ips_dmamem_free(struct dmamem *);
=======
void	ips_scsi_cmd(struct scsi_xfer *);
void	ips_scsi_pt_cmd(struct scsi_xfer *);
int	ips_scsi_ioctl(struct scsi_link *, u_long, caddr_t, int);

#if NBIO > 0
int	ips_ioctl(struct device *, u_long, caddr_t);
int	ips_ioctl_inq(struct ips_softc *, struct bioc_inq *);
int	ips_ioctl_vol(struct ips_softc *, struct bioc_vol *);
int	ips_ioctl_disk(struct ips_softc *, struct bioc_disk *);
int	ips_ioctl_setstate(struct ips_softc *, struct bioc_setstate *);
#endif

#ifndef SMALL_KERNEL
void	ips_sensors(void *);
#endif

int	ips_load_xs(struct ips_softc *, struct ips_ccb *, struct scsi_xfer *);
void	ips_start_xs(struct ips_softc *, struct ips_ccb *, struct scsi_xfer *);

int	ips_cmd(struct ips_softc *, struct ips_ccb *);
int	ips_poll(struct ips_softc *, struct ips_ccb *);
void	ips_done(struct ips_softc *, struct ips_ccb *);
void	ips_done_xs(struct ips_softc *, struct ips_ccb *);
void	ips_done_pt(struct ips_softc *, struct ips_ccb *);
void	ips_done_mgmt(struct ips_softc *, struct ips_ccb *);
int	ips_error(struct ips_softc *, struct ips_ccb *);
int	ips_error_xs(struct ips_softc *, struct ips_ccb *);
int	ips_intr(void *);
void	ips_timeout(void *);

int	ips_getadapterinfo(struct ips_softc *, int);
int	ips_getdriveinfo(struct ips_softc *, int);
int	ips_getconf(struct ips_softc *, int);
int	ips_getpg5(struct ips_softc *, int);

#if NBIO > 0
int	ips_getrblstat(struct ips_softc *, int);
int	ips_setstate(struct ips_softc *, int, int, int, int);
int	ips_rebuild(struct ips_softc *, int, int, int, int, int);
#endif

void	ips_copperhead_exec(struct ips_softc *, struct ips_ccb *);
void	ips_copperhead_intren(struct ips_softc *);
int	ips_copperhead_isintr(struct ips_softc *);
u_int32_t ips_copperhead_status(struct ips_softc *);

void	ips_morpheus_exec(struct ips_softc *, struct ips_ccb *);
void	ips_morpheus_intren(struct ips_softc *);
int	ips_morpheus_isintr(struct ips_softc *);
u_int32_t ips_morpheus_status(struct ips_softc *);

struct ips_ccb *ips_ccb_alloc(struct ips_softc *, int);
void	ips_ccb_free(struct ips_softc *, struct ips_ccb *, int);
struct ips_ccb *ips_ccb_get(struct ips_softc *);
void	ips_ccb_put(struct ips_softc *, struct ips_ccb *);

int	ips_dmamem_alloc(struct dmamem *, bus_dma_tag_t, bus_size_t);
void	ips_dmamem_free(struct dmamem *);
>>>>>>> origin/master

struct cfattach ips_ca = {
	sizeof(struct ips_softc),
	ips_match,
	ips_attach
};

struct cfdriver ips_cd = {
	NULL, "ips", DV_DULL
};

static const struct pci_matchid ips_ids[] = {
	{ PCI_VENDOR_IBM,	PCI_PRODUCT_IBM_SERVERAID },
	{ PCI_VENDOR_IBM,	PCI_PRODUCT_IBM_SERVERAID2 },
	{ PCI_VENDOR_ADP2,	PCI_PRODUCT_ADP2_SERVERAID }
};

static struct scsi_adapter ips_scsi_adapter = {
	ips_scsi_cmd,
<<<<<<< HEAD
	ips_scsi_minphys,
=======
	scsi_minphys,
>>>>>>> origin/master
	NULL,
	NULL,
	ips_scsi_ioctl
};

static struct scsi_adapter ips_scsi_pt_adapter = {
	ips_scsi_pt_cmd,
	scsi_minphys,
	NULL,
	NULL,
	NULL
};

<<<<<<< HEAD
=======
static const struct pci_matchid ips_ids[] = {
	{ PCI_VENDOR_IBM,	PCI_PRODUCT_IBM_SERVERAID },
	{ PCI_VENDOR_IBM,	PCI_PRODUCT_IBM_SERVERAID2 },
	{ PCI_VENDOR_ADP2,	PCI_PRODUCT_ADP2_SERVERAID }
};

static const struct ips_chipset {
	enum {
		IPS_CHIP_COPPERHEAD = 0,
		IPS_CHIP_MORPHEUS
	}		ic_id;

	int		ic_bar;

	void		(*ic_exec)(struct ips_softc *, struct ips_ccb *);
	void		(*ic_intren)(struct ips_softc *);
	int		(*ic_isintr)(struct ips_softc *);
	u_int32_t	(*ic_status)(struct ips_softc *);
} ips_chips[] = {
	{
		IPS_CHIP_COPPERHEAD,
		0x14,
		ips_copperhead_exec,
		ips_copperhead_intren,
		ips_copperhead_isintr,
		ips_copperhead_status
	},
	{
		IPS_CHIP_MORPHEUS,
		0x10,
		ips_morpheus_exec,
		ips_morpheus_intren,
		ips_morpheus_isintr,
		ips_morpheus_status
	}
};

#define ips_exec(s, c)	(s)->sc_chip->ic_exec((s), (c))
#define ips_intren(s)	(s)->sc_chip->ic_intren((s))
#define ips_isintr(s)	(s)->sc_chip->ic_isintr((s))
#define ips_status(s)	(s)->sc_chip->ic_status((s))

static const char *ips_names[] = {
	NULL,
	NULL,
	"II",
	"onboard",
	"onboard",
	"3H",
	"3L",
	"4H",
	"4M",
	"4L",
	"4Mx",
	"4Lx",
	"5i",
	"5i",
	"6M",
	"6i",
	"7t",
	"7k",
	"7M"
};

>>>>>>> origin/master
int
ips_match(struct device *parent, void *match, void *aux)
{
	return (pci_matchbyid(aux, ips_ids,
	    sizeof(ips_ids) / sizeof(ips_ids[0])));
}

void
ips_attach(struct device *parent, struct device *self, void *aux)
{
	struct ips_softc *sc = (struct ips_softc *)self;
	struct pci_attach_args *pa = aux;
	struct scsibus_attach_args saa;
<<<<<<< HEAD
	int bar;
=======
	struct ips_adapterinfo *ai;
	struct ips_driveinfo *di;
	struct ips_pg5 *pg5;
>>>>>>> origin/master
	pcireg_t maptype;
	bus_size_t iosize;
	pci_intr_handle_t ih;
	const char *intrstr;
<<<<<<< HEAD
	int i, maxcmds;
=======
	int type, i;
>>>>>>> origin/master

	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;
	sc->sc_dmat = pa->pa_dmat;

<<<<<<< HEAD
	/* Identify the chipset */
	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_IBM_SERVERAID:
		printf(": Copperhead");
		sc->sc_exec = ips_copperhead_exec;
		sc->sc_inten = ips_copperhead_inten;
		sc->sc_intr = ips_copperhead_intr;
		break;
	case PCI_PRODUCT_IBM_SERVERAID2:
	case PCI_PRODUCT_ADP2_SERVERAID:
		printf(": Morpheus");
		sc->sc_exec = ips_morpheus_exec;
		sc->sc_inten = ips_morpheus_inten;
		sc->sc_intr = ips_morpheus_intr;
		break;
	}

	/* Map I/O space */
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_IBM_SERVERAID)
		bar = IPS_BAR1;
	else
		bar = IPS_BAR0;
	maptype = pci_mapreg_type(sc->sc_pc, sc->sc_tag, bar);
	if (pci_mapreg_map(pa, bar, maptype, 0, &sc->sc_iot, &sc->sc_ioh,
	    NULL, &iosize, 0)) {
		printf(": can't map I/O space\n");
		return;
	}

	/* Allocate command DMA buffer */
	if ((sc->sc_cmdm = ips_dmamem_alloc(sc->sc_dmat,
	    IPS_MAXCMDSZ)) == NULL) {
		printf(": can't alloc command DMA buffer\n");
		goto fail1;
	}

	/* Get adapter info */
	if (ips_getadapterinfo(sc, &sc->sc_ai)) {
		printf(": can't get adapter info\n");
		goto fail2;
	}

	/* Get logical drives info */
	if (ips_getdriveinfo(sc, &sc->sc_di)) {
		printf(": can't get drives info\n");
		goto fail2;
	}

	/* Allocate command queue */
	maxcmds = sc->sc_ai.max_concurrent_cmds;
	if ((sc->sc_ccb = ips_ccb_alloc(sc->sc_dmat, maxcmds)) == NULL) {
		printf(": can't alloc command queue\n");
		goto fail2;
	}

	TAILQ_INIT(&sc->sc_ccbq);
	for (i = 0; i < maxcmds; i++)
		TAILQ_INSERT_TAIL(&sc->sc_ccbq, &sc->sc_ccb[i], c_link);
=======
	/* Identify chipset */
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_IBM_SERVERAID)
		sc->sc_chip = &ips_chips[IPS_CHIP_COPPERHEAD];
	else
		sc->sc_chip = &ips_chips[IPS_CHIP_MORPHEUS];

	/* Map registers */
	maptype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, sc->sc_chip->ic_bar);
	if (pci_mapreg_map(pa, sc->sc_chip->ic_bar, maptype, 0, &sc->sc_iot,
	    &sc->sc_ioh, NULL, &iosize, IPS_IOSIZE)) {
		printf(": can't map regs\n");
		return;
	}

	/* Allocate command buffer */
	if (ips_dmamem_alloc(&sc->sc_cmdbm, sc->sc_dmat,
	    IPS_MAXCMDS * sizeof(struct ips_cmdb))) {
		printf(": can't alloc cmd buffer\n");
		goto fail1;
	}

	/* Allocate info buffer */
	if (ips_dmamem_alloc(&sc->sc_infom, sc->sc_dmat,
	    sizeof(struct ips_info))) {
		printf(": can't alloc info buffer\n");
		goto fail2;
	}
	sc->sc_info = sc->sc_infom.dm_vaddr;
	ai = &sc->sc_info->adapter;
	di = &sc->sc_info->drive;
	pg5 = &sc->sc_info->pg5;

	/* Allocate status queue for the Copperhead chipset */
	if (sc->sc_chip->ic_id == IPS_CHIP_COPPERHEAD) {
		if (ips_dmamem_alloc(&sc->sc_sqm, sc->sc_dmat, IPS_SQSZ)) {
			printf(": can't alloc status queue\n");
			goto fail3;
		}
		sc->sc_sqtail = sc->sc_sqm.dm_paddr;
		sc->sc_sqbuf = sc->sc_sqm.dm_vaddr;
		sc->sc_sqidx = 0;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, IPS_REG_SQS,
		    sc->sc_sqm.dm_paddr);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, IPS_REG_SQE,
		    sc->sc_sqm.dm_paddr + IPS_SQSZ);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, IPS_REG_SQH,
		    sc->sc_sqm.dm_paddr + sizeof(u_int32_t));
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, IPS_REG_SQT,
		    sc->sc_sqm.dm_paddr);
	}

	/* Bootstrap CCB queue */
	sc->sc_nccbs = 1;
	sc->sc_ccb = &ccb0;
	bzero(&ccb0, sizeof(ccb0));
	ccb0.c_cmdbva = sc->sc_cmdbm.dm_vaddr;
	ccb0.c_cmdbpa = sc->sc_cmdbm.dm_paddr;
	SLIST_INIT(&sc->sc_ccbq_free);
	SLIST_INSERT_HEAD(&sc->sc_ccbq_free, &ccb0, c_link);

	/* Get adapter info */
	if (ips_getadapterinfo(sc, SCSI_NOSLEEP)) {
		printf(": can't get adapter info\n");
		goto fail4;
	}

	/* Get logical drives info */
	if (ips_getdriveinfo(sc, SCSI_NOSLEEP)) {
		printf(": can't get ld info\n");
		goto fail4;
	}
	sc->sc_nunits = di->drivecnt;

	/* Get configuration */
	if (ips_getconf(sc, SCSI_NOSLEEP)) {
		printf(": can't get config\n");
		goto fail4;
	}

	/* Read NVRAM page 5 for additional info */
	(void)ips_getpg5(sc, SCSI_NOSLEEP);

	/* Initialize CCB queue */
	sc->sc_nccbs = ai->cmdcnt;
	if ((sc->sc_ccb = ips_ccb_alloc(sc, sc->sc_nccbs)) == NULL) {
		printf(": can't alloc ccb queue\n");
		goto fail4;
	}
	SLIST_INIT(&sc->sc_ccbq_free);
	for (i = 0; i < sc->sc_nccbs; i++)
		SLIST_INSERT_HEAD(&sc->sc_ccbq_free,
		    &sc->sc_ccb[i], c_link);
>>>>>>> origin/master

	/* Install interrupt handler */
	if (pci_intr_map(pa, &ih)) {
		printf(": can't map interrupt\n");
		goto fail5;
	}
	intrstr = pci_intr_string(sc->sc_pc, ih);
	if ((sc->sc_ih = pci_intr_establish(sc->sc_pc, ih, IPL_BIO,
	    sc->sc_intr, sc, sc->sc_dev.dv_xname)) == NULL) {
		printf(": can't establish interrupt");
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		goto fail5;
	}
<<<<<<< HEAD
	printf(", %s\n", intrstr);

	/* Enable interrupts */
	(*sc->sc_inten)(sc);

	/* Attach SCSI bus */
	sc->sc_scsi_link.openings = 1; /* XXX: for now */
	sc->sc_scsi_link.adapter_target = IPS_MAXTARGETS;
	sc->sc_scsi_link.adapter_buswidth = IPS_MAXTARGETS;
	sc->sc_scsi_link.device = &ips_scsi_device;
=======
	printf(": %s\n", intrstr);

	/* Display adapter info */
	printf("%s: ServeRAID", sc->sc_dev.dv_xname);
	type = letoh16(pg5->type);
	if (type < sizeof(ips_names) / sizeof(ips_names[0]) && ips_names[type])
		printf(" %s", ips_names[type]);
	printf(", FW %c%c%c%c%c%c%c", ai->firmware[0], ai->firmware[1],
	    ai->firmware[2], ai->firmware[3], ai->firmware[4], ai->firmware[5],
	    ai->firmware[6]);
	printf(", BIOS %c%c%c%c%c%c%c", ai->bios[0], ai->bios[1], ai->bios[2],
	    ai->bios[3], ai->bios[4], ai->bios[5], ai->bios[6]);
	printf(", %d cmds, %d LD%s", sc->sc_nccbs, sc->sc_nunits,
	    (sc->sc_nunits == 1 ? "" : "s"));
	printf("\n");

	/* Attach SCSI bus */
	if (sc->sc_nunits > 0)
		sc->sc_scsi_link.openings = sc->sc_nccbs / sc->sc_nunits;
	sc->sc_scsi_link.adapter_target = sc->sc_nunits;
	sc->sc_scsi_link.adapter_buswidth = sc->sc_nunits;
>>>>>>> origin/master
	sc->sc_scsi_link.adapter = &ips_scsi_adapter;
	sc->sc_scsi_link.adapter_softc = sc;

	bzero(&saa, sizeof(saa));
	saa.saa_sc_link = &sc->sc_scsi_link;
<<<<<<< HEAD
=======
	sc->sc_scsibus = (struct scsibus_softc *)config_found(self, &saa,
	    scsiprint);

	/* For each channel attach SCSI pass-through bus */
	bzero(&saa, sizeof(saa));
	for (i = 0; i < IPS_MAXCHANS; i++) {
		struct ips_pt *pt;
		struct scsi_link *link;
		int target, lastarget;

		pt = &sc->sc_pt[i];
		pt->pt_sc = sc;
		pt->pt_chan = i;
		pt->pt_proctgt = -1;

		/* Check if channel has any devices besides disks */
		for (target = 0, lastarget = -1; target < IPS_MAXTARGETS;
		    target++) {
			struct ips_dev *idev;
			int type;

			idev = &sc->sc_info->conf.dev[i][target];
			type = idev->params & SID_TYPE;
			if (idev->state && type != T_DIRECT) {
				lastarget = target;
				if (type == T_PROCESSOR ||
				    type == T_ENCLOSURE)
					/* remember enclosure address */
					pt->pt_proctgt = target;
			}
		}
		if (lastarget == -1)
			continue;

		link = &pt->pt_link;
		link->openings = 1;
		link->adapter_target = IPS_MAXTARGETS;
		link->adapter_buswidth = lastarget + 1;
		link->adapter = &ips_scsi_pt_adapter;
		link->adapter_softc = pt;

		saa.saa_sc_link = link;
		config_found(self, &saa, scsiprint);
	}
>>>>>>> origin/master

	sc->sc_scsi_bus = (struct scsibus_softc *)config_found(self, &saa,
	    scsiprint);

#if NBIO > 0
	/* Install ioctl handler */
	if (bio_register(&sc->sc_dev, ips_ioctl))
		printf("%s: no ioctl support\n", sc->sc_dev.dv_xname);
#endif

#ifndef SMALL_KERNEL
	/* Add sensors */
	if ((sc->sc_sensors = malloc(sizeof(struct ksensor) * sc->sc_nunits,
	    M_DEVBUF, M_NOWAIT | M_ZERO)) == NULL) {
		printf(": can't alloc sensors\n");
		return;
	}
	strlcpy(sc->sc_sensordev.xname, sc->sc_dev.dv_xname,
	    sizeof(sc->sc_sensordev.xname));
	for (i = 0; i < sc->sc_nunits; i++) {
		struct device *dev;

		sc->sc_sensors[i].type = SENSOR_DRIVE;
		sc->sc_sensors[i].status = SENSOR_S_UNKNOWN;
		dev = scsi_get_link(sc->sc_scsibus, i, 0)->device_softc;
		strlcpy(sc->sc_sensors[i].desc, dev->dv_xname,
		    sizeof(sc->sc_sensors[i].desc));
		sensor_attach(&sc->sc_sensordev, &sc->sc_sensors[i]);
	}
	if (sensor_task_register(sc, ips_sensors, 10) == NULL) {
		printf(": no sensors support\n");
		free(sc->sc_sensors, M_DEVBUF);
		return;
	}
	sensordev_install(&sc->sc_sensordev);
#endif	/* !SMALL_KERNEL */

	return;
<<<<<<< HEAD
fail3:
	ips_ccb_free(sc->sc_ccb, sc->sc_dmat, maxcmds);
fail2:
	ips_dmamem_free(sc->sc_cmdm);
=======
fail5:
	ips_ccb_free(sc, sc->sc_ccb, sc->sc_nccbs);
fail4:
	if (sc->sc_chip->ic_id == IPS_CHIP_COPPERHEAD)
		ips_dmamem_free(&sc->sc_sqm);
fail3:
	ips_dmamem_free(&sc->sc_infom);
fail2:
	ips_dmamem_free(&sc->sc_cmdbm);
>>>>>>> origin/master
fail1:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, iosize);
}

void
ips_scsi_cmd(struct scsi_xfer *xs)
{
	struct scsi_link *link = xs->sc_link;
	struct ips_softc *sc = link->adapter_softc;
<<<<<<< HEAD
	struct scsi_inquiry_data *inq;
	struct scsi_read_cap_data *cap;
	struct scsi_sense_data *sns;
	int target = link->target;
	int s;

	if (target >= sc->sc_di.drivecount || link->lun != 0)
		goto error;
=======
	struct ips_driveinfo *di = &sc->sc_info->drive;
	struct ips_drive *drive;
	struct scsi_inquiry_data inq;
	struct scsi_read_cap_data rcd;
	struct scsi_sense_data sd;
	struct scsi_rw *rw;
	struct scsi_rw_big *rwb;
	struct ips_ccb *ccb;
	struct ips_cmd *cmd;
	int target = link->target;
	u_int32_t blkno, blkcnt;
	int code, s;

	DPRINTF(IPS_D_XFER, ("%s: ips_scsi_cmd: xs %p, target %d, "
	    "opcode 0x%02x, flags 0x%x\n", sc->sc_dev.dv_xname, xs, target,
	    xs->cmd->opcode, xs->flags));

	if (target >= sc->sc_nunits || link->lun != 0) {
		DPRINTF(IPS_D_INFO, ("%s: ips_scsi_cmd: invalid params "
		    "target %d, lun %d\n", sc->sc_dev.dv_xname,
		    target, link->lun));
		xs->error = XS_DRIVER_STUFFUP;
		scsi_done(xs);
		return;
	}

	drive = &di->drive[target];
	xs->error = XS_NOERROR;
>>>>>>> origin/master

	switch (xs->cmd->opcode) {
	case READ_BIG:
	case READ_COMMAND:
	case WRITE_BIG:
	case WRITE_COMMAND:
<<<<<<< HEAD
		return (ips_scsi_io(xs));
	case INQUIRY:
		inq = (void *)xs->data;
		bzero(inq, sizeof(*inq));
		inq->device = T_DIRECT;
		inq->version = 2;
		inq->response_format = 2;
		inq->additional_length = 32;
		strlcpy(inq->vendor, "IBM", sizeof(inq->vendor));
		snprintf(inq->product, sizeof(inq->product),
		    "ServeRAID LD %02d", target);
		goto done;
	case READ_CAPACITY:
		cap = (void *)xs->data;
		bzero(cap, sizeof(*cap));
		_lto4b(sc->sc_di.drives[target].sector_count - 1, cap->addr);
		_lto4b(512, cap->length);
		goto done;
	case REQUEST_SENSE:
		sns = (void *)xs->data;
		bzero(sns, sizeof(*sns));
		sns->error_code = 0x70;
		sns->flags = SKEY_NO_SENSE;
		goto done;
	case SYNCHRONIZE_CACHE:
		ips_flushcache(sc);
		goto done;
=======
		if (xs->cmdlen == sizeof(struct scsi_rw)) {
			rw = (void *)xs->cmd;
			blkno = _3btol(rw->addr) &
			    (SRW_TOPADDR << 16 | 0xffff);
			blkcnt = rw->length ? rw->length : 0x100;
		} else {
			rwb = (void *)xs->cmd;
			blkno = _4btol(rwb->addr);
			blkcnt = _2btol(rwb->length);
		}

		if (blkno >= letoh32(drive->seccnt) || blkno + blkcnt >
		    letoh32(drive->seccnt)) {
			DPRINTF(IPS_D_ERR, ("%s: ips_scsi_cmd: invalid params "
			    "blkno %u, blkcnt %u\n", sc->sc_dev.dv_xname,
			    blkno, blkcnt));
			xs->error = XS_DRIVER_STUFFUP;
			break;
		}

		if (xs->flags & SCSI_DATA_IN)
			code = IPS_CMD_READ;
		else
			code = IPS_CMD_WRITE;

		s = splbio();
		ccb = ips_ccb_get(sc);
		splx(s);
		if (ccb == NULL) {
			DPRINTF(IPS_D_ERR, ("%s: ips_scsi_cmd: no ccb\n",
			    sc->sc_dev.dv_xname));
			xs->error = XS_NO_CCB;
			scsi_done(xs);
			return;
		}

		cmd = ccb->c_cmdbva;
		cmd->code = code;
		cmd->drive = target;
		cmd->lba = htole32(blkno);
		cmd->seccnt = htole16(blkcnt);

		if (ips_load_xs(sc, ccb, xs)) {
			DPRINTF(IPS_D_ERR, ("%s: ips_scsi_cmd: ips_load_xs "
			    "failed\n", sc->sc_dev.dv_xname));

			s = splbio();
			ips_ccb_put(sc, ccb);
			splx(s);
			xs->error = XS_DRIVER_STUFFUP;
			break;
		}

		if (cmd->sgcnt > 0)
			cmd->code |= IPS_CMD_SG;

		ccb->c_done = ips_done_xs;
		ips_start_xs(sc, ccb, xs);
		return;
	case INQUIRY:
		bzero(&inq, sizeof(inq));
		inq.device = T_DIRECT;
		inq.version = 2;
		inq.response_format = 2;
		inq.additional_length = 32;
		inq.flags |= SID_CmdQue;
		strlcpy(inq.vendor, "IBM", sizeof(inq.vendor));
		snprintf(inq.product, sizeof(inq.product),
		    "LD%d RAID%d", target, drive->raid);
		strlcpy(inq.revision, "1.0", sizeof(inq.revision));
		memcpy(xs->data, &inq, MIN(xs->datalen, sizeof(inq)));
		break;
	case READ_CAPACITY:
		bzero(&rcd, sizeof(rcd));
		_lto4b(letoh32(drive->seccnt) - 1, rcd.addr);
		_lto4b(IPS_SECSZ, rcd.length);
		memcpy(xs->data, &rcd, MIN(xs->datalen, sizeof(rcd)));
		break;
	case REQUEST_SENSE:
		bzero(&sd, sizeof(sd));
		sd.error_code = SSD_ERRCODE_CURRENT;
		sd.flags = SKEY_NO_SENSE;
		memcpy(xs->data, &sd, MIN(xs->datalen, sizeof(sd)));
		break;
	case SYNCHRONIZE_CACHE:
		s = splbio();
		ccb = ips_ccb_get(sc);
		splx(s);
		if (ccb == NULL) {
			DPRINTF(IPS_D_ERR, ("%s: ips_scsi_cmd: no ccb\n",
			    sc->sc_dev.dv_xname));
			xs->error = XS_NO_CCB;
			scsi_done(xs);
			return;
		}

		cmd = ccb->c_cmdbva;
		cmd->code = IPS_CMD_FLUSH;

		ccb->c_done = ips_done_xs;
		ips_start_xs(sc, ccb, xs);
		return;
>>>>>>> origin/master
	case PREVENT_ALLOW:
	case START_STOP:
	case TEST_UNIT_READY:
		return (COMPLETE);
	}

<<<<<<< HEAD
error:
	xs->error = XS_DRIVER_STUFFUP;
done:
	s = splbio();
=======
>>>>>>> origin/master
	scsi_done(xs);
}

void
ips_scsi_pt_cmd(struct scsi_xfer *xs)
{
	struct scsi_link *link = xs->sc_link;
	struct ips_pt *pt = link->adapter_softc;
	struct ips_softc *sc = pt->pt_sc;
	struct device *dev = link->device_softc;
	struct ips_ccb *ccb;
	struct ips_cmdb *cmdb;
	struct ips_cmd *cmd;
	struct ips_dcdb *dcdb;
	int chan = pt->pt_chan, target = link->target;
	int s;

	DPRINTF(IPS_D_XFER, ("%s: ips_scsi_pt_cmd: xs %p, chan %d, target %d, "
	    "opcode 0x%02x, flags 0x%x\n", sc->sc_dev.dv_xname, xs, chan,
	    target, xs->cmd->opcode, xs->flags));

	if (pt->pt_procdev[0] == '\0' && target == pt->pt_proctgt && dev)
		strlcpy(pt->pt_procdev, dev->dv_xname, sizeof(pt->pt_procdev));

	if (xs->cmdlen > IPS_MAXCDB) {
		DPRINTF(IPS_D_ERR, ("%s: cmdlen %d too big\n",
		    sc->sc_dev.dv_xname, xs->cmdlen));

		bzero(&xs->sense, sizeof(xs->sense));
		xs->sense.error_code = SSD_ERRCODE_VALID | SSD_ERRCODE_CURRENT;
		xs->sense.flags = SKEY_ILLEGAL_REQUEST;
		xs->sense.add_sense_code = 0x20; /* illcmd, 0x24 illfield */
		xs->error = XS_SENSE;
		scsi_done(xs);
		return;
	}

	xs->error = XS_NOERROR;

	s = splbio();
	ccb = ips_ccb_get(sc);
	splx(s);
<<<<<<< HEAD
	return (COMPLETE);
}

int
ips_scsi_io(struct scsi_xfer *xs)
{
	struct scsi_link *link = xs->sc_link;
	struct ips_softc *sc = link->adapter_softc;
	struct scsi_rw *rw;
	struct scsi_rw_big *rwb;
	struct ccb *ccb;
	struct ips_cmd_io *cmd;
	u_int32_t blkno, blkcnt;
	int i, s;

	/* Pick up the first free ccb */
	s = splbio();
	ccb = TAILQ_FIRST(&sc->sc_ccbq);
	if (ccb != NULL)
		TAILQ_REMOVE(&sc->sc_ccbq, ccb, c_link);
	splx(s);
	if (ccb == NULL) {
		DPRINTF(IPS_D_ERR, ("%s: scsi_io, no free ccb\n",
		    sc->sc_dev.dv_xname));
		return (TRY_AGAIN_LATER);
	}
	DPRINTF(IPS_D_XFER, ("%s: scsi_io, ccb id %d\n", sc->sc_dev.dv_xname,
	    ccb->c_id));

	bus_dmamap_load(sc->sc_dmat, ccb->c_dmam, xs->data, xs->datalen, NULL,
	    BUS_DMA_NOWAIT);
	ccb->c_xfer = xs;

	if (xs->cmd->opcode == READ_COMMAND ||
	    xs->cmd->opcode == WRITE_COMMAND) {
		rw = (void *)xs->cmd;
		blkno = _3btol(rw->addr) & (SRW_TOPADDR << 16 | 0xffff);
		blkcnt = rw->length > 0 ? rw->length : 0x100;
	} else {
		rwb = (void *)xs->cmd;
		blkno = _4btol(rwb->addr);
		blkcnt = _2btol(rwb->length);
	}

	cmd = sc->sc_cmdm->dm_kva;
	bzero(cmd, sizeof(*cmd));
	cmd->command = (xs->flags & SCSI_DATA_IN) ? IPS_CMD_READ :
	    IPS_CMD_WRITE;
	cmd->id = ccb->c_id;
	cmd->drivenum = link->target;
	cmd->lba = blkno;
	cmd->length = blkcnt;
	if (ccb->c_dmam->dm_nsegs > 1) {
		cmd->command = (xs->flags & SCSI_DATA_IN) ? IPS_CMD_READ_SG :
		    IPS_CMD_WRITE_SG;
		cmd->segnum = ccb->c_dmam->dm_nsegs;

		for (i = 0; i < ccb->c_dmam->dm_nsegs; i++) {
			*(u_int32_t *)((u_int8_t *)sc->sc_cmdm->dm_kva + 24 +
			    i * 8) = ccb->c_dmam->dm_segs[i].ds_addr;
			*(u_int32_t *)((u_int8_t *)sc->sc_cmdm->dm_kva + 24 +
			    i * 8 + 4) = ccb->c_dmam->dm_segs[i].ds_len;
		}
		cmd->buffaddr = sc->sc_cmdm->dm_seg.ds_addr + 24;
	} else {
		cmd->buffaddr = ccb->c_dmam->dm_segs[0].ds_addr;
	}

	timeout_set(&xs->stimeout, ips_xfer_timeout, ccb);
	timeout_add(&xs->stimeout, (xs->timeout * 1000) / hz);

	s = splbio();
	(*sc->sc_exec)(sc);
	ccb->c_flags |= CCB_F_RUN;
	splx(s);

	return (SUCCESSFULLY_QUEUED);
}

int
ips_scsi_ioctl(struct scsi_link *link, u_long cmd, caddr_t addr, int flags,
    struct proc *p)
{
	return (ENOTTY);
}
=======
	if (ccb == NULL) {
		DPRINTF(IPS_D_ERR, ("%s: ips_scsi_pt_cmd: no ccb\n",
		    sc->sc_dev.dv_xname));
		xs->error = XS_NO_CCB;
		scsi_done(xs);
		return;
	}

	cmdb = ccb->c_cmdbva;
	cmd = &cmdb->cmd;
	dcdb = &cmdb->dcdb;

	cmd->code = IPS_CMD_DCDB;

	dcdb->device = (chan << 4) | target;
	if (xs->flags & SCSI_DATA_IN)
		dcdb->attr |= IPS_DCDB_DATAIN;
	if (xs->flags & SCSI_DATA_OUT)
		dcdb->attr |= IPS_DCDB_DATAOUT;

	/*
	 * Adjust timeout value to what controller supports. Make sure our
	 * timeout will be fired after controller gives up.
	 */
	if (xs->timeout <= 10000) {
		dcdb->attr |= IPS_DCDB_TIMO10;
		xs->timeout = 11000;
	} else if (xs->timeout <= 60000) {
		dcdb->attr |= IPS_DCDB_TIMO60;
		xs->timeout = 61000;
	} else {
		dcdb->attr |= IPS_DCDB_TIMO20M;
		xs->timeout = 20 * 60000 + 1000;
	}

	dcdb->attr |= IPS_DCDB_DISCON;
	dcdb->datalen = htole16(xs->datalen);
	dcdb->cdblen = xs->cmdlen;
	dcdb->senselen = MIN(sizeof(xs->sense), sizeof(dcdb->sense));
	memcpy(dcdb->cdb, xs->cmd, xs->cmdlen);

	if (ips_load_xs(sc, ccb, xs)) {
		DPRINTF(IPS_D_ERR, ("%s: ips_scsi_pt_cmd: ips_load_xs "
		    "failed\n", sc->sc_dev.dv_xname));

		s = splbio();
		ips_ccb_put(sc, ccb);
		splx(s);
		xs->error = XS_DRIVER_STUFFUP;
		scsi_done(xs);
		return;
	}
	if (cmd->sgcnt > 0)
		cmd->code |= IPS_CMD_SG;
	dcdb->sgaddr = cmd->sgaddr;
	dcdb->sgcnt = cmd->sgcnt;
	cmd->sgaddr = htole32(ccb->c_cmdbpa + offsetof(struct ips_cmdb, dcdb));
	cmd->sgcnt = 0;

	ccb->c_done = ips_done_pt;
	ips_start_xs(sc, ccb, xs);
}

int
ips_scsi_ioctl(struct scsi_link *link, u_long cmd, caddr_t addr, int flag)
{
#if NBIO > 0
	return (ips_ioctl(link->adapter_softc, cmd, addr));
#else
	return (ENOTTY);
#endif
}

#if NBIO > 0
int
ips_ioctl(struct device *dev, u_long cmd, caddr_t addr)
{
	struct ips_softc *sc = (struct ips_softc *)dev;

	DPRINTF(IPS_D_INFO, ("%s: ips_ioctl: cmd %lu\n",
	    sc->sc_dev.dv_xname, cmd));

	switch (cmd) {
	case BIOCINQ:
		return (ips_ioctl_inq(sc, (struct bioc_inq *)addr));
	case BIOCVOL:
		return (ips_ioctl_vol(sc, (struct bioc_vol *)addr));
	case BIOCDISK:
		return (ips_ioctl_disk(sc, (struct bioc_disk *)addr));
	case BIOCSETSTATE:
		return (ips_ioctl_setstate(sc, (struct bioc_setstate *)addr));
	default:
		return (ENOTTY);
	}
}

int
ips_ioctl_inq(struct ips_softc *sc, struct bioc_inq *bi)
{
	struct ips_conf *conf = &sc->sc_info->conf;
	int i;

	strlcpy(bi->bi_dev, sc->sc_dev.dv_xname, sizeof(bi->bi_dev));
	bi->bi_novol = sc->sc_nunits;
	for (i = 0, bi->bi_nodisk = 0; i < sc->sc_nunits; i++)
		bi->bi_nodisk += conf->ld[i].chunkcnt;

	DPRINTF(IPS_D_INFO, ("%s: ips_ioctl_inq: novol %d, nodisk %d\n",
	    bi->bi_dev, bi->bi_novol, bi->bi_nodisk));

	return (0);
}

int
ips_ioctl_vol(struct ips_softc *sc, struct bioc_vol *bv)
{
	struct ips_driveinfo *di = &sc->sc_info->drive;
	struct ips_conf *conf = &sc->sc_info->conf;
	struct ips_rblstat *rblstat = &sc->sc_info->rblstat;
	struct ips_ld *ld;
	int vid = bv->bv_volid;
	struct device *dv;
	int error, rebuild = 0;
	u_int32_t total = 0, done = 0;

	if (vid >= sc->sc_nunits)
		return (EINVAL);
	if ((error = ips_getconf(sc, 0)))
		return (error);
	ld = &conf->ld[vid];

	switch (ld->state) {
	case IPS_DS_ONLINE:
		bv->bv_status = BIOC_SVONLINE;
		break;
	case IPS_DS_DEGRADED:
		bv->bv_status = BIOC_SVDEGRADED;
		rebuild++;
		break;
	case IPS_DS_OFFLINE:
		bv->bv_status = BIOC_SVOFFLINE;
		break;
	default:
		bv->bv_status = BIOC_SVINVALID;
	}

	if (rebuild && ips_getrblstat(sc, 0) == 0) {
		total = letoh32(rblstat->ld[vid].total);
		done = total - letoh32(rblstat->ld[vid].remain);
		if (total && total > done) {
			bv->bv_status = BIOC_SVREBUILD;
			bv->bv_percent = 100 * done / total;
		}
	}

	bv->bv_size = (u_quad_t)letoh32(ld->size) * IPS_SECSZ;
	bv->bv_level = di->drive[vid].raid;
	bv->bv_nodisk = ld->chunkcnt;

	/* Associate all unused and spare drives with first volume */
	if (vid == 0) {
		struct ips_dev *dev;
		int chan, target;

		for (chan = 0; chan < IPS_MAXCHANS; chan++)
			for (target = 0; target < IPS_MAXTARGETS; target++) {
				dev = &conf->dev[chan][target];
				if (dev->state && !(dev->state &
				    IPS_DVS_MEMBER) &&
				    (dev->params & SID_TYPE) == T_DIRECT)
					bv->bv_nodisk++;
			}
	}

	dv = scsi_get_link(sc->sc_scsibus, vid, 0)->device_softc;
	strlcpy(bv->bv_dev, dv->dv_xname, sizeof(bv->bv_dev));
	strlcpy(bv->bv_vendor, "IBM", sizeof(bv->bv_vendor));

	DPRINTF(IPS_D_INFO, ("%s: ips_ioctl_vol: vid %d, state 0x%02x, "
	    "total %u, done %u, size %llu, level %d, nodisk %d, dev %s\n",
	    sc->sc_dev.dv_xname, vid, ld->state, total, done, bv->bv_size,
	    bv->bv_level, bv->bv_nodisk, bv->bv_dev));

	return (0);
}

int
ips_ioctl_disk(struct ips_softc *sc, struct bioc_disk *bd)
{
	struct ips_conf *conf = &sc->sc_info->conf;
	struct ips_ld *ld;
	struct ips_chunk *chunk;
	struct ips_dev *dev;
	int vid = bd->bd_volid, did = bd->bd_diskid;
	int chan, target, error, i;

	if (vid >= sc->sc_nunits)
		return (EINVAL);
	if ((error = ips_getconf(sc, 0)))
		return (error);
	ld = &conf->ld[vid];

	if (did >= ld->chunkcnt) {
		/* Probably unused or spare drives */
		if (vid != 0)
			return (EINVAL);

		i = ld->chunkcnt;
		for (chan = 0; chan < IPS_MAXCHANS; chan++)
			for (target = 0; target < IPS_MAXTARGETS; target++) {
				dev = &conf->dev[chan][target];
				if (dev->state && !(dev->state &
				    IPS_DVS_MEMBER) &&
				    (dev->params & SID_TYPE) == T_DIRECT)
					if (i++ == did)
						goto out;
			}
	} else {
		chunk = &ld->chunk[did];
		chan = chunk->channel;
		target = chunk->target;
	}

out:
	if (chan >= IPS_MAXCHANS || target >= IPS_MAXTARGETS)
		return (EINVAL);
	dev = &conf->dev[chan][target];

	bd->bd_channel = chan;
	bd->bd_target = target;
	bd->bd_lun = 0;
	bd->bd_size = (u_quad_t)letoh32(dev->seccnt) * IPS_SECSZ;

	bzero(bd->bd_vendor, sizeof(bd->bd_vendor));
	memcpy(bd->bd_vendor, dev->devid, MIN(sizeof(bd->bd_vendor),
	    sizeof(dev->devid)));
	strlcpy(bd->bd_procdev, sc->sc_pt[chan].pt_procdev,
	    sizeof(bd->bd_procdev));

	if (dev->state & IPS_DVS_READY) {
		bd->bd_status = BIOC_SDUNUSED;
		if (dev->state & IPS_DVS_MEMBER)
			bd->bd_status = BIOC_SDONLINE;
		if (dev->state & IPS_DVS_SPARE)
			bd->bd_status = BIOC_SDHOTSPARE;
		if (dev->state & IPS_DVS_REBUILD)
			bd->bd_status = BIOC_SDREBUILD;
	} else {
		bd->bd_status = BIOC_SDOFFLINE;
	}

	DPRINTF(IPS_D_INFO, ("%s: ips_ioctl_disk: vid %d, did %d, channel %d, "
	    "target %d, size %llu, state 0x%02x\n", sc->sc_dev.dv_xname,
	    vid, did, bd->bd_channel, bd->bd_target, bd->bd_size, dev->state));

	return (0);
}

int
ips_ioctl_setstate(struct ips_softc *sc, struct bioc_setstate *bs)
{
	struct ips_conf *conf = &sc->sc_info->conf;
	struct ips_dev *dev;
	int state, error;

	if (bs->bs_channel >= IPS_MAXCHANS || bs->bs_target >= IPS_MAXTARGETS)
		return (EINVAL);
	if ((error = ips_getconf(sc, 0)))
		return (error);
	dev = &conf->dev[bs->bs_channel][bs->bs_target];
	state = dev->state;

	switch (bs->bs_status) {
	case BIOC_SSONLINE:
		state |= IPS_DVS_READY;
		break;
	case BIOC_SSOFFLINE:
		state &= ~IPS_DVS_READY;
		break;
	case BIOC_SSHOTSPARE:
		state |= IPS_DVS_SPARE;
		break;
	case BIOC_SSREBUILD:
		return (ips_rebuild(sc, bs->bs_channel, bs->bs_target,
		    bs->bs_channel, bs->bs_target, 0));
	default:
		return (EINVAL);
	}

	return (ips_setstate(sc, bs->bs_channel, bs->bs_target, state, 0));
}
#endif	/* NBIO > 0 */

#ifndef SMALL_KERNEL
void
ips_sensors(void *arg)
{
	struct ips_softc *sc = arg;
	struct ips_conf *conf = &sc->sc_info->conf;
	struct ips_ld *ld;
	int i;

	/* ips_sensors() runs from work queue thus allowed to sleep */
	if (ips_getconf(sc, 0)) {
		DPRINTF(IPS_D_ERR, ("%s: ips_sensors: ips_getconf failed\n",
		    sc->sc_dev.dv_xname));

		for (i = 0; i < sc->sc_nunits; i++) {
			sc->sc_sensors[i].value = 0;
			sc->sc_sensors[i].status = SENSOR_S_UNKNOWN;
		}
		return;
	}

	DPRINTF(IPS_D_INFO, ("%s: ips_sensors:", sc->sc_dev.dv_xname));
	for (i = 0; i < sc->sc_nunits; i++) {
		ld = &conf->ld[i];
		DPRINTF(IPS_D_INFO, (" ld%d.state 0x%02x", i, ld->state));
		switch (ld->state) {
		case IPS_DS_ONLINE:
			sc->sc_sensors[i].value = SENSOR_DRIVE_ONLINE;
			sc->sc_sensors[i].status = SENSOR_S_OK;
			break;
		case IPS_DS_DEGRADED:
			sc->sc_sensors[i].value = SENSOR_DRIVE_PFAIL;
			sc->sc_sensors[i].status = SENSOR_S_WARN;
			break;
		case IPS_DS_OFFLINE:
			sc->sc_sensors[i].value = SENSOR_DRIVE_FAIL;
			sc->sc_sensors[i].status = SENSOR_S_CRIT;
			break;
		default:
			sc->sc_sensors[i].value = 0;
			sc->sc_sensors[i].status = SENSOR_S_UNKNOWN;
		}
	}
	DPRINTF(IPS_D_INFO, ("\n"));
}
#endif	/* !SMALL_KERNEL */

int
ips_load_xs(struct ips_softc *sc, struct ips_ccb *ccb, struct scsi_xfer *xs)
{
	struct ips_cmdb *cmdb = ccb->c_cmdbva;
	struct ips_cmd *cmd = &cmdb->cmd;
	struct ips_sg *sg = cmdb->sg;
	int nsegs, i;

	if (xs->datalen == 0)
		return (0);

	/* Map data buffer into DMA segments */
	if (bus_dmamap_load(sc->sc_dmat, ccb->c_dmam, xs->data, xs->datalen,
	    NULL, (xs->flags & SCSI_NOSLEEP ? BUS_DMA_NOWAIT : 0)))
		return (1);
	bus_dmamap_sync(sc->sc_dmat, ccb->c_dmam, 0,ccb->c_dmam->dm_mapsize,
	    xs->flags & SCSI_DATA_IN ? BUS_DMASYNC_PREREAD :
	    BUS_DMASYNC_PREWRITE);

	if ((nsegs = ccb->c_dmam->dm_nsegs) > IPS_MAXSGS)
		return (1);

	if (nsegs > 1) {
		cmd->sgcnt = nsegs;
		cmd->sgaddr = htole32(ccb->c_cmdbpa + offsetof(struct ips_cmdb,
		    sg));

		/* Fill in scatter-gather array */
		for (i = 0; i < nsegs; i++) {
			sg[i].addr = htole32(ccb->c_dmam->dm_segs[i].ds_addr);
			sg[i].size = htole32(ccb->c_dmam->dm_segs[i].ds_len);
		}
	} else {
		cmd->sgcnt = 0;
		cmd->sgaddr = htole32(ccb->c_dmam->dm_segs[0].ds_addr);
	}
>>>>>>> origin/master

void
ips_scsi_minphys(struct buf *bp)
{
	minphys(bp);
}

void
<<<<<<< HEAD
ips_xfer_timeout(void *arg)
{
	struct ccb *ccb = arg;
	struct scsi_xfer *xs = ccb->c_xfer;
	struct ips_softc *sc = xs->sc_link->adapter_softc;
	int s;

	DPRINTF(IPS_D_ERR, ("%s: xfer timeout, ccb id %d\n",
	    sc->sc_dev.dv_xname, ccb->c_id));

	bus_dmamap_unload(sc->sc_dmat, ccb->c_dmam);
	xs->error = XS_TIMEOUT;
	s = splbio();
	scsi_done(xs);
	ccb->c_flags &= ~CCB_F_RUN;
	TAILQ_INSERT_TAIL(&sc->sc_ccbq, ccb, c_link);
	splx(s);
=======
ips_start_xs(struct ips_softc *sc, struct ips_ccb *ccb, struct scsi_xfer *xs)
{
	ccb->c_flags = xs->flags;
	ccb->c_xfer = xs;
	int ispoll = xs->flags & SCSI_POLL;

	if (!ispoll) {
		timeout_set(&xs->stimeout, ips_timeout, ccb);
		timeout_add_msec(&xs->stimeout, xs->timeout);
	}

	/*
	 * Return value not used here because ips_cmd() must complete
	 * scsi_xfer on any failure and SCSI layer will handle possible
	 * errors.
	 */
	ips_cmd(sc, ccb);
}

int
ips_cmd(struct ips_softc *sc, struct ips_ccb *ccb)
{
	struct ips_cmd *cmd = ccb->c_cmdbva;
	int s, error = 0;

	DPRINTF(IPS_D_XFER, ("%s: ips_cmd: id 0x%02x, flags 0x%x, xs %p, "
	    "code 0x%02x, drive %d, sgcnt %d, lba %d, sgaddr 0x%08x, "
	    "seccnt %d\n", sc->sc_dev.dv_xname, ccb->c_id, ccb->c_flags,
	    ccb->c_xfer, cmd->code, cmd->drive, cmd->sgcnt, letoh32(cmd->lba),
	    letoh32(cmd->sgaddr), letoh16(cmd->seccnt)));

	cmd->id = ccb->c_id;

	/* Post command to controller and optionally wait for completion */
	s = splbio();
	ips_exec(sc, ccb);
	ccb->c_state = IPS_CCB_QUEUED;
	if (ccb->c_flags & SCSI_POLL)
		error = ips_poll(sc, ccb);
	splx(s);

	return (error);
}

int
ips_poll(struct ips_softc *sc, struct ips_ccb *ccb)
{
	struct timeval tv;
	int error, timo;

	splassert(IPL_BIO);

	if (ccb->c_flags & SCSI_NOSLEEP) {
		/* busy-wait */
		DPRINTF(IPS_D_XFER, ("%s: ips_poll: busy-wait\n",
		    sc->sc_dev.dv_xname));

		for (timo = 10000; timo > 0; timo--) {
			delay(100);
			ips_intr(sc);
			if (ccb->c_state == IPS_CCB_DONE)
				break;
		}
	} else {
		/* sleep */
		timo = ccb->c_xfer ? ccb->c_xfer->timeout : IPS_TIMEOUT;
		tv.tv_sec = timo / 1000;
		tv.tv_usec = (timo % 1000) * 1000;
		timo = tvtohz(&tv);

		DPRINTF(IPS_D_XFER, ("%s: ips_poll: sleep %d hz\n",
		    sc->sc_dev.dv_xname, timo));
		tsleep(ccb, PRIBIO + 1, "ipscmd", timo);
	}
	DPRINTF(IPS_D_XFER, ("%s: ips_poll: state %d\n", sc->sc_dev.dv_xname,
	    ccb->c_state));

	if (ccb->c_state != IPS_CCB_DONE)
		/*
		 * Command never completed. Fake hardware status byte
		 * to indicate timeout.
		 */
		ccb->c_stat = IPS_STAT_TIMO;

	ips_done(sc, ccb);
	error = ccb->c_error;
	ips_ccb_put(sc, ccb);

	return (error);
}

void
ips_done(struct ips_softc *sc, struct ips_ccb *ccb)
{
	splassert(IPL_BIO);

	DPRINTF(IPS_D_XFER, ("%s: ips_done: id 0x%02x, flags 0x%x, xs %p\n",
	    sc->sc_dev.dv_xname, ccb->c_id, ccb->c_flags, ccb->c_xfer));

	ccb->c_error = ips_error(sc, ccb);
	ccb->c_done(sc, ccb);
}

void
ips_done_xs(struct ips_softc *sc, struct ips_ccb *ccb)
{
	struct scsi_xfer *xs = ccb->c_xfer;

	if (!(xs->flags & SCSI_POLL))
		timeout_del(&xs->stimeout);

	if (xs->flags & (SCSI_DATA_IN | SCSI_DATA_OUT)) {
		bus_dmamap_sync(sc->sc_dmat, ccb->c_dmam, 0,
		    ccb->c_dmam->dm_mapsize, xs->flags & SCSI_DATA_IN ?
		    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, ccb->c_dmam);
	}

	xs->resid = 0;
	xs->error = ips_error_xs(sc, ccb);
	scsi_done(xs);
}

void
ips_done_pt(struct ips_softc *sc, struct ips_ccb *ccb)
{
	struct scsi_xfer *xs = ccb->c_xfer;
	struct ips_cmdb *cmdb = ccb->c_cmdbva;
	struct ips_dcdb *dcdb = &cmdb->dcdb;
	int done = letoh16(dcdb->datalen);

	if (!(xs->flags & SCSI_POLL))
		timeout_del(&xs->stimeout);

	if (xs->flags & (SCSI_DATA_IN | SCSI_DATA_OUT)) {
		bus_dmamap_sync(sc->sc_dmat, ccb->c_dmam, 0,
		    ccb->c_dmam->dm_mapsize, xs->flags & SCSI_DATA_IN ?
		    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, ccb->c_dmam);
	}

	if (done && done < xs->datalen)
		xs->resid = xs->datalen - done;
	else
		xs->resid = 0;
	xs->error = ips_error_xs(sc, ccb);
	xs->status = dcdb->status;

	if (xs->error == XS_SENSE)
		memcpy(&xs->sense, dcdb->sense, MIN(sizeof(xs->sense),
		    sizeof(dcdb->sense)));

	if (xs->cmd->opcode == INQUIRY && xs->error == XS_NOERROR) {
		int type = ((struct scsi_inquiry_data *)xs->data)->device &
		    SID_TYPE;

		if (type == T_DIRECT)
			/* mask physical drives */
			xs->error = XS_DRIVER_STUFFUP;
	}

	scsi_done(xs);
}

void
ips_done_mgmt(struct ips_softc *sc, struct ips_ccb *ccb)
{
	if (ccb->c_flags & (SCSI_DATA_IN | SCSI_DATA_OUT))
		bus_dmamap_sync(sc->sc_dmat, sc->sc_infom.dm_map, 0,
		    sc->sc_infom.dm_map->dm_mapsize,
		    ccb->c_flags & SCSI_DATA_IN ? BUS_DMASYNC_POSTREAD :
		    BUS_DMASYNC_POSTWRITE);
}

int
ips_error(struct ips_softc *sc, struct ips_ccb *ccb)
{
	struct ips_cmdb *cmdb = ccb->c_cmdbva;
	struct ips_cmd *cmd = &cmdb->cmd;
	struct ips_dcdb *dcdb = &cmdb->dcdb;
	struct scsi_xfer *xs = ccb->c_xfer;
	u_int8_t gsc = IPS_STAT_GSC(ccb->c_stat);

	if (gsc == IPS_STAT_OK)
		return (0);

	DPRINTF(IPS_D_ERR, ("%s: ips_error: stat 0x%02x, estat 0x%02x, "
	    "cmd code 0x%02x, drive %d, sgcnt %d, lba %u, seccnt %d",
	    sc->sc_dev.dv_xname, ccb->c_stat, ccb->c_estat, cmd->code,
	    cmd->drive, cmd->sgcnt, letoh32(cmd->lba), letoh16(cmd->seccnt)));
	if (cmd->code == IPS_CMD_DCDB || cmd->code == IPS_CMD_DCDB_SG) {
		int i;

		DPRINTF(IPS_D_ERR, (", dcdb device 0x%02x, attr 0x%02x, "
		    "datalen %d, sgcnt %d, status 0x%02x",
		    dcdb->device, dcdb->attr, letoh16(dcdb->datalen),
		    dcdb->sgcnt, dcdb->status));

		DPRINTF(IPS_D_ERR, (", cdb"));
		for (i = 0; i < dcdb->cdblen; i++)
			DPRINTF(IPS_D_ERR, (" %x", dcdb->cdb[i]));
		if (ccb->c_estat == IPS_ESTAT_CKCOND) {
			DPRINTF(IPS_D_ERR, (", sense"));
			for (i = 0; i < dcdb->senselen; i++)
				DPRINTF(IPS_D_ERR, (" %x", dcdb->sense[i]));
		}
	}		
	DPRINTF(IPS_D_ERR, ("\n"));

	switch (gsc) {
	case IPS_STAT_RECOV:
		return (0);
	case IPS_STAT_INVOP:
	case IPS_STAT_INVCMD:
	case IPS_STAT_INVPARM:
		return (EINVAL);
	case IPS_STAT_BUSY:
		return (EBUSY);
	case IPS_STAT_TIMO:
		return (ETIMEDOUT);
	case IPS_STAT_PDRVERR:
		switch (ccb->c_estat) {
		case IPS_ESTAT_SELTIMO:
			return (ENODEV);
		case IPS_ESTAT_OURUN:
			if (xs && letoh16(dcdb->datalen) < xs->datalen)
				/* underrun */
				return (0);
			break;
		case IPS_ESTAT_RECOV:
			return (0);
		}
		break;
	}

	return (EIO);
}

int
ips_error_xs(struct ips_softc *sc, struct ips_ccb *ccb)
{
	struct ips_cmdb *cmdb = ccb->c_cmdbva;
	struct ips_dcdb *dcdb = &cmdb->dcdb;
	struct scsi_xfer *xs = ccb->c_xfer;
	u_int8_t gsc = IPS_STAT_GSC(ccb->c_stat);

	/* Map hardware error codes to SCSI ones */
	switch (gsc) {
	case IPS_STAT_OK:
	case IPS_STAT_RECOV:
		return (XS_NOERROR);
	case IPS_STAT_BUSY:
		return (XS_BUSY);
	case IPS_STAT_TIMO:
		return (XS_TIMEOUT);
	case IPS_STAT_PDRVERR:
		switch (ccb->c_estat) {
		case IPS_ESTAT_SELTIMO:
			return (XS_SELTIMEOUT);
		case IPS_ESTAT_OURUN:
			if (xs && letoh16(dcdb->datalen) < xs->datalen)
				/* underrun */
				return (XS_NOERROR);
			break;
		case IPS_ESTAT_HOSTRST:
		case IPS_ESTAT_DEVRST:
			return (XS_RESET);
		case IPS_ESTAT_RECOV:
			return (XS_NOERROR);
		case IPS_ESTAT_CKCOND:
			return (XS_SENSE);
		}
		break;
	}

	return (XS_DRIVER_STUFFUP);
>>>>>>> origin/master
}

void
ips_flushcache(struct ips_softc *sc)
{
<<<<<<< HEAD
	struct ips_cmd_generic *cmd;

	cmd = sc->sc_cmdm->dm_kva;
	cmd->command = IPS_CMD_FLUSHCACHE;
=======
	struct ips_softc *sc = arg;
	struct ips_ccb *ccb;
	u_int32_t status;
	int id;

	DPRINTF(IPS_D_XFER, ("%s: ips_intr", sc->sc_dev.dv_xname));
	if (!ips_isintr(sc)) {
		DPRINTF(IPS_D_XFER, (": not ours\n"));
		return (0);
	}
	DPRINTF(IPS_D_XFER, ("\n"));

	/* Process completed commands */
	while ((status = ips_status(sc)) != 0xffffffff) {
		DPRINTF(IPS_D_XFER, ("%s: ips_intr: status 0x%08x\n",
		    sc->sc_dev.dv_xname, status));

		id = IPS_STAT_ID(status);
		if (id >= sc->sc_nccbs) {
			DPRINTF(IPS_D_ERR, ("%s: ips_intr: invalid id %d\n",
			    sc->sc_dev.dv_xname, id));
			continue;
		}

		ccb = &sc->sc_ccb[id];
		if (ccb->c_state != IPS_CCB_QUEUED) {
			DPRINTF(IPS_D_ERR, ("%s: ips_intr: cmd 0x%02x not "
			    "queued, state %d, status 0x%08x\n",
			    sc->sc_dev.dv_xname, ccb->c_id, ccb->c_state,
			    status));
			continue;
		}

		ccb->c_state = IPS_CCB_DONE;
		ccb->c_stat = IPS_STAT_BASIC(status);
		ccb->c_estat = IPS_STAT_EXT(status);

		if (ccb->c_flags & SCSI_POLL) {
			wakeup(ccb);
		} else {
			ips_done(sc, ccb);
			ips_ccb_put(sc, ccb);
		}
	}
>>>>>>> origin/master

	(*sc->sc_exec)(sc);
	DELAY(1000);
}

void
ips_timeout(void *arg)
{
	struct ips_ccb *ccb = arg;
	struct ips_softc *sc = ccb->c_sc;
	struct scsi_xfer *xs = ccb->c_xfer;
	int s;

	s = splbio();
	if (xs)
		sc_print_addr(xs->sc_link);
	else
		printf("%s: ", sc->sc_dev.dv_xname);
	printf("timeout\n");

	/*
	 * Command never completed. Fake hardware status byte
	 * to indicate timeout.
	 * XXX: need to remove command from controller.
	 */
	ccb->c_stat = IPS_STAT_TIMO;
	ips_done(sc, ccb);
	ips_ccb_put(sc, ccb);
	splx(s);
}

int
ips_getadapterinfo(struct ips_softc *sc, int flags)
{
	struct ips_ccb *ccb;
	struct ips_cmd *cmd;
	int s;

	s = splbio();
	ccb = ips_ccb_get(sc);
	splx(s);
	if (ccb == NULL)
		return (1);

	ccb->c_flags = SCSI_DATA_IN | SCSI_POLL | flags;
	ccb->c_done = ips_done_mgmt;

	cmd = ccb->c_cmdbva;
	cmd->code = IPS_CMD_GETADAPTERINFO;
	cmd->sgaddr = htole32(sc->sc_infom.dm_paddr + offsetof(struct ips_info,
	    adapter));

	return (ips_cmd(sc, ccb));
}

int
ips_getdriveinfo(struct ips_softc *sc, int flags)
{
	struct ips_ccb *ccb;
	struct ips_cmd *cmd;
	int s;

	s = splbio();
	ccb = ips_ccb_get(sc);
	splx(s);
	if (ccb == NULL)
		return (1);

	ccb->c_flags = SCSI_DATA_IN | SCSI_POLL | flags;
	ccb->c_done = ips_done_mgmt;

	cmd = ccb->c_cmdbva;
	cmd->code = IPS_CMD_GETDRIVEINFO;
	cmd->sgaddr = htole32(sc->sc_infom.dm_paddr + offsetof(struct ips_info,
	    drive));

	return (ips_cmd(sc, ccb));
}

int
ips_getconf(struct ips_softc *sc, int flags)
{
	struct ips_ccb *ccb;
	struct ips_cmd *cmd;
	int s;

	s = splbio();
	ccb = ips_ccb_get(sc);
	splx(s);
	if (ccb == NULL)
		return (1);

	ccb->c_flags = SCSI_DATA_IN | SCSI_POLL | flags;
	ccb->c_done = ips_done_mgmt;

	cmd = ccb->c_cmdbva;
	cmd->code = IPS_CMD_READCONF;
	cmd->sgaddr = htole32(sc->sc_infom.dm_paddr + offsetof(struct ips_info,
	    conf));

	return (ips_cmd(sc, ccb));
}

int
ips_getpg5(struct ips_softc *sc, int flags)
{
	struct ips_ccb *ccb;
	struct ips_cmd *cmd;
	int s;

	s = splbio();
	ccb = ips_ccb_get(sc);
	splx(s);
	if (ccb == NULL)
		return (1);

	ccb->c_flags = SCSI_DATA_IN | SCSI_POLL | flags;
	ccb->c_done = ips_done_mgmt;

	cmd = ccb->c_cmdbva;
	cmd->code = IPS_CMD_RWNVRAM;
	cmd->drive = 5;
	cmd->sgaddr = htole32(sc->sc_infom.dm_paddr + offsetof(struct ips_info,
	    pg5));

	return (ips_cmd(sc, ccb));
}

#if NBIO > 0
int
ips_getrblstat(struct ips_softc *sc, int flags)
{
<<<<<<< HEAD
	struct dmamem *dm;
	struct ips_cmd_adapterinfo *cmd;

	if ((dm = ips_dmamem_alloc(sc->sc_dmat, sizeof(*ai))) == NULL)
		return (1);

	cmd = sc->sc_cmdm->dm_kva;
	bzero(cmd, sizeof(*cmd));
	cmd->command = IPS_CMD_ADAPTERINFO;
	cmd->buffaddr = dm->dm_seg.ds_addr;

	(*sc->sc_exec)(sc);
	DELAY(1000);
	bcopy(dm->dm_kva, ai, sizeof(*ai));
	ips_dmamem_free(dm);

	return (0);
=======
	struct ips_ccb *ccb;
	struct ips_cmd *cmd;
	int s;

	s = splbio();
	ccb = ips_ccb_get(sc);
	splx(s);
	if (ccb == NULL)
		return (1);

	ccb->c_flags = SCSI_DATA_IN | SCSI_POLL | flags;
	ccb->c_done = ips_done_mgmt;

	cmd = ccb->c_cmdbva;
	cmd->code = IPS_CMD_REBUILDSTATUS;
	cmd->sgaddr = htole32(sc->sc_infom.dm_paddr + offsetof(struct ips_info,
	    rblstat));

	return (ips_cmd(sc, ccb));
>>>>>>> origin/master
}

int
ips_setstate(struct ips_softc *sc, int chan, int target, int state, int flags)
{
<<<<<<< HEAD
	struct dmamem *dm;
	struct ips_cmd_driveinfo *cmd;

	if ((dm = ips_dmamem_alloc(sc->sc_dmat, sizeof(*di))) == NULL)
		return (1);
=======
	struct ips_ccb *ccb;
	struct ips_cmd *cmd;
	int s;

	s = splbio();
	ccb = ips_ccb_get(sc);
	splx(s);
	if (ccb == NULL)
		return (1);

	ccb->c_flags = SCSI_POLL | flags;
	ccb->c_done = ips_done_mgmt;

	cmd = ccb->c_cmdbva;
	cmd->code = IPS_CMD_SETSTATE;
	cmd->drive = chan;
	cmd->sgcnt = target;
	cmd->seg4g = state;

	return (ips_cmd(sc, ccb));
}

int
ips_rebuild(struct ips_softc *sc, int chan, int target, int nchan,
    int ntarget, int flags)
{
	struct ips_ccb *ccb;
	struct ips_cmd *cmd;
	int s;

	s = splbio();
	ccb = ips_ccb_get(sc);
	splx(s);
	if (ccb == NULL)
		return (1);

	ccb->c_flags = SCSI_POLL | flags;
	ccb->c_done = ips_done_mgmt;

	cmd = ccb->c_cmdbva;
	cmd->code = IPS_CMD_REBUILD;
	cmd->drive = chan;
	cmd->sgcnt = target;
	cmd->seccnt = htole16(ntarget << 8 | nchan);

	return (ips_cmd(sc, ccb));
}
#endif	/* NBIO > 0 */
>>>>>>> origin/master

	cmd = sc->sc_cmdm->dm_kva;
	bzero(cmd, sizeof(*cmd));
	cmd->command = IPS_CMD_DRIVEINFO;
	cmd->buffaddr = dm->dm_seg.ds_addr;

	(*sc->sc_exec)(sc);
	DELAY(1000);
	bcopy(dm->dm_kva, di, sizeof(*di));
	ips_dmamem_free(dm);

<<<<<<< HEAD
	return (0);
}

void
ips_copperhead_exec(struct ips_softc *sc)
{
}

void
ips_copperhead_inten(struct ips_softc *sc)
=======
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IPS_REG_CCSA, ccb->c_cmdbpa);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, IPS_REG_CCC,
	    IPS_REG_CCC_START);
}

void
ips_copperhead_intren(struct ips_softc *sc)
>>>>>>> origin/master
{
}

<<<<<<< HEAD
int
ips_copperhead_intr(void *arg)
{
	return (0);
}

void
ips_morpheus_exec(struct ips_softc *sc)
{
	IPS_WRITE_4(sc, IPS_MORPHEUS_IQPR, sc->sc_cmdm->dm_seg.ds_addr);
}

void
ips_morpheus_inten(struct ips_softc *sc)
=======
u_int32_t
ips_copperhead_status(struct ips_softc *sc)
{
	u_int32_t sqhead, sqtail, status;

	sqhead = bus_space_read_4(sc->sc_iot, sc->sc_ioh, IPS_REG_SQH);
	DPRINTF(IPS_D_XFER, ("%s: sqhead 0x%08x, sqtail 0x%08x\n",
	    sc->sc_dev.dv_xname, sqhead, sc->sc_sqtail));

	sqtail = sc->sc_sqtail + sizeof(u_int32_t);
	if (sqtail == sc->sc_sqm.dm_paddr + IPS_SQSZ)
		sqtail = sc->sc_sqm.dm_paddr;
	if (sqtail == sqhead)
		return (0xffffffff);

	sc->sc_sqtail = sqtail;
	if (++sc->sc_sqidx == IPS_MAXCMDS)
		sc->sc_sqidx = 0;
	status = letoh32(sc->sc_sqbuf[sc->sc_sqidx]);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IPS_REG_SQT, sqtail);

	return (status);
}

void
ips_morpheus_exec(struct ips_softc *sc, struct ips_ccb *ccb)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IPS_REG_IQP, ccb->c_cmdbpa);
}

void
ips_morpheus_intren(struct ips_softc *sc)
>>>>>>> origin/master
{
	u_int32_t reg;

	reg = IPS_READ_4(sc, IPS_MORPHEUS_OIMR);
	reg &= ~0x08;
	IPS_WRITE_4(sc, IPS_MORPHEUS_OIMR, reg);
}

int
ips_morpheus_intr(void *arg)
{
<<<<<<< HEAD
	struct ips_softc *sc = arg;
	struct ccb *ccb;
	struct scsi_xfer *xs;
	u_int32_t oisr, oqpr;
	int gsc, id, s, rv = 0;

	oisr = IPS_READ_4(sc, IPS_MORPHEUS_OISR);
	DPRINTF(IPS_D_INTR, ("%s: intr, OISR 0x%08x\n",
	    sc->sc_dev.dv_xname, oisr));

	if (!(oisr & IPS_MORPHEUS_OISR_CMD))
		return (0);

	while ((oqpr = IPS_READ_4(sc, IPS_MORPHEUS_OQPR)) != 0xffffffff) {
		DPRINTF(IPS_D_INTR, ("OQPR 0x%08x\n", oqpr));
=======
	return (bus_space_read_4(sc->sc_iot, sc->sc_ioh, IPS_REG_OIS) &
	    IPS_REG_OIS_PEND);
}
>>>>>>> origin/master

		gsc = IPS_MORPHEUS_OQPR_GSC(oqpr);
		id = IPS_MORPHEUS_OQPR_ID(oqpr);

		if (gsc != IPS_MORPHEUS_GSC_NOERR &&
		    gsc != IPS_MORPHEUS_GSC_RECOV) {
			DPRINTF(IPS_D_ERR, ("%s: intr, error 0x%x",
			    sc->sc_dev.dv_xname, gsc));
			DPRINTF(IPS_D_ERR, (", OISR 0x%08x, OQPR 0x%08x\n",
			    oisr, oqpr));
		}

		if (id >= sc->sc_ai.max_concurrent_cmds) {
			DPRINTF(IPS_D_ERR, ("%s: intr, bogus id %d",
			    sc->sc_dev.dv_xname, id));
			DPRINTF(IPS_D_ERR, (", OISR 0x%08x, OQPR 0x%08x\n",
			    oisr, oqpr));
			continue;
		}

		ccb = &sc->sc_ccb[id];
		if (!(ccb->c_flags & CCB_F_RUN)) {
			DPRINTF(IPS_D_ERR, ("%s: intr, ccb id %d not run",
			    sc->sc_dev.dv_xname, id));
			DPRINTF(IPS_D_ERR, (", OISR 0x%08x, OQPR 0x%08x\n",
			    oisr, oqpr));
			continue;
		}

		rv = 1;
		bus_dmamap_unload(sc->sc_dmat, ccb->c_dmam);
		xs = ccb->c_xfer;
		xs->resid = 0;
		xs->flags |= ITSDONE;
		timeout_del(&xs->stimeout);
		s = splbio();
		scsi_done(xs);
		ccb->c_flags &= ~CCB_F_RUN;
		TAILQ_INSERT_TAIL(&sc->sc_ccbq, ccb, c_link);
		splx(s);
	}

	return (rv);
}

struct ccb *
ips_ccb_alloc(bus_dma_tag_t dmat, int n)
{
	struct ccb *ccb;
	int i;

<<<<<<< HEAD
	if ((ccb = malloc(n * sizeof(*ccb), M_DEVBUF, M_NOWAIT)) == NULL)
=======
	if ((ccb = malloc(n * sizeof(*ccb), M_DEVBUF,
	    M_NOWAIT | M_ZERO)) == NULL)
>>>>>>> origin/master
		return (NULL);
	bzero(ccb, n * sizeof(*ccb));

	for (i = 0; i < n; i++) {
		ccb[i].c_sc = sc;
		ccb[i].c_id = i;
<<<<<<< HEAD
		if (bus_dmamap_create(dmat, IPS_MAXFER, IPS_MAXSGS,
=======
		ccb[i].c_cmdbva = (char *)sc->sc_cmdbm.dm_vaddr +
		    i * sizeof(struct ips_cmdb);
		ccb[i].c_cmdbpa = sc->sc_cmdbm.dm_paddr +
		    i * sizeof(struct ips_cmdb);
		if (bus_dmamap_create(sc->sc_dmat, IPS_MAXFER, IPS_MAXSGS,
>>>>>>> origin/master
		    IPS_MAXFER, 0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
		    &ccb[i].c_dmam))
			goto fail;
	}

	return (ccb);
fail:
	for (; i > 0; i--)
		bus_dmamap_destroy(dmat, ccb[i - 1].c_dmam);
	free(ccb, M_DEVBUF);
	return (NULL);
}

void
ips_ccb_free(struct ccb *ccb, bus_dma_tag_t dmat, int n)
{
	int i;

	for (i = 0; i < n; i++)
		bus_dmamap_destroy(dmat, ccb[i - 1].c_dmam);
	free(ccb, M_DEVBUF);
}

<<<<<<< HEAD
struct dmamem *
ips_dmamem_alloc(bus_dma_tag_t tag, bus_size_t size)
=======
struct ips_ccb *
ips_ccb_get(struct ips_softc *sc)
{
	struct ips_ccb *ccb;

	splassert(IPL_BIO);

	if ((ccb = SLIST_FIRST(&sc->sc_ccbq_free)) != NULL) {
		SLIST_REMOVE_HEAD(&sc->sc_ccbq_free, c_link);
		ccb->c_flags = 0;
		ccb->c_xfer = NULL;
		bzero(ccb->c_cmdbva, sizeof(struct ips_cmdb));
	}

	return (ccb);
}

void
ips_ccb_put(struct ips_softc *sc, struct ips_ccb *ccb)
{
	splassert(IPL_BIO);

	ccb->c_state = IPS_CCB_FREE;
	SLIST_INSERT_HEAD(&sc->sc_ccbq_free, ccb, c_link);
}

int
ips_dmamem_alloc(struct dmamem *dm, bus_dma_tag_t tag, bus_size_t size)
>>>>>>> origin/master
{
	struct dmamem *dm;
	int nsegs;

	if ((dm = malloc(sizeof(*dm), M_DEVBUF, M_NOWAIT)) == NULL)
		return (NULL);

	dm->dm_tag = tag;
	dm->dm_size = size;

	if (bus_dmamap_create(tag, size, 1, size, 0,
	    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &dm->dm_map))
		goto fail1;
	if (bus_dmamem_alloc(tag, size, 0, 0, &dm->dm_seg, 1, &nsegs,
	    BUS_DMA_NOWAIT))
		goto fail2;
	if (bus_dmamem_map(tag, &dm->dm_seg, 1, size, (caddr_t *)&dm->dm_kva,
	    BUS_DMA_NOWAIT))
		goto fail3;
	bzero(dm->dm_kva, size);
	if (bus_dmamap_load(tag, dm->dm_map, dm->dm_kva, size, NULL,
	    BUS_DMA_NOWAIT))
		goto fail4;

	return (dm);

fail4:
	bus_dmamem_unmap(tag, dm->dm_kva, size);
fail3:
	bus_dmamem_free(tag, &dm->dm_seg, 1);
fail2:
	bus_dmamap_destroy(tag, dm->dm_map);
fail1:
	free(dm, M_DEVBUF);
	return (NULL);
}

void
ips_dmamem_free(struct dmamem *dm)
{
	bus_dmamap_unload(dm->dm_tag, dm->dm_map);
	bus_dmamem_unmap(dm->dm_tag, dm->dm_kva, dm->dm_size);
	bus_dmamem_free(dm->dm_tag, &dm->dm_seg, 1);
	bus_dmamap_destroy(dm->dm_tag, dm->dm_map);
	free(dm, M_DEVBUF);
}
