/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   SMB utilities

   This file is part of FIFS (Framework for Implementing File Systems). 

   This software is distributed with NO WARRANTY OF ANY KIND.  No
   author or distributor accepts any responsibility for the
   consequences of using it, or for whether it serves any particular
   purpose or works at all, unless he or she says so in writing.
   Refer to the included modified Alladin Free Public License (the
   "License") for full details.

   Every copy of this software must include a copy of the License, in
   a plain ASCII text file named COPYING.  The License grants you the
   right to copy, modify and redistribute this software, but only
   under certain conditions described in the License.  Among other
   things, the License requires that the copyright notice and this
   notice be preserved on all copies.

*/

#include "smball.hxx"
#include <debug.hxx>

#if TRANS2_MAX_FUNCTION > 0xFF
#pragma error "TRANS2_MAX_FUNCTION > 0xFF"
#endif

typedef BOOL (*SMB_DISPATCH_FUNC)(DPB*);
typedef BOOL (*TRANS2_DISPATCH_FUNC)(DPB*, T2B*);
typedef VOID (WINAPI *DUMP_SMB_FUNC) (PVOID, DWORD, BOOL);

static SMB_DISPATCH_FUNC SmbDispatchTable[0x100] = {0};
static TRANS2_DISPATCH_FUNC Trans2DispatchTable[0x100] = {0};

static DUMP_SMB_FUNC pDumpSmb = 0;
static HMODULE hLib = 0;

// const LONGLONG tz_time64_factor = 10*1000*1000*60; // 100ns per minute
// LONGLONG tz_minutes_offset = 0;

#ifdef NDEBUG
#define mem_init()
#else
#undef mem_init
#undef mem_check
#undef mem_alloc
#undef mem_size
#undef mem_free
#include <stdio.h>
static const char MEM_MODULE[] = "server";
static const int check_interval = 100;

static int alloc_count = 0;
static int alloc_total = 0;
static int new_count = 0;
static int new_total = 0;
static CRITICAL_SECTION cs_alloc;
static CRITICAL_SECTION cs_new;
static int cs_init = 0;

static int checker = 0;

void mem_init()
{
    InitializeCriticalSection(&cs_alloc);
    InitializeCriticalSection(&cs_new);
    cs_init = 1;
}

void mem_check()
{
    const DWORD max = 100;
    HANDLE heaps[max];
    DWORD num = GetProcessHeaps(max, heaps);
    if (num > max)
        printf("%s: mem_check: more than %d heaps found (%d)\n", MEM_MODULE, max, num);
    printf("%s: heap: got %d\n", MEM_MODULE, num);
//     num = min(max, num);
//     DWORD i;
//     BOOL b;
//     PROCESS_HEAP_ENTRY h;
//     for (i = 0; i < num; i++) {
//         b = HeapLock(heaps[i]);
//         if (!b) next;
//         h.lpData = 0;
//         b = HeapWalk(heaps[i], &h);
//         printf("heap: num %d is %d data bytes %d overhead bytes\n", 
//                i, h.);
//     }
}

void* mem_alloc(size_t size)
{
    checker++;
    if (!(checker % check_interval)) mem_check();
//    size_t* p = (size_t*)LocalAlloc(LMEM_FIXED, size + sizeof(size_t));
//    *p = size;
//    p++;
//    return p;
    return LocalAlloc(LMEM_FIXED, size);
}

size_t mem_size(void* buffer)
{
//     size_t* p = (size_t*)buffer;
//     p--;
//     return *p;
    return LocalSize(buffer);
}

void mem_free(void* buffer)
{
    checker++;
    if (!(checker % check_interval)) mem_check();
//     size_t* p = (size_t*)buffer;
//     p--;
//     if(LocalFree(p))
//         printf("mem_free: could not free 0x%08X (%d bytes)\n", buffer, *p);
    if(LocalFree(buffer))
        printf("%s: mem_free: could not free 0x%08X (%d bytes)\n", 
               MEM_MODULE, buffer, LocalSize(buffer));
}

