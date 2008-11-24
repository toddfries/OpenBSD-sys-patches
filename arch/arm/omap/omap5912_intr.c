/*	$NetBSD: omap5912_intr.c,v 1.2 2008/11/21 17:13:07 matt Exp $	*/

/*
 * IRQ data specific to the Texas Instruments OMAP5912 processor.
 */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap5912_intr.c,v 1.2 2008/11/21 17:13:07 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/lock.h>

#include <arm/omap/omap_reg.h>
#include <arm/omap/omap_tipb.h>

/*
 * INTC autoconf glue
 */
CFATTACH_DECL_NEW(omap5912intc, 0,
    omapintc_match, omapintc_attach, NULL, NULL);

#define IRQ_TO_BANK_BASE(irq)						\
    (((irq) < OMAP_INT_L1_NIRQ)					\
	? OMAP_INT_L1_BASE						\
	: OMAP_INT_L2_BASE						\
	 + (irq-OMAP_INT_L1_NIRQ)/OMAP_BANK_WIDTH*OMAP_INTL2_BANK_OFF)
#define IRQ_TO_BANK_NUM(irq)						\
    ((irq)/OMAP_BANK_WIDTH)
#define IRQ_TO_ILR(irq)							\
    (IRQ_TO_BANK_BASE(irq) +						\
     OMAP_INTB_ILR_BASE +						\
     (irq) % OMAP_BANK_WIDTH * 4)
#define IRQ_TO_MASK(irq)						\
    (1 << (irq) % OMAP_BANK_WIDTH)

#define INTR_INFO(irq,t)				\
    [irq] = {						\
	.trig = (t),					\
	.bank_base = IRQ_TO_BANK_BASE(irq),		\
	.bank_num = IRQ_TO_BANK_NUM(irq),		\
	.ILR = IRQ_TO_ILR(irq),				\
	.mask = IRQ_TO_MASK(irq)			\
    }

