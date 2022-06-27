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
   License along with the GNU C Library.  If not, see
   <https://www.gnu.org/licenses/>.  */

#include <sysdep.h>

#define __HAVE_64B_ATOMICS 0

/* XXX Is this actually correct?  */
#define ATOMIC_EXCHANGE_USES_CAS 1


/* Microblaze does not have byte and halfword forms of load and reserve and
   store conditional. So for microblaze we stub out the 8- and 16-bit forms.  */
#define __arch_compare_and_exchange_bool_8_acq(mem, newval, oldval)            \
  (abort (), 0)

#define __arch_compare_and_exchange_bool_16_acq(mem, newval, oldval)           \
  (abort (), 0)

#define __arch_compare_and_exchange_val_32_acq(mem, newval, oldval)            \
  ({                                                                           \
      __typeof (*(mem)) __tmp;                                                 \
      __typeof (mem)  __memp = (mem);                                          \
      int test;                                                                \
      __asm __volatile (                                                       \
                "   addc    r0, r0, r0;"                                       \
                "1: lwx     %0, %3, r0;"                                       \
                "   addic   %1, r0, 0;"                                        \
                "   bnei    %1, 1b;"                                           \
                "   cmp     %1, %0, %4;"                                       \
                "   bnei    %1, 2f;"                                           \
                "   swx     %5, %3, r0;"                                       \
                "   addic   %1, r0, 0;"                                        \
                "   bnei    %1, 1b;"                                           \
                "2:"                                                           \
                    : "=&r" (__tmp),                                           \
                    "=&r" (test),                                              \
                    "=m" (*__memp)                                             \
                    : "r" (__memp),                                            \
                    "r" (oldval),                                              \
                    "r" (newval)                                               \
                    : "cc", "memory");                                         \
      __tmp;                                                                   \
  })

#define __arch_compare_and_exchange_val_64_acq(mem, newval, oldval)            \
  (abort (), (__typeof (*mem)) 0)

#define atomic_compare_and_exchange_val_acq(mem, newval, oldval)               \
  ({                                                                           \
    __typeof (*(mem)) __result;                                                \
    if (sizeof (*mem) == 4)                                                    \
      __result = __arch_compare_and_exchange_val_32_acq (mem, newval, oldval); \
    else if (sizeof (*mem) == 8)                                               \
      __result = __arch_compare_and_exchange_val_64_acq (mem, newval, oldval); \
    else                                                                       \
       abort ();                                                               \
    __result;                                                                  \
  })

#define atomic_compare_and_exchange_val_rel(mem, newval, oldval)               \
  ({                                                                           \
    __typeof (*(mem)) __result;                                                \
    if (sizeof (*mem) == 4)                                                    \
      __result = __arch_compare_and_exchange_val_32_acq (mem, newval, oldval); \
    else if (sizeof (*mem) == 8)                                               \
      __result = __arch_compare_and_exchange_val_64_acq (mem, newval, oldval); \
    else                                                                       \
       abort ();                                                               \
    __result;                                                                  \
  })

#define __arch_atomic_exchange_32_acq(mem, value)                              \
  ({                                                                           \
      __typeof (*(mem)) __tmp;                                                 \
      __typeof (mem)  __memp = (mem);                                          \
      int test;                                                                \
      __asm __volatile (                                                       \
                "   addc    r0, r0, r0;"                                       \
                "1: lwx     %0, %4, r0;"                                       \
                "   addic   %1, r0, 0;"                                        \
                "   bnei    %1, 1b;"                                           \
                "   swx     %3, %4, r0;"                                       \
                "   addic   %1, r0, 0;"                                        \
                "   bnei    %1, 1b;"                                           \
                    : "=&r" (__tmp),                                           \
                    "=&r" (test),                                              \
                    "=m" (*__memp)                                             \
                    : "r" (value),                                             \
                    "r" (__memp)                                               \
                    : "cc", "memory");                                         \
      __tmp;                                                                   \
  })

#define __arch_atomic_exchange_64_acq(mem, newval)                             \
  (abort (), (__typeof (*mem)) 0)

#define atomic_exchange_acq(mem, value)                                        \
  ({                                                                           \
    __typeof (*(mem)) __result;                                                \
    if (sizeof (*mem) == 4)                                                    \
      __result = __arch_atomic_exchange_32_acq (mem, value);                   \
    else if (sizeof (*mem) == 8)                                               \
      __result = __arch_atomic_exchange_64_acq (mem, value);                   \
    else                                                                       \
       abort ();                                                               \
    __result;                                                                  \
  })

#define atomic_exchange_rel(mem, value)                                        \
  ({                                                                           \
    __typeof (*(mem)) __result;                                                \
    if (sizeof (*mem) == 4)                                                    \
      __result = __arch_atomic_exchange_32_acq (mem, value);                   \
    else if (sizeof (*mem) == 8)                                               \
      __result = __arch_atomic_exchange_64_acq (mem, value);                   \
    else                                                                       \
       abort ();                                                               \
    __result;                                                                  \
  })

#define __arch_atomic_exchange_and_add_32(mem, value)                          \
  ({                                                                           \
    __typeof (*(mem)) __tmp;                                                   \
      __typeof (mem)  __memp = (mem);                                          \
    int test;                                                                  \
    __asm __volatile (                                                         \
                "   addc    r0, r0, r0;"                                       \
                "1: lwx     %0, %4, r0;"                                       \
                "   addic   %1, r0, 0;"                                        \
                "   bnei    %1, 1b;"                                           \
                "   add     %1, %3, %0;"                                       \
                "   swx     %1, %4, r0;"                                       \
                "   addic   %1, r0, 0;"                                        \
                "   bnei    %1, 1b;"                                           \
                    : "=&r" (__tmp),                                           \
                    "=&r" (test),                                              \
                    "=m" (*__memp)                                             \
                    : "r" (value),                                             \
                    "r" (__memp)                                               \
                    : "cc", "memory");                                         \
    __tmp;                                                                     \
  })

#define __arch_atomic_exchange_and_add_64(mem, value)                          \
  (abort (), (__typeof (*mem)) 0)

#define atomic_exchange_and_add(mem, value)                                    \
  ({                                                                           \
    __typeof (*(mem)) __result;                                                \
    if (sizeof (*mem) == 4)                                                    \
      __result = __arch_atomic_exchange_and_add_32 (mem, value);               \
    else if (sizeof (*mem) == 8)                                               \
      __result = __arch_atomic_exchange_and_add_64 (mem, value);               \
    else                                                                       \
       abort ();                                                               \
    __result;                                                                  \
  })
