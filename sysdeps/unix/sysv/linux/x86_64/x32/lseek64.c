/* Linux lseek implementation, 64 bits off_t.  x86_64/x32 version.
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
#include <shlib-compat.h>

static inline off64_t
__lseek64_syscall (long int name, int fd, off64_t offset, int whence)
{
  off64_t resultvar;
  asm volatile ("syscall\n\t"
		: "=a" (resultvar)
		: "0" (name),  "D" (fd), "S" (offset), "d" (whence)
		: "memory", "cc", "r11", "cx");
  return resultvar < 0
	 ? SYSCALL_ERROR_LABEL (INTERNAL_SYSCALL_ERRNO (-resultvar))
         : resultvar;
}

#undef INLINE_SYSCALL_CALL
#define INLINE_SYSCALL_CALL(name, ...) \
  __lseek64_syscall(__NR_##name, __VA_ARGS__)

/* Disable the llseek compat symbol.  */
#undef SHLIB_COMPAT
#define SHLIB_COMPAT(a, b, c) 0

#include <sysdeps/unix/sysv/linux/lseek64.c>
