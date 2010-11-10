/*-
 * Copyright (c) 1996, by Steve Passe
 * Copyright (c) 2003, by Peter Wemm
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/amd64/amd64/mp_machdep.c,v 1.337 2010/11/08 20:05:22 jhb Exp $");

#include "opt_cpu.h"
#include "opt_kstack_pages.h"
#include "opt_mp_watchdog.h"
#include "opt_sched.h"
#include "opt_smp.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#ifdef GPROF 
#include <sys/gmon.h>
#endif
#include <sys/kernel.h>
#include <sys/ktr.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/memrange.h>
#include <sys/mutex.h>
#include <sys/pcpu.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/smp.h>
#include <sys/sysctl.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>
#include <vm/vm_extern.h>

#include <x86/apicreg.h>
#include <machine/clock.h>
#include <machine/cputypes.h>
#include <machine/cpufunc.h>
#include <x86/mca.h>
#include <machine/md_var.h>
#include <machine/mp_watchdog.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/smp.h>
#include <machine/specialreg.h>
#include <machine/tss.h>

#define WARMBOOT_TARGET		0
#define WARMBOOT_OFF		(KERNBASE + 0x0467)
#define WARMBOOT_SEG		(KERNBASE + 0x0469)

#define CMOS_REG		(0x70)
#define CMOS_DATA		(0x71)
#define BIOS_RESET		(0x0f)
#define BIOS_WARM		(0x0a)

/* lock region used by kernel profiling */
int	mcount_lock;

int	mp_naps;		/* # of Applications processors */
int	boot_cpu_id = -1;	/* designated BSP */

extern  struct pcpu __pcpu[];

/* AP uses this during bootstrap.  Do not staticize.  */
char *bootSTK;
static int bootAP;

/* Free these after use */
void *bootstacks[MAXCPU];

/* Temporary variables for init_secondary()  */
char *doublefault_stack;
char *nmi_stack;
void *dpcpu;

struct pcb stoppcbs[MAXCPU];
struct pcb **susppcbs = NULL;

/* Variables needed for SMP tlb shootdown. */
vm_offset_t smp_tlb_addr1;
vm_offset_t smp_tlb_addr2;
volatile int smp_tlb_wait;

#ifdef COUNT_IPIS
/* Interrupt counts. */
static u_long *ipi_preempt_counts[MAXCPU];
static u_long *ipi_ast_counts[MAXCPU];
u_long *ipi_invltlb_counts[MAXCPU];
u_long *ipi_invlrng_counts[MAXCPU];
u_long *ipi_invlpg_counts[MAXCPU];
u_long *ipi_invlcache_counts[MAXCPU];
u_long *ipi_rendezvous_counts[MAXCPU];
u_long *ipi_lazypmap_counts[MAXCPU];
static u_long *ipi_hardclock_counts[MAXCPU];
#endif

extern inthand_t IDTVEC(fast_syscall), IDTVEC(fast_syscall32);

/*
 * Local data and functions.
 */

static volatile cpumask_t ipi_nmi_pending;

/* used to hold the AP's until we are ready to release them */
static struct mtx ap_boot_mtx;

/* Set to 1 once we're ready to let the APs out of the pen. */
static volatile int aps_ready = 0;

/*
 * Store data from cpu_add() until later in the boot when we actually setup
 * the APs.
 */
struct cpu_info {
	int	cpu_present:1;
	int	cpu_bsp:1;
	int	cpu_disabled:1;
	int	cpu_hyperthread:1;
} static cpu_info[MAX_APIC_ID + 1];
int cpu_apic_ids[MAXCPU];
int apic_cpuids[MAX_APIC_ID + 1];

/* Holds pending bitmap based IPIs per CPU */
static volatile u_int cpu_ipi_pending[MAXCPU];

static u_int boot_address;
static int cpu_logical;			/* logical cpus per core */
static int cpu_cores;			/* cores per package */

static void	assign_cpu_ids(void);
static void	set_interrupt_apic_ids(void);
static int	start_all_aps(void);
static int	start_ap(int apic_id);
static void	release_aps(void *dummy);

static int	hlt_logical_cpus;
static u_int	hyperthreading_cpus;	/* logical cpus sharing L1 cache */
static cpumask_t	hyperthreading_cpus_mask;
static int	hyperthreading_allowed = 1;
static struct	sysctl_ctx_list logical_cpu_clist;
static u_int	bootMP_size;

static void
mem_range_AP_init(void)
{
	if (mem_range_softc.mr_op && mem_range_softc.mr_op->initAP)
		mem_range_softc.mr_op->initAP(&mem_range_softc);
}

static void
topo_probe_amd(void)
{

	/* AMD processors do not support HTT. */
	cpu_cores = (amd_feature2 & AMDID2_CMP) != 0 ?
	    (cpu_procinfo2 & AMDID_CMP_CORES) + 1 : 1;
	cpu_logical = 1;
}

/*
 * Round up to the next power of two, if necessary, and then
 * take log2.
 * Returns -1 if argument is zero.
 */
static __inline int
mask_width(u_int x)
{

	return (fls(x << (1 - powerof2(x))) - 1);
}

static void
topo_probe_0x4(void)
{
	u_int p[4];
	int pkg_id_bits;
	int core_id_bits;
	int max_cores;
	int max_logical;
	int id;

	/* Both zero and one here mean one logical processor per package. */
	max_logical = (cpu_feature & CPUID_HTT) != 0 ?
	    (cpu_procinfo & CPUID_HTT_CORES) >> 16 : 1;
	if (max_logical <= 1)
		return;

	/*
	 * Because of uniformity assumption we examine only
	 * those logical processors that belong to the same
	 * package as BSP.  Further, we count number of
	 * logical processors that belong to the same core
	 * as BSP thus deducing number of threads per core.
	 */
	cpuid_count(0x04, 0, p);
	max_cores = ((p[0] >> 26) & 0x3f) + 1;
	core_id_bits = mask_width(max_logical/max_cores);
	if (core_id_bits < 0)
		return;
	pkg_id_bits = core_id_bits + mask_width(max_cores);

	for (id = 0; id <= MAX_APIC_ID; id++) {
		/* Check logical CPU availability. */
		if (!cpu_info[id].cpu_present || cpu_info[id].cpu_disabled)
			continue;
		/* Check if logical CPU has the same package ID. */
		if ((id >> pkg_id_bits) != (boot_cpu_id >> pkg_id_bits))
			continue;
		cpu_cores++;
		/* Check if logical CPU has the same package and core IDs. */
		if ((id >> core_id_bits) == (boot_cpu_id >> core_id_bits))
			cpu_logical++;
	}

	KASSERT(cpu_cores >= 1 && cpu_logical >= 1,
	    ("topo_probe_0x4 couldn't find BSP"));

	cpu_cores /= cpu_logical;
	hyperthreading_cpus = cpu_logical;
}

