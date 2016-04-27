/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   Debug header

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

#ifndef __DEBUG_HXX__
#define __DEBUG_HXX__

#ifdef NDEBUG

#define DFN(x)
#define DEBUG_PRINT(x)

#else

#include <stdio.h>

void debug_acquire();
void debug_release();

#define DFN(x) const char DEBUG_FUNC_NAME[] = #x
#define DEBUG_PRINT(x) { debug_acquire(); printf("%s [%08X]: ", DEBUG_FUNC_NAME, GetCurrentThreadId()); printf x ; debug_release(); }

#endif

#endif /* __DEBUG_HXX__ */
