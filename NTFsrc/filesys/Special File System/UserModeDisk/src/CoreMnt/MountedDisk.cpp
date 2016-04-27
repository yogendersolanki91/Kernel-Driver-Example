#include "MountedDisk.h"

MountedDisk::MountedDisk(PDRIVER_OBJECT	DriverObject, 
                         MountManager* mountManager, 
                         int devId, 
                         UINT64 totalLength)
              : irpQueue_(irpQueueNotEmpty_), 
                lastIrp_(0), 
                irpDispatcher_(devId, totalLength, DriverObject, mountManager)
{
}
void MountedDisk::CompleteLastIrp(NTSTATUS status, ULONG information)
{
    ASSERT(lastIrp_);
    irpDispatcher_.onCompleteIrp(lastIrp_, status, information);
    lastIrp_->IoStatus.Status = status;
    lastIrp_->IoStatus.Information = information;
    IoCompleteRequest( lastIrp_, IO_NO_INCREMENT);
    lastIrp_ = 0;
}
MountedDisk::~MountedDisk()
{
    stopEvent_.set();
    //complete all IRP
    if(lastIrp_)
        CompleteLastIrp(STATUS_DEVICE_NOT_READY, 0);

    while(irpQueue_.pop(lastIrp_))
        CompleteLastIrp(STATUS_DEVICE_NOT_READY, 0);
}
NTSTATUS MountedDisk::DispatchIrp(PIRP irp)
{
    IrpParam irpParam(0,0,0,0);
    irpDispatcher_.getIrpParam(irp, &irpParam);
    if(irpParam.type == directOperationEmpty)
    {
        try
        {
            irpDispatcher_.dispatch(irp);
        }
        catch(...)
        {
            ASSERT(FALSE);
        }

        NTSTATUS status = irp->IoStatus.Status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return status;
    }
    IoMarkIrpPending( irp );
    irpQueue_.push(irp);
    return STATUS_PENDING;
}

void MountedDisk::RequestExchange(UINT32 lastType, UINT32 lastStatus, UINT32 lastSize, char* buf, UINT32 bufSize,
                     UINT32 * type, UINT32 * length, UINT64 * offset)
{

    if(lastType != directOperationEmpty)
    {
        ASSERT(lastIrp_);
        lastIrp_->IoStatus.Status = lastStatus;
        if(lastStatus == STATUS_SUCCESS 
            && DiskOperationType(lastType) == directOperationRead)
        {
            IrpParam irpParam(0,0,0,0);
            irpDispatcher_.getIrpParam(lastIrp_, &irpParam);
            if(irpParam.buffer)
                memcpy(irpParam.buffer, buf, lastSize);
            else 
                lastIrp_->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        CompleteLastIrp(lastIrp_->IoStatus.Status, lastSize);
    }
    ASSERT(!lastIrp_);
    *type = directOperationEmpty;
    PVOID eventsArray[] = {irpQueueNotEmpty_.getPointer(), stopEvent_.getPointer()};
    NTSTATUS status = KeWaitForMultipleObjects(sizeof(eventsArray)/sizeof(PVOID), eventsArray, WaitAny, 
        Executive, KernelMode, FALSE, NULL, 0);
    if(status != STATUS_SUCCESS)
    {
        throw std::exception("MountedDisk::RequestExchange - mount stop.");
    }
    while(irpQueue_.pop(lastIrp_))
    {
        IrpParam irpParam(0,0,0,0);
        irpDispatcher_.getIrpParam(lastIrp_, &irpParam);
        *type = irpParam.type;
        *length = irpParam.size;
        *offset = irpParam.offset;
        if(*type != directOperationEmpty)
            break;
        ASSERT(FALSE);
    }
    if(*type != directOperationEmpty 
        && DiskOperationType(*type) == directOperationWrite)
    {
        IrpParam irpParam(0,0,0,0);
        irpDispatcher_.getIrpParam(lastIrp_, &irpParam);

        if(irpParam.buffer)
            memcpy(buf, irpParam.buffer, *length);
        else 
        {
            CompleteLastIrp(STATUS_INSUFFICIENT_RESOURCES, 0);
            *type = directOperationEmpty;
        }
    }
}