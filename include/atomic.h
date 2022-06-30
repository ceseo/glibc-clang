/* Internal macros for atomic operations for GNU C Library.
   Copyright (C) 2002-2022 Free Software Foundation, Inc.
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

#ifndef _ATOMIC_H
#define _ATOMIC_H	1

/* This header defines three types of macros:

   - atomic arithmetic and logic operation on memory.  They all
     have the prefix "atomic_".

   - conditionally atomic operations of the same kinds.  These
     always behave identical but can be faster when atomicity
     is not really needed since only one thread has access to
     the memory location.  In that case the code is slower in
     the multi-thread case.  The interfaces have the prefix
     "catomic_".

   - support functions like barriers.  They also have the prefix
     "atomic_".

   Architectures must provide a few lowlevel macros (the compare
   and exchange definitions).  All others are optional.  They
   should only be provided if the architecture has specific
   support for the operation.

   As <atomic.h> macros are usually heavily nested and often use local
   variables to make sure side-effects are evaluated properly, use for
   macro local variables a per-macro unique prefix.  This file uses
   __atgN_ prefix where N is different in each macro.  */

#include <stdlib.h>

#include <atomic-machine.h>

/* Wrapper macros to call pre_NN_post (mem, ...) where NN is the
   bit width of *MEM.  The calling macro puts parens around MEM
   and following args.  */
#define __atomic_val_bysize(pre, post, mem, ...)			      \
  ({									      \
    __typeof ((__typeof (*(mem))) *(mem)) __atg1_result;		      \
    if (sizeof (*mem) == 1)						      \
      __atg1_result = pre##_8_##post (mem, __VA_ARGS__);		      \
    else if (sizeof (*mem) == 2)					      \
      __atg1_result = pre##_16_##post (mem, __VA_ARGS__);		      \
    else if (sizeof (*mem) == 4)					      \
      __atg1_result = pre##_32_##post (mem, __VA_ARGS__);		      \
    else if (sizeof (*mem) == 8)					      \
      __atg1_result = pre##_64_##post (mem, __VA_ARGS__);		      \
    else								      \
      abort ();								      \
    __atg1_result;							      \
  })
#define __atomic_bool_bysize(pre, post, mem, ...)			      \
  ({									      \
    int __atg2_result;							      \
    if (sizeof (*mem) == 1)						      \
      __atg2_result = pre##_8_##post (mem, __VA_ARGS__);		      \
    else if (sizeof (*mem) == 2)					      \
      __atg2_result = pre##_16_##post (mem, __VA_ARGS__);		      \
    else if (sizeof (*mem) == 4)					      \
      __atg2_result = pre##_32_##post (mem, __VA_ARGS__);		      \
    else if (sizeof (*mem) == 8)					      \
      __atg2_result = pre##_64_##post (mem, __VA_ARGS__);		      \
    else								      \
      abort ();								      \
    __atg2_result;							      \
  })


/* Atomically store NEWVAL in *MEM if *MEM is equal to OLDVAL.
   Return the old *MEM value.  */
#if !defined atomic_compare_and_exchange_val_acq \
    && defined __arch_compare_and_exchange_val_32_acq
# define atomic_compare_and_exchange_val_acq(mem, newval, oldval) \
  __atomic_val_bysize (__arch_compare_and_exchange_val,acq,		      \
		       mem, newval, oldval)
#endif


#ifndef catomic_compare_and_exchange_val_acq
# ifdef __arch_c_compare_and_exchange_val_32_acq
#  define catomic_compare_and_exchange_val_acq(mem, newval, oldval) \
  __atomic_val_bysize (__arch_c_compare_and_exchange_val,acq,		      \
		       mem, newval, oldval)
# else
#  define catomic_compare_and_exchange_val_acq(mem, newval, oldval) \
  atomic_compare_and_exchange_val_acq (mem, newval, oldval)
# endif
#endif


#ifndef catomic_compare_and_exchange_val_rel
# ifndef atomic_compare_and_exchange_val_rel
#  define catomic_compare_and_exchange_val_rel(mem, newval, oldval)	      \
  catomic_compare_and_exchange_val_acq (mem, newval, oldval)
# else
#  define catomic_compare_and_exchange_val_rel(mem, newval, oldval)	      \
  atomic_compare_and_exchange_val_rel (mem, newval, oldval)
# endif
#endif


#ifndef atomic_compare_and_exchange_val_rel
# define atomic_compare_and_exchange_val_rel(mem, newval, oldval)	      \
  atomic_compare_and_exchange_val_acq (mem, newval, oldval)
#endif


/* Atomically store NEWVAL in *MEM if *MEM is equal to OLDVAL.
   Return zero if *MEM was changed or non-zero if no exchange happened.  */
#ifndef atomic_compare_and_exchange_bool_acq
# ifdef __arch_compare_and_exchange_bool_32_acq
#  define atomic_compare_and_exchange_bool_acq(mem, newval, oldval) \
  __atomic_bool_bysize (__arch_compare_and_exchange_bool,acq,		      \
		        mem, newval, oldval)
