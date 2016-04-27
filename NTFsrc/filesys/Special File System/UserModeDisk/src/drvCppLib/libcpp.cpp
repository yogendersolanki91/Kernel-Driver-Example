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

extern "C" {
#include <ntddk.h>

NTSYSAPI
VOID
NTAPI
RtlRaiseException (
    IN PEXCEPTION_RECORD ExceptionRecord
    );

}

#include <typeinfo>

const char* type_info::name()const
{
    return raw_name();
}

#include "libcpp.h"
#include "new.h"
typedef void (__cdecl *func_t)();

//
// Data segment with pointers to C++ initializers.
//
#pragma data_seg(".CRT$XCA")
func_t xc_a[] = { 0 };
#pragma data_seg(".CRT$XCZ")
func_t xc_z[] = { 0 };
#pragma data_seg()
#pragma comment(linker, "/merge:.CRT=.data")

//
// Simple class to keep track of functions registred to be called when
// unloading the driver. Since we only use this internaly from the load
// and unload functions it doesn't need to be thread safe.
//
class AtExitCall {
public:
    AtExitCall(func_t f) : m_func(f), m_next(m_exit_list) { m_exit_list = this; }
    ~AtExitCall() { m_func(); m_exit_list = m_next; }
    static run() { while (m_exit_list) delete m_exit_list; }
private:
    func_t              m_func;
    AtExitCall*         m_next;
    static AtExitCall*  m_exit_list;
};

AtExitCall* AtExitCall::m_exit_list = 0;


#if DBG
void* __cdecl operator new(size_t size, POOL_TYPE type)
{
    if (size == 0)//When the value of the expression in a direct-new-declarator is zero, 
        size = 4;//the allocation function is called to allocatean array with no elements.(ISO)

    if (void * pData = ExAllocatePool(type, size))
        return pData;
    throw std::bad_alloc();
}

void* __cdecl operator new(size_t size)
{
    return operator new(size, NonPagedPool);
}

void __cdecl operator delete(void* p)
{
    //In either alternative, if the value of the operand 
    //of delete is the null pointer the operation has no effect.
    if (p)ExFreePool(p); 
}
#endif
extern "C" {

//
// Calls functions the compiler has registred to call constructors
// for global and static objects.
//
void __cdecl libcpp_init()
{
    for (func_t* f = xc_a; f < xc_z; f++) if (*f) (*f)();
}

//
// Calls functions the compiler has registred to call destructors
// for global and static objects.
//
void __cdecl libcpp_exit()
{
    AtExitCall::run();
}

//
// The run-time support for RTTI uses malloc and free so we include them here.
//

void * __cdecl malloc(size_t size)
{
    return size ? ExAllocatePool(NonPagedPool, size) : 0;
}

void __cdecl free(void *p)
{
    if (p) { ExFreePool(p); }
}

//
// Registers a function to be called when unloading the driver. If memory
// couldn't be allocated the function is called immediately since it never
// will be called otherwise.
//
int __cdecl atexit(func_t f)
{
    return (new AtExitCall(f) == 0) ? (*f)(), 1 : 0;
}

/*
 * The statement:
 *
 *   throw E();
 *
 * will be translated by the compiler to:
 *
 *   E e = E();
 *   _CxxThrowException(&e, ...);
 *
 * and _CxxThrowException is implemented as:
 *
 *   #define CXX_FRAME_MAGIC 0x19930520
 *   #define CXX_EXCEPTION   0xe06d7363
 *
 *   void _CxxThrowException(void *object, cxx_exception_type *type)
 *   {
 *       ULONG args[3];
 *
 *       args[0] = CXX_FRAME_MAGIC;
 *       args[1] = (ULONG) object;
 *       args[2] = (ULONG) type;
 *
 *       RaiseException(CXX_EXCEPTION, EXCEPTION_NONCONTINUABLE, 3, args);
 *   }
 *
 * so whats left for us to implement is RaiseException
 *
 */
VOID
NTAPI
RaiseException (
    ULONG   ExceptionCode,
    ULONG   ExceptionFlags,
    ULONG   NumberParameters,
    PULONG  ExceptionInformation
    )
{
    EXCEPTION_RECORD ExceptionRecord = {
        ExceptionCode,
        ExceptionFlags & EXCEPTION_NONCONTINUABLE,
        NULL,
        RaiseException,
        NumberParameters > EXCEPTION_MAXIMUM_PARAMETERS ? EXCEPTION_MAXIMUM_PARAMETERS : NumberParameters
    };

    RtlCopyMemory(
        ExceptionRecord.ExceptionInformation,
        ExceptionInformation,
        ExceptionRecord.NumberParameters * sizeof(ULONG)
        );

    RtlRaiseException(&ExceptionRecord);
}

//
// Internal function to probe memory.
//
static ULONG NTAPI probe(CONST PVOID Buffer, ULONG Length, LOCK_OPERATION Operation)
{
    PMDL    Mdl;
    ULONG   IsBadPtr;

    Mdl = IoAllocateMdl(Buffer, Length, FALSE, FALSE, 0);

    __try
    {
        MmProbeAndLockPages(Mdl, KernelMode, Operation);
        MmUnlockPages(Mdl);
        IsBadPtr = FALSE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        IsBadPtr = TRUE;
    }

    IoFreeMdl(Mdl);

    return IsBadPtr;
}

//
// Used by the run-time support for exception handling.
//

ULONG NTAPI IsBadReadPtr(CONST PVOID Buffer, ULONG Length)
{
    return probe(Buffer, Length, IoReadAccess);
}

ULONG NTAPI IsBadWritePtr(PVOID Buffer, ULONG Length)
{
    return probe(Buffer, Length, IoWriteAccess);
}

ULONG NTAPI IsBadCodePtr(CONST PVOID Buffer)
{
    return probe(Buffer, 1, IoReadAccess);
}

//
// When using exception handling the compiler will generate a call to
// SetUnhandledExceptionFilter to install a function that calls abort.
// In a driver there is no top level C++ exception handler to catch an
// unhandled exception so we ignore these but dummy functions must be
// included to link the driver.
//

void __cdecl abort()
{
    ASSERT(FALSE);
}

PVOID NTAPI SetUnhandledExceptionFilter(PVOID p)
{
    return 0;
}

} // extern "C"
