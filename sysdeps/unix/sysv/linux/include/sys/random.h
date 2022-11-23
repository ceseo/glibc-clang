#ifndef _SYS_RANDOM_H

#ifndef _ISOMAC

#include <bits/types.h>

#define VGRA_DEALLOCATE	0x0001ULL

/* The unsigned 64-bit and 8-byte aligned integer type.  */
typedef __U64_TYPE __aligned_uint64_t __attribute__ ((__aligned__ (8)));

struct vgetrandom_alloc_args
{
  __aligned_uint64_t flags;
  __aligned_uint64_t states;
  __aligned_uint64_t num;
  __aligned_uint64_t size_per_each;
};

#define VGETRANDOM_ALLOC_ARGS_SIZE_VER0 32

#endif

#endif
