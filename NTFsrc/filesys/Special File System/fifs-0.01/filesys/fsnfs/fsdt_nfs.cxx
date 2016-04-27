/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   fsnfs file system dispatch table object

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

#include "fsdt_nfs.hxx"
#include "conf_nfs.hxx"
#include <autolock.hxx>
#include <debug.hxx>

///////////////////////////////////////////////////////////////////////////////
//
// Implements filesystem dispatch table
//
///////////////////////////////////////////////////////////////////////////////

// #if (NFS_COOKIESIZE != sizeof(UINT32))
// #error "NFS_COOKIESIZE should be equal to sizeof(UINT32)"
// #endif

ULONG FsDT_NFS::global_count = 0;
bool FsDT_NFS::global_initialized = false;
TIME64 FsDT_NFS::time_offset = 0;
TIME64 FsDT_NFS::time_ufactor = 10;
TIME64 FsDT_NFS::time_factor = time_ufactor*1000*1000;
DWORD FsDT_NFS::errxlat[NUM_ERR] = {0};

void
FsDT_NFS::global_init()
{
    DFN(FsDT_NFS::global_init);
    global_initialized = true;
    for (int i = 0; i < NUM_ERR; i++) errxlat[i] = ERROR_UNEXP_NET_ERR;
    errxlat[NFS_OK] = ERROR_SUCCESS;
    errxlat[NFSERR_PERM] = ERROR_INVALID_FUNCTION; // XXX - is this right?
    errxlat[NFSERR_NOENT] = ERROR_FILE_NOT_FOUND;
    errxlat[NFSERR_IO] = ERROR_GEN_FAILURE; // ERROR_IO_DEVICE
    errxlat[NFSERR_NXIO] = ERROR_BAD_UNIT; // ERROR_BAD_DEVICE
    errxlat[NFSERR_ACCES] = ERROR_ACCESS_DENIED;
    errxlat[NFSERR_EXIST] = ERROR_FILE_EXISTS;
    errxlat[NFSERR_NODEV] = ERROR_BAD_UNIT; // ERROR_BAD_DEVICE
    errxlat[NFSERR_NOTDIR] = ERROR_PATH_NOT_FOUND;
    errxlat[NFSERR_ISDIR] = ERROR_FILE_NOT_FOUND;
// errxlat[NFSERR_FBIG] =
    errxlat[NFSERR_NOSPC] = ERROR_DISK_FULL;
    errxlat[NFSERR_ROFS] = ERROR_WRITE_PROTECT;
    errxlat[NFSERR_NAMETOOLONG] = ERROR_INVALID_NAME;
    errxlat[NFSERR_NOTEMPTY] = ERROR_DIR_NOT_EMPTY;
    errxlat[NFSERR_DQUOT] = ERROR_NOT_ENOUGH_QUOTA;
// errxlat[NFSERR_STALE] = 
// errxlat[NFSERR_WFLUSH] = 

    SYSTEMTIME st;
    st.wYear = 1970;
    st.wMonth = 1;
    st.wDay = 1;
    st.wHour = 0;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;
    if (!SystemTimeToFileTime(&st, (LPFILETIME)&time_offset)) {
        DEBUG_PRINT(("CANNOT CONVERT TIME!\n"));
    }
}

ULONG
FsDT_NFS::AddRef(
    )
{
    AutoLock lock(&csObject);
    ULONG count = ref_count;
    ref_count++;
    return count;
}


