/* Copyright (C) 2003-2022 Free Software Foundation, Inc.
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

#include <ia64intrin.h>

#define __HAVE_64B_ATOMICS 1

/* XXX Is this actually correct?  */
#define ATOMIC_EXCHANGE_USES_CAS 0


#define __arch_compare_and_exchange_bool_8_acq(mem, newval, oldval) \
  (abort (), 0)

#define __arch_compare_and_exchange_bool_16_acq(mem, newval, oldval) \
  (abort (), 0)

#define __arch_compare_and_exchange_bool_32_acq(mem, newval, oldval) \
  (!__sync_bool_compare_and_swap ((mem), (int) (long) (oldval), \
				  (int) (long) (newval)))

#define __arch_compare_and_exchange_bool_64_acq(mem, newval, oldval) \
  (!__sync_bool_compare_and_swap ((mem), (long) (oldval), \
				  (long) (newval)))

#define __arch_compare_and_exchange_val_8_acq(mem, newval, oldval) \
  (abort (), (__typeof (*mem)) 0)

#define __arch_compare_and_exchange_val_16_acq(mem, newval, oldval) \
  (abort (), (__typeof (*mem)) 0)

#define __arch_compare_and_exchange_val_32_acq(mem, newval, oldval) \
  __sync_val_compare_and_swap ((mem), (int) (long) (oldval), \
			       (int) (long) (newval))

#define __arch_compare_and_exchange_val_64_acq(mem, newval, oldval) \
  __sync_val_compare_and_swap ((mem), (long) (oldval), (long) (newval))

/* Atomically store newval and return the old value.  */
#define atomic_exchange_acq(mem, value) \
  __sync_lock_test_and_set (mem, value)

#define atomic_full_barrier() __sync_synchronize ()
