#ifndef LOGIC_IRP_DISPATCHER_H_INCLUDED
#define LOGIC_IRP_DISPATCHER_H_INCLUDED
extern "C"
{
#include "ntifs.h"
}
#include "CoreMntInterface.h"

class MountManager;
struct DiskDevExt
{
    int deviceId;
};
struct IrpParam
{
    IrpParam(int in_type,
            UINT32 in_size,
            UINT64 in_offset,
            char* in_buffer):
                type(in_type),
                size(in_size),
                offset(in_offset),
                buffer(in_buffer){}
    int type;
    UINT32 size;
    UINT64 offset;
    char* buffer;
};
class IrpHandler
{
    int devId_; 
    UINT64 totalLength_;
    PDEVICE_OBJECT		deviceObject_;

    void dispatchIoctl(PIRP irp);
    void dispatchPnp(PIRP irp);
public:
    IrpHandler(int devId, 
               UINT64 totalLength, 
               PDRIVER_OBJECT DriverObject,
               MountManager* mountManager);
    bool isLogic(){return true;}
    IrpHandler::~IrpHandler();
    void getIrpParam(PIRP irp, IrpParam* params);
    void dispatch(PIRP irp);
    void onCompleteIrp(PIRP irp, NTSTATUS status, ULONG information){}
};

#endif