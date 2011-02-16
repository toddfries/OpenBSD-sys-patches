<<<<<<< HEAD
/*	$OpenBSD: hci.h,v 1.2 2005/01/17 18:12:49 mickey Exp $	*/
=======
/*	$OpenBSD: hci.h,v 1.13 2008/11/25 14:00:12 uwe Exp $	*/
/*	$NetBSD: hci.h,v 1.28 2008/09/08 23:36:55 gmcgarry Exp $	*/
>>>>>>> origin/master

/*
 * ng_hci.h
 *
 * Copyright (c) 2001 Maksim Yevmenkin <m_evmenkin@yahoo.com>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
<<<<<<< HEAD
 * $FreeBSD: src/sys/netgraph/bluetooth/include/ng_hci.h,v 1.4 2004/08/10 00:38:50 emax Exp $
=======
 * $Id: hci.h,v 1.13 2008/11/25 14:00:12 uwe Exp $
 * $FreeBSD: src/sys/netgraph/bluetooth/include/ng_hci.h,v 1.6 2005/01/07 01:45:43 imp Exp $
>>>>>>> origin/master
 */

/*
 * This file contains everything that application needs to know about
 * Host Controller Interface (HCI). All information was obtained from
 * Bluetooth Specification Book v1.1.
 *
 * This file can be included by both kernel and userland applications.
 *
 * NOTE: Here and after Bluetooth device is called a "unit". Bluetooth
 *       specification refers to both devices and units. They are the
 *       same thing (i think), so to be consistent word "unit" will be
 *       used.
 */

#ifndef _NETGRAPH_HCI_H_
#define _NETGRAPH_HCI_H_

/**************************************************************************
 **************************************************************************
 **     Netgraph node hook name, type name and type cookie and commands
 **************************************************************************
 **************************************************************************/

/* Node type name and type cookie */
#define NG_HCI_NODE_TYPE			"hci"
#define NGM_HCI_COOKIE				1000774184

/* Netgraph node hook names */
#define NG_HCI_HOOK_DRV				"drv" /* Driver <-> HCI */
#define NG_HCI_HOOK_ACL				"acl" /* HCI <-> Upper */
#define NG_HCI_HOOK_SCO				"sco" /* HCI <-> Upper */ 
#define NG_HCI_HOOK_RAW				"raw" /* HCI <-> Upper */ 

/**************************************************************************
 **************************************************************************
 **                   Common defines and types (HCI)
 **************************************************************************
 **************************************************************************/

/* All sizes are in bytes */
#define NG_HCI_BDADDR_SIZE			6   /* unit address */
#define NG_HCI_LAP_SIZE				3   /* unit LAP */
#define NG_HCI_KEY_SIZE				16  /* link key */
#define NG_HCI_PIN_SIZE				16  /* link PIN */
#define NG_HCI_EVENT_MASK_SIZE			8   /* event mask */
#define NG_HCI_CLASS_SIZE			3   /* unit class */
#define NG_HCI_FEATURES_SIZE			8   /* LMP features */
#define NG_HCI_UNIT_NAME_SIZE			248 /* unit name size */

/* HCI specification */
#define NG_HCI_SPEC_V10				0x00 /* v1.0 */
#define NG_HCI_SPEC_V11				0x01 /* v1.1 */
/* 0x02 - 0xFF - reserved for future use */

/* LMP features */
/* ------------------- byte 0 --------------------*/
#define NG_HCI_LMP_3SLOT			0x01
#define NG_HCI_LMP_5SLOT			0x02
#define NG_HCI_LMP_ENCRYPTION			0x04
#define NG_HCI_LMP_SLOT_OFFSET			0x08
#define NG_HCI_LMP_TIMING_ACCURACY		0x10
#define NG_HCI_LMP_SWITCH			0x20
#define NG_HCI_LMP_HOLD_MODE			0x40
#define NG_HCI_LMP_SNIFF_MODE			0x80
/* ------------------- byte 1 --------------------*/
#define NG_HCI_LMP_PARK_MODE			0x01
#define NG_HCI_LMP_RSSI				0x02
#define NG_HCI_LMP_CHANNEL_QUALITY		0x04
#define NG_HCI_LMP_SCO_LINK			0x08
#define NG_HCI_LMP_HV2_PKT			0x10
#define NG_HCI_LMP_HV3_PKT			0x20
#define NG_HCI_LMP_ULAW_LOG			0x40
#define NG_HCI_LMP_ALAW_LOG			0x80
/* ------------------- byte 2 --------------------*/
#define NG_HCI_LMP_CVSD				0x01
#define NG_HCI_LMP_PAGING_SCHEME		0x02
#define NG_HCI_LMP_POWER_CONTROL		0x04
#define NG_HCI_LMP_TRANSPARENT_SCO		0x08
#define NG_HCI_LMP_FLOW_CONTROL_LAG0		0x10
#define NG_HCI_LMP_FLOW_CONTROL_LAG1		0x20
#define NG_HCI_LMP_FLOW_CONTROL_LAG2		0x40

/* Link types */
#define NG_HCI_LINK_SCO				0x00 /* Voice */
#define NG_HCI_LINK_ACL				0x01 /* Data */
/* 0x02 - 0xFF - reserved for future use */

/* Packet types */
				/* 0x0001 - 0x0004 - reserved for future use */
#define NG_HCI_PKT_DM1				0x0008 /* ACL link */
#define NG_HCI_PKT_DH1				0x0010 /* ACL link */
#define NG_HCI_PKT_HV1				0x0020 /* SCO link */
#define NG_HCI_PKT_HV2				0x0040 /* SCO link */
#define NG_HCI_PKT_HV3				0x0080 /* SCO link */
				/* 0x0100 - 0x0200 - reserved for future use */
#define NG_HCI_PKT_DM3				0x0400 /* ACL link */
#define NG_HCI_PKT_DH3				0x0800 /* ACL link */
				/* 0x1000 - 0x2000 - reserved for future use */
#define NG_HCI_PKT_DM5				0x4000 /* ACL link */
#define NG_HCI_PKT_DH5				0x8000 /* ACL link */

/* 
 * Connection modes/Unit modes
 *
 * This is confusing. It means that one of the units change its mode
 * for the specific connection. For example one connection was put on 
 * hold (but i could be wrong :) 
 */

#define NG_HCI_UNIT_MODE_ACTIVE			0x00
#define NG_HCI_UNIT_MODE_HOLD			0x01
#define NG_HCI_UNIT_MODE_SNIFF			0x02
#define NG_HCI_UNIT_MODE_PARK			0x03
/* 0x04 - 0xFF - reserved for future use */

/* Page scan modes */
#define NG_HCI_MANDATORY_PAGE_SCAN_MODE		0x00
#define NG_HCI_OPTIONAL_PAGE_SCAN_MODE1		0x01
#define NG_HCI_OPTIONAL_PAGE_SCAN_MODE2		0x02
#define NG_HCI_OPTIONAL_PAGE_SCAN_MODE3		0x03
/* 0x04 - 0xFF - reserved for future use */

/* Page scan repetition modes */
#define NG_HCI_SCAN_REP_MODE0			0x00
#define NG_HCI_SCAN_REP_MODE1			0x01
#define NG_HCI_SCAN_REP_MODE2			0x02
/* 0x03 - 0xFF - reserved for future use */

/* Page scan period modes */
#define NG_HCI_PAGE_SCAN_PERIOD_MODE0		0x00
#define NG_HCI_PAGE_SCAN_PERIOD_MODE1		0x01
#define NG_HCI_PAGE_SCAN_PERIOD_MODE2		0x02
/* 0x03 - 0xFF - reserved for future use */

/* Scan enable */
#define NG_HCI_NO_SCAN_ENABLE			0x00
#define NG_HCI_INQUIRY_ENABLE_PAGE_DISABLE	0x01
#define NG_HCI_INQUIRY_DISABLE_PAGE_ENABLE	0x02
#define NG_HCI_INQUIRY_ENABLE_PAGE_ENABLE	0x03
/* 0x04 - 0xFF - reserved for future use */

/* Hold mode activities */
#define NG_HCI_HOLD_MODE_NO_CHANGE		0x00
#define NG_HCI_HOLD_MODE_SUSPEND_PAGE_SCAN	0x01
#define NG_HCI_HOLD_MODE_SUSPEND_INQUIRY_SCAN	0x02
#define NG_HCI_HOLD_MODE_SUSPEND_PERIOD_INQUIRY	0x04
/* 0x08 - 0x80 - reserved for future use */

/* Connection roles */
#define NG_HCI_ROLE_MASTER			0x00
#define NG_HCI_ROLE_SLAVE			0x01
/* 0x02 - 0xFF - reserved for future use */

/* Key flags */
#define NG_HCI_USE_SEMI_PERMANENT_LINK_KEYS	0x00
#define NG_HCI_USE_TEMPORARY_LINK_KEY		0x01
/* 0x02 - 0xFF - reserved for future use */

/* Pin types */
#define NG_HCI_PIN_TYPE_VARIABLE		0x00
#define NG_HCI_PIN_TYPE_FIXED			0x01

/* Link key types */
#define NG_HCI_LINK_KEY_TYPE_COMBINATION_KEY	0x00
#define NG_HCI_LINK_KEY_TYPE_LOCAL_UNIT_KEY	0x01
#define NG_HCI_LINK_KEY_TYPE_REMOTE_UNIT_KEY	0x02
/* 0x03 - 0xFF - reserved for future use */

/* Encryption modes */
#define NG_HCI_ENCRYPTION_MODE_NONE		0x00
#define NG_HCI_ENCRYPTION_MODE_P2P		0x01
#define NG_HCI_ENCRYPTION_MODE_ALL		0x02
/* 0x03 - 0xFF - reserved for future use */

/* Quality of service types */
#define NG_HCI_SERVICE_TYPE_NO_TRAFFIC		0x00
#define NG_HCI_SERVICE_TYPE_BEST_EFFORT		0x01
#define NG_HCI_SERVICE_TYPE_GUARANTEED		0x02
/* 0x03 - 0xFF - reserved for future use */

/* Link policy settings */
#define NG_HCI_LINK_POLICY_DISABLE_ALL_LM_MODES	0x0000
#define NG_HCI_LINK_POLICY_ENABLE_ROLE_SWITCH	0x0001 /* Master/Slave switch */
#define NG_HCI_LINK_POLICY_ENABLE_HOLD_MODE	0x0002
#define NG_HCI_LINK_POLICY_ENABLE_SNIFF_MODE	0x0004
#define NG_HCI_LINK_POLICY_ENABLE_PARK_MODE	0x0008
/* 0x0010 - 0x8000 - reserved for future use */

/* Event masks */
#define NG_HCI_EVMSK_ALL			0x00000000ffffffff
#define NG_HCI_EVMSK_NONE			0x0000000000000000
#define NG_HCI_EVMSK_INQUIRY_COMPL		0x0000000000000001
#define NG_HCI_EVMSK_INQUIRY_RESULT		0x0000000000000002
#define NG_HCI_EVMSK_CON_COMPL			0x0000000000000004
#define NG_HCI_EVMSK_CON_REQ			0x0000000000000008
#define NG_HCI_EVMSK_DISCON_COMPL		0x0000000000000010
#define NG_HCI_EVMSK_AUTH_COMPL			0x0000000000000020
#define NG_HCI_EVMSK_REMOTE_NAME_REQ_COMPL	0x0000000000000040
#define NG_HCI_EVMSK_ENCRYPTION_CHANGE		0x0000000000000080
#define NG_HCI_EVMSK_CHANGE_CON_LINK_KEY_COMPL	0x0000000000000100
#define NG_HCI_EVMSK_MASTER_LINK_KEY_COMPL	0x0000000000000200
#define NG_HCI_EVMSK_READ_REMOTE_FEATURES_COMPL	0x0000000000000400
#define NG_HCI_EVMSK_READ_REMOTE_VER_INFO_COMPL	0x0000000000000800
#define NG_HCI_EVMSK_QOS_SETUP_COMPL		0x0000000000001000
#define NG_HCI_EVMSK_COMMAND_COMPL		0x0000000000002000
#define NG_HCI_EVMSK_COMMAND_STATUS		0x0000000000004000
#define NG_HCI_EVMSK_HARDWARE_ERROR		0x0000000000008000
#define NG_HCI_EVMSK_FLUSH_OCCUR		0x0000000000010000
#define NG_HCI_EVMSK_ROLE_CHANGE		0x0000000000020000
#define NG_HCI_EVMSK_NUM_COMPL_PKTS		0x0000000000040000
#define NG_HCI_EVMSK_MODE_CHANGE		0x0000000000080000
#define NG_HCI_EVMSK_RETURN_LINK_KEYS		0x0000000000100000
#define NG_HCI_EVMSK_PIN_CODE_REQ		0x0000000000200000
#define NG_HCI_EVMSK_LINK_KEY_REQ		0x0000000000400000
#define NG_HCI_EVMSK_LINK_KEY_NOTIFICATION	0x0000000000800000
#define NG_HCI_EVMSK_LOOPBACK_COMMAND		0x0000000001000000
#define NG_HCI_EVMSK_DATA_BUFFER_OVERFLOW	0x0000000002000000
#define NG_HCI_EVMSK_MAX_SLOT_CHANGE		0x0000000004000000
#define NG_HCI_EVMSK_READ_CLOCK_OFFSET_COMLETE	0x0000000008000000
#define NG_HCI_EVMSK_CON_PKT_TYPE_CHANGED	0x0000000010000000
#define NG_HCI_EVMSK_QOS_VIOLATION		0x0000000020000000
#define NG_HCI_EVMSK_PAGE_SCAN_MODE_CHANGE	0x0000000040000000
#define NG_HCI_EVMSK_PAGE_SCAN_REP_MODE_CHANGE	0x0000000080000000
/* 0x0000000100000000 - 0x8000000000000000 - reserved for future use */

/* Filter types */
#define NG_HCI_FILTER_TYPE_NONE			0x00
#define NG_HCI_FILTER_TYPE_INQUIRY_RESULT	0x01
#define NG_HCI_FILTER_TYPE_CON_SETUP		0x02
/* 0x03 - 0xFF - reserved for future use */

/* Filter condition types for NG_HCI_FILTER_TYPE_INQUIRY_RESULT */
#define NG_HCI_FILTER_COND_INQUIRY_NEW_UNIT	0x00
#define NG_HCI_FILTER_COND_INQUIRY_UNIT_CLASS	0x01
#define NG_HCI_FILTER_COND_INQUIRY_BDADDR	0x02
/* 0x03 - 0xFF - reserved for future use */

/* Filter condition types for NG_HCI_FILTER_TYPE_CON_SETUP */
#define NG_HCI_FILTER_COND_CON_ANY_UNIT		0x00
#define NG_HCI_FILTER_COND_CON_UNIT_CLASS	0x01
#define NG_HCI_FILTER_COND_CON_BDADDR		0x02
/* 0x03 - 0xFF - reserved for future use */

/* Xmit level types */
#define NG_HCI_XMIT_LEVEL_CURRENT		0x00
#define NG_HCI_XMIT_LEVEL_MAXIMUM		0x01
/* 0x02 - 0xFF - reserved for future use */

/* Host to Host Controller flow control */
#define NG_HCI_H2HC_FLOW_CONTROL_NONE		0x00
#define NG_HCI_H2HC_FLOW_CONTROL_ACL		0x01
#define NG_HCI_H2HC_FLOW_CONTROL_SCO		0x02
#define NG_HCI_H2HC_FLOW_CONTROL_BOTH		0x03	/* ACL and SCO */
/* 0x04 - 0xFF - reserved future use */

/* Country codes */
#define NG_HCI_COUNTRY_CODE_NAM_EUR_JP		0x00
#define NG_HCI_COUNTRY_CODE_FRANCE		0x01
/* 0x02 - 0xFF - reserved future use */

/* Loopback modes */
#define NG_HCI_LOOPBACK_NONE			0x00
#define NG_HCI_LOOPBACK_LOCAL			0x01
#define NG_HCI_LOOPBACK_REMOTE			0x02
/* 0x03 - 0xFF - reserved future use */

/**************************************************************************
 **************************************************************************
 **                 Link level defines, headers and types
 **************************************************************************
 **************************************************************************/

/* 
 * Macro(s) to combine OpCode and extract OGF (OpCode Group Field) 
 * and OCF (OpCode Command Field) from OpCode.
 */

#define NG_HCI_OPCODE(gf,cf)		((((gf) & 0x3f) << 10) | ((cf) & 0x3ff))
#define NG_HCI_OCF(op)			((op) & 0x3ff)
#define NG_HCI_OGF(op)			(((op) >> 10) & 0x3f)

/* 
 * Marco(s) to extract/combine connection handle, BC (Broadcast) and 
 * PB (Packet boundary) flags.
 */

#define NG_HCI_CON_HANDLE(h)		((h) & 0x0fff)
#define NG_HCI_PB_FLAG(h)		(((h) & 0x3000) >> 12)
#define NG_HCI_BC_FLAG(h)		(((h) & 0xc000) >> 14)
#define NG_HCI_MK_CON_HANDLE(h, pb, bc) \
	(((h) & 0x0fff) | (((pb) & 3) << 12) | (((bc) & 3) << 14))

/* PB flag values */
					/* 00 - reserved for future use */
#define	NG_HCI_PACKET_FRAGMENT		0x1 
#define	NG_HCI_PACKET_START		0x2
					/* 11 - reserved for future use */

/* BC flag values */
#define NG_HCI_POINT2POINT		0x0 /* only Host controller to Host */
#define NG_HCI_BROADCAST_ACTIVE		0x1 /* both directions */
#define NG_HCI_BROADCAST_PICONET	0x2 /* both directions */
					/* 11 - reserved for future use */

