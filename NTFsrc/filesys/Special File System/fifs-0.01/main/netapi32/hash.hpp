/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   hash object header

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

#ifndef __INT_HASH_H__
#define __INT_HASH_H__

#ifdef __cplusplus

class IntHashBucket;

class IntHash {
private:
    IntHashBucket* m_table;
    int m_size;
    IntHashBucket& _Get(int);
public:
    IntHash(int size);
    ~IntHash();
    bool Get(int, void**);
    bool Del(int, void**);
    bool Add(int, void*);
};

#else

typedef void* IntHash;

void *IntHash_New(int size);
int IntHash_Get(void*, int, void**);
int IntHash_Del(void*, int, void**);
int IntHash_Add(void*, int, void*);
void IntHash_Free(void*);

#endif

#endif
