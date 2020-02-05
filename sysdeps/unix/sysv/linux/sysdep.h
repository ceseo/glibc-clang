/* Copyright (C) 2015-2020 Free Software Foundation, Inc.
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

#ifndef _SYSDEP_LINUX_H
#define _SYSDEP_LINUX_H

#include <bits/wordsize.h>
#include <kernel-features.h>
#include <errno.h>
#include <stdbool.h>

#ifndef __ASSEMBLER__

# define __syscall_concat_X(a,b) a##b
# define __syscall_concat(a,b)   __syscall_concat_X (a, b)

# ifndef ARGIFY
#  define ARGIFY(X) ((__syscall_arg_t) (X))
typedef long int __syscall_arg_t;
# endif

#define __internal_syscall0(name) \
  internal_syscall0 (name)
#define __internal_syscall1(name, a1) \
  internal_syscall1 (name, ARGIFY (a1))
#define __internal_syscall2(name, a1, a2) \
  internal_syscall2 (name, ARGIFY (a1), ARGIFY (a2))
#define __internal_syscall3(name, a1, a2, a3) \
  internal_syscall3 (name, ARGIFY (a1), ARGIFY (a2), ARGIFY (a3))
#define __internal_syscall4(name, a1, a2, a3, a4) \
  internal_syscall4 (name, ARGIFY (a1), ARGIFY (a2), ARGIFY (a3), ARGIFY (a4))
#define __internal_syscall5(name, a1, a2, a3, a4, a5) \
  internal_syscall5 (name, ARGIFY (a1), ARGIFY (a2), ARGIFY (a3),\
		     ARGIFY (a4), ARGIFY (a5))
#define __internal_syscall6(name, a1, a2, a3, a4, a5, a6) \
  internal_syscall6 (name, ARGIFY (a1), ARGIFY (a2), ARGIFY (a3), \
		     ARGIFY (a4), ARGIFY (a5), ARGIFY (a6))
#define __internal_syscall7(name, a1, a2, a3, a4, a5, a6, a7) \
  internal_syscall7 (name, ARGIFY (a1), ARGIFY (a2), ARGIFY (a3), \
		     ARGIFY (a4), ARGIFY (a5), ARGIFY (a6), ARGIFY (a7))

#define __internal_syscall_nargs_x(a,b,c,d,e,f,g,h,n,o,...) n
#define __internal_syscall_nargs(...) \
  __internal_syscall_nargs_x (__VA_ARGS__,7,6,5,4,3,2,1,0,)
#define __internal_syscall_disp(b,...) \
  __syscall_concat (b,__internal_syscall_nargs(__VA_ARGS__))(__VA_ARGS__)

#define internal_syscall(...) \
  __internal_syscall_disp (__internal_syscall, __VA_ARGS__)

static inline long int
syscall_error_ret (unsigned long r)
{
  __set_errno (r);
  return -1;
}

static inline bool
internal_syscall_error (unsigned long r)
{
  return r > -4096UL;
}

static inline long int
internal_syscall_ret (unsigned long r)
{
  if (internal_syscall_error (r))
    return syscall_error_ret (r);
  return 0;
}

/* Provide a dummy argument that can be used to force register
   alignment for register pairs if required by the syscall ABI.  */
#ifdef __ASSUME_ALIGNED_REGISTER_PAIRS
#define __ALIGNMENT_ARG 0,
#define __ALIGNMENT_COUNT(a,b) b
#else
#define __ALIGNMENT_ARG
#define __ALIGNMENT_COUNT(a,b) a
#endif

/* Provide a common macro to pass 64-bit value on syscalls.  */
#if __WORDSIZE == 64 || defined __ASSUME_WORDSIZE64_ILP32
# define SYSCALL_LL(val)   (val)
# define SYSCALL_LL64(val) (val)
#else
#define SYSCALL_LL(val)   \
  __LONG_LONG_PAIR ((val) >> 31, (val))
#define SYSCALL_LL64(val) \
  __LONG_LONG_PAIR ((long) ((val) >> 32), (long) ((val) & 0xffffffff))
#endif

/* Provide a common macro to pass 64-bit value on pread and pwrite
   syscalls.  */
#ifdef __ASSUME_PRW_DUMMY_ARG
# define SYSCALL_LL_PRW(val)   0, SYSCALL_LL (val)
# define SYSCALL_LL64_PRW(val) 0, SYSCALL_LL64 (val)
#else
# define SYSCALL_LL_PRW(val)   __ALIGNMENT_ARG SYSCALL_LL (val)
# define SYSCALL_LL64_PRW(val) __ALIGNMENT_ARG SYSCALL_LL64 (val)
#endif

/* Provide a macro to pass the off{64}_t argument on p{readv,writev}{64}.  */
#define LO_HI_LONG(val) \
 (long) (val), \
 (long) (((uint64_t) (val)) >> 32)

/* Exports the __send symbol on send.c linux implementation (some ABI have
   it missing due the usage of a old generic version without it).  */
#define HAVE_INTERNAL_SEND_SYMBOL	1

#endif /* __ASSEMBLER__  */

#endif /* _SYSDEP_LINUX_H  */