/* HCI command packet header */
#define NG_HCI_CMD_PKT			0x01
#define NG_HCI_CMD_PKT_SIZE		0xff /* without header */
typedef struct {
<<<<<<< HEAD
	u_int8_t	type;   /* MUST be 0x1 */
	u_int16_t	opcode; /* OpCode */
	u_int8_t	length; /* parameter(s) length in bytes */
} __attribute__ ((packed)) ng_hci_cmd_pkt_t;
=======
	uint8_t		type;	/* MUST be 0x01 */
	uint16_t	opcode; /* OpCode */
	uint8_t		length; /* parameter(s) length in bytes */
} __packed hci_cmd_hdr_t;

#define HCI_CMD_PKT			0x01
#define HCI_CMD_PKT_SIZE		(sizeof(hci_cmd_hdr_t) + 0xff)
>>>>>>> origin/master

/* ACL data packet header */
#define NG_HCI_ACL_DATA_PKT		0x02
#define NG_HCI_ACL_PKT_SIZE		0xffff /* without header */
typedef struct {
<<<<<<< HEAD
	u_int8_t	type;        /* MUST be 0x2 */
	u_int16_t	con_handle;  /* connection handle + PB + BC flags */
	u_int16_t	length;      /* payload length in bytes */
} __attribute__ ((packed)) ng_hci_acldata_pkt_t;
=======
	uint8_t		type;	     /* MUST be 0x02 */
	uint16_t	con_handle;  /* connection handle + PB + BC flags */
	uint16_t	length;      /* payload length in bytes */
} __packed hci_acldata_hdr_t;

#define HCI_ACL_DATA_PKT		0x02
#define HCI_ACL_PKT_SIZE		(sizeof(hci_acldata_hdr_t) + 0xffff)
>>>>>>> origin/master

/* SCO data packet header */
#define NG_HCI_SCO_DATA_PKT		0x03
#define NG_HCI_SCO_PKT_SIZE		0xff /* without header */
typedef struct {
<<<<<<< HEAD
	u_int8_t	type;       /* MUST be 0x3 */
	u_int16_t	con_handle; /* connection handle + reserved bits */
	u_int8_t	length;     /* payload length in bytes */
} __attribute__ ((packed)) ng_hci_scodata_pkt_t;
=======
	uint8_t		type;	    /* MUST be 0x03 */
	uint16_t	con_handle; /* connection handle + reserved bits */
	uint8_t		length;     /* payload length in bytes */
} __packed hci_scodata_hdr_t;

#define HCI_SCO_DATA_PKT		0x03
#define HCI_SCO_PKT_SIZE		(sizeof(hci_scodata_hdr_t) + 0xff)
>>>>>>> origin/master

/* HCI event packet header */
#define NG_HCI_EVENT_PKT		0x04
#define NG_HCI_EVENT_PKT_SIZE		0xff /* without header */
typedef struct {
<<<<<<< HEAD
	u_int8_t	type;   /* MUST be 0x4 */
	u_int8_t	event;  /* event */
	u_int8_t	length; /* parameter(s) length in bytes */
} __attribute__ ((packed)) ng_hci_event_pkt_t;
=======
	uint8_t		type;	/* MUST be 0x04 */
	uint8_t		event;  /* event */
	uint8_t		length; /* parameter(s) length in bytes */
} __packed hci_event_hdr_t;
>>>>>>> origin/master

/* Bluetooth unit address */
typedef struct {
	u_int8_t	b[NG_HCI_BDADDR_SIZE];
} __attribute__ ((packed)) bdaddr_t;
typedef bdaddr_t *	bdaddr_p;

/* Any BD_ADDR. Note: This is actually 7 bytes (count '\0' terminator) */
#define NG_HCI_BDADDR_ANY	((bdaddr_p) "\000\000\000\000\000\000")

/* HCI status return parameter */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status; /* 0x00 - success */
} __attribute__ ((packed)) ng_hci_status_rp;
=======
	uint8_t		status; /* 0x00 - success */
} __packed hci_status_rp;
>>>>>>> origin/master

/**************************************************************************
 **************************************************************************
 **        Upper layer protocol interface. LP_xxx event parameters
 **************************************************************************
 **************************************************************************/

<<<<<<< HEAD
/* Connection Request Event */
#define NGM_HCI_LP_CON_REQ			1  /* Upper -> HCI */
typedef struct {
	u_int16_t	link_type; /* type of connection */
	bdaddr_t	bdaddr;    /* remote unit address */
} ng_hci_lp_con_req_ep;

/*
 * XXX XXX XXX
 *
 * NOTE: This request is not defined by Bluetooth specification, 
 * but i find it useful :)
 */
#define NGM_HCI_LP_DISCON_REQ			2 /* Upper -> HCI */
typedef struct {
	u_int16_t	con_handle; /* connection handle */
	u_int16_t	reason;	    /* reason to disconnect (only low byte) */
} ng_hci_lp_discon_req_ep;
=======
#define HCI_OGF_LINK_CONTROL			0x01

#define HCI_OCF_INQUIRY					0x0001
#define HCI_CMD_INQUIRY					0x0401
typedef struct {
	uint8_t		lap[HCI_LAP_SIZE]; /* LAP */
	uint8_t		inquiry_length;    /* (N x 1.28) sec */
	uint8_t		num_responses;     /* Max. # of responses */
} __packed hci_inquiry_cp;
/* No return parameter(s) */

#define HCI_OCF_INQUIRY_CANCEL				0x0002
#define HCI_CMD_INQUIRY_CANCEL				0x0402
/* No command parameter(s) */
typedef hci_status_rp	hci_inquiry_cancel_rp;

#define HCI_OCF_PERIODIC_INQUIRY			0x0003
#define HCI_CMD_PERIODIC_INQUIRY			0x0403
typedef struct {
	uint16_t	max_period_length; /* Max. and min. amount of time */
	uint16_t	min_period_length; /* between consecutive inquiries */
	uint8_t		lap[HCI_LAP_SIZE]; /* LAP */
	uint8_t		inquiry_length;    /* (inquiry_length * 1.28) sec */
	uint8_t		num_responses;     /* Max. # of responses */
} __packed hci_periodic_inquiry_cp;

typedef hci_status_rp	hci_periodic_inquiry_rp;

#define HCI_OCF_EXIT_PERIODIC_INQUIRY			0x0004
#define HCI_CMD_EXIT_PERIODIC_INQUIRY			0x0404
/* No command parameter(s) */
typedef hci_status_rp	hci_exit_periodic_inquiry_rp;

#define HCI_OCF_CREATE_CON				0x0005
#define HCI_CMD_CREATE_CON				0x0405
typedef struct {
	bdaddr_t	bdaddr;             /* destination address */
	uint16_t	pkt_type;           /* packet type */
	uint8_t		page_scan_rep_mode; /* page scan repetition mode */
	uint8_t		page_scan_mode;     /* reserved - set to 0x00 */
	uint16_t	clock_offset;       /* clock offset */
	uint8_t		accept_role_switch; /* accept role switch? 0x00 == No */
} __packed hci_create_con_cp;
/* No return parameter(s) */

#define HCI_OCF_DISCONNECT				0x0006
#define HCI_CMD_DISCONNECT				0x0406
typedef struct {
	uint16_t	con_handle; /* connection handle */
	uint8_t		reason;     /* reason to disconnect */
} __packed hci_discon_cp;
/* No return parameter(s) */

/* Add SCO Connection is deprecated */
#define HCI_OCF_ADD_SCO_CON				0x0007
#define HCI_CMD_ADD_SCO_CON				0x0407
typedef struct {
	uint16_t	con_handle; /* connection handle */
	uint16_t	pkt_type;   /* packet type */
} __packed hci_add_sco_con_cp;
/* No return parameter(s) */
>>>>>>> origin/master

/* Connection Confirmation Event */
#define NGM_HCI_LP_CON_CFM			3  /* HCI -> Upper */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;     /* 0x00 - success */
	u_int8_t	link_type;  /* link type */
	u_int16_t	con_handle; /* con_handle */
	bdaddr_t	bdaddr;     /* remote unit address */
} ng_hci_lp_con_cfm_ep;
=======
	bdaddr_t	bdaddr;		/* destination address */
} __packed hci_create_con_cancel_cp;
>>>>>>> origin/master

/* Connection Indication Event */
#define NGM_HCI_LP_CON_IND			4  /* HCI -> Upper */
typedef struct {
<<<<<<< HEAD
	u_int8_t	link_type;                 /* link type */
	u_int8_t	uclass[NG_HCI_CLASS_SIZE]; /* unit class */
	bdaddr_t	bdaddr;                    /* remote unit address */
} ng_hci_lp_con_ind_ep;
=======
	uint8_t		status;		/* 0x00 - success */
	bdaddr_t	bdaddr;		/* destination address */
} __packed hci_create_con_cancel_rp;
>>>>>>> origin/master

/* Connection Response Event */
#define NGM_HCI_LP_CON_RSP			5  /* Upper -> HCI */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;    /* 0x00 - accept connection */
	u_int8_t	link_type; /* link type */
	bdaddr_t	bdaddr;    /* remote unit address */
} ng_hci_lp_con_rsp_ep;
=======
	bdaddr_t	bdaddr; /* address of unit to be connected */
	uint8_t		role;   /* connection role */
} __packed hci_accept_con_cp;
/* No return parameter(s) */
>>>>>>> origin/master

/* Disconnection Indication Event */
#define NGM_HCI_LP_DISCON_IND			6  /* HCI -> Upper */
typedef struct {
<<<<<<< HEAD
	u_int8_t	reason;     /* reason to disconnect (only low byte) */
	u_int8_t	link_type;  /* link type */
	u_int16_t	con_handle; /* connection handle */
} ng_hci_lp_discon_ind_ep;
=======
	bdaddr_t	bdaddr; /* remote address */
	uint8_t		reason; /* reason to reject */
} __packed hci_reject_con_cp;
/* No return parameter(s) */
>>>>>>> origin/master

/* QoS Setup Request Event */
#define NGM_HCI_LP_QOS_REQ			7  /* Upper -> HCI */
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle;      /* connection handle */
	u_int8_t	flags;           /* reserved */
	u_int8_t	service_type;    /* service type */
	u_int32_t	token_rate;      /* bytes/sec */
	u_int32_t	peak_bandwidth;  /* bytes/sec */
	u_int32_t	latency;         /* msec */
	u_int32_t	delay_variation; /* msec */
} ng_hci_lp_qos_req_ep;
=======
	bdaddr_t	bdaddr;            /* remote address */
	uint8_t		key[HCI_KEY_SIZE]; /* key */
} __packed hci_link_key_rep_cp;
>>>>>>> origin/master

/* QoS Conformition Event */
#define NGM_HCI_LP_QOS_CFM			8  /* HCI -> Upper */
typedef struct {
<<<<<<< HEAD
	u_int16_t	status;          /* 0x00 - success  (only low byte) */
	u_int16_t	con_handle;      /* connection handle */
} ng_hci_lp_qos_cfm_ep;
=======
	uint8_t		status; /* 0x00 - success */
	bdaddr_t	bdaddr; /* unit address */
} __packed hci_link_key_rep_rp;
>>>>>>> origin/master

/* QoS Violation Indication Event */
#define NGM_HCI_LP_QOS_IND			9  /* HCI -> Upper */
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} ng_hci_lp_qos_ind_ep;

/**************************************************************************
 **************************************************************************
 **                    HCI node command/event parameters
 **************************************************************************
 **************************************************************************/

/* Debug levels */
#define NG_HCI_ALERT_LEVEL		1
#define NG_HCI_ERR_LEVEL		2
#define NG_HCI_WARN_LEVEL		3
#define NG_HCI_INFO_LEVEL		4

/* Unit states */
#define NG_HCI_UNIT_CONNECTED		(1 << 0)
#define NG_HCI_UNIT_INITED		(1 << 1)
#define NG_HCI_UNIT_READY	(NG_HCI_UNIT_CONNECTED|NG_HCI_UNIT_INITED)
#define NG_HCI_UNIT_COMMAND_PENDING	(1 << 2)

/* Connection state */
#define NG_HCI_CON_CLOSED		0 /* connection closed */
#define NG_HCI_CON_W4_LP_CON_RSP	1 /* wait for LP_ConnectRsp */
#define NG_HCI_CON_W4_CONN_COMPLETE	2 /* wait for Connection_Complete evt */
#define NG_HCI_CON_OPEN			3 /* connection open */

/* Get HCI node (unit) state (see states above) */
#define NGM_HCI_NODE_GET_STATE			100  /* HCI -> User */
typedef u_int16_t	ng_hci_node_state_ep;

/* Turn on "inited" bit */
#define NGM_HCI_NODE_INIT			101 /* User -> HCI */
/* No parameters */

/* Get/Set node debug level (see debug levels above) */
#define NGM_HCI_NODE_GET_DEBUG			102 /* HCI -> User */
#define NGM_HCI_NODE_SET_DEBUG			103 /* User -> HCI */
typedef u_int16_t	ng_hci_node_debug_ep;

/* Get node buffer info */
#define NGM_HCI_NODE_GET_BUFFER			104 /* HCI -> User */
typedef struct {
	u_int8_t	cmd_free; /* number of free command packets */
	u_int8_t	sco_size; /* max. size of SCO packet */
	u_int16_t	sco_pkts; /* number of SCO packets */
	u_int16_t	sco_free; /* number of free SCO packets */
	u_int16_t	acl_size; /* max. size of ACL packet */
	u_int16_t	acl_pkts; /* number of ACL packets */
	u_int16_t	acl_free; /* number of free ACL packets */
} ng_hci_node_buffer_ep;

/* Get BDADDR */
#define NGM_HCI_NODE_GET_BDADDR			105 /* HCI -> User */
/* bdaddr_t -- BDADDR */

/* Get features */
#define NGM_HCI_NODE_GET_FEATURES		106 /* HCI -> User */
/* features[NG_HCI_FEATURES_SIZE] -- features */

#define NGM_HCI_NODE_GET_STAT			107 /* HCI -> User */
typedef struct {
	u_int32_t	cmd_sent;   /* number of HCI commands sent */
	u_int32_t	evnt_recv;  /* number of HCI events received */
	u_int32_t	acl_recv;   /* number of ACL packets received */
	u_int32_t	acl_sent;   /* number of ACL packets sent */
	u_int32_t	sco_recv;   /* number of SCO packets received */
	u_int32_t	sco_sent;   /* number of SCO packets sent */
	u_int32_t	bytes_recv; /* total number of bytes received */
	u_int32_t	bytes_sent; /* total number of bytes sent */
} ng_hci_node_stat_ep;

#define NGM_HCI_NODE_RESET_STAT			108 /* User -> HCI */
/* No parameters */

#define NGM_HCI_NODE_FLUSH_NEIGHBOR_CACHE	109 /* User -> HCI */

#define NGM_HCI_NODE_GET_NEIGHBOR_CACHE		110 /* HCI -> User */
typedef struct {
	u_int32_t	num_entries;	/* number of entries */
} ng_hci_node_get_neighbor_cache_ep;

typedef struct {
	u_int16_t	page_scan_rep_mode;             /* page rep scan mode */
	u_int16_t	page_scan_mode;                 /* page scan mode */
	u_int16_t	clock_offset;                   /* clock offset */
	bdaddr_t	bdaddr;                         /* bdaddr */
	u_int8_t	features[NG_HCI_FEATURES_SIZE]; /* features */
} ng_hci_node_neighbor_cache_entry_ep;

#define NG_HCI_MAX_NEIGHBOR_NUM \
	((0xffff - sizeof(ng_hci_node_get_neighbor_cache_ep))/sizeof(ng_hci_node_neighbor_cache_entry_ep))

#define NGM_HCI_NODE_GET_CON_LIST		111 /* HCI -> User */
typedef struct {
	u_int32_t	num_connections; /* number of connections */
} ng_hci_node_con_list_ep;

typedef struct {
	u_int8_t	link_type;       /* ACL or SCO */
	u_int8_t	encryption_mode; /* none, p2p, ... */
	u_int8_t	mode;            /* ACTIVE, HOLD ... */
	u_int8_t	role;            /* MASTER/SLAVE */
	u_int16_t	state;           /* connection state */
	u_int16_t	reserved;        /* place holder */
	u_int16_t	pending;         /* number of pending packets */
	u_int16_t	queue_len;       /* number of packets in queue */
	u_int16_t	con_handle;      /* connection handle */
	bdaddr_t	bdaddr;          /* remote bdaddr */
} ng_hci_node_con_ep;

#define NG_HCI_MAX_CON_NUM \
	((0xffff - sizeof(ng_hci_node_con_list_ep))/sizeof(ng_hci_node_con_ep))

#define NGM_HCI_NODE_UP				112 /* HCI -> Upper */
typedef struct {
	u_int16_t	pkt_size; /* max. ACL/SCO packet size (w/out header) */
	u_int16_t	num_pkts; /* ACL/SCO packet queue size */
	u_int16_t	reserved; /* place holder */
	bdaddr_t	bdaddr;	  /* bdaddr */
} ng_hci_node_up_ep;
=======
	bdaddr_t	bdaddr; /* remote address */
} __packed hci_link_key_neg_rep_cp;

typedef struct {
	uint8_t		status; /* 0x00 - success */
	bdaddr_t	bdaddr; /* unit address */
} __packed hci_link_key_neg_rep_rp;

