/* Copyright (C) 1997-2022 Free Software Foundation, Inc.
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

#include <array_length.h>
#include <atomic.h>
#include <stdlib.h>
#include <libc-internal.h>
#include <unwind-link.h>
#include <dlfcn/dlerror.h>
#include <ldsodefs.h>
#include <set-freeres-system.h>

#include "../nss/nsswitch.h"
#include "../libio/libioP.h"

/* Resource Freeing Hooks:

   Normally a process exits and the OS cleans up any allocated
   memory.  However, when tooling like mtrace or valgrind is monitoring
   the process we need to free all resources that are part of the
   process in order to provide the consistency required to track
   memory leaks.

   A single public API exists and is __libc_freeres(), and this is used
   by applications like valgrind to freee resouces.

   Each free routines must be explicit listed below, with the care to define
   weak functions for external symbol if applicable.   */

/* From libc.so.  */
extern void __libc_freemem (void) weak_function;
extern void __hdestroy (void) weak_function;
extern void __gconv_cache_freemem (void) weak_function;
extern void __gconv_conf_freemem (void) weak_function;
extern void __gconv_db_freemem (void) weak_function;
extern void __gconv_dl_freemem (void) weak_function;
extern void __intl_freemem (void) weak_function;
extern void __libio_freemem (void) weak_function;
extern void __libc_fstab_freemem (void) weak_function;
extern void __nscd_gr_map_freemem (void) weak_function;
extern void __nscd_hst_map_freemem (void) weak_function;
extern void __nscd_pw_map_freemem (void) weak_function;
extern void __nscd_serv_map_freemem (void) weak_function;
extern void __nscd_group_map_freemem (void) weak_function;
extern void __libc_regcomp_freemem (void) weak_function;
extern void __libc_atfork_freemem (void) weak_function;
extern void __libc_resolv_conf_freemem (void) weak_function;
extern void __libc_resolv_res_hconf_freemem (void) weak_function;
extern void __res_thread_freeres (void) weak_function;
extern void __libc_printf_freemem (void) weak_function;
extern void __libc_fmtmsg_freemem (void) weak_function;
extern void __libc_setenv_freemem (void) weak_function;
extern void __rpc_freemem (void) weak_function;
extern void __rpc_thread_destroy (void) weak_function;
extern void __libc_getaddrinfo_freemem (void) weak_function;
extern void __libc_tzset_freemem (void) weak_function;
extern void __libc_fgetgrent_freemem (void) weak_function;
extern void __libc_getnameinfo_freemem (void) weak_function;
extern void __libc_getnetgrent_freemem (void) weak_function;
extern void __libc_rcmd_freemem (void) weak_function;
extern void __libc_rexec_freemem (void) weak_function;
extern void __libc_localealias_freemem (void) weak_function;
extern void __libc_getutent_freemem (void) weak_function;
extern void __libc_getutid_freemem (void) weak_function;
extern void __libc_getutline_freemem (void) weak_function;
extern void __libc_mntent_freemem (void) weak_function;
extern void __libc_fgetpwent_freemem (void) weak_function;
extern void __libc_fgetspent_freemem (void) weak_function;
extern void __libc_reg_printf_freemem (void) weak_function;
extern void __libc_reg_type_freemem (void) weak_function;
extern void __libc_tzfile_freemem (void) weak_function;
/* From misc/efgcvt-template.c  */
extern void __libc_efgcvt_freemem (void) weak_function;
extern void __libc_qefgcvt_freemem (void) weak_function;
/* From nss/getXXbyYY.c  */
extern void __libc_getgrgid_freemem (void) weak_function;
extern void __libc_getgrnam_freemem (void) weak_function;
extern void __libc_getpwnam_freemem (void) weak_function;
extern void __libc_getpwuid_freemem (void) weak_function;
extern void __libc_getspnam_freemem (void) weak_function;
extern void __libc_getaliasbyname_freemem (void) weak_function;
extern void __libc_gethostbyaddr_freemem (void) weak_function;
extern void __libc_gethostbyname_freemem (void) weak_function;
extern void __libc_gethostbyname2_freemem (void) weak_function;
extern void __libc_getnetbyaddr_freemem (void) weak_function;
extern void __libc_getnetbyname_freemem (void) weak_function;
extern void __libc_getprotobynumber_freemem (void) weak_function;
extern void __libc_getprotobyname_freemem (void) weak_function;
extern void __libc_getrpcbyname_freemem (void) weak_function;
extern void __libc_getrpcbynumber_freemem (void) weak_function;
extern void __libc_getservbyname_freemem (void) weak_function;
extern void __libc_getservbyport_freemem (void) weak_function;;
/* From nss/getXXent.c */
extern void __libc_getgrent_freemem (void) weak_function;
extern void __libc_getpwent_freemem (void) weak_function;
extern void __libc_getspent_freemem (void) weak_function;
extern void __libc_getaliasent_freemem (void) weak_function;
extern void __libc_gethostent_freemem (void) weak_function;
extern void __libc_getnetent_freemem (void) weak_function;
extern void __libc_getprotoent_freemem (void) weak_function;
extern void __libc_getrpcent_freemem (void) weak_function;
extern void __libc_getservent_freemem (void) weak_function;

