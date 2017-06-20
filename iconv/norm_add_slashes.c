/* Normalize the charset name and add a suffix with slashes.
   Copyright (C) 1997-2017 Free Software Foundation, Inc.
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

#include <gconv_int.h>

#define CHAR_ARRAY_INITIAL_SIZE 0
#include <malloc/char_array-skeleton.c>

char *
__gconv_norm_add_slashes (const char *name, size_t name_len,
                          const char *suffix)
{
  size_t cnt = 0;
  for (size_t i = 0; i < name_len; i++)
    if (name[i] == '/')
      cnt++;

  struct char_array result;
  if (!char_array_init_str_size (&result, name, name_len))
    return NULL;

  for (size_t i = 0; i < char_array_length (&result); i++)
    *char_array_at (&result, i) = __toupper_l (name[i], _nl_C_locobj_ptr);

  if (cnt < 2)
    {
      if (!char_array_append_str (&result, "/"))
	return NULL;
      if (cnt < 1)
        {
	  if (!char_array_append_str (&result, "/")
	      | !char_array_append_str (&result, suffix))
	    return NULL;
        }
    }

  return char_array_finalize (&result, NULL);
}
