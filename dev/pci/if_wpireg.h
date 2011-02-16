<<<<<<< HEAD
/*	$OpenBSD: if_wpireg.h,v 1.11 2006/08/28 19:47:43 damien Exp $	*/

/*-
 * Copyright (c) 2006
=======
/*	$OpenBSD: if_wpireg.h,v 1.27 2009/10/24 20:17:17 damien Exp $	*/

/*-
 * Copyright (c) 2006-2008
>>>>>>> origin/master
 *	Damien Bergamini <damien.bergamini@free.fr>
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

#define WPI_TX_RING_COUNT	256
<<<<<<< HEAD
#define WPI_SVC_RING_COUNT	256
#define WPI_CMD_RING_COUNT	256
#define WPI_RX_RING_COUNT	64

/*
 * Rings must be aligned on a four 4K-pages boundary.
 * I had a hard time figuring this out.
 */
#define WPI_RING_DMA_ALIGN	0x4000

/* maximum scatter/gather */
#define WPI_MAX_SCATTER	4

/* maximum Rx buffer size */
#define WPI_RBUF_SIZE	(3 * 1024)	/* XXX 3000 but must be aligned! */
=======
#define WPI_TX_RING_LOMARK	192
#define WPI_TX_RING_HIMARK	224
#define WPI_RX_RING_COUNT_LOG	6
#define WPI_RX_RING_COUNT	(1 << WPI_RX_RING_COUNT_LOG)

#define WPI_NTXQUEUES		8
#define WPI_NDMACHNLS		6

/* Maximum scatter/gather. */
#define WPI_MAX_SCATTER	4

/* Maximum RX buffer size (larger than MCLBYTES.) */
#define WPI_RBUF_SIZE	(3 * 1024)

/* Base Address Register. */
#define WPI_PCI_BAR0	PCI_MAPREG_START
>>>>>>> origin/master

/*
 * Control and status registers.
 */
#define WPI_HW_IF_CONFIG	0x000
#define WPI_INT			0x008
#define WPI_MASK		0x00c
#define WPI_FH_INT		0x010
#define WPI_GPIO_IN		0x018
#define WPI_RESET		0x020
<<<<<<< HEAD
#define WPI_GPIO_CTL		0x024
#define WPI_EEPROM_CTL		0x02c
#define WPI_EEPROM_STATUS	0x030
#define WPI_UCODE_CLR		0x05c
#define WPI_TEMPERATURE		0x060
#define WPI_CHICKEN		0x100
#define WPI_PLL_CTL		0x20c
#define WPI_FW_TARGET		0x410
#define WPI_WRITE_MEM_ADDR  	0x444
#define WPI_READ_MEM_ADDR   	0x448
#define WPI_WRITE_MEM_DATA  	0x44c
#define WPI_READ_MEM_DATA   	0x450
#define WPI_TX_WIDX		0x460
#define WPI_TX_CTL(qid)		(0x940 + (qid) * 8)
#define WPI_TX_BASE(qid)	(0x944 + (qid) * 8)
#define WPI_TX_DESC(qid)	(0x980 + (qid) * 80)
#define WPI_RX_CONFIG		0xc00
#define WPI_RX_BASE		0xc04
#define WPI_RX_WIDX		0xc20
#define WPI_RX_RIDX_PTR		0xc24
#define WPI_RX_CTL		0xcc0
#define WPI_RX_STATUS		0xcc4
#define WPI_TX_CONFIG(qid)	(0xd00 + (qid) * 32)
#define WPI_TX_CREDIT(qid)	(0xd04 + (qid) * 32)
#define WPI_TX_STATE(qid)	(0xd08 + (qid) * 32)
#define WPI_TX_BASE_PTR		0xe80
#define WPI_MSG_CONFIG		0xe88
#define WPI_TX_STATUS		0xe90
=======
#define WPI_GP_CNTRL		0x024
#define WPI_EEPROM		0x02c
#define WPI_EEPROM_GP		0x030
#define WPI_UCODE_GP1_CLR	0x05c
#define WPI_UCODE_GP2		0x060
#define WPI_GIO_CHICKEN		0x100
#define WPI_ANA_PLL		0x20c
#define WPI_MEM_RADDR		0x40c
#define WPI_MEM_WADDR		0x410
#define WPI_MEM_WDATA		0x418
#define WPI_MEM_RDATA		0x41c
#define WPI_PRPH_WADDR		0x444
#define WPI_PRPH_RADDR		0x448
#define WPI_PRPH_WDATA		0x44c
#define WPI_PRPH_RDATA		0x450
#define WPI_HBUS_TARG_WRPTR	0x460
#define WPI_FH_CBBC_CTRL(qid)	(0x940 + (qid) * 8)
#define WPI_FH_CBBC_BASE(qid)	(0x944 + (qid) * 8)
#define WPI_FH_RX_CONFIG	0xc00
#define WPI_FH_RX_BASE		0xc04
#define WPI_FH_RX_WPTR		0xc20
#define WPI_FH_RX_RPTR_ADDR	0xc24
#define WPI_FH_RSSR_TBL		0xcc0
#define WPI_FH_RX_STATUS	0xcc4
#define WPI_FH_TX_CONFIG(qid)	(0xd00 + (qid) * 32)
#define WPI_FH_TX_BASE		0xe80
#define WPI_FH_MSG_CONFIG	0xe88
#define WPI_FH_TX_STATUS	0xe90
>>>>>>> origin/master


/*
 * NIC internal memory offsets.
 */
<<<<<<< HEAD
#define WPI_MEM_MODE		0x2e00
#define WPI_MEM_RA		0x2e04
#define WPI_MEM_TXCFG		0x2e10
#define WPI_MEM_MAGIC4		0x2e14
#define WPI_MEM_MAGIC5		0x2e20
#define WPI_MEM_BYPASS1		0x2e2c
#define WPI_MEM_BYPASS2		0x2e30
#define WPI_MEM_CLOCK1		0x3004
#define WPI_MEM_CLOCK2		0x3008
#define WPI_MEM_POWER		0x300c
#define WPI_MEM_PCIDEV		0x3010
#define WPI_MEM_UCODE_CTL	0x3400
#define WPI_MEM_UCODE_SRC	0x3404
#define WPI_MEM_UCODE_DST	0x3408
#define WPI_MEM_UCODE_SIZE	0x340c
#define WPI_MEM_UCODE_BASE	0x3800


