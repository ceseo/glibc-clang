#ifndef __ASSEMBLER__
# include <misc/sys/single_threaded.h>

# ifndef _ISOMAC

libc_hidden_proto_alias (__libc_single_threaded,
			 __libc_single_threaded_internal);

#if !defined SHARED || !IS_IN(libc)
# define __libc_single_threaded_internal __libc_single_threaded
#endif

# define SINGLE_THREAD_P (__libc_single_threaded_internal != 0)

# define RTLD_SINGLE_THREAD_P SINGLE_THREAD_P

# endif

#endif /* __ASSEMBLER__ */
