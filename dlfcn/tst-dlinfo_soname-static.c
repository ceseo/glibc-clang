/* Test for the dlinfo_soname function.  Static version.
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

#include <dlfcn.h>
#include <gnu/lib-names.h>
#include <link.h>
#include <stddef.h>
#include <stdlib.h>
#include <support/check.h>
#include <support/support.h>
#include <support/xdlfcn.h>

static int
do_test (void)
{
  Dl_info info;
  void *handle;

  /* Check the soname of a library loaded by soname.  */
  void *moddummy1_handle = xdlopen ("moddummy1.so", RTLD_NOW);
  TEST_COMPARE_STRING (dlinfo_soname (moddummy1_handle), "moddummy1.so");

  /* This is an arbitrary symbol in libc.so.  It is used to locate a
     handle for libc.so only.  */
  void *ptr = xdlsym (moddummy1_handle, "grantpt");
  if (dladdr1 (ptr, &info, &handle, RTLD_DL_HANDLE) == 0)
    FAIL_EXIT1 ("dladdr1 for grantpt in namespace failed");
  TEST_COMPARE_STRING (dlinfo_soname (handle), LIBC_SO);

  /* Check the soname of a library loaded by full path.  */
  char *full_path = xasprintf ("%s/dlfcn/moddummy2.so", support_objdir_root);
  void *moddummy2_handle = xdlopen (full_path, RTLD_NOW);
  TEST_COMPARE_STRING (dlinfo_soname (moddummy2_handle), "moddummy2.so");

  xdlclose (moddummy2_handle);
  free (full_path);
  xdlclose (moddummy1_handle);

  return 0;
}

#include <support/test-driver.c>