#define HCI_OCF_PIN_CODE_REP				0x000d
#define HCI_CMD_PIN_CODE_REP				0x040D
typedef struct {
	bdaddr_t	bdaddr;               /* remote address */
	uint8_t		pin_size;             /* pin code length (in bytes) */
	uint8_t		pin[HCI_PIN_SIZE];    /* pin code */
} __packed hci_pin_code_rep_cp;

typedef struct {
	uint8_t		status; /* 0x00 - success */
	bdaddr_t	bdaddr; /* unit address */
} __packed hci_pin_code_rep_rp;
>>>>>>> origin/master

#define NGM_HCI_SYNC_CON_QUEUE			113 /* HCI -> Upper */
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
	u_int16_t	completed;  /* number of completed packets */
} ng_hci_sync_con_queue_ep;

#define NGM_HCI_NODE_GET_LINK_POLICY_SETTINGS_MASK	114 /* HCI -> User */
#define NGM_HCI_NODE_SET_LINK_POLICY_SETTINGS_MASK	115 /* User -> HCI */
typedef u_int16_t	ng_hci_node_link_policy_mask_ep;

#define NGM_HCI_NODE_GET_PACKET_MASK		116 /* HCI -> User */
#define NGM_HCI_NODE_SET_PACKET_MASK		117 /* User -> HCI */
typedef u_int16_t	ng_hci_node_packet_mask_ep;

#define NGM_HCI_NODE_GET_ROLE_SWITCH		118 /* HCI -> User */
#define NGM_HCI_NODE_SET_ROLE_SWITCH		119 /* User -> HCI */
typedef u_int16_t	ng_hci_node_role_switch_ep;

/**************************************************************************
 **************************************************************************
 **             Link control commands and return parameters
 **************************************************************************
 **************************************************************************/

#define NG_HCI_OGF_LINK_CONTROL			0x01 /* OpCode Group Field */
=======
	bdaddr_t	bdaddr; /* remote address */
} __packed hci_pin_code_neg_rep_cp;

typedef struct {
	uint8_t		status; /* 0x00 - success */
	bdaddr_t	bdaddr; /* unit address */
} __packed hci_pin_code_neg_rep_rp;

#define HCI_OCF_CHANGE_CON_PACKET_TYPE			0x000f
#define HCI_CMD_CHANGE_CON_PACKET_TYPE			0x040F
typedef struct {
	uint16_t	con_handle; /* connection handle */
	uint16_t	pkt_type;   /* packet type */
} __packed hci_change_con_pkt_type_cp;
/* No return parameter(s) */

#define HCI_OCF_AUTH_REQ				0x0011
#define HCI_CMD_AUTH_REQ				0x0411
typedef struct {
	uint16_t	con_handle; /* connection handle */
} __packed hci_auth_req_cp;
/* No return parameter(s) */

#define HCI_OCF_SET_CON_ENCRYPTION			0x0013
#define HCI_CMD_SET_CON_ENCRYPTION			0x0413
typedef struct {
	uint16_t	con_handle;        /* connection handle */
	uint8_t		encryption_enable; /* 0x00 - disable, 0x01 - enable */
} __packed hci_set_con_encryption_cp;
/* No return parameter(s) */

#define HCI_OCF_CHANGE_CON_LINK_KEY			0x0015
#define HCI_CMD_CHANGE_CON_LINK_KEY			0x0415
typedef struct {
	uint16_t	con_handle; /* connection handle */
} __packed hci_change_con_link_key_cp;
/* No return parameter(s) */
>>>>>>> origin/master

#define NG_HCI_OCF_INQUIRY			0x0001
typedef struct {
<<<<<<< HEAD
	u_int8_t	lap[NG_HCI_LAP_SIZE]; /* LAP */
	u_int8_t	inquiry_length; /* (N x 1.28) sec */
	u_int8_t	num_responses;  /* Max. # of responses before halted */
} __attribute__ ((packed)) ng_hci_inquiry_cp;
/* No return parameter(s) */

#define NG_HCI_OCF_INQUIRY_CANCEL		0x0002
/* No command parameter(s) */
typedef ng_hci_status_rp	ng_hci_inquiry_cancel_rp;
=======
	uint8_t		key_flag; /* key flag */
} __packed hci_master_link_key_cp;
/* No return parameter(s) */

#define HCI_OCF_REMOTE_NAME_REQ				0x0019
#define HCI_CMD_REMOTE_NAME_REQ				0x0419
typedef struct {
	bdaddr_t	bdaddr;             /* remote address */
	uint8_t		page_scan_rep_mode; /* page scan repetition mode */
	uint8_t		page_scan_mode;     /* page scan mode */
	uint16_t	clock_offset;       /* clock offset */
} __packed hci_remote_name_req_cp;
/* No return parameter(s) */
>>>>>>> origin/master

#define NG_HCI_OCF_PERIODIC_INQUIRY		0x0003
typedef struct {
<<<<<<< HEAD
	u_int16_t	max_period_length; /* Max. and min. amount of time */
	u_int16_t	min_period_length; /* between consecutive inquiries */
	u_int8_t	lap[NG_HCI_LAP_SIZE]; /* LAP */
	u_int8_t	inquiry_length;    /* (inquiry_length * 1.28) sec */
	u_int8_t	num_responses;     /* Max. # of responses */
} __attribute__ ((packed)) ng_hci_periodic_inquiry_cp;

typedef ng_hci_status_rp	ng_hci_periodic_inquiry_rp;
	
#define NG_HCI_OCF_EXIT_PERIODIC_INQUIRY	0x0004
/* No command parameter(s) */
typedef ng_hci_status_rp	ng_hci_exit_periodic_inquiry_rp;
=======
	bdaddr_t	bdaddr;             /* remote address */
} __packed hci_remote_name_req_cancel_cp;

typedef struct {
	uint8_t		status;		/* 0x00 - success */
	bdaddr_t	bdaddr;         /* remote address */
} __packed hci_remote_name_req_cancel_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_CREATE_CON			0x0005
typedef struct {
<<<<<<< HEAD
	bdaddr_t	bdaddr;             /* destination address */
	u_int16_t	pkt_type;           /* packet type */
	u_int8_t	page_scan_rep_mode; /* page scan repetition mode */
	u_int8_t	page_scan_mode;     /* page scan mode */
	u_int16_t	clock_offset;       /* clock offset */
	u_int8_t	accept_role_switch; /* accept role switch? 0x00 - no */
} __attribute__ ((packed)) ng_hci_create_con_cp;
=======
	uint16_t	con_handle; /* connection handle */
} __packed hci_read_remote_features_cp;
>>>>>>> origin/master
/* No return parameter(s) */

#define NG_HCI_OCF_DISCON			0x0006
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
	u_int8_t	reason;     /* reason to disconnect */
} __attribute__ ((packed)) ng_hci_discon_cp;
=======
	uint16_t	con_handle;	/* connection handle */
	uint8_t		page;		/* page number */
} __packed hci_read_remote_extended_features_cp;
>>>>>>> origin/master
/* No return parameter(s) */

#define NG_HCI_OCF_ADD_SCO_CON			0x0007
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
	u_int16_t	pkt_type;   /* packet type */
} __attribute__ ((packed)) ng_hci_add_sco_con_cp;
=======
	uint16_t	con_handle; /* connection handle */
} __packed hci_read_remote_ver_info_cp;
>>>>>>> origin/master
/* No return parameter(s) */

#define NG_HCI_OCF_ACCEPT_CON			0x0009
typedef struct {
<<<<<<< HEAD
	bdaddr_t	bdaddr; /* address of unit to be connected */
	u_int8_t	role;   /* connection role */
} __attribute__ ((packed)) ng_hci_accept_con_cp;
/* No return parameter(s) */

#define NG_HCI_OCF_REJECT_CON			0x000a
typedef struct {
	bdaddr_t	bdaddr; /* remote address */
	u_int8_t	reason; /* reason to reject */
} __attribute__ ((packed)) ng_hci_reject_con_cp;
=======
	uint16_t	con_handle; /* connection handle */
} __packed hci_read_clock_offset_cp;
/* No return parameter(s) */

#define HCI_OCF_READ_LMP_HANDLE				0x0020
#define HCI_CMD_READ_LMP_HANDLE				0x0420
typedef struct {
	uint16_t	con_handle; /* connection handle */
} __packed hci_read_lmp_handle_cp;

typedef struct {
	uint8_t		status;	    /* 0x00 - success */
	uint16_t	con_handle; /* connection handle */
	uint8_t		lmp_handle; /* LMP handle */
	uint32_t	reserved;   /* reserved */
} __packed hci_read_lmp_handle_rp;

#define HCI_OCF_SETUP_SCO_CON				0x0028
#define HCI_CMD_SETUP_SCO_CON				0x0428
typedef struct {
	uint16_t	con_handle;	/* connection handle */
	uint32_t	tx_bandwidth;	/* transmit bandwidth */
	uint32_t	rx_bandwidth;	/* receive bandwidth */
	uint16_t	latency;	/* maximum latency */
	uint16_t	voice;		/* voice setting */
	uint8_t		rt_effort;	/* retransmission effort */
	uint16_t	pkt_type;	/* packet types */
} __packed hci_setup_sco_con_cp;
/* No return parameter(s) */

#define HCI_OCF_ACCEPT_SCO_CON_REQ			0x0029
#define HCI_CMD_ACCEPT_SCO_CON_REQ			0x0429
typedef struct {
	bdaddr_t	bdaddr;		/* remote address */
	uint32_t	tx_bandwidth;	/* transmit bandwidth */
	uint32_t	rx_bandwidth;	/* receive bandwidth */
	uint16_t	latency;	/* maximum latency */
	uint16_t	content;	/* voice setting */
	uint8_t		rt_effort;	/* retransmission effort */
	uint16_t	pkt_type;	/* packet types */
} __packed hci_accept_sco_con_req_cp;
>>>>>>> origin/master
/* No return parameter(s) */

#define NG_HCI_OCF_LINK_KEY_REP			0x000b
typedef struct {
<<<<<<< HEAD
	bdaddr_t	bdaddr;               /* remote address */
	u_int8_t	key[NG_HCI_KEY_SIZE]; /* key */
} __attribute__ ((packed)) ng_hci_link_key_rep_cp;
=======
	bdaddr_t	bdaddr;		/* remote address */
	uint8_t		reason;		/* reject error code */
} __packed hci_reject_sco_con_req_cp;
/* No return parameter(s) */
>>>>>>> origin/master

typedef struct {
<<<<<<< HEAD
	u_int8_t	status; /* 0x00 - success */
	bdaddr_t	bdaddr; /* unit address */
} __attribute__ ((packed)) ng_hci_link_key_rep_rp;
=======
	bdaddr_t	bdaddr;		/* remote address */
	uint8_t		io_cap;		/* IO capability */
	uint8_t		oob_data;	/* OOB data present */
	uint8_t		auth_req;	/* auth requirements */
} __packed hci_io_capability_rep_cp;
>>>>>>> origin/master

#define NG_HCI_OCF_LINK_KEY_NEG_REP		0x000c
typedef struct {
<<<<<<< HEAD
	bdaddr_t	bdaddr; /* remote address */
} __attribute__ ((packed)) ng_hci_link_key_neg_rep_cp;
=======
	uint8_t		status;		/* 0x00 - success */
	bdaddr_t	bdaddr;		/* remote address */
} __packed hci_io_capability_rep_rp;
>>>>>>> origin/master

typedef struct {
<<<<<<< HEAD
	u_int8_t	status; /* 0x00 - success */
	bdaddr_t	bdaddr; /* unit address */
} __attribute__ ((packed)) ng_hci_link_key_neg_rep_rp;
=======
	bdaddr_t	bdaddr;		/* remote address */
} __packed hci_user_confirm_rep_cp;
>>>>>>> origin/master

#define NG_HCI_OCF_PIN_CODE_REP			0x000d
typedef struct {
<<<<<<< HEAD
	bdaddr_t	bdaddr;               /* remote address */
	u_int8_t	pin_size;             /* pin code length (in bytes) */
	u_int8_t	pin[NG_HCI_PIN_SIZE]; /* pin code */
} __attribute__ ((packed)) ng_hci_pin_code_rep_cp;
=======
	uint8_t		status;		/* 0x00 - success */
	bdaddr_t	bdaddr;		/* remote address */
} __packed hci_user_confirm_rep_rp;
>>>>>>> origin/master

typedef struct {
<<<<<<< HEAD
	u_int8_t	status; /* 0x00 - success */
	bdaddr_t	bdaddr; /* unit address */
} __attribute__ ((packed)) ng_hci_pin_code_rep_rp;
=======
	bdaddr_t	bdaddr;		/* remote address */
} __packed hci_user_confirm_neg_rep_cp;
>>>>>>> origin/master

#define NG_HCI_OCF_PIN_CODE_NEG_REP		0x000e
typedef struct {
<<<<<<< HEAD
	bdaddr_t	bdaddr;  /* remote address */
} __attribute__ ((packed)) ng_hci_pin_code_neg_rep_cp;
=======
	uint8_t		status;		/* 0x00 - success */
	bdaddr_t	bdaddr;		/* remote address */
} __packed hci_user_confirm_neg_rep_rp;
>>>>>>> origin/master

typedef struct {
<<<<<<< HEAD
	u_int8_t	status; /* 0x00 - success */
	bdaddr_t	bdaddr; /* unit address */
} __attribute__ ((packed)) ng_hci_pin_code_neg_rep_rp;
=======
	bdaddr_t	bdaddr;		/* remote address */
	uint32_t	value;		/* 000000 - 999999 */
} __packed hci_user_passkey_rep_cp;
>>>>>>> origin/master

#define NG_HCI_OCF_CHANGE_CON_PKT_TYPE		0x000f
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
	u_int16_t	pkt_type;   /* packet type */
} __attribute__ ((packed)) ng_hci_change_con_pkt_type_cp;
/* No return parameter(s) */
=======
	uint8_t		status;		/* 0x00 - success */
	bdaddr_t	bdaddr;		/* remote address */
} __packed hci_user_passkey_rep_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_AUTH_REQ			0x0011
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_auth_req_cp;
/* No return parameter(s) */
=======
	bdaddr_t	bdaddr;		/* remote address */
} __packed hci_user_passkey_neg_rep_cp;
>>>>>>> origin/master

#define NG_HCI_OCF_SET_CON_ENCRYPTION		0x0013
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle;        /* connection handle */
	u_int8_t	encryption_enable; /* 0x00 - disable, 0x01 - enable */
} __attribute__ ((packed)) ng_hci_set_con_encryption_cp;
/* No return parameter(s) */
=======
	uint8_t		status;		/* 0x00 - success */
	bdaddr_t	bdaddr;		/* remote address */
} __packed hci_user_passkey_neg_rep_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_CHANGE_CON_LINK_KEY		0x0015
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_change_con_link_key_cp;
/* No return parameter(s) */
=======
	bdaddr_t	bdaddr;		/* remote address */
	uint8_t		c[16];		/* pairing hash */
	uint8_t		r[16];		/* pairing randomizer */
} __packed hci_user_oob_data_rep_cp;
>>>>>>> origin/master

#define NG_HCI_OCF_MASTER_LINK_KEY		0x0017
typedef struct {
<<<<<<< HEAD
	u_int8_t	key_flag; /* key flag */
} __attribute__ ((packed)) ng_hci_master_link_key_cp;
/* No return parameter(s) */
=======
	uint8_t		status;		/* 0x00 - success */
	bdaddr_t	bdaddr;		/* remote address */
} __packed hci_user_oob_data_rep_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_REMOTE_NAME_REQ		0x0019
typedef struct {
<<<<<<< HEAD
	bdaddr_t	bdaddr;             /* remote address */
	u_int8_t	page_scan_rep_mode; /* page scan repetition mode */
	u_int8_t	page_scan_mode;     /* page scan mode */
	u_int16_t	clock_offset;       /* clock offset */
} __attribute__ ((packed)) ng_hci_remote_name_req_cp;
/* No return parameter(s) */
=======
	bdaddr_t	bdaddr;		/* remote address */
} __packed hci_user_oob_data_neg_rep_cp;
>>>>>>> origin/master

#define NG_HCI_OCF_READ_REMOTE_FEATURES		0x001b
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_read_remote_features_cp;
/* No return parameter(s) */
=======
	uint8_t		status;		/* 0x00 - success */
	bdaddr_t	bdaddr;		/* remote address */
} __packed hci_user_oob_data_neg_rep_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_READ_REMOTE_VER_INFO		0x001d
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_read_remote_ver_info_cp;
/* No return parameter(s) */
=======
	bdaddr_t	bdaddr;		/* remote address */
	uint8_t		reason;		/* error code */
} __packed hci_io_capability_neg_rep_cp;
>>>>>>> origin/master

#define NG_HCI_OCF_READ_CLOCK_OFFSET		 0x001f
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_read_clock_offset_cp;
/* No return parameter(s) */
=======
	uint8_t		status;		/* 0x00 - success */
	bdaddr_t	bdaddr;		/* remote address */
} __packed hci_io_capability_neg_rep_rp;
>>>>>>> origin/master

/**************************************************************************
 **************************************************************************
 **        Link policy commands and return parameters
 **************************************************************************
 **************************************************************************/

#define NG_HCI_OGF_LINK_POLICY			0x02 /* OpCode Group Field */

#define NG_HCI_OCF_HOLD_MODE			0x0001
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle;   /* connection handle */
	u_int16_t	max_interval; /* (max_interval * 0.625) msec */
	u_int16_t	min_interval; /* (max_interval * 0.625) msec */
} __attribute__ ((packed)) ng_hci_hold_mode_cp;
=======
	uint16_t	con_handle;   /* connection handle */
	uint16_t	max_interval; /* (max_interval * 0.625) msec */
	uint16_t	min_interval; /* (max_interval * 0.625) msec */
} __packed hci_hold_mode_cp;
>>>>>>> origin/master
/* No return parameter(s) */

