/* Atomic operations.  PowerPC Common version.
   Copyright (C) 2003-2022 Free Software Foundation, Inc.
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

/*
 * Never include sysdeps/powerpc/atomic-machine.h directly.
 * Alway use include/atomic.h which will include either
 * sysdeps/powerpc/powerpc32/atomic-machine.h
 * or
 * sysdeps/powerpc/powerpc64/atomic-machine.h
 * as appropriate and which in turn include this file.
 */

/*
 * Powerpc does not have byte and halfword forms of load and reserve and
 * store conditional. So for powerpc we stub out the 8- and 16-bit forms.
 */
#define __arch_compare_and_exchange_bool_8_acq(mem, newval, oldval) \
  (abort (), 0)

#define __arch_compare_and_exchange_bool_16_acq(mem, newval, oldval) \
  (abort (), 0)

#define __ARCH_ACQ_INSTR	"isync"
#ifndef __ARCH_REL_INSTR
# define __ARCH_REL_INSTR	"sync"
#endif

#ifndef MUTEX_HINT_ACQ
# define MUTEX_HINT_ACQ
#endif
#ifndef MUTEX_HINT_REL
# define MUTEX_HINT_REL
#endif

#define atomic_full_barrier()	__asm ("sync" ::: "memory")

#define __arch_compare_and_exchange_val_32_acq(mem, newval, oldval)	      \
  ({									      \
      __typeof (*(mem)) __tmp;						      \
      __typeof (mem)  __memp = (mem);					      \
      __asm __volatile (						      \
		        "1:	lwarx	%0,0,%1" MUTEX_HINT_ACQ "\n"	      \
		        "	cmpw	%0,%2\n"			      \
		        "	bne	2f\n"				      \
		        "	stwcx.	%3,0,%1\n"			      \
		        "	bne-	1b\n"				      \
		        "2:	" __ARCH_ACQ_INSTR			      \
		        : "=&r" (__tmp)					      \
		        : "b" (__memp), "r" (oldval), "r" (newval)	      \
		        : "cr0", "memory");				      \
      __tmp;								      \
  })

#define __arch_compare_and_exchange_val_32_rel(mem, newval, oldval)	      \
  ({									      \
      __typeof (*(mem)) __tmp;						      \
      __typeof (mem)  __memp = (mem);					      \
      __asm __volatile (__ARCH_REL_INSTR "\n"				      \
		        "1:	lwarx	%0,0,%1" MUTEX_HINT_REL "\n"	      \
		        "	cmpw	%0,%2\n"			      \
		        "	bne	2f\n"				      \
		        "	stwcx.	%3,0,%1\n"			      \
		        "	bne-	1b\n"				      \
		        "2:	"					      \
		        : "=&r" (__tmp)					      \
		        : "b" (__memp), "r" (oldval), "r" (newval)	      \
		        : "cr0", "memory");				      \
      __tmp;								      \
  })

#define atomic_compare_and_exchange_val_acq(mem, newval, oldval) \
  ({									      \
    __typeof (*(mem)) __result;						      \
    if (sizeof (*mem) == 4)						      \
      __result = __arch_compare_and_exchange_val_32_acq(mem, newval, oldval); \
    else if (sizeof (*mem) == 8)					      \
      __result = __arch_compare_and_exchange_val_64_acq(mem, newval, oldval); \
    else 								      \
       abort ();							      \
    __result;								      \
  })

#define atomic_compare_and_exchange_val_rel(mem, newval, oldval) \
  ({									      \
    __typeof (*(mem)) __result;						      \
    if (sizeof (*mem) == 4)						      \
      __result = __arch_compare_and_exchange_val_32_rel(mem, newval, oldval); \
    else if (sizeof (*mem) == 8)					      \
      __result = __arch_compare_and_exchange_val_64_rel(mem, newval, oldval); \
    else 								      \
       abort ();							      \
    __result;								      \
  })
