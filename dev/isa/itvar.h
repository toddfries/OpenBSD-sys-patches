<<<<<<< HEAD
/*	$OpenBSD: itvar.h,v 1.3 2006/12/23 17:46:39 deraadt Exp $	*/
=======
/*	$OpenBSD: itvar.h,v 1.13 2011/01/20 16:59:55 form Exp $	*/
>>>>>>> origin/master

/*
 * Copyright (c) 2003 Julien Bordet <zejames@greyhats.org>
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

#ifndef _DEV_ISA_ITVAR_H
#define _DEV_ISA_ITVAR_H

<<<<<<< HEAD
#define IT_NUM_SENSORS	15
=======
#define IT_EC_INTERVAL		5
#define IT_EC_NUMSENSORS	17
#define IT_EC_VREF		4096
>>>>>>> origin/master

/* chip ids */
#define IT_ID_IT87	0x90

/* ctl registers */

<<<<<<< HEAD
#define ITC_ADDR	0x05
#define ITC_DATA	0x06
=======
#define IT_ID_8705		0x8705
#define IT_ID_8712		0x8712
#define IT_ID_8716		0x8716
#define IT_ID_8718		0x8718
#define IT_ID_8720		0x8720
#define IT_ID_8721		0x8721
#define IT_ID_8726		0x8726
>>>>>>> origin/master

/* data registers */

#define ITD_CONFIG	0x00
#define ITD_ISR1	0x01
#define ITD_ISR2	0x02
#define ITD_ISR3	0x03
#define ITD_SMI1	0x04
#define ITD_SMI2	0x05
#define ITD_SMI3	0x06
#define ITD_IMR1	0x07
#define ITD_IMR2	0x08
#define ITD_IMR3	0x09
#define ITD_VID		0x0a
#define ITD_FAN		0x0b

#define ITD_FANMINBASE	0x10
#define ITD_FANENABLE	0x13

<<<<<<< HEAD
#define ITD_SENSORFANBASE	0x0d	/* Fan from 0x0d to 0x0f */
#define ITD_SENSORVOLTBASE	0x20	/* Fan from 0x20 to 0x28 */
#define ITD_SENSORTEMPBASE	0x29	/* Fan from 0x29 to 0x2b */
=======
#define IT_EC_CFG		0x00
#define IT_EC_FAN_DIV		0x0b
#define IT_EC_FAN_ECER		0x0c
#define IT_EC_FAN_TAC1		0x0d
#define IT_EC_FAN_TAC2		0x0e
#define IT_EC_FAN_TAC3		0x0f
#define IT_EC_FAN_MCR		0x13
#define IT_EC_FAN_EXT_TAC1	0x18
#define IT_EC_FAN_EXT_TAC2	0x19
#define IT_EC_FAN_EXT_TAC3	0x1a
#define IT_EC_VOLTBASE		0x20
#define IT_EC_TEMPBASE		0x29
#define IT_EC_ADC_VINER		0x50
#define IT_EC_ADC_TEMPER	0x51
#define IT_EC_FAN_TAC4_LSB	0x80
#define IT_EC_FAN_TAC4_MSB	0x81
#define IT_EC_FAN_TAC5_LSB	0x82
#define IT_EC_FAN_TAC5_MSB	0x83

#define IT_EC_CFG_START		0x01
#define IT_EC_CFG_INTCLR	0x08
#define IT_EC_CFG_UPDVBAT	0x40
>>>>>>> origin/master

#define ITD_VOLTMAXBASE	0x30
#define ITD_VOLTMINBASE	0x31

#define ITD_TEMPMAXBASE 0x40
#define ITD_TEMPMINBASE 0x41

#define ITD_SBUSADDR	0x48
#define ITD_VOLTENABLE	0x50
#define ITD_TEMPENABLE	0x51

#define ITD_CHIPID	0x58

#define IT_VREF		(4096) /* Vref = 4.096 V */

struct it_softc {
	struct device sc_dev;

	bus_space_tag_t it_iot;
	bus_space_handle_t it_ioh;

	struct ksensor sensors[IT_NUM_SENSORS];
	struct ksensordev sensordev;
	u_int numsensors;
	void (*refresh_sensor_data)(struct it_softc *);

	u_int8_t (*it_readreg)(struct it_softc *, int);
	void (*it_writereg)(struct it_softc *, int, int);
};

#endif
