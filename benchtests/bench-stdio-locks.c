/* Measure stdio lock overhead.
   Copyright The GNU Toolchain Authors.
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

#include <support/check.h>
#include <support/support.h>
#include <support/temp_file.h>
#include <support/timespec.h>
#include <support/xstdio.h>
#include <support/xtime.h>
#include <support/xthread.h>
#include <sys/mman.h>
#include <unistd.h>
#include "bench-timing.h"
#include "bench-util.h"
#include "json-lib.h"

struct latency_result_t
{
  double max;
  double min;
  double mean;
};
const size_t niters = 64 * 1024;

struct latency_result_t
bench_latency (FILE *f)
{
  rewind (f);

  timing_t min = 0x7fffffffffffffff, max = 0, total = 0;

  for (size_t n = 0; n < niters; n++)
    {
      timing_t start, end, elapsed;

      TIMING_NOW (start);
      DO_NOT_OPTIMIZE (fgetc (f));
      TIMING_NOW (end);
      TIMING_DIFF (elapsed, start, end);
      if (min > elapsed)
	min = elapsed;
      if (max < elapsed)
	max = elapsed;
      TIMING_ACCUM (total, elapsed);
    }

  return (struct latency_result_t) { max, min, (double) total / niters };
}

struct thr_latency_args_t
{
  FILE *f;
  struct latency_result_t r;
};

static void *
bench_latency_tf (void *closure)
{
  struct thr_latency_args_t *args = closure;
  args->r = bench_latency (args->f);
  return NULL;
}

static struct latency_result_t
bench_latency_mt (FILE *f)
{
  struct thr_latency_args_t args = { .f = f };
  pthread_t t = xpthread_create (NULL, bench_latency_tf, &args);
  xpthread_join (t);
  return args.r;
}


static timer_t timer;
static volatile sig_atomic_t timer_finished;

static void timer_callback (int unused)
{
  timer_finished = 1;
}

/* Run for approximately DURATION seconds, and it does not matter who
   receive the signal (so not need to mask it on main thread).  */
static void
timer_start (void)
{
  timer_finished = 0;
  timer = support_create_timer (DURATION, 0, false, timer_callback);
}

static void
timer_stop (void)
{
  support_delete_timer (timer);
}

static double
bench_throughput (FILE *f)
{
  rewind (f);

  uint64_t n = 0;

  timer_start ();

  struct timespec start, end;
  xclock_gettime (CLOCK_MONOTONIC, &start);
  while (1)
    {
      int c;
      DO_NOT_OPTIMIZE (c = fgetc (f));
      if (c == EOF)
	rewind (f);

      n++;

      if (timer_finished == 1)
        break;
    }
  xclock_gettime (CLOCK_MONOTONIC, &end);
  struct timespec diff = timespec_sub (end, start);

  timer_stop ();

  double total = (double) n;
  double duration = (double) diff.tv_sec
    + (double) diff.tv_nsec / TIMESPEC_HZ;

  return total / duration;
}

struct thr_throughput_args_t
{
  FILE *f;
  double r;
};

static void *
bench_throughput_tf (void *closure)
{
  struct thr_throughput_args_t *args = closure;
  args->r = bench_throughput (args->f);
  return NULL;
}

static double
bench_throughput_mt (FILE *f)
{
  struct thr_throughput_args_t args = { .f = f };
  pthread_t t = xpthread_create (NULL, bench_throughput_tf, &args);
  xpthread_join (t);
  return args.r;
}

/* Create a FILE object with a backing object that generate as littler as
   possible OS interference through read syscalls.  */
static FILE *
open_bench_memfile (size_t size)
{
  bool mfd = false;
  int fd = memfd_create ("bench-stdio-locks", MFD_CLOEXEC);
  if (fd == -1)
    fd = create_temp_file ("bench-stdio-locks", NULL);
  else
    mfd = true;

  if (fd == -1)
    FAIL_EXIT1 ("cannot create temporary file: %m");

  if (ftruncate (fd, size) == -1)
    FAIL_EXIT1 ("ftruncate (%d, %zu): %m", fd, size);
  if (!mfd)
    {
      /* Try to fill OS page buffer with file contents to minimize OS jitter
	 on benchmark.  */
      char buf[BUFSIZ];
      ssize_t r;
      do
	{
	  r = read (fd, buf, sizeof buf);
	  if (r == -1)
	    FAIL_EXIT1 ("read temporary file failed: %m");
	} while (r > 0);
    }

  FILE *f = fdopen (fd, "r");
  if (f == NULL)
    FAIL_EXIT1 ("could not fdopen (mode \"r\"): %m");

  return f;
}

static void
json_latency (json_ctx_t *json_ctx, struct latency_result_t (*bench) (FILE *),
	      FILE *f)
{
  json_attr_object_begin (json_ctx, "latency");
  struct latency_result_t r = bench_latency (f);
  json_attr_double (json_ctx, "max", r.max);
  json_attr_double (json_ctx, "min", r.min);
  json_attr_double (json_ctx, "mean", r.mean);
  json_attr_object_end (json_ctx);
}

static void
json_throughput (json_ctx_t *json_ctx, double (*bench) (FILE *), FILE *f)
{
  double r = bench (f);
  json_attr_object_begin (json_ctx, "throughput");
  json_attr_double (json_ctx, "bytes/s", r);
  json_attr_object_end (json_ctx);
}

static int
do_test (void)
{
  enum { fsize = 256 * 1024 * 1024 };
  FILE *f = open_bench_memfile (fsize);

  json_ctx_t json_ctx;
  json_init (&json_ctx, 0, stdout);
  json_document_begin (&json_ctx);

  json_attr_string (&json_ctx, "timing_type", TIMING_TYPE);
  json_attr_object_begin (&json_ctx, "stdio locks");

  json_attr_object_begin (&json_ctx, "single thread");
  json_latency (&json_ctx, bench_latency, f);
  json_throughput (&json_ctx, bench_throughput, f);
  json_attr_object_end (&json_ctx);

  json_attr_object_begin (&json_ctx, "multi-thread");
  json_latency (&json_ctx, bench_latency_mt, f);
  json_throughput (&json_ctx, bench_throughput_mt, f);
  json_attr_object_end (&json_ctx);

  json_attr_object_end (&json_ctx);
  json_document_end (&json_ctx);

  xfclose (f);

  return 0;
}

#include <support/test-driver.c>