static void
topo_probe_0xb(void)
{
	u_int p[4];
	int bits;
	int cnt;
	int i;
	int logical;
	int type;
	int x;

	/* We only support three levels for now. */
	for (i = 0; i < 3; i++) {
		cpuid_count(0x0b, i, p);

		/* Fall back if CPU leaf 11 doesn't really exist. */
		if (i == 0 && p[1] == 0) {
			topo_probe_0x4();
			return;
		}

		bits = p[0] & 0x1f;
		logical = p[1] &= 0xffff;
		type = (p[2] >> 8) & 0xff;
		if (type == 0 || logical == 0)
			break;
		/*
		 * Because of uniformity assumption we examine only
		 * those logical processors that belong to the same
		 * package as BSP.
		 */
		for (cnt = 0, x = 0; x <= MAX_APIC_ID; x++) {
			if (!cpu_info[x].cpu_present ||
			    cpu_info[x].cpu_disabled)
				continue;
			if (x >> bits == boot_cpu_id >> bits)
				cnt++;
		}
		if (type == CPUID_TYPE_SMT)
			cpu_logical = cnt;
		else if (type == CPUID_TYPE_CORE)
			cpu_cores = cnt;
	}
	if (cpu_logical == 0)
		cpu_logical = 1;
	cpu_cores /= cpu_logical;
}

/*
 * Both topology discovery code and code that consumes topology
 * information assume top-down uniformity of the topology.
 * That is, all physical packages must be identical and each
 * core in a package must have the same number of threads.
 * Topology information is queried only on BSP, on which this
 * code runs and for which it can query CPUID information.
 * Then topology is extrapolated on all packages using the
 * uniformity assumption.
 */
static void
topo_probe(void)
{
	static int cpu_topo_probed = 0;

	if (cpu_topo_probed)
		return;

	logical_cpus_mask = 0;
	if (mp_ncpus <= 1)
		cpu_cores = cpu_logical = 1;
	else if (cpu_vendor_id == CPU_VENDOR_AMD)
		topo_probe_amd();
	else if (cpu_vendor_id == CPU_VENDOR_INTEL) {
		/*
		 * See Intel(R) 64 Architecture Processor
		 * Topology Enumeration article for details.
		 *
		 * Note that 0x1 <= cpu_high < 4 case should be
		 * compatible with topo_probe_0x4() logic when
		 * CPUID.1:EBX[23:16] > 0 (cpu_cores will be 1)
		 * or it should trigger the fallback otherwise.
		 */
		if (cpu_high >= 0xb)
			topo_probe_0xb();
		else if (cpu_high >= 0x1)
			topo_probe_0x4();
	}

	/*
	 * Fallback: assume each logical CPU is in separate
	 * physical package.  That is, no multi-core, no SMT.
	 */
	if (cpu_cores == 0 || cpu_logical == 0)
		cpu_cores = cpu_logical = 1;
	cpu_topo_probed = 1;
}

struct cpu_group *
cpu_topo(void)
{
	int cg_flags;

	/*
	 * Determine whether any threading flags are
	 * necessry.
	 */
	topo_probe();
	if (cpu_logical > 1 && hyperthreading_cpus)
		cg_flags = CG_FLAG_HTT;
	else if (cpu_logical > 1)
		cg_flags = CG_FLAG_SMT;
	else
		cg_flags = 0;
	if (mp_ncpus % (cpu_cores * cpu_logical) != 0) {
		printf("WARNING: Non-uniform processors.\n");
		printf("WARNING: Using suboptimal topology.\n");
		return (smp_topo_none());
	}
	/*
	 * No multi-core or hyper-threaded.
	 */
	if (cpu_logical * cpu_cores == 1)
		return (smp_topo_none());
	/*
	 * Only HTT no multi-core.
	 */
	if (cpu_logical > 1 && cpu_cores == 1)
		return (smp_topo_1level(CG_SHARE_L1, cpu_logical, cg_flags));
	/*
	 * Only multi-core no HTT.
	 */
	if (cpu_cores > 1 && cpu_logical == 1)
		return (smp_topo_1level(CG_SHARE_L2, cpu_cores, cg_flags));
	/*
	 * Both HTT and multi-core.
	 */
	return (smp_topo_2level(CG_SHARE_L2, cpu_cores,
	    CG_SHARE_L1, cpu_logical, cg_flags));
}

/*
 * Calculate usable address in base memory for AP trampoline code.
 */
u_int
mp_bootaddress(u_int basemem)
{

	bootMP_size = mptramp_end - mptramp_start;
	boot_address = trunc_page(basemem * 1024); /* round down to 4k boundary */
	if (((basemem * 1024) - boot_address) < bootMP_size)
		boot_address -= PAGE_SIZE;	/* not enough, lower by 4k */
	/* 3 levels of page table pages */
	mptramp_pagetables = boot_address - (PAGE_SIZE * 3);

	return mptramp_pagetables;
}

void
cpu_add(u_int apic_id, char boot_cpu)
{

	if (apic_id > MAX_APIC_ID) {
		panic("SMP: APIC ID %d too high", apic_id);
		return;
	}
	KASSERT(cpu_info[apic_id].cpu_present == 0, ("CPU %d added twice",
	    apic_id));
	cpu_info[apic_id].cpu_present = 1;
	if (boot_cpu) {
		KASSERT(boot_cpu_id == -1,
		    ("CPU %d claims to be BSP, but CPU %d already is", apic_id,
		    boot_cpu_id));
		boot_cpu_id = apic_id;
		cpu_info[apic_id].cpu_bsp = 1;
	}
	if (mp_ncpus < MAXCPU) {
		mp_ncpus++;
		mp_maxid = mp_ncpus - 1;
	}
	if (bootverbose)
		printf("SMP: Added CPU %d (%s)\n", apic_id, boot_cpu ? "BSP" :
		    "AP");
}

void
cpu_mp_setmaxid(void)
{

	/*
	 * mp_maxid should be already set by calls to cpu_add().
	 * Just sanity check its value here.
	 */
	if (mp_ncpus == 0)
		KASSERT(mp_maxid == 0,
		    ("%s: mp_ncpus is zero, but mp_maxid is not", __func__));
	else if (mp_ncpus == 1)
		mp_maxid = 0;
	else
		KASSERT(mp_maxid >= mp_ncpus - 1,
		    ("%s: counters out of sync: max %d, count %d", __func__,
			mp_maxid, mp_ncpus));
}

int
cpu_mp_probe(void)
{

	/*
	 * Always record BSP in CPU map so that the mbuf init code works
	 * correctly.
	 */
	all_cpus = 1;
	if (mp_ncpus == 0) {
		/*
		 * No CPUs were found, so this must be a UP system.  Setup
		 * the variables to represent a system with a single CPU
		 * with an id of 0.
		 */
		mp_ncpus = 1;
		return (0);
	}

	/* At least one CPU was found. */
	if (mp_ncpus == 1) {
		/*
		 * One CPU was found, so this must be a UP system with
		 * an I/O APIC.
		 */
		mp_maxid = 0;
		return (0);
	}

	/* At least two CPUs were found. */
	return (1);
}

/*
 * Initialize the IPI handlers and start up the AP's.
 */
