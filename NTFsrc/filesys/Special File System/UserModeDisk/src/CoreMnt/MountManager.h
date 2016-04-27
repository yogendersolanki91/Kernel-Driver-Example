#ifndef MOUNT_MANAGER_H_INCLUDED
#define MOUNT_MANAGER_H_INCLUDED
#include "MountedDisk.h"
#include <map>
#include "threadWrapper.h"
#include "boost/shared_ptr.hpp"

class MountManager
{
public:
    MountManager(PDRIVER_OBJECT driverObject);
    int Mount(UINT64 totalLength);
    void Unmount(int devId);

    NTSTATUS DispatchIrp(int devId, PIRP irp);

    void RequestExchange(UINT32 devId, UINT32 lastType, UINT32 lastStatus, UINT32 lastSize, char* buf, UINT32 bufSize,
        UINT32 * type, UINT32 * length, UINT64 * offset);
private:
    PDRIVER_OBJECT driverObject_;

    typedef std::map<int, boost::shared_ptr<MountedDisk> > MountedDiskMap;
    typedef MountedDiskMap::iterator MountedDiskMapIter;
    typedef std::pair<MountedDiskMapIter, bool> MountedDiskMapPairIB;

    int diskCount_;
    MountedDiskMap diskMap_;
    Mutex diskMapLock_;
};

#endif