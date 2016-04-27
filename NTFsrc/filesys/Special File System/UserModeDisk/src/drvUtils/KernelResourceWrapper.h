#ifndef KERNEL_RESOURCE_WRAPPER_H_INCLUDED
#define KERNEL_RESOURCE_WRAPPER_H_INCLUDED
extern "C"
{
#include "ntifs.h"
}
#include "sync.h"
#include "new"
#include "exception"

namespace utils
{

template<EVENT_TYPE eventType>
class KernelCustomEvent
{
    KEVENT event_;

    KernelCustomEvent(const KernelCustomEvent&);
    KernelCustomEvent& operator =(const KernelCustomEvent&);
public:
    KernelCustomEvent(BOOLEAN initState = FALSE)
    {
        KeInitializeEvent(&event_, eventType, initState);
    }
    KEVENT* getPointer()
    {
        return &event_;
    }
    void wait()
    {
        NTSTATUS status = KeWaitForSingleObject(&event_, Executive, KernelMode, FALSE, NULL);
        if(!NT_SUCCESS(status))
        {
            KdPrint((__FUNCTION__" KeWaitForMutexObject fail %08X\n", status));
            __asm int 3;
        }
    }
    bool readState()
    {
        return KeReadStateEvent(&event_) != 0;
    }
    void set()
    {
        KeSetEvent(&event_, 0, FALSE);
    }
    void reset()
    {
        KeClearEvent(&event_);
    }
};

typedef KernelCustomEvent<NotificationEvent> KernelNotificationEvent;
typedef KernelCustomEvent<SynchronizationEvent> KernelSynchronizationEvent;

template<class LockType>
class ReverseSemaphore
{
    KernelNotificationEvent readyEvent_;
    LockType lock_;
    int count_;
public:
    ReverseSemaphore():count_(0), readyEvent_(TRUE){}
    KEVENT* getPointer()
    {
        return readyEvent_.getPointer();
    }
    void enter()
    {
        AutoGuard<LockType> guard(lock_);
        ++count_;
        readyEvent_.reset();
    }
    void leave()
    {
    bool ready = false;
    {
            AutoGuard<LockType> guard(lock_);
            --count_;
            ASSERT(count_ >= 0);
            if(count_ == 0)
            ready = true;
    }
    if(ready)
            readyEvent_.set();
    }
};
typedef ReverseSemaphore<Mutex> ReverseMutexSemaphore;
typedef AutoGuard<ReverseMutexSemaphore> AutoReverseMutexSemaphore;
typedef AutoGuardPtr<ReverseMutexSemaphore> AutoReverseMutexSemaphorePtr;


//---------------------------------------------------------------
// auto_ptr_kernel
template<class ResourceType>
class auto_ptr_kernel
{
    ResourceType * m_pResource;
public:
    auto_ptr_kernel()
        : m_pResource(0)
    {
    }
        
    explicit auto_ptr_kernel(ResourceType * pResource)
        : m_pResource( pResource )
    {
    }
    auto_ptr_kernel(auto_ptr_kernel & source_ptr)
        :
        m_pResource( source_ptr.get() )
    {
        source_ptr.m_pResource = 0;
    }

    auto_ptr_kernel & operator = (auto_ptr_kernel & source_ptr)
    {
        m_pResource = source_ptr.get();
        source_ptr.m_pResource = 0;
        return *this;
    }

    ~auto_ptr_kernel()
    {
        if (m_pResource)
            ExFreePool( m_pResource );
    }

    const ResourceType * get() const { return m_pResource; }
    ResourceType * get() { return m_pResource; }

    ResourceType * release()
    {
        ResourceType * pRes = m_pResource;
        m_pResource = 0;
        return pRes;
    }

    ResourceType * operator -> ()   {   return m_pResource;    }
    const ResourceType * operator -> () const {   return m_pResource;    }
    ResourceType & operator * ()   {   return *m_pResource;    }
    const ResourceType & operator * () const {   return *m_pResource;    }
    
    // non-standard
    ResourceType ** getPtr2() 
    { 
        if (m_pResource)
            throw std::exception("Cannot access member");
        return &m_pResource; 
    }
};
//--------------------------------------------------------------
inline void * AllocatePaged(int iAdditionalSize)
{
    void * pData = ExAllocatePool(PagedPool, iAdditionalSize);
    if (!pData)
        throw std::bad_alloc();
    return pData;
}
//--------------------------------------------------------------
template<class Type>
Type * AllocatePaged(int iAdditionalSize=0)
{
    void * pData = ExAllocatePool(PagedPool, sizeof(Type)+iAdditionalSize);
    if (!pData)
        throw std::bad_alloc();
    memset(pData, 0, sizeof(Type));
    return (Type * )pData;
}
//--------------------------------------------------------------
template<class Type>
Type * AllocateNonPaged(int iAdditionalSize=0)
{
    void * pData = ExAllocatePool(NonPagedPool, sizeof(Type)+iAdditionalSize);
    if (!pData)
        throw std::bad_alloc();
    memset(pData, 0, sizeof(Type));
    return (Type * )pData;
}
//--------------------------------------------------------------

}//namespace utils
#endif