#define NG_HCI_OCF_SNIFF_MODE			0x0003
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle;   /* connection handle */
	u_int16_t	max_interval; /* (max_interval * 0.625) msec */
	u_int16_t	min_interval; /* (max_interval * 0.625) msec */
	u_int16_t	attempt;      /* (2 * attempt - 1) * 0.625 msec */
	u_int16_t	timeout;      /* (2 * attempt - 1) * 0.625 msec */
} __attribute__ ((packed)) ng_hci_sniff_mode_cp;
=======
	uint16_t	con_handle;   /* connection handle */
	uint16_t	max_interval; /* (max_interval * 0.625) msec */
	uint16_t	min_interval; /* (max_interval * 0.625) msec */
	uint16_t	attempt;      /* (2 * attempt - 1) * 0.625 msec */
	uint16_t	timeout;      /* (2 * attempt - 1) * 0.625 msec */
} __packed hci_sniff_mode_cp;
>>>>>>> origin/master
/* No return parameter(s) */

#define NG_HCI_OCF_EXIT_SNIFF_MODE		0x0004
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_exit_sniff_mode_cp;
=======
	uint16_t	con_handle; /* connection handle */
} __packed hci_exit_sniff_mode_cp;
>>>>>>> origin/master
/* No return parameter(s) */

#define NG_HCI_OCF_PARK_MODE			0x0005
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle;   /* connection handle */
	u_int16_t	max_interval; /* (max_interval * 0.625) msec */
	u_int16_t	min_interval; /* (max_interval * 0.625) msec */
} __attribute__ ((packed)) ng_hci_park_mode_cp;
=======
	uint16_t	con_handle;   /* connection handle */
	uint16_t	max_interval; /* (max_interval * 0.625) msec */
	uint16_t	min_interval; /* (max_interval * 0.625) msec */
} __packed hci_park_mode_cp;
>>>>>>> origin/master
/* No return parameter(s) */

#define NG_HCI_OCF_EXIT_PARK_MODE		0x0006
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_exit_park_mode_cp;
/* No return parameter(s) */

#define NG_HCI_OCF_QOS_SETUP			0x0007
typedef struct {
	u_int16_t	con_handle;      /* connection handle */
	u_int8_t	flags;           /* reserved for future use */
	u_int8_t	service_type;    /* service type */
	u_int32_t	token_rate;      /* bytes per second */
	u_int32_t	peak_bandwidth;  /* bytes per second */
	u_int32_t	latency;         /* microseconds */
	u_int32_t	delay_variation; /* microseconds */
} __attribute__ ((packed)) ng_hci_qos_setup_cp;
=======
	uint16_t	con_handle; /* connection handle */
} __packed hci_exit_park_mode_cp;
/* No return parameter(s) */

#define HCI_OCF_QOS_SETUP				0x0007
#define HCI_CMD_QOS_SETUP				0x0807
typedef struct {
	uint16_t	con_handle;      /* connection handle */
	uint8_t		flags;           /* reserved for future use */
	uint8_t		service_type;    /* service type */
	uint32_t	token_rate;      /* bytes per second */
	uint32_t	peak_bandwidth;  /* bytes per second */
	uint32_t	latency;         /* microseconds */
	uint32_t	delay_variation; /* microseconds */
} __packed hci_qos_setup_cp;
>>>>>>> origin/master
/* No return parameter(s) */

#define NG_HCI_OCF_ROLE_DISCOVERY		0x0009
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_role_discovery_cp;

typedef struct {
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* connection handle */
	u_int8_t	role;       /* role for the connection handle */
} __attribute__ ((packed)) ng_hci_role_discovery_rp;
=======
	uint16_t	con_handle; /* connection handle */
} __packed hci_role_discovery_cp;

typedef struct {
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* connection handle */
	uint8_t		role;       /* role for the connection handle */
} __packed hci_role_discovery_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_SWITCH_ROLE			0x000b
typedef struct {
	bdaddr_t	bdaddr; /* remote address */
<<<<<<< HEAD
	u_int8_t	role;   /* new local role */
} __attribute__ ((packed)) ng_hci_switch_role_cp;
=======
	uint8_t		role;   /* new local role */
} __packed hci_switch_role_cp;
>>>>>>> origin/master
/* No return parameter(s) */

#define NG_HCI_OCF_READ_LINK_POLICY_SETTINGS	0x000c
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_read_link_policy_settings_cp;
	
typedef struct {
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* connection handle */
	u_int16_t	settings;   /* link policy settings */
} __attribute__ ((packed)) ng_hci_read_link_policy_settings_rp;
=======
	uint16_t	con_handle; /* connection handle */
} __packed hci_read_link_policy_settings_cp;

typedef struct {
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* connection handle */
	uint16_t	settings;   /* link policy settings */
} __packed hci_read_link_policy_settings_rp;

#define HCI_OCF_WRITE_LINK_POLICY_SETTINGS		0x000d
#define HCI_CMD_WRITE_LINK_POLICY_SETTINGS		0x080D
typedef struct {
	uint16_t	con_handle; /* connection handle */
	uint16_t	settings;   /* link policy settings */
} __packed hci_write_link_policy_settings_cp;

typedef struct {
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* connection handle */
} __packed hci_write_link_policy_settings_rp;

#define HCI_OCF_READ_DEFAULT_LINK_POLICY_SETTINGS	0x000e
#define HCI_CMD_READ_DEFAULT_LINK_POLICY_SETTINGS	0x080E
/* No command parameter(s) */
typedef struct {
	uint8_t		status;     /* 0x00 - success */
	uint16_t	settings;   /* link policy settings */
} __packed hci_read_default_link_policy_settings_rp;

#define HCI_OCF_WRITE_DEFAULT_LINK_POLICY_SETTINGS	0x000f
#define HCI_CMD_WRITE_DEFAULT_LINK_POLICY_SETTINGS	0x080F
typedef struct {
	uint16_t	settings;   /* link policy settings */
} __packed hci_write_default_link_policy_settings_cp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_LINK_POLICY_SETTINGS	0x000d
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
	u_int16_t	settings;   /* link policy settings */
} __attribute__ ((packed)) ng_hci_write_link_policy_settings_cp;
=======
	uint16_t	con_handle;	/* connection handle */
	uint8_t		flags;		/* reserved */
	uint8_t		flow_direction;
	uint8_t		service_type;
	uint32_t	token_rate;
	uint32_t	token_bucket;
	uint32_t	peak_bandwidth;
	uint32_t	latency;
} __packed hci_flow_specification_cp;
/* No return parameter(s) */
>>>>>>> origin/master

typedef struct {
<<<<<<< HEAD
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_write_link_policy_settings_rp;
=======
	uint16_t	con_handle;	/* connection handle */
	uint16_t	max_latency;
	uint16_t	max_timeout;	/* max remote timeout */
	uint16_t	min_timeout;	/* min local timeout */
} __packed hci_sniff_subrating_cp;

typedef struct {
	uint8_t		status;		/* 0x00 - success */
	uint16_t	con_handle;	/* connection handle */
} __packed hci_sniff_subrating_rp;
>>>>>>> origin/master

/**************************************************************************
 **************************************************************************
 **   Host controller and baseband commands and return parameters 
 **************************************************************************
 **************************************************************************/

#define NG_HCI_OGF_HC_BASEBAND			0x03 /* OpCode Group Field */

#define NG_HCI_OCF_SET_EVENT_MASK		0x0001
typedef struct {
<<<<<<< HEAD
	u_int8_t	event_mask[NG_HCI_EVENT_MASK_SIZE]; /* event_mask */
} __attribute__ ((packed)) ng_hci_set_event_mask_cp;
=======
	uint8_t		event_mask[HCI_EVENT_MASK_SIZE]; /* event_mask */
} __packed hci_set_event_mask_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_set_event_mask_rp;

#define NG_HCI_OCF_RESET			0x0003
/* No command parameter(s) */
typedef ng_hci_status_rp	ng_hci_reset_rp;

#define NG_HCI_OCF_SET_EVENT_FILTER		0x0005
typedef struct {
<<<<<<< HEAD
	u_int8_t	filter_type;           /* filter type */
	u_int8_t	filter_condition_type; /* filter condition type */
	u_int8_t	condition[0];          /* conditions - variable size */
} __attribute__ ((packed)) ng_hci_set_event_filter_cp;
=======
	uint8_t		filter_type;           /* filter type */
	uint8_t		filter_condition_type; /* filter condition type */
/* variable size condition
	uint8_t		condition[]; -- conditions */
} __packed hci_set_event_filter_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_set_event_filter_rp;

#define NG_HCI_OCF_FLUSH			0x0008
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_flush_cp;

typedef struct {
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_flush_rp;
=======
	uint16_t	con_handle; /* connection handle */
} __packed hci_flush_cp;

typedef struct {
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* connection handle */
} __packed hci_flush_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_READ_PIN_TYPE		0x0009
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;   /* 0x00 - success */
	u_int8_t	pin_type; /* PIN type */
} __attribute__ ((packed)) ng_hci_read_pin_type_rp;
=======
	uint8_t		status;   /* 0x00 - success */
	uint8_t		pin_type; /* PIN type */
} __packed hci_read_pin_type_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_PIN_TYPE		0x000a
typedef struct {
<<<<<<< HEAD
	u_int8_t	pin_type; /* PIN type */
} __attribute__ ((packed)) ng_hci_write_pin_type_cp;
=======
	uint8_t		pin_type; /* PIN type */
} __packed hci_write_pin_type_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_write_pin_type_rp;

#define NG_HCI_OCF_CREATE_NEW_UNIT_KEY		0x000b
/* No command parameter(s) */
typedef ng_hci_status_rp	ng_hci_create_new_unit_key_rp;

#define NG_HCI_OCF_READ_STORED_LINK_KEY		0x000d
typedef struct {
	bdaddr_t	bdaddr;   /* address */
<<<<<<< HEAD
	u_int8_t	read_all; /* read all keys? 0x01 - yes */
} __attribute__ ((packed)) ng_hci_read_stored_link_key_cp;

typedef struct {
	u_int8_t	status;        /* 0x00 - success */
	u_int16_t	max_num_keys;  /* Max. number of keys */
	u_int16_t	num_keys_read; /* Number of stored keys */
} __attribute__ ((packed)) ng_hci_read_stored_link_key_rp;
=======
	uint8_t		read_all; /* read all keys? 0x01 - yes */
} __packed hci_read_stored_link_key_cp;

typedef struct {
	uint8_t		status;        /* 0x00 - success */
	uint16_t	max_num_keys;  /* Max. number of keys */
	uint16_t	num_keys_read; /* Number of stored keys */
} __packed hci_read_stored_link_key_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_STORED_LINK_KEY	0x0011
typedef struct {
<<<<<<< HEAD
	u_int8_t	num_keys_write; /* # of keys to write */
/* these are repeated "num_keys_write" times 
	bdaddr_t	bdaddr;                --- remote address(es)
	u_int8_t	key[NG_HCI_KEY_SIZE];  --- key(s) */
} __attribute__ ((packed)) ng_hci_write_stored_link_key_cp;

typedef struct {
	u_int8_t	status;           /* 0x00 - success */
	u_int8_t	num_keys_written; /* # of keys successfully written */
} __attribute__ ((packed)) ng_hci_write_stored_link_key_rp;
=======
	uint8_t		num_keys_write; /* # of keys to write */
/* these are repeated "num_keys_write" times
	bdaddr_t	bdaddr;             --- remote address(es)
	uint8_t		key[HCI_KEY_SIZE];  --- key(s) */
} __packed hci_write_stored_link_key_cp;

typedef struct {
	uint8_t		status;           /* 0x00 - success */
	uint8_t		num_keys_written; /* # of keys successfully written */
} __packed hci_write_stored_link_key_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_DELETE_STORED_LINK_KEY	0x0012
typedef struct {
	bdaddr_t	bdaddr;     /* address */
<<<<<<< HEAD
	u_int8_t	delete_all; /* delete all keys? 0x01 - yes */
} __attribute__ ((packed)) ng_hci_delete_stored_link_key_cp;

typedef struct {
	u_int8_t	status;           /* 0x00 - success */
	u_int16_t	num_keys_deleted; /* Number of keys deleted */
} __attribute__ ((packed)) ng_hci_delete_stored_link_key_rp;
=======
	uint8_t		delete_all; /* delete all keys? 0x01 - yes */
} __packed hci_delete_stored_link_key_cp;

typedef struct {
	uint8_t		status;           /* 0x00 - success */
	uint16_t	num_keys_deleted; /* Number of keys deleted */
} __packed hci_delete_stored_link_key_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_CHANGE_LOCAL_NAME		0x0013
typedef struct {
<<<<<<< HEAD
	char		name[NG_HCI_UNIT_NAME_SIZE]; /* new unit name */
} __attribute__ ((packed)) ng_hci_change_local_name_cp;
=======
	char		name[HCI_UNIT_NAME_SIZE]; /* new unit name */
} __packed hci_write_local_name_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_change_local_name_rp;

#define NG_HCI_OCF_READ_LOCAL_NAME		0x0014
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;  /* 0x00 - success */
	char		name[NG_HCI_UNIT_NAME_SIZE]; /* unit name */
} __attribute__ ((packed)) ng_hci_read_local_name_rp;
=======
	uint8_t		status;                   /* 0x00 - success */
	char		name[HCI_UNIT_NAME_SIZE]; /* unit name */
} __packed hci_read_local_name_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_READ_CON_ACCEPT_TIMO		0x0015
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;  /* 0x00 - success */
	u_int16_t	timeout; /* (timeout * 0.625) msec */
} __attribute__ ((packed)) ng_hci_read_con_accept_timo_rp;
=======
	uint8_t		status;  /* 0x00 - success */
	uint16_t	timeout; /* (timeout * 0.625) msec */
} __packed hci_read_con_accept_timeout_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_CON_ACCEPT_TIMO	0x0016
typedef struct {
<<<<<<< HEAD
	u_int16_t	timeout; /* (timeout * 0.625) msec */
} __attribute__ ((packed)) ng_hci_write_con_accept_timo_cp;
=======
	uint16_t	timeout; /* (timeout * 0.625) msec */
} __packed hci_write_con_accept_timeout_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_write_con_accept_timo_rp;

#define NG_HCI_OCF_READ_PAGE_TIMO		0x0017
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;  /* 0x00 - success */
	u_int16_t	timeout; /* (timeout * 0.625) msec */
} __attribute__ ((packed)) ng_hci_read_page_timo_rp;
=======
	uint8_t		status;  /* 0x00 - success */
	uint16_t	timeout; /* (timeout * 0.625) msec */
} __packed hci_read_page_timeout_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_PAGE_TIMO		0x0018
typedef struct {
<<<<<<< HEAD
	u_int16_t	timeout; /* (timeout * 0.625) msec */
} __attribute__ ((packed)) ng_hci_write_page_timo_cp;
=======
	uint16_t	timeout; /* (timeout * 0.625) msec */
} __packed hci_write_page_timeout_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_write_page_timo_rp;

#define NG_HCI_OCF_READ_SCAN_ENABLE		0x0019
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;      /* 0x00 - success */
	u_int8_t	scan_enable; /* Scan enable */
} __attribute__ ((packed)) ng_hci_read_scan_enable_rp;
=======
	uint8_t		status;      /* 0x00 - success */
	uint8_t		scan_enable; /* Scan enable */
} __packed hci_read_scan_enable_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_SCAN_ENABLE		0x001a
typedef struct {
<<<<<<< HEAD
	u_int8_t	scan_enable; /* Scan enable */
} __attribute__ ((packed)) ng_hci_write_scan_enable_cp;
=======
	uint8_t		scan_enable; /* Scan enable */
} __packed hci_write_scan_enable_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_write_scan_enable_rp;

#define NG_HCI_OCF_READ_PAGE_SCAN_ACTIVITY	0x001b
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;             /* 0x00 - success */
	u_int16_t	page_scan_interval; /* interval * 0.625 msec */
	u_int16_t	page_scan_window;   /* window * 0.625 msec */
} __attribute__ ((packed)) ng_hci_read_page_scan_activity_rp;
=======
	uint8_t		status;             /* 0x00 - success */
	uint16_t	page_scan_interval; /* interval * 0.625 msec */
	uint16_t	page_scan_window;   /* window * 0.625 msec */
} __packed hci_read_page_scan_activity_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_PAGE_SCAN_ACTIVITY	0x001c
typedef struct {
<<<<<<< HEAD
	u_int16_t	page_scan_interval; /* interval * 0.625 msec */
	u_int16_t	page_scan_window;   /* window * 0.625 msec */
} __attribute__ ((packed)) ng_hci_write_page_scan_activity_cp;
=======
	uint16_t	page_scan_interval; /* interval * 0.625 msec */
	uint16_t	page_scan_window;   /* window * 0.625 msec */
} __packed hci_write_page_scan_activity_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_write_page_scan_activity_rp;

#define NG_HCI_OCF_READ_INQUIRY_SCAN_ACTIVITY	0x001d
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;                /* 0x00 - success */
	u_int16_t	inquiry_scan_interval; /* interval * 0.625 msec */
	u_int16_t	inquiry_scan_window;   /* window * 0.625 msec */
} __attribute__ ((packed)) ng_hci_read_inquiry_scan_activity_rp;
=======
	uint8_t		status;                /* 0x00 - success */
	uint16_t	inquiry_scan_interval; /* interval * 0.625 msec */
	uint16_t	inquiry_scan_window;   /* window * 0.625 msec */
} __packed hci_read_inquiry_scan_activity_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_INQUIRY_SCAN_ACTIVITY	0x001e
typedef struct {
<<<<<<< HEAD
	u_int16_t	inquiry_scan_interval; /* interval * 0.625 msec */
	u_int16_t	inquiry_scan_window;   /* window * 0.625 msec */
} __attribute__ ((packed)) ng_hci_write_inquiry_scan_activity_cp;
=======
	uint16_t	inquiry_scan_interval; /* interval * 0.625 msec */
	uint16_t	inquiry_scan_window;   /* window * 0.625 msec */
} __packed hci_write_inquiry_scan_activity_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_write_inquiry_scan_activity_rp;

