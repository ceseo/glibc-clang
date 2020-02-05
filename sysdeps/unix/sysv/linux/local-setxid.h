/* SETxID functions which only have to change the local thread and
   none of the possible other threads.  */
#include <sysdep.h>

#ifdef __NR_setresuid32
# define local_seteuid(id) INLINE_SYSCALL_CALL (setresuid32, -1, id, -1)
#else
# define local_seteuid(id) INLINE_SYSCALL_CALL (setresuid, -1, id, -1)
#endif


#ifdef __NR_setresgid32
# define local_setegid(id) INLINE_SYSCALL_CALL (setresgid32, -1, id, -1)
#else
# define local_setegid(id) INLINE_SYSCALL_CALL (setresgid, -1, id, -1)
#endif
