/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   fswin32 file system object header

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

#ifndef __FS_WIN32_HXX__
#define __FS_WIN32_HXX__

#include <windows.h>
#include <fsinterface.hxx>

using namespace FsInterface;

///////////////////////////////////////////////////////////////////////////////
//
// Defines filesystem
//
///////////////////////////////////////////////////////////////////////////////

class Fs_Win32: public FileSystem {
public:
    ULONG AddRef();
    ULONG Release();
    
    DWORD connect(const char* principal, FsDispatchTable** ppDT);

private:
    Fs_Win32(const char* config_path);
    ~Fs_Win32();

    // helpers:
    DWORD init();

    // global:
    static ULONG global_count;

    // data:
    ULONG ref_count;
    CRITICAL_SECTION csObject;

    // friends:
    friend BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID);
    friend DWORD WINAPI FileSystemCreate(LPCSTR, LPCSTR, DWORD, FileSystem**);
};

#endif /* __FS_WIN32_HXX__ */
