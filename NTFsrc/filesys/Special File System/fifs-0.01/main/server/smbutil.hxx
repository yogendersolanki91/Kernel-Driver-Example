/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   SMB utilities header

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

#ifndef __SMB_UTIL_HXX__
#define __SMB_UTIL_HXX__

#ifndef __cplusplus
#error "C++ compiler required"
#endif

#define SET_DOSERROR(dpb, ec, err) {               \
        dpb->out.smb->Status.DosError.ErrorClass = SMB_ERR_CLASS_##ec; \
        dpb->out.smb->Status.DosError.Error = SMB_ERR_##err;     \
    }

#ifdef NDEBUG
inline void* xmalloc(size_t size)  { return LocalAlloc(LMEM_FIXED, size); }
inline void  xfree  (void* buffer) { LocalFree(buffer); }
#else
void* xmalloc(size_t size);
void  xfree(void* buffer);
#endif

struct Packet {
    DWORD connection; // monotonically increasing
    UCHAR lsn;
    LPVOID buffer;
    WORD len;
};

struct DPBI {
    DWORD command;
    PNT_SMB_HEADER smb;
    DWORD size;
    DWORD offset;
};

struct DPBO {
    PNT_SMB_HEADER smb;
    DWORD size;
    DWORD valid;
};

struct DPB {
    Packet* p;
    DPBI in;
    DPBO out;
};

struct T2BI {
    PREQ_TRANSACTION pReq;
//    PUSHORT pByteCount;
    PUCHAR pParameters;
    PUCHAR pData;
};

struct T2BO {
    PRESP_TRANSACTION pResp;
    USHORT ParameterBytesLeft;
    USHORT DataBytesLeft;
    PUSHORT pByteCount;
};

struct T2B {
    T2BI in;
    T2BO out;
};

//
// Not Thread-Safe:
//

void
smbutil_init(
    );

void
smbutil_cleanup(
    );

//
// Thread-Safe:
//

#ifdef NDEBUG
#define DumpSmb(x,y,z)
#else
VOID
DumpSmb(
    PVOID buffer,
    DWORD size,
    BOOL bRequest
    );
#endif

BOOL
IsSmb(
    LPVOID pBuffer,
    DWORD nLength
    );

BOOL
SmbDispatch(
    DPB* dpb
    );

BOOL
Trans2Dispatch(
    DPB* dpb,
    T2B* t2b
    );

VOID
InitSmbHeader(
    DPB* dpb
    );

LPCSTR
SmbUnparseCommand(
    UCHAR command
    );

LPCSTR
SmbUnparseTrans2(
    USHORT code
    );

#include "fsinterface.hxx"
using namespace FsInterface;

inline
USHORT
attribs_to_smb_attribs(
    UINT32 attribs
    )
{
    USHORT smb_attribs = 0;
    if (attribs & ATTR_READONLY)  smb_attribs |= SMB_FILE_ATTRIBUTE_READONLY;
    if (attribs & ATTR_HIDDEN)    smb_attribs |= SMB_FILE_ATTRIBUTE_HIDDEN;
    if (attribs & ATTR_SYSTEM)    smb_attribs |= SMB_FILE_ATTRIBUTE_SYSTEM;
    if (attribs & ATTR_ARCHIVE)   smb_attribs |= SMB_FILE_ATTRIBUTE_ARCHIVE;
    if (attribs & ATTR_DIRECTORY) smb_attribs |= SMB_FILE_ATTRIBUTE_DIRECTORY;
    return smb_attribs;
}

inline
UINT32
smb_attribs_to_attribs(
    USHORT smb_attribs
    )
{
    UINT32 attribs = 0;
    if (smb_attribs & SMB_FILE_ATTRIBUTE_READONLY)  attribs |= ATTR_READONLY;
    if (smb_attribs & SMB_FILE_ATTRIBUTE_HIDDEN)    attribs |= ATTR_HIDDEN;
    if (smb_attribs & SMB_FILE_ATTRIBUTE_SYSTEM)    attribs |= ATTR_SYSTEM;
    if (smb_attribs & SMB_FILE_ATTRIBUTE_ARCHIVE)   attribs |= ATTR_ARCHIVE;
    if (smb_attribs & SMB_FILE_ATTRIBUTE_DIRECTORY) attribs |= ATTR_DIRECTORY;
    return attribs;
}

inline
UINT32
smb_access_to_flags(
    USHORT access
    )
{
    UINT32 flags = 0;
    switch (access & SMB_DA_SHARE_MASK) {
    case SMB_DA_SHARE_COMPATIBILITY:
    case SMB_DA_SHARE_DENY_NONE:
        flags |= SHARE_READ | SHARE_WRITE;
        break;
    case SMB_DA_SHARE_DENY_WRITE:
        flags |= SHARE_READ;
        break;
    case SMB_DA_SHARE_DENY_READ:
        flags |= SHARE_WRITE;
        break;
    case SMB_DA_SHARE_EXCLUSIVE:
    default:
        break;
    }
    switch (access & SMB_DA_ACCESS_MASK) {
    case SMB_DA_ACCESS_READ:
    case SMB_DA_ACCESS_EXECUTE:
        flags |= ACCESS_READ;
        break;
    case SMB_DA_ACCESS_WRITE:
        flags |= ACCESS_WRITE;
        break;
    case SMB_DA_ACCESS_READ_WRITE:
        flags |= ACCESS_READ | ACCESS_WRITE;
        break;
    }
    return flags;
}

