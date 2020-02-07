/* Syscall definitions, Linux PowerPC generic version.
   Copyright (C) 2019-2020 Free Software Foundation, Inc.
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

#ifndef _LINUX_POWERPC_SYSDEP_H
#define _LINUX_POWERPC_SYSDEP_H 1

#include <sysdeps/unix/sysv/linux/sysdep.h>
#include <sysdeps/unix/powerpc/sysdep.h>
#include <tls.h>
/* Define __set_errno() for INLINE_SYSCALL macro below.  */
#include <errno.h>

/* For Linux we can use the system call table in the header file
       /usr/include/asm/unistd.h
   of the kernel.  But these symbols do not follow the SYS_* syntax
   so we have to redefine the `SYS_ify' macro here.  */
#undef SYS_ify
#define SYS_ify(syscall_name)  __NR_##syscall_name

#ifndef __ASSEMBLER__

/* Define a macro which expands inline into the wrapper code for a system
   call.  This use is for internal calls that do not need to handle errors
   normally. It will never touch errno.  This returns just what the kernel
   gave back in the non-error (CR0.SO cleared) case, otherwise (CR0.SO set)
   the negation of the return value in the kernel gets reverted.  */

static inline long int
internal_syscall0 (long int name)
{
  register long int r0  __asm__ ("r0") = name;
  register long int r3  __asm__ ("r3");
  asm volatile ("sc\n\t"
		"neg  9, %1\n\t"
		"isel %1, 9, %1, 3\n\t"
		: "+r" (r0), "=r" (r3)
		:
		: "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12",
		  "cr0", "memory");
  return r3;
}

static inline long int
internal_syscall1 (long int name, __syscall_arg_t arg1)
{
  register long int r0  __asm__ ("r0") = name;
  register long int r3  __asm__ ("r3") = arg1;
  asm volatile ("sc\n\t"
		"neg  9, %1\n\t"
		"isel %1, 9, %1, 3\n\t"
		: "+r" (r0), "+r" (r3)
		:
		: "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12",
		  "cr0", "memory");
  return r3;
}

static inline long int
internal_syscall2 (long int name, __syscall_arg_t arg1, __syscall_arg_t arg2)
{
  register long int r0  __asm__ ("r0") = name;
  register long int r3  __asm__ ("r3") = arg1;
  register long int r4  __asm__ ("r4") = arg2;
  asm volatile ("sc\n\t"
		"neg  9, %1\n\t"
		"isel %1, 9, %1, 3\n\t"
		: "+r" (r0), "+r" (r3), "+r" (r4)
		:
		: "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12",
		  "cr0", "memory");
  return r3;
}

static inline long int
internal_syscall3 (long int name, __syscall_arg_t arg1, __syscall_arg_t arg2,
		   __syscall_arg_t arg3)
{
  register long int r0  __asm__ ("r0") = name;
  register long int r3  __asm__ ("r3") = arg1;
  register long int r4  __asm__ ("r4") = arg2;
  register long int r5  __asm__ ("r5") = arg3;
  asm volatile ("sc\n\t"
		"neg  9, %1\n\t"
		"isel %1, 9, %1, 3\n\t"
		: "+r" (r0), "+r" (r3), "+r" (r4), "+r" (r5)
		:
		: "r6", "r7", "r8", "r9", "r10", "r11", "r12",
		  "cr0", "memory");
  return r3;
}

static inline long int
internal_syscall4 (long int name, __syscall_arg_t arg1, __syscall_arg_t arg2,
		   __syscall_arg_t arg3, __syscall_arg_t arg4)
{
  register long int r0  __asm__ ("r0") = name;
  register long int r3  __asm__ ("r3") = arg1;
  register long int r4  __asm__ ("r4") = arg2;
  register long int r5  __asm__ ("r5") = arg3;
  register long int r6  __asm__ ("r6") = arg4;
  asm volatile ("sc\n\t"
		"neg  9, %1\n\t"
		"isel %1, 9, %1, 3\n\t"
		: "+r" (r0), "+r" (r3), "+r" (r4), "+r" (r5), "+r" (r6)
		:
		: "r7", "r8", "r9", "r10", "r11", "r12",
		  "cr0", "memory");
  return r3;
}

