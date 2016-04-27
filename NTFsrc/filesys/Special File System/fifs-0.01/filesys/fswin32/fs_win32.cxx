/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   fswin32 file system object

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

#include "fsdt_win32.hxx"
#include "conf_win32.hxx"
#include <autolock.hxx>

///////////////////////////////////////////////////////////////////////////////
//
// Implements filesystem
//
///////////////////////////////////////////////////////////////////////////////

ULONG Fs_Win32::global_count = 0;


ULONG
Fs_Win32::AddRef(
    )
{
    AutoLock lock(&csObject);
    ULONG count = ref_count;
    ref_count++;
    return count;
}


ULONG
Fs_Win32::Release(
    )
{
    EnterCriticalSection(&csObject);
    if (ref_count) ref_count--;
    ULONG count = ref_count;
    if (!ref_count) {
        delete this; // we delete csObject here...
    } else {
        LeaveCriticalSection(&csObject);
    }
    return count;
}


DWORD
Fs_Win32::connect(
    const char* principal,
    FsDispatchTable** ppDT
    )
{
    if (!ppDT) return ERROR_INVALID_PARAMETER;
    *ppDT = 0;
    AddRef();
    FsDT_Win32* pDT = new FsDT_Win32(principal, this);
    if (!pDT) {
        Release();
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    DWORD error = pDT->init();
    if (error) {
        pDT->Release();
        return error;
    }
    *ppDT = pDT;
    return ERROR_SUCCESS;
}


//
// constructor/destructor:
//

Fs_Win32::Fs_Win32(
    const char* config_path
    ):ref_count(1)
{
    InitializeCriticalSection(&csObject);
    InitConfig(config_path);
    global_count++;
}


Fs_Win32::~Fs_Win32(
    )
{
    CleanupConfig();
    DeleteCriticalSection(&csObject);
    global_count--;
}


//
// helpers:
//


DWORD
Fs_Win32::init(
    )
{
    return ERROR_SUCCESS;
}
