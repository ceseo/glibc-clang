/* Copyright (C) 1993-2022 Free Software Foundation, Inc.
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
   <https://www.gnu.org/licenses/>.

   As a special exception, if you link the code in this file with
   files compiled with a GNU compiler to produce an executable,
   that does not cause the resulting executable to be covered by
   the GNU Lesser General Public License.  This exception does not
   however invalidate any other reasons why the executable file
   might be covered by the GNU Lesser General Public License.
   This exception applies to code released by its copyright holders
   in files containing the exception.  */

#include "libioP.h"
#include "strfile.h"

int __THROW
_IO_str_chk_overflow (FILE *fp, int c)
{
  /* If we get here, the user-supplied buffer would be overrun by
     further output.  */
  __chk_fail ();
}

/* This function is called by regular vsprintf with maxlen set to -1,
   and by vsprintf_chk with maxlen set to the size of the output
   string.  In the former case, _IO_str_chk_overflow will never be
   called; in the latter case it will crash the program if the buffer
   overflows.  */

int
__vsprintf_internal (char *string, size_t maxlen,
		     const char *format, va_list args,
		     unsigned int mode_flags)
{
  _IO_strfile sf;
  int ret;

#ifdef _IO_MTSAFE_IO
  sf._sbf._f._lock = NULL;
#endif
  _IO_no_init (&sf._sbf._f, _IO_USER_LOCK, -1, NULL, NULL);
  /* When called from fortified sprintf/vsprintf, erase the destination
     buffer and try to detect overflows.  When called from regular
     sprintf/vsprintf, do not erase the destination buffer, because
     known user code relies on this behavior (even though its undefined
     by ISO C), nor try to detect overflows.  */
  if ((mode_flags & PRINTF_CHK) != 0)
    {
      _IO_JUMPS (&sf._sbf) = &_IO_str_chk_jumps;
      string[0] = '\0';
    }
  else
    _IO_JUMPS (&sf._sbf) = &_IO_str_jumps;
  _IO_str_init_static_internal (&sf, string,
				(maxlen == -1) ? -1 : maxlen - 1,
				string);

  ret = __vfprintf_internal (&sf._sbf._f, format, args, mode_flags);

  *sf._sbf._f._IO_write_ptr = '\0';
  return ret;
}

int
__vsprintf (char *string, const char *format, va_list args)
{
  return __vsprintf_internal (string, -1, format, args, 0);
}

ldbl_strong_alias (__vsprintf, _IO_vsprintf)
ldbl_weak_alias (__vsprintf, vsprintf)
