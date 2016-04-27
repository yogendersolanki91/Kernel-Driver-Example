/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   filter object

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
#include "filter.hxx"
#include <debug.hxx>

filter_t::filter_t(
    const char* str
    ):wilds(false),all(false),s(0)
{
    if (!str) return;

    const char* start = str;
    while (*str) {
        wilds = wilds || ((*str) == '*') || ((*str) == '?');
        str++;
    }
    all = wilds && ((str - start) == 1);
    s = new char[str - start + 1];
    lstrcpy(s, start);
}

// filter_t::filter_t(
//     const filter_t& f
//     ):wilds(f.wilds),all(f.all),s(0)
// {
//     if (f.s) {
//         s = new char[lstrlen(f.s) + 1];
//         lstrcpy(s, f.s);
//     }
// }

const
char*
filter_t::value()
{
    return (const char*)s;
}

bool
filter_t::is_literal()
{
    return !wilds && !all;
}

bool
filter_t::wildmatch(
    const char* str
    )
{
    DFN(filter_t::wildmatch);
    const bool found = true;
    DEBUG_PRINT(("wildmatch(%s) on filter (%s,%d,%d)...%s\n", 
                 str, s, wilds, all, found?"FOUND":"NOT found"));
    return found;
}

bool
filter_t::match(
    const char* str
    )
{
    DFN(filter_t::match);
    const bool found = !s || all || (wilds && wildmatch(str)) || (!wilds && !lstrcmpi(s, str));
    DEBUG_PRINT(("match(%s) on filter (%s,%d,%d)...%s\n", 
                 str, s, wilds, all, found?"FOUND":"NOT found"));
    return found;
}

filter_t::~filter_t()
{
    delete [] s;
}