static inline long int
internal_syscall5 (long int name, __syscall_arg_t arg1, __syscall_arg_t arg2,
		   __syscall_arg_t arg3, __syscall_arg_t arg4,
		   __syscall_arg_t arg5)
{
  register long int r0  __asm__ ("r0") = name;
  register long int r3  __asm__ ("r3") = arg1;
  register long int r4  __asm__ ("r4") = arg2;
  register long int r5  __asm__ ("r5") = arg3;
  register long int r6  __asm__ ("r6") = arg4;
  register long int r7  __asm__ ("r7") = arg5;
  asm volatile ("sc\n\t"
		"neg  9, %1\n\t"
		"isel %1, 9, %1, 3\n\t"
		: "+r" (r0), "+r" (r3), "+r" (r4), "+r" (r5), "+r" (r6),
		  "+r" (r7)
		:
		: "r8", "r9", "r10", "r11", "r12",
		  "cr0", "memory");
  return r3;
}

static inline long int
internal_syscall6 (long int name, __syscall_arg_t arg1, __syscall_arg_t arg2,
		   __syscall_arg_t arg3, __syscall_arg_t arg4,
		   __syscall_arg_t arg5, __syscall_arg_t arg6)
{
  register long int r0  __asm__ ("r0") = name;
  register long int r3  __asm__ ("r3") = arg1;
  register long int r4  __asm__ ("r4") = arg2;
  register long int r5  __asm__ ("r5") = arg3;
  register long int r6  __asm__ ("r6") = arg4;
  register long int r7  __asm__ ("r7") = arg5;
  register long int r8  __asm__ ("r8") = arg6;
  asm volatile ("sc\n\t"
		"neg  9, %1\n\t"
		"isel %1, 9, %1, 3\n\t"
		: "+r" (r0), "+r" (r3), "+r" (r4), "+r" (r5), "+r" (r6),
		  "+r" (r7), "+r" (r8)
		:
		: "r9", "r10", "r11", "r12",
		  "cr0", "memory");
  return r3;
}

/* Similar to internal_syscall, issues a vDSO call by branching to the
   defined symbol.  The powerpc vDSO implementation might issue the syscall
   if the vDSO can not handle the input, which requires handling the error
   case where CR0.SO is set.
   Different than internal_syscall, we need to use a macro instead of
   inline function due the function pointer.  */

#define __internal_vsyscall1(rettype, name) \
  internal_vsyscall0 (name, rettype)
#define __internal_vsyscall2(rettype, name, a1) \
  internal_vsyscall1 (name, rettype, a1)
#define __internal_vsyscall3(rettype, name, a1, a2) \
  internal_vsyscall2 (name, rettype, a1, a2)
#define __internal_vsyscall4(rettype, name, a1, a2, a3) \
  internal_vsyscall3 (name, rettype, a1, a2, a3)
#define __internal_vsyscall5(rettype, name, a1, a2, a3, a4) \
  internal_vsyscall4 (name, rettype, a1), a2, a3, a4
#define __internal_vsyscall6(rettype, name, a1, a2, a3, a4, a5) \
  internal_vsyscall5 (name, rettype, a1), a2, a3, a4, a5
#define __internal_vsyscall7(rettype, name, a1, a2, a3, a4, a5, a6) \
  internal_vsyscall6 (name, rettype, a1, a2, a3, a4, a5, a6)
#define __internal_vsyscall8(rettype, name, a1, a2, a3, a4, a5, a6, a7) \
  internal_vsyscall7 (name, rettype, a1, a2, a3, a4, a5, a6, a7)

#define __internal_vsyscall_nargs_x(a,b,c,d,e,f,g,h,n,o,...) n
#define __internal_vsyscall_nargs(...) \
  __internal_vsyscall_nargs_x (__VA_ARGS__,6,5,4,3,2,1,0,)
#define __internal_vsyscall_disp(rettype, b,...) \
  __syscall_concat (b,__internal_vsyscall_nargs(__VA_ARGS__))(__VA_ARGS__)

#define internal_vsyscall_type(...) \
  __internal_syscall_disp (__internal_vsyscall, __VA_ARGS__)
#define internal_vsyscall(...) \
  internal_vsyscall_type (long int, __VA_ARGS__)

#define internal_vsyscall0(name, type, dummy...)			\
  ({									\
    register long int r0  __asm__ ("r0") = (long int) (name);		\
    register long int r3  __asm__ ("r3");				\
    type rval;								\
    __asm__ __volatile__						\
      ("mtctr %0\n\t"							\
       "bctrl\n\t"							\
       "neg 9, %1\n\t"							\
       "isel %1, 9, %1, 3\n\t"						\
       : "+r" (r0), "=r" (r3)						\
       : 								\
       : "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12",	\
	 "lr", "ctr", "cr0", "memory");					\
    __asm__ __volatile__ ("" : "=r" (rval) : "r" (r3));			\
    rval;								\
  })

