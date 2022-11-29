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
    uint64_t p;
    uint64_t num;
    uint64_t spe;
    size_t len;
    void **states;
  } *maps;
  unsigned int nmaps;
} grnd_allocator =
{
  .lock = LLL_LOCK_INITIALIZER
};

_Static_assert (sizeof (struct vgetrandom_alloc_args)
		== VGETRANDOM_ALLOC_ARGS_SIZE_VER0,
		"sizeof (struct vgetrandom_alloc_args) != "
		"VGETRANDOM_ALLOC_ARGS_SIZE_VER0");

static bool
vgetrandom_alloc (struct vgetrandom_alloc_args *alloc)
{
  long int r = INTERNAL_SYSCALL_CALL (vgetrandom_alloc, alloc,
				      VGETRANDOM_ALLOC_ARGS_SIZE_VER0);
  return INTERNAL_SYSCALL_ERROR_P (r) ? false : true;
}

static struct grnd_allocator_map *
vgetrandom_new_map (unsigned int *idx, uint64_t rnd_block, uint64_t num,
		    uint64_t spe)
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
  map->spe = spe;
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
     map list.  */
  struct vgetrandom_alloc_args alloc = {
      .flags = 0, .states = 0, .num = 1, .size_per_each = 0
  };
  if (!vgetrandom_alloc (&alloc))
    goto out;

  /* Only extends the maps list if there is no empty entry available.  */
  if (free_map == NULL)
    map = vgetrandom_new_map (&idx, alloc.states, alloc.num,
			      alloc.size_per_each);
  else
    {
      map = free_map;
      assert (map->num == alloc.num);
      assert (map->spe == alloc.size_per_each);
      map->len = alloc.num;
      idx = free_idx;
    }

  if (map != NULL)
    {
      char *rnd_block = (char *) alloc.states;
      for (size_t i = 0; i < alloc.num; ++i)
	{
	  map->states[i] = rnd_block;
	  rnd_block += alloc.size_per_each;
	}
    }
  else
    {
      alloc.flags = VGRA_DEALLOCATE;
      vgetrandom_alloc (&alloc);
    }

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
      vgetrandom_alloc (
	&(struct vgetrandom_alloc_args) { .flags = VGRA_DEALLOCATE,
					  .states = map->p,
					  .num = map->num,
					  .size_per_each = map->spe });
      map->p = 0ULL;
      map->len = 0;
    }

  __libc_lock_unlock (grnd_allocator.lock);
#endif
}

void
__getrandom_fork_subprocess (void)
{
  grnd_allocator.lock = LLL_LOCK_INITIALIZER;
}
