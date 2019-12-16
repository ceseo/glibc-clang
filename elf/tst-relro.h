/* Support code for testing RELRO protection.
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

#include <support/xdlfcn.h>
#include <support/check.h>
#include <support/check_fault.h>
#include <stdio.h>

/* Check that the symbol NAME in HANDLE (which can be null) is not
   located in writable memory.  The corresponding address must be
   located in the shared object with soname equal to EXPECTED_SONAME.

   This function uses dlsym to avoid introducing copy relocations,
   rather than accepting the pointer directly.  Otherwise, the test
   exercises RELRO protection in a different object.  */
static void
check_relro_symbol (void *handle, const char *name,
                    const char *expected_soname)
{
  void *ptr = xdlsym (handle, name);
  printf ("info: checking symbol %s in DSO %s at %p (handle %p)\n",
          name, expected_soname, ptr, handle);
  support_check_fault_write (ptr, 1);
  Dl_info info;
  void *actual_handle;
  if (dladdr1 (ptr, &info, &actual_handle, RTLD_DL_HANDLE) == 0)
    FAIL_EXIT1 ("dladdr1 for %s in %s failed", name, expected_soname);
  TEST_COMPARE_STRING (dlinfo_soname (actual_handle), expected_soname);
}
