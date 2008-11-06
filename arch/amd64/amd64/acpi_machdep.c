/*	$OpenBSD: acpi_machdep.c,v 1.13 2008/06/01 17:59:55 marco Exp $	*/
/*
 * Copyright (c) 2005 Thorsten Lockert <tholo@sigmasoft.com>
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/conf.h>
#include <machine/biosvar.h>
#include <machine/isa_machdep.h>

#include <machine/cpufunc.h>

#include <dev/isa/isareg.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include "acpi_wakecode.h"

#include "ioapic.h"

#define ACPI_BIOS_RSDP_WINDOW_BASE        0xe0000
#define ACPI_BIOS_RSDP_WINDOW_SIZE        0x20000

/*
 * See comment in init386 in machdep.c to see the reason behind the 8.
 */
#define ACPI_TRAMPOLINE         (8 * PAGE_SIZE)

unsigned char *acpi_trampoline;

int acpi_savecpu(void);
void acpi_restorecpu(void);
int acpi_sleep_machdep(struct acpi_softc *, int);

/*
  The ACPI sleep and resume code on amd64 probably won't work for you.
  You have been warned.
*/

extern uint64_t acpi_saved_cr0, acpi_saved_cr2, acpi_saved_cr3,  acpi_saved_cr4;
extern uint32_t acpi_saved_efer;
extern uint16_t acpi_saved_cs, acpi_saved_ds, acpi_saved_es, acpi_saved_fs;
extern uint16_t acpi_saved_gs, acpi_saved_ss, acpi_saved_tr, acpi_saved_ldt;
extern struct region_descriptor acpi_saved_gdt, acpi_saved_idt;

u_int8_t	*acpi_scan(struct acpi_mem_map *, paddr_t, size_t);

int
acpi_map(paddr_t pa, size_t len, struct acpi_mem_map *handle)
{
	paddr_t pgpa = trunc_page(pa);
	paddr_t endpa = round_page(pa + len);
	vaddr_t va = uvm_km_valloc(kernel_map, endpa - pgpa);

	if (va == 0)
		return (ENOMEM);

	handle->baseva = va;
	handle->va = (u_int8_t *)(va + (pa & PGOFSET));
	handle->vsize = endpa - pgpa;
	handle->pa = pa;

	do {
		pmap_kenter_pa(va, pgpa, VM_PROT_READ | VM_PROT_WRITE);
		va += NBPG;
		pgpa += NBPG;
	} while (pgpa < endpa);

	return 0;
}

void
acpi_unmap(struct acpi_mem_map *handle)
{
	pmap_kremove(handle->baseva, handle->vsize);
	uvm_km_free(kernel_map, handle->baseva, handle->vsize);
}

u_int8_t *
acpi_scan(struct acpi_mem_map *handle, paddr_t pa, size_t len)
{
	size_t i;
	u_int8_t *ptr;
	struct acpi_rsdp1 *rsdp;

	if (acpi_map(pa, len, handle))
		return (NULL);
	for (ptr = handle->va, i = 0;
	     i < len;
	     ptr += 16, i += 16)
		if (memcmp(ptr, RSDP_SIG, sizeof(RSDP_SIG) - 1) == 0) {
			rsdp = (struct acpi_rsdp1 *)ptr;
			/*
			 * Only checksum whichever portion of the
			 * RSDP that is actually present
			 */
			if (rsdp->revision == 0 &&
			    acpi_checksum(ptr, sizeof(struct acpi_rsdp1)) == 0)
				return (ptr);
			else if (rsdp->revision >= 2 && rsdp->revision <= 3 &&
			    acpi_checksum(ptr, sizeof(struct acpi_rsdp)) == 0)
				return (ptr);
		}
	acpi_unmap(handle);

	return (NULL);
}

