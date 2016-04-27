/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   NetBIOS snooper SMB command handler

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

#include "smball.h"

#define TEMP_SIZE 1024

LPSTR
SmbComSessionSetupAndx(
    UCHAR command,
    LPSTR buffer,
    PNT_SMB_HEADER pSmb,
    DWORD dwSize,
    DWORD dwOffset,
    BOOL bRequest
    )
{
    PVOID pData = ((PUCHAR) pSmb) + dwOffset;
    if (bRequest) {
        PREQ_SESSION_SETUP_ANDX pReq = pData;

        buffer += wsprintf(buffer,
                           "WordCount    : 0x%02x\n"
                           "AndXCommand  : 0x%02x (%s)\n"
                           "AndXReserved : 0x%02x\n"
                           "AndXOffset   : 0x%04x\n"
                           "MaxBufferSize: 0x%04x\n"
                           "MaxMpxCount  : 0x%04x\n"
                           "VcNumber     : 0x%04x\n"
                           "SessionKey   : 0x%08x\n",
                           pReq->WordCount,
                           pReq->AndXCommand,
                           SmbUnparseCommand(pReq->AndXCommand),
                           pReq->AndXReserved,
                           pReq->AndXOffset,
                           pReq->MaxBufferSize,
                           pReq->MaxMpxCount,
                           pReq->VcNumber,
                           pReq->SessionKey);

        switch (pReq->WordCount) {
        case 10: {
            LPCSTR szAccountName = NULL;
            LPCSTR szPrimaryDomain = NULL;
            LPCSTR szNativeOS = NULL;
            LPCSTR szNativeLanMan = NULL;
            PUCHAR pAccountPassword = NULL;
            int offset = 0;

            buffer += wsprintf(buffer,
                               "PasswordLength : 0x%04x\n"
                               "Reserved       : 0x%08x\n"
                               "ByteCount      : 0x%04x\n",
                               pReq->PasswordLength,
                               pReq->Reserved,
                               pReq->ByteCount);

            if (pReq->ByteCount > offset) {
                pAccountPassword =
                    NEXT_LOCATION(pReq, REQ_SESSION_SETUP_ANDX, offset);
                offset += pReq->PasswordLength;
            }
            if (pReq->ByteCount > offset) {
                szAccountName =
                    NEXT_LOCATION(pReq, REQ_SESSION_SETUP_ANDX, offset);
                offset += lstrlen(szAccountName) + 1;
            }
            if (pReq->ByteCount > offset) {
                szPrimaryDomain = 
                    NEXT_LOCATION(pReq, REQ_SESSION_SETUP_ANDX, offset);
                offset += lstrlen(szPrimaryDomain) + 1;
            }
            if (pReq->ByteCount > offset) {
                szNativeOS =
                    NEXT_LOCATION(pReq, REQ_SESSION_SETUP_ANDX, offset);
                offset += lstrlen(szNativeOS) + 1;
            }
            if (pReq->ByteCount > offset) {
                szNativeLanMan =
                    NEXT_LOCATION(pReq, REQ_SESSION_SETUP_ANDX, offset);
            }

            if (pAccountPassword) {
                buffer += wsprintf(buffer,
                                   "AccountPassword: <see below>\n");
                buffer = DumpData(buffer, pAccountPassword, 
                                  pReq->PasswordLength);

            }
            if (szAccountName)
                buffer += wsprintf(buffer,
                                   "AccountName    : %s\n",
                                   szAccountName);
            if (szPrimaryDomain)
                buffer += wsprintf(buffer,
                                   "PrimaryDomain  : %s\n",
                                   szPrimaryDomain);
            if (szNativeOS)
                buffer += wsprintf(buffer,
                                   "NativeOS       : %s\n",
                                   szNativeOS);
            if (szNativeLanMan)
                buffer += wsprintf(buffer,
                                   "NativeLanMan   : %s\n",
                                   szNativeLanMan);

            buffer = SmbDispatch(pReq->AndXCommand, 
                                 buffer, pSmb, dwSize, dwOffset + 
                                 SIZEOF_SMB_PARAMS(REQ_SESSION_SETUP_ANDX,
                                                   pReq->ByteCount),
                                 bRequest);
            break;
        }
        default:
            buffer += wsprintf(buffer, "(could not dump)\n");
        }
    } else {
        PRESP_SESSION_SETUP_ANDX pResp = pData;
        LPCSTR szPrimaryDomain = NULL;
        LPCSTR szNativeOS = NULL;
        LPCSTR  szNativeLanMan = NULL;
        int offset = 0;

        if (pResp->ByteCount > offset) {
            szNativeOS =
                NEXT_LOCATION(pResp, RESP_SESSION_SETUP_ANDX, offset);
            offset += lstrlen(szNativeOS) + 1;
        }
        if (pResp->ByteCount > offset) {
            szNativeLanMan =
                NEXT_LOCATION(pResp, RESP_SESSION_SETUP_ANDX, offset);
            offset += lstrlen(szNativeLanMan) + 1;
        }
        if (pResp->ByteCount > offset) {
            szPrimaryDomain = 
                NEXT_LOCATION(pResp, RESP_SESSION_SETUP_ANDX, offset);
        }

        buffer += wsprintf(buffer,
                           "WordCount    : 0x%02x\n"
                           "AndXCommand  : 0x%02x (%s)\n"
                           "AndXReserved : 0x%02x\n"
                           "AndXOffset   : 0x%04x\n"
                           "Action       : 0x%04x\n"
                           "ByteCount    : 0x%04x\n",
                           pResp->WordCount,
                           pResp->AndXCommand,
                           SmbUnparseCommand(pResp->AndXCommand),
                           pResp->AndXReserved,
                           pResp->AndXOffset,
                           pResp->Action,
                           pResp->ByteCount);

        if (szNativeOS)
            buffer += wsprintf(buffer,
                               "NativeOS     : %s\n",
                               szNativeOS);
        if (szNativeLanMan)
            buffer += wsprintf(buffer,
                               "NativeLanMan : %s\n",
                               szNativeLanMan);
        if (szPrimaryDomain)
            buffer += wsprintf(buffer,
                               "PrimaryDomain: %s\n",
                               szPrimaryDomain);

        buffer = SmbDispatch(pResp->AndXCommand,
                             buffer, pSmb, dwSize, dwOffset + 
                             SIZEOF_SMB_PARAMS(RESP_SESSION_SETUP_ANDX,
                                               pResp->ByteCount),
                             bRequest);
    }
    return buffer;
}