# else
#  define atomic_compare_and_exchange_bool_acq(mem, newval, oldval) \
  ({ /* Cannot use __oldval here, because macros later in this file might     \
	call this macro with __oldval argument.	 */			      \
     __typeof (oldval) __atg3_old = (oldval);				      \
     atomic_compare_and_exchange_val_acq (mem, newval, __atg3_old)	      \
       != __atg3_old;							      \
  })
# endif
#endif


#ifndef catomic_compare_and_exchange_bool_acq
# ifdef __arch_c_compare_and_exchange_bool_32_acq
#  define catomic_compare_and_exchange_bool_acq(mem, newval, oldval) \
  __atomic_bool_bysize (__arch_c_compare_and_exchange_bool,acq,		      \
		        mem, newval, oldval)
# else
#  define catomic_compare_and_exchange_bool_acq(mem, newval, oldval) \
  ({ /* Cannot use __oldval here, because macros later in this file might     \
	call this macro with __oldval argument.	 */			      \
     __typeof (oldval) __atg4_old = (oldval);				      \
     catomic_compare_and_exchange_val_acq (mem, newval, __atg4_old)	      \
       != __atg4_old;							      \
  })
# endif
#endif


#ifndef atomic_full_barrier
# define atomic_full_barrier() __asm ("" ::: "memory")
#endif


#ifndef atomic_read_barrier
# define atomic_read_barrier() atomic_full_barrier ()
#endif


#ifndef atomic_write_barrier
# define atomic_write_barrier() atomic_full_barrier ()
#endif


/* This is equal to 1 iff the architecture supports 64b atomic operations.  */
#ifndef __HAVE_64B_ATOMICS
#error Unable to determine if 64-bit atomics are present.
#endif

/* The following functions are a subset of the atomic operations provided by
   C11.  Usually, a function named atomic_OP_MO(args) is equivalent to C11's
   atomic_OP_explicit(args, memory_order_MO); exceptions noted below.  */

/* We require 32b atomic operations; some archs also support 64b atomic
   operations.  */
void __atomic_link_error (void);
#if __HAVE_64B_ATOMICS == 1
# define __atomic_check_size(mem) \
   if ((sizeof (*mem) != 4) && (sizeof (*mem) != 8))			      \
     __atomic_link_error ();
#else
# define __atomic_check_size(mem) \
   if (sizeof (*mem) != 4)						      \
     __atomic_link_error ();
#endif
/* We additionally provide 8b and 16b atomic loads and stores; we do not yet
   need other atomic operations of such sizes, and restricting the support to
   loads and stores makes this easier for archs that do not have native
   support for atomic operations to less-than-word-sized data.  */
#if __HAVE_64B_ATOMICS == 1
# define __atomic_check_size_ls(mem) \
   if ((sizeof (*mem) != 1) && (sizeof (*mem) != 2) && (sizeof (*mem) != 4)   \
       && (sizeof (*mem) != 8))						      \
     __atomic_link_error ();
#else
# define __atomic_check_size_ls(mem) \
   if ((sizeof (*mem) != 1) && (sizeof (*mem) != 2) && sizeof (*mem) != 4)    \
     __atomic_link_error ();
#endif

#define atomic_thread_fence_acquire() \
  __atomic_thread_fence (__ATOMIC_ACQUIRE)
#define atomic_thread_fence_release() \
  __atomic_thread_fence (__ATOMIC_RELEASE)
#define atomic_thread_fence_seq_cst() \
  __atomic_thread_fence (__ATOMIC_SEQ_CST)

#define atomic_load_relaxed(mem) \
  ({ __atomic_check_size_ls((mem));					      \
     __atomic_load_n ((mem), __ATOMIC_RELAXED); })
#define atomic_load_acquire(mem) \
  ({ __atomic_check_size_ls((mem));					      \
     __atomic_load_n ((mem), __ATOMIC_ACQUIRE); })

#define atomic_store_relaxed(mem, val) \
  do {									      \
    __atomic_check_size_ls((mem));					      \
    __atomic_store_n ((mem), (val), __ATOMIC_RELAXED);			      \
  } while (0)
#define atomic_store_release(mem, val) \
  do {									      \
    __atomic_check_size_ls((mem));					      \
    __atomic_store_n ((mem), (val), __ATOMIC_RELEASE);			      \
  } while (0)

/* On failure, this CAS has memory_order_relaxed semantics.  */
#define atomic_compare_exchange_weak_relaxed(mem, expected, desired) \
  ({ __atomic_check_size((mem));					      \
  __atomic_compare_exchange_n ((mem), (expected), (desired), 1,		      \
    __ATOMIC_RELAXED, __ATOMIC_RELAXED); })
#define atomic_compare_exchange_weak_acquire(mem, expected, desired) \
  ({ __atomic_check_size((mem));					      \
  __atomic_compare_exchange_n ((mem), (expected), (desired), 1,		      \
    __ATOMIC_ACQUIRE, __ATOMIC_RELAXED); })
