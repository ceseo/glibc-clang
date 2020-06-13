/* Linux lseek implementation, 64 bits off_t.  MIPS64n32 version.
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
   License along with the GNU C Library.  If not, see
   <https://www.gnu.org/licenses/>.  */

#include <unistd.h>
#include <sysdep.h>

static inline off64_t
__lseek64_syscall (long int name, int fd, off64_t offset, int whence)
{
  register __syscall_arg_t s0 asm ("$16") = name;
  register __syscall_arg_t v0 asm ("$2");
  register __syscall_arg_t a0 asm ("$4") = ARGIFY (fd);
  register __syscall_arg_t a1 asm ("$5") = ARGIFY (offset);
  register __syscall_arg_t a2 asm ("$6") = ARGIFY (whence);
  register __syscall_arg_t a3 asm ("$7");
  asm volatile (".set\tnoreorder\n\t"
		MOVE32 "\t%0, %2\n\t"
		"syscall\n\t"
		".set reorder"
		: "=r" (v0), "=r" (a3)
		: "r" (s0), "r" (a0), "r" (a1), "r" (a2)
		: __SYSCALL_CLOBBERS);
  return a3 != 0
	 ? SYSCALL_ERROR_LABEL (INTERNAL_SYSCALL_ERRNO (v0))
         : v0;
}

#undef INLINE_SYSCALL_CALL
#define INLINE_SYSCALL_CALL(name, ...) \
  __lseek64_syscall(__NR_##name, __VA_ARGS__)

#include <sysdeps/unix/sysv/linux/lseek64.c>
