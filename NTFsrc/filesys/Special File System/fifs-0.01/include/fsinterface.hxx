/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   File Sysstem Interface header

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

#ifndef __FS_INTERFACE_HXX__
#define __FS_INTERFACE_HXX__

// note: we assume DWORD and DWORDDLONG are defined (from windows.h)

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef _BASETSD_H_
typedef DWORDLONG UINT64;    // a 64-bit unsigned value
typedef DWORD     UINT32;    // a 32-bit unsigned value
#endif

// disposition:
namespace FsInterface {

    typedef UINT64    TIME64;    // in units of 100ns since Jan 1, 1601 (AD)
    typedef UINT64    fhandle_t; // a file handle -- a bit wide

    const UINT64    INVALID_UINT64    = ((UINT64)(-1));
    const UINT32    INVALID_UINT32    = ((UINT32)(-1));
    const TIME64    INVALID_TIME64    = INVALID_UINT64;
    const fhandle_t INVALID_FHANDLE_T = ((fhandle_t)(-1));

    // disposition:
    const UINT32 DISP_CREATE_NEW        = 0x10000000;
    const UINT32 DISP_CREATE_ALWAYS     = 0x20000000;
    const UINT32 DISP_OPEN_EXISTING     = 0x30000000;
    const UINT32 DISP_OPEN_ALWAYS       = 0x40000000;
    const UINT32 DISP_TRUNCATE_EXISTING = 0x50000000;
    const UINT32 DISP_DIRECTORY         = 0x60000000;
    const UINT32 DISP_MASK              = 0x70000000;

    // access:
    const UINT32 ACCESS_READ         = 0x00010000;
    const UINT32 ACCESS_WRITE        = 0x00020000;
    const UINT32 ACCESS_MASK         = 0x00030000;

    // sharing:
    const UINT32 SHARE_READ          = 0x00100000;
    const UINT32 SHARE_WRITE         = 0x00200000;
    const UINT32 SHARE_MASK          = 0x00300000;

    // flags = dispositions | access | sharing
    const UINT32 FLAGS_MASK = DISP_MASK | ACCESS_MASK | SHARE_MASK;

    // attributes:
    const UINT32 ATTR_SYMLINK        = 0x00002000;
    const UINT32 ATTR_DIRECTORY      = 0x00000010;
    const UINT32 ATTR_READONLY       = 0x00000001;
    const UINT32 ATTR_HIDDEN         = 0x00000002;
    const UINT32 ATTR_SYSTEM         = 0x00000004;
    const UINT32 ATTR_ARCHIVE        = 0x00000020;
    const UINT32 ATTR_COMPRESSED     = 0x00000800;
    const UINT32 ATTR_OFFLINE        = 0x00001000;
    const UINT32 ATTR_MASK           = (ATTR_SYMLINK  | ATTR_DIRECTORY | 
                                        ATTR_READONLY | ATTR_HIDDEN    | 
                                        ATTR_SYSTEM   | ATTR_ARCHIVE   | 
                                        ATTR_COMPRESSED | ATTR_OFFLINE
                                        ); // 0x00003837;

    const size_t MAX_FS_NAME_LEN = 64; // somewhat arbitrary...

    // file system attributes
    struct fs_attr_t {
        char fs_name[MAX_FS_NAME_LEN];
        UINT64 total_bytes;
        UINT64 free_bytes;
    };

    // file/dir attributes
    // - we do not use gid/uid because SMB does not deal
    // - passing in INVALID_xxx (-1) for any of the values below makes it 
    //   unspecified.

    struct fattr_t {
        // sizes
        UINT64 file_size;
        UINT64 alloc_size;
        // times
        TIME64 create_time;
        TIME64 access_time;
        TIME64 mod_time;
        // mode/attr
        // - a la Win32, but wider so that we can pass in -1 safely
        UINT32 attributes;
    };

    const size_t MAX_NAME_LENGTH = 256;

    typedef struct {
        char name[MAX_NAME_LENGTH];
        struct fattr_t attribs;
        UINT32 cookie;
    } dirinfo_t;

    class FsDispatchTable {
    public:
        virtual ULONG AddRef()  = 0;   // returns new reference count
        virtual ULONG Release() = 0;   // returns new reference count

