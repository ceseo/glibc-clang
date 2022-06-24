/* Atomic operations.  X86 version.
   Copyright (C) 2018-2022 Free Software Foundation, Inc.
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

#ifndef _X86_ATOMIC_MACHINE_H
#define _X86_ATOMIC_MACHINE_H 1

#include <stdint.h>
#include <tls.h>			/* For tcbhead_t.  */
#include <libc-pointer-arith.h>		/* For cast_to_integer.  */

#define LOCK_PREFIX "lock;"

#define USE_ATOMIC_COMPILER_BUILTINS	1

#ifdef __x86_64__
# define __HAVE_64B_ATOMICS		1
# define SP_REG				"rsp"
# define SEG_REG			"fs"
# define BR_CONSTRAINT			"q"
# define IBR_CONSTRAINT			"iq"
#else
/* Since the Pentium, i386 CPUs have supported 64-bit atomics, but the
   i386 psABI supplement provides only 4-byte alignment for uint64_t
   inside structs, so it is currently not possible to use 64-bit
   atomics on this platform.  */
# define __HAVE_64B_ATOMICS		0
# define SP_REG				"esp"
# define SEG_REG			"gs"
# define BR_CONSTRAINT			"r"
# define IBR_CONSTRAINT			"ir"
#endif
#define ATOMIC_EXCHANGE_USES_CAS	0

#define atomic_compare_and_exchange_val_acq(mem, newval, oldval) \
  __sync_val_compare_and_swap (mem, oldval, newval)
#define atomic_compare_and_exchange_bool_acq(mem, newval, oldval) \
  (! __sync_bool_compare_and_swap (mem, oldval, newval))

#define __cmpxchg_op(lock, mem, newval, oldval)				      \
  ({ __typeof (*mem) __ret;						      \
     if (sizeof (*mem) == 1)						      \
       asm volatile (lock "cmpxchgb %2, %1"				      \
		     : "=a" (__ret), "+m" (*mem)			      \
		     : BR_CONSTRAINT (newval), "0" (oldval)	  	      \
		     : "memory");					      \
     else if (sizeof (*mem) == 2)					      \
       asm volatile (lock "cmpxchgw %2, %1"				      \
		     : "=a" (__ret), "+m" (*mem)			      \
		     : BR_CONSTRAINT (newval), "0" (oldval)	  	      \
		     : "memory");					      \
     else if (sizeof (*mem) == 4)					      \
       asm volatile (lock "cmpxchgl %2, %1"				      \
		     : "=a" (__ret), "+m" (*mem)			      \
		     : BR_CONSTRAINT (newval), "0" (oldval)	  	      \
		     : "memory");					      \
     else if (__HAVE_64B_ATOMICS)					      \
       asm volatile (lock "cmpxchgq %2, %1"				      \
                    : "=a" (__ret), "+m" (*mem)				      \
                    : "q" ((int64_t) cast_to_integer (newval)),		      \
                      "0" ((int64_t) cast_to_integer (oldval))		      \
                    : "memory");					      \
     else								      \
       __atomic_link_error ();						      \
     __ret; })

#define __arch_c_compare_and_exchange_val_8_acq(mem, newval, oldval)	      \
  ({ __typeof (*mem) __ret;						      \
     if (SINGLE_THREAD_P)						      \
       __ret = __cmpxchg_op ("", (mem), (newval), (oldval));		      \
     else								      \
       __ret = __cmpxchg_op (LOCK_PREFIX, (mem), (newval), (oldval));	      \
     __ret; })

#define __arch_c_compare_and_exchange_val_16_acq(mem, newval, oldval)	      \
  ({ __typeof (*mem) __ret;						      \
     if (SINGLE_THREAD_P)						      \
       __ret = __cmpxchg_op ("", (mem), (newval), (oldval));		      \
     else								      \
       __ret = __cmpxchg_op (LOCK_PREFIX, (mem), (newval), (oldval));	      \
     __ret; })

