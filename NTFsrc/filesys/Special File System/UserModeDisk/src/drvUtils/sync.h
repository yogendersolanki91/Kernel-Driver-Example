#ifndef SYNC_H_INCLUDED
#define SYNC_H_INCLUDED
extern "C"
{
#include <ntifs.h>
//#include "core_NtDefinitions.h"
}
//#include "cmnComplexException.h"
//#include "cmnPrintfLog.h"
#include "memory"
#include "cmn_sys_defines.h"

#include "sync_resources.h"

class SpinLock
{
    KIRQL OldIrql;
    KSPIN_LOCK spinLock;

    SpinLock(const SpinLock& );
    SpinLock& operator = (const SpinLock& );
public:
    SpinLock()
    {
        KeInitializeSpinLock(&spinLock);
    }
    void enter()
    {
        KeAcquireSpinLock(&spinLock, &OldIrql);
    }
    void leave()
    {
        KeReleaseSpinLock(&spinLock, OldIrql);
    }
};
class Mutex
{
	std::auto_ptr<KMUTEX> pMutexGuard_;
	KMUTEX * pMutex_;
	//We can't store mutex in stack memory
	//DRIVER_VERIFIER_DETECTED_VIOLATION (c4-1005)

	//MSDN say - 
	//Storage for a mutex object must be resident: in the device extension of a
	//driver-created device object, in the controller extension of a driver-created 
	//controller object, or in nonpaged pool allocated by the caller.
	Mutex(const Mutex& );
	Mutex& operator = (const Mutex& );
public:
	Mutex():
	  pMutexGuard_(new KMUTEX)
	  {
		  pMutex_ = pMutexGuard_.get();
		  KeInitializeMutex(pMutex_, 0);
	  }
	  Mutex(KMUTEX* pMutex):
	  pMutex_(pMutex)
	  {
	  }
	  void enter()
	  {
		  NTSTATUS status = KeWaitForMutexObject(pMutex_, Executive, KernelMode, FALSE, NULL);
		  if(!NT_SUCCESS(status))
              throw std::exception("KeWaitForMutexObject fails");
	  }
	  void leave()
	  {
		  KeReleaseMutex(pMutex_, FALSE);
	  }
};

class FastMutex
{
	std::auto_ptr<FAST_MUTEX> pFastMutex_;
	FastMutex(const FastMutex& );
	FastMutex& operator = (const FastMutex& );
public:
	FastMutex():
	  pFastMutex_(new FAST_MUTEX)
	  {
		  ExInitializeFastMutex(pFastMutex_.get());
	  }
	  void enter()
	  {
		  ExAcquireFastMutex(pFastMutex_.get());
	  }
	  void leave()
	  {
		  ExReleaseFastMutex(pFastMutex_.get());
	  }
};

template<class Event>
class AutoEventSetter
{
    Event& event_;
    bool bSet_;
    AutoEventSetter();

    AutoEventSetter(const AutoEventSetter& );
    AutoEventSetter& operator = (const AutoEventSetter& );
public:
    explicit AutoEventSetter(Event& event, bool bSet = true ):event_(event),bSet_(bSet){}
    ~AutoEventSetter()
    {
        if ( bSet_ )
            event_.set();
        else
            event_.reset ();
            
    }
};

