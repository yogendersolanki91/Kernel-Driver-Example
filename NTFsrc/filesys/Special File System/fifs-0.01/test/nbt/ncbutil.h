/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   NetBIOS control block utilities header

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

#ifndef __NCB_UTIL_H__
#define __NCB_UTIL_H__

#ifdef __cplusplus__
extern "C" {
#endif

typedef UCHAR NETBIOS_NAME[NCBNAMSZ];

#define COPY_NETBIOS_NAME(to, from) memcpy(to, from, NCBNAMSZ)

PNCB
AllocateNCB(
    );

VOID
FreeNCB(
    PNCB pncb
    );

void
MakeNetbiosName(
    NETBIOS_NAME achDest,
    LPCSTR szSrc
    );

VOID
InitializeNCB(
    PNCB pncb,
    int command,
    int nLana,
    int lsn,
    NETBIOS_NAME callname,
    NETBIOS_NAME name
    );

#ifdef __cplusplus__
}
#endif

#endif /* __NCB_UTIL_H__ */
