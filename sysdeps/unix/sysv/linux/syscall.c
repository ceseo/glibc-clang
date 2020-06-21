/* Indirect system call.  Linux version.
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

#include <stdarg.h>
#include <sysdep.h>

long int
syscall (long int number, ...)
{
  va_list args;

  va_start (args, number);
  long int arg0 = va_arg (args, long int);
  long int arg1 = va_arg (args, long int);
  long int arg2 = va_arg (args, long int);
  long int arg3 = va_arg (args, long int);
  long int arg4 = va_arg (args, long int);
  long int arg5 = va_arg (args, long int);
  va_end (args);

  return syscall_ret (INTERNAL_SYSCALL_NCS (number, 6, arg0, arg1, arg2,
					    arg3, arg4, arg5));
}