/* possible flags for register WPI_HWCONFIG */
#define WPI_HW_ALM_MB	(1 << 8)
#define WPI_HW_ALM_MM	(1 << 9)
#define WPI_HW_SKU_MRC	(1 << 10)
#define WPI_HW_REV_D	(1 << 11)
#define WPI_HW_TYPE_B	(1 << 12)

/* possible flags for registers WPI_READ_MEM_ADDR/WPI_WRITE_MEM_ADDR */
#define WPI_MEM_4	((sizeof (uint32_t) - 1) << 24)

/* possible values for WPI_FW_TARGET */
#define WPI_FW_TEXT	0x00000000
#define WPI_FW_DATA	0x00800000

/* possible flags for WPI_GPIO_STATUS */
#define WPI_POWERED		(1 << 9)

/* possible flags for register WPI_RESET */
#define WPI_NEVO_RESET		(1 << 0)
#define WPI_SW_RESET		(1 << 7)
#define WPI_MASTER_DISABLED	(1 << 8)
#define WPI_STOP_MASTER		(1 << 9)

/* possible flags for register WPI_GPIO_CTL */
#define WPI_GPIO_CLOCK		(1 << 0)
#define WPI_GPIO_INIT		(1 << 2)
#define WPI_GPIO_MAC		(1 << 3)
#define WPI_GPIO_SLEEP		(1 << 4)
#define WPI_GPIO_PWR_STATUS	0x07000000
#define WPI_GPIO_PWR_SLEEP	(4 << 24)

/* possible flags for register WPI_CHICKEN */
#define WPI_CHICKEN_RXNOLOS	(1 << 23)

/* possible flags for register WPI_PLL_CTL */
#define WPI_PLL_INIT		(1 << 24)

/* possible flags for register WPI_UCODE_CLR */
#define WPI_RADIO_OFF		(1 << 1)
#define WPI_DISABLE_CMD		(1 << 2)

/* possible flags for WPI_RX_STATUS */
#define	WPI_RX_IDLE	(1 << 24)

/* possible flags for register WPI_UC_CTL */
#define WPI_UC_RUN	(1 << 30)

/* possible flags for register WPI_INTR_CSR */
#define WPI_ALIVE_INTR	(1 << 0)
#define WPI_WAKEUP_INTR	(1 << 1)
#define WPI_SW_ERROR	(1 << 25)
#define WPI_TX_INTR	(1 << 27)
#define WPI_HW_ERROR	(1 << 29)
#define WPI_RX_INTR	(1 << 31)

#define WPI_INTR_MASK							\
	(WPI_SW_ERROR | WPI_HW_ERROR | WPI_TX_INTR | WPI_RX_INTR |	\
	 WPI_ALIVE_INTR | WPI_WAKEUP_INTR)

/* possible flags for register WPI_TX_STATUS */
#define WPI_TX_IDLE(qid)	(1 << ((qid) + 24) | 1 << ((qid) + 16))

/* possible flags for register WPI_EEPROM_CTL */
#define WPI_EEPROM_READY	(1 << 0)

/* possible flags for register WPI_EEPROM_STATUS */
#define WPI_EEPROM_VERSION	0x00000007
#define WPI_EEPROM_LOCKED	0x00000180

=======
#define WPI_ALM_SCHED_MODE		0x2e00
#define WPI_ALM_SCHED_ARASTAT		0x2e04
#define WPI_ALM_SCHED_TXFACT		0x2e10
#define WPI_ALM_SCHED_TXF4MF		0x2e14
#define WPI_ALM_SCHED_TXF5MF		0x2e20
#define WPI_ALM_SCHED_SBYPASS_MODE1	0x2e2c
#define WPI_ALM_SCHED_SBYPASS_MODE2	0x2e30
#define WPI_APMG_CLK_ENA		0x3004
#define WPI_APMG_CLK_DIS		0x3008
#define WPI_APMG_RFKILL			0x3014
#define WPI_APMG_PS			0x300c
#define WPI_APMG_PCI_STT		0x3010
#define WPI_BSM_WR_CTRL			0x3400
#define WPI_BSM_WR_MEM_SRC		0x3404
#define WPI_BSM_WR_MEM_DST		0x3408
#define WPI_BSM_WR_DWCOUNT		0x340c
#define WPI_BSM_SRAM_BASE		0x3800
#define WPI_BSM_DRAM_TEXT_ADDR		0x3490
#define WPI_BSM_DRAM_TEXT_SIZE		0x3494
#define WPI_BSM_DRAM_DATA_ADDR		0x3498
#define WPI_BSM_DRAM_DATA_SIZE		0x349c


/* Possible flags for register WPI_HW_IF_CONFIG. */
#define WPI_HW_IF_CONFIG_ALM_MB		(1 << 8)
#define WPI_HW_IF_CONFIG_ALM_MM		(1 << 9)
#define WPI_HW_IF_CONFIG_SKU_MRC	(1 << 10)
#define WPI_HW_IF_CONFIG_REV_D		(1 << 11)
#define WPI_HW_IF_CONFIG_TYPE_B		(1 << 12)

/* Possible flags for registers WPI_PRPH_RADDR/WPI_PRPH_WADDR. */
#define WPI_PRPH_DWORD	((sizeof (uint32_t) - 1) << 24)

/* Possible values for WPI_BSM_WR_MEM_DST. */
#define WPI_FW_TEXT_BASE	0x00000000
#define WPI_FW_DATA_BASE	0x00800000

/* Possible flags for WPI_GPIO_IN. */
#define WPI_GPIO_IN_VMAIN	(1 << 9)