template<class Guard>
class AutoGuard
{
    Guard & guard_;
    bool m_bCallDestructor;
    AutoGuard();
    AutoGuard(AutoGuard&);
    AutoGuard& operator = (const AutoGuard& );
public:
    explicit AutoGuard(Guard& guard)
        :guard_(guard),m_bCallDestructor(true)
    {
        guard_.enter();
    }
    ~AutoGuard()
    {
        if (m_bCallDestructor)
            guard_.leave();
    }
    void Release()
    {
        if (m_bCallDestructor)
        {
            m_bCallDestructor = false;
            guard_.leave();
        }
    }
};
template<class Guard>
class AutoGuardPtr
{
    Guard* guard_;
    AutoGuardPtr(const AutoGuardPtr&);
    AutoGuardPtr& operator =(const AutoGuardPtr&);
public:
    explicit AutoGuardPtr(Guard* guard)
        :guard_(guard)
    {
        if(guard_)
            guard_->enter();
    }
    void reset(Guard* guard)
    {
        if(guard_)
            guard_->leave();
        guard_= guard;
        if(guard_)
            guard_->enter();            
    }
    ~AutoGuardPtr()
    {
        if(guard_)
            guard_->leave();
    }
};
inline void CloseUserOrKernelHandle(HANDLE hFile)
{                
    if (IS_KERNEL_HANDLE(hFile))
        ZwClose(hFile);
    else
        NtClose(hFile);
};
class HandleGuard
{
    HANDLE h_;
    HandleGuard(HandleGuard&);
    HandleGuard& operator=(HandleGuard&);
public:
    explicit HandleGuard(HANDLE h)
        :h_(h){}
        ~HandleGuard(void)
        {
            if(h_)
            {
                CloseUserOrKernelHandle(h_);
            }
        }
        HANDLE get(){return h_;}
        HANDLE release()
        {
            HANDLE tmp = h_;
            h_ = 0;
            return tmp;
        }
        void reset(HANDLE h)
        {
            if(h_)
            {
                CloseUserOrKernelHandle(h_);
            }
            h_ = h;
        }
};
class ObjectGuard
{
    PVOID h_;
    ObjectGuard(ObjectGuard&);
    ObjectGuard& operator=(ObjectGuard&);
public:
    explicit ObjectGuard(PVOID h)
        :h_(h){}
        ~ObjectGuard(void)
        {
            if(h_)
                ObDereferenceObject(h_);
        }
        PVOID get(){return h_;}
        PVOID release()
        {
            PVOID tmp = h_;
            h_ = 0;
            return tmp;
        }
        void reset(PVOID h)
        {
            if(h_)
                ObDereferenceObject(h_);
            h_ = h;
        }
};
template<class Type>
class ObjectGuard2:protected ObjectGuard
{
public:
    explicit ObjectGuard2(Type *  ptr=0)
        : ObjectGuard(ptr)
    {
    }

    Type * get() { return (Type * )ObjectGuard::get(); }
    Type * release() { return (Type * )ObjectGuard::release(); }
    void reset(Type * ptr) { ObjectGuard::reset(ptr); }
    Type * operator -> () { return get(); }
};
class ProcessAttacher
{
    PEPROCESS h_;
    ProcessAttacher(ProcessAttacher&);
    ProcessAttacher& operator=(ProcessAttacher&);
public:
    explicit ProcessAttacher(PEPROCESS h)
        :h_(h){}
        ~ProcessAttacher()
        {
            if(h_)
                KeAttachProcess(h_);
        }
        PEPROCESS get(){return h_;}
        PEPROCESS release()
        {
            PEPROCESS tmp = h_;
            h_ = 0;
            return tmp;
        }
        void reset(PEPROCESS h)
        {
            if(h_)
                KeDetachProcess();
            h_ = h;
        }
};
typedef AutoGuard<Mutex> AutoMutex;
typedef AutoGuard<SpinLock> AutoSpinLock;
typedef AutoGuard<FastMutex> AutoFastMutex;


class CSyncMutexStrategy_BuiltIn
{
    Mutex  m_section;
public:
    typedef AutoMutex GuardType;
    typedef Mutex  SyncType;
    typedef AutoMutex ReadGuardType;

    SyncType & GetSyncObject() { return m_section; }
};

class CSyncMutexStrategy_Outside
{
    Mutex  * m_pSection;
public:
        CSyncMutexStrategy_Outside(Mutex  * pSection)
            : m_pSection(pSection)
        { 
        }

    typedef AutoMutex GuardType;
    typedef Mutex  SyncType;
    typedef AutoMutex ReadGuardType;
    SyncType & GetSyncObject() { return *m_pSection; }
};

class CSyncSpinLockStrategy_BuiltIn

{
    SpinLock  m_section;
public:
    typedef AutoSpinLock GuardType;
    typedef SpinLock  SyncType;
    typedef AutoMutex ReadGuardType;
    SyncType & GetSyncObject() { return m_section; }
};

inline NTSTATUS WaitTwoEvents(PVOID Event0, PVOID Event1, 
                       WAIT_TYPE WaitType = WaitAny, 
                       KWAIT_REASON WaitReason = Executive, 
                       KPROCESSOR_MODE WaitMode = KernelMode,
                       BOOLEAN Alertable = false, 
                       PLARGE_INTEGER Timeout = NULL, 
                       PKWAIT_BLOCK WaitBlockArray = NULL)
{
    ASSERT(Event0);
    ASSERT(Event1);
    
    PVOID  events[] = {Event0, Event1};
    
    return KeWaitForMultipleObjects(
            sizeof(events) / sizeof(events[0]),
            events,
            WaitAny,
            Executive,
            KernelMode,
            false,
            NULL,
            NULL);    
}

#endif
