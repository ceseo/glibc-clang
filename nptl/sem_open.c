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
#include <fcntl.h>
#include <pthread.h>
#include <search.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "semaphoreP.h"
#include <shm-directory.h>
#include <futex-internal.h>
#include <libc-lock.h>

/* Lock to protect the search tree.  */
static int sem_mappings_lock = LLL_LOCK_INITIALIZER;

/* Keeping track of currently used mappings.  */
struct inuse_sem
{ 
  ino_t ino;
  int refcnt;
  sem_t *sem;
};

#define DYNARRAY_STRUCT        sem_array
#define DYNARRAY_ELEMENT       struct inuse_sem
#define DYNARRAY_PREFIX        sem_array_
#define DYNARRAY_INITIAL_SIZE  16
#include <malloc/dynarray-skeleton.c>

static struct sem_array sem_mapping;
static bool sem_array_initted = false;

/* Search for existing mapping and if possible add the one provided.  */
static sem_t *
check_add_mapping (int fd, sem_t *existing)
{
  sem_t *result = SEM_FAILED;

  /* Get the information about the file.  */
  struct stat64 st;
  if (__fxstat64 (_STAT_VER, fd, &st) == 0)
    {
      lll_lock (sem_mappings_lock, LLL_PRIVATE);

      if (!sem_array_initted)
	{
	   sem_array_init (&sem_mapping);
	   sem_array_initted = true;
	}

      size_t first_unused = -1;
      size_t i = 0, limit = sem_array_size (&sem_mapping);
      for (; i < limit; i++)
	{
	  struct inuse_sem *rec = sem_array_at (&sem_mapping, i);
	  if (rec->sem == 0)
	    first_unused = i;
	  else if (rec->ino == st.st_ino)
	    {
	      result = rec->sem;
	      rec->refcnt++;
	      break;
	    }
	}

      if (i == limit)
	{
	  if (existing == SEM_FAILED)
	    existing = (sem_t *) mmap (NULL, sizeof (sem_t),
				       PROT_READ | PROT_WRITE, MAP_SHARED,
				       fd, 0);
	  if (existing != MAP_FAILED)
	    {
	      if (first_unused != -1)
		{
		  *sem_array_at (&sem_mapping, first_unused) =
		    (struct inuse_sem) { st.st_ino, 1, existing };
		  result = existing;
		}
	      else
		{
	          sem_array_add (&sem_mapping,
				 (struct inuse_sem) { st.st_ino, 1, existing });
		  if (!sem_array_has_failed (&sem_mapping))	
		    result = existing;
		}
	    }
	}

      lll_unlock (sem_mappings_lock, LLL_PRIVATE);
    }

  if (result != existing && existing != SEM_FAILED && existing != MAP_FAILED)
    {
      /* Do not disturb errno.  */
      int save = errno;
      munmap (existing, sizeof (sem_t));
      errno = save;
    }

  return result;
}


