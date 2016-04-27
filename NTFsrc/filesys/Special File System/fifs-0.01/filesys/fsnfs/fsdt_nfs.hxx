/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   fsnfs file system dispatch table object header

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

#ifndef __FSDT_NFS_HXX__
#define __FSDT_NFS_HXX__

#include "fs_nfs.hxx"
#include <rpc/rpc.h>
#include "mount_prot.h"
#include "nfs_prot.h"

///////////////////////////////////////////////////////////////////////////////
//
// Defines filesystem dispatch table
//
///////////////////////////////////////////////////////////////////////////////

#define NUM_ERR 100

class FsDT_NFS: public FsDispatchTable {
public:
    // FsDispatchTable:
    ULONG AddRef();
    ULONG Release();
    const char* get_principal();
    fhandle_t get_root();
    DWORD create(fhandle_t dir, const char* name, UINT32 flags, fattr_t* attr, fhandle_t* handle);
    DWORD set_attr(fhandle_t handle, fattr_t* attr);
    DWORD lookup(fhandle_t dir, const char* name, fattr_t* attr);
    DWORD get_attr(fhandle_t handle, fattr_t* attr);
    DWORD close(fhandle_t handle);
    DWORD write(fhandle_t handle, UINT64 offset, UINT64* count, void* buffer);
    DWORD read(fhandle_t handle,  UINT64 offset, UINT64* count, void* buffer);
    DWORD readlink(fhandle_t handle, int* size, char* path_buffer);
    DWORD symlink(fhandle_t dir, const char* name, const char* path);
    DWORD read_dir(fhandle_t dir, UINT32 cookie, dirinfo_t* buffer, UINT32 size, UINT32 *entries_found);
    DWORD statfs(fhandle_t handle, fs_attr_t* attr);
    DWORD remove(fhandle_t dir, const char* name);
    DWORD rename(fhandle_t fromdir, const char* fromname, fhandle_t todir, const char* toname);
    DWORD mkdir(fhandle_t dir, const char* name, fattr_t* attr);
    DWORD rmdir(fhandle_t dir, const char* name);
    DWORD link(fhandle_t dir, const char* name, fhandle_t handle);
    DWORD ioctl(fhandle_t handle, UINT32 code, void* in_buffer, 
                UINT32* out_size, void* out_buffer);
    DWORD flush(fhandle_t handle);

private:
    FsDT_NFS(const char* _principal, FileSystem* _pFs); // caller must AddRef
    ~FsDT_NFS();

    // helpers:
    DWORD init();
    DWORD close2(fhandle_t handle);
    inline fhandle_t get_new_handle();
    inline fhandle_t dup_handle(fhandle_t h);
    inline bool invalid_handle(fhandle_t handle);
    inline bool invalid_dir_handle(fhandle_t handle);
    inline bool invalid_file_handle(fhandle_t handle);

    DWORD mount(
        char* localmachine,
        int uid,
        int gid,
        int len,
        int gids[]
        );
    void unmount();

    inline DWORD nfslookup(
        nfs_fh* pdir,
        const char* name,
        nfs_fh* pfh,
        fattr* pfa
        );
    inline DWORD nfscreate(
        nfs_fh* pdir,
        const char* name,
        sattr* psa,
        nfs_fh* pfh,
        fattr* pfa
        );
    inline DWORD nfssetattr(
        nfs_fh* pfh,
        sattr* psa
        );
    inline DWORD nfsgetattr(
        nfs_fh* pfh,
        fattr* pfa
        );
    inline DWORD nfswrite(
        nfs_fh* pfh,
        u_int offset,
        u_int count,
        void* data,
        fattr* pfa
        );
    inline DWORD nfsread(
        nfs_fh* pfh,
        u_int offset,
        u_int count,
        void* data,
        u_int* pcount,
        fattr* pfa
        );
    inline DWORD nfsstatfs(
        nfs_fh* pfh,
        u_int* tsize,
        u_int* bsize,
        u_int* blocks,
        u_int* bfree,
        u_int* bavail
        );
    inline DWORD nfsremove(
        nfs_fh* pdir,
        const char* name
        );
    inline DWORD nfsrename(
        nfs_fh* pdir_from,
        const char* name_from,
        nfs_fh* pdir_to,
        const char* name_to
        );
    inline DWORD nfsmkdir(
        nfs_fh* pdir,
        const char* name,
        sattr* psa,
        nfs_fh* pfh,
        fattr* pfa
        );
    inline DWORD nfsrmdir(
        nfs_fh* pdir,
        const char* name
        );

    // global:
    static ULONG global_count;
    static DWORD errxlat[NUM_ERR];
    static TIME64 time_ufactor;
    static TIME64 time_factor;
    static TIME64 time_offset;
    static bool global_initialized;

    static void global_init();

    inline static DWORD create_auth_clnt(
        char* localmachine,
        int uid,
        int gid,
        int len,
        int gids[],
        char* machine,
        int prog,
        int version,
        char* protocol,
        CLIENT*& cl
        );
    inline static DWORD nfserr_to_win32(nfsstat status) {
        return ((status >= 0) && (status < NUM_ERR))?errxlat[status]:ERROR_UNEXP_NET_ERR;
    }
    inline static TIME64 get_time(nfstime& nt);
    inline static nfstime unget_time(TIME64 t);
    inline static UINT32 get_attributes(u_int mode, ftype ft);
    inline static u_int unget_attributes(UINT32 attr);

    // types:
    struct handle_info_t {
        bool used;
        bool dir;
        nfs_fh fh;
        bool allow_write;
        bool allow_read;
        handle_info_t():
            used(false),dir(false),allow_read(false),allow_write(false) {}
    };

    // data:
    ULONG ref_count;
    FileSystem* pFs;
    char* principal;
    CLIENT* clmnt;
    CLIENT* clnfs;
    int uid;
    int gid;
    int len;
    int gids[NGRPS];

    handle_info_t* handles;
    int handles_used;
    int max_handles;
    u_int tsize;

    CRITICAL_SECTION csObject;

    // friends:
    friend class Fs_NFS;
};

#endif /* __FSDT_NFS_HXX__ */
