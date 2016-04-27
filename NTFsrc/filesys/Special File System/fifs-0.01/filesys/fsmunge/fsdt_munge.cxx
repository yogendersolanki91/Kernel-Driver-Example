/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   fsmunge file system dispatch table object

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
#include <autolock.hxx>
#include <debug.hxx>

///////////////////////////////////////////////////////////////////////////////
//
// Implements filesystem dispatch table
//
///////////////////////////////////////////////////////////////////////////////

ULONG FsDT_Munge::global_count = 0;


ULONG
FsDT_Munge::AddRef(
    )
{
    AutoLock lock(&csObject);
    ULONG count = ref_count;
    ref_count++;
    return count;
}


ULONG
FsDT_Munge::Release(
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


const char*
FsDT_Munge::get_principal()
{
    return pDT->get_principal();
}

fhandle_t
FsDT_Munge::get_root(
    )
{
    return 0;
}

DWORD
FsDT_Munge::create(
    fhandle_t dir, 
    const char* name, 
    UINT32 flags, 
    fattr_t* fattr, 
    fhandle_t* phandle
    )
{
    PathHandle p(dir, name, pDT);
    if (p.error()) return p.error();
    return pDT->create(p.dir(), p.name(), flags, fattr, phandle);
}


DWORD
FsDT_Munge::lookup(
    fhandle_t dir,
    const char* name,
    fattr_t* attr
    )
{
    PathHandle p(dir, name, pDT);
    if (p.error()) return p.error();
    return pDT->lookup(p.dir(), p.name(), attr);
}


DWORD
FsDT_Munge::remove(
    fhandle_t dir, 
    const char* name
    )
{
    PathHandle p(dir, name, pDT);
    if (p.error()) return p.error();
    return pDT->remove(p.dir(), p.name());
}

DWORD
FsDT_Munge::rename(
    fhandle_t from_dir, 
    const char* from_name, 
    fhandle_t to_dir, 
    const char* to_name
    )
{
    PathHandle pf(from_dir, from_name, pDT);
    if (pf.error()) return pf.error();
    PathHandle pt(to_dir, to_name, pDT);
    if (pt.error()) return pt.error();
    return pDT->rename(pf.dir(), pf.name(),
                       pt.dir(), pt.name());
}


DWORD
FsDT_Munge::mkdir(
    fhandle_t dir, 
    const char* name, 
    fattr_t* attr
    )
{
    PathHandle p(dir, name, pDT);
    if (p.error()) return p.error();
    return pDT->mkdir(p.dir(), p.name(), attr);
}


DWORD
FsDT_Munge::rmdir(
    fhandle_t dir, 
    const char* name
    )
{
    PathHandle p(dir, name, pDT);
    if (p.error()) return p.error();
    return pDT->rmdir(p.dir(), p.name());
}


DWORD
FsDT_Munge::set_attr(
    fhandle_t handle,
    fattr_t* attr
    )
{
    return pDT->set_attr(handle, attr);
}

DWORD
FsDT_Munge::get_attr(
    fhandle_t handle, 
    fattr_t* attr
    )
{
    return pDT->get_attr(handle, attr);
}


DWORD
FsDT_Munge::close(
    fhandle_t handle
    )
{
    return pDT->close(handle);
}

DWORD
FsDT_Munge::write(
    fhandle_t handle, 
    UINT64 offset, 
    UINT64* pcount, 
    void* buffer
    )
{
    return pDT->write(handle, offset, pcount, buffer);
}


DWORD
FsDT_Munge::read(
    fhandle_t handle, 
    UINT64 offset, 
    UINT64* pcount, 
    void* buffer
    )
{
    return pDT->read(handle, offset, pcount, buffer);
}


DWORD
FsDT_Munge::readlink(
    fhandle_t handle, 
    int* size, 
    char* path_buffer
    )
{
    return pDT->readlink(handle, size, path_buffer);
}


DWORD
FsDT_Munge::symlink(
    fhandle_t dir, 
    const char* name, 
    const char* path
    )
{
    PathHandle p(dir, name, pDT);
    if (p.error()) return p.error();
    return pDT->symlink(p.dir(), p.name(), path);
}


DWORD
FsDT_Munge::read_dir(
    fhandle_t dir, 
    UINT32 cookie, 
    dirinfo_t* buffer,
    UINT32 size, 
    UINT32 *entries_found
    )
{
    return pDT->read_dir(dir, cookie, buffer, size, entries_found);
}


DWORD
FsDT_Munge::statfs(
    fhandle_t handle, 
    fs_attr_t* attr
    )
{
    return pDT->statfs(handle, attr);
}

DWORD
FsDT_Munge::link(
    fhandle_t dir, 
    const char* name, 
    fhandle_t handle
    )
{
    PathHandle p(dir, name, pDT);
    if (p.error()) return p.error();
    return pDT->link(p.dir(), p.name(), handle);
}

DWORD 
FsDT_Munge::ioctl(
    fhandle_t handle, 
    UINT32 code, 
    void* in_buffer, 
    UINT32* out_size, 
    void* out_buffer
    )
{
    return pDT->ioctl(handle, code, in_buffer, out_size, out_buffer);
}

DWORD
FsDT_Munge::flush(
    fhandle_t handle
    )
{
    return pDT->flush(handle);
}


//
// constructor/destructor:
//


FsDT_Munge::FsDT_Munge(
    FsDispatchTable* _pDT,
    FileSystem* _pFs // caller must AddRef beforehand
    ):ref_count(1), pDT(_pDT), pFs(_pFs)
{
    InitializeCriticalSection(&csObject);
    global_count++;
}

FsDT_Munge::~FsDT_Munge(
    )
{
    pDT->Release();
    pFs->Release();
    DeleteCriticalSection(&csObject);
    global_count--;
}
