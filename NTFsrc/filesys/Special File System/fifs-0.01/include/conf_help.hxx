/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   Configuration Helper header

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

#ifndef __CONFIG_HELP_HXX__
#define __CONFIG_HELP_HXX__

typedef char* subst_table_t[256];

struct config_string_params {
    const char* name;
    const char* def;
    int max_size;        // -1 for unlimited (includes NULL byte)
};

struct config_ranged_num_params {
    const char* name;
    DWORD def;
    DWORD min;
    DWORD max;
};

struct __config_handle_t;

typedef __config_handle_t* config_handle_t;

const char*
ConfigOpen(
    const char* config_path,
    const char* default_config_path,
    config_handle_t& h,
    DWORD& error
    );

void
ConfigClose(
    config_handle_t h
    );

char*
ConfigAllocPath(
    const char* base_path,
    const char* path_candidate
    );

void
ConfigFreePath(
    char* path
    );

bool
ConfigDefRangedNumber(
    config_handle_t h,
    const config_ranged_num_params& p,
    DWORD& value
    );

bool
ConfigDefString(
    config_handle_t h,
    const config_string_params& p,
    subst_table_t table, // 0 for no subst
    char*& target        // where to put string pointer
    );

void
ConfigUndefString(
    char* str
    );

DWORD
ConfigGetSizedData(
    config_handle_t h,
    const char* name,
    void* pData,
    DWORD* pSize
    );

DWORD
ConfigGetData(
    config_handle_t h,
    const char* name,
    void** ppData,
    DWORD* pSize
    );

void
ConfigFreeData(
    void* pData
    );

int
substitute_vars(
    const char* string_template,
    char* buffer,
    int size,           // put 0 here to get necessary size
    subst_table_t table
    );


#endif /* __CONFIG_HELP_HXX__ */