/* From either libc.so or libpthread.so  */
extern void __libpthread_freeres (void) weak_function;
/* From either libc.so or libanl.so  */
extern void __gai_freemem (void) weak_function;
/* From either libc.so or librt.so  */
extern void __aio_freemem (void) weak_function;

/* From libcrypto.so.  */
extern void __md5_crypt_freemem (void) weak_function;
extern void __sha256_crypt_freemem (void) weak_function;
extern void __sha512_crypt_freemem (void) weak_function;

static void (*__libc_freeres_funcs[])(void) attribute_relro =
{
  __libc_freemem,
  __hdestroy,
  __gconv_cache_freemem,
  __gconv_conf_freemem,
  __gconv_db_freemem,
  __gconv_dl_freemem,
  __intl_freemem,
  __libio_freemem,
  __libc_fstab_freemem,
  __nscd_gr_map_freemem,
  __nscd_hst_map_freemem,
  __nscd_pw_map_freemem,
  __nscd_serv_map_freemem,
  __nscd_group_map_freemem,
  __libc_regcomp_freemem,
  __libc_atfork_freemem,
  /* __res_thread_freeres deallocates the per-thread resolv_context, which
     in turn drop the reference count of the current global object.  So
     it need to be before __libc_resolv_conf_freemem.  */
  __res_thread_freeres,
  __libc_resolv_res_hconf_freemem,
  __libc_resolv_conf_freemem,
  __libc_printf_freemem,
  __libc_fmtmsg_freemem,
  __libc_setenv_freemem,
  __rpc_freemem,
  __rpc_thread_destroy,
  __libc_getaddrinfo_freemem,
  __libc_tzset_freemem,
  __libc_fgetgrent_freemem,
  __libc_getnameinfo_freemem,
  __libc_getnetgrent_freemem,
  __libc_rcmd_freemem,
  __libc_rexec_freemem,
  __libc_localealias_freemem,
  __libc_getutent_freemem,
  __libc_getutid_freemem,
  __libc_getutline_freemem,
  __libc_mntent_freemem,
  __libc_fgetpwent_freemem,
  __libc_fgetspent_freemem,
  __libc_reg_printf_freemem,
  __libc_reg_type_freemem,
  __libc_tzfile_freemem,

  __libc_efgcvt_freemem,
  __libc_qefgcvt_freemem,

  __libc_getgrgid_freemem,
  __libc_getgrnam_freemem,
  __libc_getpwnam_freemem,
  __libc_getpwuid_freemem,
  __libc_getspnam_freemem,
  __libc_getaliasbyname_freemem,
  __libc_gethostbyaddr_freemem,
  __libc_gethostbyname_freemem,
  __libc_gethostbyname2_freemem,
  __libc_getnetbyaddr_freemem,
  __libc_getnetbyname_freemem,
  __libc_getprotobynumber_freemem,
  __libc_getprotobyname_freemem,
  __libc_getrpcbyname_freemem,
  __libc_getrpcbynumber_freemem,
  __libc_getservbyname_freemem,
  __libc_getservbyport_freemem,

  __libc_getgrent_freemem,
  __libc_getpwent_freemem,
  __libc_getspent_freemem,
  __libc_getaliasent_freemem,
  __libc_gethostent_freemem,
  __libc_getnetent_freemem,
  __libc_getprotoent_freemem,
  __libc_getrpcent_freemem,
  __libc_getservent_freemem,

  __gai_freemem,

  __aio_freemem,

  __libpthread_freeres,

  __md5_crypt_freemem,
  __sha256_crypt_freemem,
  __sha512_crypt_freemem,

  SET_FREERES_SYSTEM_FUNCS
};

void
__libc_freeres (void)
{
  /* This function might be called from different places.  So better
     protect for multiple executions since these are fatal.  */
  static long int already_called;

  if (!atomic_compare_and_exchange_bool_acq (&already_called, 1, 0))
    {
      call_function_static_weak (__nss_module_freeres);
      call_function_static_weak (__nss_action_freeres);
      call_function_static_weak (__nss_database_freeres);

      _IO_cleanup ();

      /* We run the resource freeing after IO cleanup.  */
      for (int i = 0; i < array_length (__libc_freeres_funcs); i++)
	if (__libc_freeres_funcs[i] != NULL)
	  __libc_freeres_funcs[i] ();

#ifdef SHARED
      __libc_unwind_link_freeres ();
#endif

      call_function_static_weak (__libc_dlerror_result_free);

#ifdef SHARED
      GLRO (dl_libc_freeres) ();
#endif
    }
}
libc_hidden_def (__libc_freeres)
