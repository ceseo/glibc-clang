/* Linux wrappers for setting errno.
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

#ifndef _SYSCALL_ERROR_H
#define _SYSCALL_ERROR_H

/* Each architecture might how __syscall_error is called, either by defining
   an inline function (default) or by calling a hidden function.  Check the
   sysdep.h file for the implementation.  */
#define SYSCALL_ERROR_FUNC        0

/* Any function attribute used to define the __syscall_error in case of
   __SYSCALL_ERROR_FUNC (for instance by using a different calling covention
   for intra-DSO calls.  */
#define SYSCALL_ERROR_FUNC_ATTR

#endif