LPSTR
SmbComTreeConnectAndx(
    UCHAR command,
    LPSTR buffer,
    PNT_SMB_HEADER pSmb,
    DWORD dwSize,
    DWORD dwOffset,
    BOOL bRequest
    )
{
    PVOID pData = ((PUCHAR) pSmb) + dwOffset;
    if (bRequest) {
        PREQ_TREE_CONNECT_ANDX pReq = pData;
        PUCHAR pPassword = NULL;
        LPCSTR szPath = NULL;
        LPCSTR szService = NULL;
        int offset = 0;

        if (pReq->ByteCount > offset) {
            pPassword = NEXT_LOCATION(pReq, REQ_TREE_CONNECT_ANDX, offset);
            offset += pReq->PasswordLength;
        }
        if (pReq->ByteCount > offset) {
            szPath =  NEXT_LOCATION(pReq, REQ_TREE_CONNECT_ANDX, offset);
            offset += lstrlen(szPath) + 1;
        }
        if (pReq->ByteCount > offset) {
            szService = NEXT_LOCATION(pReq, REQ_TREE_CONNECT_ANDX, offset);
        }

        buffer += wsprintf(buffer,
                           "WordCount     : 0x%02x\n"
                           "AndXCommand   : 0x%02x (%s)\n"
                           "AndXReserved  : 0x%02x\n"
                           "AndXOffset    : 0x%04x\n"
                           "Flags         : 0x%04x\n"
                           "PasswordLength: 0x%04x\n"
                           "ByteCount     : 0x%04x\n",
                           pReq->WordCount,
                           pReq->AndXCommand,
                           SmbUnparseCommand(pReq->AndXCommand),
                           pReq->AndXReserved,
                           pReq->AndXOffset,
                           pReq->Flags,
                           pReq->PasswordLength,
                           pReq->ByteCount);
                           
        if (pPassword) {
            buffer += wsprintf(buffer,
                               "Password      : <see below>\n");
            buffer = DumpData(buffer, pPassword, 
                              pReq->PasswordLength);
        }
        if (szPath)
            buffer += wsprintf(buffer,
                               "Path          : %s\n",
                               szPath);
        if (szService)
            buffer += wsprintf(buffer,
                               "Service       : %s\n",
                               szService);

        buffer = SmbDispatch(pReq->AndXCommand, 
                             buffer, pSmb, dwSize, dwOffset + 
                             SIZEOF_SMB_PARAMS(REQ_TREE_CONNECT_ANDX,
                                               pReq->ByteCount),
                             bRequest);
    } else {
        PRESP_TREE_CONNECT_ANDX pResp = pData;

        buffer += wsprintf(buffer,
                           "WordCount    : 0x%02x\n"
                           "AndXCommand  : 0x%02x (%s)\n"
                           "AndXReserved : 0x%02x\n"
                           "AndXOffset   : 0x%04x\n",
                           pResp->WordCount,
                           pResp->AndXCommand,
                           SmbUnparseCommand(pResp->AndXCommand),
                           pResp->AndXReserved,
                           pResp->AndXOffset);

        switch(pResp->WordCount) {
        case 2: {
            LPCSTR szService = NULL;
            int offset = 0;

            if (pResp->ByteCount > offset) {
                szService = 
                    NEXT_LOCATION(pResp, RESP_TREE_CONNECT_ANDX, offset);
            }

            buffer += wsprintf(buffer,
                               "ByteCount    : 0x%04x\n",
                               pResp->ByteCount);
            if (szService) {
                buffer += wsprintf(buffer,
                                   "Service      : %s\n",
                                   szService);
            }
            break;
        }
        case 3: {
            PRESP_21_TREE_CONNECT_ANDX pResp = pData;
            LPCSTR szService = NULL;
            LPCSTR szNativeFS = NULL;
            int offset = 0;

            if (pResp->ByteCount > offset) {
                szService = 
                    NEXT_LOCATION(pResp, RESP_TREE_CONNECT_ANDX, offset);
                offset += lstrlen(szService) + 1;
            }
            if (pResp->ByteCount > offset) {
                szNativeFS = 
                    NEXT_LOCATION(pResp, RESP_TREE_CONNECT_ANDX, offset);
            }

            buffer += wsprintf(buffer,
                               "OptionalSupport: 0x%04x\n"
                               "ByteCount      : 0x%04x\n",
                               pResp->OptionalSupport,
                               pResp->ByteCount);

            if (szService)
                buffer += wsprintf(buffer,
                                   "Service        : %s\n",
                                   szService);
            if (szNativeFS)
                buffer += wsprintf(buffer,
                                   "NativeFileSys  : %s\n",
                                   szNativeFS);
            break;
        }
        default:
            buffer += wsprintf(buffer, "(could not dump)\n");
        }

        buffer = SmbDispatch(pResp->AndXCommand,
                             buffer, pSmb, dwSize, dwOffset + 
                             SIZEOF_SMB_PARAMS(RESP_TREE_CONNECT_ANDX,
                                               pResp->ByteCount),
                             bRequest);
    }
    return buffer;
}

