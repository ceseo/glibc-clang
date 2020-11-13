/* Assembler macros for ARC.
   Copyright (C) 2020 Free Software Foundation, Inc.
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
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#ifndef _LINUX_ARC_SYSDEP_H
#define _LINUX_ARC_SYSDEP_H 1

#include <sysdeps/arc/sysdep.h>
#include <sysdeps/unix/sysv/linux/generic/sysdep.h>

/* "workarounds" for generic code needing to handle 64-bit time_t.  */

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

/* For RTLD_PRIVATE_ERRNO.  */
#include <dl-sysdep.h>

#include <tls.h>

#undef SYS_ify
#define SYS_ify(syscall_name)   __NR_##syscall_name

#ifdef __ASSEMBLER__

# define SYSCALL_ERROR_HANDLER				\
L (call_syscall_err):			ASM_LINE_SEP	\
    push_s   blink			ASM_LINE_SEP	\
    cfi_adjust_cfa_offset (4)		ASM_LINE_SEP	\
    cfi_rel_offset (blink, 0)		ASM_LINE_SEP	\
    bl __syscall_error			ASM_LINE_SEP	\
    pop_s  blink			ASM_LINE_SEP	\
    cfi_adjust_cfa_offset (-4)		ASM_LINE_SEP	\
    cfi_restore (blink)			ASM_LINE_SEP	\
    j_s      [blink]

# define ARC_TRAP_INSN	trap_s 0

#else

# define SINGLE_THREAD_BY_GLOBAL		1

# define ARC_TRAP_INSN	"trap_s 0	\n\t"

/* Pointer mangling not yet supported.  */
# define PTR_MANGLE(var) (void) (var)
# define PTR_DEMANGLE(var) (void) (var)

# undef HAVE_INTERNAL_BRK_ADDR_SYMBOL
# define HAVE_INTERNAL_BRK_ADDR_SYMBOL  1

static inline long int
__internal_syscall0 (long int name)
{
  register long int sc asm ("r8") = name;
  register long int r0 asm ("r0");
  asm volatile (ARC_TRAP_INSN
		: "+r" (r0)
		: "r" (sc)
		: "memory");
  return r0;
}

static inline long int
__internal_syscall1 (long int name, __syscall_arg_t arg1)
{
  register long int sc asm ("r8") = name;
  register long int r0 asm ("r0") = arg1;
  asm volatile (ARC_TRAP_INSN
		: "+r" (r0)
		: "r" (sc), "r" (r0)
		: "memory");
  return r0;
}

static inline long int
__internal_syscall2 (long int name, __syscall_arg_t arg1,
		     __syscall_arg_t arg2)
{
  register long int sc asm ("r8") = name;
  register long int r0 asm ("r0") = arg1;
  register long int r1 asm ("r1") = arg2;
  asm volatile (ARC_TRAP_INSN
		: "+r" (r0)
		: "r" (sc), "r" (r0), "r" (r1)
		: "memory");
  return r0;
}

static inline long int
__internal_syscall3 (long int name, __syscall_arg_t arg1,
		     __syscall_arg_t arg2, __syscall_arg_t arg3)
{
  register long int sc asm ("r8") = name;
  register long int r0 asm ("r0") = arg1;
  register long int r1 asm ("r1") = arg2;
  register long int r2 asm ("r2") = arg3;
  asm volatile (ARC_TRAP_INSN
		: "+r" (r0)
		: "r" (sc), "r" (r0), "r" (r1), "r" (r2)
		: "memory");
  return r0;
}

static inline long int
__internal_syscall4 (long int name, __syscall_arg_t arg1,
		     __syscall_arg_t arg2, __syscall_arg_t arg3,
		     __syscall_arg_t arg4)
{
  register long int sc asm ("r8") = name;
  register long int r0 asm ("r0") = arg1;
  register long int r1 asm ("r1") = arg2;
  register long int r2 asm ("r2") = arg3;
  register long int r3 asm ("r3") = arg4;
  asm volatile (ARC_TRAP_INSN
		: "+r" (r0)
		: "r" (sc), "r" (r0), "r" (r1), "r" (r2), "r" (r3)
		: "memory");
  return r0;
}

static inline long int
__internal_syscall5 (long int name, __syscall_arg_t arg1,
		     __syscall_arg_t arg2, __syscall_arg_t arg3,
		     __syscall_arg_t arg4, __syscall_arg_t arg5)
{
  register long int sc asm ("r8") = name;
  register long int r0 asm ("r0") = arg1;
  register long int r1 asm ("r1") = arg2;
  register long int r2 asm ("r2") = arg3;
  register long int r3 asm ("r3") = arg4;
  register long int r4 asm ("r4") = arg5;
  asm volatile (ARC_TRAP_INSN
		: "+r" (r0)
		: "r" (sc), "r" (r0), "r" (r1), "r" (r2), "r" (r3),
		  "r" (r4)
		: "memory");
  return r0;
}

static inline long int
__internal_syscall6 (long int name, __syscall_arg_t arg1,
		     __syscall_arg_t arg2, __syscall_arg_t arg3,
		     __syscall_arg_t arg4, __syscall_arg_t arg5,
		     __syscall_arg_t arg6)
{
  register long int sc asm ("r8") = name;
  register long int r0 asm ("r0") = arg1;
  register long int r1 asm ("r1") = arg2;
  register long int r2 asm ("r2") = arg3;
  register long int r3 asm ("r3") = arg4;
  register long int r4 asm ("r4") = arg5;
  register long int r5 asm ("r5") = arg6;
  asm volatile (ARC_TRAP_INSN
		: "+r" (r0)
		: "r" (sc), "r" (r0), "r" (r1), "r" (r2), "r" (r3),
		  "r" (r4), "r" (r5)
		: "memory");
  return r0;
}

#endif /* !__ASSEMBLER__ */

#endif /* linux/arc/sysdep.h */
