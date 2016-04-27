/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   server NetBIOS helper header

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

#ifndef __NBHELP_HXX__
#define __NBHELP_HXX__

#define NB_SUCCESS(x) ((x) == NRC_GOODRET)
#define NB_BADSESSION(x) (((x) == NRC_SNUMOUT) || ((x) == NRC_SCLOSED))

#define COPY_NETBIOS_NAME(to, from) CopyMemory((PVOID)to,(PVOID)from,NCBNAMSZ)

#ifdef __cplusplus__
extern "C" {
#endif

typedef UCHAR NETBIOS_NAME[NCBNAMSZ];

void
MakeNetbiosName(
    NETBIOS_NAME nbDest,
    LPCSTR szSrc
    );

void
SetNetbiosNameType(
    NETBIOS_NAME nbName,
    UCHAR chType
    );

UCHAR
NBFindFirstLanaWithName(
    LPCSTR szName,
    PUCHAR pLana
    );

UCHAR
NBFindFirstLanaWithNameN(
    NETBIOS_NAME nbName,
    PUCHAR pLana
    );

UCHAR
NBEnumLana(
    PLANA_ENUM pLanaEnum
    );

UCHAR
NBReset(
    UCHAR nLana, 
    UCHAR nSessions, 
    UCHAR nNames
    );

UCHAR
NBAddName(
    UCHAR nLana, 
    LPCSTR szName,
    PUCHAR pnum
    );

UCHAR
NBAddNameN(
    UCHAR nLana, 
    NETBIOS_NAME nbName,
    PUCHAR pnum
    );

UCHAR
NBListen(
    UCHAR nLana, 
    LPCSTR szRemoteName,
    LPCSTR szLocalName,
    UCHAR nLocalName,
    PUCHAR plsn,
    HANDLE hEvent
    );

UCHAR
NBListenN(
    UCHAR nLana, 
    NETBIOS_NAME nbRemoteName,
    NETBIOS_NAME nbLocalName,
    UCHAR nLocalName,
    PUCHAR plsn,
    HANDLE hEvent
    );

UCHAR
NBListenEx(
    UCHAR nLana, 
    LPCSTR szRemoteName,
    LPCSTR szLocalName,
    UCHAR nLocalName,
    PUCHAR plsn,
    HANDLE hEvent,
    PNCB pncb
    );

UCHAR
NBListenExN(
    UCHAR nLana, 
    NETBIOS_NAME nbRemoteName,
    NETBIOS_NAME nbLocalName,
    UCHAR nLocalName,
    PUCHAR plsn,
    HANDLE hEvent,
    PNCB pncb
    );

UCHAR
NBCall(
    UCHAR nLana, 
    LPCSTR szRemoteName,
    LPCSTR szLocalName,
    UCHAR nLocalName,
    PUCHAR plsn
    );

UCHAR
NBCallN(
    UCHAR nLana, 
    NETBIOS_NAME nbRemoteName,
    NETBIOS_NAME nbLocalName,
    UCHAR nLocalName,
    PUCHAR plsn
    );

UCHAR
NBHangup(
    UCHAR nLana,
    UCHAR lsn
    );

UCHAR
NBRecv(
    UCHAR nLana, 
    UCHAR lsn,
    PVOID buffer,
    PWORD plength
    );

UCHAR
NBRecvEx(
    UCHAR nLana,
    UCHAR lsn,
    PVOID buffer,
    PWORD plength,
    HANDLE hEvent,
    PNCB pncb
    );

UCHAR
NBSend(
    UCHAR nLana, 
    UCHAR lsn,
    PVOID buffer,
    PWORD plength
    );

void
NBCancel(
    PNCB pncb
    );

#ifdef __cplusplus__
}
#endif

#endif /* __NBHELP_HXX__ */
