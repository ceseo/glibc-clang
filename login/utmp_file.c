/* Copyright (C) 1996-2021 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>
   and Paul Janzen <pcj@primenet.com>, 1996.

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

#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include <not-cancel.h>

#include <utmp-private.h>
#include <utmp-path.h>
#include <utmp-compat.h>
#include <shlib-compat.h>

#ifndef PATH_MAX
# define PATH_MAX 1024
#endif


/* Descriptor for the file and position.  */
static int file_fd = -1;
static bool file_writable;
static off64_t file_offset;


/* The utmp{x} internal functions work on two operations modes

   1. Read/write 64-bit time utmp{x} entries using the exported
      'struct utmp{x}'

   2. Read/write 32-bit time utmp{x} entries using the old 'struct utmp32'

   The operation mode mainly change the register size and how to interpret
   the 'last_entry' buffered record.  */
enum operation_mode_t
{
  UTMP_TIME64,
  UTMP_TIME32
};
static enum operation_mode_t cur_mode = UTMP_TIME64;

enum
{
  utmp_buffer_size = MAX (sizeof (struct utmp), sizeof (struct utmp32))
};

/* Cache for the last read entry.  */
static char last_entry[utmp_buffer_size];

static inline size_t last_entry_size (enum operation_mode_t mode)
{
  return mode == UTMP_TIME64 ? sizeof (struct utmp) : sizeof (struct utmp32);
}

static inline short int last_entry_type (enum operation_mode_t mode)
{
  short int r;
  if (mode == UTMP_TIME32)
    memcpy (&r, last_entry + offsetof (struct utmp32, ut_type), sizeof (r));
  else
    memcpy (&r, last_entry + offsetof (struct utmp, ut_type), sizeof (r));
  return r;
}

static inline const char *last_entry_id (enum operation_mode_t mode)
{
  if (mode == UTMP_TIME64)
   return ((struct utmp *) (last_entry))->ut_id;
  return ((struct utmp32 *) (last_entry))->ut_id;
}

static inline const char *last_entry_line (enum operation_mode_t mode)
{
  if (mode == UTMP_TIME64)
   return ((struct utmp *) (last_entry))->ut_line;
  return ((struct utmp32 *) (last_entry))->ut_line;
}

/* Returns true if *ENTRY matches last_entry, based on data->ut_type.  */
static bool
matches_last_entry (enum operation_mode_t mode, short int type,
		    const char *id, const char *line)
{
  if (file_offset <= 0)
    /* Nothing has been read.  last_entry is stale and cannot match.  */
    return false;

  switch (type)
    {
    case RUN_LVL:
    case BOOT_TIME:
    case OLD_TIME:
    case NEW_TIME:
      /* For some entry types, only a type match is required.  */
      return type == last_entry_type (mode);
    default:
      /* For the process-related entries, a full match is needed.  */
      return (type == INIT_PROCESS
	      || type == LOGIN_PROCESS
	      || type == USER_PROCESS
	      || type == DEAD_PROCESS)
	&& (last_entry_type (mode) == INIT_PROCESS
	    || last_entry_type (mode) == LOGIN_PROCESS
	    || last_entry_type (mode) == USER_PROCESS
	    || last_entry_type (mode) == DEAD_PROCESS)
	&& (id[0] != '\0' && last_entry_id (mode)[0] != '\0'
	    ? strncmp (id, last_entry_id (mode), 4 * sizeof (char)) == 0
	    : (strncmp (line, last_entry_id (mode), UT_LINESIZE) == 0));
    }
}

/* Construct a lock file base on FILE depending of DEFAULT_DB: if true
   the lock is constructed on /var/lock; otherwise is used the FILE
   path itself.  The lock file is also created if it does not exist if
   DEFAULT_DB is false.  */
static int
lock_write_file (const char *file, bool default_db)
{
  char path[PATH_MAX];
  if (default_db)
    __snprintf (path, sizeof (path), "/var/lock/%s.lock", __basename (file));
  else
    __snprintf (path, sizeof (path), "%s.lock", file);

  int flags = O_RDWR | O_LARGEFILE | O_CLOEXEC;
  mode_t mode = 0644;

  /* The errno need to reset if 'create_file' is set and the O_CREAT does not
     fail.  */
  int saved_errno = errno;
  int fd = __open_nocancel (path, flags, mode);
  if (fd == -1 && errno == ENOENT && !default_db)
    fd = __open_nocancel (path, flags | O_CREAT, mode);
  if (fd == -1)
    return -1;
  __set_errno (saved_errno);

  struct flock64 fl =
    {
     .l_type = F_WRLCK,
     .l_whence = SEEK_SET,
    };

  if (__fcntl64_nocancel (fd, F_SETLKW, &fl) == -1)
    {
      __close_nocancel_nostatus (fd);
      return -1;
    }
  return fd;
}

