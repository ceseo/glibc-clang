/* Copyright (C) 1995-2018 Free Software Foundation, Inc.
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
   <http://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <not-cancel.h>

#define HOSTIDFILE "/etc/hostid"

int
sethostid (long int id)
{
  int fd;
  ssize_t written;
  int32_t id32 = id;

  /* Test for appropriate rights to set host ID.  */
  if (__libc_enable_secure)
    {
      __set_errno (EPERM);
      return -1;
    }

  /* Make sure the ID is not too large.  Needed for bi-arch support.   */
  if (id32 != id)
    {
      __set_errno (EOVERFLOW);
      return -1;
    }

  /* Open file for writing.  Everybody is allowed to read this file.  */
  fd = __open_nocancel (HOSTIDFILE, O_CREAT|O_WRONLY|O_TRUNC, 0644);
  if (fd < 0)
    return -1;

  written = __write_nocancel (fd, &id32, sizeof (id32));

  __close_nocancel_nostatus (fd);

  return written != sizeof (id32) ? -1 : 0;
}
