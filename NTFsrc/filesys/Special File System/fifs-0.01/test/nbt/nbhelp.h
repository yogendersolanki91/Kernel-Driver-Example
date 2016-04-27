/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   NetBIOS tester helper header

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

#define MAX_SESSIONS 0xFF
#define MAX_NAMES 0xFF

#define NBCheck(x)  if (NRC_GOODRET != (x)->ncb_retcode) { \
                        printf("%s Line %d: Got 0x%x from NetBios()\n", \
                               __FILE__, __LINE__, (x)->ncb_retcode); \
                    }

BOOL
NBReset(
    int nLana, 
    int nSessions, 
    int nNames
    );

BOOL
NBAddName(
    int nLana, 
    LPCSTR szName
    );

BOOL
NBListNames(
    int nLana,
    LPCSTR szName
    );

BOOL
NBAdapterStatus(
    int nLana,
    PVOID pBuffer,
    int cbBuffer,
    LPCSTR szName
    );

BOOL
NBEnumLana(
    int nLana
    );

VOID
NBDump(
    PNCB pncb
    );