inline
UINT32
smb_openfunc_to_flags(
    USHORT openfunc
    )
{
    switch (openfunc & (SMB_OFUN_OPEN_MASK | SMB_OFUN_CREATE_MASK)) {
    case (SMB_OFUN_CREATE_FAIL   | SMB_OFUN_OPEN_FAIL):
        return 0;
    case (SMB_OFUN_CREATE_FAIL   | SMB_OFUN_OPEN_OPEN):
        return DISP_OPEN_EXISTING;
    case (SMB_OFUN_CREATE_FAIL   | SMB_OFUN_OPEN_TRUNCATE):
        return DISP_TRUNCATE_EXISTING;
    case (SMB_OFUN_CREATE_CREATE | SMB_OFUN_OPEN_FAIL):
        return DISP_CREATE_NEW;
    case (SMB_OFUN_CREATE_CREATE | SMB_OFUN_OPEN_OPEN):
        return DISP_OPEN_ALWAYS;
    case (SMB_OFUN_CREATE_CREATE | SMB_OFUN_OPEN_TRUNCATE):
        return DISP_CREATE_ALWAYS;
    default:
        return 0;
    }
}

inline
void
local_time64(
    TIME64& time  // system time to be converted to local time
    )
{
    TIME64 local = time;
    FileTimeToLocalFileTime((LPFILETIME)&time, (LPFILETIME)&local);
    time = local;
}

inline
void
sys_time64(
    TIME64& time  // local time to be converted to system time
    )
{
    TIME64 sys = time;
    LocalFileTimeToFileTime((LPFILETIME)&time, (LPFILETIME)&sys);
    time = sys;
}

inline
void
smb_datetime_to_time64(
    const USHORT& smbdate,
    const USHORT& smbtime,
    TIME64& time
    )
{
    DosDateTimeToFileTime(smbdate, smbtime, (LPFILETIME)&time);
    sys_time64(time);
}

inline
TIME64 // system time
smb_timedate_to_time64(
    ULONG smb_timedate // local time
    )
{
    struct SMB_TIMEDATE {
        SMB_TIME time;
        SMB_DATE date;
    };
    TIME64 time = 0;
    DosDateTimeToFileTime(((SMB_TIMEDATE*)&smb_timedate)->date.Ushort,
                          ((SMB_TIMEDATE*)&smb_timedate)->time.Ushort,
                          (LPFILETIME)&time);
    sys_time64(time);
    return time;
}

inline
void
time64_to_smb_datetime(
    TIME64 time,
    USHORT& smbdate,
    USHORT& smbtime
    )
{
    local_time64(time);
    smbdate = smbtime = 0;
    FileTimeToDosDateTime((LPFILETIME)&time, &smbdate, &smbtime);
}

inline
ULONG // local time
time64_to_smb_timedate(
    TIME64 time // system time
    )
{
    local_time64(time);
    struct SMB_TIMEDATE {
        SMB_TIME time;
        SMB_DATE date;
    };
    ULONG smb_timedate = 0;
    FileTimeToDosDateTime((LPFILETIME)&time,
                          &(((SMB_TIMEDATE*)&smb_timedate)->date.Ushort),
                          &(((SMB_TIMEDATE*)&smb_timedate)->date.Ushort));
    return smb_timedate;
}

inline bool dword_in_range(
    const DWORD& in, 
    const DWORD& low, 
    const DWORD& high
    )
{
    return (low <= in) && (in <= high);
}

inline void set_doserror(DPB* dpb, USHORT ec, USHORT err)
{
    dpb->out.smb->Status.DosError.ErrorClass = ec;
    dpb->out.smb->Status.DosError.Error = err;
}

inline void SET_WIN32ERROR(
    DPB* dpb, 
    DWORD error
    )
{

    if (!error) return;
    if (dword_in_range(error, 1, 18) || (error == 32) || (error == 33) || 
        (error == 80) || dword_in_range(error, 230, 234) || (error == 145)) {
        set_doserror(dpb,
                     SMB_ERR_CLASS_DOS, 
                     (USHORT)error);
    } else if (dword_in_range(error, 19, 31) || 
        (error == 34) || (error == 36) || (error == 39)) {
        set_doserror(dpb,
                     SMB_ERR_CLASS_HARDWARE, 
                     (USHORT)error);
    } else if (error == 112) {
        SET_DOSERROR(dpb, HARDWARE, DISK_FULL);
    } else if (error == 123) {
        set_doserror(dpb,
                     SMB_ERR_CLASS_SERVER,
                     (USHORT)error);
//     } else if (error == 145) {
//         SET_DOSERROR(dpb, DOS, CURRENT_DIRECTORY);
    } else if (error == 183) {
        SET_DOSERROR(dpb, DOS, FILE_EXISTS);
    } else {
        SET_DOSERROR(dpb, SERVER, ERROR);
    }
}

#endif
