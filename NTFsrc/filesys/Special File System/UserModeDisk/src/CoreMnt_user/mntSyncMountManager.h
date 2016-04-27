#ifndef MNT_SYNC_MOUNT_MANAGER_H_INCLUDED
#define MNT_SYNC_MOUNT_MANAGER_H_INCLUDED

#include <string>
#include <map>
#include <algorithm>
#include "windows.h"
#include "SyncMounter.h"
#include "mntImage.h"
#include "windows.h"
#include <atlsync.h>
#include "mntDriverControl.h"

class SyncMountManager
{
public:
    SyncMountManager(){}
    DriverControl * GetDriverControl()
    {
        return &m_driverControl;
    }
    int AsyncMountImage(std::auto_ptr<IImage> image, wchar_t mountPoint);
    void UnmountImage(int deviceId);
private:
    struct DispatchMountContext
    {
        DispatchMountContext(SyncMountManager* mntMan, int dev, IImage * img):
            mountManager(mntMan), devId(dev), image(img){}
        SyncMountManager* mountManager;
        int devId;
        IImage * image;
    };
    typedef std::map<int, IImage*> MountedImageMap;
    typedef MountedImageMap::iterator MountedImageMapIter;
    typedef std::pair<MountedImageMap::iterator, bool> MountedImageMapPairIB;

    ATL::CCriticalSection mountersLock_;
    MountedImageMap mounters_;

    DriverControl m_driverControl;

    SyncMountManager(SyncMountManager&);
    SyncMountManager& operator=(SyncMountManager&);
    static void mountDispatchThread(void* pContext);

    void OnUnmount(IImage * image, const std::string& reason);
    void Erase(int devId)
    {
        ATL::CCritSecLock guard(mountersLock_);
        mounters_.erase(devId);
    }
};
#endif