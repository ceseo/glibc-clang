/* Implementation of the getrandom system call.
   Copyright (C) 2016-2022 Free Software Foundation, Inc.
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

#include <assert.h>
#include <libc-lock.h>
#include <sys/random.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <errno.h>
#include <unistd.h>
#include <sysdep-cancel.h>
#include <ldsodefs.h>
#include <tls-internal.h>

#ifdef __NR_vgetrandom_alloc

static struct
{
  __libc_lock_define (, lock);
  struct grnd_allocator_map
  {
    void *p;
    unsigned int num;
    unsigned int size_per_each;
    size_t len;
    void **states;
  } *maps;
  unsigned int nmaps;
} grnd_allocator =
{
  .lock = LLL_LOCK_INITIALIZER
};

static void *
vgetrandom_alloc (unsigned int *num, unsigned int *size_per_each)
{
  *size_per_each = 0;  /* Must be zero on input.  */
  long int r = INTERNAL_SYSCALL_CALL (vgetrandom_alloc, num, size_per_each,
				      0 /* reserved @addr */,
				      0 /* reserved @flags */);
  return INTERNAL_SYSCALL_ERROR_P (r) ? MAP_FAILED : (void *) r;
}

static struct grnd_allocator_map *
vgetrandom_new_map (unsigned int *idx, void * rnd_block, unsigned int num,
		    unsigned int size_per_each)
{
  size_t new_nmapslen = (grnd_allocator.nmaps + 1)
			* sizeof (struct grnd_allocator_map);
  void *new_maps = realloc (grnd_allocator.maps, new_nmapslen);
  if (new_maps == NULL)
    return NULL;

  void **new_states = malloc (num * sizeof (void *));
  if (new_states == NULL)
    return NULL;

  *idx = grnd_allocator.nmaps;
  grnd_allocator.maps = new_maps;

  struct grnd_allocator_map *map = &grnd_allocator.maps[grnd_allocator.nmaps++];
  map->p = rnd_block;
  map->len = map->num = num;
  map->size_per_each = size_per_each;
  map->states = new_states;

  return map;
}

static bool
vgetrandom_get_state (struct tls_internal_t *ti)
{
  struct grnd_allocator_map *map = NULL;
  struct grnd_allocator_map *free_map = NULL;
  unsigned int idx;
  unsigned int free_idx;

  __libc_lock_lock (grnd_allocator.lock);

  for (idx = 0; idx < grnd_allocator.nmaps; ++idx)
    {
      if (grnd_allocator.maps[idx].len > 0)
	{
	  map = &grnd_allocator.maps[idx];
	  goto out;
	}
      /* Also keep track of an available empty maps entry.  */
      else if (grnd_allocator.maps[idx].p == 0ULL)
	{
	  free_map = &grnd_allocator.maps[idx];
	  free_idx = idx;
	}
    }

  /* Allocate a new getrandom vDSO state vector and add it on the tracked
     map list.  The 1 for num is to allocate the minumum size of an page.  */
  unsigned int num = 1;
  unsigned int size_per_each;
  void *rnd_block = vgetrandom_alloc (&num, &size_per_each);
  if (rnd_block == MAP_FAILED)
    goto out;

  /* Only extends the maps list if there is no empty entry available.  */
  if (free_map == NULL)
    map = vgetrandom_new_map (&idx, rnd_block, num, size_per_each);
  else
    {
      map = free_map;
      assert (map->num == num);
      assert (map->size_per_each == size_per_each);
      map->len = num;
      idx = free_idx;
    }

  if (map != NULL)
    {
      for (size_t i = 0; i < num; ++i)
	{
	  map->states[i] = rnd_block;
	  rnd_block += size_per_each;
	}
    }
  else
    __munmap (rnd_block, num * size_per_each);

out:
  if (map != NULL)
    {
      ti->getrandom_buf = map->states[--map->len];
      ti->getrandom_idx = idx;
    }

  __libc_lock_unlock (grnd_allocator.lock);
  return map != NULL;
}

/* Return true if the syscall fallback should be issued in the case the vDSO
   is not present, or the the buffer is already in use due reentracy, or if
   any memory allocation fails.  */
static bool
__getrandom_internal (ssize_t *ret, void *buffer, size_t length,
		      unsigned int flags)
{
  if (GLRO(dl_vdso_getrandom) == NULL)
    return false;

  struct tls_internal_t *ti = __glibc_tls_internal ();
  if (ti->getrandom_buf == NULL && !vgetrandom_get_state (ti))
    return false;

  *ret = GLRO(dl_vdso_getrandom)(buffer, length, flags, ti->getrandom_buf);
  if (INTERNAL_SYSCALL_ERROR_P (*ret))
    {
      __set_errno (INTERNAL_SYSCALL_ERRNO (*ret));
      *ret = -1;
    }
  return true;
}
#endif

ssize_t
__getrandom_nocancel (void *buffer, size_t length, unsigned int flags)
{
#ifdef __NR_vgetrandom_alloc
  ssize_t r;
  if (__getrandom_internal (&r, buffer, length, flags))
    return r;
#endif

  return SYSCALL_CANCEL (getrandom, buffer, length, flags);
}

/* Write up to LENGTH bytes of randomness starting at BUFFER.
   Return the number of bytes written, or -1 on error.  */
ssize_t
__getrandom (void *buffer, size_t length, unsigned int flags)
{
#ifdef __NR_vgetrandom_alloc
  ssize_t r;
  if (__getrandom_internal (&r, buffer, length, flags))
    return r;
#endif

  return INTERNAL_SYSCALL_CALL (getrandom, buffer, length, flags);
}
libc_hidden_def (__getrandom)
weak_alias (__getrandom, getrandom)

void
__getrandom_vdso_release (void)
{
#ifdef __NR_vgetrandom_alloc
  struct tls_internal_t *ti = __glibc_tls_internal ();
  if (ti->getrandom_buf == NULL)
    return;

  __libc_lock_lock (grnd_allocator.lock);

  struct grnd_allocator_map *map = &grnd_allocator.maps[ti->getrandom_idx];
  map->states[map->len++] = ti->getrandom_buf;

  if (map->len == map->num)
    {
      __munmap (map->p, map->num * map->size_per_each);
      map->p = 0ULL;
      map->len = 0;
    }

  __libc_lock_unlock (grnd_allocator.lock);
#endif
}

void
__getrandom_fork_subprocess (void)
{
#ifdef __NR_vgetrandom_alloc
  grnd_allocator.lock = LLL_LOCK_INITIALIZER;
#endif
}

void
__libc_getrandom_free (void)
{
#ifdef __NR_vgetrandom_alloc
  for (unsigned int i = 0; i < grnd_allocator.nmaps; i++)
    free (grnd_allocator.maps[i].states);
  free (grnd_allocator.maps);
#endif
}
