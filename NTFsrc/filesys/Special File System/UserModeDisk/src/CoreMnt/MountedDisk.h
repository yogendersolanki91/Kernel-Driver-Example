#ifndef MOUNTED_DISK_H_INCLUDED
#define MOUNTED_DISK_H_INCLUDED

extern "C"
{
#include "ntifs.h"
}
#include "cmnProtectedList.h"
#include "sync.h"
#include "KernelResourceWrapper.h"
#include "IrpHandler.h"

class MountedDisk
{
    IrpHandler irpDispatcher_;

    //irp queue
    utils::KernelNotificationEvent irpQueueNotEmpty_;
    utils::KernelNotificationEvent stopEvent_;
    typedef utils::ProtectedList<PIRP, SpinLock, utils::KernelNotificationEvent> IrpQueue;
    IrpQueue irpQueue_;
    PIRP lastIrp_;

    int getDiskId();
public:
    MountedDisk(PDRIVER_OBJECT DriverObject,
                MountManager* mountManager,
                int devId,
                UINT64 totalLength);
    MountedDisk::~MountedDisk();

    NTSTATUS DispatchIrp(PIRP irp);
    void CompleteLastIrp(NTSTATUS status, ULONG information);
    void RequestExchange(UINT32 lastType, UINT32 lastStatus, UINT32 lastSize, char* buf, UINT32 bufSize,
        UINT32 * type, UINT32 * length, UINT64 * offset);
};

#endif