const omap_intr_info_t omap_intr_info[OMAP_NIRQ] = {
	INTR_INFO(		     0, TRIG_LEVEL),	/* Level 2 IRQ */
	INTR_INFO(		     1, TRIG_LEVEL),	/* Camera IF */
	INTR_INFO(		     2, TRIG_LEVEL),	/* Level 2 FIQ */
	INTR_INFO(		     3, TRIG_LEVEL_OR_EDGE), /* External FIQ */
	INTR_INFO(		     4, TRIG_EDGE),	/* McBSP2 TX */
	INTR_INFO(		     5, TRIG_EDGE),	/* McBSP2 RX */
	INTR_INFO(		     6, TRIG_EDGE),	/* IRQ_RTDX */
	INTR_INFO(		     7, TRIG_LEVEL),	/* IRQ_DSP_MMU_ABORT */
	INTR_INFO(		     8, TRIG_EDGE),	/* IRQ_HOST_INT */
	INTR_INFO(		     9, TRIG_LEVEL),	/* IRQ_ABORT */
	INTR_INFO(		    10, TRIG_LEVEL),	/* IRQ_DSP_MAILBOX1 */
	INTR_INFO(		    11, TRIG_LEVEL),	/* IRQ_DSP_MAILBOX2 */
	INTR_INFO(		    12, TRIG_LEVEL),	/* IRQ_LCD_LINE */
	INTR_INFO(		    13, TRIG_LEVEL),	/* Private TIPB Abort */
	INTR_INFO(		    14, TRIG_LEVEL),	/* IRQ1_GPIO1 */
	INTR_INFO(		    15, TRIG_LEVEL),	/* UART3 */
	INTR_INFO(		    16, TRIG_EDGE),	/* IRQ_TIMER3 */
	INTR_INFO(		    17, TRIG_LEVEL),	/* GPTIMER1 */
	INTR_INFO(		    18, TRIG_LEVEL),	/* GPTIMER2 */
	INTR_INFO(		    19, TRIG_LEVEL),	/* IRQ_DMA_CH0 */
	INTR_INFO(		    20, TRIG_LEVEL),	/* IRQ_DMA_CH1 */
	INTR_INFO(		    21, TRIG_LEVEL),	/* IRQ_DMA_CH2 */
	INTR_INFO(		    22, TRIG_LEVEL),	/* IRQ_DMA_CH3 */
	INTR_INFO(		    23, TRIG_LEVEL),	/* IRQ_DMA_CH4 */
	INTR_INFO(		    24, TRIG_LEVEL),	/* IRQ_DMA_CH5 */
	INTR_INFO(		    25, TRIG_LEVEL),	/* IRQ_DMA_CH_LCD */
	INTR_INFO(		    26, TRIG_EDGE),	/* IRQ_TIMER1 */
	INTR_INFO(		    27, TRIG_EDGE),	/* IRQ_WD_TIMER */
	INTR_INFO(		    28, TRIG_LEVEL),	/* Public TIPB Abort */
	INTR_INFO(		    30, TRIG_EDGE),	/* IRQ_TIMER2 */
	INTR_INFO(		    31,	TRIG_EDGE),	/* IRQ_LCD_CTRL */
	INTR_INFO(OMAP_INT_L1_NIRQ+  0, TRIG_LEVEL),	/* FAC */
	INTR_INFO(OMAP_INT_L1_NIRQ+  1, TRIG_EDGE),	/* Keyboard */
	INTR_INFO(OMAP_INT_L1_NIRQ+  2, TRIG_LEVEL),	/* uWIRE TX */
	INTR_INFO(OMAP_INT_L1_NIRQ+  3, TRIG_LEVEL),	/* uWIRE RX */
	INTR_INFO(OMAP_INT_L1_NIRQ+  4, TRIG_LEVEL),	/* I2C */
	INTR_INFO(OMAP_INT_L1_NIRQ+  5, TRIG_LEVEL),	/* MPUIO */
	INTR_INFO(OMAP_INT_L1_NIRQ+  6, TRIG_LEVEL),	/* USB HHC 1 */
	INTR_INFO(OMAP_INT_L1_NIRQ+  7, TRIG_LEVEL),	/* USB HHC 2 */
	INTR_INFO(OMAP_INT_L1_NIRQ+  8, TRIG_LEVEL),	/* USB_OTG */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 10, TRIG_EDGE),	/* McBSP3 TX */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 11, TRIG_EDGE),	/* McBSP3 RX */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 12, TRIG_EDGE),	/* McBSP1 TX */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 13, TRIG_EDGE),	/* McBSP1 RX */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 14, TRIG_LEVEL),	/* UART1 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 15, TRIG_LEVEL),	/* UART2 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 16, TRIG_LEVEL),	/* MCSI1 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 17, TRIG_LEVEL),	/* MCSI2 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 18, TRIG_EDGE),	/* Free 1 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 20, TRIG_LEVEL),	/* USB Geni IT */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 21, TRIG_LEVEL),	/* 1-Wire */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 22, TRIG_EDGE),	/* OS timer */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 23, TRIG_LEVEL),	/* MMC/SDIO1 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 24, TRIG_EDGE),	/* USB client wakeup */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 25, TRIG_EDGE),	/* RTC periodic */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 26, TRIG_LEVEL),	/* RTC alarm */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 28, TRIG_LEVEL),	/* DSP_MMU_IRQ */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 29, TRIG_LEVEL),	/* USB IRQ_ISO_ON */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 30, TRIG_LEVEL),	/* USB IRQ_NON_ISO_ON */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 34, TRIG_LEVEL),	/* GPTIMER3 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 35, TRIG_LEVEL),	/* GPTIMER4 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 36, TRIG_LEVEL),	/* GPTIMER5 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 37, TRIG_LEVEL),	/* GPTIMER6 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 38, TRIG_LEVEL),	/* GPTIMER7 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 39, TRIG_LEVEL),	/* GPTIMER8 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 40, TRIG_LEVEL),	/* IRQ1_GPIO2 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 41, TRIG_LEVEL),	/* IRQ1_GPIO3 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 42, TRIG_LEVEL),	/* MMC/SDIO2 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 43, TRIG_EDGE),	/* CompactFlash */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 44, TRIG_LEVEL),	/* COMMRX */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 45, TRIG_LEVEL),	/* COMMTX */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 46, TRIG_EDGE),	/* Peripheral wake up */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 47, TRIG_EDGE),	/* Free 2 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 48, TRIG_LEVEL),	/* IRQ1_GPIO4 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 49, TRIG_LEVEL),	/* SPI */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 53, TRIG_LEVEL),	/* IRQ_DMA_CH6 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 54, TRIG_LEVEL),	/* IRQ_DMA_CH7 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 55, TRIG_LEVEL),	/* IRQ_DMA_CH8 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 56, TRIG_LEVEL),	/* IRQ_DMA_CH9 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 57, TRIG_LEVEL),	/* IRQ_DMA_CH10 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 58, TRIG_LEVEL),	/* IRQ_DMA_CH11 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 59, TRIG_LEVEL),	/* IRQ_DMA_CH12 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 60, TRIG_LEVEL),	/* IRQ_DMA_CH13 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 61, TRIG_LEVEL),	/* IRQ_DMA_CH14 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 62, TRIG_LEVEL),	/* IRQ_DMA_CH15 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 66, TRIG_EDGE),	/* Free 3 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 91, TRIG_LEVEL),	/* SHA1/MD5 */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 92, TRIG_LEVEL),	/* RNG */
	INTR_INFO(OMAP_INT_L1_NIRQ+ 93, TRIG_LEVEL),	/* RNGIDLE */
	INTR_INFO(OMAP_INT_L1_NIRQ+103, TRIG_EDGE),	/* Free 4 */
	INTR_INFO(OMAP_INT_L1_NIRQ+104, TRIG_EDGE),	/* Free 5 */
	INTR_INFO(OMAP_INT_L1_NIRQ+105, TRIG_EDGE),	/* Free 6 */
	INTR_INFO(OMAP_INT_L1_NIRQ+106, TRIG_EDGE),	/* Free 7 */
	INTR_INFO(OMAP_INT_L1_NIRQ+107, TRIG_EDGE),	/* Free 8 */
	INTR_INFO(OMAP_INT_L1_NIRQ+108, TRIG_EDGE),	/* Free 9 */
	INTR_INFO(OMAP_INT_L1_NIRQ+109, TRIG_EDGE),	/* Free 10 */
	INTR_INFO(OMAP_INT_L1_NIRQ+110, TRIG_EDGE),	/* Free 11 */
	INTR_INFO(OMAP_INT_L1_NIRQ+111, TRIG_EDGE),	/* Free 12 */
	INTR_INFO(OMAP_INT_L1_NIRQ+112, TRIG_EDGE),	/* Free 13 */
	INTR_INFO(OMAP_INT_L1_NIRQ+113, TRIG_EDGE),	/* Free 14 */
	INTR_INFO(OMAP_INT_L1_NIRQ+114, TRIG_EDGE),	/* Free 15 */
	INTR_INFO(OMAP_INT_L1_NIRQ+115, TRIG_EDGE),	/* Free 16 */
	INTR_INFO(OMAP_INT_L1_NIRQ+116, TRIG_EDGE),	/* Free 17 */
	INTR_INFO(OMAP_INT_L1_NIRQ+117, TRIG_EDGE),	/* Free 18 */
	INTR_INFO(OMAP_INT_L1_NIRQ+118, TRIG_EDGE),	/* Free 19 */
	INTR_INFO(OMAP_INT_L1_NIRQ+119, TRIG_EDGE),	/* Free 20 */
	INTR_INFO(OMAP_INT_L1_NIRQ+120, TRIG_EDGE),	/* Free 21 */
	INTR_INFO(OMAP_INT_L1_NIRQ+121, TRIG_EDGE),	/* Free 22 */
	INTR_INFO(OMAP_INT_L1_NIRQ+122, TRIG_EDGE),	/* Free 23 */
	INTR_INFO(OMAP_INT_L1_NIRQ+123, TRIG_EDGE),	/* Free 24 */
	INTR_INFO(OMAP_INT_L1_NIRQ+124, TRIG_EDGE),	/* Free 25 */
	INTR_INFO(OMAP_INT_L1_NIRQ+125, TRIG_EDGE),	/* Free 26 */
	INTR_INFO(OMAP_INT_L1_NIRQ+126, TRIG_EDGE),	/* Free 27 */
	INTR_INFO(OMAP_INT_L1_NIRQ+127, TRIG_EDGE),	/* Free 28 */
};