void
cpu_mp_start(void)
{
	int i;

	/* Initialize the logical ID to APIC ID table. */
	for (i = 0; i < MAXCPU; i++) {
		cpu_apic_ids[i] = -1;
		cpu_ipi_pending[i] = 0;
	}

	/* Install an inter-CPU IPI for TLB invalidation */
	setidt(IPI_INVLTLB, IDTVEC(invltlb), SDT_SYSIGT, SEL_KPL, 0);
	setidt(IPI_INVLPG, IDTVEC(invlpg), SDT_SYSIGT, SEL_KPL, 0);
	setidt(IPI_INVLRNG, IDTVEC(invlrng), SDT_SYSIGT, SEL_KPL, 0);

	/* Install an inter-CPU IPI for cache invalidation. */
	setidt(IPI_INVLCACHE, IDTVEC(invlcache), SDT_SYSIGT, SEL_KPL, 0);

	/* Install an inter-CPU IPI for all-CPU rendezvous */
	setidt(IPI_RENDEZVOUS, IDTVEC(rendezvous), SDT_SYSIGT, SEL_KPL, 0);

	/* Install generic inter-CPU IPI handler */
	setidt(IPI_BITMAP_VECTOR, IDTVEC(ipi_intr_bitmap_handler),
	       SDT_SYSIGT, SEL_KPL, 0);

	/* Install an inter-CPU IPI for CPU stop/restart */
	setidt(IPI_STOP, IDTVEC(cpustop), SDT_SYSIGT, SEL_KPL, 0);

	/* Install an inter-CPU IPI for CPU suspend/resume */
	setidt(IPI_SUSPEND, IDTVEC(cpususpend), SDT_SYSIGT, SEL_KPL, 0);

	/* Set boot_cpu_id if needed. */
	if (boot_cpu_id == -1) {
		boot_cpu_id = PCPU_GET(apic_id);
		cpu_info[boot_cpu_id].cpu_bsp = 1;
	} else
		KASSERT(boot_cpu_id == PCPU_GET(apic_id),
		    ("BSP's APIC ID doesn't match boot_cpu_id"));

	/* Probe logical/physical core configuration. */
	topo_probe();

	assign_cpu_ids();

	/* Start each Application Processor */
	start_all_aps();

	set_interrupt_apic_ids();
}


/*
 * Print various information about the SMP system hardware and setup.
 */
void
cpu_mp_announce(void)
{
	const char *hyperthread;
	int i;

	printf("FreeBSD/SMP: %d package(s) x %d core(s)",
	    mp_ncpus / (cpu_cores * cpu_logical), cpu_cores);
	if (hyperthreading_cpus > 1)
	    printf(" x %d HTT threads", cpu_logical);
	else if (cpu_logical > 1)
	    printf(" x %d SMT threads", cpu_logical);
	printf("\n");

	/* List active CPUs first. */
	printf(" cpu0 (BSP): APIC ID: %2d\n", boot_cpu_id);
	for (i = 1; i < mp_ncpus; i++) {
		if (cpu_info[cpu_apic_ids[i]].cpu_hyperthread)
			hyperthread = "/HT";
		else
			hyperthread = "";
		printf(" cpu%d (AP%s): APIC ID: %2d\n", i, hyperthread,
		    cpu_apic_ids[i]);
	}

	/* List disabled CPUs last. */
	for (i = 0; i <= MAX_APIC_ID; i++) {
		if (!cpu_info[i].cpu_present || !cpu_info[i].cpu_disabled)
			continue;
		if (cpu_info[i].cpu_hyperthread)
			hyperthread = "/HT";
		else
			hyperthread = "";
		printf("  cpu (AP%s): APIC ID: %2d (disabled)\n", hyperthread,
		    i);
	}
}

/*
 * AP CPU's call this to initialize themselves.
 */
