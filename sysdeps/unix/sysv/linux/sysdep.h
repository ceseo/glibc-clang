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
#include <syscall_error.h>
#include <endian.h>
#include <errno.h>

#ifndef __ASSEMBLER__
/* The errno setting might be set either inline or with a helper function.
   For some ABIs (x86_64 for instance), handling it inline might generate
   less code; while for others (i686) a function call is preferable.

   To use the helper function the ABI must define SYSCALL_ERROR_FUNC, it will
   build a hidden function on each shared object that issue direct syscall
   with {INLINE,INTERNAL}_SYSCALL_CALL.  */

# if SYSCALL_ERROR_FUNC
long int __syscall_error (long int err) attribute_hidden
  SYSCALL_ERROR_FUNC_ATTR;
# else
static inline long int
__syscall_error (long int err)
{
  __set_errno (-err);
  return -1L;
}
# endif

static inline long int
syscall_ret (unsigned long int val)
{
  if (val > -4096UL)
    return __syscall_error (val);
  return val;
}

/* Define a macro which expands into the inline wrapper code for a system
   call.  It sets the errno and returns -1 on a failure, or the syscall
   return value otherwise.  */
#undef INLINE_SYSCALL
#define INLINE_SYSCALL(...)				\
  syscall_ret (INTERNAL_SYSCALL (__VA_ARGS__))
#endif

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

#endif /* _SYSDEP_LINUX_H  */
