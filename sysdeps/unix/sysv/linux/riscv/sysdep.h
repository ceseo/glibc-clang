/* Assembly macros for RISC-V.
   Copyright (C) 2011-2018
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library.  If not, see
   <https://www.gnu.org/licenses/>.  */

#ifndef _LINUX_RISCV_SYSDEP_H
#define _LINUX_RISCV_SYSDEP_H 1

#include <sysdeps/unix/sysv/linux/generic/sysdep.h>

#undef SYS_ify
#define SYS_ify(syscall_name)	__NR_##syscall_name

#if __WORDSIZE == 32

/* Workarounds for generic code needing to handle 64-bit time_t.  */

/* Fix sysdeps/unix/sysv/linux/clock_getcpuclockid.c.  */
#define __NR_clock_getres	__NR_clock_getres_time64
/* Fix sysdeps/nptl/lowlevellock-futex.h.  */
#define __NR_futex		__NR_futex_time64
/* Fix sysdeps/unix/sysv/linux/pause.c.  */
#define __NR_ppoll		__NR_ppoll_time64
/* Fix sysdeps/unix/sysv/linux/select.c.  */
#define __NR_pselect6		__NR_pselect6_time64
/* Fix sysdeps/unix/sysv/linux/recvmmsg.c.  */
#define __NR_recvmmsg		__NR_recvmmsg_time64
/* Fix sysdeps/unix/sysv/linux/sigtimedwait.c.  */
#define __NR_rt_sigtimedwait	__NR_rt_sigtimedwait_time64
/* Fix sysdeps/unix/sysv/linux/semtimedop.c.  */
#define __NR_semtimedop		__NR_semtimedop_time64
/* Hack sysdeps/unix/sysv/linux/generic/utimes.c.  */
#define __NR_utimensat		__NR_utimensat_time64

#endif /* __WORDSIZE == 32 */

#ifndef __ASSEMBLER__

# if __WORDSIZE == 64
#  define VDSO_NAME	"LINUX_4.15"
#  define VDSO_HASH	182943605

/* List of system calls which are supported as vsyscalls only
   for RV64.  */
#  define HAVE_CLOCK_GETRES64_VSYSCALL	"__vdso_clock_getres"
#  define HAVE_CLOCK_GETTIME64_VSYSCALL	"__vdso_clock_gettime"
#  define HAVE_GETTIMEOFDAY_VSYSCALL	"__vdso_gettimeofday"
# else
#  define VDSO_NAME	"LINUX_5.4"
#  define VDSO_HASH	61765876
/* RV32 does not support the gettime VDSO syscalls.  */
# endif

/* List of system calls which are supported as vsyscalls (for RV32 and
   RV64).  */
# define HAVE_GETCPU_VSYSCALL		"__vdso_getcpu"

# undef HAVE_INTERNAL_BRK_ADDR_SYMBOL
# define HAVE_INTERNAL_BRK_ADDR_SYMBOL 1

extern long int __syscall_error (long int neg_errno);

static inline long int
__internal_syscall0 (long int name)
{
  register long int a7 asm ("a7") = name;
  register long int a0 asm ("a0");
  asm volatile ("scall\n\t"
		: "=r" (a0)
		: "r" (a7)
		: "memory");
  return a0;
}

static inline long int
__internal_syscall1 (long int name, __syscall_arg_t arg1)
{
  register long int a7 asm ("a7") = name;
  register long int a0 asm ("a0") = arg1;
  asm volatile ("scall\n\t"
		: "+r" (a0)
		: "r" (a7)
		: "memory");
  return a0;
}

static inline long int
__internal_syscall2 (long int name, __syscall_arg_t arg1,
		     __syscall_arg_t arg2)
{
  register long int a7 asm ("a7") = name;
  register long int a0 asm ("a0") = arg1;
  register long int a1 asm ("a1") = arg2;
  asm volatile ("scall\n\t"
		: "+r" (a0)
		: "r" (a7), "r" (a1)
		: "memory");
  return a0;
}