/* Possible flags for register WPI_RESET. */
#define WPI_RESET_NEVO			(1 << 0)
#define WPI_RESET_SW			(1 << 7)
#define WPI_RESET_MASTER_DISABLED	(1 << 8)
#define WPI_RESET_STOP_MASTER		(1 << 9)

/* Possible flags for register WPI_GP_CNTRL. */
#define WPI_GP_CNTRL_MAC_CLOCK_READY	(1 <<  0)
#define WPI_GP_CNTRL_MAC_ACCESS_ENA	(1 <<  0)
#define WPI_GP_CNTRL_INIT_DONE		(1 <<  2)
#define WPI_GP_CNTRL_MAC_ACCESS_REQ	(1 <<  3)
#define WPI_GP_CNTRL_SLEEP		(1 <<  4)
#define WPI_GP_CNTRL_PS_MASK		(7 << 24)
#define WPI_GP_CNTRL_MAC_PS		(4 << 24)
#define WPI_GP_CNTRL_RFKILL		(1 << 27)

/* Possible flags for register WPI_GIO_CHICKEN. */
#define WPI_GIO_CHICKEN_L1A_NO_L0S_RX	(1 << 23)

/* Possible flags for register WPI_FH_RX_CONFIG. */
#define WPI_FH_RX_CONFIG_DMA_ENA		(1 << 31)
#define WPI_FH_RX_CONFIG_RDRBD_ENA		(1 << 29)
#define WPI_FH_RX_CONFIG_WRSTATUS_ENA		(1 << 27)
#define WPI_FH_RX_CONFIG_MAXFRAG		(1 << 24)
#define WPI_FH_RX_CONFIG_NRBD(x)		((x) << 20)
#define WPI_FH_RX_CONFIG_IRQ_DST_HOST		(1 << 12)
#define WPI_FH_RX_CONFIG_IRQ_RBTH(x)		((x) << 4)

/* Possible flags for register WPI_ANA_PLL. */
#define WPI_ANA_PLL_INIT	(1 << 24)

/* Possible flags for register WPI_UCODE_GP1_CLR. */
#define WPI_UCODE_GP1_RFKILL		(1 << 1)
#define WPI_UCODE_GP1_CMD_BLOCKED	(1 << 2)

/* Possible flags for WPI_FH_RX_STATUS. */
#define	WPI_FH_RX_STATUS_IDLE	(1 << 24)

/* Possible flags for register WPI_BSM_WR_CTRL. */
#define WPI_BSM_WR_CTRL_START_EN	(1 << 30)
#define WPI_BSM_WR_CTRL_START		(1 << 31)

/* Possible flags for register WPI_INT. */
#define WPI_INT_ALIVE		(1 <<  0)
#define WPI_INT_WAKEUP		(1 <<  1)
#define WPI_INT_SW_RX		(1 <<  3)
#define WPI_INT_RF_TOGGLED	(1 <<  7)
#define WPI_INT_SW_ERR		(1 << 25)
#define WPI_INT_FH_TX		(1 << 27)
#define WPI_INT_HW_ERR		(1 << 29)
#define WPI_INT_FH_RX		(1 << 31)

/* Shortcut. */
#define WPI_INT_MASK							\
	(WPI_INT_SW_ERR | WPI_INT_HW_ERR | WPI_INT_FH_TX | 		\
	 WPI_INT_FH_RX | WPI_INT_ALIVE | WPI_INT_WAKEUP |		\
	 WPI_INT_SW_RX | WPI_INT_RF_TOGGLED)

/* Possible flags for register WPI_FH_INT. */
#define WPI_FH_INT_RX_CHNL(x)	(1 << ((x) + 16))
#define WPI_FH_INT_HI_PRIOR	(1 << 30)
/* Shortcuts for the above. */
#define WPI_FH_INT_RX			\
	(WPI_FH_INT_RX_CHNL(0) |	\
	 WPI_FH_INT_RX_CHNL(1) |	\
	 WPI_FH_INT_RX_CHNL(2) |	\
	 WPI_FH_INT_HI_PRIOR)

/* Possible flags for register WPI_FH_TX_STATUS. */
#define WPI_FH_TX_STATUS_IDLE(qid)	\
	(1 << ((qid) + 24) | 1 << ((qid) + 16))

/* Possible flags for register WPI_EEPROM. */
#define WPI_EEPROM_READ_VALID	(1 << 0)
#define WPI_EEPROM_CMD		(1 << 1)

/* Possible flags for register WPI_EEPROM_GP. */
#define WPI_EEPROM_GP_IF_OWNER	0x00000180

/* Possible flags for register WPI_APMG_PS. */
#define WPI_APMG_PS_PWR_SRC_MASK	(3 << 24)

/* Possible flags for register WPI_APMG_CLK_ENA/WPI_APMG_CLK_DIS. */
#define WPI_APMG_CLK_DMA_CLK_RQT	(1 <<  9)
#define WPI_APMG_CLK_BSM_CLK_RQT	(1 << 11)

/* Possible flags for register WPI_APMG_PCI_STT. */
#define WPI_APMG_PCI_STT_L1A_DIS	(1 << 11)
>>>>>>> origin/master

struct wpi_shared {
	uint32_t	txbase[8];
	uint32_t	next;
	uint32_t	reserved[2];
} __packed;

#define WPI_MAX_SEG_LEN	65520
struct wpi_tx_desc {
	uint32_t	flags;
#define WPI_PAD32(x)	((((x) + 3) & ~3) - (x))

	struct {
		uint32_t	addr;
		uint32_t	len;
	} __packed	segs[WPI_MAX_SCATTER];
	uint8_t		reserved[28];
} __packed;

struct wpi_tx_stat {
	uint8_t		nrts;
	uint8_t		retrycnt;
	uint8_t		nkill;
	uint8_t		rate;
	uint32_t	duration;
	uint32_t	status;
} __packed;

