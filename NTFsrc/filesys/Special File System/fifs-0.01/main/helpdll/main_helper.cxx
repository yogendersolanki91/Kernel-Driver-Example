/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   fshelper main

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

#include <windows.h>
#include "debug.hxx"

static CRITICAL_SECTION cs;
//DWORD dwTlsIndex = 0xFFFFFFFF;

#ifdef NDEBUG
#define mem_init()
#else
#undef mem_init
#undef mem_check
#undef mem_alloc
#undef mem_size
#undef mem_free
#include <stdio.h>
static const char MEM_MODULE[] = "fshelper";
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
#endif

BOOL WINAPI 
DllMain(
    HINSTANCE hinstDLL, // handle to DLL module
    DWORD fdwReason, // reason for calling function
    LPVOID lpvReserved // reserved
    )
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
//         dwTlsIndex = TlsAlloc();
//         if (dwTlsIndex == 0xFFFFFFFF)
//             return FALSE;
//         TlsSetValue(dwTlsIndex, 0);
        InitializeCriticalSection(&cs);
        mem_init();
        break;
    case DLL_PROCESS_DETACH:
//         TlsFree(dwTlsIndex);
        DeleteCriticalSection(&cs);
        break;
    case DLL_THREAD_ATTACH:
//        TlsSetValue(dwTlsIndex, 0);
        break;
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

void debug_acquire()
{
    EnterCriticalSection(&cs);
}

void debug_release()
{
    LeaveCriticalSection(&cs);
}
