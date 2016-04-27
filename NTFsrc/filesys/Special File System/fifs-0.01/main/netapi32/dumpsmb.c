/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   NetBIOS snooper dumper

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
#include "dumpsmb.h"
#include "config.h"

IntHash ht = NULL;

LPSTR
DumpData(
    LPSTR buffer,
    PUCHAR data,
    DWORD size
    )
{
#define line_size 16
    DWORD i;
    UCHAR line_buffer[line_size + 1];
    UCHAR current = line_buffer[line_size] = '\0';

    RtlZeroMemory(line_buffer, line_size);

    for (i = 0; i < size; i++) {
        current = data[i];
        if (!(i % line_size)) {
            if (i) {
                buffer += wsprintf(buffer, "  %s\n", line_buffer);
            }
            RtlZeroMemory(line_buffer, line_size);
            buffer += wsprintf(buffer, "0x%08x : ", i);
        }
        buffer += wsprintf(buffer, " %02x", (int) current);
        line_buffer[i % line_size] = isprint(current)?current:'.';
    }
    i %= line_size;
    for (i = line_size - (i?i:line_size); i > 0; i--) {
        buffer += wsprintf(buffer, "   ");
    }
    buffer += wsprintf(buffer, "  %s\n", line_buffer);
    return buffer;
}

LPSTR
DumpSMB(
    LPSTR buffer,
    PVOID psmb,
    DWORD dwSize,
    BOOL bRequest
    )
{
    PNT_SMB_HEADER pSmb = (PNT_SMB_HEADER) psmb;
    if (!IsSmb(pSmb, dwSize)) {
        buffer += wsprintf(buffer, "NOT AN SMB PACKET\n");
        return buffer;
    }
    buffer += wsprintf(buffer, "--- SMB HEADER --- (%s)\n",
                       bRequest?"CLIENT REQUEST":"SERVER RESPONSE");
    buffer += wsprintf(buffer,
                       "Command : 0x%02x (%s)\n"
                       "NtStatus: 0x%08x\n"
                       "Flags   : 0x%02x\n"
                       "Flags2  : 0x%04x\n"
                       "PidHigh : 0x%04x\n"
                       "Key     : 0x%08x\n"
                       "Sid     : 0x%04x\n"
                       "SeqNum  : 0x%04x\n"
                       "Gid     : 0x%04x\n"
                       "Tid     : 0x%04x\n"
                       "Pid     : 0x%04x\n"
                       "Uid     : 0x%04x\n"
                       "Mid     : 0x%04x\n",
                       pSmb->Command,
                       SmbUnparseCommand(pSmb->Command),
                       pSmb->Status.NtStatus,
                       pSmb->Flags,
                       pSmb->Flags2,
                       pSmb->PidHigh,
                       pSmb->Key,
                       pSmb->Sid,
                       pSmb->SequenceNumber,
                       pSmb->Gid,
                       pSmb->Tid,
                       pSmb->Pid,
                       pSmb->Uid,
                       pSmb->Mid);
    buffer = SmbDispatch(pSmb->Command, buffer, pSmb, dwSize, 
                         sizeof(NT_SMB_HEADER), bRequest);
    return buffer;
}