void *operator new( size_t size)
{
    void* buffer =  mem_alloc(size);
    if (buffer) {
        size = mem_size(buffer);
        int count, total;
        if (cs_init) EnterCriticalSection(&cs_new);
        count = ++new_count;
        total = new_total += size;
        if (cs_init) LeaveCriticalSection(&cs_new);
        printf("%s: new: 0x%08X (%d bytes) (total: %d blocks for %d bytes)\n", 
               MEM_MODULE, buffer, size, count, total);
    }
    return buffer;
}

void operator delete(void* buffer)
{
    if (buffer) {
        size_t size = mem_size(buffer);
        mem_free(buffer);
        int count, total;
        if (cs_init) EnterCriticalSection(&cs_new);
        count = --new_count;
        total = new_total -= size;
        if (cs_init) LeaveCriticalSection(&cs_new);
        printf("%s: del: 0x%08X (%d bytes) (total: %d blocks for %d bytes)\n", 
               MEM_MODULE, buffer, size, count, total);
    }
}

void* xmalloc(size_t size)
{ 
    void* buffer =  mem_alloc(size);
    if (buffer) {
        int count, total;
        if (cs_init) EnterCriticalSection(&cs_alloc);
        count = ++alloc_count;
        total = alloc_total += size;
        if (cs_init) LeaveCriticalSection(&cs_alloc);
        printf("%s: alloc: 0x%08X (%d bytes) (total: %d blocks for %d bytes)\n", 
               MEM_MODULE, buffer, size, count, total);
    }
    return buffer;
}

void  xfree(void* buffer)
{
    if (buffer) {
        size_t size = mem_size(buffer);
        mem_free(buffer);
        int count, total;
        if (cs_init) EnterCriticalSection(&cs_alloc);
        count = --alloc_count;
        total = alloc_total -= size;
        if (cs_init) LeaveCriticalSection(&cs_alloc);
        printf("%s: free: 0x%08X (%d bytes) (total: %d blocks for %d bytes)\n", 
               MEM_MODULE, buffer, size, count, total);
    }
}
#endif

VOID
SmbInitDispTable(
    );

void
smbutil_init()
{
//    freopen( "out", "w", stdout );
    mem_init();
    DFN(smbutil_init);
    if (!pDumpSmb) {
        HMODULE hLib = LoadLibrary("netapi32.dll");
        if (hLib) pDumpSmb = (DUMP_SMB_FUNC) GetProcAddress(hLib, "DumpSmb");
    }
    SmbInitDispTable();

//     TIME_ZONE_INFORMATION tzi;
//     tzi.Bias = 0;
//     tzi.StandardBias = 0;
//     tzi.DaylightBias = 0;

//     DWORD code = GetTimeZoneInformation(&tzi);
//     switch (code) {
//     case TIME_ZONE_ID_UNKNOWN:
//         tz_minutes_offset = tzi.Bias;
//         break;
//     case TIME_ZONE_ID_STANDARD:
//         tz_minutes_offset = tzi.Bias + tzi.StandardBias;
//         break;        
//     case TIME_ZONE_ID_DAYLIGHT:
//         tz_minutes_offset = tzi.Bias + tzi.DaylightBias;
//         break;
//     default:
//         DEBUG_PRINT(("CANNOT FIGURE OUT TIMEZONE!\n"));
//         tz_minutes_offset = 0;
//     }
//     tz_minutes_offset *= tz_time64_factor;
}

void
smbutil_cleanup()
{
    pDumpSmb = 0;
    FreeLibrary(hLib);
}

#ifndef NDEBUG
VOID
DumpSmb(
    PVOID buffer,
    DWORD size,
    BOOL bRequest
    )
{
    if (!pDumpSmb) return;
    pDumpSmb(buffer, size, bRequest);
}
#endif

BOOL
IsSmb(
    LPVOID pBuffer,
    DWORD nLength
    )
{
    PNT_SMB_HEADER pSmb = (PNT_SMB_HEADER) pBuffer;
    return (pSmb && !(nLength < sizeof(NT_SMB_HEADER)) &&
            (*(PULONG)pSmb->Protocol == SMB_HEADER_PROTOCOL));
}

