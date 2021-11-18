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

#include <array_length.h>
#include <dl-check.h>
#include <endian.h>
#include <ldsodefs.h>
#include <libintl.h>

int
_dl_elfhdr_check (const ElfW(Ehdr) *ehdr)
{
#define ELF32_CLASS ELFCLASS32
#define ELF64_CLASS ELFCLASS64
#if BYTE_ORDER == BIG_ENDIAN
# define byteorder ELFDATA2MSB
#elif BYTE_ORDER == LITTLE_ENDIAN
# define byteorder ELFDATA2LSB
#else
# error "Unknown BYTE_ORDER " BYTE_ORDER
# define byteorder ELFDATANONE
#endif
  MORE_ELF_HEADER_DATA;
  static const unsigned char expected[EI_NIDENT] =
  {
    [EI_MAG0] = ELFMAG0,
    [EI_MAG1] = ELFMAG1,
    [EI_MAG2] = ELFMAG2,
    [EI_MAG3] = ELFMAG3,
    [EI_CLASS] = ELFW(CLASS),
    [EI_DATA] = byteorder,
    [EI_VERSION] = EV_CURRENT,
    [EI_OSABI] = ELFOSABI_SYSV,
    [EI_ABIVERSION] = 0
  };

  /* See whether the ELF header is what we expect.  */
  if (__glibc_unlikely (! VALID_ELF_HEADER (ehdr->e_ident, expected,
					    EI_ABIVERSION)
			|| !VALID_ELF_ABIVERSION (ehdr->e_ident[EI_OSABI],
						  ehdr->e_ident[EI_ABIVERSION])
			|| memcmp (&ehdr->e_ident[EI_PAD],
				   &expected[EI_PAD],
				   EI_NIDENT - EI_PAD) != 0))
    {
      /* Something is wrong.  */
      const Elf32_Word *magp = (const void *) ehdr->e_ident;
      if (*magp !=
#if BYTE_ORDER == LITTLE_ENDIAN
	  ((ELFMAG0 << (EI_MAG0 * 8))
	   | (ELFMAG1 << (EI_MAG1 * 8))
	   | (ELFMAG2 << (EI_MAG2 * 8))
	   | (ELFMAG3 << (EI_MAG3 * 8)))
#else
	  ((ELFMAG0 << (EI_MAG3 * 8))
	   | (ELFMAG1 << (EI_MAG2 * 8))
	   | (ELFMAG2 << (EI_MAG1 * 8))
	   | (ELFMAG3 << (EI_MAG0 * 8)))
#endif
	  )
	return DL_ELFHDR_ERR_ELFMAG;
      else if (ehdr->e_ident[EI_CLASS] != ELFW(CLASS))
	return ELFW(CLASS) == ELFCLASS32
	       ? DL_ELFHDR_ERR_CLASS64
	       : DL_ELFHDR_ERR_CLASS32;
      else if (ehdr->e_ident[EI_DATA] != byteorder)
	{
	  if (BYTE_ORDER == BIG_ENDIAN)
	    return DL_ELFHDR_ERR_BENDIAN;
	  else
	    return DL_ELFHDR_ERR_LENDIAN;
	}
      else if (ehdr->e_ident[EI_VERSION] != EV_CURRENT)
	return DL_ELFHDR_ERR_EIVERSION;
      /* XXX We should be able so set system specific versions which are
	 allowed here.  */
      else if (!VALID_ELF_OSABI (ehdr->e_ident[EI_OSABI]))
	return DL_ELFHDR_ERR_OSABI;
      else if (!VALID_ELF_ABIVERSION (ehdr->e_ident[EI_OSABI],
				      ehdr->e_ident[EI_ABIVERSION]))
	return DL_ELFHDR_ERR_ABIVERSION;
      else if (memcmp (&ehdr->e_ident[EI_PAD], &expected[EI_PAD],
		       EI_NIDENT - EI_PAD) != 0)
	return DL_ELFHDR_ERR_PAD;
      else
	return DL_ELFHDR_ERR_INTERNAL;
    }

  if (__glibc_unlikely (ehdr->e_version != EV_CURRENT))
    return DL_ELFHDR_ERR_VERSION;
  else if (__glibc_unlikely (ehdr->e_type != ET_DYN
			     && ehdr->e_type != ET_EXEC))
    return DL_ELFHDR_ERR_TYPE;
  else if (__glibc_unlikely (ehdr->e_phentsize != sizeof (ElfW(Phdr))))
    return DL_ELFHDR_ERR_PHENTSIZE;

  return DL_ELFHDR_OK;
}

static const union elfhdr_errstr_t
{
  struct
  {
#define _S(n, s) char str##n[sizeof (s)];
#include "dl-check-err.h"
#undef _S
  };
  char str[0];
} elfhdr_errstr =
{
  {
#define _S(n, s) s,
#include "dl-check-err.h"
#undef _S
  }
};

static const unsigned short elfhder_erridx[] =
{
#define _S(n, s) [n] = offsetof(union elfhdr_errstr_t, str##n),
#include "dl-check-err.h"
#undef _S
};

const char *
_dl_elfhdr_errstr (int err)
{
#if 0
  if (err >= 0 && err < array_length (elfhdr_errstr))
    return elfhdr_errstr[err];
  return NULL;
#endif
  if (err < 0 || err >= array_length (elfhder_erridx))
    err = 0;
  return elfhdr_errstr.str + elfhder_erridx[err];
}
