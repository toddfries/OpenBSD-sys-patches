/*
 * $FreeBSD: src/sys/ia64/include/smp.h,v 1.11 2008/09/28 18:34:14 marius Exp $
 */
#ifndef _MACHINE_SMP_H_
#define _MACHINE_SMP_H_

#ifdef _KERNEL

/*
 * Interprocessor interrupts for SMP. The following values are indices
 * into the IPI vector table. The SAL gives us the vector used for AP
 * wake-up. We base the other vectors on that. Keep IPI_AP_WAKEUP at
 * index 0. See sal.c for details.
 */
/* Architecture specific IPIs. */
#define	IPI_AP_WAKEUP		0
#define	IPI_HIGH_FP		1
#define	IPI_MCA_CMCV		2
#define	IPI_MCA_RENDEZ		3
/* Machine independent IPIs. */
#define	IPI_AST			4
#define	IPI_RENDEZVOUS		5
#define	IPI_STOP		6
#define	IPI_PREEMPT		7

#define	IPI_COUNT		8

#ifndef LOCORE

struct pcpu;

extern int ipi_vector[];

void	ipi_all_but_self(int ipi);
void	ipi_selected(cpumask_t cpus, int ipi);
void	ipi_send(struct pcpu *, int ipi);

#endif /* !LOCORE */
#endif /* _KERNEL */
#endif /* !_MACHINE_SMP_H */
