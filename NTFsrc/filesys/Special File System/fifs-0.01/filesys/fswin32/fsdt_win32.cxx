/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   fswin32 file system dispatch table object

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
#include <debug.hxx>

///////////////////////////////////////////////////////////////////////////////
//
// Implements filesystem dispatch table
//
///////////////////////////////////////////////////////////////////////////////

ULONG FsDT_Win32::global_count = 0;


ULONG
FsDT_Win32::AddRef(
    )
{
    AutoLock lock(&csObject);
    ULONG count = ref_count;
    ref_count++;
    return count;
}


ULONG
FsDT_Win32::Release(
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
FsDT_Win32::get_principal()
{
    AutoLock lock(&csObject);
    return principal;
}

fhandle_t
FsDT_Win32::get_root(
    )
{
    return 0;
}

DWORD
FsDT_Win32::create(
    fhandle_t dir, 
    const char* name, 
    UINT32 flags, 
    fattr_t* fattr, 
    fhandle_t* phandle
    )
{
    DFN(FsDT_Win32::create);
    AutoLock lock(&csObject);
    DEBUG_PRINT(("FsDT::create(0x%016I64X, %s, 0x%08X, 0x%08X, 0x%08X)\n",
                 dir, name, flags, fattr, phandle));

    if (!phandle) return ERROR_INVALID_PARAMETER;
    *phandle = INVALID_FHANDLE_T;

    if (!name) return ERROR_INVALID_PARAMETER;

    if (flags != (FLAGS_MASK & flags)) {
        return ERROR_INVALID_PARAMETER;
    }

    char* basename;
    if (invalid_dir_handle(dir))
        return ERROR_INVALID_PARAMETER;
    else
        basename = handles[dir].pathname;

    if (!basename) return ERROR_INVALID_PARAMETER; // assertion...

    size_t base_size = lstrlen(basename);
    size_t name_size = lstrlen(name);
    size_t full_size = base_size + name_size + 2;
    char* fullname = new char[full_size];

    if (!fullname) return ERROR_NOT_ENOUGH_MEMORY;

    RtlCopyMemory(fullname, basename, base_size);
    RtlCopyMemory(fullname + base_size, "\\", 1);
    RtlCopyMemory(fullname + base_size + 1, name, name_size + 1);

    DWORD error = create2(fullname, full_size, flags, fattr, phandle);
    delete [] fullname;
    return error;
}


DWORD
FsDT_Win32::set_attr(
    fhandle_t handle,
    fattr_t* attr
    )
{
    DFN(FsDT_Win32::set_attr);
    AutoLock lock(&csObject);
    if (invalid_handle(handle) || !attr) return ERROR_INVALID_PARAMETER;
    DEBUG_PRINT(("setting attributes for %s\n", handles[handle].pathname));

    if (attr->attributes != INVALID_UINT32) {
        DEBUG_PRINT(("\tattributes are: 0x%08X\n", attr->attributes));
        if (attr->attributes != (attr->attributes & ATTR_MASK))
            return ERROR_INVALID_PARAMETER;
        DWORD dwAttributes = unget_attributes(attr->attributes);
        if (!SetFileAttributes(handles[handle].pathname, dwAttributes)) {
            DEBUG_PRINT(("\ttrying attributes...failed\n"));
            return GetLastError();
        }
        DEBUG_PRINT(("\ttrying attributes...success\n"));
    }

    FILETIME ftAccess = unget_time(attr->access_time);
    FILETIME ftCreate = unget_time(attr->create_time);
    FILETIME ftMod = unget_time(attr->mod_time);

    LPFILETIME lpftAccess = (attr->access_time == INVALID_TIME64)?0:&ftAccess;
    LPFILETIME lpftCreate = (attr->create_time == INVALID_TIME64)?0:&ftCreate;
    LPFILETIME lpftMod = (attr->mod_time == INVALID_TIME64)?0:&ftMod;

    if (lpftAccess || lpftCreate || lpftMod) {
        if (!SetFileTime(handles[handle].h,
                         lpftCreate,
                         lpftAccess,
                         lpftMod)) {
            DEBUG_PRINT(("\ttrying times...failed\n"));
            return GetLastError();
        }
        DEBUG_PRINT(("\ttrying times...success\n"));
    }

    return ERROR_SUCCESS;
}

DWORD
FsDT_Win32::lookup(
    fhandle_t dir,
    const char* name,
    fattr_t* attr
    )
{
    DFN(FsDT_Win32::lookup);
    AutoLock lock(&csObject);
    if (invalid_dir_handle(dir) || !attr) return ERROR_INVALID_PARAMETER;
    char* basename = handles[dir].pathname;
    
    char* pathname = new char[lstrlen(basename)+lstrlen(name)+2];
    lstrcpy(pathname, basename);
    lstrcat(pathname, "\\");
    lstrcat(pathname, name);

    WIN32_FILE_ATTRIBUTE_DATA data;
    DEBUG_PRINT(("lookup on %s\n", pathname));
    BOOL ok = GetFileAttributesEx(pathname,
                                  GetFileExInfoStandard,
                                  (LPVOID)&data);
    delete [] pathname;
    if (!ok) return GetLastError();

    attr->file_size = ((UINT64)data.nFileSizeLow) + 
        (((UINT64)data.nFileSizeHigh) << 32);;
    attr->alloc_size = attr->file_size;
    attr->access_time = get_time(data.ftLastAccessTime);
    attr->create_time = get_time(data.ftCreationTime);
    attr->mod_time = get_time(data.ftLastWriteTime);
    attr->attributes = get_attributes(data.dwFileAttributes);
    return ERROR_SUCCESS;
}

DWORD
FsDT_Win32::get_attr(
    fhandle_t handle, 
    fattr_t* attr
    )
{
    DFN(FsDT_Win32::get_attr);
    AutoLock lock(&csObject);
    if (invalid_handle(handle) || !attr) return ERROR_INVALID_PARAMETER;
    WIN32_FILE_ATTRIBUTE_DATA data;
    DEBUG_PRINT(("getting attributes for %s\n", handles[handle].pathname));
    if (!GetFileAttributesEx(handles[handle].pathname,
                             GetFileExInfoStandard,
                             (LPVOID)&data)) {
        return GetLastError();
    }
    attr->file_size = ((UINT64)data.nFileSizeLow) + 
        (((UINT64)data.nFileSizeHigh) << 32);;
    attr->alloc_size = attr->file_size;
    attr->access_time = get_time(data.ftLastAccessTime);
    attr->create_time = get_time(data.ftCreationTime);
    attr->mod_time = get_time(data.ftLastWriteTime);
    attr->attributes = get_attributes(data.dwFileAttributes);
    return ERROR_SUCCESS;
}


DWORD
FsDT_Win32::close(
    fhandle_t handle
    )
{
    AutoLock lock(&csObject);
    if (!handle) return ERROR_INVALID_PARAMETER;
    return close2(handle);
}

DWORD
FsDT_Win32::write(
    fhandle_t handle, 
    UINT64 offset, 
    UINT64* pcount, 
    void* buffer
    )
{
    DFN(FsDT_Win32::write);
    AutoLock lock(&csObject);
    if (invalid_file_handle(handle) || !pcount) return ERROR_INVALID_PARAMETER;

    UINT64 count = *pcount;
    if (count > 0xFFFFFFFF) {
        DEBUG_PRINT(("write cannot handle counts larger than 32 bits\n"));
        return ERROR_INVALID_PARAMETER;
    }

    UINT64 new_offset = offset;
    LARGE_INTEGER* plg = (LARGE_INTEGER*)&new_offset;
    
    plg->LowPart = SetFilePointer(handles[handle].h, 
                                  (LONG)plg->LowPart, 
                                  &plg->HighPart,
                                  FILE_BEGIN);
    DWORD error;
    if ((plg->LowPart == 0xFFFFFFFF) && (error = GetLastError())) {
        DEBUG_PRINT(("write could not set file position\n"));
        return error;
    }
    if (offset != new_offset) {
        DEBUG_PRINT(("write found mismatched file position\n"));
        return ERROR_SEEK;
    }
    DWORD actual = 0;
    if (!WriteFile(handles[handle].h,
                   buffer,
                   (DWORD)count,
                   &actual,
                   0)) {
        DEBUG_PRINT(("write failed to write\n"));
        return GetLastError();
    }
    *pcount = actual;
    return ERROR_SUCCESS;
}


DWORD
FsDT_Win32::read(
    fhandle_t handle, 
    UINT64 offset, 
    UINT64* pcount, 
    void* buffer
    )
{
    DFN(FsDT_Win32::read);
    AutoLock lock(&csObject);
    if (invalid_file_handle(handle) || !pcount) return ERROR_INVALID_PARAMETER;

    UINT64 count = *pcount;
    if (count > 0xFFFFFFFF) {
        DEBUG_PRINT(("read cannot handle counts larger than 32 bits\n"));
        return ERROR_INVALID_PARAMETER;
    }

    UINT64 new_offset = offset;
    LARGE_INTEGER* plg = (LARGE_INTEGER*)&new_offset;
    
    plg->LowPart = SetFilePointer(handles[handle].h, 
                                  (LONG)plg->LowPart, 
                                  &plg->HighPart,
                                  FILE_BEGIN);
    DWORD error;
    if ((plg->LowPart == 0xFFFFFFFF) && (error = GetLastError())) {
        DEBUG_PRINT(("read could not set file position\n"));
        return error;
    }
    if (offset != new_offset) {
        DEBUG_PRINT(("read found mismatched file position\n"));
        return ERROR_SEEK;
    }
    DWORD actual = 0;
    if (!ReadFile(handles[handle].h,
                  buffer,
                  (DWORD)count,
                  &actual,
                  0)) {
        DEBUG_PRINT(("read failed to read\n"));
        return GetLastError();
    }
    *pcount = actual;
    return ERROR_SUCCESS;
}


DWORD
FsDT_Win32::readlink(
    fhandle_t handle, 
    int* size, 
    char* path_buffer
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
FsDT_Win32::symlink(
    fhandle_t dir, 
    const char* name, 
    const char* path
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
FsDT_Win32::read_dir(
    fhandle_t dir, 
    UINT32 cookie, 
    dirinfo_t* buffer,
    UINT32 size, 
    UINT32 *entries_found
    )
{
    DFN(FsDT_Win32::read_dir);
    AutoLock lock(&csObject);
    if (!entries_found) return ERROR_INVALID_PARAMETER;
    (*entries_found) = 0;

    if (invalid_dir_handle(dir)) return ERROR_INVALID_PARAMETER;

    handle_info_t& hinfo = handles[dir];

    DEBUG_PRINT(("ready to rock, cookie is %d, size is %d vs %d\n", 
                 cookie, size, sizeof(dirinfo_t)));

    if (size < sizeof(dirinfo_t)) return ERROR_INVALID_PARAMETER;

    while (size >= sizeof(dirinfo_t)) {

        DEBUG_PRINT(("cookie is %d...\n", cookie));
        WIN32_FIND_DATA* data = hinfo.dc->get(cookie);
        if (!data) return hinfo.dc->error()?hinfo.dc->error():ERROR_INVALID_PARAMETER;
        lstrcpyn(buffer->name, data->cFileName, MAX_NAME_LENGTH);
        buffer->attribs.file_size = ((UINT64)data->nFileSizeLow) + 
            (((UINT64)data->nFileSizeHigh) << 32);
        buffer->attribs.alloc_size = buffer->attribs.file_size;
        buffer->attribs.access_time = get_time(data->ftLastAccessTime);
        buffer->attribs.mod_time = get_time(data->ftLastWriteTime);
        buffer->attribs.create_time = get_time(data->ftCreationTime);
        buffer->attribs.attributes = get_attributes(data->dwFileAttributes);
        buffer->cookie = ++cookie;
        buffer += 1;
        size -= sizeof(dirinfo_t);
        (*entries_found)++;

    }

    return ERROR_SUCCESS;
}


DWORD
FsDT_Win32::statfs(
    fhandle_t handle, 
    fs_attr_t* attr
    )
{
    AutoLock lock(&csObject);
    if (!attr) return ERROR_INVALID_PARAMETER;
    if (invalid_handle(handle)) return ERROR_INVALID_PARAMETER;

    ULARGE_INTEGER temp;
    if (!GetDiskFreeSpaceEx(handles[handle].pathname, 
                            &temp,
                            (PULARGE_INTEGER)&attr->total_bytes,
                            (PULARGE_INTEGER)&attr->free_bytes)) {
        return GetLastError();
    }
                       
    lstrcpyn(attr->fs_name, config.label, MAX_FS_NAME_LEN);
//    attr->total_bytes = 0xFFFFFFFF;
//    attr->free_bytes  = 0x00FFFFFF;
    return ERROR_SUCCESS;
}


DWORD
FsDT_Win32::remove(
    fhandle_t dir, 
    const char* name
    )
{
    DFN(FsDT_Win32::remove);
    AutoLock lock(&csObject);
    if (invalid_handle(dir) || !name) return ERROR_INVALID_PARAMETER;

    char* basename = handles[dir].pathname;
    char* pathname = new char[lstrlen(basename)+lstrlen(name)+2];
    lstrcpy(pathname, basename);
    lstrcat(pathname, "\\");
    lstrcat(pathname, name);

    DEBUG_PRINT(("remove on %s\n", pathname));
    BOOL ok = DeleteFile(pathname);
    delete [] pathname;
    if (!ok) return GetLastError();
    return ERROR_SUCCESS;
}

DWORD
FsDT_Win32::rename(
    fhandle_t from_dir, 
    const char* from_name, 
    fhandle_t to_dir, 
    const char* to_name
    )
{
    DFN(FsDT_Win32::rename);
    AutoLock lock(&csObject);
    if (invalid_handle(from_dir) || !from_name ||
        invalid_handle(to_dir) || !to_name) return ERROR_INVALID_PARAMETER;

    char* from_basename = handles[from_dir].pathname;
    char* from_pathname = construct_pathname(from_basename, from_name);

    char* to_basename = handles[to_dir].pathname;
    char* to_pathname = construct_pathname(to_basename, to_name);

    DEBUG_PRINT(("rename %s to %s\n", from_pathname, to_pathname));
    BOOL ok = MoveFile(from_pathname, to_pathname);

    destroy_pathname(from_pathname);
    destroy_pathname(to_pathname);

    if (!ok) return GetLastError();
    return ERROR_SUCCESS;
}


DWORD
FsDT_Win32::mkdir(
    fhandle_t dir, 
    const char* name, 
    fattr_t* attr
    )
{
    DFN(FsDT_Win32::mkdir);
    AutoLock lock(&csObject);
    // XXX: we ignore attr for now...
    if (invalid_handle(dir) || !name) return ERROR_INVALID_PARAMETER;

    char* basename = handles[dir].pathname;
    char* pathname = construct_pathname(basename, name);

    DEBUG_PRINT(("mkdir on %s\n", pathname));
    BOOL ok = CreateDirectory(pathname, 0);
    destroy_pathname(pathname);
    if (!ok) return GetLastError();
    return ERROR_SUCCESS;
}


DWORD
FsDT_Win32::rmdir(
    fhandle_t dir, 
    const char* name
    )
{
    DFN(FsDT_Win32::rmdir);
    AutoLock lock(&csObject);
    if (invalid_handle(dir) || !name) return ERROR_INVALID_PARAMETER;

    char* basename = handles[dir].pathname;
    char* pathname = construct_pathname(basename, name);

    DEBUG_PRINT(("rmdir on %s\n", pathname));
    BOOL ok = RemoveDirectory(pathname);
    destroy_pathname(pathname);
    if (!ok) return GetLastError();
    return ERROR_SUCCESS;
}

DWORD
FsDT_Win32::link(
    fhandle_t dir, 
    const char* name, 
    fhandle_t handle
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD 
FsDT_Win32::ioctl(
    fhandle_t handle, 
    UINT32 code, 
    void* in_buffer, 
    UINT32* out_size, 
    void* out_buffer
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
FsDT_Win32::flush(
    fhandle_t handle
    )
{
    AutoLock lock(&csObject);
    if (handle == INVALID_FHANDLE_T)
        // XXX - flush the entire file system...we don't currently support that
        return ERROR_CALL_NOT_IMPLEMENTED;

    if (invalid_handle(handle)) return ERROR_INVALID_PARAMETER;

    if (!FlushFileBuffers(handles[handle].h))
        return GetLastError();
    else
        return ERROR_SUCCESS;
}

//
// constructor/destructor:
//


FsDT_Win32::FsDT_Win32(
    const char* _principal,
    FileSystem* _pFs // caller must AddRef beforehand
    ):
    handles(new handle_info_t[config.max_handles]),
    handles_used(0), ref_count(1), pFs(_pFs)
{
    DFN(FsDT_Win32::FsDT_Win32);
    InitializeCriticalSection(&csObject);
    if (_principal) {
        principal = new char[lstrlen(_principal) + 1];
    } else {
        principal = 0;
    }
    lstrcpy(principal, _principal);
    ULONG val = InterlockedIncrement((LONG*)&global_count);
    DEBUG_PRINT(("global_count = %u\n", val));
}

FsDT_Win32::~FsDT_Win32(
    )
{
    DFN(FsDT_Win32::~FsDT_Win32);
    for(int i = 0; handles_used && (i < config.max_handles); i++) {
        close2((fhandle_t)i);
    }
    delete [] principal;
    delete [] handles;
    pFs->Release();
    DeleteCriticalSection(&csObject);
    ULONG val = InterlockedDecrement((LONG*)&global_count);
    DEBUG_PRINT(("global_count = %u\n", val));
}


//
// helpers:
//


DWORD
FsDT_Win32::init(
    )
{
    DFN(FsDT_Win32::init);
    fhandle_t f;
    DWORD error = create2(config.root, lstrlen(config.root) + 1, DISP_DIRECTORY | SHARE_READ | SHARE_WRITE, 0, &f);
    if (error) {
        DEBUG_PRINT(("ERROR: 0x%08x\n", error));
        return error;
    };
    if (f != 0) return ERROR_INVALID_PARAMETER;
    return ERROR_SUCCESS;
}


DWORD
FsDT_Win32::create2(
    const char* fullname,
    size_t fullname_size, // including NULL byte...
    UINT32 flags, 
    fattr_t* fattr, 
    fhandle_t* phandle
    )
{
    DFN(FsDT_Win32::create2);
    DEBUG_PRINT(("FsDT::create2(%s, %d, 0x%08X, 0x%08X, 0x%08X)\n",
                 fullname, fullname_size, flags, fattr, phandle));

    handle_info_t hinfo;
    hinfo.dc = 0;
    hinfo.h = INVALID_HANDLE_VALUE;
    hinfo.pathname = new char[fullname_size];
    RtlCopyMemory(hinfo.pathname, fullname, fullname_size);
    DWORD error = ERROR_SUCCESS;

    fhandle_t h;

    switch (flags & DISP_MASK) {
    case DISP_DIRECTORY:
        hinfo.dc = new dir_cache(hinfo.pathname, hinfo.h);
        if (!hinfo.dc) {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        if (!hinfo.dc->valid()) {
            error = hinfo.dc->error();
            goto cleanup;
        }
        hinfo.h = CreateFile(hinfo.pathname, 
                             GENERIC_READ, 
                             FILE_SHARE_READ | FILE_SHARE_WRITE, 
                             0, 
                             OPEN_EXISTING, 
                             FILE_FLAG_BACKUP_SEMANTICS,
                             0);
        break;
    case DISP_CREATE_NEW:
    case DISP_CREATE_ALWAYS:
    case DISP_OPEN_EXISTING:
    case DISP_OPEN_ALWAYS:
    case DISP_TRUNCATE_EXISTING:

        if (fattr && (fattr->attributes & (ATTR_SYMLINK | ATTR_DIRECTORY | 
                                           ATTR_COMPRESSED | ATTR_OFFLINE))) {
            error = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }

        hinfo.h = CreateFile(hinfo.pathname,
                             unget_access(flags),
                             unget_share(flags),
                             NULL,
                             unget_disp(flags),
                             fattr?unget_attributes(fattr->attributes):0,
                             NULL);
        break;
    default:
        error = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (hinfo.h == INVALID_HANDLE_VALUE) {
        error = GetLastError();
        goto cleanup;
    }
    h = get_new_handle();
    if (h == INVALID_FHANDLE_T) {
        error = ERROR_TOO_MANY_OPEN_FILES;
        goto cleanup;
    }
    handles[h] = hinfo;
    handles_used++;
    *phandle = h;
    return ERROR_SUCCESS;

cleanup:
    if (hinfo.dc)
        delete hinfo.dc;
    if (hinfo.h != INVALID_HANDLE_VALUE)
        CloseHandle(hinfo.h);
    if (hinfo.pathname)
        delete [] hinfo.pathname;
    return error;
}


DWORD
FsDT_Win32::close2(
    fhandle_t handle
    )
{
    if (invalid_handle(handle))
        return ERROR_INVALID_PARAMETER;

    DWORD error = ERROR_SUCCESS;

    if (!CloseHandle(handles[handle].h)) error = GetLastError();
    handles[handle].h = INVALID_HANDLE_VALUE;
    delete [] handles[handle].pathname;
    delete handles[handle].dc;
    handles[handle].pathname = 0;
    handles[handle].dc = 0;
    handles_used--;

    return error;
}

//
// handle managagement:
//

inline
fhandle_t
FsDT_Win32::get_new_handle()
{
    for (fhandle_t i = 0; i < config.max_handles; i++)
        if (handles[i].h == INVALID_HANDLE_VALUE) return i;
    return INVALID_FHANDLE_T;
}

inline
bool
FsDT_Win32::invalid_handle(
    fhandle_t handle
    )
{
    return ((handle >= config.max_handles) || 
            (handles[handle].h == INVALID_HANDLE_VALUE));
}

inline
bool
FsDT_Win32::invalid_dir_handle(
    fhandle_t handle
    )
{
    return invalid_handle(handle) || !handles[handle].dc;
}

inline
bool
FsDT_Win32::invalid_file_handle(
    fhandle_t handle
    )
{
    return invalid_handle(handle) || handles[handle].dc;
}

//
// static helpers:
//

inline
TIME64
FsDT_Win32::get_time(
    FILETIME ft
    )
{
    return *((TIME64*)&ft);
}

inline
FILETIME
FsDT_Win32::unget_time(
    TIME64 t
    )
{
    return *((LPFILETIME)&t);
}

inline
UINT32
FsDT_Win32::get_attributes(
    DWORD a
    )
{
    UINT32 attr = 0;
    if (a & FILE_ATTRIBUTE_READONLY) attr |= ATTR_READONLY;
    if (a & FILE_ATTRIBUTE_HIDDEN)   attr |= ATTR_HIDDEN;
    if (a & FILE_ATTRIBUTE_SYSTEM)   attr |= ATTR_SYSTEM;
    if (a & FILE_ATTRIBUTE_ARCHIVE)  attr |= ATTR_ARCHIVE;
    if (a & FILE_ATTRIBUTE_DIRECTORY) attr |= ATTR_DIRECTORY;
    return attr;
}

inline
DWORD
FsDT_Win32::unget_attributes(
    UINT32 attr
    )
{
    DWORD a = 0;
    if (attr & ATTR_READONLY)  a |= FILE_ATTRIBUTE_READONLY;
    if (attr & ATTR_HIDDEN)    a |= FILE_ATTRIBUTE_HIDDEN;
    if (attr & ATTR_SYSTEM)    a |= FILE_ATTRIBUTE_SYSTEM;
    if (attr & ATTR_ARCHIVE)   a |= FILE_ATTRIBUTE_ARCHIVE;
    if (attr & ATTR_DIRECTORY) a |= FILE_ATTRIBUTE_DIRECTORY;
    return a;
}

inline
char*
FsDT_Win32::construct_pathname(
    const char* basename,
    const char* name
    )
{
    char* pathname = new char[lstrlen(basename)+lstrlen(name)+2];
    lstrcpy(pathname, basename);
    lstrcat(pathname, "\\");
    lstrcat(pathname, name);
    return pathname;
}

inline
void
FsDT_Win32::destroy_pathname(
    char* pathname
    )
{
    delete [] pathname;
}

inline
DWORD
FsDT_Win32::unget_disp(
    UINT32 flags
    )
{
    switch (flags & DISP_MASK) {
    case DISP_CREATE_NEW:        return CREATE_NEW;
    case DISP_CREATE_ALWAYS:     return CREATE_ALWAYS;
    case DISP_OPEN_EXISTING:     return OPEN_EXISTING;
    case DISP_OPEN_ALWAYS:       return OPEN_ALWAYS;
    case DISP_TRUNCATE_EXISTING: return TRUNCATE_EXISTING;
    default: return 0;
    }
}

inline
DWORD
FsDT_Win32::unget_access(
    UINT32 flags
    )
{
    DWORD win32_access = 0;
    if (flags & ACCESS_READ)  win32_access |= GENERIC_READ;
    if (flags & ACCESS_WRITE) win32_access |= GENERIC_WRITE;
    return win32_access;
}

inline
DWORD
FsDT_Win32::unget_share(
    UINT32 flags
    )
{
    DWORD win32_share = 0;
    if (flags & SHARE_READ)  win32_share |= FILE_SHARE_READ;
    if (flags & SHARE_WRITE) win32_share |= FILE_SHARE_WRITE;
    return win32_share;
}