int
acpi_probe(struct device *parent, struct cfdata *match, struct bios_attach_args *ba)
{
	struct acpi_mem_map handle;
	u_int8_t *ptr;
	paddr_t ebda;
	bios_memmap_t *im;

	/*
	 * First look for ACPI entries in the BIOS memory map
	 */
	for (im = bios_memmap; im->type != BIOS_MAP_END; im++)
		if (im->type == BIOS_MAP_ACPI) {
			if ((ptr = acpi_scan(&handle, im->addr, im->size)))
				goto havebase;
		}

	/*
	 * Next try to find ACPI table entries in the EBDA
	 */
	if (acpi_map(0, NBPG, &handle))
		printf("acpi: failed to map BIOS data area\n");
	else {
		ebda = *(const u_int16_t *)(&handle.va[0x40e]);
		ebda <<= 4;
		acpi_unmap(&handle);

		if (ebda && ebda < IOM_BEGIN) {
			if ((ptr = acpi_scan(&handle, ebda, 1024)))
				goto havebase;
		}
	}

	/*
	 * Finally try to find the ACPI table entries in the
	 * BIOS memory
	 */
	if ((ptr = acpi_scan(&handle, ACPI_BIOS_RSDP_WINDOW_BASE,
	    ACPI_BIOS_RSDP_WINDOW_SIZE)))
		goto havebase;

	return (0);

havebase:
	ba->ba_acpipbase = ptr - handle.va + handle.pa;
	acpi_unmap(&handle);

	return (1);
}

