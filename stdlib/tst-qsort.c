/* Basic qsort tests with internal alignment checks.
   Copyright (C) 2002-2021 Free Software Foundation, Inc.
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

/* Test case by Paul Eggert <eggert@twinsun.com> */
#include <stdio.h>
#include <stdlib.h>

#include <array_length.h>
#include <tst-stack-align.h>

#include <support/check.h>

struct big { char c[4 * 1024]; };

struct big *array;
struct big *array_end;

static int align_check;

static int
compare (void const *a1, void const *b1)
{
  struct big const *a = a1;
  struct big const *b = b1;

  if (!align_check)
    align_check = TEST_STACK_ALIGN () ? -1 : 1;

  TEST_VERIFY_EXIT (array <= a);
  TEST_VERIFY_EXIT (a < array_end);
  TEST_VERIFY_EXIT (array <= b);
  TEST_VERIFY_EXIT (b < array_end);

  return (b->c[0] - a->c[0]) > 0;
}

static int
do_test (void)
{
  const size_t sizes[] = { 8, 16, 24, 48, 96, 192, 384 };

  for (const size_t *s = sizes; s < array_end (sizes); s++)
    {
      array = (struct big *) malloc (*s * sizeof *array);
      TEST_VERIFY_EXIT (array != NULL);

      array_end = array + *s;
      for (size_t i = 0; i < *s; i++)
        array[i].c[0] = i % 128;

      qsort (array, *s, sizeof *array, compare);
      TEST_VERIFY_EXIT (align_check != -1);

      free (array);
    }

  return 0;
}

#include <support/test-driver.c>
