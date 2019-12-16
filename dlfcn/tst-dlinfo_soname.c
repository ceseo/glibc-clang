/* Test for the dlinfo_soname function.  Shared version.
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
#include <dlfcn.h>
#include <gnu/lib-names.h>
#include <link.h>
#include <stddef.h>
#include <stdlib.h>
#include <support/check.h>
#include <support/support.h>
#include <support/xdlfcn.h>

static void
local_function (void)
{
}

static int
do_test (void)
{
  Dl_info info;
  void *handle;

  if (dladdr1 (&local_function, &info, &handle, RTLD_DL_HANDLE) == 0)
    FAIL_EXIT1 ("dladdr1 for local_function failed");
  /* The main program does not have a soname.  */
  TEST_VERIFY (dlinfo_soname (handle) == NULL);

  /* Check the soname of libc. Use an obscure libc function as
     reference, to avoid PLT stubs and similar constructs moving the
     active definition to another object.  */
  void *ptr = xdlsym (NULL, "grantpt");
  if (dladdr1 (ptr, &info, &handle, RTLD_DL_HANDLE) == 0)
    FAIL_EXIT1 ("dladdr1 for grantpt failed");
  TEST_COMPARE_STRING (dlinfo_soname (handle), LIBC_SO);

  /* Check the soname of a library loaded by soname.  */
  void *moddummy1_handle = xdlopen ("moddummy1.so", RTLD_NOW);
  TEST_COMPARE_STRING (dlinfo_soname (moddummy1_handle), "moddummy1.so");

  /* Check the soname of a library loaded by full path.  */
  char *full_path = xasprintf ("%s/dlfcn/moddummy2.so", support_objdir_root);
  void *moddummy2_handle = xdlopen (full_path, RTLD_NOW);
  TEST_COMPARE_STRING (dlinfo_soname (moddummy2_handle), "moddummy2.so");
  ptr = xdlsym (moddummy2_handle, "grantpt");
  if (dladdr1 (ptr, &info, &handle, RTLD_DL_HANDLE) == 0)
    FAIL_EXIT1 ("dladdr1 for grantpt in namespace failed");
  TEST_COMPARE_STRING (dlinfo_soname (handle), LIBC_SO);

  xdlclose (moddummy2_handle);
  free (full_path);
  xdlclose (moddummy1_handle);

  return 0;
}

#include <support/test-driver.c>
