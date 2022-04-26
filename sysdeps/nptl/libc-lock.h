/* libc-internal interface for mutex locks.  NPTL version.
   Copyright (C) 1996-2022 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If
   not, see <https://www.gnu.org/licenses/>.  */

#ifndef _LIBC_LOCK_H
#define _LIBC_LOCK_H 1

#include <pthread.h>
#define __need_NULL
#include <stddef.h>


/* Mutex type.  */
typedef struct { int lock; int cnt; void *owner; } __libc_lock_recursive_t;

/* Define a lock variable NAME with storage class CLASS.  The lock must be
   initialized with __libc_lock_init before it can be used (or define it
   with __libc_lock_define_initialized, below).  Use `extern' for CLASS to
   declare a lock defined in another module.  In public structure
   definitions you must use a pointer to the lock structure (i.e., NAME
   begins with a `*'), because its storage size will not be known outside
   of libc.  */
#define __libc_lock_define_recursive(CLASS,NAME) \
  CLASS __libc_lock_recursive_t NAME;

/* Define an initialized recursive lock variable NAME with storage
   class CLASS.  */
#define __libc_lock_define_initialized_recursive(CLASS, NAME) \
  CLASS __libc_lock_recursive_t NAME = _LIBC_LOCK_RECURSIVE_INITIALIZER;
#define _LIBC_LOCK_RECURSIVE_INITIALIZER \
  { LLL_LOCK_INITIALIZER, 0, NULL }

/* Initialize a recursive mutex.  */
#define __libc_lock_init_recursive(NAME) \
  ((void) ((NAME) = (__libc_lock_recursive_t) _LIBC_LOCK_RECURSIVE_INITIALIZER))

/* Finalize recursive named lock.  */
#define __libc_lock_fini_recursive(NAME) ((void) 0)

/* Lock the recursive named lock variable.  */
#define __libc_lock_lock_recursive(NAME) \
  do {									      \
    void *self = THREAD_SELF;						      \
    if ((NAME).owner != self)						      \
      {									      \
	lll_lock ((NAME).lock, LLL_PRIVATE);				      \
	(NAME).owner = self;						      \
      }									      \
    ++(NAME).cnt;							      \
  } while (0)

/* Try to lock the recursive named lock variable.  */
#define __libc_lock_trylock_recursive(NAME) \
  ({									      \
    int result = 0;							      \
    void *self = THREAD_SELF;						      \
    if ((NAME).owner != self)						      \
      {									      \
	if (lll_trylock ((NAME).lock) == 0)				      \
	  {								      \
	    (NAME).owner = self;					      \
	    (NAME).cnt = 1;						      \
	  }								      \
	else								      \
	  result = EBUSY;						      \
      }									      \
    else								      \
      ++(NAME).cnt;							      \
    result;								      \
  })

/* Unlock the recursive named lock variable.  */
/* We do no error checking here.  */
#define __libc_lock_unlock_recursive(NAME) \
  do {									      \
    if (--(NAME).cnt == 0)						      \
      {									      \
	(NAME).owner = NULL;						      \
	lll_unlock ((NAME).lock, LLL_PRIVATE);				      \
      }									      \
  } while (0)

/* Put the unwind buffer BUFFER on the per-thread callback stack.  The
   caller must fill BUFFER->__routine and BUFFER->__arg before calling
   this function.  */
void __libc_cleanup_push_defer (struct _pthread_cleanup_buffer *buffer);
libc_hidden_proto (__libc_cleanup_push_defer)
/* Remove BUFFER from the unwind callback stack.  The caller must invoke
   the callback if desired.  */
void __libc_cleanup_pop_restore (struct _pthread_cleanup_buffer *buffer);
libc_hidden_proto (__libc_cleanup_pop_restore)

/* Start critical region with cleanup.  */
#define __libc_cleanup_region_start(DOIT, FCT, ARG)			\
  {   bool _cleanup_start_doit;						\
  struct _pthread_cleanup_buffer _buffer;				\
  /* Non-addressable copy of FCT, so that we avoid indirect calls on	\
     the non-unwinding path.  */					\
  void (*_cleanup_routine) (void *) = (FCT);				\
  _buffer.__arg = (ARG);						\
  if (DOIT)								\
    {									\
      _cleanup_start_doit = true;					\
      _buffer.__routine = _cleanup_routine;				\
      __libc_cleanup_push_defer (&_buffer);				\
    }									\
  else									\
      _cleanup_start_doit = false;

/* End critical region with cleanup.  */
#define __libc_cleanup_region_end(DOIT)		\
  if (_cleanup_start_doit)			\
    __libc_cleanup_pop_restore (&_buffer);	\
  if (DOIT)					\
    _cleanup_routine (_buffer.__arg);		\
  } /* matches __libc_cleanup_region_start */


/* Hide the definitions which are only supposed to be used inside libc in
   a separate file.  This file is not present in the installation!  */
#include "libc-lockP.h"

#endif	/* libc-lock.h */
