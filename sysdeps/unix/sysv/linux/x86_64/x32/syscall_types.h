/* Types and macros used for syscall issuing.  x86_64/x32 version.
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

#ifndef _SYSCALL_TYPES_H
#define _SYSCALL_TYPES_H

#include <libc-diag.h>

typedef long long int __syscall_arg_t;

/* Syscall arguments for x32 follows x86_64 ABI, however pointers are 32 bits
   should be zero extended.  However compiler may not see such cases and
   accuse a cast from pointer to integer of different size.  */
#define __SSC(__x)						\
  ({								\
    DIAG_PUSH_NEEDS_COMMENT;					\
    DIAG_IGNORE_NEEDS_COMMENT (5, "-Wpointer-to-int-cast");	\
    __syscall_arg_t __arg = sizeof (1 ? (__x) : 0ULL) < 8	\
			    ? (unsigned long int) (__x) 	\
			    : (long long int) ((__x));		\
    DIAG_POP_NEEDS_COMMENT;					\
    __arg;							\
  })

#endif
