/*	$OpenBSD: linux_sysent.c,v 1.68 2011/09/19 22:49:57 pirofti Exp $	*/

/*
 * System call switch table.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	OpenBSD: syscalls.master,v 1.63 2011/09/19 14:33:14 pirofti Exp 
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <compat/linux/linux_types.h>
#include <compat/linux/linux_signal.h>
#include <compat/linux/linux_misc.h>
#include <compat/linux/linux_syscallargs.h>
#include <machine/linux_machdep.h>

#define	s(type)	sizeof(type)

struct sysent linux_sysent[] = {
	{ 0, 0, 0,
	    sys_nosys },			/* 0 = syscall */
	{ 1, s(struct sys_exit_args), 0,
	    sys_exit },				/* 1 = exit */
	{ 0, 0, 0,
	    sys_fork },				/* 2 = fork */
	{ 3, s(struct sys_read_args), 0,
	    sys_read },				/* 3 = read */
	{ 3, s(struct sys_write_args), 0,
	    sys_write },			/* 4 = write */
	{ 3, s(struct linux_sys_open_args), 0,
	    linux_sys_open },			/* 5 = open */
	{ 1, s(struct sys_close_args), 0,
	    sys_close },			/* 6 = close */
	{ 3, s(struct linux_sys_waitpid_args), 0,
	    linux_sys_waitpid },		/* 7 = waitpid */
	{ 2, s(struct linux_sys_creat_args), 0,
	    linux_sys_creat },			/* 8 = creat */
	{ 2, s(struct sys_link_args), 0,
	    sys_link },				/* 9 = link */
	{ 1, s(struct linux_sys_unlink_args), 0,
	    linux_sys_unlink },			/* 10 = unlink */
	{ 3, s(struct linux_sys_execve_args), 0,
	    linux_sys_execve },			/* 11 = execve */
	{ 1, s(struct linux_sys_chdir_args), 0,
	    linux_sys_chdir },			/* 12 = chdir */
	{ 1, s(struct linux_sys_time_args), 0,
	    linux_sys_time },			/* 13 = time */
	{ 3, s(struct linux_sys_mknod_args), 0,
	    linux_sys_mknod },			/* 14 = mknod */
	{ 2, s(struct linux_sys_chmod_args), 0,
	    linux_sys_chmod },			/* 15 = chmod */
	{ 3, s(struct linux_sys_lchown16_args), 0,
	    linux_sys_lchown16 },		/* 16 = lchown16 */
	{ 1, s(struct linux_sys_break_args), 0,
	    linux_sys_break },			/* 17 = break */
	{ 0, 0, 0,
	    linux_sys_ostat },			/* 18 = ostat */
	{ 3, s(struct linux_sys_lseek_args), 0,
	    linux_sys_lseek },			/* 19 = lseek */
	{ 0, 0, 0,
	    linux_sys_getpid },			/* 20 = getpid */
	{ 5, s(struct linux_sys_mount_args), 0,
	    linux_sys_mount },			/* 21 = mount */
	{ 1, s(struct linux_sys_umount_args), 0,
	    linux_sys_umount },			/* 22 = umount */
	{ 1, s(struct sys_setuid_args), 0,
	    sys_setuid },			/* 23 = linux_setuid16 */
	{ 0, 0, 0,
	    linux_sys_getuid },			/* 24 = linux_getuid16 */
	{ 1, s(struct linux_sys_stime_args), 0,
	    linux_sys_stime },			/* 25 = stime */
#ifdef PTRACE
	{ 0, 0, 0,
	    linux_sys_ptrace },			/* 26 = ptrace */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 26 = unimplemented ptrace */
#endif
	{ 1, s(struct linux_sys_alarm_args), 0,
	    linux_sys_alarm },			/* 27 = alarm */
	{ 0, 0, 0,
	    linux_sys_ofstat },			/* 28 = ofstat */
	{ 0, 0, 0,
	    linux_sys_pause },			/* 29 = pause */
	{ 2, s(struct linux_sys_utime_args), 0,
	    linux_sys_utime },			/* 30 = utime */
	{ 0, 0, 0,
	    linux_sys_stty },			/* 31 = stty */
	{ 0, 0, 0,
	    linux_sys_gtty },			/* 32 = gtty */
	{ 2, s(struct linux_sys_access_args), 0,
	    linux_sys_access },			/* 33 = access */
	{ 1, s(struct linux_sys_nice_args), 0,
	    linux_sys_nice },			/* 34 = nice */
	{ 0, 0, 0,
	    linux_sys_ftime },			/* 35 = ftime */
	{ 0, 0, 0,
	    sys_sync },				/* 36 = sync */
	{ 2, s(struct linux_sys_kill_args), 0,
	    linux_sys_kill },			/* 37 = kill */
	{ 2, s(struct linux_sys_rename_args), 0,
	    linux_sys_rename },			/* 38 = rename */
	{ 2, s(struct linux_sys_mkdir_args), 0,
	    linux_sys_mkdir },			/* 39 = mkdir */
	{ 1, s(struct linux_sys_rmdir_args), 0,
	    linux_sys_rmdir },			/* 40 = rmdir */
	{ 1, s(struct sys_dup_args), 0,
	    sys_dup },				/* 41 = dup */
	{ 1, s(struct sys_pipe_args), 0,
	    sys_pipe },				/* 42 = pipe */
	{ 1, s(struct linux_sys_times_args), 0,
	    linux_sys_times },			/* 43 = times */
	{ 0, 0, 0,
	    linux_sys_prof },			/* 44 = prof */
	{ 1, s(struct linux_sys_brk_args), 0,
	    linux_sys_brk },			/* 45 = brk */
	{ 1, s(struct sys_setgid_args), 0,
	    sys_setgid },			/* 46 = linux_setgid16 */
	{ 0, 0, 0,
	    linux_sys_getgid },			/* 47 = linux_getgid16 */
	{ 2, s(struct linux_sys_signal_args), 0,
	    linux_sys_signal },			/* 48 = signal */
	{ 0, 0, 0,
	    sys_geteuid },			/* 49 = linux_geteuid16 */
	{ 0, 0, 0,
	    sys_getegid },			/* 50 = linux_getegid16 */
