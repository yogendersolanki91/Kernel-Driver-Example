/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   NetBIOS tester helper

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

#include <windows.h>
#include <nb30.h>
#include "nbhelp.h"
#include "ncbutil.h"
#include <stdio.h>

BOOL
NBReset(
    int nLana, 
    int nSessions, 
    int nNames
    )
{
    NCB ncb;

    memset (&ncb, 0, sizeof (ncb));
    ncb.ncb_command = NCBRESET;
    ncb.ncb_lsn = 0;                // Allocate new lana_num resources 
    ncb.ncb_lana_num = nLana;
    ncb.ncb_callname[0] = nSessions;  // maximum sessions 
    ncb.ncb_callname[2] = nNames;   // maximum names 

    Netbios (&ncb);
    NBCheck (&ncb);

    return (NRC_GOODRET == ncb.ncb_retcode);
}

BOOL
NBAddName(
    int nLana, 
    LPCSTR szName
    )
{
    NCB ncb;

    memset (&ncb, 0, sizeof (ncb));
    ncb.ncb_command = NCBADDNAME;
    ncb.ncb_lana_num = nLana;

    MakeNetbiosName (ncb.ncb_name, szName);

    Netbios (&ncb);
    NBCheck (&ncb);

    return (NRC_GOODRET == ncb.ncb_retcode);
}

BOOL
NBListNames(
    int nLana,
    LPCSTR szName
    )
{
    int cbBuffer;
    ADAPTER_STATUS *pStatus;
    NAME_BUFFER *pNames;
    int i;
    HANDLE hHeap;

    hHeap = GetProcessHeap();

    // Allocate the largest buffer that might be needed. 
    cbBuffer = sizeof (ADAPTER_STATUS) + 255 * sizeof (NAME_BUFFER);
    pStatus = (ADAPTER_STATUS *) HeapAlloc (hHeap, 0, cbBuffer);
    if (NULL == pStatus)
        return FALSE;

    if (!NBAdapterStatus (nLana, (PVOID) pStatus, cbBuffer, szName))
    {
        HeapFree (hHeap, 0, pStatus);
        return FALSE;
    }

    // The list of names follows the adapter status structure.
    pNames = (NAME_BUFFER *) (pStatus + 1);

    for (i = 0; i < pStatus->name_count; i++)
        printf ("\t%.*s\n", NCBNAMSZ, pNames[i].name);

    HeapFree (hHeap, 0, pStatus);

    return TRUE;
}

BOOL
NBAdapterStatus(
    int nLana,
    PVOID pBuffer,
    int cbBuffer,
    LPCSTR szName
    )
{
    NCB ncb;

    memset (&ncb, 0, sizeof (ncb));
    ncb.ncb_command = NCBASTAT;
    ncb.ncb_lana_num = nLana;

    ncb.ncb_buffer = (PUCHAR) pBuffer;
    ncb.ncb_length = cbBuffer;

    MakeNetbiosName (ncb.ncb_callname, szName);

    Netbios (&ncb);
    NBCheck (&ncb);

    return (NRC_GOODRET == ncb.ncb_retcode);
}

BOOL
NBEnumLana(
    int nLana
    )
{
    NCB ncb;
    LANA_ENUM lana_enum;
    int i;

    memset(&ncb, 0, sizeof(ncb));
    ncb.ncb_command = NCBENUM;
    ncb.ncb_lana_num = nLana;

    ncb.ncb_buffer = (PUCHAR) &lana_enum;
    ncb.ncb_length = sizeof(lana_enum);

    Netbios (&ncb);
    NBCheck (&ncb);

    if (NRC_GOODRET == ncb.ncb_retcode) {
        for (i = 0; i < lana_enum.length; i++) {
            printf("lana[%d]: %d\n", i, lana_enum.lana[i]);
        }
        return TRUE;
    }
    return FALSE;
}

VOID
NBDumpName(
    PCHAR pName
    )
{
    PCHAR pStart = pName;
    PCHAR pEnd = pName + NCBNAMSZ;
    putchar('"');
    while (pName != pEnd)
        putchar(*(pName++));
    putchar('"');
    putchar(' ');
    putchar('(');
    pName = pStart;
    while (pName != pEnd) {
        printf("%02x", *(pName++));
        if (pName != pEnd) putchar(' ');
    }
    putchar(')');
}

VOID
NBDump(
    PNCB pncb
    )
{
#define GDUMP(structure, accessor, member, type) \
    printf(#member ": " #type "\n", structure##accessor##member);

#define DUMP(structure, member, type)  GDUMP(structure, ., member, type)
#define PDUMP(structure, member, type) GDUMP(structure, ->, member, type)

    puts("--- NCB ---");
    PDUMP(pncb, ncb_command, %d);
    PDUMP(pncb, ncb_retcode, %d);
    PDUMP(pncb, ncb_lsn, %d);
    PDUMP(pncb, ncb_num, %d);
    PDUMP(pncb, ncb_buffer, %d);
    PDUMP(pncb, ncb_length, %d);
    printf("ncb_callname: ");
    NBDumpName(pncb->ncb_callname);
    printf("\nncb_name: ");
    NBDumpName(pncb->ncb_name);
    putchar('\n');
    PDUMP(pncb, ncb_rto, %d);
    PDUMP(pncb, ncb_sto, %d);
    PDUMP(pncb, ncb_post, %d);
    PDUMP(pncb, ncb_lana_num, %d);
    PDUMP(pncb, ncb_cmd_cplt, %d);
//    PDUMP(pncb, ncb_reserve, %s);
    PDUMP(pncb, ncb_event, %d);
}
