/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   hash object

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

#include "hash.hpp"

class IntHashNode {
public:
    int m_key;
    void* m_value;
    IntHashNode* m_next;
public:
    IntHashNode(int, void*, IntHashNode*);
    ~IntHashNode();
};

class IntHashBucket {
public:
    IntHashNode* m_head;
public:
    IntHashBucket();
    ~IntHashBucket();
    bool Get(int, void**);
    bool Del(int, void**);
    void Add(int, void*);
};

IntHashNode::IntHashNode(int key, void* value, IntHashNode* next)
{
    m_key = key;
    m_value = value;
    m_next = next;
}

IntHashNode::~IntHashNode()
{
    delete m_next;
}

IntHashBucket::IntHashBucket()
{
    m_head = 0;
}

IntHashBucket::~IntHashBucket()
{
    delete m_head;
}

bool
IntHashBucket::Get(int key, void** ptr_value)
{
    IntHashNode* p = m_head;
    while (p) {
        if (p->m_key == key)
            break;
        p = p->m_next;
    }
    if (!p)
        return false;
    if (ptr_value)
        *ptr_value = p->m_value;
    return true;
}

void
IntHashBucket::Add(int key, void* value)
{
    m_head = new IntHashNode(key, value, m_head);
}

bool
IntHashBucket::Del(int key, void** ptr_value)
{
    IntHashNode* b = m_head;
    IntHashNode* p = m_head;

    while (p) {
        if (p->m_key == key)
            break;
        b = p;
        p = p->m_next;
    }
    if (!p)
        return false;
    if (ptr_value)
        *ptr_value = p->m_value;
    if (b == p) // at head
        m_head = p->m_next;
    else
        b->m_next = p->m_next;
    delete p;
    return true;
}

IntHash::IntHash(int size)
{
    m_table = new IntHashBucket[size];
    m_size = size;
}

IntHash::~IntHash()
{
    delete[] m_table;
}

IntHashBucket&
IntHash::_Get(int key)
{
    return m_table[key % m_size];
}

bool
IntHash::Get(int key, void **ptr_value)
{
    return _Get(key).Get(key, ptr_value);
}

bool
IntHash::Add(int key, void *value)
{
    IntHashBucket& bucket = _Get(key);
    if (bucket.Get(key, 0))
        return false;
    bucket.Add(key, value);
    return true;
}

bool
IntHash::Del(int key, void **ptr_value)
{
    return _Get(key).Del(key, ptr_value);
}

extern "C" {

void *IntHash_New(int size)
{
    return (void*) new IntHash(size);
}

void IntHash_Free(void* ht)
{
    delete ((IntHash*) ht);
}

int IntHash_Get(void* ht, int key, void** ptr_value)
{
    return ((IntHash*)ht)->Get(key, ptr_value);
}

int IntHash_Del(void* ht, int key, void** ptr_value)
{
    return ((IntHash*)ht)->Del(key, ptr_value);
}

int IntHash_Add(void* ht, int key, void* value)
{
    return ((IntHash*)ht)->Add(key, value);
}

}