#define __arch_c_compare_and_exchange_val_32_acq(mem, newval, oldval)	      \
  ({ __typeof (*mem) __ret;						      \
     if (SINGLE_THREAD_P)						      \
       __ret = __cmpxchg_op ("", (mem), (newval), (oldval));		      \
     else								      \
       __ret = __cmpxchg_op (LOCK_PREFIX, (mem), (newval), (oldval));	      \
     __ret; })

#define __arch_c_compare_and_exchange_val_64_acq(mem, newval, oldval)	      \
  ({ __typeof (*mem) __ret;						      \
     if (SINGLE_THREAD_P)						      \
       __ret = __cmpxchg_op ("", (mem), (newval), (oldval));		      \
     else								      \
       __ret =__cmpxchg_op (LOCK_PREFIX, (mem), (newval), (oldval));	      \
     __ret; })


#define __xchg_op(lock, mem, arg, op)					      \
  ({ __typeof (*mem) __ret = (arg);					      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile (lock #op "b %b0, %1"				      \
			 : "=q" (__ret), "=m" (*mem)			      \
			 : "0" (arg), "m" (*mem)			      \
			 : "memory", "cc");				      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile (lock #op "w %w0, %1"				      \
			 : "=r" (__ret), "=m" (*mem)			      \
			 : "0" (arg), "m" (*mem)			      \
			 : "memory", "cc");				      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile (lock #op "l %0, %1"				      \
			 : "=r" (__ret), "=m" (*mem)			      \
			 : "0" (arg), "m" (*mem)			      \
			 : "memory", "cc");				      \
     else if (__HAVE_64B_ATOMICS)					      \
       __asm __volatile (lock #op "q %q0, %1"				      \
			 : "=r" (__ret), "=m" (*mem)			      \
			 : "0" ((int64_t) cast_to_integer (arg)),	      \
			   "m" (*mem)					      \
			 : "memory", "cc");				      \
     else								      \
       __atomic_link_error ();						      \
     __ret; })

#define __single_op(lock, mem, op)					      \
  ({									      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile (lock #op "b %b0"				      \
			 : "=m" (*mem)					      \
			 : "m" (*mem)					      \
			 : "memory", "cc");				      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile (lock #op "w %b0"				      \
			 : "=m" (*mem)					      \
			 : "m" (*mem)					      \
			 : "memory", "cc");				      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile (lock #op "l %b0"				      \
			 : "=m" (*mem)					      \
			 : "m" (*mem)					      \
			 : "memory", "cc");				      \
     else if (__HAVE_64B_ATOMICS)					      \
       __asm __volatile (lock #op "q %b0"				      \
			 : "=m" (*mem)					      \
			 : "m" (*mem)					      \
			 : "memory", "cc");				      \
     else								      \
       __atomic_link_error ();						      \
  })

/* Note that we need no lock prefix.  */
#define atomic_exchange_acq(mem, newvalue)				      \
  __xchg_op ("", (mem), (newvalue), xchg)

#define atomic_add(mem, value) \
  __xchg_op (LOCK_PREFIX, (mem), (value), add);				      \

#define catomic_add(mem, value)						      \
  ({									      \
    if (SINGLE_THREAD_P)						      \
      __xchg_op ("", (mem), (value), add);				      \
   else									      \
     atomic_add (mem, value);						      \
  })

