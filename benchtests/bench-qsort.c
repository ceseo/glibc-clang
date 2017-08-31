/* Measure qsort function.
   Copyright (C) 2018 Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include <array_length.h>

#include "json-lib.h"
#include "bench-timing.h"
#include "bench-util.h"

#include <support/test-driver.h>
#include <support/support.h>
#include <support/support_random.h>

/* Type of inputs arrays:
   - Sorted:       array already sorted in placed.
   - MostlySorted: sorted array with 'MostlySortedRatio * size' elements
		   in random positions set to random values.
   - Random:       all elements in array set to random values.
   - Repeated:     random array with 'RepeatedRation' elements in random
		   positions set to an unique value.
   - Bitonic:      strictly increasing to up middle then strictly
		   decreasing.  */
typedef enum
{
  Sorted,
  MostlySorted,
  Random,
  Repeated,
  Bitonic
} arraytype_t;

/* Ratio of total of elements which will randomized.  */
static const double MostlySortedRatio = 0.2;

/* Ratio of total of elements which will be repeated.  */
static const double RepeatedRatio = 0.2;

struct array_t
{
  arraytype_t type;
  const char *name;
} static const arraytypes[] =
{
  { Sorted,       "Sorted" },
  { MostlySorted, "MostlySorted" },
  { Random,       "Random" },
  { Repeated,     "Repeated" },
  { Bitonic,      "Bitonic" }
};

typedef int (*cmpfunc_t)(const void *, const void *);

/* Return the index of BASE as interpreted as an array of elements
   of size SIZE.  */
static inline void *
arr (void *base, size_t idx, size_t size)
{
  return (void*)((uintptr_t)base + (idx * size));
}

/* Function used to create the input arrays.  */
static support_random_state rand_state;

static void
seq (void *elem, size_t type_size, int value)
{
  if (type_size == sizeof (uint32_t))
    *(uint32_t*)elem = value;
  else if (type_size == sizeof (uint64_t))
    *(uint64_t*)elem = value;
  else
    *(uint32_t*)elem = value;
}

static void *
create_array (size_t nmemb, size_t type_size, arraytype_t type)
{
  size_t size = nmemb * type_size;
  void *array = xmalloc (size);

  switch (type)
    {
    case Sorted:
      for (size_t i = 0; i < nmemb; i++)
	seq (arr (array, i, type_size), type_size, i);
      break;

    case MostlySorted:
      {
        for (size_t i = 0; i < nmemb; i++)
	  seq (arr (array, i, type_size), type_size, i);

	/* Change UNSORTED elements (based on MostlySortedRatio ratio)
	   in the sorted array.  */
        size_t unsorted = (size_t)(nmemb * MostlySortedRatio);
	for (size_t i = 0; i < unsorted; i++)
	  {
	    size_t pos = support_random_uniform_distribution (&rand_state,
							      0, nmemb - 1);
            support_random_buf (&rand_state, arr (array, pos, type_size),
				type_size);
	  }
      }
      break;

    case Random:
      support_random_buf (&rand_state, array, size);
      break;

    case Repeated:
      {
        support_random_buf (&rand_state, array, size);

	void *randelem = xmalloc (type_size);
	support_random_buf (&rand_state, randelem, type_size);

	/* Repeat REPEATED elements (based on RepeatRatio ratio) in the random
	   array.  */
        size_t repeated = (size_t)(nmemb * RepeatedRatio);
	for (size_t i = 0; i < repeated; i++)
	  {
	    size_t pos = support_random_uniform_distribution (&rand_state,
							      0, nmemb - 1);
	    memcpy (arr (array, pos, type_size), randelem, type_size);
	  }
	free (randelem);
      }
      break;

    case Bitonic:
      {
	size_t i;
        for (i = 0; i < nmemb / 2; i++)
	  seq (arr (array, i, type_size), type_size, i);
        for (     ; i < nmemb;     i++)
	  seq (arr (array, i, type_size), type_size, (nmemb - 1) - i);
      }
      break;
    }

  return array;
}

/* Functions for uint32_t type.  */
static int
cmp_uint32_t (const void *a, const void *b)
{
  uint32_t ia = *(uint32_t*)a;
  uint32_t ib = *(uint32_t*)b;
  return (ia > ib) - (ia < ib);
}

/* Functions for uint64_t type.  */
static int
cmp_uint64_t (const void *a, const void *b)
{
  uint64_t ia = *(uint64_t*)a;
  uint64_t ib = *(uint64_t*)b;
  return (ia > ib) - (ia < ib);
}

/* Number of elements of determined type to be measured.  */
static const size_t default_elem[] =
{
  256/sizeof(size_t),       /* 64/128 which covers mostly used element number
			       on GCC build.  */
  32768/sizeof(size_t),	    /* 4096/8192 to fit on a L1 with 32 KB.  */
  262144/sizeof(size_t),    /* 32768/65536 to fit on a L2 with 256 KB.  */
  4194304/sizeof(size_t),   /* 524288/1048576 to fix on a L3 with 4 MB.  */
};