#define NG_HCI_OCF_READ_AUTH_ENABLE		0x001f
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;      /* 0x00 - success */
	u_int8_t	auth_enable; /* 0x01 - enabled */
} __attribute__ ((packed)) ng_hci_read_auth_enable_rp;
=======
	uint8_t		status;      /* 0x00 - success */
	uint8_t		auth_enable; /* 0x01 - enabled */
} __packed hci_read_auth_enable_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_AUTH_ENABLE		0x0020
typedef struct {
<<<<<<< HEAD
	u_int8_t	auth_enable; /* 0x01 - enabled */
} __attribute__ ((packed)) ng_hci_write_auth_enable_cp;
=======
	uint8_t		auth_enable; /* 0x01 - enabled */
} __packed hci_write_auth_enable_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_write_auth_enable_rp;

#define NG_HCI_OCF_READ_ENCRYPTION_MODE		0x0021
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;          /* 0x00 - success */
	u_int8_t	encryption_mode; /* encryption mode */
} __attribute__ ((packed)) ng_hci_read_encryption_mode_rp;
=======
	uint8_t		status;          /* 0x00 - success */
	uint8_t		encryption_mode; /* encryption mode */
} __packed hci_read_encryption_mode_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_ENCRYPTION_MODE	0x0022
typedef struct {
<<<<<<< HEAD
	u_int8_t	encryption_mode; /* encryption mode */
} __attribute__ ((packed)) ng_hci_write_encryption_mode_cp;
=======
	uint8_t		encryption_mode; /* encryption mode */
} __packed hci_write_encryption_mode_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_write_encryption_mode_rp;

#define NG_HCI_OCF_READ_UNIT_CLASS		0x0023
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;                    /* 0x00 - success */
	u_int8_t	uclass[NG_HCI_CLASS_SIZE]; /* unit class */
} __attribute__ ((packed)) ng_hci_read_unit_class_rp;
=======
	uint8_t		status;                 /* 0x00 - success */
	uint8_t		uclass[HCI_CLASS_SIZE]; /* unit class */
} __packed hci_read_unit_class_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_UNIT_CLASS		0x0024
typedef struct {
<<<<<<< HEAD
	u_int8_t	uclass[NG_HCI_CLASS_SIZE]; /* unit class */
} __attribute__ ((packed)) ng_hci_write_unit_class_cp;
=======
	uint8_t		uclass[HCI_CLASS_SIZE]; /* unit class */
} __packed hci_write_unit_class_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_write_unit_class_rp;

#define NG_HCI_OCF_READ_VOICE_SETTINGS		0x0025
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;   /* 0x00 - success */
	u_int16_t	settings; /* voice settings */
} __attribute__ ((packed)) ng_hci_read_voice_settings_rp;
=======
	uint8_t		status;   /* 0x00 - success */
	uint16_t	settings; /* voice settings */
} __packed hci_read_voice_setting_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_VOICE_SETTINGS		0x0026
typedef struct {
<<<<<<< HEAD
	u_int16_t	settings; /* voice settings */
} __attribute__ ((packed)) ng_hci_write_voice_settings_cp;
=======
	uint16_t	settings; /* voice settings */
} __packed hci_write_voice_setting_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_write_voice_settings_rp;

#define NG_HCI_OCF_READ_AUTO_FLUSH_TIMO		0x0027
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_read_auto_flush_timo_cp;

typedef struct {
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* connection handle */
	u_int16_t	timeout;    /* 0x00 - no flush, timeout * 0.625 msec */
} __attribute__ ((packed)) ng_hci_read_auto_flush_timo_rp;
	
#define NG_HCI_OCF_WRITE_AUTO_FLUSH_TIMO	0x0028
typedef struct {
	u_int16_t	con_handle; /* connection handle */
	u_int16_t	timeout;    /* 0x00 - no flush, timeout * 0.625 msec */
} __attribute__ ((packed)) ng_hci_write_auto_flush_timo_cp;

typedef struct {
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_write_auto_flush_timo_rp;
=======
	uint16_t	con_handle; /* connection handle */
} __packed hci_read_auto_flush_timeout_cp;

typedef struct {
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* connection handle */
	uint16_t	timeout;    /* 0x00 - no flush, timeout * 0.625 msec */
} __packed hci_read_auto_flush_timeout_rp;

#define HCI_OCF_WRITE_AUTO_FLUSH_TIMEOUT		0x0028
#define HCI_CMD_WRITE_AUTO_FLUSH_TIMEOUT		0x0C28
typedef struct {
	uint16_t	con_handle; /* connection handle */
	uint16_t	timeout;    /* 0x00 - no flush, timeout * 0.625 msec */
} __packed hci_write_auto_flush_timeout_cp;

typedef struct {
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* connection handle */
} __packed hci_write_auto_flush_timeout_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_READ_NUM_BROADCAST_RETRANS	0x0029
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;  /* 0x00 - success */
	u_int8_t	counter; /* number of broadcast retransmissions */
} __attribute__ ((packed)) ng_hci_read_num_broadcast_retrans_rp;
=======
	uint8_t		status;  /* 0x00 - success */
	uint8_t		counter; /* number of broadcast retransmissions */
} __packed hci_read_num_broadcast_retrans_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_NUM_BROADCAST_RETRANS	0x002a
typedef struct {
<<<<<<< HEAD
	u_int8_t	counter; /* number of broadcast retransmissions */
} __attribute__ ((packed)) ng_hci_write_num_broadcast_retrans_cp;
=======
	uint8_t		counter; /* number of broadcast retransmissions */
} __packed hci_write_num_broadcast_retrans_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_write_num_broadcast_retrans_rp;

#define NG_HCI_OCF_READ_HOLD_MODE_ACTIVITY	0x002b
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;             /* 0x00 - success */
	u_int8_t	hold_mode_activity; /* Hold mode activities */
} __attribute__ ((packed)) ng_hci_read_hold_mode_activity_rp;
=======
	uint8_t		status;             /* 0x00 - success */
	uint8_t		hold_mode_activity; /* Hold mode activities */
} __packed hci_read_hold_mode_activity_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_HOLD_MODE_ACTIVITY	0x002c
typedef struct {
<<<<<<< HEAD
	u_int8_t	hold_mode_activity; /* Hold mode activities */
} __attribute__ ((packed)) ng_hci_write_hold_mode_activity_cp;
=======
	uint8_t		hold_mode_activity; /* Hold mode activities */
} __packed hci_write_hold_mode_activity_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_write_hold_mode_activity_rp;

#define NG_HCI_OCF_READ_XMIT_LEVEL		0x002d
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
	u_int8_t	type;       /* Xmit level type */
} __attribute__ ((packed)) ng_hci_read_xmit_level_cp;
=======
	uint16_t	con_handle; /* connection handle */
	uint8_t		type;       /* Xmit level type */
} __packed hci_read_xmit_level_cp;
>>>>>>> origin/master

typedef struct {
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* connection handle */
	char		level;      /* -30 <= level <= 30 dBm */
<<<<<<< HEAD
} __attribute__ ((packed)) ng_hci_read_xmit_level_rp;
=======
} __packed hci_read_xmit_level_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_READ_SCO_FLOW_CONTROL	0x002e
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;       /* 0x00 - success */
	u_int8_t	flow_control; /* 0x00 - disabled */
} __attribute__ ((packed)) ng_hci_read_sco_flow_control_rp;
=======
	uint8_t		status;       /* 0x00 - success */
	uint8_t		flow_control; /* 0x00 - disabled */
} __packed hci_read_sco_flow_control_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_SCO_FLOW_CONTROL	0x002f
typedef struct {
<<<<<<< HEAD
	u_int8_t	flow_control; /* 0x00 - disabled */
} __attribute__ ((packed)) ng_hci_write_sco_flow_control_cp;
=======
	uint8_t		flow_control; /* 0x00 - disabled */
} __packed hci_write_sco_flow_control_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_write_sco_flow_control_rp;

#define NG_HCI_OCF_H2HC_FLOW_CONTROL		0x0031
typedef struct {
<<<<<<< HEAD
	u_int8_t	h2hc_flow; /* Host to Host controller flow control */
} __attribute__ ((packed)) ng_hci_h2hc_flow_control_cp;
=======
	uint8_t		hc2h_flow; /* Host Controller to Host flow control */
} __packed hci_hc2h_flow_control_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_h2hc_flow_control_rp;

#define NG_HCI_OCF_HOST_BUFFER_SIZE		0x0033
typedef struct {
<<<<<<< HEAD
	u_int16_t	max_acl_size; /* Max. size of ACL packet (bytes) */
	u_int8_t	max_sco_size; /* Max. size of SCO packet (bytes) */
	u_int16_t	num_acl_pkt;  /* Max. number of ACL packets */
	u_int16_t	num_sco_pkt;  /* Max. number of SCO packets */
} __attribute__ ((packed)) ng_hci_host_buffer_size_cp;
=======
	uint16_t	max_acl_size; /* Max. size of ACL packet (bytes) */
	uint8_t		max_sco_size; /* Max. size of SCO packet (bytes) */
	uint16_t	num_acl_pkts;  /* Max. number of ACL packets */
	uint16_t	num_sco_pkts;  /* Max. number of SCO packets */
} __packed hci_host_buffer_size_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_host_buffer_size_rp;

#define NG_HCI_OCF_HOST_NUM_COMPL_PKTS		0x0035
typedef struct {
	u_int8_t	num_con_handles; /* # of connection handles */
/* these are repeated "num_con_handles" times
<<<<<<< HEAD
	u_int16_t	con_handle; --- connection handle(s)
	u_int16_t	compl_pkt;  --- # of completed packets */
} __attribute__ ((packed)) ng_hci_host_num_compl_pkts_cp;
/* No return parameter(s) */

#define NG_HCI_OCF_READ_LINK_SUPERVISION_TIMO	0x0036
typedef struct {
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_read_link_supervision_timo_cp;
=======
	uint16_t	con_handle;    --- connection handle(s)
	uint16_t	compl_pkts;    --- # of completed packets */
} __packed hci_host_num_compl_pkts_cp;
/* No return parameter(s) */

#define HCI_OCF_READ_LINK_SUPERVISION_TIMEOUT		0x0036
#define HCI_CMD_READ_LINK_SUPERVISION_TIMEOUT		0x0C36
typedef struct {
	uint16_t	con_handle; /* connection handle */
} __packed hci_read_link_supervision_timeout_cp;

typedef struct {
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* connection handle */
	uint16_t	timeout;    /* Link supervision timeout * 0.625 msec */
} __packed hci_read_link_supervision_timeout_rp;

#define HCI_OCF_WRITE_LINK_SUPERVISION_TIMEOUT		0x0037
#define HCI_CMD_WRITE_LINK_SUPERVISION_TIMEOUT		0x0C37
typedef struct {
	uint16_t	con_handle; /* connection handle */
	uint16_t	timeout;    /* Link supervision timeout * 0.625 msec */
} __packed hci_write_link_supervision_timeout_cp;

typedef struct {
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* connection handle */
} __packed hci_write_link_supervision_timeout_rp;

#define HCI_OCF_READ_NUM_SUPPORTED_IAC			0x0038
#define HCI_CMD_READ_NUM_SUPPORTED_IAC			0x0C38
/* No command parameter(s) */
typedef struct {
	uint8_t		status;  /* 0x00 - success */
	uint8_t		num_iac; /* # of supported IAC during scan */
} __packed hci_read_num_supported_iac_rp;

#define HCI_OCF_READ_IAC_LAP				0x0039
#define HCI_CMD_READ_IAC_LAP				0x0C39
/* No command parameter(s) */
typedef struct {
	uint8_t		status;  /* 0x00 - success */
	uint8_t		num_iac; /* # of IAC */
/* these are repeated "num_iac" times
	uint8_t		laps[HCI_LAP_SIZE]; --- LAPs */
} __packed hci_read_iac_lap_rp;
>>>>>>> origin/master

typedef struct {
<<<<<<< HEAD
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* connection handle */
	u_int16_t	timeout;    /* Link supervision timeout * 0.625 msec */
} __attribute__ ((packed)) ng_hci_read_link_supervision_timo_rp;
=======
	uint8_t		num_iac; /* # of IAC */
/* these are repeated "num_iac" times
	uint8_t		laps[HCI_LAP_SIZE]; --- LAPs */
} __packed hci_write_iac_lap_cp;

typedef hci_status_rp	hci_write_iac_lap_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_LINK_SUPERVISION_TIMO	0x0037
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
	u_int16_t	timeout;    /* Link supervision timeout * 0.625 msec */
} __attribute__ ((packed)) ng_hci_write_link_supervision_timo_cp;
=======
	uint8_t		status;                /* 0x00 - success */
	uint8_t		page_scan_period_mode; /* Page scan period mode */
} __packed hci_read_page_scan_period_rp;
>>>>>>> origin/master

typedef struct {
<<<<<<< HEAD
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_write_link_supervision_timo_rp;
=======
	uint8_t		page_scan_period_mode; /* Page scan period mode */
} __packed hci_write_page_scan_period_cp;

typedef hci_status_rp	hci_write_page_scan_period_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_READ_SUPPORTED_IAC_NUM	0x0038
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;  /* 0x00 - success */
	u_int8_t	num_iac; /* # of supported IAC during scan */
} __attribute__ ((packed)) ng_hci_read_supported_iac_num_rp;

#define NG_HCI_OCF_READ_IAC_LAP			0x0039
=======
	uint8_t		status;         /* 0x00 - success */
	uint8_t		page_scan_mode; /* Page scan mode */
} __packed hci_read_page_scan_rp;

/* Write Page Scan Mode is deprecated */
#define HCI_OCF_WRITE_PAGE_SCAN				0x003e
#define HCI_CMD_WRITE_PAGE_SCAN				0x0C3E
typedef struct {
	uint8_t		page_scan_mode; /* Page scan mode */
} __packed hci_write_page_scan_cp;

typedef hci_status_rp	hci_write_page_scan_rp;

#define HCI_OCF_SET_AFH_CLASSIFICATION			0x003f
#define HCI_CMD_SET_AFH_CLASSIFICATION			0x0C3F
typedef struct {
	uint8_t		classification[10];
} __packed hci_set_afh_classification_cp;

typedef hci_status_rp	hci_set_afh_classification_rp;

#define HCI_OCF_READ_INQUIRY_SCAN_TYPE			0x0042
#define HCI_CMD_READ_INQUIRY_SCAN_TYPE			0x0C42
>>>>>>> origin/master
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;  /* 0x00 - success */
	u_int8_t	num_iac; /* # of IAC */
/* these are repeated "num_iac" times 
	u_int8_t	laps[NG_HCI_LAP_SIZE]; --- LAPs */
} __attribute__ ((packed)) ng_hci_read_iac_lap_rp;
=======
	uint8_t		status;		/* 0x00 - success */
	uint8_t		type;		/* inquiry scan type */
} __packed hci_read_inquiry_scan_type_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_IAC_LAP		0x003a
typedef struct {
<<<<<<< HEAD
	u_int8_t	num_iac; /* # of IAC */
/* these are repeated "num_iac" times 
	u_int8_t	laps[NG_HCI_LAP_SIZE]; --- LAPs */
} __attribute__ ((packed)) ng_hci_write_iac_lap_cp;
=======
	uint8_t		type;		/* inquiry scan type */
} __packed hci_write_inquiry_scan_type_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_write_iac_lap_rp;

#define NG_HCI_OCF_READ_PAGE_SCAN_PERIOD	0x003b
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;                /* 0x00 - success */
	u_int8_t	page_scan_period_mode; /* Page scan period mode */
} __attribute__ ((packed)) ng_hci_read_page_scan_period_rp;
=======
	uint8_t		status;		/* 0x00 - success */
	uint8_t		mode;		/* inquiry mode */
} __packed hci_read_inquiry_mode_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_PAGE_SCAN_PERIOD	0x003c
typedef struct {
<<<<<<< HEAD
	u_int8_t	page_scan_period_mode; /* Page scan period mode */
} __attribute__ ((packed)) ng_hci_write_page_scan_period_cp;
=======
	uint8_t		mode;		/* inquiry mode */
} __packed hci_write_inquiry_mode_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_write_page_scan_period_rp;

#define NG_HCI_OCF_READ_PAGE_SCAN		0x003d
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;         /* 0x00 - success */
	u_int8_t	page_scan_mode; /* Page scan mode */
} __attribute__ ((packed)) ng_hci_read_page_scan_rp;
=======
	uint8_t		status;		/* 0x00 - success */
	uint8_t		type;		/* page scan type */
} __packed hci_read_page_scan_type_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_PAGE_SCAN		0x003e
typedef struct {
<<<<<<< HEAD
	u_int8_t	page_scan_mode; /* Page scan mode */
} __attribute__ ((packed)) ng_hci_write_page_scan_cp;

typedef ng_hci_status_rp	ng_hci_write_page_scan_rp;
=======
	uint8_t		type;		/* page scan type */
} __packed hci_write_page_scan_type_cp;

typedef hci_status_rp	hci_write_page_scan_type_rp;

