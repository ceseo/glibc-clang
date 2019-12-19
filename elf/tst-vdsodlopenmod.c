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

#include <time.h>
#include <sys/time.h>
#include <sched.h>
#include <errno.h>

int
clock_gettime_wrapper (clockid_t clk_id, struct timespec *tp, int *errnop)
{
  errno = *errnop;
  int ret = clock_gettime (clk_id, tp);
  *errnop = errno;
  return ret;
}

int
clock_getres_wrapper (clockid_t clk_id, struct timespec *tp, int *errnop)
{
  errno = *errnop;
  int ret = clock_getres (clk_id, tp);
  *errnop = errno;
  return ret;
}

int
gettimeofday_wrapper (struct timeval *tv, void *tz)
{
  return gettimeofday (tv, tz);
}

time_t
time_wrapper (time_t *t)
{
  return time (t);
}
