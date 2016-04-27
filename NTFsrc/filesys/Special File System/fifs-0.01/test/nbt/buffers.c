/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   multiple-part buffers

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
#include <assert.h>
#include "buffers.h"

VOID
Buffers_Initialize(
    PBUFFER_LIST pBuffers,
    PVOID buffer,
    int size
    )
{
    pBuffers->Head.buffer = buffer;
    pBuffers->Head.size = size;
    pBuffers->pTail = &pBuffers->Head;
}

VOID
Buffers_Cleanup(
    PBUFFER_LIST pBuffers
    )
{
    PBUFFER_NODE pCurrent, pTemp;

    assert(pBuffers);

    pCurrent = pBuffers->Head.next;
    pBuffers->Head.next = NULL;

    while (pCurrent) {
        free(pCurrent->buffer);
        pTemp = pCurrent;
        pCurrent = pCurrent->next;
        free(pTemp);
    }

    pBuffers->pTail = &pBuffers->Head;
}

BOOL
Buffers_AddNode(
    PBUFFER_LIST pBuffers,
    int size
    )
{
    PBUFFER_NODE pNew = NULL;

    assert(pBuffers);

    pNew = malloc(sizeof(BUFFER_NODE));
    if (!pNew) {
        SetLastError(errno);
        return FALSE;
    }
    pNew->next = NULL;
    pNew->buffer = malloc(size);
    if (!pNew->buffer) {
        int error_code = errno;
        free(pNew);
        SetLastError(error_code);
        return FALSE;
    }
    pNew->size = size;

    pBuffers->pTail->next = pNew;
    pBuffers->pTail = pNew;
    return TRUE;
}

BOOL
Buffers_Merge(
    PBUFFER_LIST pBuffers,
    int length,
    PVOID *ppBuffer
    )
{
    PBUFFER_NODE pCurrent;
    PUCHAR current;
    PVOID buffer = NULL;
    int size;

    assert(pBuffers);
    assert(ppBuffer);

    *ppBuffer = NULL;

    buffer = malloc(length);

    if (!buffer) {
        SetLastError(errno);
        return FALSE;
    }

    current = buffer;
    pCurrent = &pBuffers->Head;

    while (pCurrent && (length > 0)) {
        size = min(length, pCurrent->size);
        memcpy(current, pCurrent->buffer, size);
        pCurrent = pCurrent->next;
        length -= size;
    }

    if (pCurrent || length) {
        SetLastError(ERROR_INVALID_PARAMETER);
        free(buffer);
        return FALSE;
    }

    *ppBuffer = buffer;
    return TRUE;
}

PVOID
Buffers_GetNodeBuffer(
    PBUFFER_LIST pBuffers
    )
{
    return pBuffers->pTail->buffer;
}

int
Buffers_GetNodeSize(
    PBUFFER_LIST pBuffers
    )
{
    return pBuffers->pTail->size;
}

BOOL
Buffers_IsMultiNode(
    PBUFFER_LIST pBuffers
    )
{
    return (pBuffers->pTail != &pBuffers->Head);
}