        // returns string for principal owning this interface
        virtual const char* get_principal() = 0;

        // returns immutable root handle, which can only be used by create()
        virtual fhandle_t   get_root()      = 0;

        // crete() can take an empty string (a NULL byte) as its name
        // argument to signify opening the given dir again.
        virtual DWORD create(
            IN     fhandle_t   dir, 
            IN     const char* name, 
            IN     UINT32      flags, 
            IN     fattr_t*    attr, 
            OUT    fhandle_t*  handle
            ) = 0;
        virtual DWORD lookup(
            IN     fhandle_t   dir, 
            IN     const char* name, 
            OUT    fattr_t*    attr
            ) = 0;
        virtual DWORD set_attr(
            IN     fhandle_t   handle,
            IN     fattr_t*    attr
            ) = 0;
        virtual DWORD get_attr(
            IN     fhandle_t   handle, 
            OUT    fattr_t*    attr
            ) = 0;
        virtual DWORD close(
            IN     fhandle_t   handle
            ) = 0;
        virtual DWORD write(
            IN     fhandle_t   handle, 
            IN     UINT64      offset, 
            IN OUT UINT64*     count, 
            IN     void*       buffer
            ) = 0;
        virtual DWORD read(
            IN     fhandle_t   handle, 
            IN     UINT64      offset, 
            IN OUT UINT64*     count, 
            OUT    void*       buffer
            ) = 0;

        // to use readir(), we do a create() with DISP_DIRECTORY
        // returns ERROR_NO_MORE_FILES when done...
        virtual DWORD read_dir(
            IN     fhandle_t   dir, 
            IN     UINT32      cookie, 
            OUT    dirinfo_t*  buffer, 
            IN     UINT32      size, 
            OUT    UINT32*     entries_found
            ) = 0;

        virtual DWORD statfs(
            IN     fhandle_t   handle, 
            OUT    fs_attr_t*  attr
            ) = 0;
        virtual DWORD remove(
            IN     fhandle_t   dir, 
            IN     const char* name
            ) = 0;
        virtual DWORD rename(
            IN     fhandle_t   fromdir, 
            IN     const char* fromname, 
            IN     fhandle_t   todir,
            IN     const char* toname
            ) = 0;
        virtual DWORD mkdir(
            IN     fhandle_t   dir, 
            IN     const char* name, 
            IN     fattr_t*    attr
            ) = 0;
        virtual DWORD rmdir(
            IN     fhandle_t   dir,
            IN     const char* name
            ) = 0;

        virtual DWORD readlink(
            IN     fhandle_t   handle, 
            IN OUT int*        size,
            OUT    char*       path_buffer
            ) = 0;
        virtual DWORD symlink(
            IN     fhandle_t   dir, 
            IN     const char* name, 
            IN     const char* path
            ) = 0;
        virtual DWORD link(
            IN     fhandle_t   dir,
            IN     const char* name,
            IN     fhandle_t   handle
            ) = 0;
        virtual DWORD ioctl(
            IN     fhandle_t   handle,
            IN     UINT32      code,
            IN     void*       in_buffer,
            IN OUT UINT32*     out_size,
            OUT    void*       out_buffer
            ) = 0;
        virtual DWORD flush(
            IN     fhandle_t   handle
            ) = 0;
    };


    class FileSystem {
    public:
        virtual ULONG AddRef()  = 0;   // returns new reference count
        virtual ULONG Release() = 0;   // returns new reference count

        virtual DWORD connect(
            IN     const char* principal, 
            OUT    FsDispatchTable** ppDT
            ) = 0;
    };

    typedef DWORD (WINAPI *FS_CREATE_PROC)(
        IN     const char*, // fs_name
        IN     const char*, // fs_config_path
        IN     DWORD,       // interface_version
        OUT    FileSystem** // interface pointer 
                            // (i.e., FileSystem** if FS_VERSION_0)
        );

    const char FS_CREATE_PROC_NAME[] = "FileSystemCreate";

    const DWORD FS_VERSION_0 = 0;
}

#endif /* __FS_INTERFACE_HXX__ */