void
init_secondary(void)
{
	struct pcpu *pc;
	struct nmi_pcpu *np;
	u_int64_t msr, cr0;
	int cpu, gsel_tss, x;
	struct region_descriptor ap_gdt;

	/* Set by the startup code for us to use */
	cpu = bootAP;

	/* Init tss */
	common_tss[cpu] = common_tss[0];
	common_tss[cpu].tss_rsp0 = 0;   /* not used until after switch */
	common_tss[cpu].tss_iobase = sizeof(struct amd64tss) +
	    IOPAGES * PAGE_SIZE;
	common_tss[cpu].tss_ist1 = (long)&doublefault_stack[PAGE_SIZE];

	/* The NMI stack runs on IST2. */
	np = ((struct nmi_pcpu *) &nmi_stack[PAGE_SIZE]) - 1;
	common_tss[cpu].tss_ist2 = (long) np;

	/* Prepare private GDT */
	gdt_segs[GPROC0_SEL].ssd_base = (long) &common_tss[cpu];
	for (x = 0; x < NGDT; x++) {
		if (x != GPROC0_SEL && x != (GPROC0_SEL + 1) &&
		    x != GUSERLDT_SEL && x != (GUSERLDT_SEL + 1))
			ssdtosd(&gdt_segs[x], &gdt[NGDT * cpu + x]);
	}
	ssdtosyssd(&gdt_segs[GPROC0_SEL],
	    (struct system_segment_descriptor *)&gdt[NGDT * cpu + GPROC0_SEL]);
	ap_gdt.rd_limit = NGDT * sizeof(gdt[0]) - 1;
	ap_gdt.rd_base =  (long) &gdt[NGDT * cpu];
	lgdt(&ap_gdt);			/* does magic intra-segment return */

	/* Get per-cpu data */
	pc = &__pcpu[cpu];

	/* prime data page for it to use */
	pcpu_init(pc, cpu, sizeof(struct pcpu));
	dpcpu_init(dpcpu, cpu);
	pc->pc_apic_id = cpu_apic_ids[cpu];
	pc->pc_prvspace = pc;
	pc->pc_curthread = 0;
	pc->pc_tssp = &common_tss[cpu];
	pc->pc_commontssp = &common_tss[cpu];
	pc->pc_rsp0 = 0;
	pc->pc_tss = (struct system_segment_descriptor *)&gdt[NGDT * cpu +
	    GPROC0_SEL];
	pc->pc_fs32p = &gdt[NGDT * cpu + GUFS32_SEL];
	pc->pc_gs32p = &gdt[NGDT * cpu + GUGS32_SEL];
	pc->pc_ldt = (struct system_segment_descriptor *)&gdt[NGDT * cpu +
	    GUSERLDT_SEL];

	/* Save the per-cpu pointer for use by the NMI handler. */
	np->np_pcpu = (register_t) pc;

	wrmsr(MSR_FSBASE, 0);		/* User value */
	wrmsr(MSR_GSBASE, (u_int64_t)pc);
	wrmsr(MSR_KGSBASE, (u_int64_t)pc);	/* XXX User value while we're in the kernel */

	lidt(&r_idt);

	gsel_tss = GSEL(GPROC0_SEL, SEL_KPL);
	ltr(gsel_tss);

	/*
	 * Set to a known state:
	 * Set by mpboot.s: CR0_PG, CR0_PE
	 * Set by cpu_setregs: CR0_NE, CR0_MP, CR0_TS, CR0_WP, CR0_AM
	 */
	cr0 = rcr0();
	cr0 &= ~(CR0_CD | CR0_NW | CR0_EM);
	load_cr0(cr0);

	/* Set up the fast syscall stuff */
	msr = rdmsr(MSR_EFER) | EFER_SCE;
	wrmsr(MSR_EFER, msr);
	wrmsr(MSR_LSTAR, (u_int64_t)IDTVEC(fast_syscall));
	wrmsr(MSR_CSTAR, (u_int64_t)IDTVEC(fast_syscall32));
	msr = ((u_int64_t)GSEL(GCODE_SEL, SEL_KPL) << 32) |
	      ((u_int64_t)GSEL(GUCODE32_SEL, SEL_UPL) << 48);
	wrmsr(MSR_STAR, msr);
	wrmsr(MSR_SF_MASK, PSL_NT|PSL_T|PSL_I|PSL_C|PSL_D);

	/* Disable local APIC just to be sure. */
	lapic_disable();

	/* signal our startup to the BSP. */
	mp_naps++;

	/* Spin until the BSP releases the AP's. */
	while (!aps_ready)
		ia32_pause();

	/* Initialize the PAT MSR. */
	pmap_init_pat();

	/* set up CPU registers and state */
	cpu_setregs();

	/* set up SSE/NX registers */
	initializecpu();

	/* set up FPU state on the AP */
	fpuinit();

	/* A quick check from sanity claus */
	if (PCPU_GET(apic_id) != lapic_id()) {
		printf("SMP: cpuid = %d\n", PCPU_GET(cpuid));
		printf("SMP: actual apic_id = %d\n", lapic_id());
		printf("SMP: correct apic_id = %d\n", PCPU_GET(apic_id));
		panic("cpuid mismatch! boom!!");
	}

	/* Initialize curthread. */
	KASSERT(PCPU_GET(idlethread) != NULL, ("no idle thread"));
	PCPU_SET(curthread, PCPU_GET(idlethread));

	mca_init();

	mtx_lock_spin(&ap_boot_mtx);

	/* Init local apic for irq's */
	lapic_setup(1);

	/* Set memory range attributes for this CPU to match the BSP */
	mem_range_AP_init();

	smp_cpus++;

	CTR1(KTR_SMP, "SMP: AP CPU #%d Launched", PCPU_GET(cpuid));
	printf("SMP: AP CPU #%d Launched!\n", PCPU_GET(cpuid));

	/* Determine if we are a logical CPU. */
	/* XXX Calculation depends on cpu_logical being a power of 2, e.g. 2 */
	if (cpu_logical > 1 && PCPU_GET(apic_id) % cpu_logical != 0)
		logical_cpus_mask |= PCPU_GET(cpumask);
	
	/* Determine if we are a hyperthread. */
	if (hyperthreading_cpus > 1 &&
	    PCPU_GET(apic_id) % hyperthreading_cpus != 0)
		hyperthreading_cpus_mask |= PCPU_GET(cpumask);

	/* Build our map of 'other' CPUs. */
	PCPU_SET(other_cpus, all_cpus & ~PCPU_GET(cpumask));

	if (bootverbose)
		lapic_dump("AP");

	if (smp_cpus == mp_ncpus) {
		/* enable IPI's, tlb shootdown, freezes etc */
		atomic_store_rel_int(&smp_started, 1);
		smp_active = 1;	 /* historic */
	}

	/*
	 * Enable global pages TLB extension
	 * This also implicitly flushes the TLB 
	 */

	load_cr4(rcr4() | CR4_PGE);
	load_ds(_udatasel);
	load_es(_udatasel);
	load_fs(_ufssel);
	mtx_unlock_spin(&ap_boot_mtx);

	/* Wait until all the AP's are up. */
	while (smp_started == 0)
		ia32_pause();

	/* Start per-CPU event timers. */
	cpu_initclocks_ap();

	sched_throw(NULL);

	panic("scheduler returned us to %s", __func__);
	/* NOTREACHED */
}

/*******************************************************************
 * local functions and data
 */

/*
 * We tell the I/O APIC code about all the CPUs we want to receive
 * interrupts.  If we don't want certain CPUs to receive IRQs we
 * can simply not tell the I/O APIC code about them in this function.
 * We also do not tell it about the BSP since it tells itself about
 * the BSP internally to work with UP kernels and on UP machines.
 */
static void
set_interrupt_apic_ids(void)
{
	u_int i, apic_id;

	for (i = 0; i < MAXCPU; i++) {
		apic_id = cpu_apic_ids[i];
		if (apic_id == -1)
			continue;
		if (cpu_info[apic_id].cpu_bsp)
			continue;
		if (cpu_info[apic_id].cpu_disabled)
			continue;

		/* Don't let hyperthreads service interrupts. */
		if (hyperthreading_cpus > 1 &&
		    apic_id % hyperthreading_cpus != 0)
			continue;

		intr_add_cpu(i);
	}
}

/*
 * Assign logical CPU IDs to local APICs.
 */
static void
assign_cpu_ids(void)
{
	u_int i;

	TUNABLE_INT_FETCH("machdep.hyperthreading_allowed",
	    &hyperthreading_allowed);

	/* Check for explicitly disabled CPUs. */
	for (i = 0; i <= MAX_APIC_ID; i++) {
		if (!cpu_info[i].cpu_present || cpu_info[i].cpu_bsp)
			continue;

		if (hyperthreading_cpus > 1 && i % hyperthreading_cpus != 0) {
			cpu_info[i].cpu_hyperthread = 1;
#if defined(SCHED_ULE)
			/*
			 * Don't use HT CPU if it has been disabled by a
			 * tunable.
			 */
			if (hyperthreading_allowed == 0) {
				cpu_info[i].cpu_disabled = 1;
				continue;
			}
#endif
		}

		/* Don't use this CPU if it has been disabled by a tunable. */
		if (resource_disabled("lapic", i)) {
			cpu_info[i].cpu_disabled = 1;
			continue;
		}
	}

	/*
	 * Assign CPU IDs to local APIC IDs and disable any CPUs
	 * beyond MAXCPU.  CPU 0 is always assigned to the BSP.
	 *
	 * To minimize confusion for userland, we attempt to number
	 * CPUs such that all threads and cores in a package are
	 * grouped together.  For now we assume that the BSP is always
	 * the first thread in a package and just start adding APs
	 * starting with the BSP's APIC ID.
	 */
	mp_ncpus = 1;
	cpu_apic_ids[0] = boot_cpu_id;
	apic_cpuids[boot_cpu_id] = 0;
	for (i = boot_cpu_id + 1; i != boot_cpu_id;
	     i == MAX_APIC_ID ? i = 0 : i++) {
		if (!cpu_info[i].cpu_present || cpu_info[i].cpu_bsp ||
		    cpu_info[i].cpu_disabled)
			continue;

		if (mp_ncpus < MAXCPU) {
			cpu_apic_ids[mp_ncpus] = i;
			apic_cpuids[i] = mp_ncpus;
			mp_ncpus++;
		} else
			cpu_info[i].cpu_disabled = 1;
	}
	KASSERT(mp_maxid >= mp_ncpus - 1,
	    ("%s: counters out of sync: max %d, count %d", __func__, mp_maxid,
	    mp_ncpus));		
}

