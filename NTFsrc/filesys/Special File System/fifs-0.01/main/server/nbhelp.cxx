/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   server NetBIOS helper

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
#include "nbhelp.hxx"

//#define USE_NBDUMP
//#define USE_NBCHECK

// ---------------------------- NBCHECK -------------------------------
#ifdef USE_NBCHECK
#define NBCheck(code) if ((code) != NRC_GOODRET) { \
                        printf("%s Line %d: Got 0x%x from NetBios()\n", \
                        __FILE__, __LINE__, (code)); \
                    }
#else
#define NBCheck(x)
#endif

// ---------------------------- NBDUMP -------------------------------
#ifdef USE_NBDUMP
#include <stdio.h>

static void
NBDumpName(
    PCHAR pName
    )
{
    PCHAR pEnd = pName + NCBNAMSZ;
    putchar('"');
    while (pName != pEnd)
        putchar(*(pName++));
    putchar('"');
}

static void
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
    NBDumpName((PCHAR)pncb->ncb_callname);
    printf("\nncb_name: ");
    NBDumpName((PCHAR)pncb->ncb_name);
    putchar('\n');
    PDUMP(pncb, ncb_rto, %d);
    PDUMP(pncb, ncb_sto, %d);
    PDUMP(pncb, ncb_post, %d);
    PDUMP(pncb, ncb_lana_num, %d);
    PDUMP(pncb, ncb_cmd_cplt, %d);
//    PDUMP(pncb, ncb_reserve, %s);
    PDUMP(pncb, ncb_event, %d);
}
#else
#define NBDump(x)
#endif

// -------------------------------------------------------------------

static
void
InitializeNCB(
    PNCB pncb,
    UCHAR command,
    UCHAR nLana,
    UCHAR lsn,
    NETBIOS_NAME callname,
    NETBIOS_NAME name
    )
{
    memset(pncb, 0, sizeof(NCB));
    pncb->ncb_command = command;
    pncb->ncb_lana_num = nLana;
    pncb->ncb_lsn = lsn;
    if (callname) {
        COPY_NETBIOS_NAME(pncb->ncb_callname, callname);
    }
    if (name) {
        COPY_NETBIOS_NAME(pncb->ncb_name, name);
    }
}

//
// MakeNetbiosName()
//
// Build a name of length NCBNAMSZ, padding with spaces
//

void
MakeNetbiosName(
    NETBIOS_NAME nbDest,
    LPCSTR szSrc
    )
{
    int cchSrc;

    cchSrc = lstrlen (szSrc);
    if (cchSrc > NCBNAMSZ)
        cchSrc = NCBNAMSZ;

    memset (nbDest, ' ', NCBNAMSZ);
    memcpy (nbDest, szSrc, cchSrc);
}

void
SetNetbiosNameType(
    NETBIOS_NAME nbName,
    UCHAR chType
    )
{
    nbName[NCBNAMSZ-1] = chType;
}

UCHAR
NBFindFirstLanaWithName(
    LPCSTR szName,
    PUCHAR pLana
    )
{
    NETBIOS_NAME nbName;
    MakeNetbiosName(nbName, szName);
    return NBFindFirstLanaWithNameN(nbName, pLana);
}

UCHAR
NBFindFirstLanaWithNameN(
    NETBIOS_NAME nbName,
    PUCHAR pLana
    )
{
    LANA_ENUM lana_enum;
    NCB ncb;
    const WORD len = sizeof(FIND_NAME_HEADER) + 255*sizeof(FIND_NAME_BUFFER);
    UCHAR buffer[len];

    UCHAR code = NBEnumLana(&lana_enum);
    if (code == NRC_GOODRET) {
        for (int i = 0; i < lana_enum.length; i++) {
            NBReset(lana_enum.lana[i], 0xFF, 0xFF);
            memset(&ncb, 0, sizeof(NCB));
            ncb.ncb_command = NCBFINDNAME;
            ncb.ncb_buffer = buffer;
            ncb.ncb_length = len;
            COPY_NETBIOS_NAME(ncb.ncb_callname, nbName);
            code = Netbios(&ncb);
            if (code == NRC_GOODRET) {
                if (pLana)
                    *pLana = lana_enum.lana[i];
                return code;
            }
        }
    }
    return code;
}