static void
unlock_write_file (const char *file, int lockfd, bool default_db)
{
  __close_nocancel_nostatus (lockfd);

  char path[PATH_MAX];
  __snprintf (path, sizeof (path), "/var/lock/%s.lock", __basename (file));
  if (! default_db)
    {
      /* Ignore error for the case the file does not exist.  */
      int saved_errno = errno;
      __unlink (path);
      __set_errno (saved_errno);
    }
}


static void
file_unlock (int fd)
{
  struct flock64 fl =
    {
      .l_type = F_UNLCK,
    };
  __fcntl64_nocancel (fd, F_SETLKW, &fl);
}

static bool
internal_setutent (enum operation_mode_t mode)
{
  if (file_fd < 0)
    {
      const char *file_name = mode == UTMP_TIME64
	?__libc_utmp_file_name
	: utmp_file_name_time32 (__libc_utmp_file_name);

      file_writable = false;
      file_fd = __open_nocancel
	(file_name, O_RDONLY | O_LARGEFILE | O_CLOEXEC);
      if (file_fd == -1)
	return false;
      cur_mode = mode;
    }

  __lseek64 (file_fd, 0, SEEK_SET);
  file_offset = 0;

  return true;
}

/* Preform initialization if necessary.  */
static bool
maybe_setutent (enum operation_mode_t mode)
{
  if (file_fd >= 0 && cur_mode != mode)
    {
      __close_nocancel_nostatus (file_fd);
      file_fd = -1;
      file_offset = 0;
    }
  return file_fd >= 0 || internal_setutent (mode);
}

/* Reads the entry at file_offset, storing it in last_entry and
   updating file_offset on success.  Returns -1 for a read error, 0
   for EOF, and 1 for a successful read.  last_entry and file_offset
   are only updated on a successful and complete read.  */
static ssize_t
read_last_entry (enum operation_mode_t mode)
{
  char buffer[utmp_buffer_size];
  const size_t size = last_entry_size (mode);
  ssize_t nbytes = __pread64_nocancel (file_fd, &buffer, size, file_offset);
  if (nbytes < 0)
    return -1;
  else if (nbytes != size)
    /* Assume EOF.  */
    return 0;
  else
    {
      memcpy (last_entry, buffer, size);
      file_offset += size;
      return 1;
    }
}

static int
internal_getutent_r (enum operation_mode_t mode, void *buffer)
{
  int saved_errno = errno;

  if (!maybe_setutent (mode))
    return -1;

  ssize_t nbytes = read_last_entry (mode);
  file_unlock (file_fd);

  if (nbytes <= 0)		/* Read error or EOF.  */
    {
      if (nbytes == 0)
	/* errno should be unchanged to indicate success.  A premature
	   EOF is treated like an EOF (missing complete record at the
	   end).  */
	__set_errno (saved_errno);
      return -1;
    }

  memcpy (buffer, &last_entry, last_entry_size (mode));
  return 0;
}

/* Search for *ID, updating last_entry and file_offset.  Return 0 on
   success and -1 on failure.  Does not perform locking; for that see
   internal_getut_r below.  */
static bool
internal_getut_nolock (enum operation_mode_t mode, short int type,
		       const char *id, const char *line)
{
  while (true)
    {
      ssize_t nbytes = read_last_entry (mode);
      if (nbytes < 0)
	return false;
      if (nbytes == 0)
	{
	  /* End of file reached.  */
	  __set_errno (ESRCH);
	  return false;
	}

      if (matches_last_entry (mode, type, id, line))
	break;
    }
  return true;
}

/* Search for *ID, updating last_entry and file_offset.  Return 0 on
   success and -1 on failure.  If the locking operation failed, write
   true to *LOCK_FAILED.  */
static bool
internal_getut_r (enum operation_mode_t mode, short int type, const char *id,
		  const char *line)
{
  bool r = internal_getut_nolock (mode, type, id, line);
  file_unlock (file_fd);
  return r;
}