sem_t *
sem_open (const char *name, int oflag, ...)
{
  int fd;
  sem_t *result;

  /* Check that shared futexes are supported.  */
  int err = futex_supports_pshared (PTHREAD_PROCESS_SHARED);
  if (err != 0)
    {
      __set_errno (err);
      return SEM_FAILED;
    }

  /* Create the name of the final file in local variable SHM_NAME.  */
  struct char_array sem_name, sem_dir, tmpfname;
  if (!char_array_init_empty (&sem_name)
      || !char_array_init_empty (&sem_dir)
      || !char_array_init_empty (&tmpfname))
    {
      __set_errno (ENOMEM);
      return SEM_FAILED;
    }

  err = __shm_get_name_and_dir (SEM_SHM_PREFIX, name, &sem_dir, &sem_name);
  switch (err)
    {
    case __SHM_NO_DIR:       __set_errno (ENOSYS); return SEM_FAILED;
    case __SHM_INVALID_NAME: __set_errno (EINVAL); return SEM_FAILED;
    case __SHM_MEM_ERROR:    __set_errno (ENOMEM); return SEM_FAILED;
    case __SHM_OK:	     break;
    }

  /* Disable asynchronous cancellation.  */
#ifdef __libc_ptf_call
  int state;
  __libc_ptf_call (__pthread_setcancelstate,
                   (PTHREAD_CANCEL_DISABLE, &state), 0);
#endif

  /* If the semaphore object has to exist simply open it.  */
  if ((oflag & O_CREAT) == 0 || (oflag & O_EXCL) == 0)
    {
    try_again:
      fd = __libc_open (char_array_str (&sem_name),
			(oflag & ~(O_CREAT|O_ACCMODE)) | O_NOFOLLOW | O_RDWR);

      if (fd == -1)
	{
	  /* If we are supposed to create the file try this next.  */
	  if ((oflag & O_CREAT) != 0 && errno == ENOENT)
	    goto try_create;

	  /* Return.  errno is already set.  */
	}
      else
	/* Check whether we already have this semaphore mapped and
	   create one if necessary.  */
	result = check_add_mapping (fd, SEM_FAILED);
    }
  else
    {
      /* We have to open a temporary file first since it must have the
	 correct form before we can start using it.  */
      mode_t mode;
      unsigned int value;
      va_list ap;

    try_create:
      va_start (ap, oflag);

      mode = va_arg (ap, mode_t);
      value = va_arg (ap, unsigned int);

      va_end (ap);

      if (value > SEM_VALUE_MAX)
	{
	  __set_errno (EINVAL);
	  result = SEM_FAILED;
	  goto out;
	}

      /* Create the initial file content.  */
      union
      {
	sem_t initsem;
	struct new_sem newsem;
      } sem = { 0 };

#if __HAVE_64B_ATOMICS
      sem.newsem.data = value;
#else
      sem.newsem.value = value << SEM_VALUE_SHIFT;
      /* pad is used as a mutex on pre-v9 sparc and ignored otherwise.  */
      sem.newsem.pad = 0;
      sem.newsem.nwaiters = 0;
#endif
      /* This always is a shared semaphore.  */
      sem.newsem.private = FUTEX_SHARED;

      if (!char_array_set_array (&tmpfname, &sem_dir))
	{
	  result = SEM_FAILED;
	  goto out;
	}
      size_t sem_dirlen = char_array_length (&sem_dir);

      int retries = 0;
#define NRETRIES 50
      while (1)
	{
	  /* Add the suffix for mktemp.  */
	  if (!char_array_replace_str_pos (&tmpfname, sem_dirlen,
					   "XXXXXX", sizeof ("XXXXXX")))
	    {
	      result = SEM_FAILED;
	      goto out;
	    }

	  /* We really want to use mktemp here.  We cannot use mkstemp
	     since the file must be opened with a specific mode.  The
	     mode cannot later be set since then we cannot apply the
	     file create mask.  */
	  if (__mktemp (char_array_begin (&tmpfname)) == NULL)
	    {
	      result = SEM_FAILED;
	      goto out;
	    }

	  /* Open the file.  Make sure we do not overwrite anything.  */
	  fd = __libc_open (char_array_str (&tmpfname),
			    O_RDWR | O_CREAT | O_EXCL, mode);
	  if (fd == -1)
	    {
	      if (errno == EEXIST)
		{
		  if (++retries < NRETRIES)
		    continue;

		  __set_errno (EAGAIN);
		}

	      result = SEM_FAILED;
	      goto out;
	    }

	  /* We got a file.  */
	  break;
	}

      if (TEMP_FAILURE_RETRY (__libc_write (fd, &sem.initsem, sizeof (sem_t)))
	  == sizeof (sem_t)
	  /* Map the sem_t structure from the file.  */
	  && (result = (sem_t *) mmap (NULL, sizeof (sem_t),
				       PROT_READ | PROT_WRITE, MAP_SHARED,
				       fd, 0)) != MAP_FAILED)
	{
	  /* Create the file.  Don't overwrite an existing file.  */
	  if (link (char_array_str (&tmpfname),
		    char_array_str (&sem_name)) != 0)
	    {
	      /* Undo the mapping.  */
	      (void) munmap (result, sizeof (sem_t));

	      /* Reinitialize 'result'.  */
	      result = SEM_FAILED;

	      /* This failed.  If O_EXCL is not set and the problem was
		 that the file exists, try again.  */
	      if ((oflag & O_EXCL) == 0 && errno == EEXIST)
		{
		  /* Remove the file.  */
		  unlink (char_array_str (&tmpfname));

		  /* Close the file.  */
		  (void) __libc_close (fd);

		  goto try_again;
		}
	    }
	  else
	    /* Insert the mapping into the search tree.  This also
	       determines whether another thread sneaked by and already
	       added such a mapping despite the fact that we created it.  */
	    result = check_add_mapping (fd, result);
	}

      /* Now remove the temporary name.  This should never fail.  If
	 it fails we leak a file name.  Better fix the kernel.  */
      unlink (char_array_str (&tmpfname));
    }

  /* Map the mmap error to the error we need.  */
  if (MAP_FAILED != (void *) SEM_FAILED && result == MAP_FAILED)
    result = SEM_FAILED;

  /* We don't need the file descriptor anymore.  */
  if (fd != -1)
    {
      /* Do not disturb errno.  */
      int save = errno;
      __libc_close (fd);
      errno = save;
    }

out:
#ifdef __libc_ptf_call
  __libc_ptf_call (__pthread_setcancelstate, (state, NULL), 0);
#endif

   char_array_free (&sem_name);
   char_array_free (&sem_dir);
   char_array_free (&tmpfname);

  return result;
}

int
sem_close (sem_t *sem)
{
  int result = 0;

  lll_lock (sem_mappings_lock, LLL_PRIVATE);

  size_t i = 0, limit = sem_array_size (&sem_mapping);
  for (; i < limit; i++)
    {
      struct inuse_sem *rec = sem_array_at (&sem_mapping, i);
      if (rec->sem == sem)
	{
	  if (--rec->refcnt == 0)
	    {
	      rec->ino = 0;
	      rec->sem = NULL;

	      result = munmap (rec->sem, sizeof (sem_t));
	    }
	  break;
	}
    }
  if (i == limit)
    {
      result = -1;
      __set_errno (EINVAL);
    }

  lll_unlock (sem_mappings_lock, LLL_PRIVATE);

  return result;
}
