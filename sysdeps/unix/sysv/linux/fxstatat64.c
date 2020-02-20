/* Copyright (C) 2005-2020 Free Software Foundation, Inc.
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

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <kernel_stat.h>

#include <sysdep.h>
#include <sys/syscall.h>

#include <statx_cp.h>

/* Get information about the file NAME in BUF.  */

int
__fxstatat64 (int vers, int fd, const char *file, struct stat64 *st, int flag)
{
  if (__glibc_unlikely (vers != _STAT_VER_LINUX))
    return syscall_error_ret (EINVAL);

  int result;

#ifdef __NR_fstatat64
  result = internal_syscall (__NR_fstatat64, fd, file, st, flag);
#else
  struct statx tmp;

  result = internal_syscall (__NR_statx, fd, file, AT_NO_AUTOMOUNT | flag,
			     STATX_BASIC_STATS, &tmp);
  if (result == 0)
    __cp_stat64_statx (st, &tmp);
#endif
  if (__glibc_likely (!syscall_error_ret (result)))
    return 0;
  return syscall_error_ret (-result);
}
libc_hidden_def (__fxstatat64)