static int
internal_getutid_r (enum operation_mode_t mode, short int type,
		    const char *id, const char *line, void *buffer)
{
  if (!maybe_setutent (mode))
    return -1;

  /* We don't have to distinguish whether we can lock the file or
     whether there is no entry.  */
  if (! internal_getut_r (mode, type, id, line))
    return -1;

  memcpy (buffer, &last_entry, last_entry_size (mode));

  return 0;
}

/* For implementing this function we don't use the getutent_r function
   because we can avoid the reposition on every new entry this way.  */
static int
internal_getutline_r (enum operation_mode_t mode, const char *line,
		      void *buffer)
{
  if (!maybe_setutent (mode))
    return -1;

  while (1)
    {
      ssize_t nbytes = read_last_entry (mode);
      if (nbytes < 0)
	{
	  file_unlock (file_fd);
	  return -1;
	}
      if (nbytes == 0)
	{
	  /* End of file reached.  */
	  file_unlock (file_fd);
	  __set_errno (ESRCH);
	  return -1;
	}

      /* Stop if we found a user or login entry.  */
      if ((last_entry_type (mode) == USER_PROCESS
	   || last_entry_type (mode) == LOGIN_PROCESS)
	  && (strncmp (line, last_entry_line (mode), UT_LINESIZE) == 0))
	break;
    }

  file_unlock (file_fd);
  memcpy (buffer, &last_entry, last_entry_size (mode));

  return 0;
}

static bool
internal_pututline (enum operation_mode_t mode, short int type,
		    const char *id, const char *line, const void *data)
{
  if (!maybe_setutent (mode))
    return false;

  const char *file_name = mode == UTMP_TIME64
    ? __libc_utmp_file_name
    : utmp_file_name_time32 (__libc_utmp_file_name);

  if (! file_writable)
    {
      /* We must make the file descriptor writable before going on.  */
      int new_fd = __open_nocancel
	(file_name, O_RDWR | O_LARGEFILE | O_CLOEXEC);
      if (new_fd == -1)
	return false;

      if (__dup2 (new_fd, file_fd) < 0)
	{
	  __close_nocancel_nostatus (new_fd);
	  return false;
	}
      __close_nocancel_nostatus (new_fd);
      file_writable = true;
    }

  /* To avoid DOS when accessing the utmp{x} database for update, the lock
     file should be accessible only by previleged users (BZ #24492).  For non
     default utmp{x} database the function tries to create the lock file.  */
  bool default_db = __libc_utmpname_mode == UTMPNAME_TIME64
		   || __libc_utmpname_mode == UTMPNAME_TIME32;
  int lockfd = lock_write_file (file_name, default_db);
  if (lockfd == -1)
    return false;

  /* Find the correct place to insert the data.  */
  const size_t utmp_size = last_entry_size (mode);
  bool ret = false;

  if (matches_last_entry (mode, type, id, line))
    /* Read back the entry under the write lock.  */
    file_offset -= utmp_size;
  bool found = internal_getut_nolock (mode, type, id, line);
  if (!found && errno != ESRCH)
    goto internal_pututline_out;

  off64_t write_offset;
  if (!found)
    {
      /* We append the next entry.  */
      write_offset = __lseek64 (file_fd, 0, SEEK_END);

      /* Round down to the next multiple of the entry size.  This
	 ensures any partially-written record is overwritten by the
	 new record.  */
      write_offset = write_offset / utmp_size * utmp_size;
    }
  else
    /* Overwrite last_entry.  */
    write_offset = file_offset - utmp_size;

  /* Write the new data.  */
  ssize_t nbytes;
  if (__lseek64 (file_fd, write_offset, SEEK_SET) < 0
      || (nbytes = __write_nocancel (file_fd, data, utmp_size)) < 0)
    /* There is no need to recover the file position because all reads use
       pread64, and any future write is preceded by another seek.  */
    goto internal_pututline_out;

  if (nbytes != utmp_size)
    {
      /* If we appended a new record this is only partially written.
	 Remove it.  */
      if (!found)
	__ftruncate64 (file_fd, write_offset);
      /* Assume that the write failure was due to missing disk
	 space.  */
      __set_errno (ENOSPC);
      goto internal_pututline_out;
    }

  file_offset = write_offset + utmp_size;
  ret = true;

internal_pututline_out:
  /* Release the write lock.  */
  unlock_write_file (file_name, lockfd, default_db);
  return ret;
}