BOOL
SmbDispatch(
    DPB* dpb
    )
{
    DFN(SmbDispatch);
    if (dpb->in.command > 0xFF) {
        DEBUG_PRINT(("(command > 0xFF)\n"));
        return FALSE;
    }
    UCHAR command = (UCHAR) dpb->in.command;
    DEBUG_PRINT(("- parameter block - <0x%02x = %s>\n",
                 command,
                 SmbUnparseCommand(command)));
    if (SmbDispatchTable[command])
        return SmbDispatchTable[command](dpb);
    else {
        return SmbComUnknown(dpb);
    }
}

// returns whether or not to conitnue
BOOL
Trans2Dispatch(
    DPB* dpb,
    T2B* t2b
    )
{
    DFN(Trans2Dispatch);
    if (dpb->in.command > 0xFF) {
        DEBUG_PRINT(("DOSERROR: (function > 0xFF)\n"));
        SET_DOSERROR(dpb, SERVER, NO_SUPPORT);
        return FALSE;
    }
    UCHAR command = (UCHAR) dpb->in.command;
    if (command > TRANS2_MAX_FUNCTION) {
        DEBUG_PRINT(("DOSERROR: (function > maximum)\n"));
        SET_DOSERROR(dpb, SERVER, NO_SUPPORT);
        return FALSE;
    }
    DEBUG_PRINT(("- parameter block - <0x%02x = %s>\n",
                 command,
                 SmbUnparseTrans2(command)));
    if (Trans2DispatchTable[command])
        return Trans2DispatchTable[command](dpb, t2b);
    else {
        return Trans2Unknown(dpb, t2b);
    }
}   

VOID
InitSmbHeader(
    DPB* dpb
    )
{
    ZeroMemory(dpb->out.smb, sizeof(NT_SMB_HEADER));
    CopyMemory(dpb->out.smb->Protocol, dpb->in.smb->Protocol, 
               sizeof(dpb->out.smb->Protocol));
    dpb->out.smb->Command = dpb->in.smb->Command;
    dpb->out.smb->Flags = 0x80; // ??
    dpb->out.smb->Flags2 = 1; // ??
    dpb->out.smb->Pid = dpb->in.smb->Pid;
    dpb->out.smb->Tid = dpb->in.smb->Tid;
    dpb->out.smb->Mid = dpb->in.smb->Mid;
    dpb->out.smb->Uid = dpb->in.smb->Uid;
}

#define XLAT_STRING(code) static const char STRING_##code[] = #code
#define XLAT_CASE(code) case code: return STRING_##code
#define XLAT_STRING_DEFAULT XLAT_STRING(Unknown)
#define XLAT_CASE_DEFAULT default: return STRING_Unknown

