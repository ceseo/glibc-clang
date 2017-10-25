/* Header for directory for shm/sem files.
   Copyright (C) 2014-2018 Free Software Foundation, Inc.
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

#ifndef _SHM_DIRECTORY_H

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <malloc/char_array-skeleton.c>

extern const char *__shm_directory (size_t *len);

enum
{
  __SHM_OK,
  __SHM_NO_DIR,
  __SHM_INVALID_NAME,
  __SHM_MEM_ERROR
};

static inline int
__shm_get_name (const char *prefix, const char *name,
		struct char_array *shm_name)
{
  size_t dirlen;
  const char *dir = __shm_directory (&dirlen);
  if (__glibc_unlikely (dir == NULL))
    return __SHM_NO_DIR;

  /* Construct the filename.  */
  while (name[0] == '/')
    ++name;
  size_t namelen = strlen (name) + 1;
  /* Validate the filename.  */
  if (namelen == 1 || namelen >= NAME_MAX || strchr (name, '/') != NULL)
    return __SHM_INVALID_NAME;

  if (!char_array_set_str (shm_name, dir)
      || !char_array_append_str (shm_name, prefix)
      || !char_array_append_str (shm_name, name))
    return __SHM_MEM_ERROR;

  return __SHM_OK;
}

/* Sets the shared memory directory on SHM_DIR char array and the shared
   memory filename on SHM_NAME using PREFIX and NAME as base.  */
static inline int
__shm_get_name_and_dir (const char *prefix, const char *name,
			struct char_array *shm_dir,
			struct char_array *shm_name)
{
  int ret =__shm_get_name (prefix, name, shm_name);
  if (ret != __SHM_OK)
    return ret;

  size_t dirlen;
  if (!char_array_set_str (shm_dir, __shm_directory (&dirlen)))
    return __SHM_MEM_ERROR;

  return __SHM_OK;
}

#endif	/* shm-directory.h */