#ifdef ACCOUNTING
	{ 1, s(struct sys_acct_args), 0,
	    sys_acct },				/* 51 = acct */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 51 = unimplemented acct */
#endif
	{ 0, 0, 0,
	    linux_sys_phys },			/* 52 = phys */
	{ 0, 0, 0,
	    linux_sys_lock },			/* 53 = lock */
	{ 3, s(struct linux_sys_ioctl_args), 0,
	    linux_sys_ioctl },			/* 54 = ioctl */
	{ 3, s(struct linux_sys_fcntl_args), 0,
	    linux_sys_fcntl },			/* 55 = fcntl */
	{ 0, 0, 0,
	    linux_sys_mpx },			/* 56 = mpx */
	{ 2, s(struct sys_setpgid_args), 0,
	    sys_setpgid },			/* 57 = setpgid */
	{ 0, 0, 0,
	    linux_sys_ulimit },			/* 58 = ulimit */
	{ 1, s(struct linux_sys_oldolduname_args), 0,
	    linux_sys_oldolduname },		/* 59 = oldolduname */
	{ 1, s(struct sys_umask_args), 0,
	    sys_umask },			/* 60 = umask */
	{ 1, s(struct sys_chroot_args), 0,
	    sys_chroot },			/* 61 = chroot */
	{ 0, 0, 0,
	    linux_sys_ustat },			/* 62 = ustat */
	{ 2, s(struct sys_dup2_args), 0,
	    sys_dup2 },				/* 63 = dup2 */
	{ 0, 0, 0,
	    sys_getppid },			/* 64 = getppid */
	{ 0, 0, 0,
	    sys_getpgrp },			/* 65 = getpgrp */
	{ 0, 0, 0,
	    sys_setsid },			/* 66 = setsid */
	{ 3, s(struct linux_sys_sigaction_args), 0,
	    linux_sys_sigaction },		/* 67 = sigaction */
	{ 0, 0, 0,
	    linux_sys_siggetmask },		/* 68 = siggetmask */
	{ 1, s(struct linux_sys_sigsetmask_args), 0,
	    linux_sys_sigsetmask },		/* 69 = sigsetmask */
	{ 2, s(struct linux_sys_setreuid16_args), 0,
	    linux_sys_setreuid16 },		/* 70 = setreuid16 */
	{ 2, s(struct linux_sys_setregid16_args), 0,
	    linux_sys_setregid16 },		/* 71 = setregid16 */
	{ 3, s(struct linux_sys_sigsuspend_args), 0,
	    linux_sys_sigsuspend },		/* 72 = sigsuspend */
	{ 1, s(struct linux_sys_sigpending_args), 0,
	    linux_sys_sigpending },		/* 73 = sigpending */
	{ 2, s(struct linux_sys_sethostname_args), 0,
	    linux_sys_sethostname },		/* 74 = sethostname */
	{ 2, s(struct linux_sys_setrlimit_args), 0,
	    linux_sys_setrlimit },		/* 75 = setrlimit */
	{ 2, s(struct linux_sys_getrlimit_args), 0,
	    linux_sys_getrlimit },		/* 76 = getrlimit */
	{ 2, s(struct sys_getrusage_args), 0,
	    sys_getrusage },			/* 77 = getrusage */
	{ 2, s(struct sys_gettimeofday_args), 0,
	    sys_gettimeofday },			/* 78 = gettimeofday */
	{ 2, s(struct sys_settimeofday_args), 0,
	    sys_settimeofday },			/* 79 = settimeofday */
	{ 2, s(struct sys_getgroups_args), 0,
	    sys_getgroups },			/* 80 = linux_getgroups */
	{ 2, s(struct sys_setgroups_args), 0,
	    sys_setgroups },			/* 81 = linux_setgroups */
	{ 1, s(struct linux_sys_oldselect_args), 0,
	    linux_sys_oldselect },		/* 82 = oldselect */
	{ 2, s(struct linux_sys_symlink_args), 0,
	    linux_sys_symlink },		/* 83 = symlink */
	{ 2, s(struct linux_sys_lstat_args), 0,
	    linux_sys_lstat },			/* 84 = olstat */
	{ 3, s(struct linux_sys_readlink_args), 0,
	    linux_sys_readlink },		/* 85 = readlink */
	{ 1, s(struct linux_sys_uselib_args), 0,
	    linux_sys_uselib },			/* 86 = uselib */
	{ 1, s(struct linux_sys_swapon_args), 0,
	    linux_sys_swapon },			/* 87 = swapon */
	{ 1, s(struct sys_reboot_args), 0,
	    sys_reboot },			/* 88 = reboot */
	{ 3, s(struct linux_sys_readdir_args), 0,
	    linux_sys_readdir },		/* 89 = readdir */
	{ 1, s(struct linux_sys_mmap_args), 0,
	    linux_sys_mmap },			/* 90 = mmap */
	{ 2, s(struct sys_munmap_args), 0,
	    sys_munmap },			/* 91 = munmap */
	{ 2, s(struct linux_sys_truncate_args), 0,
	    linux_sys_truncate },		/* 92 = truncate */
	{ 2, s(struct linux_sys_ftruncate_args), 0,
	    linux_sys_ftruncate },		/* 93 = ftruncate */
	{ 2, s(struct sys_fchmod_args), 0,
	    sys_fchmod },			/* 94 = fchmod */
	{ 3, s(struct linux_sys_fchown16_args), 0,
	    linux_sys_fchown16 },		/* 95 = fchown16 */
	{ 2, s(struct sys_getpriority_args), 0,
	    sys_getpriority },			/* 96 = getpriority */
	{ 3, s(struct sys_setpriority_args), 0,
	    sys_setpriority },			/* 97 = setpriority */
	{ 4, s(struct sys_profil_args), 0,
	    sys_profil },			/* 98 = profil */
	{ 2, s(struct linux_sys_statfs_args), 0,
	    linux_sys_statfs },			/* 99 = statfs */
	{ 2, s(struct linux_sys_fstatfs_args), 0,
	    linux_sys_fstatfs },		/* 100 = fstatfs */
