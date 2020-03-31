/* Copyright (C) 2002-2020 Free Software Foundation, Inc.
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
   <https://www.gnu.org/licenses/>.  */

#include <setjmp.h>
#include <stdlib.h>
#include "pthreadP.h"
#include <futex-internal.h>


/* The next two functions are similar to pthread_setcanceltype() but
   more specialized for the use in the cancelable functions like write().
   They do not need to check parameters etc.  These functions must be
   AS-safe, with the exception of the actual cancellation, because they
   are called by wrappers around AS-safe functions like write().*/
int
attribute_hidden
__pthread_enable_asynccancel (void)
{
  struct pthread *self = THREAD_SELF;

  int oldval = THREAD_GETMEM (self, canceltype);
  THREAD_SETMEM (self, canceltype, PTHREAD_CANCEL_ASYNCHRONOUS);

  int ch = THREAD_GETMEM (self, cancelhandling);

  if (self->cancelstate == PTHREAD_CANCEL_ENABLE
      && (ch & (CANCELED_BITMASK | EXITING_BITMASK | TERMINATED_BITMASK))
	  == CANCELED_BITMASK)
    {
      THREAD_SETMEM (self, result, PTHREAD_CANCELED);
      __do_cancel ();
    }

  return oldval;
}

/* See the comment for __pthread_enable_asynccancel regarding
   the AS-safety of this function.  */
void
attribute_hidden
__pthread_disable_asynccancel (int oldtype)
{
  /* If asynchronous cancellation was enabled before we do not have
     anything to do.  */
  if (oldtype == PTHREAD_CANCEL_ASYNCHRONOUS)
    return;

  struct pthread *self = THREAD_SELF;
  THREAD_SETMEM (self, canceltype, PTHREAD_CANCEL_DEFERRED);

  /* We cannot return when we are being canceled.  Upon return the
     thread might be things which would have to be undone.  The
     following loop should loop until the cancellation signal is
     delivered.  */
  int ch = THREAD_GETMEM (self, cancelhandling);
  while (__glibc_unlikely ((ch & (CANCELING_BITMASK | CANCELED_BITMASK))
			    == CANCELING_BITMASK))
    {
      futex_wait_simple ((unsigned int *) &self->cancelhandling, ch,
			 FUTEX_PRIVATE);
      ch = THREAD_GETMEM (self, cancelhandling);
    }
}
