/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   NetBIOS control block utilities

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
#include "ncbutil.h"
#include <assert.h>

PNCB
AllocateNCB(
    )
{
    return malloc(sizeof(NCB));
}

VOID
FreeNCB(
    PNCB pncb
    )
{
    free(pncb);
}

// Build a name of length NCBNAMSZ, padding with spaces.
void
MakeNetbiosName(
    NETBIOS_NAME achDest,
    LPCSTR szSrc
    )
{
    int cchSrc;

    cchSrc = lstrlen (szSrc);
    if (cchSrc > NCBNAMSZ)
        cchSrc = NCBNAMSZ;

    memset (achDest, ' ', NCBNAMSZ);
    memcpy (achDest, szSrc, cchSrc);
}

VOID
InitializeNCB(
    PNCB pncb,
    int command,
    int nLana,
    int lsn,
    NETBIOS_NAME callname,
    NETBIOS_NAME name
    )
{
    assert(pncb);
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