#ifdef __i386__
	{ 3, s(struct linux_sys_ioperm_args), 0,
	    linux_sys_ioperm },			/* 101 = ioperm */
#else
	{ 0, 0, 0,
	    linux_sys_ioperm },			/* 101 = ioperm */
#endif
	{ 2, s(struct linux_sys_socketcall_args), 0,
	    linux_sys_socketcall },		/* 102 = socketcall */
	{ 0, 0, 0,
	    linux_sys_klog },			/* 103 = klog */
	{ 3, s(struct sys_setitimer_args), 0,
	    sys_setitimer },			/* 104 = setitimer */
	{ 2, s(struct sys_getitimer_args), 0,
	    sys_getitimer },			/* 105 = getitimer */
	{ 2, s(struct linux_sys_stat_args), 0,
	    linux_sys_stat },			/* 106 = stat */
	{ 2, s(struct linux_sys_lstat_args), 0,
	    linux_sys_lstat },			/* 107 = lstat */
	{ 2, s(struct linux_sys_fstat_args), 0,
	    linux_sys_fstat },			/* 108 = fstat */
	{ 1, s(struct linux_sys_olduname_args), 0,
	    linux_sys_olduname },		/* 109 = olduname */
#ifdef __i386__
	{ 1, s(struct linux_sys_iopl_args), 0,
	    linux_sys_iopl },			/* 110 = iopl */
#else
	{ 0, 0, 0,
	    linux_sys_iopl },			/* 110 = iopl */
#endif
	{ 0, 0, 0,
	    linux_sys_vhangup },		/* 111 = vhangup */
	{ 0, 0, 0,
	    linux_sys_idle },			/* 112 = idle */
	{ 0, 0, 0,
	    linux_sys_vm86old },		/* 113 = vm86old */
	{ 4, s(struct linux_sys_wait4_args), 0,
	    linux_sys_wait4 },			/* 114 = wait4 */
	{ 0, 0, 0,
	    linux_sys_swapoff },		/* 115 = swapoff */
	{ 1, s(struct linux_sys_sysinfo_args), 0,
	    linux_sys_sysinfo },		/* 116 = sysinfo */
	{ 5, s(struct linux_sys_ipc_args), 0,
	    linux_sys_ipc },			/* 117 = ipc */
	{ 1, s(struct sys_fsync_args), 0,
	    sys_fsync },			/* 118 = fsync */
	{ 1, s(struct linux_sys_sigreturn_args), 0,
	    linux_sys_sigreturn },		/* 119 = sigreturn */
	{ 5, s(struct linux_sys_clone_args), 0,
	    linux_sys_clone },			/* 120 = clone */
	{ 2, s(struct linux_sys_setdomainname_args), 0,
	    linux_sys_setdomainname },		/* 121 = setdomainname */
	{ 1, s(struct linux_sys_uname_args), 0,
	    linux_sys_uname },			/* 122 = uname */
