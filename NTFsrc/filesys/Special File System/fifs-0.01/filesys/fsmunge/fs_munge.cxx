/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   fsmunge file system object

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

#include "fsdt_munge.hxx"
#include "conf_munge.hxx"
#include <autolock.hxx>
#include <debug.hxx>

///////////////////////////////////////////////////////////////////////////////
//
// Implements filesystem
//
///////////////////////////////////////////////////////////////////////////////

ULONG Fs_Munge::global_count = 0;


ULONG
Fs_Munge::AddRef(
    )
{
    AutoLock lock(&csObject);
    ULONG count = ref_count;
    ref_count++;
    return count;
}


ULONG
Fs_Munge::Release(
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
Fs_Munge::connect(
    const char* principal,
    FsDispatchTable** ppDT
    )
{
    DFN(FsMunge::connect);

    if (!ppDT) return ERROR_INVALID_PARAMETER;
    *ppDT = 0;

    FsDispatchTable* pDT2;
    {
        DWORD error = pFs->connect(principal, &pDT2);
        if (error) {
            DEBUG_PRINT(("cannot connect to underlying FS (0x%08X)\n", error));
            return error;
        }
    }

    AddRef();
    FsDT_Munge* pDT = new FsDT_Munge(pDT2, this);
    if (!pDT) {
        Release();
        pDT2->Release();
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    *ppDT = pDT;
    return ERROR_SUCCESS;
}


//
// constructor/destructor:
//

Fs_Munge::Fs_Munge(
    const char* config_path
    ):ref_count(1),hFs(0),pFsCreate(0),pFs(0)
{
    InitializeCriticalSection(&csObject);
    InitConfig(config_path);
    global_count++;
}


Fs_Munge::~Fs_Munge(
    )
{
    CleanupConfig();
    if (hFs) FreeLibrary(hFs);
    if (pFs) pFs->Release();
    DeleteCriticalSection(&csObject);
    global_count--;
}


//
// helpers:
//

DWORD
Fs_Munge::init(
    )
{
    DFN(Fs_Munge::init);
    if (!hFs) hFs = LoadLibrary(config.fs_dll);
    if (!hFs) {
        DWORD error = GetLastError();
        DEBUG_PRINT(("could not get filesystem (0x%02x)\n", error));
        return error;
    }
    if (!pFsCreate)
        pFsCreate = (FS_CREATE_PROC)GetProcAddress(hFs, FS_CREATE_PROC_NAME);
    if (!pFsCreate) {
        DWORD error = GetLastError();
        DEBUG_PRINT(("could not get filesystem create function (0x%02x)\n", 
                     error));
        return error;
    }

    DWORD error = pFsCreate(config.fs_name, config.fs_config, 
                            FS_VERSION_0, &pFs);
    if (error) {
        DEBUG_PRINT(("could not instantiate filesystem interface (0x%08X)\n",
                     error));
        return error;
    }

    return ERROR_SUCCESS;
}
