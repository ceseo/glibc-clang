/* Test static dlopen a module that calls vDSO symbols.
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

#include <stdio.h>
#include <string.h>

#include <time.h>
#include <sys/time.h>
#include <sched.h>
#include <errno.h>

#include <support/check.h>
#include <support/xdlfcn.h>

static int do_test (void)
{
  TEST_COMPARE (clock_gettime (CLOCK_REALTIME, &(struct timespec) {}), 0);
  TEST_COMPARE (clock_getres (CLOCK_REALTIME, &(struct timespec) {}), 0);
  TEST_COMPARE (gettimeofday (&(struct timeval){}, NULL), 0);
  TEST_VERIFY (time (NULL) != (time_t)-1);

  void *handler = xdlopen ("tst-vdsodlopenmod.so", RTLD_NOW);

  /* To check errno value it requires wrapper to save is explicit, since the
     moduled is loaded on a different namespace from application.  */
  {
    int (*wrapper)(clockid_t, struct timespec *, int *)
      = xdlsym (handler, "clock_gettime_wrapper");
    int ierrno = 0;
    TEST_COMPARE (wrapper (CLOCK_REALTIME, &(struct timespec) {}, &ierrno),
		  0);
    TEST_COMPARE (wrapper (-1, &(struct timespec) {}, &ierrno), -1);
    TEST_COMPARE (ierrno, EINVAL);
  }

  {
    int (*wrapper)(clockid_t, struct timespec *, int *)
      = xdlsym (handler, "clock_getres_wrapper");
    int ierrno = 0;
    TEST_COMPARE (wrapper (CLOCK_REALTIME, &(struct timespec) {}, &ierrno),
		  0);
    TEST_COMPARE (wrapper (-1, &(struct timespec) {}, &ierrno), -1);
    TEST_COMPARE (ierrno, EINVAL);
  }

  {
    int (*wrapper)(struct timeval *tv, void *)
      = xdlsym (handler, "gettimeofday_wrapper");
    TEST_COMPARE (wrapper (&(struct timeval) {}, NULL), 0);
  }

  {
    int (*wrapper)(time_t *) = xdlsym (handler, "time_wrapper");
    TEST_VERIFY (wrapper (NULL) != (time_t)-1);
  }

  return 0;
}

#include <support/test-driver.c>