#ifndef SMALL_KERNEL
void
acpi_attach_machdep(struct acpi_softc *sc)
{
	extern void (*cpuresetfn)(void);

	sc->sc_interrupt = isa_intr_establish(NULL, sc->sc_fadt->sci_int,
	    IST_LEVEL, IPL_TTY, acpi_interrupt, sc, sc->sc_dev.dv_xname);
	cpuresetfn = acpi_reset;

#ifdef acpi_sleep_enabled
        acpi_trampoline = (char *)uvm_km_valloc(kernel_map, PAGE_SIZE);
        pmap_kenter_pa((vaddr_t)acpi_trampoline, (paddr_t)ACPI_TRAMPOLINE,
            VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
        memcpy(acpi_trampoline, wakecode, sizeof(wakecode));
        printf("Got acpi_trampoline @ %lx\n", acpi_trampoline);
#endif

}
#endif /* SMALL_KERNEL */


void acpi_cpu_flush(struct acpi_softc *, int);

void
acpi_cpu_flush(struct acpi_softc *sc, int state)
{
        /*
         * Flush write back caches since we'll lose them.
         */
        if (state > ACPI_STATE_S1)
                wbinvd();
}

int
acpi_sleep_machdep(struct acpi_softc *sc, int state)
{
#ifdef acpi_sleep_enabled
        if (sc->sc_facs == NULL) {
                printf("acpi_sleep: no FACS\n");
                return ENXIO;
        }

        if (rcr3() != pmap_kernel()->pm_pdirpa) {
                printf("acpi_sleep: only kernel may sleep\n");
                return ENXIO;
        }

        /*
         *
         * ACPI defines two wakeup vectors. One is used for ACPI
         * 1.0 implementations - it's in the FACS table as
         * wakeup_vector and indicates a 32-bit physical address
         * containing real-mode wakeup code (16-bit code).
         *
         * The second wakeup vector is in the FACS table as
         * x_wakeup_vector and indicates a 64-bit physical address
         * containing protected-mode wakeup code.
         *
         * According to ACPI, we're only supposed to set the
         * x_wakeup_vector when the version of the FACS table
         * is 0x1. Unfortunately, some BIOSes set this to
         * garbage, some set it to ASCII '1', and some others
         * invent their own setting. In the cases where
         * the version is 0x1 or '1', we set the
         * x_wakeup_vector to NULL and hope that the BIOS
         * does the right thing. If the version is something else,
         * we do not set the x_wakeup_vector because we could be
         * scribbling on random memory at the end of the table in
         * that case. If your BIOS does this, you need to get
         * a new BIOS.
         *
         * We process the 64 bit wakeup_vector since the BIOS 
         * won't know * or care about the kernel arch. We set 
         * it to NULL,  which means that we don't supply a 
         * protected mode wakeup routine. All ACPI-compliant
         * BIOSes are supposed to honor this and defer to the 
         * real mode wakeup routine. If your BIOS doesn't do this,
         * it's buggy and you need to get a new one.
         *
         */
        sc->sc_facs->wakeup_vector = (u_int32_t)ACPI_TRAMPOLINE;
        printf("ACPI Wakeup Vector-32: %d,%x\n", sc->sc_facs->length, sc->sc_facs->wakeup_vector);
        if( sc->sc_facs->version == 1  || sc->sc_facs->version == '1' )
        {
                sc->sc_facs->x_wakeup_vector = (u_int64_t)0;
                printf("ACPI Wakeup Vector-64: %d,%x\n", sc->sc_facs->length, sc->sc_facs->x_wakeup_vector);
        }


#define WAKECODE_COPY(offset, val, type)                        \
        memcpy(&acpi_trampoline[offset], &val, sizeof(type))

        disable_intr();

        /*
         * acpi_savecpu() copies the current cpu registers
         * into previous_*. We then copy these values into the
         * wakecode binary code living at address ACPI_TRAMPOLINE.
         *
         * The previous_* variables are defined in acpi_wakeup.S.
         *
         */
 
	if (acpi_savecpu())
        {
                void (*ret)(void) = acpi_restorecpu;
                uint32_t wakecode_32_ptr;
		uint32_t wakecode_64_ptr;

                wakecode_32_ptr = ACPI_TRAMPOLINE + wakeup_32;
		wakecode_64_ptr = ACPI_TRAMPOLINE + wakeup_64;

                WAKECODE_COPY(wakeup_sw32 + 2, wakecode_32_ptr, uint32_t);
                WAKECODE_COPY(wakeup_sw64 + 2, wakecode_64_ptr, uint32_t);

                WAKECODE_COPY(previous_efer, acpi_saved_efer, uint32_t);

                WAKECODE_COPY(previous_cr0, acpi_saved_cr0, uint64_t);
                WAKECODE_COPY(previous_cr2, acpi_saved_cr2, uint64_t);
                WAKECODE_COPY(previous_cr3, acpi_saved_cr3, uint64_t);
                WAKECODE_COPY(previous_cr4, acpi_saved_cr4, uint64_t);
                WAKECODE_COPY(previous_tr, acpi_saved_tr, uint16_t);
                WAKECODE_COPY(previous_ldt, acpi_saved_ldt, uint16_t);
                WAKECODE_COPY(previous_gdt, acpi_saved_gdt,
                    struct region_descriptor);
                WAKECODE_COPY(previous_idt, acpi_saved_idt,
                    struct region_descriptor);
                WAKECODE_COPY(previous_ds, acpi_saved_ds, uint16_t);
                WAKECODE_COPY(previous_es, acpi_saved_es, uint16_t);
                WAKECODE_COPY(previous_fs, acpi_saved_fs, uint16_t);
                WAKECODE_COPY(previous_gs, acpi_saved_gs, uint16_t);
                WAKECODE_COPY(previous_ss, acpi_saved_ss, uint16_t);
                WAKECODE_COPY(where_to_recover, ret, void *);

                wbinvd();

                acpi_enter_sleep_state(sc, state);
                printf( "ACPI: Continuing after entering sleep state.\n" );

                /* We should have entered sleep already,
                   but in case the ACPI implementation hasn't yet
                   put us there, we'll just wait (forever) for
                   it to happen... This is also a catchall for times
                   when we refused to sleep for some reason -
                   we don't really want to continue executing here.
                */
                for(;;);
        }

        /* On resume, the execution path will actually occur here.
           This is because we previously saved the stack location
           in acpi_savecpu, and issued a far jmp to acpi_restorecpu.
           At the end of acpi_restorecpu, we issue a jmp to the
           previously saved return address. This means we are
           returning to the location immediately following the
           last call instruction - after the call to acpi_savecpu.
         */


	/* ?? What's the right resume code here for AMD64? 
           Some things from i386 don't seem to apply.. Maybe there
           are others that need to be done..?
	*/
     

        /* Stack will point here after resume, or if we failed   
           earlier in which case we need to undo what we did   
        */

        intr_calculatemasks(&cpu_info_primary);
#if NIOAPIC > 0
        ioapic_enable();
#endif
        initrtclock();
        enable_intr();

#endif
        return (0);
}
