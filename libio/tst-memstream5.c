/* Test for open_memstream BZ #15298.
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
   <http://www.gnu.org/licenses/>.  */

#include "tst-memstream.h"

static void
mcheck_abort (enum mcheck_status ev)
{
  FAIL_EXIT1 ("mecheck failed with status %d\n", (int) ev);
}

static int
do_test (void)
{
  mcheck_pedantic (mcheck_abort);

  {
    CHAR_T *buf;
    size_t size;

    FILE *fp = OPEN_MEMSTREAM (&buf, &size);
    TEST_VERIFY_EXIT (fp != NULL);

    /* Move internal position but do not write any bytes.  Final size should
       be 0.  */
    TEST_VERIFY_EXIT (fseek (fp, 10, SEEK_SET) != -1);
    TEST_VERIFY_EXIT (fseek (fp, 20, SEEK_CUR) != -1);
    TEST_VERIFY_EXIT (fseek (fp, 30, SEEK_CUR) != -1);
    TEST_VERIFY_EXIT (fflush (fp) != -1);
    TEST_VERIFY (size == 0);

    /* Now write some bytes and change internal position.  Final size should
      be based on written bytes.  */
    TEST_VERIFY_EXIT (fseek (fp, 0, SEEK_SET) != -1);
    TEST_VERIFY_EXIT (FWRITE (W("abc"), 1, 3, fp) == 3);
    TEST_VERIFY_EXIT (fseek (fp, 20, SEEK_CUR) != -1);
    TEST_VERIFY_EXIT (fseek (fp, 30, SEEK_CUR) != -1);
    TEST_VERIFY_EXIT (fflush (fp) != -1);
    TEST_VERIFY (size == 3);

    /* Finally set position, write some bytes and change position again.
       Final size should be based again on write position.  */
    size_t offset = 2048;
    TEST_VERIFY_EXIT (fseek (fp, offset, SEEK_SET) != -1);
    TEST_VERIFY_EXIT (FWRITE (W("def"), 1, 3, fp) == 3);
    TEST_VERIFY_EXIT (fseek (fp, 20, SEEK_CUR) != -1);
    TEST_VERIFY_EXIT (fseek (fp, 20, SEEK_CUR) != -1);
    TEST_VERIFY_EXIT (fflush (fp) != -1);
    TEST_VERIFY (size == (offset + 3));

    TEST_VERIFY_EXIT (fclose (fp) == 0);
    free (buf);
  }

  return 0;
}

#include <support/test-driver.c>
