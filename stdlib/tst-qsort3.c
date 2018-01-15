/* qsort(_r) generic tests.
   Copyright (C) 2021 Free Software Foundation, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <stdbool.h>

#include <array_length.h>

#include <support/check.h>
#include <support/support.h>
#include <support/support_random.h>
#include <support/test-driver.h>

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
  { Random,       "Random" },
  { MostlySorted, "MostlySorted" },
  { Repeated,     "Repeated" },
  { Bitonic,      "Bitonic" },
};

/* Return the index of BASE as interpreted as an array of elements
   of size SIZE.  */
static inline void *
arr (void *base, size_t idx, size_t size)
{
  return (void*)((uintptr_t)base + (idx * size));
}

/* Functions used to check qsort.  */
static int
uint8_t_cmp (const void *a, const void *b)
{
  uint8_t ia = *(uint8_t*)a;
  uint8_t ib = *(uint8_t*)b;
  return (ia > ib) - (ia < ib);
}

static int
uint16_t_cmp (const void *a, const void *b)
{
  uint16_t ia = *(uint16_t*)a;
  uint16_t ib = *(uint16_t*)b;
  return (ia > ib) - (ia < ib);
}

static int
uint32_t_cmp (const void *a, const void *b)
{
  uint32_t ia = *(uint32_t*)a;
  uint32_t ib = *(uint32_t*)b;
  return (ia > ib) - (ia < ib);
}

static int
uint64_t_cmp (const void *a, const void *b)
{
  uint64_t ia = *(uint64_t*)a;
  uint64_t ib = *(uint64_t*)b;
  return (ia > ib) - (ia < ib);
}

/* Function used to check qsort_r.  */
typedef enum
{
  UINT8_CMP_T,
  UINT16_CMP_T,
  UINT32_CMP_T,
  UINT64_CMP_T
} type_cmp_t;

static type_cmp_t
uint_t_cmp_type (size_t sz)
{
  switch (sz)
    {
      case sizeof (uint8_t):  return UINT8_CMP_T;
      case sizeof (uint16_t): return UINT16_CMP_T;
      case sizeof (uint64_t): return UINT64_CMP_T;
      case sizeof (uint32_t):
      default:                return UINT32_CMP_T;
    }
}

static int
uint_t_cmp (const void *a, const void *b, void *arg)
{
  type_cmp_t type = *(type_cmp_t*) arg;
  switch (type)
    {
    case UINT8_CMP_T:  return uint8_t_cmp (a, b);
    case UINT16_CMP_T: return uint16_t_cmp (a, b);
    case UINT64_CMP_T: return uint64_t_cmp (a, b);
    case UINT32_CMP_T:
    default:           return uint32_t_cmp (a, b);
    }
}

/* Function used to create the input arrays.  */
static support_random_state rand_state;

static void
seq (void *elem, size_t type_size, int value)
{
  if (type_size == sizeof (uint8_t))
    *(uint8_t*)elem = value;
  else if (type_size == sizeof (uint16_t))
    *(uint16_t*)elem = value;
  else if (type_size == sizeof (uint32_t))
    *(uint32_t*)elem = value;
  else if (type_size == sizeof (uint64_t))
    *(uint64_t*)elem = value;
  else
    memset (elem, value, type_size);
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

typedef int (*cmpfunc_t)(const void *, const void *);

/* Check if ARRAY of total NMEMB element of size SIZE is sorted
   based on CMPFUNC.  */
static void
check_array (void *array, size_t nmemb, size_t type_size,
	     cmpfunc_t cmpfunc)
{
  for (size_t i = 1; i < nmemb; i++)
    {
      int ret = cmpfunc (arr (array, i,   type_size),
			 arr (array, i-1, type_size));
      TEST_VERIFY_EXIT (ret >= 0);
    }
}

static void
check_qsort (size_t nelem, size_t type_size, arraytype_t type,
	     cmpfunc_t cmpfunc)
{
  void *array = create_array (nelem, type_size, type);

  qsort (array, nelem, type_size, cmpfunc);

  check_array (array, nelem, type_size, cmpfunc);

  free (array);
}

static void
check_qsort_r (size_t nelem, size_t type_size, arraytype_t type,
	       cmpfunc_t cmpfunc)
{
  void *array = create_array (nelem, type_size, type);

  type_cmp_t typecmp = uint_t_cmp_type (type_size);
  qsort_r (array, nelem, type_size, uint_t_cmp, &typecmp);

  check_array (array, nelem, type_size, cmpfunc);

  free (array);
}

static int
do_test (void)
{
  support_random_rseed (&rand_state);

  const size_t nelems[] = { 0, 1, 16, 32, 64, 128, 256, 4096, 16384, 262144 };

  struct test_t
    {
      size_t type_size;
      cmpfunc_t cmpfunc;
    }
  const tests[] =
    {
      { sizeof (uint8_t),  uint8_t_cmp },
      { sizeof (uint16_t), uint16_t_cmp },
      { sizeof (uint32_t), uint32_t_cmp },
      { sizeof (uint64_t), uint64_t_cmp },
      /* Test swap with large elements.  */
      { 32,                uint32_t_cmp },
    };

  for (const struct test_t *test = tests; test < array_end (tests); ++test)
    {
      if (test_verbose > 0)
	printf ("info: testing qsort with type_size=%zu\n", test->type_size);
      for (const struct array_t *arraytype = arraytypes;
	   arraytype < array_end (arraytypes);
	   ++arraytype)
	{
	  if (test_verbose > 0)
            printf ("  distribution=%s\n", arraytype->name);
	  for (const size_t *nelem = nelems;
	       nelem < array_end (nelems);
	       ++nelem)
	    {
	      if (test_verbose > 0)
		printf ("  i  nelem=%zu, total size=%zu\n", *nelem,
			*nelem * test->type_size);

	      check_qsort (*nelem, test->type_size, arraytype->type,
			   test->cmpfunc);
	      check_qsort_r (*nelem, test->type_size, arraytype->type,
			   test->cmpfunc);
	   }
	}
    }

  return 0;
}

#include <support/test-driver.c>