#define atomic_compare_exchange_weak_release(mem, expected, desired) \
  ({ __atomic_check_size((mem));					      \
  __atomic_compare_exchange_n ((mem), (expected), (desired), 1,		      \
    __ATOMIC_RELEASE, __ATOMIC_RELAXED); })

#define atomic_exchange_relaxed(mem, desired) \
  ({ __atomic_check_size((mem));					      \
  __atomic_exchange_n ((mem), (desired), __ATOMIC_RELAXED); })
#define atomic_exchange_acquire(mem, desired) \
  ({ __atomic_check_size((mem));					      \
  __atomic_exchange_n ((mem), (desired), __ATOMIC_ACQUIRE); })
#define atomic_exchange_release(mem, desired) \
  ({ __atomic_check_size((mem));					      \
  __atomic_exchange_n ((mem), (desired), __ATOMIC_RELEASE); })

#define atomic_fetch_add_relaxed(mem, operand) \
  ({ __atomic_check_size((mem));					      \
  __atomic_fetch_add ((mem), (operand), __ATOMIC_RELAXED); })
#define atomic_fetch_add_acquire(mem, operand) \
  ({ __atomic_check_size((mem));					      \
  __atomic_fetch_add ((mem), (operand), __ATOMIC_ACQUIRE); })
#define atomic_fetch_add_release(mem, operand) \
  ({ __atomic_check_size((mem));					      \
  __atomic_fetch_add ((mem), (operand), __ATOMIC_RELEASE); })
#define atomic_fetch_add_acq_rel(mem, operand) \
  ({ __atomic_check_size((mem));					      \
  __atomic_fetch_add ((mem), (operand), __ATOMIC_ACQ_REL); })

#define atomic_fetch_sub_relaxed(mem, operand) \
  ({ __atomic_check_size((mem));					      \
  __atomic_fetch_sub ((mem), (operand), __ATOMIC_RELAXED); })
#define atomic_fetch_sub_acquire(mem, operand) \
  ({ __atomic_check_size((mem));					      \
  __atomic_fetch_sub ((mem), (operand), __ATOMIC_ACQUIRE); })
#define atomic_fetch_sub_release(mem, operand) \
  ({ __atomic_check_size((mem));					      \
  __atomic_fetch_sub ((mem), (operand), __ATOMIC_RELEASE); })
#define atomic_fetch_sub_acq_rel(mem, operand) \
  ({ __atomic_check_size((mem));					      \
  __atomic_fetch_sub ((mem), (operand), __ATOMIC_ACQ_REL); })

#define atomic_fetch_and_relaxed(mem, operand) \
  ({ __atomic_check_size((mem));					      \
  __atomic_fetch_and ((mem), (operand), __ATOMIC_RELAXED); })
#define atomic_fetch_and_acquire(mem, operand) \
  ({ __atomic_check_size((mem));					      \
  __atomic_fetch_and ((mem), (operand), __ATOMIC_ACQUIRE); })
#define atomic_fetch_and_release(mem, operand) \
  ({ __atomic_check_size((mem));					      \
  __atomic_fetch_and ((mem), (operand), __ATOMIC_RELEASE); })

#define atomic_fetch_or_relaxed(mem, operand) \
  ({ __atomic_check_size((mem));					      \
  __atomic_fetch_or ((mem), (operand), __ATOMIC_RELAXED); })
#define atomic_fetch_or_acquire(mem, operand) \
  ({ __atomic_check_size((mem));					      \
  __atomic_fetch_or ((mem), (operand), __ATOMIC_ACQUIRE); })
#define atomic_fetch_or_release(mem, operand) \
  ({ __atomic_check_size((mem));					      \
  __atomic_fetch_or ((mem), (operand), __ATOMIC_RELEASE); })

#define atomic_fetch_xor_release(mem, operand) \
  ({ __atomic_check_size((mem));					      \
  __atomic_fetch_xor ((mem), (operand), __ATOMIC_RELEASE); })

/* This operation does not affect synchronization semantics but can be used
   in the body of a spin loop to potentially improve its efficiency.  */
#ifndef atomic_spin_nop
# define atomic_spin_nop() do { /* nothing */ } while (0)
#endif

#ifndef atomic_max
# define atomic_max(mem, value) \
  do {									      \
    __atomic_check_size_ls ((mem));					      \
    __typeof (*(mem)) __oldval;						      \
    __typeof (mem) __memp = (mem);					      \
    __typeof (*(mem)) __value = (value);				      \
    do {								      \
      __oldval = atomic_load_relaxed (__memp);				      \
      if (__oldval >= __value)						      \
	break;								      \
    } while (__glibc_likely						      \
	     (atomic_compare_exchange_weak_acquire (__memp, &__oldval,	      \
						    __value) == __oldval));   \
  } while (0)
#endif

/* ATOMIC_EXCHANGE_USES_CAS is non-zero if atomic_exchange operations
   are implemented based on a CAS loop; otherwise, this is zero and we assume
   that the atomic_exchange operations could provide better performance
   than a CAS loop.  */
#ifndef ATOMIC_EXCHANGE_USES_CAS
# error ATOMIC_EXCHANGE_USES_CAS has to be defined.
#endif

#endif	/* atomic.h */
