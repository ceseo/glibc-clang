/* Cancellable syscall wrapper.  Linux/i686 version.
   Copyright (C) 2021 Free Software Foundation, Inc.
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

/* long int [eax] __syscall_cancel_arch (int *cancelhandling [SP],
					 long int nr   [SP+4],
					 long int arg1 [SP+8],
					 long int arg2 [SP+12],
					 long int arg3 [SP+16],
					 long int arg4 [SP+20],
					 long int arg5 [SP+24],
					 long int arg6 [SP+28])  */

ENTRY (__syscall_cancel_arch)
	subl	$28, %esp
	cfi_def_cfa_offset (32)
	movl	%ebx, 12(%esp)
	movl	%esi, 16(%esp)
	movl	%edi, 20(%esp)
	movl	%ebp, 24(%esp)
	cfi_offset (ebp, -20)
	cfi_offset (edi, -16)
	cfi_offset (esi, -12)
	cfi_offset (ebx, -8)

	.global __syscall_cancel_arch_start
__syscall_cancel_arch_start:
	movl	32(%esp), %eax
	movl	(%eax), %eax
	testl	%eax, %eax
	jne	1f

	/* Issue a 6 argument syscall, the nr [%eax] being the syscall
	   number.  */
	movl	60(%esp), %ebp
	movl	36(%esp), %eax
	movl	44(%esp), %ecx
	movl	48(%esp), %edx
	movl	40(%esp), %ebx
	movl	52(%esp), %esi
	movl	56(%esp), %edi

	/* We can not use the vDSO helper for syscall (__kernel_vsyscall)
	   because the returned PC from kernel will point to the vDSO page
	   instead of the expected __syscall_cancel_arch_{start,end}
	   marks.  */
	int	$0x80

	.global __syscall_cancel_arch_end
__syscall_cancel_arch_end:

	movl	12(%esp), %ebx
	movl	16(%esp), %esi
	movl	20(%esp), %edi
	movl	24(%esp), %ebp
	addl	$28, %esp
	cfi_remember_state
	cfi_restore (ebx)
	cfi_restore (esi)
	cfi_restore (edi)
	cfi_restore (ebp)
	cfi_def_cfa_offset (4)
        ret

1:
	/* Although the __do_cancel do not return, we need to stack
	   being set correctly for unwind.  */
	movl	12(%esp), %ebx
	movl	16(%esp), %esi
	movl	20(%esp), %edi
	movl	24(%esp), %ebp
	addl	$28, %esp
	cfi_restore (ebx)
	cfi_restore (esi)
	cfi_restore (edi)
	cfi_restore (ebp)
	cfi_restore_state
	jmp	__do_cancel

END (__syscall_cancel_arch)
libc_hidden_def (__syscall_cancel_arch)
