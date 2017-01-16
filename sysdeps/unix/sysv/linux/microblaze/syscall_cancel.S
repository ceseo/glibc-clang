/* Cancellable syscall wrapper.  Linux/microblaze version.
   Copyright (C) 2020 Free Software Foundation, Inc.
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
   <http://www.gnu.org/licenses/>.  */

#include <sysdep.h>

/* long int __syscall_cancel_arch (int *cancelhandling,
				   long int nr,
				   long int arg1,
				   long int arg2,
				   long int arg3,
				   long int arg4,
				   long int arg5,
				   long int arg6)  */

ENTRY (__syscall_cancel_arch)
#ifdef PIC
	addik	r1, r1, -32
	swi	r20, r1, 28
	swi	r15, r1, 0
	mfs	r20, rpc
	addik	r20, r20,_GLOBAL_OFFSET_TABLE_+8
#else
	addik	r1, r1, -28
	swi	r15, r1, 0
#endif

	.globl __syscall_cancel_arch_start
__syscall_cancel_arch_start:
	lwi	r3, r5, 0
	bneid	r3, 1f
	addk	r12, r6, r0

	addk	r5, r7, r0
	addk	r6, r8, r0
	addk	r7, r9, r0
	addk	r8, r10, r0
#ifdef PIC
	lwi	r9, r1, 60
	lwi	r10, r1, 64
#else
	lwi	r9, r1, 56
	lwi	r10, r1, 60
#endif
	brki	r14, 8

	.globl __syscall_cancel_arch_end
__syscall_cancel_arch_end:
	nop
	lwi	r15, r1, 0
#ifdef PIC
	lwi	r20, r1, 28
	rtsd	r15, 8
	addik	r1, r1, 32
#else
	rtsd	r15, 8
	addik	r1, r1, 28
#endif
1:
#ifdef PIC
	lwi	r3, r20, __do_cancel@GOT
	brald	r15, r3
#else
	brlid	r15, __do_cancel
#endif
	nop
END (__syscall_cancel_arch)
libc_hidden_def (__syscall_cancel_arch)
