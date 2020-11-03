abi-variants := 32 64
abi-32-condition := __WORDSIZE == 32
abi-64-condition := __WORDSIZE == 64

ifeq ($(subdir),sysvipc)
sysdep_routines += getshmlba
endif

ifeq ($(subdir),signal)
sysdep_routines += sigreturn_stub
endif

ifeq ($(subdir),nptl)
# pull in __syscall_error routine
libpthread-routines += sigreturn_stub
libpthread-shared-only-routines += sigreturn_stub
endif
