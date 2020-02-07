/* Linux times.  X32 version.
   Copyright (C) 2015-2020 Free Software Foundation, Inc.
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

#include <sysdep.h>

static unsigned long long int
internal_syscall1_u64 (long int name, __syscall_arg_t arg1)
{
  unsigned long long int resultvar;
  register __syscall_arg_t a1 asm ("rdi") = arg1;
  asm volatile ("syscall\n\t"
		: "=a" (resultvar)
		: "0" (name),  "r" (a1)
		: "memory", "cc", "r11", "cx");
  return resultvar;
}

static long long int
internal_syscall_ret_64 (unsigned long long r)
{
  if (r > -4096ULL)
    {
      __set_errno (-r);
      return -1;
    }
  return r;
}

/* Linux times system call returns 64-bit integer.  */
#undef internal_syscall
#define internal_syscall(number, arg1)					\
  internal_syscall_ret_64 (internal_syscall1_u64 (number, ARGIFY (arg1)))

#include <sysdeps/unix/sysv/linux/times.c>