LPSTR
DumpNCB(
    LPSTR buffer,
    PNCB pncb,
    BOOL bIn
    )
{
    UCHAR command = pncb->ncb_command & (bIn?~ASYNCH:0xFF);

    if (config.bShowTime) {
        SYSTEMTIME SystemTime;
        GetLocalTime(&SystemTime);
        buffer += wsprintf(buffer, 
                           "[%02d/%02d/%04d - %02d:%02d:%02d.%03d] <%s>\n", 
                           SystemTime.wMonth,
                           SystemTime.wDay,
                           SystemTime.wYear,
                           SystemTime.wHour,
                           SystemTime.wMinute,
                           SystemTime.wSecond,
                           SystemTime.wMilliseconds,
                           bIn?"IN":"OUT");
    }

    if (config.bShowHeader) {
        buffer += wsprintf(buffer, "--- NCB ---\n");
        buffer += wsprintf(buffer,
                           "ncb_command : 0x%02x (%s - %s)\n"
                           "ncb_retcode : 0x%02x (%s)\n"
                           "ncb_lsn     : %d\n"
                           "ncb_num     : %d\n"
                           "ncb_buffer  : %d\n"
                           "ncb_length  : %d\n"
                           "ncb_callname: ",
                           pncb->ncb_command,
                           NcbUnparseCommand(pncb->ncb_command),
                           NcbUnparseAsynch(pncb->ncb_command),
                           pncb->ncb_retcode,
                           NcbUnparseRetCode(pncb->ncb_retcode),
                           pncb->ncb_lsn,
                           pncb->ncb_num,
                           pncb->ncb_buffer,
                           pncb->ncb_length);
        if (pncb->ncb_command == NCBRESET) {
            wsprintf(buffer,
                     "(%s: Max. Sessions = %d, Max. Names = %d)",
                     pncb->ncb_lsn?"CLEAR":"RESIZE",
                     pncb->ncb_callname[0],
                     pncb->ncb_callname[2]);
            buffer += lstrlen(buffer);
        } else {
            lstrcpyn(buffer, pncb->ncb_callname, NCBNAMSZ);
            buffer += min(lstrlen(buffer), NCBNAMSZ);
        }
        buffer += wsprintf(buffer,
                           "\n"
                           "ncb_name    : ");
        if (pncb->ncb_command == NCBRESET) {
            wsprintf(buffer,
                     "(%s: Max. Sessions = %d, Max. Names = %d)",
                     pncb->ncb_lsn?"CLEAR":"RESIZE",
                     pncb->ncb_name[0],
                     pncb->ncb_name[2]);
            buffer += lstrlen(buffer);
        } else {
            lstrcpyn(buffer, pncb->ncb_name, NCBNAMSZ);
            buffer += min(lstrlen(buffer), NCBNAMSZ);
        }
        buffer += wsprintf(buffer,
                           "\n"
                           "ncb_rto     : %d\n"
                           "ncb_sto     : %d\n"
                           "ncb_post    : %d\n"
                           "ncb_lana_num: %d\n"
                           "ncb_cmd_cplt: 0x%02x (%s)\n"
//                       "ncb_reserve : ?\n"
                           "ncb_event   : 0x%08x\n",
                           pncb->ncb_rto,
                           pncb->ncb_sto,
                           pncb->ncb_post,
                           pncb->ncb_lana_num,
                           pncb->ncb_cmd_cplt,
                           NcbUnparseRetCode(pncb->ncb_cmd_cplt),
                           pncb->ncb_event);
    }

    if (pncb->ncb_buffer && pncb->ncb_length) {
        if ((!bIn && ((command == NCBRECV) ||
                      (command == NCBRECVANY) ||
                      (command == NCBDGRECV) ||
                      (command == NCBDGRECVBC))) ||
            ( bIn && ((command == NCBSEND) ||
                      (command == NCBCHAINSEND) ||
                      (command == NCBDGSEND) ||
                      (command == NCBDGSEND))))
        {

            if (config.bShowBuffer) {
                if (config.bShowBufferIffSmb && !IsSmb(pncb->ncb_buffer, pncb->ncb_length)) {
                    buffer += wsprintf(buffer, "NOT AN SMB PACKET\n");
                    return buffer;
                }
                buffer = DumpData(buffer, 
                                  pncb->ncb_buffer, pncb->ncb_length);
            }
            if (config.bShowSmb)
                buffer = DumpSMB(buffer, pncb->ncb_buffer, 
                                 pncb->ncb_length, !bIn);
//        } else if (config.bShowBuffer) {
//            buffer += wsprintf(buffer, 
//                               "DO NOT KNOW WHAT TO DO WITH BUFFER\n");
        }
    }
    return buffer;
}

VOID
InitDump(
    )
{
    ht = IntHash_New(67); // a prime...
    SmbInitDispTable();
}

VOID
CleanupDump(
    )
{
    IntHash_Free(ht);
    ht = NULL;
}