/*
 * start each AP in our list
 */
static int
start_all_aps(void)
{
	vm_offset_t va = boot_address + KERNBASE;
	u_int64_t *pt4, *pt3, *pt2;
	u_int32_t mpbioswarmvec;
	int apic_id, cpu, i;
	u_char mpbiosreason;

	mtx_init(&ap_boot_mtx, "ap boot", NULL, MTX_SPIN);

	/* install the AP 1st level boot code */
	pmap_kenter(va, boot_address);
	pmap_invalidate_page(kernel_pmap, va);
	bcopy(mptramp_start, (void *)va, bootMP_size);

	/* Locate the page tables, they'll be below the trampoline */
	pt4 = (u_int64_t *)(uintptr_t)(mptramp_pagetables + KERNBASE);
	pt3 = pt4 + (PAGE_SIZE) / sizeof(u_int64_t);
	pt2 = pt3 + (PAGE_SIZE) / sizeof(u_int64_t);

	/* Create the initial 1GB replicated page tables */
	for (i = 0; i < 512; i++) {
		/* Each slot of the level 4 pages points to the same level 3 page */
		pt4[i] = (u_int64_t)(uintptr_t)(mptramp_pagetables + PAGE_SIZE);
		pt4[i] |= PG_V | PG_RW | PG_U;

		/* Each slot of the level 3 pages points to the same level 2 page */
		pt3[i] = (u_int64_t)(uintptr_t)(mptramp_pagetables + (2 * PAGE_SIZE));
		pt3[i] |= PG_V | PG_RW | PG_U;

		/* The level 2 page slots are mapped with 2MB pages for 1GB. */
		pt2[i] = i * (2 * 1024 * 1024);
		pt2[i] |= PG_V | PG_RW | PG_PS | PG_U;
	}

	/* save the current value of the warm-start vector */
	mpbioswarmvec = *((u_int32_t *) WARMBOOT_OFF);
	outb(CMOS_REG, BIOS_RESET);
	mpbiosreason = inb(CMOS_DATA);

	/* setup a vector to our boot code */
	*((volatile u_short *) WARMBOOT_OFF) = WARMBOOT_TARGET;
	*((volatile u_short *) WARMBOOT_SEG) = (boot_address >> 4);
	outb(CMOS_REG, BIOS_RESET);
	outb(CMOS_DATA, BIOS_WARM);	/* 'warm-start' */

	/* start each AP */
	for (cpu = 1; cpu < mp_ncpus; cpu++) {
		apic_id = cpu_apic_ids[cpu];

		/* allocate and set up an idle stack data page */
		bootstacks[cpu] = (void *)kmem_alloc(kernel_map, KSTACK_PAGES * PAGE_SIZE);
		doublefault_stack = (char *)kmem_alloc(kernel_map, PAGE_SIZE);
		nmi_stack = (char *)kmem_alloc(kernel_map, PAGE_SIZE);
		dpcpu = (void *)kmem_alloc(kernel_map, DPCPU_SIZE);

		bootSTK = (char *)bootstacks[cpu] + KSTACK_PAGES * PAGE_SIZE - 8;
		bootAP = cpu;

		/* attempt to start the Application Processor */
		if (!start_ap(apic_id)) {
			/* restore the warmstart vector */
			*(u_int32_t *) WARMBOOT_OFF = mpbioswarmvec;
			panic("AP #%d (PHY# %d) failed!", cpu, apic_id);
		}

		all_cpus |= (1 << cpu);		/* record AP in CPU map */
	}

	/* build our map of 'other' CPUs */
	PCPU_SET(other_cpus, all_cpus & ~PCPU_GET(cpumask));

	/* restore the warmstart vector */
	*(u_int32_t *) WARMBOOT_OFF = mpbioswarmvec;

	outb(CMOS_REG, BIOS_RESET);
	outb(CMOS_DATA, mpbiosreason);

	/* number of APs actually started */
	return mp_naps;
}


/*
 * This function starts the AP (application processor) identified
 * by the APIC ID 'physicalCpu'.  It does quite a "song and dance"
 * to accomplish this.  This is necessary because of the nuances
 * of the different hardware we might encounter.  It isn't pretty,
 * but it seems to work.
 */
static int
start_ap(int apic_id)
{
	int vector, ms;
	int cpus;

	/* calculate the vector */
	vector = (boot_address >> 12) & 0xff;

	/* used as a watchpoint to signal AP startup */
	cpus = mp_naps;

	/*
	 * first we do an INIT/RESET IPI this INIT IPI might be run, reseting
	 * and running the target CPU. OR this INIT IPI might be latched (P5
	 * bug), CPU waiting for STARTUP IPI. OR this INIT IPI might be
	 * ignored.
	 */

	/* do an INIT IPI: assert RESET */
	lapic_ipi_raw(APIC_DEST_DESTFLD | APIC_TRIGMOD_EDGE |
	    APIC_LEVEL_ASSERT | APIC_DESTMODE_PHY | APIC_DELMODE_INIT, apic_id);

	/* wait for pending status end */
	lapic_ipi_wait(-1);

	/* do an INIT IPI: deassert RESET */
	lapic_ipi_raw(APIC_DEST_ALLESELF | APIC_TRIGMOD_LEVEL |
	    APIC_LEVEL_DEASSERT | APIC_DESTMODE_PHY | APIC_DELMODE_INIT, 0);

	/* wait for pending status end */
	DELAY(10000);		/* wait ~10mS */
	lapic_ipi_wait(-1);

	/*
	 * next we do a STARTUP IPI: the previous INIT IPI might still be
	 * latched, (P5 bug) this 1st STARTUP would then terminate
	 * immediately, and the previously started INIT IPI would continue. OR
	 * the previous INIT IPI has already run. and this STARTUP IPI will
	 * run. OR the previous INIT IPI was ignored. and this STARTUP IPI
	 * will run.
	 */

	/* do a STARTUP IPI */
	lapic_ipi_raw(APIC_DEST_DESTFLD | APIC_TRIGMOD_EDGE |
	    APIC_LEVEL_DEASSERT | APIC_DESTMODE_PHY | APIC_DELMODE_STARTUP |
	    vector, apic_id);
	lapic_ipi_wait(-1);
	DELAY(200);		/* wait ~200uS */

	/*
	 * finally we do a 2nd STARTUP IPI: this 2nd STARTUP IPI should run IF
	 * the previous STARTUP IPI was cancelled by a latched INIT IPI. OR
	 * this STARTUP IPI will be ignored, as only ONE STARTUP IPI is
	 * recognized after hardware RESET or INIT IPI.
	 */

	lapic_ipi_raw(APIC_DEST_DESTFLD | APIC_TRIGMOD_EDGE |
	    APIC_LEVEL_DEASSERT | APIC_DESTMODE_PHY | APIC_DELMODE_STARTUP |
	    vector, apic_id);
	lapic_ipi_wait(-1);
	DELAY(200);		/* wait ~200uS */

	/* Wait up to 5 seconds for it to start. */
	for (ms = 0; ms < 5000; ms++) {
		if (mp_naps > cpus)
			return 1;	/* return SUCCESS */
		DELAY(1000);
	}
	return 0;		/* return FAILURE */
}

