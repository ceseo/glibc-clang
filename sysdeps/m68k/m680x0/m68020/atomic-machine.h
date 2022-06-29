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

#define __HAVE_64B_ATOMICS 0

/* XXX Is this actually correct?  */
#define ATOMIC_EXCHANGE_USES_CAS 1

#define __arch_compare_and_exchange_val_8_acq(mem, newval, oldval) \
  ({ __typeof (*(mem)) __ret;						      \
     __asm __volatile ("cas%.b %0,%2,%1"				      \
		       : "=d" (__ret), "+m" (*(mem))			      \
		       : "d" (newval), "0" (oldval));			      \
     __ret; })

#define __arch_compare_and_exchange_val_16_acq(mem, newval, oldval) \
  ({ __typeof (*(mem)) __ret;						      \
     __asm __volatile ("cas%.w %0,%2,%1"				      \
		       : "=d" (__ret), "+m" (*(mem))			      \
		       : "d" (newval), "0" (oldval));			      \
     __ret; })

#define __arch_compare_and_exchange_val_32_acq(mem, newval, oldval) \
  ({ __typeof (*(mem)) __ret;						      \
     __asm __volatile ("cas%.l %0,%2,%1"				      \
		       : "=d" (__ret), "+m" (*(mem))			      \
		       : "d" (newval), "0" (oldval));			      \
     __ret; })

# define __arch_compare_and_exchange_val_64_acq(mem, newval, oldval) \
  ({ __typeof (*(mem)) __ret;						      \
     __typeof (mem) __memp = (mem);					      \
     __asm __volatile ("cas2%.l %0:%R0,%1:%R1,(%2):(%3)"		      \
		       : "=d" (__ret)					      \
		       : "d" ((__typeof (*(mem))) (newval)), "r" (__memp),    \
			 "r" ((char *) __memp + 4), "0" (oldval)	      \
		       : "memory");					      \
     __ret; })

#define atomic_exchange_acq(mem, newvalue) \
  ({ __typeof (*(mem)) __result = *(mem);				      \
     if (sizeof (*(mem)) == 1)						      \
       __asm __volatile ("1: cas%.b %0,%2,%1;"				      \
			 "   jbne 1b"					      \
			 : "=d" (__result), "+m" (*(mem))		      \
			 : "d" (newvalue), "0" (__result));		      \
     else if (sizeof (*(mem)) == 2)					      \
       __asm __volatile ("1: cas%.w %0,%2,%1;"				      \
			 "   jbne 1b"					      \
			 : "=d" (__result), "+m" (*(mem))		      \
			 : "d" (newvalue), "0" (__result));		      \
     else if (sizeof (*(mem)) == 4)					      \
       __asm __volatile ("1: cas%.l %0,%2,%1;"				      \
			 "   jbne 1b"					      \
			 : "=d" (__result), "+m" (*(mem))		      \
			 : "d" (newvalue), "0" (__result));		      \
     else								      \
       {								      \
	 __typeof (mem) __memp = (mem);					      \
	 __asm __volatile ("1: cas2%.l %0:%R0,%1:%R1,(%2):(%3);"	      \
			   "   jbne 1b"					      \
			   : "=d" (__result)				      \
			   : "d" ((__typeof (*(mem))) (newvalue)),	      \
			     "r" (__memp), "r" ((char *) __memp + 4),	      \
			     "0" (__result)				      \
			   : "memory");					      \
       }								      \
     __result; })