LPSTR
SmbComNoAndx(
    UCHAR command,
    LPSTR buffer,
    PNT_SMB_HEADER pSmb,
    DWORD dwSize,
    DWORD dwOffset,
    BOOL bRequest
    )
{
    return buffer;
}

LPSTR
SmbComNegotiate(
    UCHAR command,
    LPSTR buffer,
    PNT_SMB_HEADER pSmb,
    DWORD dwSize,
    DWORD dwOffset,
    BOOL bRequest
    )
{
    PVOID pData = ((PUCHAR) pSmb) + dwOffset;
    if (bRequest) {
        PREQ_NEGOTIATE pReqNeg = pData;
        LPCSTR szDialect = NULL;
        int offset, dialect;

        buffer += wsprintf(buffer,
                           "WordCount: %d\n"
                           "ByteCount: %d\n",
                           pReqNeg->WordCount,
                           pReqNeg->ByteCount);
        offset = sizeof(UCHAR);
        dialect = 0;
        while (offset < pReqNeg->ByteCount) {
            szDialect = (LPCSTR)NEXT_LOCATION(pReqNeg, REQ_NEGOTIATE, offset);
            buffer += wsprintf(buffer,
                               "DialectName[%d]: %s\n",
                               dialect++, szDialect);
            offset += lstrlen(szDialect) + 1 + sizeof(UCHAR);
        }
    } else {
        PRESP_OLD_NEGOTIATE pRespOldNeg = pData;
        buffer += wsprintf(buffer,
                           "WordCount   : 0x%02x\n"
                           "DialectIndex: 0x%04x\n",
                           pRespOldNeg->WordCount,
                           pRespOldNeg->DialectIndex);
        switch (*(PUCHAR)pData) {
        case 13: {
            PRESP_NEGOTIATE pRespNeg = pData;
            UCHAR szSecurityMode[TEMP_SIZE];
            UCHAR szServerTime[TEMP_SIZE];
            UCHAR szServerDate[TEMP_SIZE];
            SmbDumpSecurityMode(szSecurityMode, pRespNeg->SecurityMode);
            SmbDumpTime(szServerTime, pRespNeg->ServerTime);
            SmbDumpDate(szServerDate, pRespNeg->ServerDate),
            buffer += wsprintf(buffer,
                               "SecurityMode : 0x%04x (%s)\n"
                               "MaxBufferSize: 0x%04x\n"
                               "MaxMpxCount  : 0x%04x\n"
                               "MaxNumberVcs : 0x%04x\n"
                               "RawMode      : 0x%04x\n"
                               "SessionKey   : 0x%08x\n"
                               "ServerTime   : 0x%04x (%s)\n"
                               "ServerDate   : 0x%04x (%s)\n"
                               "ServerTZ     : 0x%04x\n"
                               "EncryptKeyLen: 0x%04x\n"
                               "Reserved     : 0x%04x\n"
                               "ByteCount    : 0x%04x\n",
                               pRespNeg->SecurityMode,
                               szSecurityMode,
                               pRespNeg->MaxBufferSize,
                               pRespNeg->MaxMpxCount,
                               pRespNeg->MaxNumberVcs,
                               pRespNeg->RawMode,
                               pRespNeg->SessionKey,
                               pRespNeg->ServerTime,
                               szServerTime,
                               pRespNeg->ServerDate,
                               szServerDate,
                               pRespNeg->ServerTimeZone,
                               pRespNeg->EncryptionKeyLength,
                               pRespNeg->Reserved,
                               pRespNeg->ByteCount);
            break;
        }
        }
    }
    return buffer;
}

