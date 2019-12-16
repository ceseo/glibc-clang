/* Test for dladdr1 with RTLD_DL_HANDLE.
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
#include <support/check.h>
#include <support/xdlfcn.h>
#include <stddef.h>

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

  /* The handle should be usable with dlsym.  */
  TEST_VERIFY (xdlsym (handle, "dlsym") == xdlsym (NULL, "dlsym"));

  /* Check that libc is correctly identified.  Use an obscure libc
     function as reference, to avoid PLT stubs and similar constructs
     moving the active definition to another object.  */
  void *libc_handle = xdlopen (LIBC_SO, RTLD_NOW);
  void *ptr = xdlsym (libc_handle, "grantpt");
  if (dladdr1 (ptr, &info, &handle, RTLD_DL_HANDLE) == 0)
    FAIL_EXIT1 ("dladdr1 for grantpt failed");
  TEST_VERIFY (handle == libc_handle);

  /* Check that the handle of a new-loaded shared object is
     returned from dladdr1.  */
  void *moddummy1_handle = xdlopen ("moddummy1.so", RTLD_NOW);
  ptr = xdlsym (moddummy1_handle, "dummy1");
  if (dladdr1 (ptr, &info, &handle, RTLD_DL_HANDLE) == 0)
    FAIL_EXIT1 ("dladdr1 for dummy1 failed");
  TEST_VERIFY (handle == moddummy1_handle);

  /* Check that the handle of a shared object loaded into a new
     namespace is returned from dladdr1.  */
  void *moddummy2_handle = xdlmopen (LM_ID_NEWLM, "moddummy2.so", RTLD_NOW);
  ptr = xdlsym (moddummy2_handle, "dummy2");
  if (dladdr1 (ptr, &info, &handle, RTLD_DL_HANDLE) == 0)
    FAIL_EXIT1 ("dladdr1 for dummy2 failed");
  TEST_VERIFY (handle == moddummy2_handle);

  xdlclose (moddummy2_handle);
  xdlclose (moddummy1_handle);
  xdlclose (libc_handle);

  return 0;
}

#include <support/test-driver.c>