static int
internal_updwtmp (enum operation_mode_t mode, const char *file,
		  const void *utmp)
{
  int result = -1;
  off64_t offset;
  int fd;

  /* Open WTMP file.  */
  fd = __open_nocancel (file, O_WRONLY | O_LARGEFILE);
  if (fd < 0)
    return -1;

  bool default_db = strcmp (file, _PATH_UTMP) == 0
		    || strcmp (file, _PATH_UTMP_BASE) == 0;
  int lockfd = lock_write_file (file, default_db);
  if (lockfd == -1)
    {
      __close_nocancel_nostatus (fd);
      return -1;
    }

  /* Remember original size of log file.  */
  offset = __lseek64 (fd, 0, SEEK_END);
  const size_t utmp_size = last_entry_size (mode);
  if (offset % utmp_size != 0)
    {
      offset -= offset % utmp_size;
      __ftruncate64 (fd, offset);

      if (__lseek64 (fd, 0, SEEK_END) < 0)
	goto unlock_return;
    }

  /* Write the entry.  If we can't write all the bytes, reset the file
     size back to the original size.  That way, no partial entries
     will remain.  */
  if (__write_nocancel (fd, utmp, utmp_size) != utmp_size)
    {
      __ftruncate64 (fd, offset);
      goto unlock_return;
    }

  result = 0;

unlock_return:
  /* Close WTMP file.  */
  unlock_write_file (file, lockfd, default_db);
  __close_nocancel_nostatus (fd);

  return result;
}

void
__libc_setutent (void)
{
  internal_setutent (UTMP_TIME64);
}

void
__libc_setutent32 (void)
{
  internal_setutent (UTMP_TIME32);
}

int
__libc_getutent_r (struct utmp *buffer, struct utmp **result)
{
  int r = internal_getutent_r (UTMP_TIME64, buffer);
  *result = r == 0 ? buffer : NULL;
  return r;
}

/* For implementing this function we don't use the getutent_r function
   because we can avoid the reposition on every new entry this way.  */
int
__libc_getutid_r (const struct utmp *id, struct utmp *buffer,
		  struct utmp **result)
{
  int r = internal_getutid_r (UTMP_TIME64, id->ut_type, id->ut_id,
			      id->ut_line, buffer);
  *result = r == 0? buffer : NULL;
  return r;
}

/* For implementing this function we don't use the getutent_r function
   because we can avoid the reposition on every new entry this way.  */
int
__libc_getutline_r (const struct utmp *line, struct utmp *buffer,
		    struct utmp **result)
{
  int r = internal_getutline_r (UTMP_TIME64, line->ut_line, buffer);
  *result = r == 0 ? buffer : NULL;
  return r;
}

struct utmp *
__libc_pututline (const struct utmp *line)
{
  return internal_pututline (UTMP_TIME64, line->ut_type, line->ut_id,
			     line->ut_line, line)
	 ? (struct utmp *) line : NULL;
}

void
__libc_endutent (void)
{
  if (file_fd >= 0)
    {
      __close_nocancel_nostatus (file_fd);
      file_fd = -1;
    }
}

int
__libc_updwtmp (const char *file, const struct utmp *utmp)
{
  return internal_updwtmp (UTMP_TIME64, file, utmp);
}

#if SHLIB_COMPAT(libc, GLIBC_2_0, UTMP_COMPAT_BASE)
int
__libc_getutent32_r (struct utmp32 *buffer, struct utmp32 **result)
{
  int r = internal_getutent_r (UTMP_TIME32, buffer);
  *result = r == 0 ? buffer : NULL;
  return r;
}

int
__libc_getutid32_r (const struct utmp32 *id, struct utmp32 *buffer,
		    struct utmp32 **result)
{
  int r = internal_getutid_r (UTMP_TIME32, id->ut_type, id->ut_id,
			      id->ut_line, buffer);
  *result = r == 0 ? buffer : NULL;
  return r;
}

int
__libc_getutline32_r (const struct utmp32 *line, struct utmp32 *buffer,
		      struct utmp32 **result)
{
  int r = internal_getutline_r (UTMP_TIME32, line->ut_line, buffer);
  *result = r == 0 ? buffer : NULL;
  return r;
}

struct utmp32 *
__libc_pututline32 (const struct utmp32 *line)
{
  return internal_pututline (UTMP_TIME32, line->ut_type, line->ut_id,
			     line->ut_line, line)
	 ? (struct utmp32 *) line : NULL;
}

int
__libc_updwtmp32 (const char *file, const struct utmp32 *utmp)
{
  return internal_updwtmp (UTMP_TIME32, file, utmp);
}
#endif
