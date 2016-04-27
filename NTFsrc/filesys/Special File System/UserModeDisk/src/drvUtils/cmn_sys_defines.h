#ifndef CMN_SYS_DEFINES
#define CMN_SYS_DEFINES

// Usage:
// if (IS_KERNEL_HANDLE(FileHandle))
// {
//     ZwClose(FileHandle);
// }
// To include: #include "cmn_sys_defines.h"

#define KERNEL_HANDLE_MASK ((ULONG_PTR)((LONG)0x80000000))

#define IS_KERNEL_HANDLE(H)                                \
    (((KERNEL_HANDLE_MASK & (ULONG_PTR)(H)) == KERNEL_HANDLE_MASK) && \
    ((H) != NtCurrentThread()) &&                         \
    ((H) != NtCurrentProcess()))

// From MS ob\IsKernelHandle
//#define IS_KERNEL_HANDLE(H)                                \
//    (((KERNEL_HANDLE_MASK & (ULONG_PTR)(H)) == KERNEL_HANDLE_MASK) && \
//    /*((M) == KernelMode) &&*/                                \ // this line is not applicable 
//    ((H) != NtCurrentThread()) &&                         \
//    ((H) != NtCurrentProcess()))

#endif