LPCSTR
SmbUnparseCommand(
    UCHAR command
    )
{
    XLAT_STRING_DEFAULT;
    XLAT_STRING(SMB_COM_CREATE_DIRECTORY);
    XLAT_STRING(SMB_COM_DELETE_DIRECTORY);
    XLAT_STRING(SMB_COM_OPEN);
    XLAT_STRING(SMB_COM_CREATE);
    XLAT_STRING(SMB_COM_CLOSE);
    XLAT_STRING(SMB_COM_FLUSH);
    XLAT_STRING(SMB_COM_DELETE);
    XLAT_STRING(SMB_COM_RENAME);
    XLAT_STRING(SMB_COM_QUERY_INFORMATION);
    XLAT_STRING(SMB_COM_SET_INFORMATION);
    XLAT_STRING(SMB_COM_READ);
    XLAT_STRING(SMB_COM_WRITE);
    XLAT_STRING(SMB_COM_LOCK_BYTE_RANGE);
    XLAT_STRING(SMB_COM_UNLOCK_BYTE_RANGE);
    XLAT_STRING(SMB_COM_CREATE_TEMPORARY);
    XLAT_STRING(SMB_COM_CREATE_NEW);
    XLAT_STRING(SMB_COM_CHECK_DIRECTORY);
    XLAT_STRING(SMB_COM_PROCESS_EXIT);
    XLAT_STRING(SMB_COM_SEEK);
    XLAT_STRING(SMB_COM_LOCK_AND_READ);
    XLAT_STRING(SMB_COM_WRITE_AND_UNLOCK);
    XLAT_STRING(SMB_COM_READ_RAW);
    XLAT_STRING(SMB_COM_READ_MPX);
    XLAT_STRING(SMB_COM_READ_MPX_SECONDARY);    // server to redir only
    XLAT_STRING(SMB_COM_WRITE_RAW);
    XLAT_STRING(SMB_COM_WRITE_MPX);
    XLAT_STRING(SMB_COM_WRITE_MPX_SECONDARY);
    XLAT_STRING(SMB_COM_WRITE_COMPLETE);    // server to redir only
    XLAT_STRING(SMB_COM_QUERY_INFORMATION_SRV);
    XLAT_STRING(SMB_COM_SET_INFORMATION2);
    XLAT_STRING(SMB_COM_QUERY_INFORMATION2);
    XLAT_STRING(SMB_COM_LOCKING_ANDX);
    XLAT_STRING(SMB_COM_TRANSACTION);
    XLAT_STRING(SMB_COM_TRANSACTION_SECONDARY);
    XLAT_STRING(SMB_COM_IOCTL);
    XLAT_STRING(SMB_COM_IOCTL_SECONDARY);
    XLAT_STRING(SMB_COM_COPY);
    XLAT_STRING(SMB_COM_MOVE);
    XLAT_STRING(SMB_COM_ECHO);
    XLAT_STRING(SMB_COM_WRITE_AND_CLOSE);
    XLAT_STRING(SMB_COM_OPEN_ANDX);
    XLAT_STRING(SMB_COM_READ_ANDX);
    XLAT_STRING(SMB_COM_WRITE_ANDX);
    XLAT_STRING(SMB_COM_CLOSE_AND_TREE_DISC);
    XLAT_STRING(SMB_COM_TRANSACTION2);
    XLAT_STRING(SMB_COM_TRANSACTION2_SECONDARY);
    XLAT_STRING(SMB_COM_FIND_CLOSE2);
    XLAT_STRING(SMB_COM_FIND_NOTIFY_CLOSE);
    XLAT_STRING(SMB_COM_TREE_CONNECT);
    XLAT_STRING(SMB_COM_TREE_DISCONNECT);
    XLAT_STRING(SMB_COM_NEGOTIATE);
    XLAT_STRING(SMB_COM_SESSION_SETUP_ANDX);
    XLAT_STRING(SMB_COM_LOGOFF_ANDX);
    XLAT_STRING(SMB_COM_TREE_CONNECT_ANDX);
    XLAT_STRING(SMB_COM_QUERY_INFORMATION_DISK);
    XLAT_STRING(SMB_COM_SEARCH);
    XLAT_STRING(SMB_COM_FIND);
    XLAT_STRING(SMB_COM_FIND_UNIQUE);
    XLAT_STRING(SMB_COM_FIND_CLOSE);
    XLAT_STRING(SMB_COM_NT_TRANSACT);
    XLAT_STRING(SMB_COM_NT_TRANSACT_SECONDARY);
    XLAT_STRING(SMB_COM_NT_CREATE_ANDX);
    XLAT_STRING(SMB_COM_NT_CANCEL);
    XLAT_STRING(SMB_COM_NT_RENAME);
    XLAT_STRING(SMB_COM_OPEN_PRINT_FILE);
    XLAT_STRING(SMB_COM_WRITE_PRINT_FILE);
    XLAT_STRING(SMB_COM_CLOSE_PRINT_FILE);
    XLAT_STRING(SMB_COM_GET_PRINT_QUEUE);
    XLAT_STRING(SMB_COM_SEND_MESSAGE);
    XLAT_STRING(SMB_COM_SEND_BROADCAST_MESSAGE);
    XLAT_STRING(SMB_COM_FORWARD_USER_NAME);
    XLAT_STRING(SMB_COM_CANCEL_FORWARD);
    XLAT_STRING(SMB_COM_GET_MACHINE_NAME);
    XLAT_STRING(SMB_COM_SEND_START_MB_MESSAGE);
    XLAT_STRING(SMB_COM_SEND_END_MB_MESSAGE);
    XLAT_STRING(SMB_COM_SEND_TEXT_MB_MESSAGE);

    switch (command)
    {
        XLAT_CASE(SMB_COM_CREATE_DIRECTORY);
        XLAT_CASE(SMB_COM_DELETE_DIRECTORY);
        XLAT_CASE(SMB_COM_OPEN);
        XLAT_CASE(SMB_COM_CREATE);
        XLAT_CASE(SMB_COM_CLOSE);
        XLAT_CASE(SMB_COM_FLUSH);
        XLAT_CASE(SMB_COM_DELETE);
        XLAT_CASE(SMB_COM_RENAME);
        XLAT_CASE(SMB_COM_QUERY_INFORMATION);
        XLAT_CASE(SMB_COM_SET_INFORMATION);
        XLAT_CASE(SMB_COM_READ);
        XLAT_CASE(SMB_COM_WRITE);
        XLAT_CASE(SMB_COM_LOCK_BYTE_RANGE);
        XLAT_CASE(SMB_COM_UNLOCK_BYTE_RANGE);
        XLAT_CASE(SMB_COM_CREATE_TEMPORARY);
        XLAT_CASE(SMB_COM_CREATE_NEW);
        XLAT_CASE(SMB_COM_CHECK_DIRECTORY);
        XLAT_CASE(SMB_COM_PROCESS_EXIT);
        XLAT_CASE(SMB_COM_SEEK);
        XLAT_CASE(SMB_COM_LOCK_AND_READ);
        XLAT_CASE(SMB_COM_WRITE_AND_UNLOCK);
        XLAT_CASE(SMB_COM_READ_RAW);
        XLAT_CASE(SMB_COM_READ_MPX);
        XLAT_CASE(SMB_COM_READ_MPX_SECONDARY);    // server to redir only
        XLAT_CASE(SMB_COM_WRITE_RAW);
        XLAT_CASE(SMB_COM_WRITE_MPX);
        XLAT_CASE(SMB_COM_WRITE_MPX_SECONDARY);
        XLAT_CASE(SMB_COM_WRITE_COMPLETE);    // server to redir only
        XLAT_CASE(SMB_COM_QUERY_INFORMATION_SRV);
        XLAT_CASE(SMB_COM_SET_INFORMATION2);
        XLAT_CASE(SMB_COM_QUERY_INFORMATION2);
        XLAT_CASE(SMB_COM_LOCKING_ANDX);
        XLAT_CASE(SMB_COM_TRANSACTION);
        XLAT_CASE(SMB_COM_TRANSACTION_SECONDARY);
        XLAT_CASE(SMB_COM_IOCTL);
        XLAT_CASE(SMB_COM_IOCTL_SECONDARY);
        XLAT_CASE(SMB_COM_COPY);
        XLAT_CASE(SMB_COM_MOVE);
        XLAT_CASE(SMB_COM_ECHO);
        XLAT_CASE(SMB_COM_WRITE_AND_CLOSE);
        XLAT_CASE(SMB_COM_OPEN_ANDX);
        XLAT_CASE(SMB_COM_READ_ANDX);
        XLAT_CASE(SMB_COM_WRITE_ANDX);
        XLAT_CASE(SMB_COM_CLOSE_AND_TREE_DISC);
        XLAT_CASE(SMB_COM_TRANSACTION2);
        XLAT_CASE(SMB_COM_TRANSACTION2_SECONDARY);
        XLAT_CASE(SMB_COM_FIND_CLOSE2);
        XLAT_CASE(SMB_COM_FIND_NOTIFY_CLOSE);
        XLAT_CASE(SMB_COM_TREE_CONNECT);
        XLAT_CASE(SMB_COM_TREE_DISCONNECT);
        XLAT_CASE(SMB_COM_NEGOTIATE);
        XLAT_CASE(SMB_COM_SESSION_SETUP_ANDX);
        XLAT_CASE(SMB_COM_LOGOFF_ANDX);
        XLAT_CASE(SMB_COM_TREE_CONNECT_ANDX);
        XLAT_CASE(SMB_COM_QUERY_INFORMATION_DISK);
        XLAT_CASE(SMB_COM_SEARCH);
        XLAT_CASE(SMB_COM_FIND);
        XLAT_CASE(SMB_COM_FIND_UNIQUE);
        XLAT_CASE(SMB_COM_FIND_CLOSE);
        XLAT_CASE(SMB_COM_NT_TRANSACT);
        XLAT_CASE(SMB_COM_NT_TRANSACT_SECONDARY);
        XLAT_CASE(SMB_COM_NT_CREATE_ANDX);
        XLAT_CASE(SMB_COM_NT_CANCEL);
        XLAT_CASE(SMB_COM_NT_RENAME);
        XLAT_CASE(SMB_COM_OPEN_PRINT_FILE);
        XLAT_CASE(SMB_COM_WRITE_PRINT_FILE);
        XLAT_CASE(SMB_COM_CLOSE_PRINT_FILE);
        XLAT_CASE(SMB_COM_GET_PRINT_QUEUE);
        XLAT_CASE(SMB_COM_SEND_MESSAGE);
        XLAT_CASE(SMB_COM_SEND_BROADCAST_MESSAGE);
        XLAT_CASE(SMB_COM_FORWARD_USER_NAME);
        XLAT_CASE(SMB_COM_CANCEL_FORWARD);
        XLAT_CASE(SMB_COM_GET_MACHINE_NAME);
        XLAT_CASE(SMB_COM_SEND_START_MB_MESSAGE);
        XLAT_CASE(SMB_COM_SEND_END_MB_MESSAGE);
        XLAT_CASE(SMB_COM_SEND_TEXT_MB_MESSAGE);
        XLAT_CASE_DEFAULT;
    }
}

