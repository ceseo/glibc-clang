/* ELF header consistency and ABI checks.
   Copyright (C) 1995-2021 Free Software Foundation, Inc.
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

#ifndef _DL_OPENCHECK_H
#define _DL_OPENCHECK_H

#include <link.h>

enum
 {
   DL_ELFHDR_OK,
   DL_ELFHDR_ERR_ELFMAG,     /* Invalid ELFMAGX value.  */
   DL_ELFHDR_ERR_CLASS32,    /* Mismatched EI_CLASS.  */
   DL_ELFHDR_ERR_CLASS64,    /* Mismatched EI_CLASS.  */
   DL_ELFHDR_ERR_BENDIAN,    /* Mismatched EI_DATA (not big-endian).  */
   DL_ELFHDR_ERR_LENDIAN,    /* Mismatched EI_DATA (not little-endian).  */
   DL_ELFHDR_ERR_EIVERSION,  /* Invalid EI_VERSION.  */
   DL_ELFHDR_ERR_OSABI,      /* Invalid EI_OSABI.  */
   DL_ELFHDR_ERR_ABIVERSION, /* Invalid ABI vrsion.  */
   DL_ELFHDR_ERR_PAD,        /* Invalid EI_PAD value.  */
   DL_ELFHDR_ERR_VERSION,    /* Invalid e_version.  */
   DL_ELFHDR_ERR_TYPE,       /* Invalid e_type.  */
   DL_ELFHDR_ERR_PHENTSIZE,  /* Invalid e_phentsize.  */
   DL_ELFHDR_ERR_INTERNAL    /* Internal error.  */
 };

int _dl_elfhdr_check (const ElfW(Ehdr) *ehdr) attribute_hidden;
const char *_dl_elfhdr_errstr (int err) attribute_hidden;

#endif