#ifdef __i386__
	{ 3, s(struct linux_sys_modify_ldt_args), 0,
	    linux_sys_modify_ldt },		/* 123 = modify_ldt */
#else
	{ 0, 0, 0,
	    linux_sys_modify_ldt },		/* 123 = modify_ldt */
#endif
	{ 0, 0, 0,
	    linux_sys_adjtimex },		/* 124 = adjtimex */
	{ 3, s(struct linux_sys_mprotect_args), 0,
	    linux_sys_mprotect },		/* 125 = mprotect */
	{ 3, s(struct linux_sys_sigprocmask_args), 0,
	    linux_sys_sigprocmask },		/* 126 = sigprocmask */
	{ 0, 0, 0,
	    linux_sys_create_module },		/* 127 = create_module */
	{ 0, 0, 0,
	    linux_sys_init_module },		/* 128 = init_module */
	{ 0, 0, 0,
	    linux_sys_delete_module },		/* 129 = delete_module */
	{ 0, 0, 0,
	    linux_sys_get_kernel_syms },	/* 130 = get_kernel_syms */
	{ 0, 0, 0,
	    linux_sys_quotactl },		/* 131 = quotactl */
	{ 1, s(struct linux_sys_getpgid_args), 0,
	    linux_sys_getpgid },		/* 132 = getpgid */
	{ 1, s(struct sys_fchdir_args), 0,
	    sys_fchdir },			/* 133 = fchdir */
	{ 0, 0, 0,
	    linux_sys_bdflush },		/* 134 = bdflush */
	{ 0, 0, 0,
	    linux_sys_sysfs },			/* 135 = sysfs */
	{ 1, s(struct linux_sys_personality_args), 0,
	    linux_sys_personality },		/* 136 = personality */
	{ 0, 0, 0,
	    linux_sys_afs_syscall },		/* 137 = afs_syscall */
	{ 1, s(struct linux_sys_setfsuid_args), 0,
	    linux_sys_setfsuid },		/* 138 = linux_setfsuid16 */
	{ 0, 0, 0,
	    linux_sys_getfsuid },		/* 139 = linux_getfsuid16 */
	{ 5, s(struct linux_sys_llseek_args), 0,
	    linux_sys_llseek },			/* 140 = llseek */
	{ 3, s(struct linux_sys_getdents_args), 0,
	    linux_sys_getdents },		/* 141 = getdents */
	{ 5, s(struct linux_sys_select_args), 0,
	    linux_sys_select },			/* 142 = select */
	{ 2, s(struct sys_flock_args), 0,
	    sys_flock },			/* 143 = flock */
	{ 3, s(struct sys_msync_args), 0,
	    sys_msync },			/* 144 = msync */
	{ 3, s(struct sys_readv_args), 0,
	    sys_readv },			/* 145 = readv */
	{ 3, s(struct sys_writev_args), 0,
	    sys_writev },			/* 146 = writev */
	{ 1, s(struct sys_getsid_args), 0,
	    sys_getsid },			/* 147 = getsid */
	{ 1, s(struct linux_sys_fdatasync_args), 0,
	    linux_sys_fdatasync },		/* 148 = fdatasync */
	{ 1, s(struct linux_sys___sysctl_args), 0,
	    linux_sys___sysctl },		/* 149 = __sysctl */
	{ 2, s(struct sys_mlock_args), 0,
	    sys_mlock },			/* 150 = mlock */
	{ 2, s(struct sys_munlock_args), 0,
	    sys_munlock },			/* 151 = munlock */
	{ 0, 0, 0,
	    linux_sys_mlockall },		/* 152 = mlockall */
	{ 0, 0, 0,
	    linux_sys_munlockall },		/* 153 = munlockall */
	{ 2, s(struct linux_sys_sched_setparam_args), 0,
	    linux_sys_sched_setparam },		/* 154 = sched_setparam */
	{ 2, s(struct linux_sys_sched_getparam_args), 0,
	    linux_sys_sched_getparam },		/* 155 = sched_getparam */
	{ 3, s(struct linux_sys_sched_setscheduler_args), 0,
	    linux_sys_sched_setscheduler },	/* 156 = sched_setscheduler */
	{ 1, s(struct linux_sys_sched_getscheduler_args), 0,
	    linux_sys_sched_getscheduler },	/* 157 = sched_getscheduler */
	{ 0, 0, 0,
	    linux_sys_sched_yield },		/* 158 = sched_yield */
	{ 1, s(struct linux_sys_sched_get_priority_max_args), 0,
	    linux_sys_sched_get_priority_max },	/* 159 = sched_get_priority_max */
	{ 1, s(struct linux_sys_sched_get_priority_min_args), 0,
	    linux_sys_sched_get_priority_min },	/* 160 = sched_get_priority_min */
	{ 0, 0, 0,
	    linux_sys_sched_rr_get_interval },	/* 161 = sched_rr_get_interval */
	{ 2, s(struct sys_nanosleep_args), 0,
	    sys_nanosleep },			/* 162 = nanosleep */
	{ 4, s(struct linux_sys_mremap_args), 0,
	    linux_sys_mremap },			/* 163 = mremap */
	{ 3, s(struct linux_sys_setresuid16_args), 0,
	    linux_sys_setresuid16 },		/* 164 = setresuid16 */
	{ 3, s(struct linux_sys_getresuid16_args), 0,
	    linux_sys_getresuid16 },		/* 165 = getresuid16 */
	{ 0, 0, 0,
	    linux_sys_vm86 },			/* 166 = vm86 */
	{ 0, 0, 0,
	    linux_sys_query_module },		/* 167 = query_module */
	{ 3, s(struct sys_poll_args), 0,
	    sys_poll },				/* 168 = poll */
	{ 0, 0, 0,
	    linux_sys_nfsservctl },		/* 169 = nfsservctl */
	{ 3, s(struct linux_sys_setresgid16_args), 0,
	    linux_sys_setresgid16 },		/* 170 = setresgid16 */
	{ 3, s(struct linux_sys_getresgid16_args), 0,
	    linux_sys_getresgid16 },		/* 171 = getresgid16 */
	{ 0, 0, 0,
	    linux_sys_prctl },			/* 172 = prctl */
	{ 1, s(struct linux_sys_rt_sigreturn_args), 0,
	    linux_sys_rt_sigreturn },		/* 173 = rt_sigreturn */
	{ 4, s(struct linux_sys_rt_sigaction_args), 0,
	    linux_sys_rt_sigaction },		/* 174 = rt_sigaction */
	{ 4, s(struct linux_sys_rt_sigprocmask_args), 0,
	    linux_sys_rt_sigprocmask },		/* 175 = rt_sigprocmask */
	{ 2, s(struct linux_sys_rt_sigpending_args), 0,
	    linux_sys_rt_sigpending },		/* 176 = rt_sigpending */
	{ 0, 0, 0,
	    linux_sys_rt_sigtimedwait },	/* 177 = rt_sigtimedwait */
	{ 0, 0, 0,
	    linux_sys_rt_queueinfo },		/* 178 = rt_queueinfo */
	{ 2, s(struct linux_sys_rt_sigsuspend_args), 0,
	    linux_sys_rt_sigsuspend },		/* 179 = rt_sigsuspend */
	{ 4, s(struct linux_sys_pread_args), 0,
	    linux_sys_pread },			/* 180 = pread */
	{ 4, s(struct linux_sys_pwrite_args), 0,
	    linux_sys_pwrite },			/* 181 = pwrite */
	{ 3, s(struct linux_sys_chown16_args), 0,
	    linux_sys_chown16 },		/* 182 = chown16 */
	{ 2, s(struct sys___getcwd_args), 0,
	    sys___getcwd },			/* 183 = __getcwd */
	{ 0, 0, 0,
	    linux_sys_capget },			/* 184 = capget */
	{ 0, 0, 0,
	    linux_sys_capset },			/* 185 = capset */
	{ 2, s(struct linux_sys_sigaltstack_args), 0,
	    linux_sys_sigaltstack },		/* 186 = sigaltstack */
	{ 0, 0, 0,
	    linux_sys_sendfile },		/* 187 = sendfile */
	{ 0, 0, 0,
	    linux_sys_getpmsg },		/* 188 = getpmsg */
	{ 0, 0, 0,
	    linux_sys_putpmsg },		/* 189 = putpmsg */
	{ 0, 0, 0,
	    sys_vfork },			/* 190 = vfork */
	{ 2, s(struct linux_sys_ugetrlimit_args), 0,
	    linux_sys_ugetrlimit },		/* 191 = ugetrlimit */
	{ 6, s(struct linux_sys_mmap2_args), 0,
	    linux_sys_mmap2 },			/* 192 = mmap2 */
	{ 2, s(struct linux_sys_truncate64_args), 0,
	    linux_sys_truncate64 },		/* 193 = truncate64 */
	{ 2, s(struct linux_sys_ftruncate64_args), 0,
	    linux_sys_ftruncate64 },		/* 194 = ftruncate64 */
	{ 2, s(struct linux_sys_stat64_args), 0,
	    linux_sys_stat64 },			/* 195 = stat64 */
	{ 2, s(struct linux_sys_lstat64_args), 0,
	    linux_sys_lstat64 },		/* 196 = lstat64 */
	{ 2, s(struct linux_sys_fstat64_args), 0,
	    linux_sys_fstat64 },		/* 197 = fstat64 */
	{ 0, 0, 0,
	    linux_sys_lchown },			/* 198 = lchown */
	{ 0, 0, 0,
	    linux_sys_getuid },			/* 199 = getuid */
	{ 0, 0, 0,
	    linux_sys_getgid },			/* 200 = getgid */
	{ 0, 0, 0,
	    sys_geteuid },			/* 201 = geteuid */
	{ 0, 0, 0,
	    sys_getegid },			/* 202 = getegid */
	{ 2, s(struct sys_setreuid_args), 0,
	    sys_setreuid },			/* 203 = setreuid */
	{ 2, s(struct sys_setregid_args), 0,
	    sys_setregid },			/* 204 = setregid */
	{ 2, s(struct sys_getgroups_args), 0,
	    sys_getgroups },			/* 205 = getgroups */
	{ 2, s(struct sys_setgroups_args), 0,
	    sys_setgroups },			/* 206 = setgroups */
	{ 0, 0, 0,
	    linux_sys_fchown },			/* 207 = fchown */
	{ 3, s(struct sys_setresuid_args), 0,
	    sys_setresuid },			/* 208 = setresuid */
	{ 3, s(struct sys_getresuid_args), 0,
	    sys_getresuid },			/* 209 = getresuid */
	{ 3, s(struct sys_setresgid_args), 0,
	    sys_setresgid },			/* 210 = setresgid */
	{ 3, s(struct sys_getresgid_args), 0,
	    sys_getresgid },			/* 211 = getresgid */
	{ 0, 0, 0,
	    linux_sys_chown },			/* 212 = chown */
	{ 1, s(struct sys_setuid_args), 0,
	    sys_setuid },			/* 213 = setuid */
	{ 1, s(struct sys_setgid_args), 0,
	    sys_setgid },			/* 214 = setgid */
	{ 1, s(struct linux_sys_setfsuid_args), 0,
	    linux_sys_setfsuid },		/* 215 = setfsuid */
	{ 0, 0, 0,
	    linux_sys_setfsgid },		/* 216 = setfsgid */
	{ 0, 0, 0,
	    linux_sys_pivot_root },		/* 217 = pivot_root */
	{ 0, 0, 0,
	    linux_sys_mincore },		/* 218 = mincore */
	{ 3, s(struct sys_madvise_args), 0,
	    sys_madvise },			/* 219 = madvise */
	{ 3, s(struct linux_sys_getdents64_args), 0,
	    linux_sys_getdents64 },		/* 220 = getdents64 */
	{ 3, s(struct linux_sys_fcntl64_args), 0,
	    linux_sys_fcntl64 },		/* 221 = fcntl64 */
	{ 0, 0, 0,
	    sys_nosys },			/* 222 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 223 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 224 = unimplemented linux_sys_gettid */
	{ 0, 0, 0,
	    sys_nosys },			/* 225 = unimplemented linux_sys_readahead */
	{ 0, 0, 0,
	    linux_sys_setxattr },		/* 226 = setxattr */
	{ 0, 0, 0,
	    linux_sys_lsetxattr },		/* 227 = lsetxattr */
	{ 0, 0, 0,
	    linux_sys_fsetxattr },		/* 228 = fsetxattr */
	{ 0, 0, 0,
	    linux_sys_getxattr },		/* 229 = getxattr */
	{ 0, 0, 0,
	    linux_sys_lgetxattr },		/* 230 = lgetxattr */
	{ 0, 0, 0,
	    linux_sys_fgetxattr },		/* 231 = fgetxattr */
	{ 0, 0, 0,
	    linux_sys_listxattr },		/* 232 = listxattr */
	{ 0, 0, 0,
	    linux_sys_llistxattr },		/* 233 = llistxattr */
	{ 0, 0, 0,
	    linux_sys_flistxattr },		/* 234 = flistxattr */
	{ 0, 0, 0,
	    linux_sys_removexattr },		/* 235 = removexattr */
	{ 0, 0, 0,
	    linux_sys_lremovexattr },		/* 236 = lremovexattr */
	{ 0, 0, 0,
	    linux_sys_fremovexattr },		/* 237 = fremovexattr */
	{ 0, 0, 0,
	    sys_nosys },			/* 238 = unimplemented linux_sys_tkill */
	{ 0, 0, 0,
	    sys_nosys },			/* 239 = unimplemented linux_sys_sendfile64 */
	{ 6, s(struct linux_sys_futex_args), 0,
	    linux_sys_futex },			/* 240 = futex */
	{ 0, 0, 0,
	    sys_nosys },			/* 241 = unimplemented linux_sys_sched_setaffinity */
	{ 0, 0, 0,
	    sys_nosys },			/* 242 = unimplemented linux_sys_sched_getaffinity */
	{ 1, s(struct linux_sys_set_thread_area_args), 0,
	    linux_sys_set_thread_area },	/* 243 = set_thread_area */
	{ 1, s(struct linux_sys_get_thread_area_args), 0,
	    linux_sys_get_thread_area },	/* 244 = get_thread_area */
	{ 0, 0, 0,
	    sys_nosys },			/* 245 = unimplemented linux_sys_io_setup */
	{ 0, 0, 0,
	    sys_nosys },			/* 246 = unimplemented linux_sys_io_destroy */
	{ 0, 0, 0,
	    sys_nosys },			/* 247 = unimplemented linux_sys_io_getevents */
	{ 0, 0, 0,
	    sys_nosys },			/* 248 = unimplemented linux_sys_io_submit */
	{ 0, 0, 0,
	    sys_nosys },			/* 249 = unimplemented linux_sys_io_cancel */
	{ 0, 0, 0,
	    linux_sys_fadvise64 },		/* 250 = fadvise64 */
	{ 0, 0, 0,
	    sys_nosys },			/* 251 = unimplemented */
	{ 1, s(struct sys_exit_args), 0,
	    sys_exit },				/* 252 = linux_exit_group */
	{ 0, 0, 0,
	    sys_nosys },			/* 253 = unimplemented linux_sys_lookup_dcookie */
	{ 0, 0, 0,
	    sys_nosys },			/* 254 = unimplemented linux_sys_epoll_create */
	{ 0, 0, 0,
	    sys_nosys },			/* 255 = unimplemented linux_sys_epoll_ctl */
	{ 0, 0, 0,
	    sys_nosys },			/* 256 = unimplemented linux_sys_epoll_wait */
	{ 0, 0, 0,
	    sys_nosys },			/* 257 = unimplemented linux_sys_remap_file_pages */
	{ 1, s(struct linux_sys_set_tid_address_args), 0,
	    linux_sys_set_tid_address },	/* 258 = set_tid_address */
	{ 0, 0, 0,
	    sys_nosys },			/* 259 = unimplemented linux_sys_timer_create */
	{ 0, 0, 0,
	    sys_nosys },			/* 260 = unimplemented linux_sys_timer_settime */
	{ 0, 0, 0,
	    sys_nosys },			/* 261 = unimplemented linux_sys_timer_gettime */
	{ 0, 0, 0,
	    sys_nosys },			/* 262 = unimplemented linux_sys_timer_getoverrun */
	{ 0, 0, 0,
	    sys_nosys },			/* 263 = unimplemented linux_sys_timer_delete */
	{ 0, 0, 0,
	    sys_nosys },			/* 264 = unimplemented linux_sys_clock_settime */
	{ 2, s(struct linux_sys_clock_gettime_args), 0,
	    linux_sys_clock_gettime },		/* 265 = clock_gettime */
	{ 2, s(struct linux_sys_clock_getres_args), 0,
	    linux_sys_clock_getres },		/* 266 = clock_getres */
	{ 0, 0, 0,
	    sys_nosys },			/* 267 = unimplemented linux_sys_clock_nanosleep */
	{ 0, 0, 0,
	    sys_nosys },			/* 268 = unimplemented linux_sys_statfs64 */
	{ 0, 0, 0,
	    sys_nosys },			/* 269 = unimplemented linux_sys_fstatfs64 */
	{ 0, 0, 0,
	    sys_nosys },			/* 270 = unimplemented linux_sys_tgkill */
	{ 0, 0, 0,
	    sys_nosys },			/* 271 = unimplemented linux_sys_utimes */
	{ 0, 0, 0,
	    sys_nosys },			/* 272 = unimplemented linux_sys_fadvise64_64 */
	{ 0, 0, 0,
	    sys_nosys },			/* 273 = unimplemented linux_sys_vserver */
	{ 0, 0, 0,
	    sys_nosys },			/* 274 = unimplemented linux_sys_mbind */
	{ 0, 0, 0,
	    sys_nosys },			/* 275 = unimplemented linux_sys_get_mempolicy */
	{ 0, 0, 0,
	    sys_nosys },			/* 276 = unimplemented linux_sys_set_mempolicy */
	{ 0, 0, 0,
	    sys_nosys },			/* 277 = unimplemented linux_sys_mq_open */
	{ 0, 0, 0,
	    sys_nosys },			/* 278 = unimplemented linux_sys_mq_unlink */
	{ 0, 0, 0,
	    sys_nosys },			/* 279 = unimplemented linux_sys_mq_timedsend */
	{ 0, 0, 0,
	    sys_nosys },			/* 280 = unimplemented linux_sys_mq_timedreceive */
	{ 0, 0, 0,
	    sys_nosys },			/* 281 = unimplemented linux_sys_mq_notify */
	{ 0, 0, 0,
	    sys_nosys },			/* 282 = unimplemented linux_sys_mq_getsetattr */
	{ 0, 0, 0,
	    sys_nosys },			/* 283 = unimplemented linux_sys_sys_kexec_load */
	{ 0, 0, 0,
	    sys_nosys },			/* 284 = unimplemented linux_sys_waitid */
	{ 0, 0, 0,
	    sys_nosys },			/* 285 = unimplemented / * unused * / */
	{ 0, 0, 0,
	    sys_nosys },			/* 286 = unimplemented linux_sys_add_key */
	{ 0, 0, 0,
	    sys_nosys },			/* 287 = unimplemented linux_sys_request_key */
	{ 0, 0, 0,
	    sys_nosys },			/* 288 = unimplemented linux_sys_keyctl */
	{ 0, 0, 0,
	    sys_nosys },			/* 289 = unimplemented linux_sys_ioprio_set */
	{ 0, 0, 0,
	    sys_nosys },			/* 290 = unimplemented linux_sys_ioprio_get */
	{ 0, 0, 0,
	    sys_nosys },			/* 291 = unimplemented linux_sys_inotify_init */
	{ 0, 0, 0,
	    sys_nosys },			/* 292 = unimplemented linux_sys_inotify_add_watch */
	{ 0, 0, 0,
	    sys_nosys },			/* 293 = unimplemented linux_sys_inotify_rm_watch */
	{ 0, 0, 0,
	    sys_nosys },			/* 294 = unimplemented linux_sys_migrate_pages */
	{ 0, 0, 0,
	    sys_nosys },			/* 295 = unimplemented linux_sys_openalinux_sys_t */
	{ 0, 0, 0,
	    sys_nosys },			/* 296 = unimplemented linux_sys_mkdirat */
	{ 0, 0, 0,
	    sys_nosys },			/* 297 = unimplemented linux_sys_mknodat */
	{ 0, 0, 0,
	    sys_nosys },			/* 298 = unimplemented linux_sys_fchownat */
	{ 0, 0, 0,
	    sys_nosys },			/* 299 = unimplemented linux_sys_futimesat */
	{ 0, 0, 0,
	    sys_nosys },			/* 300 = unimplemented linux_sys_fstatat64 */
	{ 0, 0, 0,
	    sys_nosys },			/* 301 = unimplemented linux_sys_unlinkat */
	{ 0, 0, 0,
	    sys_nosys },			/* 302 = unimplemented linux_sys_renameat */
	{ 0, 0, 0,
	    sys_nosys },			/* 303 = unimplemented linux_sys_linkat */
	{ 0, 0, 0,
	    sys_nosys },			/* 304 = unimplemented linux_sys_symlinkat */
	{ 0, 0, 0,
	    sys_nosys },			/* 305 = unimplemented linux_sys_readlinkat */
	{ 0, 0, 0,
	    sys_nosys },			/* 306 = unimplemented linux_sys_fchmodat */
	{ 0, 0, 0,
	    sys_nosys },			/* 307 = unimplemented linux_sys_faccessat */
	{ 0, 0, 0,
	    sys_nosys },			/* 308 = unimplemented linux_sys_pselect6 */
	{ 0, 0, 0,
	    sys_nosys },			/* 309 = unimplemented linux_sys_ppoll */
	{ 0, 0, 0,
	    sys_nosys },			/* 310 = unimplemented linux_sys_unshare */
	{ 2, s(struct linux_sys_set_robust_list_args), 0,
	    linux_sys_set_robust_list },	/* 311 = set_robust_list */
	{ 3, s(struct linux_sys_get_robust_list_args), 0,
	    linux_sys_get_robust_list },	/* 312 = get_robust_list */
	{ 0, 0, 0,
	    sys_nosys },			/* 313 = unimplemented splice */
	{ 0, 0, 0,
	    sys_nosys },			/* 314 = unimplemented sync_file_range */
	{ 0, 0, 0,
	    sys_nosys },			/* 315 = unimplemented tee */
	{ 0, 0, 0,
	    sys_nosys },			/* 316 = unimplemented vmsplice */
	{ 0, 0, 0,
	    sys_nosys },			/* 317 = unimplemented move_pages */
	{ 0, 0, 0,
	    sys_nosys },			/* 318 = unimplemented getcpu */
	{ 0, 0, 0,
	    sys_nosys },			/* 319 = unimplemented epoll_wait */
	{ 0, 0, 0,
	    sys_nosys },			/* 320 = unimplemented utimensat */
	{ 0, 0, 0,
	    sys_nosys },			/* 321 = unimplemented signalfd */
	{ 0, 0, 0,
	    sys_nosys },			/* 322 = unimplemented timerfd_create */
	{ 0, 0, 0,
	    sys_nosys },			/* 323 = unimplemented eventfd */
	{ 0, 0, 0,
	    sys_nosys },			/* 324 = unimplemented fallocate */
	{ 0, 0, 0,
	    sys_nosys },			/* 325 = unimplemented timerfd_settime */
	{ 0, 0, 0,
	    sys_nosys },			/* 326 = unimplemented timerfd_gettime */
	{ 0, 0, 0,
	    sys_nosys },			/* 327 = unimplemented signalfd4 */
	{ 0, 0, 0,
	    sys_nosys },			/* 328 = unimplemented eventfd2 */
	{ 0, 0, 0,
	    sys_nosys },			/* 329 = unimplemented epoll_create1 */
	{ 0, 0, 0,
	    sys_nosys },			/* 330 = unimplemented dup3 */
	{ 2, s(struct linux_sys_pipe2_args), 0,
	    linux_sys_pipe2 },			/* 331 = pipe2 */
	{ 0, 0, 0,
	    sys_nosys },			/* 332 = unimplemented inotify_init1 */
	{ 0, 0, 0,
	    sys_nosys },			/* 333 = unimplemented preadv */
	{ 0, 0, 0,
	    sys_nosys },			/* 334 = unimplemented pwritev */
	{ 0, 0, 0,
	    sys_nosys },			/* 335 = unimplemented rt_tgsigqueueinfo */
	{ 0, 0, 0,
	    sys_nosys },			/* 336 = unimplemented perf_counter_open */
	{ 0, 0, 0,
	    sys_nosys },			/* 337 = unimplemented recvmmsg */
};