#define atomic_increment_and_test(mem) \
  ({ unsigned char __result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile (LOCK_PREFIX "incb %b0; sete %b1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile (LOCK_PREFIX "incw %w0; sete %w1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile (LOCK_PREFIX "incl %0; sete %1"			      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else if (__HAVE_64B_ATOMICS)					      \
       __asm __volatile (LOCK_PREFIX "incq %q0; sete %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else								      \
       __atomic_link_error ();						      \
     __result; })


#define atomic_decrement(mem)						      \
  __single_op (LOCK_PREFIX, (mem), dec)

#define catomic_decrement(mem)						      \
  ({									      \
    if (SINGLE_THREAD_P)						      \
      __single_op ("", (mem), dec);					      \
   else									      \
     atomic_decrement (mem);						      \
  })


#define atomic_decrement_and_test(mem) \
  ({ unsigned char __result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile (LOCK_PREFIX "decb %b0; sete %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile (LOCK_PREFIX "decw %w0; sete %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile (LOCK_PREFIX "decl %0; sete %1"			      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else								      \
       __asm __volatile (LOCK_PREFIX "decq %q0; sete %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     __result; })


#define atomic_bit_set(mem, bit) \
  do {									      \
    if (sizeof (*mem) == 1)						      \
      __asm __volatile (LOCK_PREFIX "orb %b2, %0"			      \
			: "=m" (*mem)					      \
			: "m" (*mem), IBR_CONSTRAINT (1L << (bit)));	      \
    else if (sizeof (*mem) == 2)					      \
      __asm __volatile (LOCK_PREFIX "orw %w2, %0"			      \
			: "=m" (*mem)					      \
			: "m" (*mem), "ir" (1L << (bit)));		      \
    else if (sizeof (*mem) == 4)					      \
      __asm __volatile (LOCK_PREFIX "orl %2, %0"			      \
			: "=m" (*mem)					      \
			: "m" (*mem), "ir" (1L << (bit)));		      \
    else if (__builtin_constant_p (bit) && (bit) < 32)			      \
      __asm __volatile (LOCK_PREFIX "orq %2, %0"			      \
			: "=m" (*mem)					      \
			: "m" (*mem), "i" (1L << (bit)));		      \
    else if (__HAVE_64B_ATOMICS)					      \
      __asm __volatile (LOCK_PREFIX "orq %q2, %0"			      \
			: "=m" (*mem)					      \
			: "m" (*mem), "r" (1UL << (bit)));		      \
    else								      \
      __atomic_link_error ();						      \
  } while (0)


#define atomic_bit_test_set(mem, bit) \
  ({ unsigned char __result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile (LOCK_PREFIX "btsb %3, %1; setc %0"		      \
			 : "=q" (__result), "=m" (*mem)			      \
			 : "m" (*mem), IBR_CONSTRAINT (bit));		      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile (LOCK_PREFIX "btsw %3, %1; setc %0"		      \
			 : "=q" (__result), "=m" (*mem)			      \
			 : "m" (*mem), "ir" (bit));			      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile (LOCK_PREFIX "btsl %3, %1; setc %0"		      \
			 : "=q" (__result), "=m" (*mem)			      \
			 : "m" (*mem), "ir" (bit));			      \
     else if (__HAVE_64B_ATOMICS)					      \
       __asm __volatile (LOCK_PREFIX "btsq %3, %1; setc %0"		      \
			 : "=q" (__result), "=m" (*mem)			      \
			 : "m" (*mem), "ir" (bit));			      \
     else							      	      \
       __atomic_link_error ();						      \
     __result; })


#define atomic_and(mem, mask)						      \
  __xchg_op (LOCK_PREFIX, (mem), (mask), and)

#define atomic_or(mem, mask)						      \
  __xchg_op (LOCK_PREFIX, (mem), (mask), or)

#define catomic_or(mem, mask) \
  ({									      \
    if (SINGLE_THREAD_P)						      \
      __xchg_op ("", (mem), (mask), or);				      \
   else									      \
      atomic_or (mem, mask);						      \
  })

/* We don't use mfence because it is supposedly slower due to having to
   provide stronger guarantees (e.g., regarding self-modifying code).  */
#define atomic_full_barrier() \
    __asm __volatile (LOCK_PREFIX "orl $0, (%%" SP_REG ")" ::: "memory")
#define atomic_read_barrier() __asm ("" ::: "memory")
#define atomic_write_barrier() __asm ("" ::: "memory")

#define atomic_spin_nop() __asm ("pause")

#endif /* atomic-machine.h */