struct wpi_rx_desc {
	uint32_t	len;
	uint8_t		type;
#define WPI_UC_READY		  1
#define WPI_RX_DONE		 27
#define WPI_TX_DONE		 28
#define WPI_START_SCAN		130
#define WPI_STOP_SCAN		132
#define WPI_STATE_CHANGED	161

	uint8_t		flags;
	uint8_t		idx;
	uint8_t		qid;
} __packed;

struct wpi_rx_stat {
	uint8_t		len;
#define WPI_STAT_MAXLEN	20

	uint8_t		id;
	uint8_t		rssi;	/* received signal strength */
#define WPI_RSSI_OFFSET	95

	uint8_t		agc;	/* access gain control */
	uint16_t	signal;
	uint16_t	noise;
} __packed;

struct wpi_rx_head {
	uint16_t	chan;
	uint16_t	flags;
	uint8_t		reserved;
	uint8_t		rate;
	uint16_t	len;
} __packed;

struct wpi_rx_tail {
	uint32_t	flags;
#define WPI_RX_NO_CRC_ERR	(1 << 0)
#define WPI_RX_NO_OVFL_ERR	(1 << 1)
/* shortcut for the above */
#define WPI_RX_NOERROR		(WPI_RX_NO_CRC_ERR | WPI_RX_NO_OVFL_ERR)

	uint64_t	tstamp;
	uint32_t	tbeacon;
} __packed;

struct wpi_tx_cmd {
	uint8_t	code;
#define WPI_CMD_RXON		 16
#define WPI_CMD_ASSOCIATE	 17
<<<<<<< HEAD
#define WPI_CMD_TSF		 20
=======
#define WPI_CMD_EDCA_PARAMS	 19
#define WPI_CMD_TIMING		 20
>>>>>>> origin/master
#define WPI_CMD_ADD_NODE	 24
#define WPI_CMD_TX_DATA		 28
#define WPI_CMD_MRR_SETUP	 71
#define WPI_CMD_SET_LED		 72
#define WPI_CMD_SET_POWER_MODE	119
#define WPI_CMD_SCAN		128
<<<<<<< HEAD
#define WPI_CMD_SET_BEACON	145
#define WPI_CMD_BLUETOOTH	155
#define WPI_CMD_TXPOWER		176
=======
#define WPI_CMD_TXPOWER		151
#define WPI_CMD_BT_COEX		155
>>>>>>> origin/master

	uint8_t	flags;
	uint8_t	idx;
	uint8_t	qid;
	uint8_t	data[124];
} __packed;

<<<<<<< HEAD
/* structure for WPI_CMD_CONFIGURE */
struct wpi_config {
=======
/* Structure for command WPI_CMD_RXON. */
struct wpi_rxon {
>>>>>>> origin/master
	uint8_t		myaddr[IEEE80211_ADDR_LEN];
	uint16_t	reserved1;
	uint8_t		bssid[IEEE80211_ADDR_LEN];
	uint16_t	reserved2;
	uint32_t	reserved3[2];
	uint8_t		mode;
#define WPI_MODE_HOSTAP		1
#define WPI_MODE_STA		3
#define WPI_MODE_IBSS		4
#define WPI_MODE_MONITOR	6

	uint8_t		reserved4[3];
	uint8_t		ofdm_mask;
	uint8_t		cck_mask;
	uint16_t	state;
#define WPI_STATE_ASSOCIATED	4

	uint32_t	flags;
#define WPI_RXON_24GHZ		(1 <<  0)
#define WPI_RXON_CCK		(1 <<  1)
#define WPI_RXON_AUTO		(1 <<  2)
#define WPI_RXON_SHSLOT		(1 <<  4)
#define WPI_RXON_SHPREAMBLE	(1 <<  5)
#define WPI_RXON_NODIVERSITY	(1 <<  7)
#define WPI_RXON_ANT_A		(1 <<  8)
#define WPI_RXON_ANT_B		(1 <<  9)
#define WPI_RXON_TSF		(1 << 15)

	uint32_t	filter;
#define WPI_FILTER_PROMISC	(1 << 0)
#define WPI_FILTER_CTL		(1 << 1)
#define WPI_FILTER_MULTICAST	(1 << 2)
#define WPI_FILTER_NODECRYPT	(1 << 3)
#define WPI_FILTER_BSS		(1 << 5)

	uint8_t		chan;
	uint8_t		reserved6[3];
} __packed;

/* Structure for command WPI_CMD_ASSOCIATE. */
struct wpi_assoc {
	uint32_t	flags;
	uint32_t	filter;
	uint8_t		ofdm_mask;
	uint8_t		cck_mask;
	uint16_t	reserved;
} __packed;

<<<<<<< HEAD
/* structure for command WPI_CMD_TSF */
struct wpi_cmd_tsf {
=======
/* Structure for command WPI_CMD_EDCA_PARAMS. */
struct wpi_edca_params {
	uint32_t	flags;
#define WPI_EDCA_UPDATE	(1 << 0)
#define WPI_EDCA_TXOP	(1 << 4)

	struct {
		uint16_t	cwmin;
		uint16_t	cwmax;
		uint8_t		aifsn;
		uint8_t		reserved;
		uint16_t	txoplimit;
	} __packed	ac[EDCA_NUM_AC];
} __packed;

/* Structure for command WPI_CMD_TIMING. */
struct wpi_cmd_timing {
>>>>>>> origin/master
	uint64_t	tstamp;
	uint16_t	bintval;
	uint16_t	atim;
	uint32_t	binitval;
	uint16_t	lintval;
	uint16_t	reserved;
} __packed;

<<<<<<< HEAD
/* structure for WPI_CMD_ADD_NODE */
=======
/* Structure for command WPI_CMD_ADD_NODE. */
>>>>>>> origin/master
struct wpi_node_info {
	uint8_t		control;
#define WPI_NODE_UPDATE	(1 << 0)

