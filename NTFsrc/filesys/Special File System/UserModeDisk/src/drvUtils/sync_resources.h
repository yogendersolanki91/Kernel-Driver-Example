#ifndef SYNC_RESOURCES_H
#define SYNC_RESOURCES_H

// to include: #include "sync_resources.h"
// native read/write section

namespace sync
{

class Resource
{
public:
    Resource();
    ~Resource();

    operator PERESOURCE();

    BOOLEAN AcquireExclusive(IN BOOLEAN Wait = TRUE);
    BOOLEAN AcquireShared(IN BOOLEAN Wait = TRUE);
    BOOLEAN AcquireSharedStarveExclusive(IN BOOLEAN Wait = TRUE);
    BOOLEAN AcquireSharedWaitForExclusive(IN BOOLEAN Wait = TRUE);

    void ConvertExclusiveToShared();

    void Release();
    void ReleaseForThread(ERESOURCE_THREAD ResourceThreadId);

    BOOLEAN IsAcquiredExclusive();
    ULONG  IsAcquiredShared();

    ULONG GetExclusiveWaiterCount();
    ULONG GetSharedWaiterCount();

protected:
    ERESOURCE m_Resource;

private:
    Resource(const Resource&);
    Resource& operator = (const Resource&);
};

class AutoResource
{
public:
    AutoResource(PERESOURCE pResource, BOOLEAN exclusive);
    ~AutoResource();

private:
    AutoResource(const AutoResource&);
    AutoResource& operator = (const AutoResource&);

private:
    PERESOURCE m_pResource;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

// Resource

inline Resource::Resource()
{
    ExInitializeResourceLite(&m_Resource);
}

inline Resource::~Resource()
{
    ExDeleteResourceLite(&m_Resource);
}

inline Resource::operator PERESOURCE()
{
    return &m_Resource;
}

inline BOOLEAN Resource::AcquireExclusive(IN BOOLEAN Wait)
{
    return ExAcquireResourceExclusiveLite(&m_Resource, Wait);
}

inline BOOLEAN Resource::AcquireShared(IN BOOLEAN Wait)
{
    return ExAcquireResourceSharedLite(&m_Resource, Wait);
}

inline BOOLEAN Resource::AcquireSharedStarveExclusive(IN BOOLEAN Wait)
{
    return ExAcquireSharedStarveExclusive(&m_Resource, Wait);
}

inline BOOLEAN Resource::AcquireSharedWaitForExclusive(IN BOOLEAN Wait)
{
    return ExAcquireSharedWaitForExclusive(&m_Resource, Wait);
}

inline void Resource::ConvertExclusiveToShared()
{
    ExConvertExclusiveToSharedLite(&m_Resource);
}

inline void Resource::Release()
{
    ExReleaseResourceLite(&m_Resource);
}

inline void Resource::ReleaseForThread(ERESOURCE_THREAD ResourceThreadId)
{
    ExReleaseResourceForThreadLite(&m_Resource, ResourceThreadId);
}

inline BOOLEAN Resource::IsAcquiredExclusive()
{
    return ExIsResourceAcquiredExclusiveLite(&m_Resource);
}

inline ULONG Resource::IsAcquiredShared()
{
    return ExIsResourceAcquiredSharedLite(&m_Resource);
}

inline ULONG Resource::GetExclusiveWaiterCount()
{
    return ExGetExclusiveWaiterCount(&m_Resource);
}

inline ULONG Resource::GetSharedWaiterCount()
{
    return ExGetSharedWaiterCount(&m_Resource);
}

// AutoResource

inline AutoResource::AutoResource(PERESOURCE pResource, BOOLEAN exclusive):
    m_pResource(pResource)
{
    ASSERT(m_pResource != NULL);

    KeEnterCriticalRegion();

    if (exclusive)
    {
        ExAcquireResourceExclusiveLite(m_pResource, TRUE);
    }
    else
    {
        ExAcquireResourceSharedLite(m_pResource, TRUE);
    }
}

inline AutoResource::~AutoResource()
{
    ExReleaseResourceLite(m_pResource);
    KeLeaveCriticalRegion();
}


} // namespace


#endif