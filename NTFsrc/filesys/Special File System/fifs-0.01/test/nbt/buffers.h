/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   multiple-part buffers header

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

#ifndef __BUFFERS_H__
#define __BUFFERS_H__

typedef struct _BUFFER_NODE {
    PVOID buffer;
    int size;
    struct _BUFFER_NODE *next;
} BUFFER_NODE, *PBUFFER_NODE;

typedef struct _BUFFER_LIST {
    BUFFER_NODE Head;
    PBUFFER_NODE pTail;
} BUFFER_LIST, *PBUFFER_LIST;

VOID
Buffers_Initialize(
    PBUFFER_LIST pBuffers,
    PVOID buffer,
    int size
    );

VOID
Buffers_Cleanup(
    PBUFFER_LIST pBuffers
    );

BOOL
Buffers_AddNode(
    PBUFFER_LIST pBuffers,
    int size
    );

BOOL
Buffers_Merge(
    PBUFFER_LIST pBuffers,
    int length,
    PVOID *ppBuffer
    );

PVOID
Buffers_GetNodeBuffer(
    PBUFFER_LIST pBuffers
    );

int
Buffers_GetNodeSize(
    PBUFFER_LIST pBuffers
    );

BOOL
Buffers_IsMultiNode(
    PBUFFER_LIST pBuffers
    );

#endif /* __BUFFLERS_H__ */