	uint8_t		reserved1[3];
	uint8_t		bssid[IEEE80211_ADDR_LEN];
	uint16_t	reserved2;
	uint8_t		id;
#define WPI_ID_BSS		0
#define WPI_ID_BROADCAST	24

	uint8_t		sta_mask;
	uint16_t	reserved3;
<<<<<<< HEAD
	uint16_t	key_flags;
	uint8_t		tkip;
	uint8_t		reserved4;
	uint16_t	ttak[5];
	uint16_t	reserved5;
	uint8_t		key[IEEE80211_KEYBUF_SIZE];
	uint32_t	flags;
	uint32_t	mask;
	uint16_t	tid;
	uint8_t		rate;
	uint8_t		reserved6;
=======
	uint16_t	kflags;
#define WPI_KFLAG_CCMP		(1 <<  1)
#define WPI_KFLAG_KID(kid)	((kid) << 8)

	uint8_t		tsc2;
	uint8_t		reserved4;
	uint16_t	ttak[5];
	uint16_t	reserved5;
	uint8_t		key[16];

	uint32_t	action;
#define WPI_ACTION_SET_RATE	(1 << 2)

	uint32_t	mask;
	uint16_t	tid;
	uint8_t		plcp;
	uint8_t		antenna;
#define WPI_ANTENNA_A		(1 << 6)
#define WPI_ANTENNA_B		(1 << 7)
#define WPI_ANTENNA_BOTH	(WPI_ANTENNA_A | WPI_ANTENNA_B)

>>>>>>> origin/master
	uint8_t		add_imm;
	uint8_t		del_imm;
	uint16_t	add_imm_start;
} __packed;

/* Structure for command WPI_CMD_TX_DATA. */
struct wpi_cmd_data {
	uint16_t	len;
	uint16_t	lnext;
	uint32_t	flags;
#define WPI_TX_NEED_RTS		(1 <<  1)
#define WPI_TX_NEED_CTS		(1 <<  2)
#define WPI_TX_NEED_ACK		(1 <<  3)
#define WPI_TX_FULL_TXOP	(1 <<  7)
#define WPI_TX_AUTO_SEQ		(1 << 13)
#define WPI_TX_INSERT_TSTAMP	(1 << 16)

	uint8_t		plcp;
	uint8_t		id;
	uint8_t		tid;
	uint8_t		security;
	uint8_t		key[IEEE80211_KEYBUF_SIZE];
	uint8_t		tkip[IEEE80211_TKIP_MICLEN];
	uint32_t	fnext;
	uint32_t	lifetime;
	uint8_t		ofdm_mask;
	uint8_t		cck_mask;
	uint8_t		rts_ntries;
	uint8_t		data_ntries;
	uint16_t	timeout;
	uint16_t	txop;
	struct		ieee80211_frame wh;
} __packed;

/* structure for command WPI_CMD_SET_BEACON */
struct wpi_cmd_beacon {
	uint16_t	len;
	uint16_t	reserved1;
	uint32_t	flags;	/* same as wpi_cmd_data */
	uint8_t		rate;
	uint8_t		id;
	uint8_t		reserved2[30];
	uint32_t	lifetime;
	uint8_t		ofdm_mask;
	uint8_t		cck_mask;
	uint16_t	reserved3[3];
	uint16_t	tim;
	uint8_t		timsz;
	uint8_t		reserved4;
	struct		ieee80211_frame wh;
} __packed;

<<<<<<< HEAD
/* structure for WPI_CMD_MRR_SETUP */
=======
/* Structure for command WPI_CMD_MRR_SETUP. */
#define WPI_RIDX_MAX	11
>>>>>>> origin/master
struct wpi_mrr_setup {
	uint32_t	which;
#define WPI_MRR_CTL	0
#define WPI_MRR_DATA	1

	struct {
		uint8_t	plcp;
		uint8_t	flags;
		uint8_t	ntries;
		uint8_t	next;
	} __packed	rates[WPI_RIDX_MAX + 1];
} __packed;

<<<<<<< HEAD
/* structure for WPI_CMD_SET_LED */
=======
/* Structure for command WPI_CMD_SET_LED. */
>>>>>>> origin/master
struct wpi_cmd_led {
	uint32_t	unit;	/* multiplier (in usecs) */
	uint8_t		which;
#define WPI_LED_ACTIVITY	1
#define WPI_LED_LINK		2

	uint8_t		off;
	uint8_t		on;
	uint8_t		reserved;
} __packed;

<<<<<<< HEAD
/* structure for WPI_CMD_SET_POWER_MODE */
struct wpi_power {
	uint32_t	flags;
	uint32_t	rx_timeout;
	uint32_t	tx_timeout;
	uint32_t	sleep[5];
} __packed;

/* structure for command WPI_CMD_SCAN */
=======
/* Structure for command WPI_CMD_SET_POWER_MODE. */
struct wpi_pmgt_cmd {
	uint32_t	flags;
#define WPI_PS_ALLOW_SLEEP	(1 << 0)
#define WPI_PS_SLEEP_OVER_DTIM	(1 << 2)
#define WPI_PS_PCI_PMGT		(1 << 3)

	uint32_t	rxtimeout;
	uint32_t	txtimeout;
	uint32_t	intval[5];
} __packed;

/* Structures for command WPI_CMD_SCAN. */
struct wpi_scan_essid {
	uint8_t	id;
	uint8_t	len;
	uint8_t	data[IEEE80211_NWID_LEN];
} __packed;

>>>>>>> origin/master
struct wpi_scan_hdr {
	uint8_t		len;
	uint8_t		first;
	uint8_t		reserved1;
	uint8_t		nchan;
<<<<<<< HEAD
	uint16_t	quiet;
	uint16_t	threshold;
	uint16_t	band;
#define WPI_SCAN_5GHZ	1