/* Array of pointers to each bank's base. */
vaddr_t omap_intr_bank_bases[OMAP_NBANKS] = {
	OMAP_INT_L1_BASE,
	OMAP_INT_L2_BASE + 0*OMAP_INTL2_BANK_OFF,
	OMAP_INT_L2_BASE + 1*OMAP_INTL2_BANK_OFF,
	OMAP_INT_L2_BASE + 2*OMAP_INTL2_BANK_OFF,
	OMAP_INT_L2_BASE + 3*OMAP_INTL2_BANK_OFF,
};

/* Array to translate from software interrupt numbers to an irq number. */
int omap_si_to_irq[OMAP_FREE_IRQ_NUM] = {
	OMAP_INT_L1_NIRQ+ 18,				/* Free 1 */
	OMAP_INT_L1_NIRQ+ 47,				/* Free 2 */
	OMAP_INT_L1_NIRQ+ 66,				/* Free 3 */
	OMAP_INT_L1_NIRQ+103,				/* Free 4 */
	OMAP_INT_L1_NIRQ+104,				/* Free 5 */
	OMAP_INT_L1_NIRQ+105,				/* Free 6 */
	OMAP_INT_L1_NIRQ+106,				/* Free 7 */
	OMAP_INT_L1_NIRQ+107,				/* Free 8 */
	OMAP_INT_L1_NIRQ+108,				/* Free 9 */
	OMAP_INT_L1_NIRQ+109,				/* Free 10 */
	OMAP_INT_L1_NIRQ+110,				/* Free 11 */
	OMAP_INT_L1_NIRQ+111,				/* Free 12 */
	OMAP_INT_L1_NIRQ+112,				/* Free 13 */
	OMAP_INT_L1_NIRQ+113,				/* Free 14 */
	OMAP_INT_L1_NIRQ+114,				/* Free 15 */
	OMAP_INT_L1_NIRQ+115,				/* Free 16 */
	OMAP_INT_L1_NIRQ+116,				/* Free 17 */
	OMAP_INT_L1_NIRQ+117,				/* Free 18 */
	OMAP_INT_L1_NIRQ+118,				/* Free 19 */
	OMAP_INT_L1_NIRQ+119,				/* Free 20 */
	OMAP_INT_L1_NIRQ+120,				/* Free 21 */
	OMAP_INT_L1_NIRQ+121,				/* Free 22 */
	OMAP_INT_L1_NIRQ+122,				/* Free 23 */
	OMAP_INT_L1_NIRQ+123,				/* Free 24 */
	OMAP_INT_L1_NIRQ+124,				/* Free 25 */
	OMAP_INT_L1_NIRQ+125,				/* Free 26 */
	OMAP_INT_L1_NIRQ+126,				/* Free 27 */
	OMAP_INT_L1_NIRQ+127,				/* Free 28 */
};
