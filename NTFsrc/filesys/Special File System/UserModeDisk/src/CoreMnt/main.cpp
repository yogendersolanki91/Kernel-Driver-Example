extern "C"
{
#include <ntifs.h>
}
#include "libcpp.h"
#include "CoreMntInterface.h"
#include "mountManager.h"

//The name of current driver
UNICODE_STRING gDeviceName;
PCWSTR gDeviceNameStr       = L"\\Device\\CoreMnt";
//The symbolic link of driver location
UNICODE_STRING gSymbolicLinkName;
PCWSTR gSymbolicLinkNameStr = L"\\DosDevices\\CoreMnt";

//The object associated with the driver
PDEVICE_OBJECT gDeviceObject = NULL;

MountManager * gMountManager = 0;

extern "C" 
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,IN PUNICODE_STRING RegistryPath);

VOID     DriverUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS ControlDeviceIrpHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp );
NTSTATUS Initialize();
VOID     Uninitialize();
inline NTSTATUS CompleteIrp( PIRP Irp, NTSTATUS status, ULONG info)
{
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp,IO_NO_INCREMENT);
    return status;
}
NTSTATUS IrpHandler( IN PDEVICE_OBJECT fdo, IN PIRP pIrp )
{
    try
    {
        if(fdo == gDeviceObject)
        {
            return ControlDeviceIrpHandler(fdo, pIrp);
        }
        DiskDevExt* devExt = 
            (DiskDevExt*)fdo->DeviceExtension;
        return gMountManager->DispatchIrp(devExt->deviceId, pIrp);
    }
    catch(const std::exception & ex)
    {
        __asm int 3;
        KdPrint((__FUNCTION__" %s\n", ex.what()));
        return CompleteIrp(pIrp, STATUS_NO_SUCH_DEVICE, 0);
    }
}
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
                     IN PUNICODE_STRING RegistryPath)
{
    libcpp_init();

    /* Start Driver initialization */
    RtlInitUnicodeString(&gDeviceName,       gDeviceNameStr);
    RtlInitUnicodeString(&gSymbolicLinkName, gSymbolicLinkNameStr);

    NTSTATUS status;
    status = IoCreateDevice(DriverObject,     // pointer on DriverObject
                            0,                // additional size of memory, for device extension
                            &gDeviceName,     // pointer to UNICODE_STRING
                            FILE_DEVICE_NULL, // Device type
                            0,                // Device characteristic
                            FALSE,            // "Exclusive" device
                            &gDeviceObject);  // pointer do device object
    if (status != STATUS_SUCCESS)
        return STATUS_FAILED_DRIVER_ENTRY;

    status = IoCreateSymbolicLink(&gSymbolicLinkName,&gDeviceName);
    if (status != STATUS_SUCCESS)
        return STATUS_FAILED_DRIVER_ENTRY;

    // Register IRP handlers
    PDRIVER_DISPATCH *mj_func;
    mj_func = DriverObject->MajorFunction;
    DriverObject->DriverUnload = DriverUnload;

    for (size_t i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i) 
        DriverObject->MajorFunction[i] = IrpHandler;

    /* Driver initialization are done */
    gMountManager = new MountManager(DriverObject);
    return STATUS_SUCCESS;
}
VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
{    
    delete gMountManager;
    IoDeleteSymbolicLink(&gSymbolicLinkName);
    IoDeleteDevice(gDeviceObject);

    libcpp_exit();

    return;
}
NTSTATUS DispatchMount(PVOID buffer, 
                       ULONG inputBufferLength, 
                       ULONG outputBufferLength)
{
    try
    {
        if(inputBufferLength < sizeof(CORE_MNT_MOUNT_REQUEST) || 
            outputBufferLength < sizeof(CORE_MNT_MOUNT_RESPONSE) )
        {
            throw std::exception(__FUNCTION__" buffer size mismatch");
        }

        CORE_MNT_MOUNT_REQUEST * request = (CORE_MNT_MOUNT_REQUEST *)buffer;
        UINT64 totalLength = request->totalLength;
        CORE_MNT_MOUNT_RESPONSE * response = 
            (CORE_MNT_MOUNT_RESPONSE *)buffer;
        response->deviceId = gMountManager->Mount(totalLength);
        return STATUS_SUCCESS;
    }
    catch(const std::exception & ex)
    {
        KdPrint((__FUNCTION__" %s\n", ex.what()));
        return STATUS_UNSUCCESSFUL;
    }
}
NTSTATUS DispatchExchange(PVOID buffer, ULONG inputBufferLength, ULONG outputBufferLength)
{
    try
    {
        if(inputBufferLength < sizeof(CORE_MNT_EXCHANGE_REQUEST) || 
            outputBufferLength < sizeof(CORE_MNT_EXCHANGE_RESPONSE))
        {
            throw std::exception(__FUNCTION__" buffer size mismatch");
        }
        CORE_MNT_EXCHANGE_REQUEST * request = (CORE_MNT_EXCHANGE_REQUEST *)buffer;

        CORE_MNT_EXCHANGE_RESPONSE response = {0};
        gMountManager->RequestExchange(request->deviceId, 
                                       request->lastType, 
                                       request->lastStatus, 
                                       request->lastSize, 
                                       request->data, 
                                       request->dataSize, 
                                       &response.type,
                                       &response.size, 
                                       &response.offset);
        memcpy(buffer, &response, sizeof(response));
        return STATUS_SUCCESS;
    }
    catch(const std::exception & ex)
    {
        KdPrint((__FUNCTION__" %s\n", ex.what()));
        return STATUS_UNSUCCESSFUL;
    }
}
NTSTATUS DispatchUnmount(PVOID buffer, ULONG inputBufferLength, ULONG outputBufferLength)
{
    try
    {
        if(inputBufferLength < sizeof(CORE_MNT_UNMOUNT_REQUEST))
        {
            throw std::exception(__FUNCTION__" buffer size mismatch");
        }
        CORE_MNT_UNMOUNT_REQUEST * request = (CORE_MNT_UNMOUNT_REQUEST *)buffer;
        gMountManager->Unmount(request->deviceId);
        return STATUS_SUCCESS;
    }
    catch(const std::exception & ex)
    {
        KdPrint((__FUNCTION__" %s\n", ex.what()));
        return STATUS_UNSUCCESSFUL;
    }
}
NTSTATUS ControlDeviceIrpHandler( IN PDEVICE_OBJECT fdo, IN PIRP pIrp )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(pIrp);
    switch(irpStack->MajorFunction)
    {
    case IRP_MJ_CREATE:
    case IRP_MJ_CLOSE:
    case IRP_MJ_CLEANUP:
        return CompleteIrp(pIrp, STATUS_SUCCESS,0);
    case IRP_MJ_DEVICE_CONTROL:
        {
            ULONG code = irpStack->Parameters.DeviceIoControl.IoControlCode;
            PVOID buffer = pIrp->AssociatedIrp.SystemBuffer;
            ULONG outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;    
            ULONG inputBufferLength = irpStack->Parameters.DeviceIoControl.InputBufferLength; 
            NTSTATUS status = STATUS_SUCCESS;
            switch (code)
            {
            case CORE_MNT_MOUNT_IOCTL:
                status = DispatchMount(buffer, inputBufferLength, outputBufferLength);
                break;
            case CORE_MNT_EXCHANGE_IOCTL:
                status = DispatchExchange(buffer, inputBufferLength, outputBufferLength);
                break;
            case CORE_MNT_UNMOUNT_IOCTL:
                status = DispatchUnmount(buffer, inputBufferLength, outputBufferLength);
                break;
            }
            return CompleteIrp(pIrp,status,outputBufferLength);
        }
    }
    __asm int 3;
    return CompleteIrp(pIrp, STATUS_UNSUCCESSFUL, 0);
}