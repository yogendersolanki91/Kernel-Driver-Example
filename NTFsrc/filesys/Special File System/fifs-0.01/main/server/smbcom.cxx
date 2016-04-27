/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   SMB command handler

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

#ifdef NT_LM
#define DIALECT_STRING "NT LM 0.12"
#else
#define DIALECT_STRING "LM1.2X002"
#endif

#define VERIFY_HEADER(flag, dpb) \
    BOOL flag = !dpb->out.valid; \
    if (flag) { dpb->out.valid = sizeof(NT_SMB_HEADER); InitSmbHeader(dpb); }

#define UPDATE_OUT_LEN(dpb, resp_type, resp)      \
    (dpb->out.valid += SIZEOF_SMB_PARAMS(resp_type, resp->ByteCount))

#define UPDATE_FOR_NEXT_ANDX(dpp, req_type, req, resp_type, resp) {    \
        resp->AndXCommand = req->AndXCommand;                          \
        resp->AndXReserved = 0;                                        \
        resp->AndXOffset = UPDATE_OUT_LEN(dpb, resp_type, resp);       \
        dpb->in.command = req->AndXCommand;                            \
        dpb->in.offset += SIZEOF_SMB_PARAMS(req_type, req->ByteCount); \
    }

#define CHECK_TOP_LEVEL(flag, dpb) { \
        if(!flag) SET_DOSERROR(dpb, SERVER, ERROR); \
    }

#define SET_REQ(type, name, dpb) \
    type name = ((type)(((PUCHAR) dpb->in.smb)+dpb->in.offset))

#define SET_RESP(type, name, dpb) \
    type name = ((type)(((PUCHAR) dpb->out.smb)+dpb->out.valid))

// #define SETUP_STUFF(main_type) \
//     SET_REQ(PREQ_##main_type, pReq, dpb); \
//     SET_RESP(PRESP_##main_type, pResp, dpb); \
//     #define _NEXT_LOCATION(

BOOL
SmbComUnknown(
    DPB* dpb
    )
{
    DFN(SmbComUnknown);
    DEBUG_PRINT(("(------ CANNOT HANDLE THIS COMMAND ------)\n"));
    VERIFY_HEADER(bTopLevel, dpb);
    SET_DOSERROR(dpb, SERVER, BAD_SMB_COMMAND);
    return TRUE;
}

BOOL
SmbComNegotiate(
    DPB* dpb
    )
{
    DFN(SmbComNegotiate);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_NEGOTIATE, pReq, dpb);
#ifdef NT_LM
    SET_RESP(PRESP_NT_NEGOTIATE, pResp, dpb);
#else
    SET_RESP(PRESP_NEGOTIATE, pResp, dpb);
#endif

    LPCSTR szDialect = NULL;
    int offset, dialect;

    offset = sizeof(UCHAR);
    dialect = 0;

    BOOL found = FALSE;
    while (offset < pReq->ByteCount) {
        szDialect = (LPCSTR)NEXT_LOCATION(pReq, REQ_NEGOTIATE, offset);
        if (!lstrcmp(szDialect, DIALECT_STRING)) {
            found = TRUE;
            break;
        }
        dialect++;
        offset += lstrlen(szDialect) + 1 + sizeof(UCHAR);
    }

    if (!found) {
        // spew back an error to the client...
        PRESP_OLD_NEGOTIATE pOldResp = (PRESP_OLD_NEGOTIATE) pResp;
        pOldResp->WordCount = 1;
        pOldResp->DialectIndex = 0xFFFF;
        pOldResp->ByteCount = 0;
        dpb->out.valid += sizeof(PRESP_OLD_NEGOTIATE);
    } else {
#ifdef NT_LM
        // NT LM 0.12
        pResp->WordCount = 17;
        pResp->DialectIndex = dialect;
        pResp->SecurityMode = 1; // user mode
        pResp->MaxMpxCount = (USHORT)min(config.num_workers, 0xFFFF);
        pResp->MaxNumberVcs = 1; // pulled 64 out of AFS...but ???
        pResp->MaxBufferSize = (USHORT)min(config.buffer_size, 0xFFFF);
        pResp->MaxRawSize = 0; // do not support raw
        pResp->SessionsKey = 1; // ??
        pResp->Capabilities = CAP_LARGE_FILES | CAP_NT_STATUS;
        pResp->SystemTimeLow = 0; // ??
        pResp->SystemTimeHigh = 0; // ??
        pResp->ServerTimeZone = 0; // ??
        pResp->EncryptionKeyLength = 0;
        pResp->ByteCount = 0;
        dpb->out.valid += sizeof(RESP_NT_NEGOTIATE);
#else
        // LM1.2X002
        // spew back the dialect we want...
        pResp->WordCount = 13;
        pResp->DialectIndex = dialect;
        pResp->SecurityMode = 3; // user mode
        pResp->MaxBufferSize = (USHORT)min(config.buffer_size, 0xFFFF);
        pResp->MaxMpxCount = (USHORT)min(config.num_workers, 
                                 0xFFFF); // add a fudge factor??
        pResp->MaxNumberVcs = 1; // pulled 64 out of AFS...
        pResp->RawMode = 0; // not supported
        pResp->SessionKey = 1; // ??
        pResp->ServerTime.Ushort = 0; // ?? (do we care?)
        pResp->ServerDate.Ushort = 0; // ?? (do we care?)
        pResp->ServerTimeZone = 0; // ?? (do we care?)
        pResp->EncryptionKeyLength = 0;
        pResp->Reserved = 0;
        pResp->ByteCount = 0;
        UPDATE_OUT_LEN(dpb, RESP_NEGOTIATE, pResp);
#endif
    }
    return TRUE;
}