#define internal_vsyscall1(name, type, arg1)				\
  ({									\
    register long int r0  __asm__ ("r0") = (long int) (name);		\
    register long int r3  __asm__ ("r3") = (long int) (arg1);		\
    type rval;								\
    __asm__ __volatile__						\
      ("mtctr %0\n\t"							\
       "bctrl\n\t"							\
       "neg 9, %1\n\t"							\
       "isel %1, 9, %1, 3\n\t"						\
       : "+r" (r0), "+r" (r3)						\
       : 								\
       : "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12",	\
	 "lr", "ctr", "cr0", "memory");					\
    __asm__ __volatile__ ("" : "=r" (rval) : "r" (r3));			\
    rval;								\
  })

#define internal_vsyscall2(name, type, arg1, arg2)			\
  ({									\
    register long int r0  __asm__ ("r0") = (long int) (name);		\
    register long int r3  __asm__ ("r3") = (long int) (arg1);		\
    register long int r4  __asm__ ("r4") = (long int) (arg2);		\
    type rval;								\
    __asm__ __volatile__						\
      ("mtctr %0\n\t"							\
       "bctrl\n\t"							\
       "neg 9, %1\n\t"							\
       "isel %1, 9, %1, 3\n\t"						\
       : "+r" (r0), "+r" (r3), "+r" (r4)				\
       : 								\
       : "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12",		\
	 "lr", "ctr", "cr0", "memory");					\
    __asm__ __volatile__ ("" : "=r" (rval) : "r" (r3));			\
    rval;								\
  })

#define internal_vsyscall3(name, type, arg1, arg2, arg3)		\
  ({									\
    register long int r0  __asm__ ("r0") = (long int) (name);		\
    register long int r3  __asm__ ("r3") = (long int) (arg1);		\
    register long int r4  __asm__ ("r4") = (long int) (arg2);		\
    register long int r5  __asm__ ("r5") = (long int) (arg3);		\
    type rval;								\
    __asm__ __volatile__						\
      ("mtctr %0\n\t"							\
       "bctrl\n\t"							\
       "neg 9, %1\n\t"							\
       "isel %1, 9, %1, 3\n\t"						\
       : "+r" (r0), "+r" (r3), "+r" (r4), "+r" (r5)			\
       : 								\
       : "r6", "r7", "r8", "r9", "r10", "r11", "r12",			\
	 "lr", "ctr", "cr0", "memory");					\
    __asm__ __volatile__ ("" : "=r" (rval) : "r" (r3));			\
    rval;								\
  })

#define internal_vsyscall4(name, type, arg1, arg2, arg3, arg4)		\
  ({									\
    register long int r0  __asm__ ("r0") = (long int) (name);		\
    register long int r3  __asm__ ("r3") = (long int) (arg1);		\
    register long int r4  __asm__ ("r4") = (long int) (arg2);		\
    register long int r5  __asm__ ("r5") = (long int) (arg3);		\
    register long int r6  __asm__ ("r6") = (long int) (arg4);		\
    type rval;								\
    __asm__ __volatile__						\
      ("mtctr %0\n\t"							\
       "bctrl\n\t"							\
       "neg 9, %1\n\t"							\
       "isel %1, 9, %1, 3\n\t"						\
       : "+r" (r0), "+r" (r3), "+r" (r4), "+r" (r5), "+r" (r6)		\
       : 								\
       : "r7", "r8", "r9", "r10", "r11", "r12",				\
	 "lr", "ctr", "cr0", "memory");					\
    __asm__ __volatile__ ("" : "=r" (rval) : "r" (r3));			\
    rval;								\
  })

#define internal_vsyscall5(name, type, arg1, arg2, arg3, arg4, arg5)	\
  ({									\
    register long int r0  __asm__ ("r0") = (long int) (name);		\
    register long int r3  __asm__ ("r3") = (long int) (arg1);		\
    register long int r4  __asm__ ("r4") = (long int) (arg2);		\
    register long int r5  __asm__ ("r5") = (long int) (arg3);		\
    register long int r6  __asm__ ("r6") = (long int) (arg4);		\
    register long int r7  __asm__ ("r7") = (long int) (arg5);		\
    type rval;								\
    __asm__ __volatile__						\
      ("mtctr %0\n\t"							\
       "bctrl\n\t"							\
       "neg 9, %1\n\t"							\
       "isel %1, 9, %1, 3\n\t"						\
       : "+r" (r0), "+r" (r3), "+r" (r4), "+r" (r5), "+r" (r6),		\
	 "+r" (r7)							\
       : 								\
       : "r8", "r9", "r10", "r11", "r12",				\
	 "lr", "ctr", "cr0", "memory");					\
    __asm__ __volatile__ ("" : "=r" (rval) : "r" (r3));			\
    rval;								\
  })

