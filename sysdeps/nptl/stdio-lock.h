/* Thread package specific definitions of stream lock type.  NPTL version.
   Copyright (C) 2000-2022 Free Software Foundation, Inc.
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

#ifndef _STDIO_LOCK_H
#define _STDIO_LOCK_H 1

#include <libc-lock.h>
#include <lowlevellock.h>


typedef struct
{
  int lock;
  int cnt;
  void *owner;
} _IO_lock_t;
#define _IO_lock_t_defined 1

#define _IO_lock_initializer { LLL_LOCK_INITIALIZER, 0, NULL }

static inline void
__io_lock_init (_IO_lock_t *l)
{
  *l = (_IO_lock_t) _IO_lock_initializer;
}

static inline void
__io_lock_fini (_IO_lock_t *l)
{
}

static inline void
__io_lock_lock (_IO_lock_t *l)
{
  void *self = THREAD_SELF;
  if (l->owner != self)
    {
      lll_lock (l->lock, LLL_PRIVATE);
      l->owner = self;
    }
  ++l->cnt;
}

static inline int
__io_lock_trylock (_IO_lock_t *l)
{
  void *self = THREAD_SELF;
  if (l->owner != self)
    {
      if (lll_trylock (l->lock) == 0)
	{
	  l->owner = self;
	  l->cnt = 1;
	}
      else
	return EBUSY;
    }
  else
    ++l->cnt;
  return 0;
}

static inline void
__io_lock_unlock (_IO_lock_t *l)
{
  if (--l->cnt == 0)
    {
      l->owner = NULL;
      lll_unlock (l->lock, LLL_PRIVATE);
    }
}

#define _IO_lock_init(__l)     __io_lock_init (&(__l))
#define _IO_lock_fini(__l)     __io_lock_fini (&(__l))
#define _IO_lock_lock(__l)     __io_lock_lock (&(__l))
#define _IO_lock_trylock(__l)  __io_lock_trylock (&(__l))
#define _IO_lock_unlock(__l)   __io_lock_unlock (&(__l))

#define _IO_cleanup_region_start(_fct, _fp) \
  __libc_cleanup_region_start (((_fp)->_flags & _IO_USER_LOCK) == 0, _fct, _fp)
#define _IO_cleanup_region_start_noarg(_fct) \
  __libc_cleanup_region_start (1, _fct, NULL)
#define _IO_cleanup_region_end(_doit) \
  __libc_cleanup_region_end (_doit)

#if defined _LIBC && IS_IN (libc)

# ifdef __EXCEPTIONS
#  define _IO_acquire_lock(_fp) \
  do {									      \
    FILE *_IO_acquire_lock_file						      \
	__attribute__((cleanup (_IO_acquire_lock_fct)))			      \
	= (_fp);							      \
    _IO_flockfile (_IO_acquire_lock_file);
# else
#  define _IO_acquire_lock(_fp) _IO_acquire_lock_needs_exceptions_enabled
# endif
# define _IO_release_lock(_fp) ; } while (0)

#endif

#endif /* stdio-lock.h */