ULONG
FsDT_NFS::Release(
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
FsDT_NFS::get_principal()
{
    AutoLock lock(&csObject);
    return principal;
}

fhandle_t
FsDT_NFS::get_root(
    )
{
    return 0;
}

DWORD
FsDT_NFS::create(
    fhandle_t dir, 
    const char* name, 
    UINT32 flags, 
    fattr_t* attr, 
    fhandle_t* phandle
    )
{
    DFN(FsDT_NFS::create);
    AutoLock lock(&csObject);
    DEBUG_PRINT(("FsDT::create(0x%016I64X, %s, 0x%08X, 0x%08X, 0x%08X)\n",
                 dir, name, flags, attr, phandle));

    if (!phandle) return ERROR_INVALID_PARAMETER;
    *phandle = INVALID_FHANDLE_T;

    if (invalid_dir_handle(dir)) return ERROR_INVALID_PARAMETER;
    if (!name) return ERROR_INVALID_PARAMETER;
    if (flags != (FLAGS_MASK & flags)) {
        return ERROR_INVALID_PARAMETER;
    }

    // allow duping of directory handles -- XXX
    if (!*name && (flags & DISP_DIRECTORY)) {
        *phandle = dup_handle(dir);
        return ERROR_SUCCESS;
    }

    // XXX - should set mode bits bases on attr->attributes
    // XXX - in general, should not ignore attr...

    nfs_fh* pdir = &handles[dir].fh;
    handle_info_t hinfo;
    hinfo.dir = false;
    sattr csa; // for create
    sattr tsa; // for truncate
    fattr fa;

    // prepare in case we need to create...
    TIME64 t;
    GetSystemTimeAsFileTime((LPFILETIME)&t);

    csa.mode = 0755;
    csa.uid = uid;
    csa.gid = gid;
    csa.size = 0;
    csa.atime = unget_time(t);
    csa.mtime = unget_time(t);

    // prepare in case we need to truncate...
    tsa.mode = -1;
    tsa.uid = -1;
    tsa.gid = -1;
    tsa.size = 0;
    tsa.atime = unget_time(INVALID_TIME64);
    tsa.mtime = unget_time(INVALID_TIME64);

    DWORD error = ERROR_SUCCESS;
    UINT32 disp = flags & DISP_MASK;

    switch (disp) {
    case DISP_DIRECTORY:
        error = nfslookup(pdir, name, &hinfo.fh, &fa);
        if (error) return error;
        if (fa.type != NFDIR) return ERROR_PATH_NOT_FOUND;
        hinfo.dir = true;
        break;
    case DISP_OPEN_EXISTING:
        error = nfslookup(pdir, name, &hinfo.fh, &fa);
        if (error) return error;
        if (fa.type == NFDIR) return ERROR_FILE_NOT_FOUND;
        break;
    case DISP_TRUNCATE_EXISTING:
        error = nfslookup(pdir, name, &hinfo.fh, &fa);
        if (error) return error;
        if (fa.type == NFDIR) return ERROR_FILE_NOT_FOUND;
        error = nfssetattr(&hinfo.fh, &tsa);
        if (error) return error;
        break;
    case DISP_OPEN_ALWAYS:
        error = nfslookup(pdir, name, &hinfo.fh, &fa);
        if (error) {
            error = nfscreate(pdir, name, &csa, &hinfo.fh, 0);
            if (error) return error;
        } else {
            if (fa.type == NFDIR) return ERROR_FILE_NOT_FOUND;
        }
        break;
    case DISP_CREATE_NEW:
        error = nfscreate(pdir, name, &csa, &hinfo.fh, 0);
        if (error) return error;
        break;
    case DISP_CREATE_ALWAYS:
        error = nfscreate(pdir, name, &csa, &hinfo.fh, 0);
        if (error) {
            error = nfslookup(pdir, name, &hinfo.fh, &fa);
            if (error) return error;
            if (fa.type == NFDIR) return ERROR_FILE_NOT_FOUND;
            error = nfssetattr(&hinfo.fh, &tsa);
            if (error) return error;
        }
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    hinfo.used = true;
    UINT32 access = flags & FsInterface::ACCESS_MASK;
    hinfo.allow_write = (access & ACCESS_WRITE) != 0;
    hinfo.allow_read = (access & ACCESS_READ) != 0;

    fhandle_t h = get_new_handle();
    if (h == INVALID_FHANDLE_T) {
        return ERROR_TOO_MANY_OPEN_FILES;
    }
    handles[h] = hinfo;
    handles_used++;
    *phandle = h;
    return ERROR_SUCCESS;
}


DWORD
FsDT_NFS::set_attr(
    fhandle_t handle,
    fattr_t* attr
    )
{
    DFN(FsDT_NFS::set_attr);
    AutoLock lock(&csObject);
    if (invalid_handle(handle) || !attr) return ERROR_INVALID_PARAMETER;

    sattr sa;

    sa.mode = unget_attributes(attr->attributes);
    sa.uid = -1;
    sa.gid = -1;
    sa.size = attr->file_size;
    sa.atime = unget_time(attr->access_time);
    sa.mtime = unget_time(attr->mod_time);

    return nfssetattr(&handles[handle].fh, &sa);
}


DWORD
FsDT_NFS::lookup(
    fhandle_t dir,
    const char* name,
    fattr_t* attr
    )
{
    DFN(FsDT_NFS::lookup);
    AutoLock lock(&csObject);
    if (invalid_dir_handle(dir) || !attr) return ERROR_INVALID_PARAMETER;

    fattr fa;
    DWORD error = nfslookup(&handles[dir].fh, name, 0, &fa);
    if (error) return error;

    if (fa.type == NFDIR) fa.size = 0;
    attr->file_size = fa.size;
    attr->alloc_size = fa.size; // fa.blocksize;
    attr->access_time = get_time(fa.atime);
    attr->create_time = get_time(fa.ctime);
    attr->mod_time = get_time(fa.mtime);
    attr->attributes = get_attributes(fa.mode, fa.type);
    return ERROR_SUCCESS;
}

DWORD
FsDT_NFS::get_attr(
    fhandle_t handle, 
    fattr_t* attr
    )
{
    DFN(FsDT_NFS::get_attr);
    AutoLock lock(&csObject);
    if (invalid_handle(handle) || !attr) return ERROR_INVALID_PARAMETER;

    fattr fa;
    DWORD error = nfsgetattr(&handles[handle].fh, &fa);
    if (error) return error;

    if (fa.type == NFDIR) fa.size = 0;
    attr->file_size = fa.size;
    attr->alloc_size = fa.size; // fa.blocksize;
    attr->access_time = get_time(fa.atime);
    attr->create_time = get_time(fa.ctime);
    attr->mod_time = get_time(fa.mtime);
    attr->attributes = get_attributes(fa.mode, fa.type);
    return ERROR_SUCCESS;
}


DWORD
FsDT_NFS::close(
    fhandle_t handle
    )
{
    AutoLock lock(&csObject);    
    if (!handle) return ERROR_INVALID_PARAMETER;
    return close2(handle);
}

DWORD
FsDT_NFS::write(
    fhandle_t handle, 
    UINT64 offset, 
    UINT64* pcount, 
    void* buffer
    )
{
    DFN(FsDT_NFS::write);
    AutoLock lock(&csObject);
    if (invalid_file_handle(handle) || !pcount) return ERROR_INVALID_PARAMETER;

    DEBUG_PRINT(("GOAL -> offset: %I64d, count: %I64d\n", offset, *pcount));

    DWORD error = ERROR_SUCCESS;
    UINT64 left = *pcount;
    char* data = (char*)buffer;

    u_int count;
    while (!error && left) {
        count = min(min(left, tsize), NFS_MAXDATA);
        DEBUG_PRINT(("LOOP -> offset: %I64d, count: %d\n", offset, count));
        error = nfswrite(&handles[handle].fh, offset, count, data, 0);
        offset += count;
        data += count;
        left -= count;
    }
    return error;
}


DWORD
FsDT_NFS::read(
    fhandle_t handle, 
    UINT64 offset, 
    UINT64* pcount, 
    void* buffer
    )
{
    DFN(FsDT_NFS::read);
    AutoLock lock(&csObject);
    if (invalid_file_handle(handle) || !pcount) return ERROR_INVALID_PARAMETER;

    DEBUG_PRINT(("GOAL -> offset: %I64d, count: %I64d\n", offset, *pcount));

    DWORD error = ERROR_SUCCESS;
    UINT64 left = *pcount;
    char* data = (char*)buffer;
    UINT64 total = 0;

    u_int count;
    u_int actual;
    while (!error && left) {
        count = min(min(left, tsize), NFS_MAXDATA);
        actual = 0;
        DEBUG_PRINT(("LOOP -> offset: %I64d, count: %d\n", offset, count));
        error = nfsread(&handles[handle].fh, offset, count, data, &actual, 0);
        total += actual;
        if (actual != count) break;
        offset += count;
        data += count;
        left -= count;
    }

    *pcount = total;
    return error;
}


DWORD
FsDT_NFS::readlink(
    fhandle_t handle, 
    int* size, 
    char* path_buffer
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
FsDT_NFS::symlink(
    fhandle_t dir, 
    const char* name, 
    const char* path
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
FsDT_NFS::read_dir(
    fhandle_t dir, 
    UINT32 cookie, 
    dirinfo_t* buffer,
    UINT32 size, 
    UINT32 *entries_found
    )
{
    DFN(FsDT_NFS::read_dir);
    AutoLock lock(&csObject);
    if (!entries_found) return ERROR_INVALID_PARAMETER;
    (*entries_found) = 0;
    if (invalid_dir_handle(dir)) return ERROR_INVALID_PARAMETER;

    DEBUG_PRINT(("FsDT::read_dir(0x%016I64X, 0x%08X, ..., %d,...)\n",
                 dir, cookie, size));

    if (size < sizeof(dirinfo_t)) return ERROR_INVALID_PARAMETER;

    readdirargs ra;
    ra.dir = handles[dir].fh;
    RtlCopyMemory(ra.cookie, &cookie, NFS_COOKIESIZE);
    ra.count = min(size / sizeof(dirinfo_t) * 
                   (4*sizeof(u_int) + 16) + 2*sizeof(u_int),
                   NFS_MAXDATA);

    readdirres* res = nfsproc_readdir_2(&ra, clnfs);
    if (!res) {
        DEBUG_PRINT(("%s\n", clnt_sperror(clnfs, "NFS_READDIR")));
        return ERROR_UNEXP_NET_ERR;
    }

    DWORD error = ERROR_SUCCESS;

    if(res->status) {
        DEBUG_PRINT(("STATUS: %d\n", res->status));
        error = nfserr_to_win32(res->status);
        if (!clnt_freeres(clnfs, (xdrproc_t)xdr_readdirres, (caddr_t)res)) {
            DEBUG_PRINT(("could not free XDR\n"));
        }
        return error;
    }

    entry* e = res->readdirres_u.reply.entries;
    bool eof = (res->readdirres_u.reply.eof != 0);

    dirinfo_t* p = buffer;

    while (e && (size >= sizeof(dirinfo_t))) {

        lstrcpyn(buffer->name, e->name, MAX_NAME_LENGTH);
        RtlCopyMemory(&buffer->cookie, e->cookie, sizeof(UINT32));
        
        (*entries_found)++;
        size -= sizeof(dirinfo_t);
        e = e->nextentry;
        DEBUG_PRINT(("got %s, 0x%08X, %d\n", 
                     buffer->name, buffer->cookie, size));
        buffer += 1;

    }

    eof = eof && !e;

    if (!clnt_freeres(clnfs, (xdrproc_t)xdr_readdirres, (caddr_t)res)) {
        DEBUG_PRINT(("could not free XDR\n"));
    }

    fattr fa;
    buffer = p;

    for(int i = 0; i < (*entries_found); i++) {
    
        error = nfslookup(&handles[dir].fh, buffer->name, 0, &fa);
        if (error) {
            DEBUG_PRINT(("got error %d on lookup\n", error));
            return error;
        }

        if (fa.type == NFDIR) fa.size = 0;
        buffer->attribs.file_size = fa.size;
        buffer->attribs.alloc_size = fa.size; // fa.blocksize;
        buffer->attribs.access_time = get_time(fa.atime);
        buffer->attribs.create_time = get_time(fa.ctime);
        buffer->attribs.mod_time = get_time(fa.mtime);
        buffer->attribs.attributes = get_attributes(fa.mode, fa.type);

        buffer += 1;
    }
    if (eof) {
        DEBUG_PRINT(("EOF\n"));
        return ERROR_NO_MORE_FILES;
    } else {
        DEBUG_PRINT(("MORE TO GO\n"));
        return ERROR_SUCCESS;
    }
}


DWORD
FsDT_NFS::statfs(
    fhandle_t handle, 
    fs_attr_t* attr
    )
{
    AutoLock lock(&csObject);
    if (!attr) return ERROR_INVALID_PARAMETER;
    if (invalid_handle(handle)) return ERROR_INVALID_PARAMETER;

//    u_int tsize;
    u_int bsize;
    u_int blocks;
//    u_int bfree;
    u_int bavail;

    DWORD error = nfsstatfs(&handles[handle].fh, 
                            0, &bsize, &blocks, 0, &bavail);
    if (error) return error;
                       
    lstrcpyn(attr->fs_name, config.label, MAX_FS_NAME_LEN);
    attr->total_bytes = bsize * blocks;
    attr->free_bytes  = bsize * bavail;
    return ERROR_SUCCESS;
}


DWORD
FsDT_NFS::remove(
    fhandle_t dir, 
    const char* name
    )
{
    DFN(FsDT_NFS::remove);
    AutoLock lock(&csObject);
    if (invalid_handle(dir) || !name) return ERROR_INVALID_PARAMETER;

    return nfsremove(&handles[dir].fh, name);
}

DWORD
FsDT_NFS::rename(
    fhandle_t from_dir, 
    const char* from_name, 
    fhandle_t to_dir, 
    const char* to_name
    )
{
    DFN(FsDT_NFS::rename);
    AutoLock lock(&csObject);
    if (invalid_handle(from_dir) || !from_name ||
        invalid_handle(to_dir) || !to_name) return ERROR_INVALID_PARAMETER;

    return nfsrename(&handles[from_dir].fh, from_name,
                     &handles[to_dir].fh, to_name);
}


DWORD
FsDT_NFS::mkdir(
    fhandle_t dir, 
    const char* name, 
    fattr_t* attr
    )
{
    DFN(FsDT_NFS::mkdir);
    AutoLock lock(&csObject);
    // XXX: we ignore attr for now...
    if (invalid_handle(dir) || !name) return ERROR_INVALID_PARAMETER;

    sattr csa;

    TIME64 t;
    GetSystemTimeAsFileTime((LPFILETIME)&t);
    csa.mode = 0755;
    csa.mode = 0755;
    csa.uid = uid;
    csa.gid = gid;
    csa.size = 0;
    csa.atime = unget_time(t);
    csa.mtime = unget_time(t);

    return nfsmkdir(&handles[dir].fh, name, &csa, 0, 0);
}


DWORD
FsDT_NFS::rmdir(
    fhandle_t dir, 
    const char* name
    )
{
    DFN(FsDT_NFS::rmdir);
    AutoLock lock(&csObject);
    if (invalid_handle(dir) || !name) return ERROR_INVALID_PARAMETER;

    return nfsrmdir(&handles[dir].fh, name);
}

DWORD
FsDT_NFS::link(
    fhandle_t dir, 
    const char* name, 
    fhandle_t handle
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD 
FsDT_NFS::ioctl(
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
FsDT_NFS::flush(
    fhandle_t handle
    )
{
    // we always do our writes, so we can just return success
    return ERROR_SUCCESS;
}


//
// constructor/destructor:
//


FsDT_NFS::FsDT_NFS(
    const char* _principal,
    FileSystem* _pFs // caller must AddRef beforehand
    ):
    max_handles(config.max_handles),
    handles(new handle_info_t[config.max_handles]),
    handles_used(1), ref_count(1), pFs(_pFs)
{
    InitializeCriticalSection(&csObject);
    if (_principal) {
        if (!*_principal) _principal = "nobody";
        principal = new char[lstrlen(_principal) + 1];
    } else {
        principal = 0;
    }
    lstrcpy(principal, _principal);
    global_count++;
    if (!global_initialized) global_init();
}

FsDT_NFS::~FsDT_NFS(
    )
{
    for(int i = 0; (handles_used > 0) && (i < max_handles); i++) {
        close((fhandle_t)i);
    }
    close2((fhandle_t)0); // close root
    delete [] principal;
    pFs->Release();
    DeleteCriticalSection(&csObject);
    global_count--;
}


//
// helpers:
//

DWORD
FsDT_NFS::create_auth_clnt(
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
    )
{
    DFN(FsDT_NFS::create_auth_clnt);

    cl = clnt_create(machine, prog, version, protocol);
    if (!cl) {
        DEBUG_PRINT(("%s\n", clnt_spcreateerror("")));
        return ERROR_UNEXP_NET_ERR;
    }
    AUTH* auth = authunix_create(localmachine, uid, gid, len, gids);
    if (!auth) {
        DEBUG_PRINT(("ERROR: could not generate mount authentication\n"));
        CLNT_DESTROY(cl);
        cl = 0;
        return ERROR_INVALID_PARAMETER;
    }
    AUTH_DESTROY(cl->cl_auth);
    cl->cl_auth = auth;
    return ERROR_SUCCESS;
}
    
DWORD
FsDT_NFS::mount(
    char* localmachine,
    int uid,
    int gid,
    int len,
    int gids[]
    )
{
    DFN(FsDT_NFS::mount);
    DWORD error;
    error = create_auth_clnt(localmachine, uid, gid, len, gids,
                             config.machine, MOUNTPROG, MOUNTVERS, 
                             config.protocol,
                             clmnt);
    if (error) {
        DEBUG_PRINT(("cannot get mount client (0x%08X)\n", error));
        return error;
    }

    fhstatus* pfhs = mountproc_mnt_1(&config.dirpath, clmnt);
    if (!pfhs) {
        DEBUG_PRINT(("%s\n", clnt_sperror(clmnt, "MNT")));
        CLNT_DESTROY(clmnt);
        clmnt = 0;
        return ERROR_UNEXP_NET_ERR;
    }
    DEBUG_PRINT(("Status: %d\n", pfhs->fhs_status));
    if (pfhs->fhs_status) {
        CLNT_DESTROY(clmnt);
        clmnt = 0;
        return nfserr_to_win32((nfsstat)pfhs->fhs_status);
    }

    error = create_auth_clnt(localmachine, uid, gid, len, gids,
                             config.machine, NFS_PROGRAM, NFS_VERSION,
                             config.protocol,
                             clnfs);
    if (error) {
        DEBUG_PRINT(("cannot get NFS client (0x%08X)\n", error));
        unmount();
        return error;
    }
    // save the handle...
    handles[0].used = true;
    handles[0].dir = true;
    RtlCopyMemory(&handles[0].fh, pfhs->fhstatus_u.fhs_fhandle, NFS_FHSIZE);
    return ERROR_SUCCESS;
}

void
FsDT_NFS::unmount(
    )
{
    if (clnfs) {
        CLNT_DESTROY(clnfs);
        clnfs = 0;
    }
    if (clmnt) {
        mountproc_umnt_1(&config.dirpath, clmnt);
        CLNT_DESTROY(clmnt);
        clmnt = 0;
    }
}

DWORD
FsDT_NFS::init(
    )
{
    DFN(FsDT_NFS::init);

    DWORD error = GetUserInfo(principal, uid, gid, len, gids, NGRPS);
    if (error) {
        DEBUG_PRINT(("ERROR: 0x%08X\n", error));
        return error;
    }
    error = mount(config.localmachine, uid, gid, len, gids);
    if (error) {
        DEBUG_PRINT(("ERROR: cannot mount (0x%08X)\n", error));
        return error;
    }
    // get the transfer size for read/write
    return nfsstatfs(&handles[0].fh, &tsize, 0, 0, 0, 0);
}


DWORD
FsDT_NFS::close2(
    fhandle_t handle
    )
{
    if (invalid_handle(handle))
        return ERROR_INVALID_PARAMETER;

    handles[handle].used = handles[handle].dir = false;
    handles_used--;

    return ERROR_SUCCESS;
}

//
// handle managagement:
//

inline
fhandle_t
FsDT_NFS::get_new_handle()
{
    for (fhandle_t i = 0; i < max_handles; i++)
        if (!handles[i].used) return i;
    return INVALID_FHANDLE_T;
}

inline
fhandle_t
FsDT_NFS::dup_handle(fhandle_t h)
{
    for (fhandle_t i = 0; i < max_handles; i++)
        if (!handles[i].used) {
            handles[i] = handles[h];
            return i;
        }
    return INVALID_FHANDLE_T;
}

inline
bool
FsDT_NFS::invalid_handle(
    fhandle_t handle
    )
{
    return ((handle > max_handles) ||
            !handles[handle].used);
}

inline
bool
FsDT_NFS::invalid_dir_handle(
    fhandle_t handle
    )
{
    return invalid_handle(handle) || !handles[handle].dir;
}

inline
bool
FsDT_NFS::invalid_file_handle(
    fhandle_t handle
    )
{
    return invalid_handle(handle) || handles[handle].dir;
}

//
// static helpers:
//

inline
TIME64
FsDT_NFS::get_time(
    nfstime& nt
    )
{
    return ((nt.seconds == -1) || (nt.useconds == -1))
        ?0
        :(time_offset + time_factor*nt.seconds + time_ufactor*nt.useconds);
}

inline
nfstime
FsDT_NFS::unget_time(
    TIME64 t
    )
{
    nfstime nt;
    if (t == INVALID_TIME64) {
        nt.seconds = -1;
        nt.useconds = -1;
    } else {
        if (t < time_offset) t = 0;
        else t -= time_offset;
        nt.seconds = t / time_factor;
        t %= time_factor;
        nt.useconds = t / time_ufactor;
    }
    return nt;
}

inline
UINT32
FsDT_NFS::get_attributes(
    u_int mode,
    ftype ft
    )
{
    // XXX - this should probably get fixed to handle mode bits
    return (ft == NFDIR)?ATTR_DIRECTORY:ATTR_ARCHIVE;
}

inline
u_int
FsDT_NFS::unget_attributes(
    UINT32 attr
    )
{
    // XXX - this should probably get fixed to yield real mode bits
    return -1;
}

DWORD
FsDT_NFS::nfslookup(
    nfs_fh* pdir,
    const char* name,
    nfs_fh* pfh,
    fattr* pfa
    )
{
    DFN(FsDT_NFS::nfslookup);
    DEBUG_PRINT(("%s\n", name));
    DWORD error = ERROR_SUCCESS;

    diropargs doa;
    doa.dir = *pdir;
    doa.name = (char*)name;

    diropres* res = nfsproc_lookup_2(&doa, clnfs);
    if (!res) {
        DEBUG_PRINT(("%s\n", clnt_sperror(clnfs, "NFS_LOOKUP")));
        return ERROR_UNEXP_NET_ERR;
    }
    if(res->status) {
        DEBUG_PRINT(("STATUS: %d\n", res->status));
        error = nfserr_to_win32(res->status);
        goto done;
    }
    if (pfh) *pfh = res->diropres_u.diropres.file;
    if (pfa) *pfa = res->diropres_u.diropres.attributes;
done:
    if (!clnt_freeres(clnfs, (xdrproc_t)xdr_diropres, (caddr_t)res)) {
        DEBUG_PRINT(("could not free XDR\n"));
    }
    return error;
}

DWORD
FsDT_NFS::nfscreate(
    nfs_fh* pdir,
    const char* name,
    sattr* psa,
    nfs_fh* pfh,
    fattr* pfa
    )
{
    DFN(FsDT_NFS::nfscreate);
    DEBUG_PRINT(("%s\n", name));
    DWORD error = ERROR_SUCCESS;

    createargs ca;
    ca.where.dir = *pdir;
    ca.where.name = (char*)name;
    ca.attributes = *psa;

    diropres* res = nfsproc_create_2(&ca, clnfs);
    if (!res) {
        DEBUG_PRINT(("%s\n", clnt_sperror(clnfs, "NFS_CREATE")));
        return ERROR_UNEXP_NET_ERR;
    }
    if(res->status) {
        DEBUG_PRINT(("STATUS: %d\n", res->status));
        error = nfserr_to_win32(res->status);
        goto done;
    }
    if (pfh) *pfh = res->diropres_u.diropres.file;
    if (pfa) *pfa = res->diropres_u.diropres.attributes;
done:
    if (!clnt_freeres(clnfs, (xdrproc_t)xdr_diropres, (caddr_t)res)) {
        DEBUG_PRINT(("could not free XDR\n"));
    }
    return error;
}

DWORD
FsDT_NFS::nfssetattr(
    nfs_fh* pfh,
    sattr* psa
    )
{
    DFN(FsDT_NFS::nfssetattr);
    DWORD error = ERROR_SUCCESS;

    sattrargs saa;
    saa.file = *pfh;
    saa.attributes = *psa;

    attrstat* res = nfsproc_setattr_2(&saa, clnfs);
    if (!res) {
        DEBUG_PRINT(("%s\n", clnt_sperror(clnfs, "NFS_SETATTR")));
        return ERROR_UNEXP_NET_ERR;
    }
    if(res->status) {
        DEBUG_PRINT(("STATUS: %d\n", res->status));
        error = nfserr_to_win32(res->status);
        goto done;
    }
done:
    if (!clnt_freeres(clnfs, (xdrproc_t)xdr_attrstat, (caddr_t)res)) {
        DEBUG_PRINT(("could not free XDR\n"));
    }
    return error;
}

DWORD
FsDT_NFS::nfsgetattr(
    nfs_fh* pfh,
    fattr* pfa
    )
{
    DFN(FsDT_NFS::nfsgetattr);
    DWORD error = ERROR_SUCCESS;
    
    attrstat* res = nfsproc_getattr_2(pfh, clnfs);
    if (!res) {
        DEBUG_PRINT(("%s\n", clnt_sperror(clnfs, "NFS_GETATTR")));
        return ERROR_UNEXP_NET_ERR;
    }
    if(res->status) {
        DEBUG_PRINT(("STATUS: %d\n", res->status));
        error = nfserr_to_win32(res->status);
        goto done;
    }
    if (pfa) *pfa = res->attrstat_u.attributes;
done:
    if (!clnt_freeres(clnfs, (xdrproc_t)xdr_attrstat, (caddr_t)res)) {
        DEBUG_PRINT(("could not free XDR\n"));
    }
    return error;
}

DWORD
FsDT_NFS::nfswrite(
    nfs_fh* pfh,
    u_int offset,
    u_int count,
    void* data,
    fattr* pfa
    )
{
    DFN(FsDT_NFS::nfswrite);
    DWORD error = ERROR_SUCCESS;
    
    writeargs wa;
    wa.file = *pfh;
    wa.offset = offset;
    wa.data.data_len = count;
    wa.data.data_val = (char*)data;

    attrstat* res = nfsproc_write_2(&wa, clnfs);
    if (!res) {
        DEBUG_PRINT(("%s\n", clnt_sperror(clnfs, "NFS_WRITE")));
        return ERROR_UNEXP_NET_ERR;
    }
    if(res->status) {
        DEBUG_PRINT(("STATUS: %d\n", res->status));
        error = nfserr_to_win32(res->status);
        goto done;
    }
    if (pfa) *pfa = res->attrstat_u.attributes;
done:
    if (!clnt_freeres(clnfs, (xdrproc_t)xdr_attrstat, (caddr_t)res)) {
        DEBUG_PRINT(("could not free XDR\n"));
    }
    return error;
}

DWORD
FsDT_NFS::nfsread(
    nfs_fh* pfh,
    u_int offset,
    u_int count,
    void* data,
    u_int* pcount,
    fattr* pfa
    )
{
    DFN(FsDT_NFS::nfsread);
    DWORD error = ERROR_SUCCESS;

    readargs ra;
    ra.file = *pfh;
    ra.offset = offset;
    ra.count = count;

    readres* res = nfsproc_read_2(&ra, clnfs);
    if (!res) {
        DEBUG_PRINT(("%s\n", clnt_sperror(clnfs, "NFS_READ")));
        return ERROR_UNEXP_NET_ERR;
    }
    if(res->status) {
        DEBUG_PRINT(("STATUS: %d\n", res->status));
        error = nfserr_to_win32(res->status);
        goto done;
    }
    if (pfa) *pfa = res->readres_u.reply.attributes;
    if (pcount) *pcount = res->readres_u.reply.data.data_len;
    if (data) RtlCopyMemory(data, res->readres_u.reply.data.data_val, 
                            res->readres_u.reply.data.data_len);
done:
    if (!clnt_freeres(clnfs, (xdrproc_t)xdr_readres, (caddr_t)res)) {
        DEBUG_PRINT(("could not free XDR\n"));
    }
    return error;
}

DWORD
FsDT_NFS::nfsstatfs(
    nfs_fh* pfh,
    u_int* tsize,
    u_int* bsize,
    u_int* blocks,
    u_int* bfree,
    u_int* bavail
    )
{
    DFN(FsDT_NFS::nfsstatfs);
    DWORD error = ERROR_SUCCESS;

    statfsres* res = nfsproc_statfs_2(pfh, clnfs);
    if (!res) {
        DEBUG_PRINT(("%s\n", clnt_sperror(clnfs, "NFS_STATFS")));
        return ERROR_UNEXP_NET_ERR;
    }
    if(res->status) {
        DEBUG_PRINT(("STATUS: %d\n", res->status));
        error = nfserr_to_win32(res->status);
        goto done;
    }
    if (tsize) *tsize = res->statfsres_u.reply.tsize;
    if (bsize) *bsize = res->statfsres_u.reply.bsize;
    if (blocks) *blocks = res->statfsres_u.reply.blocks;
    if (bfree) *bfree = res->statfsres_u.reply.bfree;
    if (bavail) *bavail = res->statfsres_u.reply.bavail;
done:
    if (!clnt_freeres(clnfs, (xdrproc_t)xdr_statfsres, (caddr_t)res)) {
        DEBUG_PRINT(("could not free XDR\n"));
    }
    return error;
}

DWORD
FsDT_NFS::nfsremove(
    nfs_fh* pdir,
    const char* name
    )
{
    DFN(FsDT_NFS::nfsremove);
    DEBUG_PRINT(("%s\n", name));
    DWORD error = ERROR_SUCCESS;

    diropargs doa;
    doa.dir = *pdir;
    doa.name = (char*)name;

    nfsstat* res = nfsproc_remove_2(&doa, clnfs);
    if (!res) {
        DEBUG_PRINT(("%s\n", clnt_sperror(clnfs, "NFS_REMOVE")));
        return ERROR_UNEXP_NET_ERR;
    }
    if(*res) {
        DEBUG_PRINT(("STATUS: %d\n", *res));
        error = nfserr_to_win32(*res);
        goto done;
    }
done:
    if (!clnt_freeres(clnfs, (xdrproc_t)xdr_nfsstat, (caddr_t)res)) {
        DEBUG_PRINT(("could not free XDR\n"));
    }
    return error;
}

DWORD
FsDT_NFS::nfsrename(
    nfs_fh* pdir_from,
    const char* name_from,
    nfs_fh* pdir_to,
    const char* name_to
    )
{
    DFN(FsDT_NFS::nfsrename);
    DEBUG_PRINT(("%s to %s\n", name_from, name_to));
    DWORD error = ERROR_SUCCESS;

    renameargs ra;
    ra.from.dir = *pdir_from;
    ra.to.dir = *pdir_to;
    ra.from.name = (char*)name_from;
    ra.to.name = (char*)name_to;

    nfsstat* res = nfsproc_rename_2(&ra, clnfs);
    if (!res) {
        DEBUG_PRINT(("%s\n", clnt_sperror(clnfs, "NFS_RENAME")));
        return ERROR_UNEXP_NET_ERR;
    }
    if(*res) {
        DEBUG_PRINT(("STATUS: %d\n", *res));
        error = nfserr_to_win32(*res);
        goto done;        
    }
done:
    if (!clnt_freeres(clnfs, (xdrproc_t)xdr_nfsstat, (caddr_t)res)) {
        DEBUG_PRINT(("could not free XDR\n"));
    }
    return error;
}

DWORD
FsDT_NFS::nfsmkdir(
    nfs_fh* pdir,
    const char* name,
    sattr* psa,
    nfs_fh* pfh,
    fattr* pfa
    )
{
    DFN(FsDT_NFS::nfsmkdir);
    DEBUG_PRINT(("%s\n", name));
    DWORD error = ERROR_SUCCESS;

    createargs ca;
    ca.where.dir = *pdir;
    ca.where.name = (char*)name;
    ca.attributes = *psa;

    diropres* res = nfsproc_mkdir_2(&ca, clnfs);
    if (!res) {
        DEBUG_PRINT(("%s\n", clnt_sperror(clnfs, "NFS_MKDIR")));
        return ERROR_UNEXP_NET_ERR;
    }
    if(res->status) {
        DEBUG_PRINT(("STATUS: %d\n", res->status));
        error = nfserr_to_win32(res->status);
        goto done;
    }
    if (pfh) *pfh = res->diropres_u.diropres.file;
    if (pfa) *pfa = res->diropres_u.diropres.attributes;
done:
    if (!clnt_freeres(clnfs, (xdrproc_t)xdr_diropres, (caddr_t)res)) {
        DEBUG_PRINT(("could not free XDR\n"));
    }
    return error;
}

DWORD
FsDT_NFS::nfsrmdir(
    nfs_fh* pdir,
    const char* name
    )
{
    DFN(FsDT_NFS::nfsrmdir);
    DEBUG_PRINT(("%s\n", name));
    DWORD error = ERROR_SUCCESS;

    diropargs doa;
    doa.dir = *pdir;
    doa.name = (char*)name;

    nfsstat* res = nfsproc_rmdir_2(&doa, clnfs);
    if (!res) {
        DEBUG_PRINT(("%s\n", clnt_sperror(clnfs, "NFS_RMDIR")));
        return ERROR_UNEXP_NET_ERR;
    }
    if(*res) {
        DEBUG_PRINT(("STATUS: %d\n", *res));
        error = nfserr_to_win32(*res);
        goto done;
    }
done:
    if (!clnt_freeres(clnfs, (xdrproc_t)xdr_nfsstat, (caddr_t)res)) {
        DEBUG_PRINT(("could not free XDR\n"));
    }
    return error;
}