UCHAR
NBEnumLana(
    PLANA_ENUM pLanaEnum
    )
{
    NCB ncb;
    UCHAR code;

    memset(&ncb, 0, sizeof(ncb));
    ncb.ncb_command = NCBENUM;

    ncb.ncb_buffer = (PUCHAR) pLanaEnum;
    ncb.ncb_length = sizeof(LANA_ENUM);

    code = Netbios (&ncb);
    NBCheck(code);

    return code;
}


UCHAR
NBReset(
    UCHAR nLana, 
    UCHAR nSessions, 
    UCHAR nNames
    )
{
    NCB ncb;
    UCHAR code;

    memset (&ncb, 0, sizeof (ncb));
    ncb.ncb_command = NCBRESET;
    ncb.ncb_lsn = 0;                // Allocate new lana_num resources 
    ncb.ncb_lana_num = nLana;
    ncb.ncb_callname[0] = nSessions;  // maximum sessions 
    ncb.ncb_callname[2] = nNames;   // maximum names 

    code = Netbios (&ncb);
    NBCheck(code)

    return code;
}

UCHAR
NBAddName(
    UCHAR nLana, 
    LPCSTR szName,
    PUCHAR pnum
    )
{
    NETBIOS_NAME nbName;
    MakeNetbiosName(nbName, szName);
    return NBAddNameN(nLana, nbName, pnum);
}

UCHAR
NBAddNameN(
    UCHAR nLana,
    NETBIOS_NAME nbName,
    PUCHAR pnum
    )
{
    NCB ncb;
    UCHAR code;

    memset (&ncb, 0, sizeof (ncb));
    ncb.ncb_command = NCBADDNAME;
    ncb.ncb_lana_num = nLana;
    memcpy(ncb.ncb_name, nbName, NCBNAMSZ);

    code = Netbios (&ncb);
    NBCheck(code);

    if (pnum && NB_SUCCESS(code))
        *pnum = ncb.ncb_num;
    return code;
}

static
UCHAR
NBConnectN(
    BOOL bCall, // Call = TRUE, Listen = FALSE
    UCHAR nLana, 
    NETBIOS_NAME nbRemoteName,
    NETBIOS_NAME nbLocalName,
    UCHAR nLocalName,
    PUCHAR plsn,
    HANDLE hEvent,
    PNCB pncb
    )
{
    NCB ncb;
    UCHAR code;

    if (!pncb) pncb = &ncb;
    memset(pncb, 0, sizeof(NCB));
    pncb->ncb_command = bCall?NCBCALL:NCBLISTEN;
    pncb->ncb_lana_num = nLana;
    memcpy(pncb->ncb_callname, nbRemoteName, NCBNAMSZ);
    if (nbLocalName)
        memcpy(pncb->ncb_name, nbLocalName, NCBNAMSZ);
    else
        pncb->ncb_num = nLocalName;
    pncb->ncb_event = hEvent;
    pncb->ncb_command |= hEvent?ASYNCH:0;

    NBDump (pncb);
    code = Netbios (pncb);
    NBCheck(code);
    NBDump (pncb);
    if (plsn && NB_SUCCESS(code))
        *plsn = pncb->ncb_lsn;
    return code;
}

static
UCHAR
NBConnect(
    BOOL bCall, // Call = TRUE, Listen = FALSE
    UCHAR nLana, 
    LPCSTR szRemoteName,
    LPCSTR szLocalName,
    UCHAR nLocalName,
    PUCHAR plsn,
    HANDLE hEvent,
    PNCB pncb
    )
{
    NETBIOS_NAME nbRemoteName;
    NETBIOS_NAME nbLocalName;
    PUCHAR pLocalName = NULL;
    MakeNetbiosName(nbRemoteName, szRemoteName);
    if (szLocalName) {
        MakeNetbiosName(nbLocalName, szLocalName);
        pLocalName = nbLocalName;
    }
    return NBConnectN(bCall, nLana, nbRemoteName, pLocalName, nLocalName, plsn, hEvent, pncb);
}

UCHAR
NBListen(
    UCHAR nLana, 
    LPCSTR szRemoteName,
    LPCSTR szLocalName,
    UCHAR nLocalName,
    PUCHAR plsn
    )
{
    return NBConnect(FALSE, nLana, szRemoteName, szLocalName, nLocalName, plsn, NULL, NULL);
}

UCHAR
NBListenN(
    UCHAR nLana, 
    NETBIOS_NAME nbRemoteName,
    NETBIOS_NAME nbLocalName,
    UCHAR nLocalName,
    PUCHAR plsn
    )
{
    return NBConnectN(FALSE, nLana, nbRemoteName, nbLocalName, nLocalName, plsn, NULL, NULL);
}

