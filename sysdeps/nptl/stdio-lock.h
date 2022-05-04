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
#include <lowlevellock-futex.h>

/* Linux allows optimizng the locks by embedding the owned TID on lower 30
   bits (the maximum tid number is 2^30, FUTEX_TID_MASK) in the lock work,
   with bit 31 used for congestion signal and bit 32 for single-thread
   optimization.  The embedded tid allows to implement recursive locks (as
   required by stdio locking functions).

   The single-thread optimization is only possible for streams linked into
   _IO_list_all since it requires disablng it once the program goes
   multithread.  For streams not n the global list, use either
   _IO_lock_initializer_not_chained or _IO_lock_init_not_chained.

   On internal calls to lock/unlock the stream, use _IO_acquire_lock and
   _IO_release_lock should since they implement the single-thread optimization
   if the lock allows it and avoid updating the internal counter.  It also
   unlocks on stack unwinding when exceptions happens.

   The _IO_flock_lock, _IO_ftrylock_lock, and _IO_funlock_lock are used to
   implement flockfile, ftrylockfile, and funlockfile since they update and
   lock counter and only issue an unlock if the counter reaches 0.
*/

typedef struct
{
  int lock;
  int cnt;
} _IO_lock_t;

#define _IO_lock_t_defined 1

#define _IO_lock_initializer { -1, 0 }
#define _IO_lock_initializer_not_chained { 0, 0 }

#define _IO_lock_maybe_waiters 0x40000000

static inline void
futex_lock_init (_IO_lock_t *l)
{
  *l = (_IO_lock_t) { SINGLE_THREAD_P ? -1 : 0, 0 };
}

static inline void
futex_lock_init_not_chained (_IO_lock_t *l)
{
  *l = (_IO_lock_t) { 0, 0 };
}

static inline void
futex_lock_enable_lock (_IO_lock_t *l)
{
  if (l->lock < 0)
    l->lock = 0;
}

static inline int
futex_lock_trylock (_IO_lock_t *l)
{
  int owner = atomic_load_relaxed (&l->lock);
  pid_t tid = THREAD_SELF->tid;

  /* Same thread trying to acquire the lock.  */
  if ((owner & ~_IO_lock_maybe_waiters) == tid)
    return 0;

  /* Disable single-thread optimization if it is enabled.  */
  if (owner < 0)
    {
      l->lock = 0;
      return 0;
    }

  return !atomic_compare_exchange_weak_acquire (&l->lock, &owner, tid)
    ? -1 : 0;
}

static inline int
futex_lock_lock (_IO_lock_t *l)
{
  int owner = atomic_load_relaxed (&l->lock);
  pid_t tid = THREAD_SELF->tid;

  /* Same thread trying to reacquire the lock.  */
  if ((owner & ~_IO_lock_maybe_waiters) == tid)
    return 0;

  /* Try to acquire the lock.  */
  owner = 0;
  if (atomic_compare_exchange_weak_acquire (&l->lock, &owner, tid))
    return 1;

  int newowner = tid | _IO_lock_maybe_waiters;
  do
    {
      /* Wait a unlock if any thread already owns the lock or if it can not
	 set the contention flag.  */
      if ((owner & _IO_lock_maybe_waiters)
	  || atomic_compare_exchange_weak_acquire (&l->lock, &owner,
					   owner | _IO_lock_maybe_waiters))
	lll_futex_wait (&l->lock, owner | _IO_lock_maybe_waiters, LLL_PRIVATE);

      owner = 0;
    }
  while (!atomic_compare_exchange_weak_acquire (&l->lock, &owner, newowner));

  return 1;
}

static inline void
futex_lock_unlock (_IO_lock_t *l)
{
  if (atomic_exchange_rel (&l->lock, 0) & _IO_lock_maybe_waiters)
    lll_futex_wake (&l->lock, 1, LLL_PRIVATE);
}

static inline void
futex_lock_flock (_IO_lock_t *l)
{
  futex_lock_lock (l);
  l->cnt = 1;
}

static inline int
futex_lock_ftrylock (_IO_lock_t *l)
{
  if (futex_lock_trylock (l) == 0)
    {
      if (l->cnt == INT_MAX)
	return -1;
      l->cnt++;
      return 0;
    }
  return -1;
}

static inline void
futex_lock_funlock (_IO_lock_t *l)
{
  if (l->cnt == 1)
    {
      l->cnt = 0;
      futex_lock_unlock (l);
    }
  else
    l->cnt--;
}

#define _IO_lock_init(__l)             futex_lock_init (&(__l))
#define _IO_lock_fini(__l)             /* Not required.  */
#define _IO_lock_lock(__l)             futex_lock_lock (&(__l))
#define _IO_lock_trylock(__l)          futex_lock_trylock (&(__l))
#define _IO_lock_unlock(__l)           futex_lock_unlock (&(__l))

#define _IO_lock_init_not_chained(__l) futex_lock_init_not_chained (&(__l))
#define _IO_lock_disable_st(__l)       futex_lock_enable_lock (&(__l))

#define _IO_flock_lock(__l)	       futex_lock_flock (&(__l))
#define _IO_ftrylock_lock(__l)	       futex_lock_ftrylock (&(__l))
#define _IO_funlock_lock(__l)	       futex_lock_funlock (&(__l))

#define _IO_cleanup_region_start(_fct, _fp) \
  __libc_cleanup_region_start (((_fp)->_flags & _IO_USER_LOCK) == 0, _fct, _fp)
#define _IO_cleanup_region_start_noarg(_fct) \
  __libc_cleanup_region_start (1, _fct, NULL)
#define _IO_cleanup_region_end(_doit) \
  __libc_cleanup_region_end (_doit)

#if IS_IN (libc)

# ifdef __EXCEPTIONS
#  define _IO_acquire_lock(_fp) \
  do {									 \
    FILE *_cfp __attribute__((cleanup (_IO_acquire_lock_fct))) =	 \
     (((_fp)->_flags & _IO_USER_LOCK) == 0)				 \
      && ((_fp)->_lock->lock >= 0 ? futex_lock_lock ((_fp)->_lock) : 0)  \
      ? (_fp) : NULL;
# else
#  define _IO_acquire_lock(_fp) _IO_acquire_lock_needs_exceptions_enabled
# endif
# define _IO_release_lock(_fp) ; } while (0)

#endif

#endif /* stdio-lock.h */
