/* vDSO common definition for Linux.
   Copyright (C) 2015-2020 Free Software Foundation, Inc.
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

#ifndef SYSDEP_VDSO_LINUX_H
# define SYSDEP_VDSO_LINUX_H

#include <ldsodefs.h>
#include <sysdep.h>

#ifndef INTERNAL_VSYSCALL_CALL
# define INTERNAL_VSYSCALL_CALL(funcptr, nr, args...)		      	      \
     funcptr (args)
#endif

#define INLINE_VSYSCALL(name, nr, args...)				      \
  ({									      \
    __label__ out;							      \
    __label__ iserr;							      \
    long int sc_ret;							      \
									      \
    __typeof (GLRO(dl_vdso_##name)) vdsop = GLRO(dl_vdso_##name);	      \
    if (vdsop != NULL)							      \
      {									      \
	sc_ret = INTERNAL_VSYSCALL_CALL (vdsop, nr, ##args);	      	      \
	if (!internal_syscall_error (sc_ret))			      	      \
	  goto out;							      \
	if (sc_ret != -ENOSYS)		      	      			      \
	  goto iserr;							      \
      }									      \
									      \
    sc_ret = internal_syscall (__NR_##name, ##args);		      	      \
    if (internal_syscall_error (sc_ret))			      	      \
      {									      \
      iserr:								      \
	syscall_error_ret (-sc_ret);					      \
      }									      \
  out:									      \
    sc_ret;								      \
  })

#endif /* SYSDEP_VDSO_LINUX_H  */
