#include "mntSyncMountmanager.h"
#include "SyncMounter.h"
#include "process.h"
#include <sstream>
#include "boost\scope_exit.hpp"
#include <iostream>

void SyncMountManager::mountDispatchThread(void* pContext)
{
    std::auto_ptr<DispatchMountContext> dispContext((DispatchMountContext*)pContext);
    std::auto_ptr<IImage> image(dispContext->image);
    BOOST_SCOPE_EXIT( (&dispContext))
    {
        dispContext->mountManager->Erase(dispContext->devId);
    } BOOST_SCOPE_EXIT_END

    try
    {
        DispatchImage(dispContext->devId, 
                      image->GetMountPoint(),
                      dispContext->image, 
                      dispContext->mountManager->GetDriverControl());
    }
    catch(const std::exception& ex)
    {
        dispContext->mountManager->OnUnmount(dispContext->image, ex.what());
    }
    catch(...)
    {
        dispContext->mountManager->OnUnmount(dispContext->image, "unknown reason");
    }
}
void SyncMountManager::UnmountImage(int deviceId)
{
    wchar_t mountPoint = 0;
    {
        ATL::CCritSecLock guard(mountersLock_);
        MountedImageMapIter i = mounters_.find(deviceId);
        if(i == mounters_.end())
        {
            throw std::exception(__FUNCTION__" Can't find disk.");
        }
        mountPoint = i->second->GetMountPoint();
    }
    ::UnmountImage(deviceId, mountPoint, GetDriverControl());

    ATL::CCritSecLock guard(mountersLock_);
    mounters_.erase(deviceId);
}

void SyncMountManager::OnUnmount(IImage * image, const std::string& reason)
{
    std::cout << reason;
    ::UnmountImage(image->GetId(), image->GetMountPoint(), GetDriverControl());
}
int SyncMountManager::AsyncMountImage(std::auto_ptr<IImage> image, wchar_t mountPoint)
{
    int deviceId = MountImage(image.get(), mountPoint, GetDriverControl());

    image->SetId(deviceId);
    image->SetMountPoint(mountPoint);

    std::auto_ptr<DispatchMountContext> 
        dispContext(new DispatchMountContext(this, deviceId, image.get()));

    ATL::CCritSecLock guard(mountersLock_);
    HANDLE hThread = (HANDLE)_beginthread(mountDispatchThread, 0, dispContext.get());
    if((int)hThread == -1)
        throw std::exception("Can't create thread.");
    
    dispContext.release();
    IImage* pImage = image.release();

    MountedImageMapPairIB pairIb = mounters_.insert(std::make_pair(deviceId, pImage));
    if(!pairIb.second)
        throw std::exception("Device already exist.");

    return deviceId;
}