/*
    This is a C++ run-time library for Windows kernel-mode drivers.
    Copyright (C) 2003 Bo Brantén.
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#if !defined(NEW_H)
#define NEW_H


#include <new>
#if DBG

void* __cdecl operator new(size_t size);
void* __cdecl operator new(size_t size, POOL_TYPE type);
void __cdecl operator delete(void* p);

#else


// Inline versions for free build.
//
inline void* __cdecl operator new(size_t size, POOL_TYPE type)
{
    if (size == 0)//When the value of the expression in a direct-new-declarator is zero, 
        size = 4;//the allocation function is called to allocatean array with no elements.(ISO)

    if (void * pData = ExAllocatePool(type, size))
        return pData;
    throw std::bad_alloc();
}

inline void* __cdecl operator new(size_t size)
{
    return operator new(size, NonPagedPool);
}


inline void __cdecl operator delete(void* p)
{
    //In either alternative, if the value of the operand 
    //of delete is the null pointer the operation has no effect.
    if (p) { ExFreePool(p); }
}

#endif

#endif // !defined(NEW_H)