LPCSTR
SmbUnparseTrans2(
    USHORT code
    )
{
    XLAT_STRING_DEFAULT;
    XLAT_STRING(TRANS2_OPEN2);
    XLAT_STRING(TRANS2_FIND_FIRST2);
    XLAT_STRING(TRANS2_FIND_NEXT2);
    XLAT_STRING(TRANS2_QUERY_FS_INFORMATION);
    XLAT_STRING(TRANS2_SET_FS_INFORMATION);
    XLAT_STRING(TRANS2_QUERY_PATH_INFORMATION);
    XLAT_STRING(TRANS2_SET_PATH_INFORMATION);
    XLAT_STRING(TRANS2_QUERY_FILE_INFORMATION);
    XLAT_STRING(TRANS2_SET_FILE_INFORMATION);
    XLAT_STRING(TRANS2_FSCTL);
    XLAT_STRING(TRANS2_IOCTL2);
    XLAT_STRING(TRANS2_FIND_NOTIFY_FIRST);
    XLAT_STRING(TRANS2_FIND_NOTIFY_NEXT);
    XLAT_STRING(TRANS2_CREATE_DIRECTORY);
    XLAT_STRING(TRANS2_SESSION_SETUP);
    XLAT_STRING(TRANS2_QUERY_FS_INFORMATION_FID);
    XLAT_STRING(TRANS2_GET_DFS_REFERRAL);
    XLAT_STRING(TRANS2_REPORT_DFS_INCONSISTENCY);

    switch (code)
    {
        XLAT_CASE(TRANS2_OPEN2);
        XLAT_CASE(TRANS2_FIND_FIRST2);
        XLAT_CASE(TRANS2_FIND_NEXT2);
        XLAT_CASE(TRANS2_QUERY_FS_INFORMATION);
        XLAT_CASE(TRANS2_SET_FS_INFORMATION);
        XLAT_CASE(TRANS2_QUERY_PATH_INFORMATION);
        XLAT_CASE(TRANS2_SET_PATH_INFORMATION);
        XLAT_CASE(TRANS2_QUERY_FILE_INFORMATION);
        XLAT_CASE(TRANS2_SET_FILE_INFORMATION);
        XLAT_CASE(TRANS2_FSCTL);
        XLAT_CASE(TRANS2_IOCTL2);
        XLAT_CASE(TRANS2_FIND_NOTIFY_FIRST);
        XLAT_CASE(TRANS2_FIND_NOTIFY_NEXT);
        XLAT_CASE(TRANS2_CREATE_DIRECTORY);
        XLAT_CASE(TRANS2_SESSION_SETUP);
        XLAT_CASE(TRANS2_QUERY_FS_INFORMATION_FID);
        XLAT_CASE(TRANS2_GET_DFS_REFERRAL);
        XLAT_CASE(TRANS2_REPORT_DFS_INCONSISTENCY);
        XLAT_CASE_DEFAULT;
    }
}