	uint16_t	reserved2[5];
=======
	uint16_t	quiet_time;
	uint16_t	quiet_threshold;
	uint16_t	crc_threshold;
	uint16_t	reserved2;
	uint32_t	max_svc;	/* background scans */
	uint32_t	pause_svc;	/* background scans */
>>>>>>> origin/master
	uint32_t	flags;
	uint32_t	filter;
	uint16_t	length;
	uint16_t	reserved4;
	uint32_t	magic1;
	uint8_t		rate;
	uint8_t		id;
	uint16_t	reserved5;
	uint32_t	reserved6[7];
	uint32_t	mask;
	uint32_t	reserved7[2];
	uint8_t		reserved8;
	uint8_t		esslen;
	uint8_t		essid[134];

<<<<<<< HEAD
	/* followed by probe request body */
	/* followed by nchan x wpi_scan_chan */
=======
	/* Followed by a struct wpi_cmd_data. */
	/* Followed by an array of 4 structq wpi_scan_essid. */
	/* Followed by probe request body. */
	/* Followed by an array of ``nchan'' structs wpi_scan_chan. */
>>>>>>> origin/master
} __packed;

struct wpi_scan_chan {
	uint8_t		flags;
<<<<<<< HEAD
#define WPI_CHAN_ACTIVE	3
=======
#define WPI_CHAN_ACTIVE		(1 << 0)
#define WPI_CHAN_NPBREQS(x)	(((1 << (x)) - 1) << 1)
>>>>>>> origin/master

	uint8_t		chan;
	uint16_t	magic;		/* XXX */
	uint16_t	active;		/* msecs */
	uint16_t	passive;	/* msecs */
} __packed;

<<<<<<< HEAD
/* structure for WPI_CMD_BLUETOOTH */
=======
/* Maximum size of a scan command. */
#define WPI_SCAN_MAXSZ	(MCLBYTES - 4)

/* Structure for command WPI_CMD_TXPOWER. */
struct wpi_cmd_txpower {
	uint8_t		band;
#define WPI_BAND_5GHZ	0
#define WPI_BAND_2GHZ	1

	uint8_t		reserved;
	uint16_t	chan;
	struct {
		uint8_t	plcp;
		uint8_t	rf_gain;
		uint8_t	dsp_gain;
		uint8_t	reserved;
	} __packed	rates[WPI_RIDX_MAX + 1];
} __packed;

/* Structure for command WPI_CMD_BT_COEX. */
>>>>>>> origin/master
struct wpi_bluetooth {
	uint8_t		flags;
#define WPI_BT_COEX_DISABLE	0
#define WPI_BT_COEX_MODE_2WIRE	1
#define WPI_BT_COEX_MODE_3WIRE	2
#define WPI_BT_COEX_MODE_4WIRE	3

	uint8_t		lead_time;
#define WPI_BT_LEAD_TIME_DEF	30

	uint8_t		max_kill;
#define WPI_BT_MAX_KILL_DEF	5

	uint8_t		reserved;
	uint32_t	kill_ack;
	uint32_t	kill_cts;
} __packed;

<<<<<<< HEAD
/* structure for command WPI_CMD_TXPOWER */
struct wpi_txpower {
	uint32_t	reserved1;
	uint16_t	pwr1[14];
	uint32_t	reserved2[2];
	uint16_t	pwr2[14];
	uint32_t	reserved3[2];
} __packed;

=======
/* Structures for WPI_RX_DONE notification. */
struct wpi_rx_stat {
	uint8_t		len;
#define WPI_STAT_MAXLEN	20

	uint8_t		id;
	uint8_t		rssi;	/* received signal strength */
#define WPI_RSSI_OFFSET	95

	uint8_t		agc;	/* access gain control */
	uint16_t	signal;
	uint16_t	noise;
} __packed;

struct wpi_rx_head {
	uint16_t	chan;
	uint16_t	flags;
	uint8_t		reserved;
	uint8_t		rate;
	uint16_t	len;
} __packed;

struct wpi_rx_tail {
	uint32_t	flags;
#define WPI_RX_NO_CRC_ERR	(1 <<  0)
#define WPI_RX_NO_OVFL_ERR	(1 <<  1)
/* Shortcut for the above. */
#define WPI_RX_NOERROR		(WPI_RX_NO_CRC_ERR | WPI_RX_NO_OVFL_ERR)
#define WPI_RX_CIPHER_MASK	(7 <<  8)
#define WPI_RX_CIPHER_CCMP	(2 <<  8)
#define WPI_RX_DECRYPT_MASK	(3 << 11)
#define WPI_RX_DECRYPT_OK	(3 << 11)
>>>>>>> origin/master

/* firmware image header */
struct wpi_firmware_hdr {
	uint32_t	version;
	uint32_t	textsz;
	uint32_t	datasz;
	uint32_t	bootsz;
} __packed;

/* Structure for WPI_UC_READY notification. */
struct wpi_ucode_info {
	uint32_t	version;
	uint8_t		revision[8];
	uint8_t		type;
	uint8_t		subtype;
#define WPI_UCODE_INIT	9

