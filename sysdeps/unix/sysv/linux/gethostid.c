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
#include <string.h>
#include <sys/param.h>
#include <resolv/netdb.h>
#include <netinet/in.h>
#include <not-cancel.h>

#define HOSTIDFILE "/etc/hostid"

#include <scratch_buffer.h>

long int
gethostid (void)
{
  char hostname[MAXHOSTNAMELEN + 1];
  struct hostent hostbuf, *hp;
  int32_t id;
  struct in_addr in;
  int herr;
  int fd;

  /* First try to get the ID from a former invocation of sethostid.  */
  fd = __open_nocancel (HOSTIDFILE, O_RDONLY|O_LARGEFILE, 0);
  if (fd >= 0)
    {
      ssize_t n = __read_nocancel (fd, &id, sizeof (id));

      __close_nocancel_nostatus (fd);

      if (n == sizeof (id))
	return id;
    }

  /* Getting from the file was not successful.  An intelligent guess for
     a unique number of a host is its IP address.  Return this.  */
  if (__gethostname (hostname, MAXHOSTNAMELEN) < 0 || hostname[0] == '\0')
    /* This also fails.  Return and arbitrary value.  */
    return 0;

  struct scratch_buffer buffer;
  scratch_buffer_init (&buffer);

  /* To get the IP address we need to know the host name.  */
  while (__gethostbyname_r (hostname, &hostbuf, buffer.data, buffer.length,
			    &hp, &herr)
	 != 0
	 || hp == NULL)
    {
      if (herr != NETDB_INTERNAL || errno != ERANGE
	  || !scratch_buffer_grow (&buffer))
	{
	  scratch_buffer_free (&buffer);
	  return 0;
	}
    }

  in.s_addr = 0;
  memcpy (&in, hp->h_addr,
	  (int) sizeof (in) < hp->h_length ? (int) sizeof (in) : hp->h_length);
  scratch_buffer_free (&buffer);

  /* For the return value to be not exactly the IP address we do some
     bit fiddling.  */
  return (int32_t) (in.s_addr << 16 | in.s_addr >> 16);
}
