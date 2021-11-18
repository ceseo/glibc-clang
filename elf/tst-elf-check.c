/* Check ELF header error paths.
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
   <https://www.gnu.org/licenses/>.  */

#include <elf.h>
#include <getopt.h>
#include <link.h>
#include <libc-abis.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <support/capture_subprocess.h>
#include <support/check.h>
#include <support/support.h>
#include <support/xunistd.h>
#include <support/temp_file.h>

static char *spargv[6];
static char *tmpbin;

static int restart;
#define CMDLINE_OPTIONS \
    { "restart", no_argument, &restart, 1 },

static void
copy_binary (const char *binary_path)
{
  int fdin = xopen (binary_path, O_RDONLY | O_LARGEFILE, 0);
  struct stat64 st;
  xfstat (fdin, &st);
  int fdout = create_temp_file ("tst-elf-check-", &tmpbin);
  xfchmod (fdout, S_IXUSR | S_IRUSR | S_IWUSR);
  TEST_VERIFY_EXIT (fdout >= 0);
  xcopy_file_range (fdin, NULL, fdout, NULL, st.st_size, 0);
  xclose (fdin);
  xclose (fdout);
}

static void
run_test_expect_failure (void (*modify)(ElfW(Ehdr) *), const char *errmsg)
{
  int fd = xopen (tmpbin, O_RDWR | O_LARGEFILE, 0);
  ElfW(Ehdr) orig_hdr;
  if (read (fd, &orig_hdr, sizeof (orig_hdr)) != sizeof (orig_hdr))
    FAIL_EXIT1 ("read (%s): %m\n", tmpbin);
  ElfW(Ehdr) hdr = orig_hdr;
  modify (&hdr);
  if (lseek (fd, 0, SEEK_SET) != 0)
    FAIL_EXIT1 ("lseek: %m");
  xwrite (fd, &hdr, sizeof (hdr));
  xclose (fd);

  struct support_capture_subprocess proc =
      support_capture_subprogram (spargv[0], spargv);
  support_capture_subprocess_check (&proc, "tst-elf-check", 127,
				    sc_allow_stderr);
  TEST_VERIFY (strstr (proc.err.buffer, errmsg) != NULL);
  support_capture_subprocess_free (&proc);

  /* Restore previous header.  */
  fd = xopen (tmpbin, O_RDWR | O_LARGEFILE, 0);
  xwrite (fd, &orig_hdr, sizeof (orig_hdr));
  xclose (fd);
}

static void
modify_mag (ElfW(Ehdr) *ehdr)
{
  ehdr->e_ident[EI_MAG0] = EI_MAG3;
  ehdr->e_ident[EI_MAG1] = EI_MAG2;
  ehdr->e_ident[EI_MAG2] = EI_MAG1;
  ehdr->e_ident[EI_MAG3] = EI_MAG0;
}

static void
modify_class (ElfW(Ehdr) *ehdr)
{
  ehdr->e_ident[EI_CLASS] = ELFCLASSNONE;
}

static void
modify_endian (ElfW(Ehdr) *ehdr)
{
  ehdr->e_ident[EI_DATA] = ELFDATANONE;
}

static void
modify_eiversion (ElfW(Ehdr) *ehdr)
{
  ehdr->e_ident[EI_VERSION] = EV_CURRENT + 1;
}

static void
modify_osabi (ElfW(Ehdr) *ehdr)
{
  ehdr->e_ident[EI_OSABI] = ELFOSABI_STANDALONE;
}

static void
modify_abiversion (ElfW(Ehdr) *ehdr)
{
  ehdr->e_ident[EI_ABIVERSION] = LIBC_ABI_MAX;
}

static void
modify_pad (ElfW(Ehdr) *ehdr)
{
  memset (&ehdr->e_ident[EI_PAD], 0xff, EI_NIDENT - EI_PAD);
}

static void
modify_version (ElfW(Ehdr) *ehdr)
{
  ehdr->e_version = EV_NONE;
}

static void
modify_type (ElfW(Ehdr) *ehdr)
{
  ehdr->e_type = ET_NONE;
}

static void
modify_phentsize (ElfW(Ehdr) *ehdr)
{
  ehdr->e_phentsize = sizeof (ElfW(Phdr)) + 1;
}

static void
do_test_kernel (void)
{
  run_test_expect_failure (modify_mag,
			   "invalid ELF header");
  run_test_expect_failure (modify_type,
			   "only ET_DYN and ET_EXEC can be loaded");
  run_test_expect_failure (modify_phentsize,
			   "ELF file's phentsize not the expected size");
  run_test_expect_failure (modify_class,
			   "wrong ELF class");
}

static void
do_test_common (void)
{
  run_test_expect_failure (modify_endian,
			   "ELF file data encoding not");
  run_test_expect_failure (modify_eiversion,
			   "ELF file version ident does not match current one");
  run_test_expect_failure (modify_pad,
			   "nonzero padding in e_ident");
  run_test_expect_failure (modify_osabi,
			   "ELF file OS ABI invalid");
  run_test_expect_failure (modify_abiversion,
			   "ELF file ABI version invalid");
  run_test_expect_failure (modify_version,
			   "ELF file version does not match current one");
}

static int
do_test (int argc, char *argv[])
{
  /* We must have one or four parameters:
     + argv[0]:   the application name
     + argv[1]:   path for ld.so        optional
     + argv[2]:   "--library-path"      optional
     + argv[3]:   the library path      optional
     + argv[4/1]: the application name  */

  if (!restart)
    copy_binary (argv[0]);
  else
    abort ();

  bool hardpath = argc == 2;

  int i;
  for (i = 0; i < argc - 2; i++)
    spargv[i] = argv[i+1];
  spargv[i++] = tmpbin;
  spargv[i++] = (char *) "--direct";
  spargv[i++] = (char *) "--restart";
  spargv[i] = NULL;

  /* Some fields are checked by the kernel results in a execve failure, so skip
     them for --enable-hardcoded-path-in-tests.  */
  if (!hardpath)
    do_test_kernel ();
  do_test_common ();

  /* Also run the tests without issuing the loader.  */
  if (hardpath)
    return 0;

  spargv[0] = tmpbin;
  spargv[1] = (char *) "--direct";
  spargv[2] = (char *) "--restart";
  spargv[3] = NULL;

  do_test_common ();

  return 0;
}

#define TEST_FUNCTION_ARGV do_test
#include <support/test-driver.c>
