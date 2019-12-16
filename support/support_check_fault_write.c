/* Check for faults at specific addresses.
   Copyright (C) 2019 Free Software Foundation, Inc.
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

#include <support/check_fault.h>

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <support/check.h>
#include <support/support.h>
#include <support/xunistd.h>
#include <sys/wait.h>
#include <unistd.h>

/* Probe once at ADDRESS.  Use START and INDEX in error reporting.  */
static bool
check_one_address (volatile char *address, const void *start, size_t index)
{
  pid_t pid = xfork ();
  if (pid == 0)
    {
      support_disable_more_coredumps ();
      *address = 0xcc;  /* Try to write an arbitrary value.  */
      _exit (0);
    }
  int status;
  xwaitpid (pid, &status, 0);
  if (WIFSIGNALED (status))
    {
      if (WTERMSIG (status) != SIGSEGV && WTERMSIG (status) != SIGBUS)
        {
          support_record_failure ();
          printf ("error: unexpected signal %d at %p (%p+%zu)\n",
                  WTERMSIG (status), address, start, index);
          return false;
        }
    }
  else
    {
      support_record_failure ();
      printf ("error: missing fault at %p (%p+%zu)\n",
              address, start, index);
      return false;
    }
  return true;
}

void
support_check_fault_write (const void *address, size_t size)
{
  TEST_VERIFY (address != NULL);
  TEST_VERIFY (size > 0);

  for (size_t i = 0; i < size; ++i)
    if (!check_one_address (((char *) address) + i, address, i))
      break;
}
