/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   SMB-FIFS glue header

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

#ifndef __SMB_GLUE_HXX__
#define __SMB_GLUE_HXX__

#include "fsinterface.hxx"
#include "filter.hxx"

using namespace FsInterface;

// user/uid

bool
is_authenticated_user(
    const char *name, 
    void* pwd,
    int pwd_size
    );

USHORT
add_user(
    const char *name,
    FsDispatchTable* pDisp
    );

FsDispatchTable*
get_user(
    USHORT uid
    );

bool
del_user(
    USHORT uid
    );

// find/sid

USHORT
add_find(
    fhandle_t fh,
    UINT32 cookie,
    filter_t* filter,
    dirinfo_t* entry
    );

bool
save_find_cookie(
    USHORT sid,
    UINT32 cookie
    );

bool
get_find(
    USHORT sid,
    fhandle_t* pfh,
    UINT32* pcookie,
    filter_t** pfilter,
    dirinfo_t** pentry
    );

bool
del_find(
    USHORT sid
    );

// file/fid

USHORT
add_file(
    fhandle_t fh
    );

fhandle_t
get_file(
    USHORT fid
    );

bool
del_file(
    USHORT fid
    );

// init/cleanup

void
smbglue_init(
    void
    );

void
smbglue_reset(
    void
    );

void
smbglue_cleanup(
    void
    );

#endif
