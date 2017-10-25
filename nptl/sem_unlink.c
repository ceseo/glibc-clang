/* Copyright (C) 2002-2018 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

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

#include <errno.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "semaphoreP.h"
#include <shm-directory.h>

int
sem_unlink (const char *name)
{
  struct char_array sem_name;
  if (!char_array_init_empty (&sem_name))
    {
      __set_errno (ENOMEM);
      return -1;
    }
  int err = __shm_get_name (SEM_SHM_PREFIX, name, &sem_name);
  switch (err)
    {
    case __SHM_NO_DIR:       __set_errno (ENOSYS); return -1;
    case __SHM_INVALID_NAME: __set_errno (ENOENT); return -1;
    case __SHM_MEM_ERROR:    __set_errno (ENOMEM); return -1;
    case __SHM_OK:	     break;
    }

  /* Now try removing it.  */
  int ret = unlink (char_array_str (&sem_name));
  if (ret < 0 && errno == EPERM)
    __set_errno (EACCES);

  char_array_free (&sem_name);

  return ret;
}
