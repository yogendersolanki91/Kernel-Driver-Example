/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   queue object

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
#include "queue.hxx"

//#include <stdio.h>

struct QueueNode {
    void* data;
    QueueNode* prev;
    QueueNode* next;
};

struct Queue_Rep {
//    int length;
    QueueNode* head;
    QueueNode* tail;
    HANDLE hNonEmpty; // this event must precede hNonBlock
    HANDLE hNonBlock; // this event must follow hNonEmpty
    HANDLE hMutex;
};

// doubly-linked, head+tail...

Queue::Queue()
{
    rep = new Queue_Rep;
//    rep->length = 0;
    rep->head = 0;
    rep->tail = 0;
    rep->hNonEmpty = CreateEvent(NULL,
                                 TRUE, // manual reset
                                 FALSE, // initial state
                                 NULL);
    rep->hNonBlock = CreateEvent(NULL,
                                 TRUE, // manual reset
                                 FALSE, // initial state
                                 NULL);
    rep->hMutex = CreateMutex(NULL, 
                              FALSE, // initial state
                              NULL);
}

Queue::~Queue()
{
    while (Remove(rep->head));
    CloseHandle(rep->hNonEmpty);
    CloseHandle(rep->hMutex);
    delete rep;
}

void
Queue::Add(void* data)
{
    if (!data) return;
    QueueNode* node = new QueueNode;
    node->data = data;
    node->next = 0;

    WaitForSingleObject(rep->hMutex, INFINITE);
    if (!rep->tail) { // empty
        rep->head = node;
        node->prev = 0;
    } else {
        rep->tail->next = node;
        node->prev = rep->tail;
    }
    rep->tail = node;
//    (rep->length)++;
//    printf("alength: %d\n", rep->length);
    SetEvent(rep->hNonEmpty);
    ReleaseMutex(rep->hMutex);
    return;
}

void*
Queue::Get()
{
    return Get(INFINITE);
}

void*
Queue::Get(DWORD milliseconds)
{
    bool acquired = false;
    DWORD code;
    while (!acquired) {
        code = WaitForMultipleObjects(2, &rep->hNonEmpty, FALSE, milliseconds);
        if (code != WAIT_OBJECT_0)
            return 0;
        if (WaitForSingleObject(rep->hMutex, milliseconds) != WAIT_OBJECT_0)
            return 0;
        if (WaitForSingleObject(rep->hNonEmpty, 0) == WAIT_OBJECT_0)
            acquired = true;
        else
            ReleaseMutex(rep->hMutex);
    }
    void* data = Remove(rep->head);
    if (!rep->head)
        ResetEvent(rep->hNonEmpty);
    ReleaseMutex(rep->hMutex);
    return data;
}

void*
Queue::Remove(void* handle)
{
    if (!rep->head) return 0; // empty

    QueueNode* node = (QueueNode*)handle;
    void* data = node->data;
    QueueNode* next = node->next;
    QueueNode* prev = node->prev;

    if (prev)
        prev->next = next;
    if (next)
        next->prev = prev;
    if (node == rep->head)
        rep->head = next;
    if (node == rep->tail)
        rep->tail = prev;
    delete node;
//    (rep->length)--;
//    printf("rlength: %d\n", rep->length);
    return data;
}

void
Queue::Unblock()
{
    SetEvent(rep->hNonBlock);
}

void
Queue::Block()
{
    ResetEvent(rep->hNonBlock);
}
