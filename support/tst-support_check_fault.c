/* Tests for support_fault_check_write.
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

#include <array_length.h>
#include <stdlib.h>
#include <support/capture_subprocess.h>
#include <support/check.h>
#include <support/support.h>
#include <support/xunistd.h>
#include <sys/mman.h>

static void
check_fault_two_bytes (void *address)
{
  support_check_fault_write (address, 2);
}

static int
do_test (void)
{
  size_t page_size = xsysconf (_SC_PAGESIZE);

  /* Tests with PROT_NONE, PROT_READ and PROT_EXEC are expected to
     succeed.  */
  {
    static const int prots[] = { PROT_NONE, PROT_READ, PROT_EXEC };
    for (int i = 0; i < array_length (prots); ++i)
      {
        void *ptr = xmmap (NULL, 2, prots[i], MAP_PRIVATE | MAP_ANONYMOUS, -1);
        support_check_fault_write (ptr, 2);
        xmunmap (ptr, 2);
      }
  }

  /* Do not call support_record_failure_reset below if the process has
     already failed.  */
  int is_failed = support_record_failure_is_failed ();

  /* Check that the lack of a fault for regular memory is reported at
     the fist byte.  */
  {
    void *ptr = xmalloc (2);
    struct support_capture_subprocess proc
      = support_capture_subprocess (check_fault_two_bytes, ptr);
    support_capture_subprocess_check (&proc, "malloc", 0, sc_allow_stdout);
    if (!is_failed)
      support_record_failure_reset ();
    char *expected = xasprintf ("error: missing fault at %p (%p+0)\n",
                                ptr, ptr);
    TEST_COMPARE_STRING (proc.out.buffer, expected);
    free (expected);
    support_capture_subprocess_free (&proc);
    free (ptr);
  }

  /* Check that a fault only at the first byte results in an error.
     The probe range crosses a page boundary.  */
  is_failed = support_record_failure_is_failed ();
  {
    char *ptr = xmmap (NULL, 2 * page_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1);
    xmprotect (ptr, page_size, PROT_READ);
    struct support_capture_subprocess proc
      = support_capture_subprocess (check_fault_two_bytes,
                                    ptr + page_size - 1);
    support_capture_subprocess_check (&proc, "R, RW", 0, sc_allow_stdout);
    if (!is_failed)
      support_record_failure_reset ();
    char *expected = xasprintf ("error: missing fault at %p (%p+1)\n",
                                ptr + page_size, ptr + page_size - 1);
    TEST_COMPARE_STRING (proc.out.buffer, expected);
    free (expected);
    support_capture_subprocess_free (&proc);
    xmunmap (ptr, 2 * page_size);
  }

  /* Check that a fault only at the second byte results in an error.
     Again, the probe range crosses a page boundary.  */
  is_failed = support_record_failure_is_failed ();
  {
    char *ptr = xmmap (NULL, 2 * page_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1);
    xmprotect (ptr + page_size, page_size, PROT_READ);
    struct support_capture_subprocess proc
      = support_capture_subprocess (check_fault_two_bytes,
                                    ptr + page_size - 1);
    support_capture_subprocess_check (&proc, "RW, R", 0, sc_allow_stdout);
    if (!is_failed)
      support_record_failure_reset ();
    char *expected = xasprintf ("error: missing fault at %p (%p+0)\n",
                                ptr + page_size - 1, ptr + page_size - 1);
    TEST_COMPARE_STRING (proc.out.buffer, expected);
    free (expected);
    support_capture_subprocess_free (&proc);
    xmunmap (ptr, 2 * page_size);
  }

  return 0;
}

#include <support/test-driver.c>