	uint16_t	reserved;
	uint32_t	logptr;
	uint32_t	errptr;
	uint32_t	timestamp;
	uint32_t	valid;
} __packed;

/* Structure for WPI_START_SCAN notification. */
struct wpi_start_scan {
	uint64_t	tstamp;
	uint32_t	tbeacon;
	uint8_t		chan;
	uint8_t		band;
	uint16_t	reserved;
	uint32_t	status;
} __packed;

/* Structure for WPI_STOP_SCAN notification. */
struct wpi_stop_scan {
	uint8_t		nchan;
	uint8_t		status;
	uint8_t		reserved;
	uint8_t		chan;
	uint64_t	tsf;
} __packed;

<<<<<<< HEAD
=======

/* Firmware error dump entry. */
struct wpi_fwdump {
	uint32_t	desc;
	uint32_t	time;
	uint32_t	blink[2];
	uint32_t	ilink[2];
	uint32_t	data;
} __packed;

/* Firmware image file header. */
struct wpi_firmware_hdr {
	uint32_t	version;
	uint32_t	main_textsz;
	uint32_t	main_datasz;
	uint32_t	init_textsz;
	uint32_t	init_datasz;
	uint32_t	boot_textsz;
} __packed;

#define WPI_FW_TEXT_MAXSZ	(80 * 1024)
#define WPI_FW_DATA_MAXSZ	(32 * 1024)
#define WPI_FW_BOOT_TEXT_MAXSZ	1024

#define WPI_FW_UPDATED	(1 << 31)

/*
 * Offsets into EEPROM.
 */
>>>>>>> origin/master
#define WPI_EEPROM_MAC		0x015
#define WPI_EEPROM_REVISION	0x035
#define WPI_EEPROM_CAPABILITIES	0x045
#define WPI_EEPROM_TYPE		0x04a
<<<<<<< HEAD
#define WPI_EEPROM_PWR1		0x1ae
#define WPI_EEPROM_PWR2		0x1bc
=======
#define WPI_EEPROM_DOMAIN	0x060
#define WPI_EEPROM_BAND1	0x063
#define WPI_EEPROM_BAND2	0x072
#define WPI_EEPROM_BAND3	0x080
#define WPI_EEPROM_BAND4	0x08d
#define WPI_EEPROM_BAND5	0x099
#define WPI_EEPROM_POWER_GRP	0x100

struct wpi_eeprom_chan {
	uint8_t	flags;
#define WPI_EEPROM_CHAN_VALID	(1 << 0)
#define WPI_EEPROM_CHAN_IBSS	(1 << 1)
#define WPI_EEPROM_CHAN_ACTIVE	(1 << 3)
#define WPI_EEPROM_CHAN_RADAR	(1 << 4)

