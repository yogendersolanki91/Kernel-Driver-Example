/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   SMB TRANS2 command handler

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

#define DEVICE_TYPE ULONG

typedef struct _FILE_FS_VOLUME_INFORMATION {
    LARGE_INTEGER VolumeCreationTime;
    ULONG VolumeSerialNumber;
    ULONG VolumeLabelLength;
    BOOLEAN SupportsObjects;
    CHAR VolumeLabel[1];
//    WCHAR VolumeLabel[1];
} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

typedef struct _FILE_FS_SIZE_INFORMATION {
    LARGE_INTEGER TotalAllocationUnits;
    LARGE_INTEGER AvailableAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

typedef struct _FILE_FS_DEVICE_INFORMATION {                    // ntddk nthal
    DEVICE_TYPE DeviceType;                                     // ntddk nthal
    ULONG Characteristics;                                      // ntddk nthal
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;     // ntddk nthal
                                                                // ntddk nthal
typedef struct _FILE_FS_ATTRIBUTE_INFORMATION {
    ULONG FileSystemAttributes;
    LONG MaximumComponentNameLength;
    ULONG FileSystemNameLength;
    CHAR FileSystemName[1];
//    WCHAR FileSystemName[1];
} FILE_FS_ATTRIBUTE_INFORMATION, *PFILE_FS_ATTRIBUTE_INFORMATION;

template<class T>
inline
void
SET_REQ(
    T*& ptr,
    T2B* t2b
    )
{
    ptr = (T*)t2b->in.pParameters;
    t2b->in.pParameters += sizeof(T);
}

inline
void
MORE_ADVANCE_PARAMS(
    T2B* t2b,
    size_t by
    )
{
    t2b->in.pParameters += by;
}

// XXX: we should perhaps worry about padding issues...

inline
void*
ADD_RESP_PARAMS(
    DPB* dpb, 
    T2B* t2b, 
    PVOID resp, 
    size_t size
    )
{
    if (t2b->out.ParameterBytesLeft < size) return 0;
    if (t2b->out.pResp->DataCount) {
        PUCHAR d = (PUCHAR)dpb->out.smb + t2b->out.pResp->DataOffset;
        // need to use MoveMemory due to possible overlap
        RtlMoveMemory((PVOID)(d + size), 
                      (PVOID)d, t2b->out.pResp->DataCount);
        // we up the data offset below
    }
    PUCHAR p = (PUCHAR)dpb->out.smb + t2b->out.pResp->ParameterOffset 
        + t2b->out.pResp->ParameterCount;
    if (resp) RtlCopyMemory((PVOID)p, (PVOID)resp, (ULONG)size);
    t2b->out.pResp->ParameterCount += size;
    t2b->out.pResp->TotalParameterCount += size;
    *(t2b->out.pByteCount) += size;
    t2b->out.pResp->DataOffset += size;
    dpb->out.valid += size;
    t2b->out.ParameterBytesLeft -= size;
    return p;
}

inline 
void*
ADD_RESP_DATA(
    DPB* dpb, 
    T2B* t2b, 
    PVOID resp, 
    size_t size
    )
{
    if (t2b->out.DataBytesLeft < size) return 0;
    PUCHAR d = (PUCHAR)dpb->out.smb + t2b->out.pResp->DataOffset 
        + t2b->out.pResp->DataCount;
    if (resp) RtlCopyMemory((PVOID)d, (PVOID)resp, (ULONG)size);
    t2b->out.pResp->DataCount += size;
    t2b->out.pResp->TotalDataCount += size;
    *(t2b->out.pByteCount) += size;
    dpb->out.valid += size;
    t2b->out.DataBytesLeft -= size;
    return d;
}

////

BOOL
Trans2Unknown(
    DPB* dpb,
    T2B* t2b
    )
{
    DFN(Trans2Unknown);
    DEBUG_PRINT(("DOSERROR: (------ CANNOT HANDLE THIS TRANS2 FUNCTION ------)\n"));
    SET_DOSERROR(dpb, SERVER, NO_SUPPORT);
    return FALSE;
}

BOOL
Trans2QueryFsInfo(
    DPB* dpb,
    T2B* t2b
    )
{
    DFN(Trans2QueryFsInfo);
    PREQ_QUERY_FS_INFORMATION pReq;
    SET_REQ(pReq, t2b);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: no such uid\n"));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return FALSE;
    }

    fs_attr_t fsattr;
    DWORD error = pDisp->statfs(pDisp->get_root(), &fsattr);
    if (error) {
        DEBUG_PRINT(("WIN32ERROR: statfs error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return FALSE;
    }

    switch (pReq->InformationLevel) {
    case SMB_INFO_ALLOCATION: {
        FSALLOCATE Resp;
        Resp.idFileSystem = 0;
        Resp.cSectorUnit = 1;
        Resp.cbSector = 0x0400; // 1024-byte sectors
        Resp.cUnit = fsattr.total_bytes/(Resp.cSectorUnit*Resp.cbSector);
        Resp.cUnitAvail = fsattr.free_bytes/(Resp.cSectorUnit*Resp.cbSector);
        ADD_RESP_DATA(dpb, t2b, &Resp, sizeof(FSALLOCATE));
        return TRUE;
    }
    case SMB_INFO_VOLUME: {
        FSINFO Resp;
        Resp.ulVsn = 0;
        const size_t maxlen = sizeof(Resp.vol.szVolLabel);
        Resp.vol.cch = min(lstrlen(fsattr.fs_name), maxlen);
        lstrcpyn(Resp.vol.szVolLabel, fsattr.fs_name, maxlen);
        ADD_RESP_DATA(dpb, t2b, &Resp, sizeof(FSINFO));
        return TRUE;
    }
    case SMB_QUERY_FS_VOLUME_INFO: {
        const int name_len = lstrlen(fsattr.fs_name);
        const size_t size = sizeof(FILE_FS_VOLUME_INFORMATION) + name_len;
        PFILE_FS_VOLUME_INFORMATION pResp = 
            (PFILE_FS_VOLUME_INFORMATION)xmalloc(size);
        pResp->VolumeCreationTime.QuadPart = 0;
        pResp->VolumeSerialNumber = 0;
        pResp->VolumeLabelLength = name_len * sizeof(WCHAR);
        wsprintf((char*)pResp->VolumeLabel, "%S", fsattr.fs_name);
        ADD_RESP_DATA(dpb, t2b, pResp, size);
        xfree(pResp);
        return TRUE;
    }
    default:
        DEBUG_PRINT(("DOSERROR: <COULD NOT UNDERSTAND INFO LEVEL>\n"));
        SET_DOSERROR(dpb, SERVER, NO_SUPPORT);
        return FALSE;
    }
}

typedef struct _FILE_DIRECTORY_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    CHAR FileName[1];
//    WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

typedef struct _FILE_FULL_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    CHAR FileName[1];
//    WCHAR FileName[1];
} FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;

typedef struct _FILE_BOTH_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    CHAR FileName[1];
//    WCHAR FileName[1];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

typedef struct _FILE_NAMES_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    ULONG FileNameLength;
    CHAR FileName[1];
//    WCHAR FileName[1];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

const char*
find_filter(
    const char* filename, bool& literal
    )
{
    literal = true;
    const char* p = filename + lstrlen(filename);
    while ((p >= filename) && (*p != '\\') && (*p != '/')) { 
        literal = literal && (*p != '?') && (*p != '*');
        p--;
    }
    if (p >= filename) return ++p;
    return 0;
}

class dir_marshaller {
public:
    USHORT info_level;
    bool add_res_key;
    USHORT base_size;

    dir_marshaller(USHORT _info_level, bool _add_res_key)
        :info_level(_info_level),add_res_key(_add_res_key)
    {
        base_size = add_res_key?sizeof(ULONG):0;
        switch (info_level) {
        case SMB_INFO_STANDARD:
            base_size += sizeof(SMB_FIND_BUFFER);
            break;
        case SMB_INFO_QUERY_EA_SIZE:
            base_size += sizeof(SMB_FIND_BUFFER2);
            break;
        case SMB_INFO_QUERY_EAS_FROM_LIST:
        case SMB_FIND_FILE_DIRECTORY_INFO:
        case SMB_FIND_FILE_FULL_DIRECTORY_INFO:
        case SMB_FIND_FILE_NAMES_INFO:
        case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:
        default:
            base_size = 0;
        }
    }

    bool do_entry(void* p, dirinfo_t& entry, USHORT name_len)
    {
        DFN(do_entry);
        
        if (add_res_key) {
            PULONG pl = (PULONG)p;
            *pl = entry.cookie;
            pl++;
            p = pl;
        }

        switch (info_level) {
        case SMB_INFO_STANDARD: {
            PSMB_FIND_BUFFER pResp = (PSMB_FIND_BUFFER) p;
            time64_to_smb_datetime(entry.attribs.create_time,
                                   pResp->CreationDate.Ushort,
                                   pResp->CreationTime.Ushort);
            time64_to_smb_datetime(entry.attribs.access_time,
                                   pResp->LastAccessDate.Ushort,
                                   pResp->LastAccessTime.Ushort);
            time64_to_smb_datetime(entry.attribs.mod_time,
                                   pResp->LastWriteDate.Ushort,
                                   pResp->LastWriteTime.Ushort);
            pResp->DataSize = entry.attribs.file_size;
            pResp->AllocationSize = entry.attribs.alloc_size;
            pResp->Attributes = attribs_to_smb_attribs(entry.attribs.attributes);
            pResp->FileNameLength = name_len;
            RtlCopyMemory(pResp->FileName, entry.name, name_len + 1);
            return true;
        }
        case SMB_INFO_QUERY_EA_SIZE: {
            PSMB_FIND_BUFFER2 pResp = (PSMB_FIND_BUFFER2) p;
            time64_to_smb_datetime(entry.attribs.create_time,
                                   pResp->CreationDate.Ushort,
                                   pResp->CreationTime.Ushort);
            time64_to_smb_datetime(entry.attribs.access_time,
                                   pResp->LastAccessDate.Ushort,
                                   pResp->LastAccessTime.Ushort);
            time64_to_smb_datetime(entry.attribs.mod_time,
                                   pResp->LastWriteDate.Ushort,
                                   pResp->LastWriteTime.Ushort);
            pResp->DataSize = entry.attribs.file_size;
            pResp->AllocationSize = entry.attribs.alloc_size;
            pResp->Attributes = attribs_to_smb_attribs(entry.attribs.attributes);
            pResp->EaSize = 0;
            pResp->FileNameLength = name_len;
            RtlCopyMemory(pResp->FileName, entry.name, name_len + 1);
            return true;
        }
        case SMB_INFO_QUERY_EAS_FROM_LIST:
        case SMB_FIND_FILE_DIRECTORY_INFO:
        case SMB_FIND_FILE_FULL_DIRECTORY_INFO:
        case SMB_FIND_FILE_NAMES_INFO:
        case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:
        default:
            return false;
        }
        
    }
};

DWORD
bad_info_level(
    DPB* dpb
    )
{
    DFN(bad_info_level);
    DEBUG_PRINT(("DOSERROR: <COULD NOT UNDERSTAND INFO LEVEL>\n"));
    return ERROR_INVALID_FUNCTION;
}

DWORD
get_dir_entries(
    FsDispatchTable* pDisp,
    fhandle_t h,
    UINT32& cookie,
    filter_t* pfilter,
    dir_marshaller& dm,
    USHORT max_count,
    USHORT& count,
    bool& done,
    void*& last,
    DPB* dpb,
    T2B* t2b
    )
{
    DFN(get_dir_entries);

    done = false;
    count = 0;
    last = 0;

    DWORD space_left =
        (dpb->out.size > dpb->out.valid)?
        (dpb->out.size - dpb->out.valid):0;

    DEBUG_PRINT(("space left is %d\n", space_left));

    DWORD max_entry_size = dm.base_size + MAX_NAME_LENGTH;
    DWORD avg_entry_size = max_entry_size - MAX_NAME_LENGTH + 16;
    DWORD guess_entries = space_left / avg_entry_size;

    dirinfo_t* entries = new dirinfo_t[guess_entries];

    if (!entries) {
        DEBUG_PRINT(("DOSERROR: could not allocate space for dir entries\n"));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    DWORD error = ERROR_SUCCESS;
    while (!done && (count < max_count) && (space_left >= max_entry_size)) {

        // decreasing...
        DEBUG_PRINT(("...space left is %d\n", space_left));
        guess_entries = max(space_left / avg_entry_size, 1);

        UINT32 sub_count = 0;
        DWORD error = pDisp->read_dir(h, cookie, entries, 
                                      guess_entries*sizeof(dirinfo_t),
                                      &sub_count);

        done = (error == ERROR_NO_MORE_FILES);
        if (error && (error != ERROR_NO_MORE_FILES)) {
            DEBUG_PRINT(("read_dir failed us with error %d\n", error));
            goto finished;
        }

        DEBUG_PRINT(("starting loop: sub %d, cnt %d, max %d, spc %d, mes %d\n",
                     sub_count, count, max_count, space_left, max_entry_size));
    
        DWORD i = 0;
        while ((i < sub_count) && (count < max_count) && 
               (space_left >= max_entry_size)) {

            DEBUG_PRINT(("loop[%d]: sub %d, cnt %d, max %d, spc %d, mes %d\n",
                        i, sub_count, count, max_count, space_left, 
                        max_entry_size));

            dirinfo_t& entry = entries[i++];
            // allways advance the cookie to keep up with  i
            cookie = entry.cookie;

            if (!pfilter->match(entry.name)) continue;

            DEBUG_PRINT(("...space left is %d\n", space_left));

            DWORD name_len = lstrlen(entry.name);
            DWORD len = dm.base_size + name_len;
            void* p = ADD_RESP_DATA(dpb, t2b, 0, len);
            if (!p || (space_left < len)) {
                if (!count) {
                    DEBUG_PRINT(("out of memory w/o getting anything!\n"));
                    error = ERROR_NOT_ENOUGH_MEMORY;
                }
                goto finished;
            }
            space_left -= len;

            if (!dm.do_entry(p, entry, name_len)) {
                error = bad_info_level(dpb);
                goto finished;
            }
            last = p;
            count++;

        } // while...

        if (i < sub_count) done = false;

    } // while...
     
finished:
    delete [] entries;
    return error;
}

BOOL
Trans2FindFirst2(
    DPB* dpb,
    T2B* t2b
    )
{
    DFN(Trans2FindFirst2);
    PREQ_FIND_FIRST2 pReq;
    SET_REQ(pReq, t2b);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: bad uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return FALSE;
    }

    dir_marshaller dm(pReq->InformationLevel, 
                      pReq->Flags & SMB_FIND_RETURN_RESUME_KEYS);
    if (!dm.base_size) {
        SET_WIN32ERROR(dpb, bad_info_level(dpb));
        return FALSE;
    }

    bool literal;
    char* path = (char*)pReq->Buffer;
    const char* filter_str = find_filter(path, literal);
    if (!filter_str) {
        DEBUG_PRINT(("DOSERROR: could not find filter in %s\n",pReq->Buffer));
        SET_DOSERROR(dpb, SERVER, FILE_SPECS);
        return FALSE;
    }

    PRESP_FIND_FIRST2 pResp = (PRESP_FIND_FIRST2)
        ADD_RESP_PARAMS(dpb, t2b, 0, sizeof(RESP_FIND_FIRST2));
    if (!pResp) {
        DEBUG_PRINT(("DOSERROR: not enough buffer space...\n"));
        SET_DOSERROR(dpb, SERVER, ERROR);
        return FALSE;
    }

    DWORD error;
    fhandle_t h;
    UINT32 cookie;
    void* last;
    bool done;
    filter_t* pfilter;
    dirinfo_t* pentry;

    DEBUG_PRINT(("FLAGS: 0x%04x\n", pReq->Flags));

    // XXX - performance hack...case-sensitivty issues exist...
    if (literal) {
        pentry = new dirinfo_t; // XXX - should check for 0...
        error = pDisp->lookup(pDisp->get_root(), path, &pentry->attribs);
        if (error) {
            DEBUG_PRINT(("WIN32ERROR: could not find %s (0x%08X)\n",
                         filter_str, error));
            SET_WIN32ERROR(dpb, error);
            return FALSE;
        }
        lstrcpyn(pentry->name, filter_str, FsInterface::MAX_NAME_LENGTH);
        pentry->cookie = 0xFFFFFFFF;
        // -- common --
        DWORD name_len = lstrlen(pentry->name);
        void* d = ADD_RESP_DATA(dpb, t2b, 0, dm.base_size + name_len);
        dm.do_entry(d, *pentry, name_len);
        pResp->SearchCount = 1;
        cookie = pentry->cookie;
        last = d;
        // -- common --
        done = true;
        h = INVALID_FHANDLE_T;
        pfilter = 0;
    } else {
        pentry = 0;

        // need to allocate filter before fiddling with path...
        pfilter = new filter_t(filter_str);
        if (!pfilter) {
            DEBUG_PRINT(("out of memeory for filter!\n"));
            SET_WIN32ERROR(dpb, ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        int path_len = filter_str - (const char*)pReq->Buffer;
        path[path_len] = 0;

        error = pDisp->create(pDisp->get_root(),
                              path,
                              DISP_DIRECTORY | SHARE_READ | SHARE_WRITE,
                              0,
                              &h);
 
        if (error) {
            DEBUG_PRINT(("WIN32ERROR: could not create %s (0x%08X)\n",
                         path, error));
            SET_WIN32ERROR(dpb, error);
            return FALSE;
        }

        cookie = 0;
        last = 0;
        // -- common --
        done = false;
        error = get_dir_entries(pDisp, h, cookie, pfilter, dm,
                                pReq->SearchCount, pResp->SearchCount,
                                done, last, dpb, t2b);
        if (error) {
            delete pfilter;
            SET_WIN32ERROR(dpb, error);
            return FALSE;
        }
        // -- common -- 
    }

    // -- common --
    pResp->EndOfSearch = (done)?1:0;
    pResp->LastNameOffset = (last)?(((USHORT)((char*)last - 
                                              (char*)dpb->out.smb)) -
                                    t2b->out.pResp->DataOffset):0;
    pResp->EaErrorOffset = 0;
    // -- common --

    if ((pReq->Flags & SMB_FIND_CLOSE_AFTER_REQUEST) || 
        ((pReq->Flags & SMB_FIND_CLOSE_AT_EOS) && done)) {
        pResp->Sid = 0xFFFF;
        pDisp->close(h);
        delete pfilter;
        delete pentry;
    } else {
        pResp->Sid = add_find(h, cookie, pfilter, pentry);
        if (!pResp->Sid) {
            SET_WIN32ERROR(dpb, ERROR_TOO_MANY_OPEN_FILES);
            return FALSE;
        }
    }
    return TRUE;
}

BOOL
Trans2FindNext2(
    DPB* dpb,
    T2B* t2b
    )
{
    DFN(Trans2FindNext2);
    PREQ_FIND_NEXT2 pReq;
    SET_REQ(pReq, t2b);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: bad uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return FALSE;
    }

    dir_marshaller dm(pReq->InformationLevel, 
                      pReq->Flags & SMB_FIND_RETURN_RESUME_KEYS);
    if (!dm.base_size) {
        SET_WIN32ERROR(dpb, bad_info_level(dpb));
        return FALSE;
    }

    PRESP_FIND_NEXT2 pResp = (PRESP_FIND_NEXT2)
        ADD_RESP_PARAMS(dpb, t2b, 0, sizeof(RESP_FIND_NEXT2));
    if (!pResp) {
        DEBUG_PRINT(("DOSERROR: not enough buffer space...\n"));
        SET_DOSERROR(dpb, SERVER, ERROR);
        return FALSE;
    }

    DWORD error;
    bool done;
    fhandle_t h;
    filter_t* pfilter;
    UINT32 cookie;
    void* last = 0;
    dirinfo_t* pentry;

    if (!get_find(pReq->Sid, &h, &cookie, &pfilter, &pentry)) {
        DEBUG_PRINT(("DOSERROR: could not find sid\n"));
        SET_DOSERROR(dpb, DOS, BAD_FID);
        return FALSE;
    }

    DEBUG_PRINT(("FLAGS: 0x%04x\n", pReq->Flags));

    if (!(pReq->Flags & SMB_FIND_CONTINUE_FROM_LAST))
        cookie = pReq->ResumeKey;

    if (pentry) {
        DEBUG_PRINT(("client tried to get files after end of search...\n"));
        if (!cookie) {
            // -- common --
            DWORD name_len = lstrlen(pentry->name);
            void* d = ADD_RESP_DATA(dpb, t2b, 0, dm.base_size + name_len);
            dm.do_entry(d, *pentry, name_len);
            pResp->SearchCount = 1;
            cookie = pentry->cookie;
            last = d;
            // -- common --
        } else {
            pResp->SearchCount = 0;
        }
        done = true;
    } else {
        // -- common --
        done = false;
        error = get_dir_entries(pDisp, h, cookie, pfilter, dm,
                                pReq->SearchCount, pResp->SearchCount,
                                done, last, dpb, t2b);
        if (error) {
            delete pfilter;
            SET_WIN32ERROR(dpb, error);
            return FALSE;
        }
        // -- common -- 
    }

    // -- common --
    pResp->EndOfSearch = (done)?1:0;
    pResp->LastNameOffset = (last)?(((USHORT)((char*)last - 
                                              (char*)dpb->out.smb)) -
                                    t2b->out.pResp->DataOffset):0;
    pResp->EaErrorOffset = 0;
    // -- common --

    if ((pReq->Flags & SMB_FIND_CLOSE_AFTER_REQUEST) || 
        ((pReq->Flags & SMB_FIND_CLOSE_AT_EOS) && done)) {
        pDisp->close(h);
        delete pfilter;
        delete pentry;
        del_find(pReq->Sid);
    } else {
        save_find_cookie(pReq->Sid, cookie); // XXX - no error-checking
    }
    return TRUE;
}

BOOL
Trans2QueryPathInfo(
    DPB* dpb,
    T2B* t2b
    )
{
    DFN(Trans2QueryPathInfo);
    PREQ_QUERY_PATH_INFORMATION pReq;
    SET_REQ(pReq, t2b);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: no such uid\n"));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return FALSE;
    }

    LPCSTR file_name = (LPCSTR)pReq->Buffer;

    fattr_t attribs;
    DWORD error = pDisp->lookup(pDisp->get_root(), file_name, &attribs);
    if (error) {
        DEBUG_PRINT(("WIN32ERROR: lookup error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return FALSE;
    }

    switch(pReq->InformationLevel) {
    case SMB_INFO_STANDARD: {
        DWORD len = sizeof(SMB_FIND_BUFFER) - sizeof(UCHAR) - sizeof(CHAR);
        PSMB_FIND_BUFFER pResp = (PSMB_FIND_BUFFER) xmalloc(len);
        time64_to_smb_datetime(attribs.create_time,
                               pResp->CreationDate.Ushort,
                               pResp->CreationTime.Ushort);
        time64_to_smb_datetime(attribs.access_time,
                               pResp->LastAccessDate.Ushort,
                               pResp->LastAccessTime.Ushort);
        time64_to_smb_datetime(attribs.mod_time,
                               pResp->LastWriteDate.Ushort,
                               pResp->LastWriteTime.Ushort);
        pResp->DataSize = attribs.file_size;
        pResp->AllocationSize = attribs.alloc_size;
        pResp->Attributes = attribs_to_smb_attribs(attribs.attributes);
        ADD_RESP_DATA(dpb, t2b, pResp, len);
        xfree(pResp);
        return TRUE;
    }
    case SMB_INFO_QUERY_EA_SIZE: {
        DWORD len = sizeof(SMB_FIND_BUFFER2) - sizeof(UCHAR) - sizeof(CHAR);
        PSMB_FIND_BUFFER2 pResp = (PSMB_FIND_BUFFER2) xmalloc(len);
        time64_to_smb_datetime(attribs.create_time,
                               pResp->CreationDate.Ushort,
                               pResp->CreationTime.Ushort);
        time64_to_smb_datetime(attribs.access_time,
                               pResp->LastAccessDate.Ushort,
                               pResp->LastAccessTime.Ushort);
        time64_to_smb_datetime(attribs.mod_time,
                               pResp->LastWriteDate.Ushort,
                               pResp->LastWriteTime.Ushort);
        pResp->DataSize = attribs.file_size;
        pResp->AllocationSize = attribs.alloc_size;
        pResp->Attributes = attribs_to_smb_attribs(attribs.attributes);
        pResp->EaSize = 0;
        ADD_RESP_DATA(dpb, t2b, pResp, len);
        xfree(pResp);
        return TRUE;
    }
    case SMB_INFO_QUERY_EAS_FROM_LIST:
    case SMB_FIND_FILE_DIRECTORY_INFO:
    case SMB_FIND_FILE_FULL_DIRECTORY_INFO:
    case SMB_FIND_FILE_NAMES_INFO:
    case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:
    default: {
        DEBUG_PRINT(("DOSERROR: <COULD NOT UNDERSTAND INFO LEVEL>\n"));
        SET_DOSERROR(dpb, SERVER, NO_SUPPORT);
        return FALSE;
    }
    }
}

BOOL
Trans2SetPathInfo(
    DPB* dpb,
    T2B* t2b
    )
{
    DFN(Trans2SetPathInfo);
    return Trans2Unknown(dpb, t2b);
}

BOOL
Trans2QueryFileInfo(
    DPB* dpb,
    T2B* t2b
    )
{
    DFN(Trans2QueryFileInfo);
    return Trans2Unknown(dpb, t2b);
}

BOOL
Trans2SetFileInfo(
    DPB* dpb,
    T2B* t2b
    )
{
    DFN(Trans2SetFileInfo);
    return Trans2Unknown(dpb, t2b);
}
