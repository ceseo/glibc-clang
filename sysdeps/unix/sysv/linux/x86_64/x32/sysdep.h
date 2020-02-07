/* Copyright (C) 2012-2020 Free Software Foundation, Inc.
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

#ifndef _LINUX_X32_SYSDEP_H
#define _LINUX_X32_SYSDEP_H 1

#include <libc-diag.h>

#ifndef __ASSEMBLER__
/* Syscall arguments for x32 follows x86_64 ABI, however pointers are 32 bits
   should be zero extended.  However compiler may not see such cases and
   accuse a cast from pointer to integer of different size.  */
#define ARGIFY(X)						\
  ({								\
    DIAG_PUSH_NEEDS_COMMENT;					\
    DIAG_IGNORE_NEEDS_COMMENT (5, "-Wpointer-to-int-cast");	\
    __syscall_arg_t __arg = sizeof (1 ? (X) : 0ULL) < 8		\
			    ? (unsigned long int) (X) 		\
			    : (long long int) ((X));		\
    DIAG_POP_NEEDS_COMMENT;					\
    __arg;							\
  })
typedef long long int __syscall_arg_t;
#endif

/* There is some commonality.  */
#include <sysdeps/unix/sysv/linux/x86_64/sysdep.h>
#include <sysdeps/x86_64/x32/sysdep.h>

/* How to pass the off{64}_t argument on p{readv,writev}{64}.  */
#undef LO_HI_LONG
#define LO_HI_LONG(val) (val)

#endif /* linux/x86_64/x32/sysdep.h */