#define OPT_NELEM 10000
#define OPT_SEED  10001
#define CMDLINE_OPTIONS \
  { "nelem", required_argument, NULL, OPT_NELEM }, \
  { "seed", required_argument, NULL, OPT_SEED },

static const size_t max_nelem = 16;
static size_t *elems = NULL;
static size_t nelem = 0;
static uint64_t seed = 0;
static bool seed_set = false;

static void
cmdline_process_function (int c)
{
  switch (c)
    {
      /* Handle the --nelem option to run different sizes than DEFAULT_ELEM.
	 The elements sizes as passed with a ':' as the delimiter, for
	 instance --nelem 32:128:1024 will ran 32, 128, and 1024 elements.  */
      case OPT_NELEM:
        {
	  elems = xmalloc (max_nelem * sizeof (size_t));
	  nelem = 0;

	  char *saveptr;
	  char *token;
	  token = strtok_r (optarg, ":", &saveptr);
	  if (token == NULL)
	    {
	      printf ("error: invalid --nelem value\n");
	      exit (EXIT_FAILURE);
	    }
	  do
	    {
	      if (nelem == max_nelem)
		{
		  printf ("error: invalid --nelem value (max elem)\n");
		  exit (EXIT_FAILURE);
		}
	      elems[nelem++] = atol (token);
	      token = strtok_r (saveptr, ":", &saveptr);
	    } while (token != NULL);
        }
      break;

      /* handle the --seed option to use a different seed than a random one.
	 The SEED used should be a uint64_t number.  */
      case OPT_SEED:
	{
	  uint64_t value = strtoull (optarg, NULL, 0);
	  if (errno == ERANGE || value > UINT64_MAX)
	    {
	      printf ("error: seed should be a value in range of "
		      "[0, UINT64_MAX]\n");
	      exit (EXIT_FAILURE);
	    }
	  seed = value;
	  seed_set = true;
	}
    }
}

#define CMDLINE_PROCESS cmdline_process_function

static const size_t inner_loop_iters = 16;

struct run_t
{
  size_t type_size;
  cmpfunc_t cmp;
} static const runs[] =
{
  { sizeof (uint32_t), cmp_uint32_t },
  { sizeof (uint64_t), cmp_uint64_t },
  { 32,                cmp_uint32_t },
};

static int
do_test (void)
{
  if (seed_set)
    support_random_seed (&rand_state, seed);
  else
    support_random_rseed (&rand_state);

  json_ctx_t json_ctx;

  json_init (&json_ctx, 0, stdout);

  json_document_begin (&json_ctx);
  json_attr_string (&json_ctx, "timing_type", TIMING_TYPE);

  json_attr_object_begin (&json_ctx, "functions");
  json_attr_object_begin (&json_ctx, "qsort");
  json_attr_uint (&json_ctx, "seed", seed);

  json_array_begin (&json_ctx, "results");

  const size_t *welem = elems == NULL ? default_elem : elems;
  const size_t wnelem = elems == NULL ? array_length (default_elem) : nelem;

  for (const struct run_t *run = runs; run < array_end (runs); ++run)
    {
      for (const struct array_t *arraytype = arraytypes;
	   arraytype < array_end (arraytypes);
	   ++arraytype)
	{
	  for (int k = 0; k < wnelem; k++)
	    {
	      json_element_object_begin (&json_ctx);

	      size_t arraysize = welem[k] * run->type_size;

	      json_attr_uint (&json_ctx, "nmemb", welem[k]);
	      json_attr_uint (&json_ctx, "type_size", run->type_size);
	      json_attr_string (&json_ctx, "property", arraytype->name);

	      void *base = create_array (welem[k], run->type_size,
					 arraytype->type);
	      void *work = xmalloc (arraysize);

	      timing_t total = 0;

	      for (int n = 0; n < inner_loop_iters; n++)
	        {
		  memcpy (work, base, arraysize);

	          timing_t start, end, diff;
	          TIMING_NOW (start);
	          qsort (work, welem[k], run->type_size, run->cmp);
	          TIMING_NOW (end);

	          TIMING_DIFF (diff, start, end);
	          TIMING_ACCUM (total, diff);
	        }

	     json_attr_uint (&json_ctx, "timings",
			     (double) total / (double) inner_loop_iters);
	     json_element_object_end (&json_ctx);

	     free (base);
	     free (work);
	   }
        }
    }

  json_array_end (&json_ctx);

  json_attr_object_end (&json_ctx);
  json_attr_object_end (&json_ctx);
  json_document_end (&json_ctx);

  return 0;
}

#define TIMEOUT 600
#include <support/test-driver.c>