#define internal_vsyscall6(name, type, arg1, arg2, arg3, arg4, arg5,	\
			   arg6)					\
  ({									\
    register long int r0  __asm__ ("r0") = (long int) (name);		\
    register long int r3  __asm__ ("r3") = (long int) (arg1);		\
    register long int r4  __asm__ ("r4") = (long int) (arg2);		\
    register long int r5  __asm__ ("r5") = (long int) (arg3);		\
    register long int r6  __asm__ ("r6") = (long int) (arg4);		\
    register long int r7  __asm__ ("r7") = (long int) (arg5);		\
    register long int r8  __asm__ ("r8") = (long int) (arg6);		\
    type rval;								\
    __asm__ __volatile__						\
      ("mtctr %0\n\t"							\
       "bctrl\n\t"							\
       "neg 9, %1\n\t"							\
       "isel %1, 9, %1, 3\n\t"						\
       : "+r" (r0), "+r" (r3), "+r" (r4), "+r" (r5), "+r" (r6),		\
	 "+r" (r7), "+r" (r8)						\
       : 								\
       : "r9", "r10", "r11", "r12",					\
	 "lr", "ctr", "cr0", "memory");					\
    __asm__ __volatile__ ("" : "=r" (rval) : "r" (r3));			\
    rval;								\
  })

#endif /* __ASSEMBLER__  */

/* Pointer mangling support.  */
#if IS_IN (rtld)
/* We cannot use the thread descriptor because in ld.so we use setjmp
   earlier than the descriptor is initialized.  */
#else
# ifdef __ASSEMBLER__
#  if defined(__PPC64__) || defined(__powerpc64__)
#   define LOAD  ld
#   define TPREG r13
#  else
#   define LOAD  lwz
#   define TPREG r2
#  endif
#  define PTR_MANGLE(reg, tmpreg) \
	LOAD	tmpreg,POINTER_GUARD(TPREG); \
	xor	reg,tmpreg,reg
#  define PTR_MANGLE2(reg, tmpreg) \
	xor	reg,tmpreg,reg
#  define PTR_MANGLE3(destreg, reg, tmpreg) \
	LOAD	tmpreg,POINTER_GUARD(TPREG); \
	xor	destreg,tmpreg,reg
#  define PTR_DEMANGLE(reg, tmpreg) PTR_MANGLE (reg, tmpreg)
#  define PTR_DEMANGLE2(reg, tmpreg) PTR_MANGLE2 (reg, tmpreg)
#  define PTR_DEMANGLE3(destreg, reg, tmpreg) PTR_MANGLE3 (destreg, reg, tmpreg)
# else
#  define PTR_MANGLE(var) \
  (var) = (__typeof (var)) ((uintptr_t) (var) ^ THREAD_GET_POINTER_GUARD ())
#  define PTR_DEMANGLE(var)	PTR_MANGLE (var)
# endif
#endif

/* List of system calls which are supported as vsyscalls.  */
#define VDSO_NAME  "LINUX_2.6.15"
#define VDSO_HASH  123718565

#if defined(__PPC64__) || defined(__powerpc64__)
#define HAVE_CLOCK_GETRES64_VSYSCALL	"__kernel_clock_getres"
#define HAVE_CLOCK_GETTIME64_VSYSCALL	"__kernel_clock_gettime"
#else
#define HAVE_CLOCK_GETRES_VSYSCALL	"__kernel_clock_getres"
#define HAVE_CLOCK_GETTIME_VSYSCALL	"__kernel_clock_gettime"
#endif
#define HAVE_GETCPU_VSYSCALL		"__kernel_getcpu"
#define HAVE_TIME_VSYSCALL		"__kernel_time"
#define HAVE_GETTIMEOFDAY_VSYSCALL      "__kernel_gettimeofday"
#define HAVE_GET_TBFREQ                 "__kernel_get_tbfreq"

#if defined(__PPC64__) || defined(__powerpc64__)
# define HAVE_SIGTRAMP_RT64		"__kernel_sigtramp_rt64"
#else
# define HAVE_SIGTRAMP_32		"__kernel_sigtramp32"
# define HAVE_SIGTRAMP_RT32		"__kernel_sigtramp_rt32"
#endif

#endif /* _LINUX_POWERPC_SYSDEP_H  */