UCHAR
NBListenEx(
    UCHAR nLana, 
    LPCSTR szRemoteName,
    LPCSTR szLocalName,
    UCHAR nLocalName,
    PUCHAR plsn,
    HANDLE hEvent,
    PNCB pncb
    )
{
    return NBConnect(FALSE, nLana, szRemoteName, szLocalName, nLocalName, plsn, hEvent, pncb);
}

UCHAR
NBListenExN(
    UCHAR nLana, 
    NETBIOS_NAME nbRemoteName,
    NETBIOS_NAME nbLocalName,
    UCHAR nLocalName,
    PUCHAR plsn,
    HANDLE hEvent,
    PNCB pncb
    )
{
    return NBConnectN(FALSE, nLana, nbRemoteName, nbLocalName, nLocalName, plsn, hEvent, pncb);
}

UCHAR
NBCall(
    UCHAR nLana, 
    LPCSTR szRemoteName,
    LPCSTR szLocalName,
    UCHAR nLocalName,
    PUCHAR plsn
    )
{
    return NBConnect(TRUE, nLana, szRemoteName, szLocalName, nLocalName, plsn, NULL, NULL);
}

UCHAR
NBCallN(
    UCHAR nLana, 
    NETBIOS_NAME nbRemoteName,
    NETBIOS_NAME nbLocalName,
    UCHAR nLocalName,
    PUCHAR plsn
    )
{
    return NBConnectN(TRUE, nLana, nbRemoteName, nbLocalName, nLocalName, plsn, NULL, NULL);
}

UCHAR
NBHangup(
    UCHAR nLana,
    UCHAR lsn
    )
{
    NCB ncb;
    UCHAR code;

    memset(&ncb, 0, sizeof(ncb));
    ncb.ncb_command = NCBHANGUP;
    ncb.ncb_lana_num = nLana;
    ncb.ncb_lsn = lsn;

    NBDump (&ncb);
    code = Netbios (&ncb);
    NBCheck(code);
    NBDump (&ncb);

    return code;
}

static
UCHAR
NBXfer(
    BOOL bSend, // Send = TRUE, Recv = FALSE
    UCHAR nLana,
    UCHAR lsn,
    PVOID buffer,
    PWORD plength,
    HANDLE hEvent,
    PNCB pncb
    )
{
    NCB ncb;
    UCHAR code;

    if (!pncb) pncb = &ncb;
    memset(pncb, 0, sizeof (NCB));
    pncb->ncb_command = bSend?NCBSEND:NCBRECV;
    pncb->ncb_lana_num = nLana;
    pncb->ncb_lsn = lsn;
    pncb->ncb_buffer = (PUCHAR)buffer;
    pncb->ncb_length = *plength;
    pncb->ncb_event = hEvent;
    pncb->ncb_command |= hEvent?ASYNCH:0;

    NBDump (pncb);
    code = Netbios(pncb);
    NBCheck(code);
    NBDump (pncb);

    if (!hEvent)
        *plength = pncb->ncb_length;
    return code;
}

UCHAR
NBRecv(
    UCHAR nLana, 
    UCHAR lsn,
    PVOID buffer,
    PWORD plength
    )
{
    return NBXfer(FALSE, nLana, lsn, buffer, plength, NULL, NULL);
}

UCHAR
NBRecvEx(
    UCHAR nLana,
    UCHAR lsn,
    PVOID buffer,
    PWORD plength,
    HANDLE hEvent,
    PNCB pncb
    )
{
    return NBXfer(FALSE, nLana, lsn, buffer, plength, hEvent, pncb);
}

UCHAR
NBSend(
    UCHAR nLana, 
    UCHAR lsn,
    PVOID buffer,
    PWORD plength
    )
{
    return NBXfer(TRUE, nLana, lsn, buffer, plength, NULL, NULL);
}

void
NBCancel(
    PNCB pncb
    )
{
    NCB ncb;
    UCHAR code;

    memset(&ncb, 0, sizeof(NCB));
    ncb.ncb_command = NCBCANCEL;
    ncb.ncb_buffer = (PUCHAR)pncb;
    ncb.ncb_length = sizeof(NCB);

    NBDump (&ncb);
    code = Netbios(&ncb);
    NBCheck(code);
    NBDump (&ncb);
}