static inline long int
__internal_syscall3 (long int name, __syscall_arg_t arg1,
		     __syscall_arg_t arg2, __syscall_arg_t arg3)
{
  register long int a7 asm ("a7") = name;
  register long int a0 asm ("a0") = arg1;
  register long int a1 asm ("a1") = arg2;
  register long int a2 asm ("a2") = arg3;
  asm volatile ("scall\n\t"
		: "+r" (a0)
		: "r" (a7), "r" (a1), "r" (a2)
		: "memory");
  return a0;
}

static inline long int
__internal_syscall4 (long int name, __syscall_arg_t arg1,
		     __syscall_arg_t arg2, __syscall_arg_t arg3,
		     __syscall_arg_t arg4)
{
  register long int a7 asm ("a7") = name;
  register long int a0 asm ("a0") = arg1;
  register long int a1 asm ("a1") = arg2;
  register long int a2 asm ("a2") = arg3;
  register long int a3 asm ("a3") = arg4;
  asm volatile ("scall\n\t"
		: "+r" (a0)
		: "r" (a7), "r" (a1), "r" (a2), "r" (a3)
		: "memory");
  return a0;
}

static inline long int
__internal_syscall5 (long int name, __syscall_arg_t arg1,
		     __syscall_arg_t arg2, __syscall_arg_t arg3,
		     __syscall_arg_t arg4, __syscall_arg_t arg5)
{
  register long int a7 asm ("a7") = name;
  register long int a0 asm ("a0") = arg1;
  register long int a1 asm ("a1") = arg2;
  register long int a2 asm ("a2") = arg3;
  register long int a3 asm ("a3") = arg4;
  register long int a4 asm ("a4") = arg5;
  asm volatile ("scall\n\t"
		: "+r" (a0)
		: "r" (a7), "r" (a1), "r" (a2), "r" (a3), "r" (a4)
		: "memory");
  return a0;
}

static inline long int
__internal_syscall6 (long int name, __syscall_arg_t arg1,
		     __syscall_arg_t arg2, __syscall_arg_t arg3,
		     __syscall_arg_t arg4, __syscall_arg_t arg5,
		     __syscall_arg_t arg6)
{
  register long int a7 asm ("a7") = name;
  register long int a0 asm ("a0") = arg1;
  register long int a1 asm ("a1") = arg2;
  register long int a2 asm ("a2") = arg3;
  register long int a3 asm ("a3") = arg4;
  register long int a4 asm ("a4") = arg5;
  register long int a5 asm ("a5") = arg6;
  asm volatile ("scall\n\t"
		: "+r" (a0)
		: "r" (a7), "r" (a1), "r" (a2), "r" (a3), "r" (a4), "r" (a5)
		: "memory");
  return a0;
}

static inline long int
__internal_syscall7 (long int name, __syscall_arg_t arg1,
		     __syscall_arg_t arg2, __syscall_arg_t arg3,
		     __syscall_arg_t arg4, __syscall_arg_t arg5,
		     __syscall_arg_t arg6, __syscall_arg_t arg7)
{
  register long int a7 asm ("a7") = name;
  register long int a0 asm ("a0") = arg1;
  register long int a1 asm ("a1") = arg2;
  register long int a2 asm ("a2") = arg3;
  register long int a3 asm ("a3") = arg4;
  register long int a4 asm ("a4") = arg5;
  register long int a5 asm ("a5") = arg6;
  register long int a6 asm ("a6") = arg7;
  asm volatile ("scall\n\t"
		: "+r" (a0)
		: "r" (a7), "r" (a1), "r" (a2), "r" (a3), "r" (a4), "r" (a5),
		  "r" (a6)
		: "memory");
  return a0;
}


#endif /* ! __ASSEMBLER__ */

/* Pointer mangling is not supported.  */
#define PTR_MANGLE(var) (void) (var)
#define PTR_DEMANGLE(var) (void) (var)

#endif /* linux/riscv/sysdep.h */