#define HCI_OCF_READ_AFH_ASSESSMENT			0x0048
#define HCI_CMD_READ_AFH_ASSESSMENT			0x0C48
/* No command parameter(s) */

typedef struct {
	uint8_t		status;		/* 0x00 - success */
	uint8_t		mode;		/* assessment mode */
} __packed hci_read_afh_assessment_rp;

#define HCI_OCF_WRITE_AFH_ASSESSMENT			0x0049
#define HCI_CMD_WRITE_AFH_ASSESSMENT			0x0C49
typedef struct {
	uint8_t		mode;		/* assessment mode */
} __packed hci_write_afh_assessment_cp;

typedef hci_status_rp	hci_write_afh_assessment_rp;

#define HCI_OCF_READ_EXTENDED_INQUIRY_RSP		0x0051
#define HCI_CMD_READ_EXTENDED_INQUIRY_RSP		0x0C51
/* No command parameter(s) */

typedef struct {
	uint8_t		status;		/* 0x00 - success */
	uint8_t		fec_required;
	uint8_t		response[240];
} __packed hci_read_extended_inquiry_rsp_rp;

#define HCI_OCF_WRITE_EXTENDED_INQUIRY_RSP		0x0052
#define HCI_CMD_WRITE_EXTENDED_INQUIRY_RSP		0x0C52
typedef struct {
	uint8_t		fec_required;
	uint8_t		response[240];
} __packed hci_write_extended_inquiry_rsp_cp;

typedef hci_status_rp	hci_write_extended_inquiry_rsp_rp;

#define HCI_OCF_REFRESH_ENCRYPTION_KEY			0x0053
#define HCI_CMD_REFRESH_ENCRYPTION_KEY			0x0C53
typedef struct {
	uint16_t	con_handle;	/* connection handle */
} __packed hci_refresh_encryption_key_cp;

typedef hci_status_rp	hci_refresh_encryption_key_rp;

#define HCI_OCF_READ_SIMPLE_PAIRING_MODE		0x0055
#define HCI_CMD_READ_SIMPLE_PAIRING_MODE		0x0C55
/* No command parameter(s) */

typedef struct {
	uint8_t		status;		/* 0x00 - success */
	uint8_t		mode;		/* simple pairing mode */
} __packed hci_read_simple_pairing_mode_rp;

#define HCI_OCF_WRITE_SIMPLE_PAIRING_MODE		0x0056
#define HCI_CMD_WRITE_SIMPLE_PAIRING_MODE		0x0C56
typedef struct {
	uint8_t		mode;		/* simple pairing mode */
} __packed hci_write_simple_pairing_mode_cp;

typedef hci_status_rp	hci_write_simple_pairing_mode_rp;

#define HCI_OCF_READ_LOCAL_OOB_DATA			0x0057
#define HCI_CMD_READ_LOCAL_OOB_DATA			0x0C57
/* No command parameter(s) */

typedef struct {
	uint8_t		status;		/* 0x00 - success */
	uint8_t		c[16];		/* pairing hash */
	uint8_t		r[16];		/* pairing randomizer */
} __packed hci_read_local_oob_data_rp;

#define HCI_OCF_READ_INQUIRY_RSP_XMIT_POWER		0x0058
#define HCI_CMD_READ_INQUIRY_RSP_XMIT_POWER		0x0C58
/* No command parameter(s) */

typedef struct {
	uint8_t		status;		/* 0x00 - success */
	int8_t		power;		/* TX power */
} __packed hci_read_inquiry_rsp_xmit_power_rp;

#define HCI_OCF_WRITE_INQUIRY_RSP_XMIT_POWER		0x0059
#define HCI_CMD_WRITE_INQUIRY_RSP_XMIT_POWER		0x0C59
typedef struct {
	int8_t		power;		/* TX power */
} __packed hci_write_inquiry_rsp_xmit_power_cp;

typedef hci_status_rp	hci_write_inquiry_rsp_xmit_power_rp;

#define HCI_OCF_READ_DEFAULT_ERRDATA_REPORTING		0x005A
#define HCI_CMD_READ_DEFAULT_ERRDATA_REPORTING		0x0C5A
/* No command parameter(s) */

typedef struct {
	uint8_t		status;		/* 0x00 - success */
	uint8_t		reporting;	/* erroneous data reporting */
} __packed hci_read_default_errdata_reporting_rp;

#define HCI_OCF_WRITE_DEFAULT_ERRDATA_REPORTING		0x005B
#define HCI_CMD_WRITE_DEFAULT_ERRDATA_REPORTING		0x0C5B
typedef struct {
	uint8_t		reporting;	/* erroneous data reporting */
} __packed hci_write_default_errdata_reporting_cp;

typedef hci_status_rp	hci_write_default_errdata_reporting_rp;

#define HCI_OCF_ENHANCED_FLUSH				0x005F
#define HCI_CMD_ENHANCED_FLUSH				0x0C5F
typedef struct {
	uint16_t	con_handle;	/* connection handle */
	uint8_t		packet_type;
} __packed hci_enhanced_flush_cp;

/* No response parameter(s) */

#define HCI_OCF_SEND_KEYPRESS_NOTIFICATION		0x0060
#define HCI_CMD_SEND_KEYPRESS_NOTIFICATION		0x0C60
typedef struct {
	bdaddr_t	bdaddr;		/* remote address */
	uint8_t		type;		/* notification type */
} __packed hci_send_keypress_notification_cp;

typedef struct {
	uint8_t		status;		/* 0x00 - success */
	bdaddr_t	bdaddr;		/* remote address */
} __packed hci_send_keypress_notification_rp;
>>>>>>> origin/master

/**************************************************************************
 **************************************************************************
 **           Informational commands and return parameters 
 **     All commands in this category do not accept any parameters
 **************************************************************************
 **************************************************************************/

<<<<<<< HEAD
#define NG_HCI_OGF_INFO				0x04 /* OpCode Group Field */
=======
#define HCI_OGF_INFO				0x04

#define HCI_OCF_READ_LOCAL_VER				0x0001
#define HCI_CMD_READ_LOCAL_VER				0x1001
/* No command parameter(s) */
typedef struct {
	uint8_t		status;         /* 0x00 - success */
	uint8_t		hci_version;    /* HCI version */
	uint16_t	hci_revision;   /* HCI revision */
	uint8_t		lmp_version;    /* LMP version */
	uint16_t	manufacturer;   /* Hardware manufacturer name */
	uint16_t	lmp_subversion; /* LMP sub-version */
} __packed hci_read_local_ver_rp;

#define HCI_OCF_READ_LOCAL_COMMANDS			0x0002
#define HCI_CMD_READ_LOCAL_COMMANDS			0x1002
/* No command parameter(s) */
typedef struct {
	uint8_t		status;		/* 0x00 - success */
	uint8_t		commands[HCI_COMMANDS_SIZE];	/* opcode bitmask */
} __packed hci_read_local_commands_rp;

#define HCI_OCF_READ_LOCAL_FEATURES			0x0003
#define HCI_CMD_READ_LOCAL_FEATURES			0x1003
/* No command parameter(s) */
typedef struct {
	uint8_t		status;                      /* 0x00 - success */
	uint8_t		features[HCI_FEATURES_SIZE]; /* LMP features bitmsk*/
} __packed hci_read_local_features_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_READ_LOCAL_VER		0x0001
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;         /* 0x00 - success */
	u_int8_t	hci_version;    /* HCI version */
	u_int16_t	hci_revision;   /* HCI revision */
	u_int8_t	lmp_version;    /* LMP version */
	u_int16_t	manufacturer;   /* Hardware manufacturer name */
	u_int16_t	lmp_subversion; /* LMP sub-version */
} __attribute__ ((packed)) ng_hci_read_local_ver_rp;
=======
	uint8_t		page;		/* page number */
} __packed hci_read_local_extended_features_cp;
>>>>>>> origin/master

#define NG_HCI_OCF_READ_LOCAL_FEATURES		0x0003
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;                         /* 0x00 - success */
	u_int8_t	features[NG_HCI_FEATURES_SIZE]; /* LMP features bitmsk*/
} __attribute__ ((packed)) ng_hci_read_local_features_rp;
=======
	uint8_t		status;		/* 0x00 - success */
	uint8_t		page;		/* page number */
	uint8_t		max_page;	/* maximum page number */
	uint8_t		features[HCI_FEATURES_SIZE];	/* LMP features */
} __packed hci_read_local_extended_features_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_READ_BUFFER_SIZE		0x0005
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;       /* 0x00 - success */
	u_int16_t	max_acl_size; /* Max. size of ACL packet (bytes) */
	u_int8_t	max_sco_size; /* Max. size of SCO packet (bytes) */
	u_int16_t	num_acl_pkt;  /* Max. number of ACL packets */
	u_int16_t	num_sco_pkt;  /* Max. number of SCO packets */
} __attribute__ ((packed)) ng_hci_read_buffer_size_rp;
=======
	uint8_t		status;       /* 0x00 - success */
	uint16_t	max_acl_size; /* Max. size of ACL packet (bytes) */
	uint8_t		max_sco_size; /* Max. size of SCO packet (bytes) */
	uint16_t	num_acl_pkts;  /* Max. number of ACL packets */
	uint16_t	num_sco_pkts;  /* Max. number of SCO packets */
} __packed hci_read_buffer_size_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_READ_COUNTRY_CODE		0x0007
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;       /* 0x00 - success */
	u_int8_t	country_code; /* 0x00 - NAM, EUR, JP; 0x01 - France */
} __attribute__ ((packed)) ng_hci_read_country_code_rp;
=======
	uint8_t		status;       /* 0x00 - success */
	uint8_t		country_code; /* 0x00 - NAM, EUR, JP; 0x01 - France */
} __packed hci_read_country_code_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_READ_BDADDR			0x0009
typedef struct {
	u_int8_t	status; /* 0x00 - success */
	bdaddr_t	bdaddr; /* unit address */
<<<<<<< HEAD
} __attribute__ ((packed)) ng_hci_read_bdaddr_rp;
=======
} __packed hci_read_bdaddr_rp;
>>>>>>> origin/master

/**************************************************************************
 **************************************************************************
 **            Status commands and return parameters 
 **************************************************************************
 **************************************************************************/

#define NG_HCI_OGF_STATUS			0x05 /* OpCode Group Field */

#define NG_HCI_OCF_READ_FAILED_CONTACT_CNTR	0x0001
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_read_failed_contact_cntr_cp;

typedef struct {
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* connection handle */
	u_int16_t	counter;    /* number of consecutive failed contacts */
} __attribute__ ((packed)) ng_hci_read_failed_contact_cntr_rp;
=======
	uint16_t	con_handle; /* connection handle */
} __packed hci_read_failed_contact_cntr_cp;

typedef struct {
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* connection handle */
	uint16_t	counter;    /* number of consecutive failed contacts */
} __packed hci_read_failed_contact_cntr_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_RESET_FAILED_CONTACT_CNTR	0x0002
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_reset_failed_contact_cntr_cp;

typedef struct {
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_reset_failed_contact_cntr_rp;
=======
	uint16_t	con_handle; /* connection handle */
} __packed hci_reset_failed_contact_cntr_cp;

typedef struct {
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* connection handle */
} __packed hci_reset_failed_contact_cntr_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_GET_LINK_QUALITY		0x0003
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_get_link_quality_cp;

typedef struct {
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* connection handle */
	u_int8_t	quality;    /* higher value means better quality */
} __attribute__ ((packed)) ng_hci_get_link_quality_rp;
=======
	uint16_t	con_handle; /* connection handle */
} __packed hci_read_link_quality_cp;

typedef struct {
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* connection handle */
	uint8_t		quality;    /* higher value means better quality */
} __packed hci_read_link_quality_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_READ_RSSI			0x0005
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_read_rssi_cp;
=======
	uint16_t	con_handle; /* connection handle */
} __packed hci_read_rssi_cp;
>>>>>>> origin/master

typedef struct {
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* connection handle */
	char		rssi;       /* -127 <= rssi <= 127 dB */
<<<<<<< HEAD
} __attribute__ ((packed)) ng_hci_read_rssi_rp;
=======
} __packed hci_read_rssi_rp;

#define HCI_OCF_READ_AFH_CHANNEL_MAP			0x0006
#define HCI_CMD_READ_AFH_CHANNEL_MAP			0x1406
typedef struct {
	uint16_t	con_handle; /* connection handle */
} __packed hci_read_afh_channel_map_cp;

typedef struct {
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* connection handle */
	uint8_t		mode;       /* AFH mode */
	uint8_t		map[10];    /* AFH Channel Map */
} __packed hci_read_afh_channel_map_rp;

#define HCI_OCF_READ_CLOCK				0x0007
#define HCI_CMD_READ_CLOCK				0x1407
typedef struct {
	uint16_t	con_handle;	/* connection handle */
	uint8_t		clock;		/* which clock */
} __packed hci_read_clock_cp;

typedef struct {
	uint8_t		status;		/* 0x00 - success */
	uint16_t	con_handle;	/* connection handle */
	uint32_t	clock;		/* clock value */
	uint16_t	accuracy;	/* clock accuracy */
} __packed hci_read_clock_rp;

>>>>>>> origin/master

/**************************************************************************
 **************************************************************************
 **             Testing commands and return parameters 
 **************************************************************************
 **************************************************************************/

#define NG_HCI_OGF_TESTING			0x06 /* OpCode Group Field */

#define NG_HCI_OCF_READ_LOOPBACK_MODE		0x0001
/* No command parameter(s) */
typedef struct {
<<<<<<< HEAD
	u_int8_t	status; /* 0x00 - success */
	u_int8_t	lbmode; /* loopback mode */
} __attribute__ ((packed)) ng_hci_read_loopback_mode_rp;
=======
	uint8_t		status; /* 0x00 - success */
	uint8_t		lbmode; /* loopback mode */
} __packed hci_read_loopback_mode_rp;
>>>>>>> origin/master

#define NG_HCI_OCF_WRITE_LOOPBACK_MODE		0x0002
typedef struct {
<<<<<<< HEAD
	u_int8_t	lbmode; /* loopback mode */
} __attribute__ ((packed)) ng_hci_write_loopback_mode_cp;
=======
	uint8_t		lbmode; /* loopback mode */
} __packed hci_write_loopback_mode_cp;
>>>>>>> origin/master

typedef ng_hci_status_rp	ng_hci_write_loopback_mode_rp;

#define NG_HCI_OCF_ENABLE_UNIT_UNDER_TEST	0x0003
/* No command parameter(s) */
<<<<<<< HEAD
typedef ng_hci_status_rp	ng_hci_enable_unit_under_test_rp;
=======
typedef hci_status_rp	hci_enable_unit_under_test_rp;

#define HCI_OCF_WRITE_SIMPLE_PAIRING_DEBUG_MODE		0x0004
#define HCI_CMD_WRITE_SIMPLE_PAIRING_DEBUG_MODE		0x1804
typedef struct {
	uint8_t		mode;	/* simple pairing debug mode */
} __packed hci_write_simple_pairing_debug_mode_cp;

typedef hci_status_rp	hci_write_simple_pairing_debug_mode_rp;
>>>>>>> origin/master

/**************************************************************************
 **************************************************************************
 **                Special HCI OpCode group field values
 **************************************************************************
 **************************************************************************/

#define NG_HCI_OGF_BT_LOGO			0x3e	

#define NG_HCI_OGF_VENDOR			0x3f

/**************************************************************************
 **************************************************************************
 **                         Events and event parameters
 **************************************************************************
 **************************************************************************/

#define NG_HCI_EVENT_INQUIRY_COMPL		0x01
typedef struct {
<<<<<<< HEAD
	u_int8_t	status; /* 0x00 - success */
} __attribute__ ((packed)) ng_hci_inquiry_compl_ep;
=======
	uint8_t		status; /* 0x00 - success */
} __packed hci_inquiry_compl_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_INQUIRY_RESULT		0x02
typedef struct {
<<<<<<< HEAD
	u_int8_t	num_responses;      /* number of responses */
/*	ng_hci_inquiry_response[num_responses]   -- see below */
} __attribute__ ((packed)) ng_hci_inquiry_result_ep;

typedef struct {
	bdaddr_t	bdaddr;                   /* unit address */
	u_int8_t	page_scan_rep_mode;       /* page scan rep. mode */
	u_int8_t	page_scan_period_mode;    /* page scan period mode */
	u_int8_t	page_scan_mode;           /* page scan mode */
	u_int8_t	uclass[NG_HCI_CLASS_SIZE]; /* unit class */
	u_int16_t	clock_offset;             /* clock offset */
} __attribute__ ((packed)) ng_hci_inquiry_response;
=======
	uint8_t		num_responses;      /* number of responses */
/*	hci_inquiry_response[num_responses]   -- see below */
} __packed hci_inquiry_result_ep;

typedef struct {
	bdaddr_t	bdaddr;                   /* unit address */
	uint8_t		page_scan_rep_mode;       /* page scan rep. mode */
	uint8_t		page_scan_period_mode;    /* page scan period mode */
	uint8_t		page_scan_mode;           /* page scan mode */
	uint8_t		uclass[HCI_CLASS_SIZE];   /* unit class */
	uint16_t	clock_offset;             /* clock offset */
} __packed hci_inquiry_response;
>>>>>>> origin/master