BOOL
SmbComSessionSetupAndx(
    DPB* dpb
    )
{
    DFN(SmbComSessionSetupAndx);
    VERIFY_HEADER(bTopLevel, dpb);
#ifdef NT_LM
    SET_REQ(PREQ_NT_SESSION_SETUP_ANDX, pReq, dpb);
    SET_RESP(PRESP_NT_SESSION_SETUP_ANDX, pResp, dpb);
#else
    SET_REQ(PREQ_SESSION_SETUP_ANDX, pReq, dpb);
    SET_RESP(PRESP_SESSION_SETUP_ANDX, pResp, dpb);
#endif

#ifdef NT_LM
    if (pReq->WordCount == 13) {
        LPCSTR szAccountName = NULL;
        PUCHAR pAccountPassword = NULL;
        int offset = 0;
    }
#else
    if (pReq->WordCount == 10) {
        LPCSTR szAccountName = NULL;
        PUCHAR pAccountPassword = NULL;
        int offset = 0;

        if (pReq->ByteCount > offset) {
            pAccountPassword = (PUCHAR)
                NEXT_LOCATION(pReq, REQ_SESSION_SETUP_ANDX, offset);
            offset += pReq->PasswordLength;
        }
        if (pReq->ByteCount > offset) {
            szAccountName = (LPCSTR)
                NEXT_LOCATION(pReq, REQ_SESSION_SETUP_ANDX, offset);
            offset += lstrlen(szAccountName) + 1;
        }

        DEBUG_PRINT(("setup username: %s\n", szAccountName));
//        DEBUG_PRING(("setup password: %s\n", pAccountPassword));

        if (!is_authenticated_user(szAccountName, 
                                   pAccountPassword, pReq->PasswordLength)) {
            DEBUG_PRINT(("DOSERROR: could not authenticate user\n"));
            SET_DOSERROR(dpb, SERVER, BAD_PASSWORD);
            return TRUE;
        }

        FsDispatchTable* pDisp = 0;
        DWORD error;
        if (error = pFs->connect(szAccountName, &pDisp)) {
            DEBUG_PRINT(("WIN32ERROR: could not connect to server as %s (0x%08X)\n", szAccountName, error));
            SET_WIN32ERROR(dpb, error);
            return TRUE;
        }
        dpb->out.smb->Uid = add_user(szAccountName, pDisp);
        if (!dpb->out.smb->Uid) {
            DEBUG_PRINT(("DOSERROR: too many uids\n"));
            SET_DOSERROR(dpb, SERVER, TOO_MANY_UIDS);
            return TRUE;
        }
        pResp->WordCount = 3;
        pResp->Action = 0; // ?? will be 1 if we cannot authenticate...
        pResp->ByteCount = 0;//?? is this ok?

        UPDATE_FOR_NEXT_ANDX(dpb, 
                             REQ_SESSION_SETUP_ANDX, pReq,
                             RESP_SESSION_SETUP_ANDX, pResp);
    }
#endif
    else {
        SET_DOSERROR(dpb, SERVER, ERROR); // ok???
        return TRUE;
    }
    return SmbDispatch(dpb);
}

BOOL
SmbComTreeConnectAndx(
    DPB* dpb
    )
{
    DFN(SmbComTreeConnectAndx);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_TREE_CONNECT_ANDX, pReq, dpb);
    SET_RESP(PRESP_TREE_CONNECT_ANDX, pResp, dpb);

    PUCHAR pPassword = NULL;
    LPCSTR szPath = NULL;
    LPCSTR szService = NULL;
    int offset = 0;

    if (pReq->ByteCount > offset) {
        pPassword = (PUCHAR)
            NEXT_LOCATION(pReq, REQ_TREE_CONNECT_ANDX, offset);
        offset += pReq->PasswordLength;
    }
    if (pReq->ByteCount > offset) {
        szPath = (LPCSTR)
            NEXT_LOCATION(pReq, REQ_TREE_CONNECT_ANDX, offset);
        offset += lstrlen(szPath) + 1;
    }
    if (pReq->ByteCount > offset) {
        szService = (LPCSTR)
            NEXT_LOCATION(pReq, REQ_TREE_CONNECT_ANDX, offset);
    }

    // we set Tid to 1 because we only support one FS at a time...
    // if we wanted to be really fancy, we could have a multi-fs 
    // loopback server...
    dpb->out.smb->Tid = 1;
    // it turns out that we can get away with an older-style reply...hehe
    pResp->WordCount = 2;
    pResp->ByteCount = 3;
    strcpy((char*)pResp->Buffer, "A:");

    UPDATE_FOR_NEXT_ANDX(dpb, 
                         REQ_TREE_CONNECT_ANDX, pReq,
                         RESP_TREE_CONNECT_ANDX, pResp);

    return SmbDispatch(dpb);
}