#ifdef COUNT_XINVLTLB_HITS
u_int xhits_gbl[MAXCPU];
u_int xhits_pg[MAXCPU];
u_int xhits_rng[MAXCPU];
SYSCTL_NODE(_debug, OID_AUTO, xhits, CTLFLAG_RW, 0, "");
SYSCTL_OPAQUE(_debug_xhits, OID_AUTO, global, CTLFLAG_RW, &xhits_gbl,
    sizeof(xhits_gbl), "IU", "");
SYSCTL_OPAQUE(_debug_xhits, OID_AUTO, page, CTLFLAG_RW, &xhits_pg,
    sizeof(xhits_pg), "IU", "");
SYSCTL_OPAQUE(_debug_xhits, OID_AUTO, range, CTLFLAG_RW, &xhits_rng,
    sizeof(xhits_rng), "IU", "");

u_int ipi_global;
u_int ipi_page;
u_int ipi_range;
u_int ipi_range_size;
SYSCTL_INT(_debug_xhits, OID_AUTO, ipi_global, CTLFLAG_RW, &ipi_global, 0, "");
SYSCTL_INT(_debug_xhits, OID_AUTO, ipi_page, CTLFLAG_RW, &ipi_page, 0, "");
SYSCTL_INT(_debug_xhits, OID_AUTO, ipi_range, CTLFLAG_RW, &ipi_range, 0, "");
SYSCTL_INT(_debug_xhits, OID_AUTO, ipi_range_size, CTLFLAG_RW, &ipi_range_size,
    0, "");

u_int ipi_masked_global;
u_int ipi_masked_page;
u_int ipi_masked_range;
u_int ipi_masked_range_size;
SYSCTL_INT(_debug_xhits, OID_AUTO, ipi_masked_global, CTLFLAG_RW,
    &ipi_masked_global, 0, "");
SYSCTL_INT(_debug_xhits, OID_AUTO, ipi_masked_page, CTLFLAG_RW,
    &ipi_masked_page, 0, "");
SYSCTL_INT(_debug_xhits, OID_AUTO, ipi_masked_range, CTLFLAG_RW,
    &ipi_masked_range, 0, "");
SYSCTL_INT(_debug_xhits, OID_AUTO, ipi_masked_range_size, CTLFLAG_RW,
    &ipi_masked_range_size, 0, "");
#endif /* COUNT_XINVLTLB_HITS */

/*
 * Flush the TLB on all other CPU's
 */
static void
smp_tlb_shootdown(u_int vector, vm_offset_t addr1, vm_offset_t addr2)
{
	u_int ncpu;

	ncpu = mp_ncpus - 1;	/* does not shootdown self */
	if (ncpu < 1)
		return;		/* no other cpus */
	if (!(read_rflags() & PSL_I))
		panic("%s: interrupts disabled", __func__);
	mtx_lock_spin(&smp_ipi_mtx);
	smp_tlb_addr1 = addr1;
	smp_tlb_addr2 = addr2;
	atomic_store_rel_int(&smp_tlb_wait, 0);
	ipi_all_but_self(vector);
	while (smp_tlb_wait < ncpu)
		ia32_pause();
	mtx_unlock_spin(&smp_ipi_mtx);
}

static void
smp_targeted_tlb_shootdown(cpumask_t mask, u_int vector, vm_offset_t addr1, vm_offset_t addr2)
{
	int ncpu, othercpus;

	othercpus = mp_ncpus - 1;
	if (mask == (cpumask_t)-1) {
		ncpu = othercpus;
		if (ncpu < 1)
			return;
	} else {
		mask &= ~PCPU_GET(cpumask);
		if (mask == 0)
			return;
		ncpu = bitcount32(mask);
		if (ncpu > othercpus) {
			/* XXX this should be a panic offence */
			printf("SMP: tlb shootdown to %d other cpus (only have %d)\n",
			    ncpu, othercpus);
			ncpu = othercpus;
		}
		/* XXX should be a panic, implied by mask == 0 above */
		if (ncpu < 1)
			return;
	}
	if (!(read_rflags() & PSL_I))
		panic("%s: interrupts disabled", __func__);
	mtx_lock_spin(&smp_ipi_mtx);
	smp_tlb_addr1 = addr1;
	smp_tlb_addr2 = addr2;
	atomic_store_rel_int(&smp_tlb_wait, 0);
	if (mask == (cpumask_t)-1)
		ipi_all_but_self(vector);
	else
		ipi_selected(mask, vector);
	while (smp_tlb_wait < ncpu)
		ia32_pause();
	mtx_unlock_spin(&smp_ipi_mtx);
}

/*
 * Send an IPI to specified CPU handling the bitmap logic.
 */
static void
ipi_send_cpu(int cpu, u_int ipi)
{
	u_int bitmap, old_pending, new_pending;

	KASSERT(cpu_apic_ids[cpu] != -1, ("IPI to non-existent CPU %d", cpu));

	if (IPI_IS_BITMAPED(ipi)) {
		bitmap = 1 << ipi;
		ipi = IPI_BITMAP_VECTOR;
		do {
			old_pending = cpu_ipi_pending[cpu];
			new_pending = old_pending | bitmap;
		} while  (!atomic_cmpset_int(&cpu_ipi_pending[cpu],
		    old_pending, new_pending)); 
		if (old_pending)
			return;
	}
	lapic_ipi_vectored(ipi, cpu_apic_ids[cpu]);
}

void
smp_cache_flush(void)
{

	if (smp_started)
		smp_tlb_shootdown(IPI_INVLCACHE, 0, 0);
}

void
smp_invltlb(void)
{

	if (smp_started) {
		smp_tlb_shootdown(IPI_INVLTLB, 0, 0);
#ifdef COUNT_XINVLTLB_HITS
		ipi_global++;
#endif
	}
}

void
smp_invlpg(vm_offset_t addr)
{

	if (smp_started) {
		smp_tlb_shootdown(IPI_INVLPG, addr, 0);
#ifdef COUNT_XINVLTLB_HITS
		ipi_page++;
#endif
	}
}

void
smp_invlpg_range(vm_offset_t addr1, vm_offset_t addr2)
{

	if (smp_started) {
		smp_tlb_shootdown(IPI_INVLRNG, addr1, addr2);
#ifdef COUNT_XINVLTLB_HITS
		ipi_range++;
		ipi_range_size += (addr2 - addr1) / PAGE_SIZE;
#endif
	}
}

