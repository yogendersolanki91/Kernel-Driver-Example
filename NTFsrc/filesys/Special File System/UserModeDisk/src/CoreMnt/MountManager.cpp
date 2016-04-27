#include "Mountmanager.h"

#include "new"
#include "exception"
#include "memory"
#include "IrpHandler.h"

MountManager::MountManager(PDRIVER_OBJECT driverObject):
        driverObject_(driverObject), 
        diskCount_(0)
{
    NTSTATUS          status;
    UNICODE_STRING    device_dir_name;
    OBJECT_ATTRIBUTES object_attributes;

    RtlInitUnicodeString(&device_dir_name, ROOT_DIR_NAME);

    InitializeObjectAttributes(
        &object_attributes,
        &device_dir_name,
        OBJ_OPENIF,
        NULL,
        NULL
        );

    HANDLE dir_handle;
    status = ZwCreateDirectoryObject(
        &dir_handle,
        DIRECTORY_ALL_ACCESS,
        &object_attributes
        );

    if (!NT_SUCCESS(status)) 
        ASSERT(false);
}
void MountManager::Unmount(int devId)
{
    AutoMutex guard(diskMapLock_);
    diskMap_.erase(devId);
}
NTSTATUS MountManager::DispatchIrp(int devId, PIRP irp)
{
    boost::shared_ptr<MountedDisk> disk;
    {
        AutoMutex guard(diskMapLock_);
        MountedDiskMapIter i = diskMap_.find(devId);
        if(i == diskMap_.end())
        {
            irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
            IoCompleteRequest( irp, IO_NO_INCREMENT);
            return STATUS_DEVICE_NOT_READY;
        }
        disk = i->second;
    }
    return disk->DispatchIrp(irp);
}
int MountManager::Mount(UINT64 totalLength)
{
    // generate id
    int devId = 0;
    { 
        AutoMutex guard(diskMapLock_);
        do
        {
            devId = diskCount_++;
        }
        while(diskMap_.find(devId) != diskMap_.end());
    }
    boost::shared_ptr<MountedDisk> disk(
        new MountedDisk(driverObject_, this, devId, totalLength));
    AutoMutex guard(diskMapLock_);
    MountedDiskMapPairIB pairIb = 
        diskMap_.insert(std::make_pair(devId, disk));
    if(!pairIb.second)
        throw std::exception(__FUNCTION__" - device ID already exist.");
    return devId;
}
void MountManager::RequestExchange(UINT32 devId, 
                                   UINT32 lastType, 
                                   UINT32 lastStatus, 
                                   UINT32 lastSize, 
                                   char* buf, 
                                   UINT32 bufSize,
                                   UINT32 * type, 
                                   UINT32 * length, 
                                   UINT64 * offset)
{
    boost::shared_ptr<MountedDisk> disk;
    {
        AutoMutex guard(diskMapLock_);
        MountedDiskMapIter i = diskMap_.find(devId);
        if(i == diskMap_.end())
            throw std::exception("MountManager::RequestExchange - device NOT found.");
        disk = i->second;
    }
    disk->RequestExchange(lastType, lastStatus, lastSize, buf, bufSize,
                          type, length, offset);
}