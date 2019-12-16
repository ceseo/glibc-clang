/* Test RELRO protection in a statically linked program.
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
#include <stdlib.h>
#include <support/check_fault.h>

static void (*const global_function_pointer) (void *) = free;

static int
do_test (void)
{
  printf ("info: checking global_function_pointer (%p)\n",
          &global_function_pointer);
  support_check_fault_write (&global_function_pointer,
                             sizeof (global_function_pointer));
  return 0;
}

#include <support/test-driver.c>