#define NG_HCI_EVENT_CON_COMPL			0x03
typedef struct {
	u_int8_t	status;          /* 0x00 - success */
	u_int16_t	con_handle;      /* Connection handle */
	bdaddr_t	bdaddr;          /* remote unit address */
<<<<<<< HEAD
	u_int8_t	link_type;       /* Link type */
	u_int8_t	encryption_mode; /* Encryption mode */
} __attribute__ ((packed)) ng_hci_con_compl_ep;
=======
	uint8_t		link_type;       /* Link type */
	uint8_t		encryption_mode; /* Encryption mode */
} __packed hci_con_compl_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_CON_REQ			0x04
typedef struct {
<<<<<<< HEAD
	bdaddr_t	bdaddr;                    /* remote unit address */
	u_int8_t	uclass[NG_HCI_CLASS_SIZE]; /* remote unit class */
	u_int8_t	link_type;                 /* link type */
} __attribute__ ((packed)) ng_hci_con_req_ep;
=======
	bdaddr_t	bdaddr;                 /* remote unit address */
	uint8_t		uclass[HCI_CLASS_SIZE]; /* remote unit class */
	uint8_t		link_type;              /* link type */
} __packed hci_con_req_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_DISCON_COMPL		0x05
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* connection handle */
	u_int8_t	reason;     /* reason to disconnect */
} __attribute__ ((packed)) ng_hci_discon_compl_ep;
=======
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* connection handle */
	uint8_t		reason;     /* reason to disconnect */
} __packed hci_discon_compl_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_AUTH_COMPL			0x06
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_auth_compl_ep;
=======
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* connection handle */
} __packed hci_auth_compl_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_REMOTE_NAME_REQ_COMPL	0x7
typedef struct {
<<<<<<< HEAD
	u_int8_t	status; /* 0x00 - success */
	bdaddr_t	bdaddr; /* remote unit address */
	char		name[NG_HCI_UNIT_NAME_SIZE]; /* remote unit name */
} __attribute__ ((packed)) ng_hci_remote_name_req_compl_ep;
=======
	uint8_t		status;                   /* 0x00 - success */
	bdaddr_t	bdaddr;                   /* remote unit address */
	char		name[HCI_UNIT_NAME_SIZE]; /* remote unit name */
} __packed hci_remote_name_req_compl_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_ENCRYPTION_CHANGE		0x08
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;            /* 0x00 - success */
	u_int16_t	con_handle;        /* Connection handle */
	u_int8_t	encryption_enable; /* 0x00 - disable */
} __attribute__ ((packed)) ng_hci_encryption_change_ep;
=======
	uint8_t		status;            /* 0x00 - success */
	uint16_t	con_handle;        /* Connection handle */
	uint8_t		encryption_enable; /* 0x00 - disable */
} __packed hci_encryption_change_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_CHANGE_CON_LINK_KEY_COMPL	0x09
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* Connection handle */
} __attribute__ ((packed)) ng_hci_change_con_link_key_compl_ep;
=======
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* Connection handle */
} __packed hci_change_con_link_key_compl_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_MASTER_LINK_KEY_COMPL	0x0a
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* Connection handle */
	u_int8_t	key_flag;   /* Key flag */
} __attribute__ ((packed)) ng_hci_master_link_key_compl_ep;
=======
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* Connection handle */
	uint8_t		key_flag;   /* Key flag */
} __packed hci_master_link_key_compl_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_READ_REMOTE_FEATURES_COMPL	0x0b
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;                         /* 0x00 - success */
	u_int16_t	con_handle;                     /* Connection handle */
	u_int8_t	features[NG_HCI_FEATURES_SIZE]; /* LMP features bitmsk*/
} __attribute__ ((packed)) ng_hci_read_remote_features_compl_ep;
=======
	uint8_t		status;                      /* 0x00 - success */
	uint16_t	con_handle;                  /* Connection handle */
	uint8_t		features[HCI_FEATURES_SIZE]; /* LMP features bitmsk*/
} __packed hci_read_remote_features_compl_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_READ_REMOTE_VER_INFO_COMPL	0x0c
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;         /* 0x00 - success */
	u_int16_t	con_handle;     /* Connection handle */
	u_int8_t	lmp_version;    /* LMP version */
	u_int16_t	manufacturer;   /* Hardware manufacturer name */
	u_int16_t	lmp_subversion; /* LMP sub-version */
} __attribute__ ((packed)) ng_hci_read_remote_ver_info_compl_ep;
=======
	uint8_t		status;         /* 0x00 - success */
	uint16_t	con_handle;     /* Connection handle */
	uint8_t		lmp_version;    /* LMP version */
	uint16_t	manufacturer;   /* Hardware manufacturer name */
	uint16_t	lmp_subversion; /* LMP sub-version */
} __packed hci_read_remote_ver_info_compl_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_QOS_SETUP_COMPL		0x0d
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;          /* 0x00 - success */
	u_int16_t	con_handle;      /* connection handle */
	u_int8_t	flags;           /* reserved for future use */
	u_int8_t	service_type;    /* service type */
	u_int32_t	token_rate;      /* bytes per second */
	u_int32_t	peak_bandwidth;  /* bytes per second */
	u_int32_t	latency;         /* microseconds */
	u_int32_t	delay_variation; /* microseconds */
} __attribute__ ((packed)) ng_hci_qos_setup_compl_ep;
=======
	uint8_t		status;          /* 0x00 - success */
	uint16_t	con_handle;      /* connection handle */
	uint8_t		flags;           /* reserved for future use */
	uint8_t		service_type;    /* service type */
	uint32_t	token_rate;      /* bytes per second */
	uint32_t	peak_bandwidth;  /* bytes per second */
	uint32_t	latency;         /* microseconds */
	uint32_t	delay_variation; /* microseconds */
} __packed hci_qos_setup_compl_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_COMMAND_COMPL		0x0e
typedef struct {
	u_int8_t	num_cmd_pkts; /* # of HCI command packets */
	u_int16_t	opcode;       /* command OpCode */
	/* command return parameters (if any) */
<<<<<<< HEAD
} __attribute__ ((packed)) ng_hci_command_compl_ep;
=======
} __packed hci_command_compl_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_COMMAND_STATUS		0x0f
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;       /* 0x00 - pending */
	u_int8_t	num_cmd_pkts; /* # of HCI command packets */
	u_int16_t	opcode;       /* command OpCode */
} __attribute__ ((packed)) ng_hci_command_status_ep;
=======
	uint8_t		status;       /* 0x00 - pending */
	uint8_t		num_cmd_pkts; /* # of HCI command packets */
	uint16_t	opcode;       /* command OpCode */
} __packed hci_command_status_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_HARDWARE_ERROR		0x10
typedef struct {
<<<<<<< HEAD
	u_int8_t	hardware_code; /* hardware error code */
} __attribute__ ((packed)) ng_hci_hardware_error_ep;
=======
	uint8_t		hardware_code; /* hardware error code */
} __packed hci_hardware_error_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_FLUSH_OCCUR		0x11
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_flush_occur_ep;
=======
	uint16_t	con_handle; /* connection handle */
} __packed hci_flush_occur_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_ROLE_CHANGE		0x12
typedef struct {
	u_int8_t	status; /* 0x00 - success */
	bdaddr_t	bdaddr; /* address of remote unit */
<<<<<<< HEAD
	u_int8_t	role;   /* new connection role */
} __attribute__ ((packed)) ng_hci_role_change_ep;
=======
	uint8_t		role;   /* new connection role */
} __packed hci_role_change_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_NUM_COMPL_PKTS		0x13
typedef struct {
<<<<<<< HEAD
	u_int8_t	num_con_handles; /* # of connection handles */
/* these are repeated "num_con_handles" times 
	u_int16_t	con_handle; --- connection handle(s)
	u_int16_t	compl_pkt;  --- # of completed packets */
} __attribute__ ((packed)) ng_hci_num_compl_pkts_ep;
=======
	uint8_t		num_con_handles; /* # of connection handles */
/* these are repeated "num_con_handles" times
	uint16_t	con_handle; --- connection handle(s)
	uint16_t	compl_pkts; --- # of completed packets */
} __packed hci_num_compl_pkts_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_MODE_CHANGE		0x14
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* connection handle */
	u_int8_t	unit_mode;  /* remote unit mode */
	u_int16_t	interval;   /* interval * 0.625 msec */
} __attribute__ ((packed)) ng_hci_mode_change_ep;
=======
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* connection handle */
	uint8_t		unit_mode;  /* remote unit mode */
	uint16_t	interval;   /* interval * 0.625 msec */
} __packed hci_mode_change_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_RETURN_LINK_KEYS		0x15
typedef struct {
	u_int8_t	num_keys; /* # of keys */
/* these are repeated "num_keys" times 
	bdaddr_t	bdaddr;               --- remote address(es)
<<<<<<< HEAD
	u_int8_t	key[NG_HCI_KEY_SIZE]; --- key(s) */
} __attribute__ ((packed)) ng_hci_return_link_keys_ep;
=======
	uint8_t		key[HCI_KEY_SIZE]; --- key(s) */
} __packed hci_return_link_keys_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_PIN_CODE_REQ		0x16
typedef struct {
	bdaddr_t	bdaddr; /* remote unit address */
<<<<<<< HEAD
} __attribute__ ((packed)) ng_hci_pin_code_req_ep;
=======
} __packed hci_pin_code_req_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_LINK_KEY_REQ		0x17
typedef struct {
	bdaddr_t	bdaddr; /* remote unit address */
<<<<<<< HEAD
} __attribute__ ((packed)) ng_hci_link_key_req_ep;
=======
} __packed hci_link_key_req_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_LINK_KEY_NOTIFICATION	0x18
typedef struct {
<<<<<<< HEAD
	bdaddr_t	bdaddr;               /* remote unit address */
	u_int8_t	key[NG_HCI_KEY_SIZE]; /* link key */
	u_int8_t	key_type;             /* type of the key */
} __attribute__ ((packed)) ng_hci_link_key_notification_ep;
=======
	bdaddr_t	bdaddr;            /* remote unit address */
	uint8_t		key[HCI_KEY_SIZE]; /* link key */
	uint8_t		key_type;          /* type of the key */
} __packed hci_link_key_notification_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_LOOPBACK_COMMAND		0x19
typedef struct {
	u_int8_t	command[0]; /* Command packet */
} __attribute__ ((packed)) ng_hci_loopback_command_ep;

#define NG_HCI_EVENT_DATA_BUFFER_OVERFLOW	0x1a
typedef struct {
<<<<<<< HEAD
	u_int8_t	link_type; /* Link type */
} __attribute__ ((packed)) ng_hci_data_buffer_overflow_ep;
=======
	uint8_t		link_type; /* Link type */
} __packed hci_data_buffer_overflow_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_MAX_SLOT_CHANGE		0x1b
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle;    /* connection handle */
	u_int8_t	lmp_max_slots; /* Max. # of slots allowed */
} __attribute__ ((packed)) ng_hci_max_slot_change_ep;
=======
	uint16_t	con_handle;    /* connection handle */
	uint8_t		lmp_max_slots; /* Max. # of slots allowed */
} __packed hci_max_slot_change_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_READ_CLOCK_OFFSET_COMPL	0x1c
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;       /* 0x00 - success */
	u_int16_t	con_handle;   /* Connection handle */
	u_int16_t	clock_offset; /* Clock offset */
} __attribute__ ((packed)) ng_hci_read_clock_offset_compl_ep;
=======
	uint8_t		status;       /* 0x00 - success */
	uint16_t	con_handle;   /* Connection handle */
	uint16_t	clock_offset; /* Clock offset */
} __packed hci_read_clock_offset_compl_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_CON_PKT_TYPE_CHANGED	0x1d
typedef struct {
<<<<<<< HEAD
	u_int8_t	status;     /* 0x00 - success */
	u_int16_t	con_handle; /* connection handle */
	u_int16_t	pkt_type;   /* packet type */
} __attribute__ ((packed)) ng_hci_con_pkt_type_changed_ep;
=======
	uint8_t		status;     /* 0x00 - success */
	uint16_t	con_handle; /* connection handle */
	uint16_t	pkt_type;   /* packet type */
} __packed hci_con_pkt_type_changed_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_QOS_VIOLATION		0x1e
typedef struct {
<<<<<<< HEAD
	u_int16_t	con_handle; /* connection handle */
} __attribute__ ((packed)) ng_hci_qos_violation_ep;
=======
	uint16_t	con_handle; /* connection handle */
} __packed hci_qos_violation_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_PAGE_SCAN_MODE_CHANGE	0x1f
typedef struct {
	bdaddr_t	bdaddr;         /* destination address */
<<<<<<< HEAD
	u_int8_t	page_scan_mode; /* page scan mode */
} __attribute__ ((packed)) ng_hci_page_scan_mode_change_ep;
=======
	uint8_t		page_scan_mode; /* page scan mode */
} __packed hci_page_scan_mode_change_ep;
>>>>>>> origin/master

#define NG_HCI_EVENT_PAGE_SCAN_REP_MODE_CHANGE	0x20
typedef struct {
	bdaddr_t	bdaddr;             /* destination address */
<<<<<<< HEAD
	u_int8_t	page_scan_rep_mode; /* page scan repetition mode */
} __attribute__ ((packed)) ng_hci_page_scan_rep_mode_change_ep;
=======
	uint8_t		page_scan_rep_mode; /* page scan repetition mode */
} __packed hci_page_scan_rep_mode_change_ep;

#define HCI_EVENT_FLOW_SPECIFICATION_COMPL	0x21
typedef struct {
	uint8_t		status;		/* 0x00 - success */
	uint16_t	con_handle;	/* connection handle */
	uint8_t		flags;		/* reserved */
	uint8_t		direction;	/* flow direction */
	uint8_t		type;		/* service type */
	uint32_t	token_rate;	/* token rate */
	uint32_t	bucket_size;	/* token bucket size */
	uint32_t	peak_bandwidth;	/* peak bandwidth */
	uint32_t	latency;	/* access latency */
} __packed hci_flow_specification_compl_ep;

#define HCI_EVENT_RSSI_RESULT			0x22
typedef struct {
	uint8_t		num_responses;      /* number of responses */
/*	hci_rssi_response[num_responses]   -- see below */
} __packed hci_rssi_result_ep;

typedef struct {
	bdaddr_t	bdaddr;			/* unit address */
	uint8_t		page_scan_rep_mode;	/* page scan rep. mode */
	uint8_t		blank;			/* reserved */
	uint8_t		uclass[HCI_CLASS_SIZE];	/* unit class */
	uint16_t	clock_offset;		/* clock offset */
	int8_t		rssi;			/* rssi */
} __packed hci_rssi_response;

#define HCI_EVENT_READ_REMOTE_EXTENDED_FEATURES	0x23
typedef struct {
	uint8_t		status;		/* 0x00 - success */
	uint16_t	con_handle;	/* connection handle */
	uint8_t		page;		/* page number */
	uint8_t		max;		/* max page number */
	uint8_t		features[HCI_FEATURES_SIZE]; /* LMP features bitmsk*/
} __packed hci_read_remote_extended_features_ep;

#define HCI_EVENT_SCO_CON_COMPL			0x2c
typedef struct {
	uint8_t		status;		/* 0x00 - success */
	uint16_t	con_handle;	/* connection handle */
	bdaddr_t	bdaddr;		/* unit address */
	uint8_t		link_type;	/* link type */
	uint8_t		interval;	/* transmission interval */
	uint8_t		window;		/* retransmission window */
	uint16_t	rxlen;		/* rx packet length */
	uint16_t	txlen;		/* tx packet length */
	uint8_t		mode;		/* air mode */
} __packed hci_sco_con_compl_ep;

#define HCI_EVENT_SCO_CON_CHANGED		0x2d
typedef struct {
	uint8_t		status;		/* 0x00 - success */
	uint16_t	con_handle;	/* connection handle */
	uint8_t		interval;	/* transmission interval */
	uint8_t		window;		/* retransmission window */
	uint16_t	rxlen;		/* rx packet length */
	uint16_t	txlen;		/* tx packet length */
} __packed hci_sco_con_changed_ep;

#define HCI_EVENT_SNIFF_SUBRATING		0x2e
typedef struct {
	uint8_t		status;		/* 0x00 - success */
	uint16_t	con_handle;	/* connection handle */
	uint16_t	tx_latency;	/* max transmit latency */
	uint16_t	rx_latency;	/* max receive latency */
	uint16_t	remote_timeout;	/* remote timeout */
	uint16_t	local_timeout;	/* local timeout */
} __packed hci_sniff_subrating_ep;

#define HCI_EVENT_EXTENDED_RESULT		0x2f
typedef struct {
	uint8_t		num_responses;	/* must be 0x01 */
	bdaddr_t	bdaddr;		/* remote device address */
	uint8_t		page_scan_rep_mode;
	uint8_t		reserved;
	uint8_t		uclass[HCI_CLASS_SIZE];
	uint16_t	clock_offset;
	int8_t		rssi;
	uint8_t		response[240];	/* extended inquiry response */
} __packed hci_extended_result_ep;

#define HCI_EVENT_ENCRYPTION_KEY_REFRESH	0x30
typedef struct {
	uint8_t		status;		/* 0x00 - success */
	uint16_t	con_handle;	/* connection handle */
} __packed hci_encryption_key_refresh_ep;

#define HCI_EVENT_IO_CAPABILITY_REQ		0x31
typedef struct {
	bdaddr_t	bdaddr;		/* remote device address */
} __packed hci_io_capability_req_ep;

#define HCI_EVENT_IO_CAPABILITY_RSP		0x32
typedef struct {
	bdaddr_t	bdaddr;		/* remote device address */
	uint8_t		io_capability;
	uint8_t		oob_data_present;
	uint8_t		auth_requirement;
} __packed hci_io_capability_rsp_ep;

#define HCI_EVENT_USER_CONFIRM_REQ		0x33
typedef struct {
	bdaddr_t	bdaddr;		/* remote device address */
	uint32_t	value;		/* 000000 - 999999 */
} __packed hci_user_confirm_req_ep;