BOOL
SmbComNoAndx(
    DPB* dpb
    )
{
    return TRUE;
}

BOOL
SmbComTrans2(
    DPB* dpb
    )
{
    DFN(SmbComTrans2);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_TRANSACTION, pReq, dpb);
    SET_RESP(PRESP_TRANSACTION, pResp, dpb);

    PUSHORT pSetup;
    int iSetup;

    pSetup = (PUSHORT) NEXT_LOCATION(pReq, REQ_TRANSACTION, 0);
    for (iSetup = 0; iSetup < pReq->SetupCount; iSetup++)
        DEBUG_PRINT(("Setup[0x%02x]    : 0x%04x (%s)\n",
                     iSetup, pSetup[iSetup],
                     SmbUnparseTrans2(pSetup[iSetup])));

    if (pReq->SetupCount > 1) {
        DEBUG_PRINT(("SetupCount > 1!!!\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok?
        return TRUE;
    }

    RtlZeroMemory(pResp, sizeof(RESP_TRANSACTION));
    pResp->WordCount = 10 + pReq->SetupCount;
    pResp->SetupCount = pReq->SetupCount;

    RtlCopyMemory((PUSHORT) NEXT_LOCATION(pResp, RESP_TRANSACTION, 0),
                  pSetup, pResp->SetupCount*sizeof(USHORT));

    T2B t2b;
    t2b.in.pReq = pReq;
    t2b.in.pParameters = (PUCHAR)dpb->in.smb + pReq->ParameterOffset;
    t2b.in.pData = (PUCHAR)dpb->in.smb + pReq->DataOffset;
    t2b.out.pResp = pResp;
    t2b.out.ParameterBytesLeft = pReq->MaxParameterCount;;
    t2b.out.DataBytesLeft = pReq->MaxDataCount;;
    t2b.out.pByteCount = 
        (PUSHORT)NEXT_LOCATION(pResp, RESP_TRANSACTION, 
                               pResp->SetupCount*sizeof(USHORT));
    *(t2b.out.pByteCount) = 0;

    // initialize parameter and data offset anyhow...
    // XXX: we choose not to pad these because the NT SMB client
    //      does not seem to care...(at least not 3.51 or 4.0)
    t2b.out.pResp->ParameterOffset = t2b.out.pResp->DataOffset = 
        (PUCHAR)t2b.out.pResp - (PUCHAR)dpb->out.smb
        + SIZEOF_TRANS2_RESP_HEADER(t2b.out.pResp);

    // we increment:
    // pResp->WordCount
    // pResp->TotalParameterCount
    // pResp->TotalDataCount
    // pResp->ParameterCount
    // pResp->ParameterOffset
    // pResp->ParameterDisplacement -- not really, since we do not honor max
    // pResp->DataCount
    // pResp->DataOffset
    // pResp->DataDisplacement -- not really, since we do not honor max
    // *(t2b.out.pByteCount)

    dpb->out.valid += sizeof(RESP_TRANSACTION) + sizeof(USHORT);
    // dispatch each transaction...
    for (iSetup = 0; iSetup < pReq->SetupCount; iSetup++) {
        dpb->out.valid += sizeof(USHORT);
        dpb->in.command = pSetup[iSetup];
        if (!Trans2Dispatch(dpb, &t2b)) {
            // someone bellow us set an error, so we need to stop the loop
            // and return TRUE so the packet gets sent.
            break;
        }
    }
    // need to set length stuff
    return TRUE;
}

BOOL
SmbComQueryInformation(
    DPB* dpb
    )
{
    DFN(SmbComQueryInformation);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_QUERY_INFORMATION, pReq, dpb);
    SET_RESP(PRESP_QUERY_INFORMATION, pResp, dpb);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->ByteCount < 2) {
        DEBUG_PRINT(("DOSERROR: ByteCount < 2\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // OK?
        return TRUE;
    }

    LPCSTR file_name = (LPCSTR)NEXT_LOCATION(pReq, 
                                             REQ_QUERY_INFORMATION, 
                                             sizeof(UCHAR));

    fattr_t attribs;
    DWORD error = pDisp->lookup(pDisp->get_root(), file_name, &attribs);
    if (error) {
        DEBUG_PRINT(("WIN32ERROR: lookup error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return TRUE;
    }

    pResp->WordCount = 10;
    pResp->FileAttributes = attribs_to_smb_attribs(attribs.attributes);
    pResp->LastWriteTimeInSeconds = time64_to_smb_timedate(attribs.mod_time);
    RtlZeroMemory(pResp->Reserved, sizeof(pResp->Reserved));
    pResp->FileSize = attribs.file_size;
    pResp->ByteCount = 0;
    UPDATE_OUT_LEN(dpb, RESP_QUERY_INFORMATION, pResp);

    return TRUE;
}

BOOL
SmbComSetInformation(
    DPB* dpb
    )
{
    DFN(SmbComSetInformation);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_SET_INFORMATION, pReq, dpb);
    SET_RESP(PRESP_SET_INFORMATION, pResp, dpb);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->ByteCount < 2) {
        DEBUG_PRINT(("DOSERROR: ByteCount < 2\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok?
        return TRUE;
    }

    LPCSTR file_name = (LPCSTR)NEXT_LOCATION(pReq, 
                                             REQ_SET_INFORMATION, 
                                             sizeof(UCHAR));

    fattr_t attribs;
    DWORD error = pDisp->lookup(pDisp->get_root(), file_name, &attribs);
    if (error) {
        DEBUG_PRINT(("WIN32ERROR: lookup error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return TRUE;
    }

    DWORD disp;
    if (attribs.attributes & ATTR_DIRECTORY) {
        disp = DISP_DIRECTORY;
    } else {
        disp = DISP_OPEN_EXISTING;
    }
    fhandle_t h;
    error = pDisp->create(pDisp->get_root(), file_name, disp, 0, &h);
    if (error) {
        DEBUG_PRINT(("WIN32ERROR: create error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return TRUE;
    }

    attribs.file_size = INVALID_UINT64;
    attribs.alloc_size = INVALID_UINT64;
    attribs.access_time = INVALID_TIME64;
    attribs.create_time = INVALID_TIME64;
    attribs.mod_time = INVALID_TIME64;

    if (pReq->LastWriteTimeInSeconds)
        attribs.mod_time = smb_timedate_to_time64(pReq->LastWriteTimeInSeconds);
    attribs.attributes = smb_attribs_to_attribs(pReq->FileAttributes);

    error = pDisp->set_attr(h, &attribs);
    pDisp->close(h);
    if (error) {
        DEBUG_PRINT(("WIN32ERROR: set_attr error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return TRUE;
    }

    pResp->WordCount = 0;
    pResp->ByteCount = 0;
    UPDATE_OUT_LEN(dpb, RESP_SET_INFORMATION, pResp);

    return TRUE;
}

BOOL
SmbComCheckDirectory(
    DPB* dpb
    )
{
    DFN(SmbComCheckDirectory);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_CHECK_DIRECTORY, pReq, dpb);
    SET_RESP(PRESP_CHECK_DIRECTORY, pResp, dpb);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->ByteCount < 2) {
        DEBUG_PRINT(("DOSERROR: ByteCount < 2\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok?
        return TRUE;
    }

    LPCSTR file_name = (LPCSTR)NEXT_LOCATION(pReq, 
                                             REQ_QUERY_INFORMATION, 
                                             sizeof(UCHAR));

    fattr_t attribs;
    DWORD error = pDisp->lookup(pDisp->get_root(), file_name, &attribs);
    if (error) {
        DEBUG_PRINT(("ERROR: lookup error 0x%08X\n", error));
        SET_DOSERROR(dpb, DOS, BAD_PATH); // XXX -- ???
        return TRUE;
    }

    if (!(attribs.attributes & ATTR_DIRECTORY)) {
        DEBUG_PRINT(("ERROR: lookup error 0x%08X\n", error));
        SET_DOSERROR(dpb, DOS, BAD_PATH); // XXX -- ???
        return TRUE;
    }

    pResp->WordCount = 0;
    pResp->ByteCount = 0;
    UPDATE_OUT_LEN(dpb, RESP_CHECK_DIRECTORY, pResp);
    return TRUE;
}

BOOL
SmbComFindClose2(
    DPB* dpb
    )
{
    DFN(SmbComFindClose2);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_FIND_CLOSE2, pReq, dpb);
    SET_RESP(PRESP_FIND_CLOSE2, pResp, dpb);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 1) {
        DEBUG_PRINT(("DOSERROR: WordCount != 1\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok?
        return TRUE;
    }

    fhandle_t h;
    filter_t* pfilter;
    dirinfo_t* pentry;
    if (!get_find(pReq->Sid, &h, 0, &pfilter, &pentry)) {
        DEBUG_PRINT(("DOSERROR: could not find sid\n"));
        SET_DOSERROR(dpb, DOS, BAD_FID);
        return TRUE;
    }

    pDisp->close(h);
    delete pfilter;
    delete pentry;

    if (!del_find(pReq->Sid)) {
        DEBUG_PRINT(("DOSERROR: could not delete SID %d\n", pReq->Sid));
        SET_DOSERROR(dpb, DOS, BAD_FID);
        return TRUE;
    }

    pResp->WordCount = 0;
    pResp->ByteCount = 0;
    UPDATE_OUT_LEN(dpb, RESP_FIND_CLOSE2, pResp);
    return TRUE;
}

BOOL
SmbComDelete(
    DPB* dpb
    )
{
    DFN(SmbComDelete);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_DELETE, pReq, dpb);
    SET_RESP(PRESP_DELETE, pResp, dpb);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->ByteCount < 2) {
        DEBUG_PRINT(("DOSERROR: ByteCount < 2\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok?
        return TRUE;
    }

    LPCSTR file_name = (LPCSTR)NEXT_LOCATION(pReq, 
                                             REQ_DELETE,
                                             sizeof(UCHAR));

    DWORD error = pDisp->remove(pDisp->get_root(), file_name);
    if (error) {
        DEBUG_PRINT(("WIN32EROR: remove error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return TRUE;
    }
    
    pResp->WordCount = 0;
    pResp->ByteCount = 0;
    UPDATE_OUT_LEN(dpb, RESP_DELETE, pResp);
    return TRUE;
}

BOOL
SmbComRename(
    DPB* dpb
    )
{
    DFN(SmbComRename);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_RENAME, pReq, dpb);
    SET_RESP(PRESP_RENAME, pResp, dpb);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->ByteCount < 4) {
        DEBUG_PRINT(("DOSERROR: ByteCount < 4\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok?
        return TRUE;
    }

    LPCSTR from_name = (LPCSTR)NEXT_LOCATION(pReq, 
                                             REQ_RENAME,
                                             sizeof(UCHAR));

    LPCSTR to_name = (LPCSTR)NEXT_LOCATION(pReq, 
                                           REQ_RENAME,
                                           2*sizeof(UCHAR)+
                                           lstrlen(from_name)+1);

    DWORD error = pDisp->rename(pDisp->get_root(), from_name,
                                pDisp->get_root(), to_name);
    if (error) {
        DEBUG_PRINT(("WIN32ERROR: rename error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return TRUE;
    }
    
    pResp->WordCount = 0;
    pResp->ByteCount = 0;
    UPDATE_OUT_LEN(dpb, RESP_RENAME, pResp);
    return TRUE;
}

BOOL
SmbComCreateDirectory(
    DPB* dpb
    )
{
    DFN(SmbComCreateDirectory);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_CREATE_DIRECTORY, pReq, dpb);
    SET_RESP(PRESP_CREATE_DIRECTORY, pResp, dpb);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->ByteCount < 2) {
        DEBUG_PRINT(("DOSERROR: ByteCount < 2\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok?
        return TRUE;
    }

    LPCSTR file_name = (LPCSTR)NEXT_LOCATION(pReq, 
                                             REQ_CREATE_DIRECTORY,
                                             sizeof(UCHAR));

    DWORD error = pDisp->mkdir(pDisp->get_root(), file_name, 0);
    if (error) {
        DEBUG_PRINT(("WIN32ERROR: mkdir error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return TRUE;
    }
    
    pResp->WordCount = 0;
    pResp->ByteCount = 0;
    UPDATE_OUT_LEN(dpb, RESP_CREATE_DIRECTORY, pResp);
    return TRUE;
}

BOOL
SmbComDeleteDirectory(
    DPB* dpb
    )
{
    DFN(SmbComDeleteDirectory);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_DELETE_DIRECTORY, pReq, dpb);
    SET_RESP(PRESP_DELETE_DIRECTORY, pResp, dpb);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->ByteCount < 2) {
        DEBUG_PRINT(("DOSERROR: ByteCount < 2\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok?
        return TRUE;
    }

    LPCSTR file_name = (LPCSTR)NEXT_LOCATION(pReq, 
                                             REQ_DELETE_DIRECTORY,
                                             sizeof(UCHAR));

    DWORD error = pDisp->rmdir(pDisp->get_root(), file_name);
    if (error) {
        DEBUG_PRINT(("WIN32ERROR: rmdir error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return TRUE;
    }
    
    pResp->WordCount = 0;
    pResp->ByteCount = 0;
    UPDATE_OUT_LEN(dpb, RESP_DELETE_DIRECTORY, pResp);
    return TRUE;
}

BOOL
SmbComOpenAndx(
    DPB* dpb
    )
{
    DFN(SmbComOpenAndx);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_OPEN_ANDX, pReq, dpb);
    SET_RESP(PRESP_OPEN_ANDX, pResp, dpb);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 15) {
        DEBUG_PRINT(("DOSERROR: WordCount != 15\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok?
        return TRUE;
    }

    LPCSTR file_name = (LPCSTR)NEXT_LOCATION(pReq, 
                                             REQ_OPEN_ANDX,
                                             sizeof(UCHAR));

    bool additional_info = (pReq->Flags & SMB_OPEN_QUERY_INFORMATION);
//    bool exclusive_oplock_requested = (pReq->Flags & SMB_OPEN_OPLOCK);
//    bool batch_oplock_requested = (pReq->Flags & SMB_OPEN_OPBATCH);

    UINT32 flags = smb_access_to_flags(pReq->DesiredAccess) | 
        smb_openfunc_to_flags(pReq->OpenFunction);

    if (!(flags & DISP_MASK)) {
        // XXX --- error!!!
        DEBUG_PRINT(("DOSERROR: nothing to do!!!!\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok
        return TRUE;
    }

    fattr_t attr;
    attr.file_size = INVALID_UINT64;
    attr.alloc_size = INVALID_UINT64;
    attr.create_time = smb_timedate_to_time64(pReq->CreationTimeInSeconds);
    attr.access_time = INVALID_TIME64;
    attr.mod_time = INVALID_TIME64;
    attr.attributes = smb_attribs_to_attribs(pReq->FileAttributes);

    fhandle_t h;
    DWORD error = pDisp->create(pDisp->get_root(),
                                file_name,
                                flags,
                                &attr,
                                &h);
    if (error) {
        DEBUG_PRINT(("WIN32ERROR: create error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return TRUE;
    }

    if (additional_info) {
        error = pDisp->get_attr(h, &attr);
        if (error) {
            DEBUG_PRINT(("WIN32ERROR: get_attr error 0x%08X\n", error));
            SET_WIN32ERROR(dpb, error);
            return TRUE;
        }
        pResp->FileAttributes = attribs_to_smb_attribs(attr.attributes);
        pResp->LastWriteTimeInSeconds = time64_to_smb_timedate(attr.mod_time);
        pResp->DataSize = attr.file_size;
    } else {
        pResp->FileAttributes = 0;
        pResp->LastWriteTimeInSeconds = 0;
        pResp->DataSize = 0;
    }
    pResp->WordCount = 15;
    pResp->Fid = add_file(h); // XXX -- do not take error into account
    pResp->GrantedAccess = pReq->DesiredAccess & SMB_DA_FCB_MASK;
    pResp->FileType = 0;
    pResp->DeviceState = 0;
    // XXX - for actual Action value, need to do a lookup beforehand...
    pResp->Action = 0;
    // XXX - is ServerFid = 0 really ok?  It seems like it never gets used...
    pResp->ServerFid = 0;
    pResp->Reserved = 0;
    pResp->ByteCount = 0;

    UPDATE_FOR_NEXT_ANDX(dpb, 
                         REQ_OPEN_ANDX, pReq,
                         RESP_OPEN_ANDX, pResp);

    return SmbDispatch(dpb);
}

BOOL
SmbComWrite(
    DPB* dpb
    )
{
    DFN(SmbComWrite);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_WRITE, pReq, dpb);
    SET_RESP(PRESP_WRITE, pResp, dpb);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 5) {
        DEBUG_PRINT(("DOSERROR: WordCount != 5\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok?
        return TRUE;
    }

    fhandle_t h = get_file(pReq->Fid);
    if (h == INVALID_FHANDLE_T) {
        DEBUG_PRINT(("DOSERROR: bad fid\n"));
        SET_DOSERROR(dpb, SERVER, BAD_FID);
        return TRUE;
    }

    USHORT count = pReq->Count;
    ULONG offset = pReq->Offset;

    // NOTE -- it turns out that the BufferFormat and DataLength fields
    // doe not exist!

    void* data = (void*)NEXT_LOCATION(pReq,
                                      REQ_WRITE,
                                      0);

    UINT64 actual_count = count;
    DWORD error = pDisp->write(h, offset, &actual_count, data);
    if (error) {
        DEBUG_PRINT(("WIN32ERROR: write error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return TRUE;
    }

    pResp->WordCount = 1;
    pResp->Count = actual_count;
    pResp->ByteCount = 0;

    UPDATE_OUT_LEN(dpb, RESP_WRITE, pResp);
    return TRUE;
}

BOOL
SmbComClose(
    DPB* dpb
    )
{
    DFN(SmbComClose);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_CLOSE, pReq, dpb);
    SET_RESP(PRESP_CLOSE, pResp, dpb);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return TRUE;
    }

    fhandle_t h = get_file(pReq->Fid);
    if (h == INVALID_FHANDLE_T) {
        DEBUG_PRINT(("DOSERROR: no such fid\n"));
        SET_DOSERROR(dpb, SERVER, BAD_FID);
        return TRUE;
    }

    // XXX -- NEED TO ADD BIT ABOUT SETTING FILE DATE/TIME...

    DWORD error = pDisp->close(h);
    if (error) {
        DEBUG_PRINT(("WIN32ERROR: close error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return TRUE;
    }

    del_file(pReq->Fid);

    pResp->WordCount = 0;
    pResp->ByteCount = 0;

    UPDATE_OUT_LEN(dpb, RESP_CLOSE, pResp);
    return TRUE;
}

BOOL
SmbComReadAndx(
    DPB* dpb
    )
{
    DFN(SmbComReadAndx);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_READ_ANDX, pReq, dpb);
    SET_RESP(PRESP_READ_ANDX, pResp, dpb);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 10) {
        DEBUG_PRINT(("DOSERROR: WordCount != 10\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok
        return TRUE;
    }

    DWORD consumed = (pResp->Buffer - (PUCHAR)dpb->out.smb);
    DWORD remaining = dpb->out.size - consumed;
    USHORT MinCount = pReq->MinCount;
    USHORT MaxCount = pReq->MaxCount;
    ULONG offset = pReq->Offset;

    DEBUG_PRINT(("MaxCount = %d, MinCount = %d\n", MaxCount, MinCount));

    if (MaxCount > remaining) {
        DEBUG_PRINT(("DOSERROR: MaxCount (%d) > remaining (%d)\n", MaxCount, remaining));
        SET_DOSERROR(dpb, SERVER, ERROR); // XXX -- ok?
        return TRUE;
    }

    fhandle_t h = get_file(pReq->Fid);
    if (h == INVALID_FHANDLE_T) {
        DEBUG_PRINT(("DOSERROR: no such fid\n"));
        SET_DOSERROR(dpb, SERVER, BAD_FID);
        return TRUE;
    }


    UINT64 actual_count = MaxCount;

    DWORD error = pDisp->read(h, offset, &actual_count, pResp->Buffer);
    if (error) {
        DEBUG_PRINT(("WIN32ERROR: read error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return TRUE;
    }

    pResp->WordCount = 12;
    pResp->Remaining = 0;
    pResp->DataCompactionMode = 0;
    pResp->Reserved = 0;

    pResp->Reserved2 = 0;
    RtlZeroMemory(pResp->Reserved3, sizeof(pResp->Reserved3));

    pResp->DataLength = actual_count;
    pResp->DataOffset = consumed;
    pResp->ByteCount = actual_count;

    UPDATE_FOR_NEXT_ANDX(dpb, 
                         REQ_READ_ANDX, pReq,
                         RESP_READ_ANDX, pResp);

    return SmbDispatch(dpb);
}

BOOL
SmbComQueryInformation2(
    DPB* dpb
    )
{
    DFN(SmbComQueryInformation2);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_QUERY_INFORMATION2, pReq, dpb);
    SET_RESP(PRESP_QUERY_INFORMATION2, pResp, dpb);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 1) {
        DEBUG_PRINT(("DOSERROR: WordCount != 1\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok?
        return TRUE;
    }

    fhandle_t h = get_file(pReq->Fid);
    if (h == INVALID_FHANDLE_T) {
        DEBUG_PRINT(("DOSERROR: no such fid\n"));
        SET_DOSERROR(dpb, SERVER, BAD_FID);
        return TRUE;
    }

    fattr_t attribs;
    DWORD error = pDisp->get_attr(h, &attribs);
    if (error) {
        DEBUG_PRINT(("WIN32ERROR: get_attr error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return TRUE;
    }

    pResp->WordCount = 11;
    RtlZeroMemory(&pResp->CreationDate, sizeof(USHORT) * 6);
    time64_to_smb_datetime(attribs.create_time,
                           pResp->CreationDate.Ushort,
                           pResp->CreationTime.Ushort);
    time64_to_smb_datetime(attribs.access_time,
                           pResp->LastAccessDate.Ushort,
                           pResp->LastAccessTime.Ushort);
    time64_to_smb_datetime(attribs.mod_time,
                           pResp->LastWriteDate.Ushort,
                           pResp->LastWriteTime.Ushort);
    pResp->FileDataSize = attribs.file_size;
    pResp->FileAllocationSize = attribs.alloc_size;
    pResp->FileAttributes = attribs_to_smb_attribs(attribs.attributes);
    pResp->ByteCount = 0;
    UPDATE_OUT_LEN(dpb, RESP_QUERY_INFORMATION2, pResp);

    return TRUE;
}

BOOL
SmbComSetInformation2(
    DPB* dpb
    )
{
    DFN(SmbComSetInformation2);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_SET_INFORMATION2, pReq, dpb);
    SET_RESP(PRESP_SET_INFORMATION2, pResp, dpb);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 7) {
        DEBUG_PRINT(("DOSERROR: WordCount != 7\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok?
        return TRUE;
    }

    fhandle_t h = get_file(pReq->Fid);
    if (h == INVALID_FHANDLE_T) {
        DEBUG_PRINT(("DOSERROR: no such fid\n"));
        SET_DOSERROR(dpb, SERVER, BAD_FID);
        return TRUE;
    }

    fattr_t attribs;

    attribs.file_size = INVALID_UINT64;
    attribs.alloc_size = INVALID_UINT64;
    attribs.access_time = INVALID_TIME64;
    attribs.create_time = INVALID_TIME64;
    attribs.mod_time = INVALID_TIME64;
    attribs.attributes = INVALID_UINT32;

    if (pReq->CreationDate.Ushort || pReq->CreationTime.Ushort) {
        smb_datetime_to_time64(pReq->CreationDate.Ushort,
                               pReq->CreationTime.Ushort,
                               attribs.create_time);
    }
    if (pReq->LastAccessDate.Ushort || pReq->LastAccessTime.Ushort) {
        smb_datetime_to_time64(pReq->LastAccessDate.Ushort,
                               pReq->LastAccessTime.Ushort,
                               attribs.access_time);
    }
    if (pReq->LastWriteDate.Ushort || pReq->LastWriteTime.Ushort) {
        smb_datetime_to_time64(pReq->LastWriteDate.Ushort,
                               pReq->LastWriteTime.Ushort,
                               attribs.mod_time);
    }

    DWORD error = pDisp->set_attr(h, &attribs);
    if (error) {
        DEBUG_PRINT(("WIN32ERROR: set_attr error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return TRUE;
    }

    pResp->WordCount = 0;
    pResp->ByteCount = 0;
    UPDATE_OUT_LEN(dpb, RESP_SET_INFORMATION2, pResp);

    return TRUE;
}

BOOL
SmbComLockingAndx(
    DPB* dpb
    )
{
    DFN(SmbComLockingAndx);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_LOCKING_ANDX, pReq, dpb);
    SET_RESP(PRESP_LOCKING_ANDX, pResp, dpb);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 8) {
        DEBUG_PRINT(("DOSERROR: WordCount != 8\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok?
        return TRUE;
    }

    fhandle_t h = get_file(pReq->Fid);
    bool bShared  = pReq->LockType & LOCKING_ANDX_SHARED_LOCK;
    bool bRelease = pReq->LockType & LOCKING_ANDX_OPLOCK_RELEASE;
    bool bChange  = pReq->LockType & LOCKING_ANDX_CHANGE_LOCKTYPE;
    bool bCancel  = pReq->LockType & LOCKING_ANDX_CANCEL_LOCK;
    bool bLarge   = pReq->LockType & LOCKING_ANDX_LARGE_FILES;
    UCHAR level   = pReq->OplockLevel;
    ULONG timeout = pReq->Timeout;
    USHORT nulocks= pReq->NumberOfUnlocks;
    USHORT nlocks = pReq->NumberOfLocks;

    DEBUG_PRINT(("FID: 0x%04X\n"
                 "Shared : %d\n"
                 "Release: %d\n"
                 "Change : %d\n"
                 "Cancel : %d\n"
                 "Large  : %d\n"
                 "Level  : %d\n"
                 "Timeout: %d msec\n"
                 "Unlocks: %d\n"
                 "Locks: %d\n",
                 pReq->Fid,
                 bShared, bRelease, bChange, bCancel, bLarge,
                 level, timeout, nulocks, nlocks                
        ));

    pResp->WordCount = 2;
    pResp->ByteCount = 0;
    DEBUG_PRINT(("CANNOT REALLY HANDLE LOCKING!"));
//    SET_DOSERROR(dpb, SERVER, BAD_SMB_COMMAND);
//    return TRUE;

    UPDATE_FOR_NEXT_ANDX(dpb, 
                         REQ_LOCKING_ANDX, pReq,
                         RESP_LOCKING_ANDX, pResp);

    return SmbDispatch(dpb);
}

BOOL
SmbComSeek(
    DPB* dpb
    )
{
    DFN(SmbComSeek);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_SEEK, pReq, dpb);
    SET_RESP(PRESP_SEEK, pResp, dpb);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 4) {
        DEBUG_PRINT(("DOSERROR: WordCount != 4\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok?
        return TRUE;
    }

    fhandle_t h = get_file(pReq->Fid);
    if (h == INVALID_FHANDLE_T) {
        DEBUG_PRINT(("DOSERROR: bad fid\n"));
        SET_DOSERROR(dpb, SERVER, BAD_FID);
        return TRUE;
    }

    USHORT mode = pReq->Mode;
    if (mode && (mode != 2)) {
        DEBUG_PRINT(("DOSERROR: only support modes 0 & 2 (not %d)\n", mode));
        SET_DOSERROR(dpb, SERVER, ERROR);
        return TRUE;
    }
    bool bStart = !mode;

    LONG offset = pReq->Offset;

    fattr_t attribs;
    DWORD error = pDisp->get_attr(h, &attribs);
    if (error) {
        DEBUG_PRINT(("WIN32ERROR: get_attr error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return TRUE;
    }

    UINT64 result;
    if (bStart) {
        if (offset > 0) result = offset;
        else result = 0;
    } else {
        if (-offset > attribs.file_size) result = 0;
        else result = attribs.file_size + offset;
    }

    pResp->WordCount = 2;
    pResp->Offset = result;
    pResp->ByteCount = 0;

    UPDATE_OUT_LEN(dpb, RESP_SEEK, pResp);
    return TRUE;
}

BOOL
SmbComFlush(
    DPB* dpb
    )
{
    DFN(SmbComFlush);
    VERIFY_HEADER(bTopLevel, dpb);
    SET_REQ(PREQ_FLUSH, pReq, dpb);
    SET_RESP(PRESP_FLUSH, pResp, dpb);

    USHORT uid = dpb->in.smb->Uid;
    FsDispatchTable* pDisp = get_user(uid);
    if (!pDisp) {
        DEBUG_PRINT(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(dpb, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 1) {
        DEBUG_PRINT(("DOSERROR: WordCount != 1\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok?
        return TRUE;
    }

    if (pReq->ByteCount != 0) {
        DEBUG_PRINT(("DOSERROR: ByteCount != 0\n"));
        SET_DOSERROR(dpb, SERVER, ERROR); // ok?
        return TRUE;
    }

    DWORD error;
    if (pReq->Fid == 0xFFFF) {
        error = pDisp->flush(INVALID_FHANDLE_T);
    } else {
        fhandle_t h = get_file(pReq->Fid);
        if (h == INVALID_FHANDLE_T) {
            DEBUG_PRINT(("DOSERROR: bad fid\n"));
            SET_DOSERROR(dpb, SERVER, BAD_FID);
            return TRUE;
        }
        error = pDisp->flush(h);
    }

    if (error) {
        DEBUG_PRINT(("WIN32ERROR: flush error 0x%08X\n", error));
        SET_WIN32ERROR(dpb, error);
        return TRUE;
    }

    pResp->WordCount = 0;
    pResp->ByteCount = 0;

    UPDATE_OUT_LEN(dpb, RESP_FLUSH, pResp);
    return SmbComUnknown(dpb);
    return TRUE;
}

BOOL
SmbComLogoffAndx(
    DPB* dpb
    )
{
    DFN(SmbComLogoffAndx);
    return SmbComUnknown(dpb);
}

BOOL
SmbComTreeDisconnect(
    DPB* dpb
    )
{
    DFN(SmbComTreeDisconnect);
    return SmbComUnknown(dpb);
}
