/* shm_open -- open a POSIX shared memory object.  Generic POSIX file version.
   Copyright (C) 2001-2018 Free Software Foundation, Inc.
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

#include <unistd.h>

#if ! _POSIX_MAPPED_FILES

# include <rt/shm_open.c>

#else

# include <fcntl.h>
# include <pthread.h>
# include <shm-directory.h>


/* Open shared memory object.  */
int
shm_open (const char *name, int oflag, mode_t mode)
{
  struct char_array shm_name;
  if (!char_array_init_empty (&shm_name))
    {
      __set_errno (ENOMEM);
      return -1;
    }
  int err = __shm_get_name ("", name, &shm_name);
  switch (err)
    {
    case __SHM_NO_DIR:       __set_errno (ENOSYS); return -1;
    case __SHM_INVALID_NAME: __set_errno (EINVAL); return -1;
    case __SHM_MEM_ERROR:    __set_errno (ENOMEM); return -1;
    case __SHM_OK:	     break;
    }

  oflag |= O_NOFOLLOW | O_CLOEXEC;

  int state;
  pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, &state);

  int fd = open (char_array_str (&shm_name), oflag, mode);
  if (fd == -1 && __glibc_unlikely (errno == EISDIR))
    /* It might be better to fold this error with EINVAL since
       directory names are just another example for unsuitable shared
       object names and the standard does not mention EISDIR.  */
    __set_errno (EINVAL);

  pthread_setcancelstate (state, NULL);

  char_array_free (&shm_name);

  return fd;
}

#endif  /* _POSIX_MAPPED_FILES */