#define HCI_EVENT_USER_PASSKEY_REQ		0x34
typedef struct {
	bdaddr_t	bdaddr;		/* remote device address */
} __packed hci_user_passkey_req_ep;

#define HCI_EVENT_REMOTE_OOB_DATA_REQ		0x35
typedef struct {
	bdaddr_t	bdaddr;		/* remote device address */
} __packed hci_remote_oob_data_req_ep;

#define HCI_EVENT_SIMPLE_PAIRING_COMPL		0x36
typedef struct {
	uint8_t		status;		/* 0x00 - success */
	bdaddr_t	bdaddr;		/* remote device address */
} __packed hci_simple_pairing_compl_ep;

#define HCI_EVENT_LINK_SUPERVISION_TO_CHANGED	0x38
typedef struct {
	uint16_t	con_handle;	/* connection handle */
	uint16_t	timeout;	/* link supervision timeout */
} __packed hci_link_supervision_to_changed_ep;

#define HCI_EVENT_ENHANCED_FLUSH_COMPL		0x39
typedef struct {
	uint16_t	con_handle;	/* connection handle */
} __packed hci_enhanced_flush_compl_ep;

#define HCI_EVENT_USER_PASSKEY_NOTIFICATION	0x3b
typedef struct {
	bdaddr_t	bdaddr;		/* remote device address */
	uint32_t	value;		/* 000000 - 999999 */
} __packed hci_user_passkey_notification_ep;

#define HCI_EVENT_KEYPRESS_NOTIFICATION		0x3c
typedef struct {
	bdaddr_t	bdaddr;		/* remote device address */
	uint8_t		notification_type;
} __packed hci_keypress_notification_ep;

#define HCI_EVENT_REMOTE_FEATURES_NOTIFICATION	0x3d
typedef struct {
	bdaddr_t	bdaddr;		/* remote device address */
	uint8_t		features[HCI_FEATURES_SIZE]; /* LMP features bitmsk*/
} __packed hci_remote_features_notification_ep;

#define HCI_EVENT_BT_LOGO			0xfe

#define HCI_EVENT_VENDOR			0xff

/**************************************************************************
 **************************************************************************
 **                 HCI Socket Definitions
 **************************************************************************
 **************************************************************************/
>>>>>>> origin/master

#define NG_HCI_EVENT_BT_LOGO			0xfe

<<<<<<< HEAD
#define NG_HCI_EVENT_VENDOR			0xff

#endif /* ndef _NETGRAPH_HCI_H_ */
=======
/* Control Messages */
#define SCM_HCI_DIRECTION		SO_HCI_DIRECTION

/*
 * HCI socket filter and get/set routines
 *
 * for ease of use, we filter 256 possible events/packets
 */
struct hci_filter {
	uint32_t	mask[8];	/* 256 bits */
};

static __inline void
hci_filter_set(uint8_t bit, struct hci_filter *filter)
{
	uint8_t off = bit - 1;

	off >>= 5;
	filter->mask[off] |= (1 << ((bit - 1) & 0x1f));
}

static __inline void
hci_filter_clr(uint8_t bit, struct hci_filter *filter)
{
	uint8_t off = bit - 1;

	off >>= 5;
	filter->mask[off] &= ~(1 << ((bit - 1) & 0x1f));
}

static __inline int
hci_filter_test(uint8_t bit, struct hci_filter *filter)
{
	uint8_t off = bit - 1;

	off >>= 5;
	return (filter->mask[off] & (1 << ((bit - 1) & 0x1f)));
}

/*
 * HCI socket ioctl's
 *
 * Apart from GBTINFOA, these are all indexed on the unit name
 */

#define SIOCGBTINFO	_IOWR('b',  5, struct btreq) /* get unit info */
#define SIOCGBTINFOA	_IOWR('b',  6, struct btreq) /* get info by address */
#define SIOCNBTINFO	_IOWR('b',  7, struct btreq) /* next unit info */

#define SIOCSBTFLAGS	_IOWR('b',  8, struct btreq) /* set unit flags */
#define SIOCSBTPOLICY	_IOWR('b',  9, struct btreq) /* set unit link policy */
#define SIOCSBTPTYPE	_IOWR('b', 10, struct btreq) /* set unit packet type */

#define SIOCGBTSTATS	_IOWR('b', 11, struct btreq) /* get unit statistics */
#define SIOCZBTSTATS	_IOWR('b', 12, struct btreq) /* zero unit statistics */

#define SIOCBTDUMP	 _IOW('b', 13, struct btreq) /* print debug info */
#define SIOCSBTSCOMTU	_IOWR('b', 17, struct btreq) /* set sco_mtu value */

struct bt_stats {
	uint32_t	err_tx;
	uint32_t	err_rx;
	uint32_t	cmd_tx;
	uint32_t	evt_rx;
	uint32_t	acl_tx;
	uint32_t	acl_rx;
	uint32_t	sco_tx;
	uint32_t	sco_rx;
	uint32_t	byte_tx;
	uint32_t	byte_rx;
};

struct btreq {
	char	btr_name[HCI_DEVNAME_SIZE];	/* device name */

	union {
	    struct {
		bdaddr_t btri_bdaddr;		/* device bdaddr */
		uint16_t btri_flags;		/* flags */
		uint16_t btri_num_cmd;		/* # of free cmd buffers */
		uint16_t btri_num_acl;		/* # of free ACL buffers */
		uint16_t btri_num_sco;		/* # of free SCO buffers */
		uint16_t btri_acl_mtu;		/* ACL mtu */
		uint16_t btri_sco_mtu;		/* SCO mtu */
		uint16_t btri_link_policy;	/* Link Policy */
		uint16_t btri_packet_type;	/* Packet Type */
	    } btri;
	    struct bt_stats btrs;   /* unit stats */
	} btru;
};

#define btr_flags	btru.btri.btri_flags
#define btr_bdaddr	btru.btri.btri_bdaddr
#define btr_num_cmd	btru.btri.btri_num_cmd
#define btr_num_acl	btru.btri.btri_num_acl
#define btr_num_sco	btru.btri.btri_num_sco
#define btr_acl_mtu	btru.btri.btri_acl_mtu
#define btr_sco_mtu	btru.btri.btri_sco_mtu
#define btr_link_policy btru.btri.btri_link_policy
#define btr_packet_type btru.btri.btri_packet_type
#define btr_stats	btru.btrs

/* hci_unit & btr_flags */
#define BTF_UP			(1<<0)	/* unit is up */
#define BTF_RUNNING		(1<<1)	/* unit is running */
#define BTF_XMIT_CMD		(1<<2)	/* unit is transmitting CMD packets */
#define BTF_XMIT_ACL		(1<<3)	/* unit is transmitting ACL packets */
#define BTF_XMIT_SCO		(1<<4)	/* unit is transmitting SCO packets */
#define BTF_XMIT		(BTF_XMIT_CMD | BTF_XMIT_ACL | BTF_XMIT_SCO)
#define BTF_INIT_BDADDR		(1<<5)	/* waiting for bdaddr */
#define BTF_INIT_BUFFER_SIZE	(1<<6)	/* waiting for buffer size */
#define BTF_INIT_FEATURES	(1<<7)	/* waiting for features */
#define BTF_POWER_UP_NOOP	(1<<8)	/* should wait for No-op on power up */
#define BTF_INIT_COMMANDS	(1<<9)	/* waiting for supported commands */

#define BTF_INIT		(BTF_INIT_BDADDR	\
				| BTF_INIT_BUFFER_SIZE	\
				| BTF_INIT_FEATURES	\
				| BTF_INIT_COMMANDS)

/**************************************************************************
 **************************************************************************
 **                 HCI Kernel Definitions
 **************************************************************************
 **************************************************************************/

#ifdef _KERNEL

#include <sys/device.h>
#include <net/if.h>		/* for struct ifqueue */

struct l2cap_channel;
struct mbuf;
struct sco_pcb;
struct socket;

/* global HCI kernel variables */

/* sysctl variables */
extern int hci_memo_expiry;
extern int hci_acl_expiry;
extern int hci_sendspace, hci_recvspace;
extern int hci_eventq_max, hci_aclrxq_max, hci_scorxq_max;

/*
 * HCI Connection Information
 */
struct hci_link {
	struct hci_unit		*hl_unit;	/* our unit */
	TAILQ_ENTRY(hci_link)	 hl_next;	/* next link on unit */

	/* common info */
	uint16_t		 hl_state;	/* connection state */
	uint16_t		 hl_flags;	/* link flags */
	bdaddr_t		 hl_bdaddr;	/* dest address */
	uint16_t		 hl_handle;	/* connection handle */
	uint8_t			 hl_type;	/* link type */

	/* ACL link info */
	uint8_t			 hl_lastid;	/* last id used */
	uint16_t		 hl_refcnt;	/* reference count */
	uint16_t		 hl_mtu;	/* signalling mtu for link */
	uint16_t		 hl_flush;	/* flush timeout */
	uint16_t		 hl_clock;	/* remote clock offset */

	TAILQ_HEAD(,l2cap_pdu)	 hl_txq;	/* queue of outgoing PDUs */
	int			 hl_txqlen;	/* number of fragments */
	struct mbuf		*hl_rxp;	/* incoming PDU (accumulating)*/
	struct timeout		 hl_expire;	/* connection expiry timer */
	TAILQ_HEAD(,l2cap_req)	 hl_reqs;	/* pending requests */

	/* SCO link info */
	struct hci_link		*hl_link;	/* SCO ACL link */
	struct sco_pcb		*hl_sco;	/* SCO pcb */
	struct ifqueue		 hl_data;	/* SCO outgoing data */
};

/* hci_link state */
#define HCI_LINK_CLOSED		0  /* closed */
#define HCI_LINK_WAIT_CONNECT	1  /* waiting to connect */
#define HCI_LINK_WAIT_AUTH	2  /* waiting for auth */
#define HCI_LINK_WAIT_ENCRYPT	3  /* waiting for encrypt */
#define HCI_LINK_WAIT_SECURE	4  /* waiting for secure */
#define HCI_LINK_OPEN		5  /* ready and willing */
#define HCI_LINK_BLOCK		6  /* open but blocking (see hci_acl_start) */

/* hci_link flags */
#define HCI_LINK_AUTH_REQ	(1<<0)  /* authentication requested */
#define HCI_LINK_ENCRYPT_REQ	(1<<1)  /* encryption requested */
#define HCI_LINK_SECURE_REQ	(1<<2)	/* secure link requested */
#define HCI_LINK_AUTH		(1<<3)	/* link is authenticated */
#define HCI_LINK_ENCRYPT	(1<<4)	/* link is encrypted */
#define HCI_LINK_SECURE		(1<<5)	/* link is secured */
#define HCI_LINK_CREATE_CON	(1<<6)	/* "Create Connection" pending */

/*
 * Bluetooth Memo
 *	cached device information for remote devices that this unit has seen
 */
struct hci_memo {
	struct timeval		time;		/* time of last response */
	bdaddr_t		bdaddr;
	uint8_t			page_scan_rep_mode;
	uint8_t			page_scan_mode;
	uint16_t		clock_offset;
	LIST_ENTRY(hci_memo)	next;
};

/*
 * The Bluetooth HCI interface attachment structure
 */
struct hci_if {
	int	(*enable)(struct device *);
	void	(*disable)(struct device *);
	void	(*output_cmd)(struct device *, struct mbuf *);
	void	(*output_acl)(struct device *, struct mbuf *);
	void	(*output_sco)(struct device *, struct mbuf *);
	void	(*get_stats)(struct device *, struct bt_stats *, int);
	int	ipl;		/* for locking */
};

/*
 * The Bluetooth HCI device unit structure
 */
struct hci_unit {
	struct device	*hci_dev;		/* bthci handle */
	struct device	*hci_bthub;		/* bthub(4) handle */
	const struct hci_if *hci_if;		/* bthci driver interface */

	/* device info */
	bdaddr_t	 hci_bdaddr;		/* device address */
	uint16_t	 hci_flags;		/* see BTF_ above */
	int		 hci_init;		/* sleep on this */

	uint16_t	 hci_packet_type;	/* packet types */
	uint16_t	 hci_acl_mask;		/* ACL packet capabilities */
	uint16_t	 hci_sco_mask;		/* SCO packet capabilities */

	uint16_t	 hci_link_policy;	/* link policy */
	uint16_t	 hci_lmp_mask;		/* link policy capabilities */

	uint8_t		 hci_cmds[HCI_COMMANDS_SIZE]; /* opcode bitmask */

	/* flow control */
	uint16_t	 hci_max_acl_size;	/* ACL payload mtu */
	uint16_t	 hci_num_acl_pkts;	/* free ACL packet buffers */
	uint8_t		 hci_num_cmd_pkts;	/* free CMD packet buffers */
	uint8_t		 hci_max_sco_size;	/* SCO payload mtu */
	uint16_t	 hci_num_sco_pkts;	/* free SCO packet buffers */

	TAILQ_HEAD(,hci_link)	hci_links;	/* list of ACL/SCO links */
	LIST_HEAD(,hci_memo)	hci_memos;	/* cached memo list */

	/* input queues */
#ifndef __OpenBSD__
	void			*hci_rxint;	/* receive interrupt cookie */
#endif
	struct mutex		 hci_devlock;	/* device queue lock */
	struct ifqueue		 hci_eventq;	/* Event queue */
	struct ifqueue		 hci_aclrxq;	/* ACL rx queue */
	struct ifqueue		 hci_scorxq;	/* SCO rx queue */
	uint16_t		 hci_eventqlen;	/* Event queue length */
	uint16_t		 hci_aclrxqlen;	/* ACL rx queue length */
	uint16_t		 hci_scorxqlen;	/* SCO rx queue length */

	/* output queues */
	struct ifqueue		 hci_cmdwait;	/* pending commands */
	struct ifqueue		 hci_scodone;	/* SCO done queue */

	TAILQ_ENTRY(hci_unit) hci_next;
};

extern TAILQ_HEAD(hci_unit_list, hci_unit) hci_unit_list;

/*
 * HCI layer function prototypes
 */

/* hci_event.c */
void hci_event(struct mbuf *, struct hci_unit *);

/* hci_ioctl.c */
int hci_ioctl(unsigned long, void *, struct proc *);

/* hci_link.c */
struct hci_link *hci_acl_open(struct hci_unit *, bdaddr_t *);
struct hci_link *hci_acl_newconn(struct hci_unit *, bdaddr_t *);
void hci_acl_close(struct hci_link *, int);
void hci_acl_timeout(void *);
int hci_acl_setmode(struct hci_link *);
void hci_acl_linkmode(struct hci_link *);
void hci_acl_recv(struct mbuf *, struct hci_unit *);
int hci_acl_send(struct mbuf *, struct hci_link *, struct l2cap_channel *);
void hci_acl_start(struct hci_link *);
void hci_acl_complete(struct hci_link *, int);
struct hci_link *hci_sco_newconn(struct hci_unit *, bdaddr_t *);
void hci_sco_recv(struct mbuf *, struct hci_unit *);
void hci_sco_start(struct hci_link *);
void hci_sco_complete(struct hci_link *, int);
struct hci_link *hci_link_alloc(struct hci_unit *, bdaddr_t *, uint8_t);
void hci_link_free(struct hci_link *, int);
struct hci_link *hci_link_lookup_bdaddr(struct hci_unit *, bdaddr_t *, uint8_t);
struct hci_link *hci_link_lookup_handle(struct hci_unit *, uint16_t);

/* hci_misc.c */
int hci_route_lookup(bdaddr_t *, bdaddr_t *);
struct hci_memo *hci_memo_find(struct hci_unit *, bdaddr_t *);
struct hci_memo *hci_memo_new(struct hci_unit *, bdaddr_t *);
void hci_memo_free(struct hci_memo *);

/* hci_socket.c */
void hci_drop(void *);
int hci_usrreq(struct socket *, int, struct mbuf *, struct mbuf *,
    struct mbuf *, struct proc *);
int hci_ctloutput(int, struct socket *, int, int, struct mbuf **);
void hci_mtap(struct mbuf *, struct hci_unit *);

/* hci_unit.c */
struct hci_unit *hci_attach(const struct hci_if *, struct device *, uint16_t);
void hci_detach(struct hci_unit *);
int hci_enable(struct hci_unit *);
void hci_disable(struct hci_unit *);
struct hci_unit *hci_unit_lookup(bdaddr_t *);
int hci_send_cmd(struct hci_unit *, uint16_t, void *, uint8_t);
void hci_num_cmds(struct hci_unit *, uint8_t);
int hci_input_event(struct hci_unit *, struct mbuf *);
int hci_input_acl(struct hci_unit *, struct mbuf *);
int hci_input_sco(struct hci_unit *, struct mbuf *);
int hci_complete_sco(struct hci_unit *, struct mbuf *);
void hci_output_cmd(struct hci_unit *, struct mbuf *);
void hci_output_acl(struct hci_unit *, struct mbuf *);
void hci_output_sco(struct hci_unit *, struct mbuf *);
void hci_intr(void *);

/* XXX mimic NetBSD for now, although we don't have these interfaces */
#define M_GETCTX(m, t)	((t)(m)->m_pkthdr.rcvif)
#define M_SETCTX(m, c)	((m)->m_pkthdr.rcvif = (void *)(c))
#define splraiseipl(ipl) splbio() /* XXX */
#define ENOLINK ENOENT		/* XXX */
#define EPASSTHROUGH ENOTTY	/* XXX */
#define device_xname(dv)	(dv)->dv_xname

#endif	/* _KERNEL */

#endif /* _NETBT_HCI_H_ */
>>>>>>> origin/master