LPSTR
SmbComTrans2(
    UCHAR command,
    LPSTR buffer,
    PNT_SMB_HEADER pSmb,
    DWORD dwSize,
    DWORD dwOffset,
    BOOL bRequest
    )
{
    PVOID pData = ((PUCHAR) pSmb) + dwOffset;
    if (bRequest) {
        PREQ_TRANSACTION pReq = pData;
        PUSHORT pSetup;
        PUCHAR pParameter, pData;
        USHORT ByteCount;
        int iSetup;

        buffer += wsprintf(buffer,
                           "WordCount      : 0x%02x\n"
                           "TotalParamCount: 0x%04x\n"
                           "TotalDataCount : 0x%04x\n"
                           "MaxParamCount  : 0x%04x\n"
                           "MaxDataCount   : 0x%04x\n"
                           "MaxSetupCount  : 0x%02x\n"
                           "Reserved       : 0x%02x\n"
                           "Flags          : 0x%04x\n"
                           "Timeout        : 0x%08x\n"
                           "Reserved2      : 0x%04x\n"
                           "ParameterCount : 0x%04x\n"
                           "ParameterOffset: 0x%04x\n"
                           "DataCount      : 0x%04x\n"
                           "DataOffset     : 0x%04x\n"
                           "SetupCount     : 0x%02x\n"
                           "Reserved3      : 0x%02x\n",
                           pReq->WordCount,
                           pReq->TotalParameterCount,
                           pReq->TotalDataCount,
                           pReq->MaxParameterCount,
                           pReq->MaxDataCount,
                           pReq->MaxSetupCount,
                           pReq->Reserved,
                           pReq->Flags,
                           pReq->Timeout,
                           pReq->Reserved2,
                           pReq->ParameterCount,
                           pReq->ParameterOffset,
                           pReq->DataCount,
                           pReq->DataOffset,
                           pReq->SetupCount,
                           pReq->Reserved3);

        pSetup = NEXT_LOCATION(pReq, REQ_TRANSACTION, 0);
        for (iSetup = 0; iSetup < pReq->SetupCount; iSetup++)
            buffer += wsprintf(buffer,
                               "Setup[0x%02x]    : 0x%04x (%s)\n",
                               iSetup, pSetup[iSetup],
                               SmbUnparseTrans2(pSetup[iSetup]));
        ByteCount = *(PUSHORT)NEXT_LOCATION(pReq, REQ_TRANSACTION, 
                                            pReq->SetupCount*sizeof(USHORT));
        buffer += wsprintf(buffer,
                           "ByteCount      : 0x%02x\n",
                           ByteCount);

        pParameter = (PUCHAR) pSmb + pReq->ParameterOffset;
        pData = (PUCHAR) pSmb + pReq->DataOffset;
        for (iSetup = 0; iSetup < pReq->SetupCount; iSetup++)
            buffer = Trans2Dispatch(pSetup[iSetup],
                                    buffer,
                                    pSmb,
                                    dwSize,
                                    dwOffset,
                                    bRequest,
                                    &pParameter,
                                    &pData);
    } else {
        PRESP_TRANSACTION pResp = pData;
        PUSHORT pSetup;
        PUCHAR pParameter, pData;
        USHORT ByteCount;
        int iSetup;

        if (pResp->WordCount) {
            buffer += wsprintf(buffer,
                               "WordCount      : 0x%02x\n"
                               "TotalParamCount: 0x%04x\n"
                               "TotalDataCount : 0x%04x\n"
                               "Reserved       : 0x%04x\n"
                               "ParameterCount : 0x%04x\n"
                               "ParameterOffset: 0x%04x\n"
                               "ParameterDisp  : 0x%04x\n"
                               "DataCount      : 0x%04x\n"
                               "DataOffset     : 0x%04x\n"
                               "DataDisp       : 0x%04x\n"
                               "SetupCount     : 0x%02x\n"
                               "Reserved2      : 0x%02x\n",
                               pResp->WordCount,
                               pResp->TotalParameterCount,
                               pResp->TotalDataCount,
                               pResp->Reserved,
                               pResp->ParameterCount,
                               pResp->ParameterOffset,
                               pResp->ParameterDisplacement,
                               pResp->DataCount,
                               pResp->DataOffset,
                               pResp->DataDisplacement,
                               pResp->SetupCount,
                               pResp->Reserved2);

            pSetup = NEXT_LOCATION(pResp, RESP_TRANSACTION, 0);
            for (iSetup = 0; iSetup < pResp->SetupCount; iSetup++)
                buffer += wsprintf(buffer,
                                   "Setup[0x%02x]    : 0x%04x (%s)\n",
                                   iSetup, pSetup[iSetup],
                                   SmbUnparseTrans2(pSetup[iSetup]));
            ByteCount = *(PUSHORT)NEXT_LOCATION(pResp, RESP_TRANSACTION, 
                                                pResp->SetupCount*
                                                sizeof(USHORT));
            buffer += wsprintf(buffer,
                               "ByteCount      : 0x%02x\n",
                               ByteCount);

            pParameter = (PUCHAR) pSmb + pResp->ParameterOffset;
            pData = (PUCHAR) pSmb + pResp->DataOffset;
            for (iSetup = 0; iSetup < pResp->SetupCount; iSetup++)
                buffer = Trans2Dispatch(pSetup[iSetup],
                                        buffer,
                                        pSmb,
                                        dwSize,
                                        dwOffset,
                                        bRequest,
                                        &pParameter,
                                        &pData);
        } else {
            buffer += wsprintf(buffer, "<interim>\n");
        }
    }
    return buffer;
}