void
smp_masked_invltlb(cpumask_t mask)
{

	if (smp_started) {
		smp_targeted_tlb_shootdown(mask, IPI_INVLTLB, 0, 0);
#ifdef COUNT_XINVLTLB_HITS
		ipi_masked_global++;
#endif
	}
}

void
smp_masked_invlpg(cpumask_t mask, vm_offset_t addr)
{

	if (smp_started) {
		smp_targeted_tlb_shootdown(mask, IPI_INVLPG, addr, 0);
#ifdef COUNT_XINVLTLB_HITS
		ipi_masked_page++;
#endif
	}
}

void
smp_masked_invlpg_range(cpumask_t mask, vm_offset_t addr1, vm_offset_t addr2)
{

	if (smp_started) {
		smp_targeted_tlb_shootdown(mask, IPI_INVLRNG, addr1, addr2);
#ifdef COUNT_XINVLTLB_HITS
		ipi_masked_range++;
		ipi_masked_range_size += (addr2 - addr1) / PAGE_SIZE;
#endif
	}
}

void
ipi_bitmap_handler(struct trapframe frame)
{
	struct trapframe *oldframe;
	struct thread *td;
	int cpu = PCPU_GET(cpuid);
	u_int ipi_bitmap;

	critical_enter();
	td = curthread;
	td->td_intr_nesting_level++;
	oldframe = td->td_intr_frame;
	td->td_intr_frame = &frame;
	ipi_bitmap = atomic_readandclear_int(&cpu_ipi_pending[cpu]);
	if (ipi_bitmap & (1 << IPI_PREEMPT)) {
#ifdef COUNT_IPIS
		(*ipi_preempt_counts[cpu])++;
#endif
		sched_preempt(td);
	}
	if (ipi_bitmap & (1 << IPI_AST)) {
#ifdef COUNT_IPIS
		(*ipi_ast_counts[cpu])++;
#endif
		/* Nothing to do for AST */
	}
	if (ipi_bitmap & (1 << IPI_HARDCLOCK)) {
#ifdef COUNT_IPIS
		(*ipi_hardclock_counts[cpu])++;
#endif
		hardclockintr();
	}
	td->td_intr_frame = oldframe;
	td->td_intr_nesting_level--;
	critical_exit();
}

/*
 * send an IPI to a set of cpus.
 */
void
ipi_selected(cpumask_t cpus, u_int ipi)
{
	int cpu;

	/*
	 * IPI_STOP_HARD maps to a NMI and the trap handler needs a bit
	 * of help in order to understand what is the source.
	 * Set the mask of receiving CPUs for this purpose.
	 */
	if (ipi == IPI_STOP_HARD)
		atomic_set_int(&ipi_nmi_pending, cpus);

	CTR3(KTR_SMP, "%s: cpus: %x ipi: %x", __func__, cpus, ipi);
	while ((cpu = ffs(cpus)) != 0) {
		cpu--;
		cpus &= ~(1 << cpu);
		ipi_send_cpu(cpu, ipi);
	}
}

/*
 * send an IPI to a specific CPU.
 */
void
ipi_cpu(int cpu, u_int ipi)
{

	/*
	 * IPI_STOP_HARD maps to a NMI and the trap handler needs a bit
	 * of help in order to understand what is the source.
	 * Set the mask of receiving CPUs for this purpose.
	 */
	if (ipi == IPI_STOP_HARD)
		atomic_set_int(&ipi_nmi_pending, 1 << cpu);

	CTR3(KTR_SMP, "%s: cpu: %d ipi: %x", __func__, cpu, ipi);
	ipi_send_cpu(cpu, ipi);
}

/*
 * send an IPI to all CPUs EXCEPT myself
 */
void
ipi_all_but_self(u_int ipi)
{

	if (IPI_IS_BITMAPED(ipi)) {
		ipi_selected(PCPU_GET(other_cpus), ipi);
		return;
	}

	/*
	 * IPI_STOP_HARD maps to a NMI and the trap handler needs a bit
	 * of help in order to understand what is the source.
	 * Set the mask of receiving CPUs for this purpose.
	 */
	if (ipi == IPI_STOP_HARD)
		atomic_set_int(&ipi_nmi_pending, PCPU_GET(other_cpus));

	CTR2(KTR_SMP, "%s: ipi: %x", __func__, ipi);
	lapic_ipi_vectored(ipi, APIC_IPI_DEST_OTHERS);
}

int
ipi_nmi_handler()
{
	cpumask_t cpumask;

	/*
	 * As long as there is not a simple way to know about a NMI's
	 * source, if the bitmask for the current CPU is present in
	 * the global pending bitword an IPI_STOP_HARD has been issued
	 * and should be handled.
	 */
	cpumask = PCPU_GET(cpumask);
	if ((ipi_nmi_pending & cpumask) == 0)
		return (1);

	atomic_clear_int(&ipi_nmi_pending, cpumask);
	cpustop_handler();
	return (0);
}
     
/*
 * Handle an IPI_STOP by saving our current context and spinning until we
 * are resumed.
 */
void
cpustop_handler(void)
{
	cpumask_t cpumask;
	u_int cpu;

	cpu = PCPU_GET(cpuid);
	cpumask = PCPU_GET(cpumask);

	savectx(&stoppcbs[cpu]);

	/* Indicate that we are stopped */
	atomic_set_int(&stopped_cpus, cpumask);

	/* Wait for restart */
	while (!(started_cpus & cpumask))
	    ia32_pause();

	atomic_clear_int(&started_cpus, cpumask);
	atomic_clear_int(&stopped_cpus, cpumask);

	if (cpu == 0 && cpustop_restartfunc != NULL) {
		cpustop_restartfunc();
		cpustop_restartfunc = NULL;
	}
}

/*
 * Handle an IPI_SUSPEND by saving our current context and spinning until we
 * are resumed.
 */
void
cpususpend_handler(void)
{
	cpumask_t cpumask;
	register_t cr3, rf;
	u_int cpu;

	cpu = PCPU_GET(cpuid);
	cpumask = PCPU_GET(cpumask);

	rf = intr_disable();
	cr3 = rcr3();

	if (savectx(susppcbs[cpu])) {
		wbinvd();
		atomic_set_int(&stopped_cpus, cpumask);
	} else {
		PCPU_SET(switchtime, 0);
		PCPU_SET(switchticks, ticks);
	}

	/* Wait for resume */
	while (!(started_cpus & cpumask))
		ia32_pause();

	atomic_clear_int(&started_cpus, cpumask);
	atomic_clear_int(&stopped_cpus, cpumask);

	/* Restore CR3 and enable interrupts */
	load_cr3(cr3);
	mca_resume();
	lapic_setup(0);
	intr_restore(rf);
}

/*
 * This is called once the rest of the system is up and running and we're
 * ready to let the AP's out of the pen.
 */