	int8_t	maxpwr;
} __packed;

struct wpi_eeprom_sample {
	uint8_t		index;
	int8_t		power;
	uint16_t	volt;
} __packed;

#define WPI_POWER_GROUPS_COUNT	5
struct wpi_eeprom_group {
	struct		wpi_eeprom_sample samples[5];
	int32_t		coef[5];
	int32_t		corr[5];
	int8_t		maxpwr;
	uint8_t		chan;
	int16_t		temp;
} __packed;

#define WPI_CHAN_BANDS_COUNT	5
#define WPI_MAX_CHAN_PER_BAND	14
static const struct wpi_chan_band {
	uint32_t	addr;	/* offset in EEPROM */
	uint8_t		nchan;
	uint8_t		chan[WPI_MAX_CHAN_PER_BAND];
} wpi_bands[5] = {
	{ WPI_EEPROM_BAND1, 14,
	    { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 } },
	{ WPI_EEPROM_BAND2, 13,
	    { 183, 184, 185, 187, 188, 189, 192, 196, 7, 8, 11, 12, 16 } },
	{ WPI_EEPROM_BAND3, 12,
	    { 34, 36, 38, 40, 42, 44, 46, 48, 52, 56, 60, 64 } },
	{ WPI_EEPROM_BAND4, 11,
	    { 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140 } },
	{ WPI_EEPROM_BAND5, 6,
	    { 145, 149, 153, 157, 161, 165 } }
};

/* HW rate indices. */
#define WPI_RIDX_OFDM6	 0
#define WPI_RIDX_OFDM36	 5
#define WPI_RIDX_OFDM48	 6
#define WPI_RIDX_OFDM54	 7
#define WPI_RIDX_CCK1	 8
#define WPI_RIDX_CCK2	 9
#define WPI_RIDX_CCK11	11

static const struct wpi_rate {
	uint8_t	rate;
	uint8_t	plcp;
} wpi_rates[WPI_RIDX_MAX + 1] = {
	{  12, 0xd },
	{  18, 0xf },
	{  24, 0x5 },
	{  36, 0x7 },
	{  48, 0x9 },
	{  72, 0xb },
	{  96, 0x1 },
	{ 108, 0x3 },
	{   2,  10 },
	{   4,  20 },
	{  11,  55 },
	{  22, 110 }
};

#define WPI_MAX_PWR_INDEX	77
/*
 * RF Tx gain values from highest to lowest power (values obtained from
 * the reference driver.)
 */
static const uint8_t wpi_rf_gain_2ghz[WPI_MAX_PWR_INDEX + 1] = {
	0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xbb, 0xbb, 0xbb,
	0xbb, 0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xd3, 0xd3, 0xb3, 0xb3, 0xb3,
	0x93, 0x93, 0x93, 0x93, 0x93, 0x93, 0x93, 0x73, 0xeb, 0xeb, 0xeb,
	0xcb, 0xcb, 0xcb, 0xcb, 0xcb, 0xcb, 0xcb, 0xab, 0xab, 0xab, 0x8b,
	0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xc3, 0xc3, 0xc3, 0xc3, 0xa3,
	0xa3, 0xa3, 0xa3, 0x83, 0x83, 0x83, 0x83, 0x63, 0x63, 0x63, 0x63,
	0x43, 0x43, 0x43, 0x43, 0x23, 0x23, 0x23, 0x23, 0x03, 0x03, 0x03,
	0x03
};

static const uint8_t wpi_rf_gain_5ghz[WPI_MAX_PWR_INDEX + 1] = {
	0xfb, 0xfb, 0xfb, 0xdb, 0xdb, 0xbb, 0xbb, 0x9b, 0x9b, 0x7b, 0x7b,
	0x7b, 0x7b, 0x5b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x1b, 0x1b,
	0x1b, 0x73, 0x73, 0x73, 0x53, 0x53, 0x53, 0x53, 0x53, 0x33, 0x33,
	0x33, 0x33, 0x13, 0x13, 0x13, 0x13, 0x13, 0xab, 0xab, 0xab, 0x8b,
	0x8b, 0x8b, 0x8b, 0x6b, 0x6b, 0x6b, 0x6b, 0x4b, 0x4b, 0x4b, 0x4b,
	0x2b, 0x2b, 0x2b, 0x2b, 0x0b, 0x0b, 0x0b, 0x0b, 0x83, 0x83, 0x63,
	0x63, 0x63, 0x63, 0x43, 0x43, 0x43, 0x43, 0x23, 0x23, 0x23, 0x23,
	0x03
};

/*
 * DSP pre-DAC gain values from highest to lowest power (values obtained
 * from the reference driver.)
 */
static const uint8_t wpi_dsp_gain_2ghz[WPI_MAX_PWR_INDEX + 1] = {
	0x7f, 0x7f, 0x7f, 0x7f, 0x7d, 0x6e, 0x69, 0x62, 0x7d, 0x73, 0x6c,
	0x63, 0x77, 0x6f, 0x69, 0x61, 0x5c, 0x6a, 0x64, 0x78, 0x71, 0x6b,
	0x7d, 0x77, 0x70, 0x6a, 0x65, 0x61, 0x5b, 0x6b, 0x79, 0x73, 0x6d,
	0x7f, 0x79, 0x73, 0x6c, 0x66, 0x60, 0x5c, 0x6e, 0x68, 0x62, 0x74,
	0x7d, 0x77, 0x71, 0x6b, 0x65, 0x60, 0x71, 0x6a, 0x66, 0x5f, 0x71,
	0x6a, 0x66, 0x5f, 0x71, 0x6a, 0x66, 0x5f, 0x71, 0x6a, 0x66, 0x5f,
	0x71, 0x6a, 0x66, 0x5f, 0x71, 0x6a, 0x66, 0x5f, 0x71, 0x6a, 0x66,
	0x5f
};

static const uint8_t wpi_dsp_gain_5ghz[WPI_MAX_PWR_INDEX + 1] = {
	0x7f, 0x78, 0x72, 0x77, 0x65, 0x71, 0x66, 0x72, 0x67, 0x75, 0x6b,
	0x63, 0x5c, 0x6c, 0x7d, 0x76, 0x6d, 0x66, 0x60, 0x5a, 0x68, 0x62,
	0x5c, 0x76, 0x6f, 0x68, 0x7e, 0x79, 0x71, 0x69, 0x63, 0x76, 0x6f,
	0x68, 0x62, 0x74, 0x6d, 0x66, 0x62, 0x5d, 0x71, 0x6b, 0x63, 0x78,
	0x71, 0x6b, 0x63, 0x78, 0x71, 0x6b, 0x63, 0x78, 0x71, 0x6b, 0x63,
	0x78, 0x71, 0x6b, 0x63, 0x78, 0x71, 0x6b, 0x63, 0x6b, 0x63, 0x78,
	0x71, 0x6b, 0x63, 0x78, 0x71, 0x6b, 0x63, 0x78, 0x71, 0x6b, 0x63,
	0x78
};
>>>>>>> origin/master

/*
 * Power saving settings (values obtained from the reference driver.)
 */
#define WPI_NDTIMRANGES		2
#define WPI_NPOWERLEVELS	6
static const struct wpi_pmgt {
	uint32_t	rxtimeout;
	uint32_t	txtimeout;
	uint32_t	intval[5];
	int		skip_dtim;
} wpi_pmgt[WPI_NDTIMRANGES][WPI_NPOWERLEVELS] = {
	/* DTIM <= 10 */
	{
	{   0,   0, {  0,  0,  0,  0,  0 }, 0 },	/* CAM */
	{ 200, 500, {  1,  2,  3,  4,  4 }, 0 },	/* PS level 1 */
	{ 200, 300, {  2,  4,  6,  7,  7 }, 0 },	/* PS level 2 */
	{  50, 100, {  2,  6,  9,  9, 10 }, 0 },	/* PS level 3 */
	{  50,  25, {  2,  7,  9,  9, 10 }, 1 },	/* PS level 4 */
	{  25,  25, {  4,  7, 10, 10, 10 }, 1 }		/* PS level 5 */
	},
	/* DTIM >= 11 */
	{
	{   0,   0, {  0,  0,  0,  0,  0 }, 0 },	/* CAM */
	{ 200, 500, {  1,  2,  3,  4, -1 }, 0 },	/* PS level 1 */
	{ 200, 300, {  2,  4,  6,  7, -1 }, 0 },	/* PS level 2 */
	{  50, 100, {  2,  6,  9,  9, -1 }, 0 },	/* PS level 3 */
	{  50,  25, {  2,  7,  9,  9, -1 }, 0 },	/* PS level 4 */
	{  25,  25, {  4,  7, 10, 10, -1 }, 0 }		/* PS level 5 */
	}
};

/* Firmware errors. */
static const char * const wpi_fw_errmsg[] = {
	"OK",
	"FAIL",
	"BAD_PARAM",
	"BAD_CHECKSUM",
	"NMI_INTERRUPT",
	"SYSASSERT",
	"FATAL_ERROR"
};

#define WPI_READ(sc, reg)						\
	bus_space_read_4((sc)->sc_st, (sc)->sc_sh, (reg))

#define WPI_WRITE(sc, reg, val)						\
	bus_space_write_4((sc)->sc_st, (sc)->sc_sh, (reg), (val))

#define WPI_WRITE_REGION_4(sc, offset, datap, count)			\
	bus_space_write_region_4((sc)->sc_st, (sc)->sc_sh, (offset),	\
	    (datap), (count))

#define WPI_SETBITS(sc, reg, mask)					\
	WPI_WRITE(sc, reg, WPI_READ(sc, reg) | (mask))
 
#define WPI_CLRBITS(sc, reg, mask)					\
	WPI_WRITE(sc, reg, WPI_READ(sc, reg) & ~(mask))

#define WPI_BARRIER_WRITE(sc)						\
	bus_space_barrier((sc)->sc_st, (sc)->sc_sh, 0, (sc)->sc_sz,	\
	    BUS_SPACE_BARRIER_WRITE)

#define WPI_BARRIER_READ_WRITE(sc)					\
	bus_space_barrier((sc)->sc_st, (sc)->sc_sh, 0, (sc)->sc_sz,	\
	    BUS_SPACE_BARRIER_READ | BUS_SPACE_BARRIER_WRITE)
