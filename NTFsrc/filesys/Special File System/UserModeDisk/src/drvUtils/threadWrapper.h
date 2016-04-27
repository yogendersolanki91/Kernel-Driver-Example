#ifndef SYSTEM_THREAD_WRAPPER_H_UINCLUDED
#define SYSTEM_THREAD_WRAPPER_H_UINCLUDED

#include "sync.h"
#include "memory"
#include "KernelResourceWrapper.h"
namespace utils
{
typedef void (*StartRoutine_Type)(void *startContext);

struct CannotStart: public std::exception
{
    CannotStart(const char* message)
        :std::exception(message)
    {}
};

inline void
CreateThreadWithConext( HandleGuard * pHandleGuard
                      , StartRoutine_Type startRoutine
                      , void *startContext)
{
    HANDLE handle = 0;
    NTSTATUS status = PsCreateSystemThread(&handle, 
        THREAD_ALL_ACCESS, 
        NULL, 
        NULL, 
        NULL, 
        (PKSTART_ROUTINE)startRoutine, 
        (PVOID)startContext);
    
    if (!NT_SUCCESS(status))
        throw CannotStart(__FUNCTION__" PsCreateSystemThread fail");

    pHandleGuard->reset(handle);
}


class SystemThreadWrapper
{
    ObjectGuard threadGuard_;
public:
    SystemThreadWrapper(PKSTART_ROUTINE startRoutine, PVOID context)
        : threadGuard_(0)
    {
        HandleGuard handleGuard_(0);
        CreateThreadWithConext(&handleGuard_,startRoutine,context);
        
        PVOID pThread;
        // getting thread pointer (for WaitForXXX)
        NTSTATUS status = 
            ObReferenceObjectByHandle(handleGuard_.get(),
                                      THREAD_ALL_ACCESS,
                                      NULL,
                                      KernelMode,
                                      &pThread,
                                      NULL);
        if (status != STATUS_SUCCESS)
            throw std::exception(__FUNCTION__" ObReferenceObjectByHandle fail.");

        threadGuard_.reset(pThread);
    }
    ~SystemThreadWrapper()
    {
        if(KeGetCurrentThread() != threadGuard_.get())
            KeWaitForSingleObject(threadGuard_.get(), Executive, KernelMode, FALSE, NULL);
    }
};
//-----------------------------------------------------------------------------    
template<class ObjType>
class SystemThreadWrapperEx
{
    typedef void (ObjType::*PFuncType)();
public:
    SystemThreadWrapperEx(ObjType * pObj, PFuncType pFunc)
        : m_pObj(pObj)
        , m_pFunc(pFunc)
    {
        ASSERT(m_pObj);
        ASSERT(m_pFunc);
    }

    ~SystemThreadWrapperEx ()
    {
    }

    void Start(bool wait_for_complete_start = true)
    {
        AutoFastMutex doSync(m_lock);

        if (m_pThreadWrapper.get() != NULL)
            throw std::runtime_error(__FUNCTION__" - Thread already started");

        m_pThreadWrapper.reset(new SystemThreadWrapper(ThreadRoutine, this));
        
        if (wait_for_complete_start)
            m_ThreadStarted.wait();
    }

    void WaitForComplete()
    {
        AutoFastMutex doSync(m_lock);                                                
        
        m_pThreadWrapper.reset(NULL); // destructor of SystemThreadWrapper 
                                      // will do wait for completing of thread
    }
    
private:
    static void _stdcall ThreadRoutine(void * pParam)
    {
        SystemThreadWrapperEx * pThis 
            = reinterpret_cast<SystemThreadWrapperEx *>(pParam);
        ObjType * pObj  = pThis->m_pObj;
        PFuncType pFunc = pThis->m_pFunc;

        pThis->m_ThreadStarted.set();
        (pObj->*pFunc)();
    }
private:
    std::auto_ptr<SystemThreadWrapper> m_pThreadWrapper;
    utils::KernelSynchronizationEvent m_ThreadStarted;
    FastMutex m_lock;
    ObjType * m_pObj;
    PFuncType m_pFunc;
};
}//namespace utils
#endif