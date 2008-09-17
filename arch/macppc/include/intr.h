/*	$OpenBSD: intr.h,v 1.4 2008/09/16 04:20:42 drahn Exp $	*/

#include <powerpc/intr.h>

#ifndef _LOCORE
void softtty(void);

<<<<<<< HEAD:arch/macppc/include/intr.h
void openpic_send_ipi(int);
void openpic_set_priority(int);
=======
void openpic_set_priority(int, int);
>>>>>>> master:arch/macppc/include/intr.h
#endif