VOID
SmbInitDispTable(
    )
{
    ZeroMemory((PVOID) SmbDispatchTable, sizeof(SmbDispatchTable));
    SmbDispatchTable[SMB_COM_NEGOTIATE] = SmbComNegotiate;
    SmbDispatchTable[SMB_COM_TRANSACTION2] = SmbComTrans2;
    SmbDispatchTable[SMB_COM_SESSION_SETUP_ANDX] = SmbComSessionSetupAndx;
    SmbDispatchTable[SMB_COM_TREE_CONNECT_ANDX] = SmbComTreeConnectAndx;
    SmbDispatchTable[SMB_COM_NO_ANDX_COMMAND] = SmbComNoAndx;
    SmbDispatchTable[SMB_COM_QUERY_INFORMATION] = SmbComQueryInformation;
    SmbDispatchTable[SMB_COM_SET_INFORMATION] = SmbComSetInformation;
    SmbDispatchTable[SMB_COM_CHECK_DIRECTORY] = SmbComCheckDirectory;
    SmbDispatchTable[SMB_COM_FIND_CLOSE2] = SmbComFindClose2;
    SmbDispatchTable[SMB_COM_DELETE] = SmbComDelete;
    SmbDispatchTable[SMB_COM_RENAME] = SmbComRename;
    SmbDispatchTable[SMB_COM_CREATE_DIRECTORY] = SmbComCreateDirectory;
    SmbDispatchTable[SMB_COM_DELETE_DIRECTORY] = SmbComDeleteDirectory;
    SmbDispatchTable[SMB_COM_OPEN_ANDX] = SmbComOpenAndx;
    SmbDispatchTable[SMB_COM_WRITE] = SmbComWrite;
    SmbDispatchTable[SMB_COM_CLOSE] = SmbComClose;
    SmbDispatchTable[SMB_COM_READ_ANDX] = SmbComReadAndx;
    SmbDispatchTable[SMB_COM_QUERY_INFORMATION2] = SmbComQueryInformation2;
    SmbDispatchTable[SMB_COM_SET_INFORMATION2] = SmbComSetInformation2;
    SmbDispatchTable[SMB_COM_LOCKING_ANDX] = SmbComLockingAndx;
    SmbDispatchTable[SMB_COM_SEEK] = SmbComSeek;
    SmbDispatchTable[SMB_COM_FLUSH] = SmbComFlush;
    SmbDispatchTable[SMB_COM_LOGOFF_ANDX] = SmbComLogoffAndx;
    SmbDispatchTable[SMB_COM_TREE_DISCONNECT] = SmbComTreeDisconnect;

    ZeroMemory((PVOID) Trans2DispatchTable, sizeof(Trans2DispatchTable));
    Trans2DispatchTable[TRANS2_QUERY_FS_INFORMATION] = Trans2QueryFsInfo;
    Trans2DispatchTable[TRANS2_FIND_FIRST2] = Trans2FindFirst2;
    Trans2DispatchTable[TRANS2_FIND_NEXT2] = Trans2FindNext2;
    Trans2DispatchTable[TRANS2_QUERY_PATH_INFORMATION] = Trans2QueryPathInfo;
    Trans2DispatchTable[TRANS2_SET_PATH_INFORMATION] = Trans2SetPathInfo;
    Trans2DispatchTable[TRANS2_QUERY_FILE_INFORMATION] = Trans2QueryFileInfo;
    Trans2DispatchTable[TRANS2_SET_FILE_INFORMATION] = Trans2SetFileInfo;
}
