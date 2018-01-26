/* Determine current working directory.  Linux version.
   Copyright (C) 1997-2018 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

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
   <http://www.gnu.org/licenses/>.  */

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>

#include <sysdep.h>
#include <sys/syscall.h>

static char *
system_getcwd (char *path, size_t size)
{
  int retval = INLINE_SYSCALL_CALL (getcwd, path, size);
  if (retval > 0 && path[0] == '/')
    return path;

  return NULL;
}

/* Get the code for the generic version.  */
#define HAVE_OPENAT			1
#define HAVE_MINIMALLY_WORKING_GETCWD	0
#define D_INO_IN_DIRENT			1
#define HAVE_SYSTEM_GETCWD		1
#include <sysdeps/posix/getcwd.c>