static void
release_aps(void *dummy __unused)
{

	if (mp_ncpus == 1) 
		return;
	atomic_store_rel_int(&aps_ready, 1);
	while (smp_started == 0)
		ia32_pause();
}
SYSINIT(start_aps, SI_SUB_SMP, SI_ORDER_FIRST, release_aps, NULL);

static int
sysctl_hlt_cpus(SYSCTL_HANDLER_ARGS)
{
	cpumask_t mask;
	int error;

	mask = hlt_cpus_mask;
	error = sysctl_handle_int(oidp, &mask, 0, req);
	if (error || !req->newptr)
		return (error);

	if (logical_cpus_mask != 0 &&
	    (mask & logical_cpus_mask) == logical_cpus_mask)
		hlt_logical_cpus = 1;
	else
		hlt_logical_cpus = 0;

	if (! hyperthreading_allowed)
		mask |= hyperthreading_cpus_mask;

	if ((mask & all_cpus) == all_cpus)
		mask &= ~(1<<0);
	hlt_cpus_mask = mask;
	return (error);
}
SYSCTL_PROC(_machdep, OID_AUTO, hlt_cpus, CTLTYPE_INT|CTLFLAG_RW,
    0, 0, sysctl_hlt_cpus, "IU",
    "Bitmap of CPUs to halt.  101 (binary) will halt CPUs 0 and 2.");

static int
sysctl_hlt_logical_cpus(SYSCTL_HANDLER_ARGS)
{
	int disable, error;

	disable = hlt_logical_cpus;
	error = sysctl_handle_int(oidp, &disable, 0, req);
	if (error || !req->newptr)
		return (error);

	if (disable)
		hlt_cpus_mask |= logical_cpus_mask;
	else
		hlt_cpus_mask &= ~logical_cpus_mask;

	if (! hyperthreading_allowed)
		hlt_cpus_mask |= hyperthreading_cpus_mask;

	if ((hlt_cpus_mask & all_cpus) == all_cpus)
		hlt_cpus_mask &= ~(1<<0);

	hlt_logical_cpus = disable;
	return (error);
}

static int
sysctl_hyperthreading_allowed(SYSCTL_HANDLER_ARGS)
{
	int allowed, error;

	allowed = hyperthreading_allowed;
	error = sysctl_handle_int(oidp, &allowed, 0, req);
	if (error || !req->newptr)
		return (error);

#ifdef SCHED_ULE
	/*
	 * SCHED_ULE doesn't allow enabling/disabling HT cores at
	 * run-time.
	 */
	if (allowed != hyperthreading_allowed)
		return (ENOTSUP);
	return (error);
#endif

	if (allowed)
		hlt_cpus_mask &= ~hyperthreading_cpus_mask;
	else
		hlt_cpus_mask |= hyperthreading_cpus_mask;

	if (logical_cpus_mask != 0 &&
	    (hlt_cpus_mask & logical_cpus_mask) == logical_cpus_mask)
		hlt_logical_cpus = 1;
	else
		hlt_logical_cpus = 0;

	if ((hlt_cpus_mask & all_cpus) == all_cpus)
		hlt_cpus_mask &= ~(1<<0);

	hyperthreading_allowed = allowed;
	return (error);
}

static void
cpu_hlt_setup(void *dummy __unused)
{

	if (logical_cpus_mask != 0) {
		TUNABLE_INT_FETCH("machdep.hlt_logical_cpus",
		    &hlt_logical_cpus);
		sysctl_ctx_init(&logical_cpu_clist);
		SYSCTL_ADD_PROC(&logical_cpu_clist,
		    SYSCTL_STATIC_CHILDREN(_machdep), OID_AUTO,
		    "hlt_logical_cpus", CTLTYPE_INT|CTLFLAG_RW, 0, 0,
		    sysctl_hlt_logical_cpus, "IU", "");
		SYSCTL_ADD_UINT(&logical_cpu_clist,
		    SYSCTL_STATIC_CHILDREN(_machdep), OID_AUTO,
		    "logical_cpus_mask", CTLTYPE_INT|CTLFLAG_RD,
		    &logical_cpus_mask, 0, "");

		if (hlt_logical_cpus)
			hlt_cpus_mask |= logical_cpus_mask;

		/*
		 * If necessary for security purposes, force
		 * hyperthreading off, regardless of the value
		 * of hlt_logical_cpus.
		 */
		if (hyperthreading_cpus_mask) {
			SYSCTL_ADD_PROC(&logical_cpu_clist,
			    SYSCTL_STATIC_CHILDREN(_machdep), OID_AUTO,
			    "hyperthreading_allowed", CTLTYPE_INT|CTLFLAG_RW,
			    0, 0, sysctl_hyperthreading_allowed, "IU", "");
			if (! hyperthreading_allowed)
				hlt_cpus_mask |= hyperthreading_cpus_mask;
		}
	}
}
SYSINIT(cpu_hlt, SI_SUB_SMP, SI_ORDER_ANY, cpu_hlt_setup, NULL);

int
mp_grab_cpu_hlt(void)
{
	cpumask_t mask;
#ifdef MP_WATCHDOG
	u_int cpuid;
#endif
	int retval;

	mask = PCPU_GET(cpumask);
#ifdef MP_WATCHDOG
	cpuid = PCPU_GET(cpuid);
	ap_watchdog(cpuid);
#endif

	retval = 0;
	while (mask & hlt_cpus_mask) {
		retval = 1;
		__asm __volatile("sti; hlt" : : : "memory");
	}
	return (retval);
}

#ifdef COUNT_IPIS
/*
 * Setup interrupt counters for IPI handlers.
 */
static void
mp_ipi_intrcnt(void *dummy)
{
	char buf[64];
	int i;

	CPU_FOREACH(i) {
		snprintf(buf, sizeof(buf), "cpu%d:invltlb", i);
		intrcnt_add(buf, &ipi_invltlb_counts[i]);
		snprintf(buf, sizeof(buf), "cpu%d:invlrng", i);
		intrcnt_add(buf, &ipi_invlrng_counts[i]);
		snprintf(buf, sizeof(buf), "cpu%d:invlpg", i);
		intrcnt_add(buf, &ipi_invlpg_counts[i]);
		snprintf(buf, sizeof(buf), "cpu%d:preempt", i);
		intrcnt_add(buf, &ipi_preempt_counts[i]);
		snprintf(buf, sizeof(buf), "cpu%d:ast", i);
		intrcnt_add(buf, &ipi_ast_counts[i]);
		snprintf(buf, sizeof(buf), "cpu%d:rendezvous", i);
		intrcnt_add(buf, &ipi_rendezvous_counts[i]);
		snprintf(buf, sizeof(buf), "cpu%d:lazypmap", i);
		intrcnt_add(buf, &ipi_lazypmap_counts[i]);
		snprintf(buf, sizeof(buf), "cpu%d:hardclock", i);
		intrcnt_add(buf, &ipi_hardclock_counts[i]);
	}
}
SYSINIT(mp_ipi_intrcnt, SI_SUB_INTR, SI_ORDER_MIDDLE, mp_ipi_intrcnt, NULL);
#endif

