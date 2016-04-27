#include "IrpHandler.h"

extern "C"
{
#include "ntdddisk.h"
#include "ntddcdrm.h"
#include "mountmgr.h"
#include "mountdev.h"
int swprintf(wchar_t *, const wchar_t *, ...);
}
#include <exception>

PVOID getIrpBuffer(PIRP irp)
{
    PVOID systemBuffer = 0;
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(irp);
    if(ioStack->MajorFunction == IRP_MJ_READ || ioStack->MajorFunction == IRP_MJ_WRITE)
        systemBuffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
    else
        systemBuffer = irp->AssociatedIrp.SystemBuffer;
    return systemBuffer;
}
class MountManager;
IrpHandler::IrpHandler(int devId, 
                       UINT64 totalLength, 
                       PDRIVER_OBJECT DriverObject, 
                       MountManager* mountManager)
             : devId_(devId), totalLength_(totalLength)
{
    UNICODE_STRING deviceName;
    NTSTATUS status;
    WCHAR device_name_buffer[MAXIMUM_FILENAME_LENGTH];

    //form device name
    swprintf(device_name_buffer, 
             DIRECT_DISK_PREFIX L"%u", 
             devId_);
    RtlInitUnicodeString(&deviceName, device_name_buffer);
    //create device
    status = IoCreateDevice(DriverObject,sizeof(DiskDevExt),
        &deviceName,FILE_DEVICE_DISK,
        0,
        FALSE,&deviceObject_);
    if (!NT_SUCCESS(status))
        throw std::exception(__FUNCTION__" can't create device.");

    DiskDevExt* devExt = 
        (DiskDevExt*)deviceObject_->DeviceExtension;
    memset(devExt, 0, sizeof(DiskDevExt));

    devExt->deviceId = devId_;

    deviceObject_->Flags |= DO_DIRECT_IO;
    deviceObject_->Flags &= ~DO_DEVICE_INITIALIZING;
}
IrpHandler::~IrpHandler()
{
    IoDeleteDevice(deviceObject_);
}
void IrpHandler::dispatchIoctl(PIRP irp)
{
    PIO_STACK_LOCATION io_stack = IoGetCurrentIrpStackLocation(irp);
    ULONG code = io_stack->Parameters.DeviceIoControl.IoControlCode;
    switch (code)
    {
    case IOCTL_DISK_GET_DRIVE_LAYOUT:
        if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof (DRIVE_LAYOUT_INFORMATION))
        {
            irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            irp->IoStatus.Information = 0;
        }
        else
        {
            PDRIVE_LAYOUT_INFORMATION outputBuffer = (PDRIVE_LAYOUT_INFORMATION)
                irp->AssociatedIrp.SystemBuffer;

            outputBuffer->PartitionCount = 1;
            outputBuffer->Signature = 0;

            outputBuffer->PartitionEntry->PartitionType = PARTITION_ENTRY_UNUSED;
            outputBuffer->PartitionEntry->BootIndicator = FALSE;
            outputBuffer->PartitionEntry->RecognizedPartition = TRUE;
            outputBuffer->PartitionEntry->RewritePartition = FALSE;
            outputBuffer->PartitionEntry->StartingOffset = RtlConvertUlongToLargeInteger (0);
            outputBuffer->PartitionEntry->PartitionLength.QuadPart= totalLength_;
            outputBuffer->PartitionEntry->HiddenSectors = 1L;

            irp->IoStatus.Status = STATUS_SUCCESS;
            irp->IoStatus.Information = sizeof (PARTITION_INFORMATION);
        }
        break;
    case IOCTL_DISK_CHECK_VERIFY:
    case IOCTL_CDROM_CHECK_VERIFY:
    case IOCTL_STORAGE_CHECK_VERIFY:
    case IOCTL_STORAGE_CHECK_VERIFY2:
        {
            irp->IoStatus.Status = STATUS_SUCCESS;
            irp->IoStatus.Information = 0;
            break;
        }
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
    case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
        {
            PDISK_GEOMETRY  disk_geometry;
            ULONGLONG       length;

            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(DISK_GEOMETRY))
            {
                irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                irp->IoStatus.Information = 0;
                break;
            }
            disk_geometry = (PDISK_GEOMETRY) irp->AssociatedIrp.SystemBuffer;
            length = totalLength_;
            disk_geometry->Cylinders.QuadPart = length / SECTOR_SIZE / 0x20 / 0x80;
            disk_geometry->MediaType = FixedMedia;
            disk_geometry->TracksPerCylinder = 0x80;
            disk_geometry->SectorsPerTrack = 0x20;
            disk_geometry->BytesPerSector = SECTOR_SIZE;
            irp->IoStatus.Status = STATUS_SUCCESS;
            irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
            break;
        }
    case IOCTL_DISK_GET_LENGTH_INFO:
        {
            PGET_LENGTH_INFORMATION get_length_information;
            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(GET_LENGTH_INFORMATION))
            {
                irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                irp->IoStatus.Information = 0;
                break;
            }
            get_length_information = (PGET_LENGTH_INFORMATION) irp->AssociatedIrp.SystemBuffer;
            get_length_information->Length.QuadPart = totalLength_;
            irp->IoStatus.Status = STATUS_SUCCESS;
            irp->IoStatus.Information = sizeof(GET_LENGTH_INFORMATION);
            break;
        }

    case IOCTL_DISK_GET_PARTITION_INFO:
        {
            PPARTITION_INFORMATION  partition_information;
            ULONGLONG               length;
            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(PARTITION_INFORMATION))
            {
                irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                irp->IoStatus.Information = 0;
                break;
            }
            partition_information = (PPARTITION_INFORMATION) irp->AssociatedIrp.SystemBuffer;
            length = totalLength_;
            partition_information->StartingOffset.QuadPart = 0;
            partition_information->PartitionLength.QuadPart = length;
            partition_information->HiddenSectors = 0;
            partition_information->PartitionNumber = 0;
            partition_information->PartitionType = 0;
            partition_information->BootIndicator = FALSE;
            partition_information->RecognizedPartition = TRUE;
            partition_information->RewritePartition = FALSE;
            irp->IoStatus.Status = STATUS_SUCCESS;
            irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);
            break;
        }

    case IOCTL_DISK_GET_PARTITION_INFO_EX:
        {
            PPARTITION_INFORMATION_EX   partition_information_ex;
            ULONGLONG                   length;
            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(PARTITION_INFORMATION_EX))
            {
                irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                irp->IoStatus.Information = 0;
                break;
            }
            partition_information_ex = (PPARTITION_INFORMATION_EX) irp->AssociatedIrp.SystemBuffer;
            length = totalLength_;
            partition_information_ex->PartitionStyle = PARTITION_STYLE_MBR;
            partition_information_ex->StartingOffset.QuadPart = 0;
            partition_information_ex->PartitionLength.QuadPart = length;
            partition_information_ex->PartitionNumber = 0;
            partition_information_ex->RewritePartition = FALSE;
            partition_information_ex->Mbr.PartitionType = 0;
            partition_information_ex->Mbr.BootIndicator = FALSE;
            partition_information_ex->Mbr.RecognizedPartition = TRUE;
            partition_information_ex->Mbr.HiddenSectors = 0;
            irp->IoStatus.Status = STATUS_SUCCESS;
            irp->IoStatus.Information = sizeof(PARTITION_INFORMATION_EX);
            break;
        }

    case IOCTL_DISK_IS_WRITABLE:
        {
            irp->IoStatus.Status = STATUS_SUCCESS;
            irp->IoStatus.Information = 0;
            break;
        }
    case IOCTL_DISK_MEDIA_REMOVAL:
    case IOCTL_STORAGE_MEDIA_REMOVAL:
        {
            irp->IoStatus.Status = STATUS_SUCCESS;
            irp->IoStatus.Information = 0;
            break;
        }
    case IOCTL_CDROM_READ_TOC:
        {
            PCDROM_TOC cdrom_toc;
            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(CDROM_TOC))
            {
                irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                irp->IoStatus.Information = 0;
                break;
            }
            cdrom_toc = (PCDROM_TOC) irp->AssociatedIrp.SystemBuffer;
            RtlZeroMemory(cdrom_toc, sizeof(CDROM_TOC));
            cdrom_toc->FirstTrack = 1;
            cdrom_toc->LastTrack = 1;
            cdrom_toc->TrackData[0].Control = TOC_DATA_TRACK;
            irp->IoStatus.Status = STATUS_SUCCESS;
            irp->IoStatus.Information = sizeof(CDROM_TOC);
            break;
        }
    case IOCTL_DISK_SET_PARTITION_INFO:
        {
             if (io_stack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SET_PARTITION_INFORMATION))
            {
                irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                irp->IoStatus.Information = 0;
                break;
            }
            irp->IoStatus.Status = STATUS_SUCCESS;
            irp->IoStatus.Information = 0;
            break;
        }
    case IOCTL_DISK_VERIFY:
        {
            PVERIFY_INFORMATION verify_information;
            if (io_stack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(VERIFY_INFORMATION))
            {
                irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                irp->IoStatus.Information = 0;
                break;
            }
            verify_information = (PVERIFY_INFORMATION) irp->AssociatedIrp.SystemBuffer;
            irp->IoStatus.Status = STATUS_SUCCESS;
            irp->IoStatus.Information = verify_information->Length;
            break;
        }
    case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
    {
        if(io_stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof (MOUNTDEV_NAME))
        {
            irp->IoStatus.Information = sizeof (MOUNTDEV_NAME);
            irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
        }
        else
        {
            PMOUNTDEV_NAME devName = (PMOUNTDEV_NAME)irp->AssociatedIrp.SystemBuffer;

            WCHAR device_name_buffer[MAXIMUM_FILENAME_LENGTH];
            swprintf(device_name_buffer, DIRECT_DISK_PREFIX L"%u", devId_);

            UNICODE_STRING deviceName;
            RtlInitUnicodeString(&deviceName, device_name_buffer);

            devName->NameLength = deviceName.Length;
            int outLength = sizeof(USHORT) + deviceName.Length;
            if(io_stack->Parameters.DeviceIoControl.OutputBufferLength < outLength)
            {
                irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
                irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
                break;
            }

            RtlCopyMemory(devName->Name, deviceName.Buffer, deviceName.Length);

            irp->IoStatus.Status = STATUS_SUCCESS;
            irp->IoStatus.Information = outLength;
        }
    }
    break;
    case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
    {
        if(io_stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof (MOUNTDEV_UNIQUE_ID))
        {
            irp->IoStatus.Information = sizeof (MOUNTDEV_UNIQUE_ID);
            irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
        }
        else
        {
            #define UNIQUE_ID_PREFIX L"coreMntMountedDrive"
            PMOUNTDEV_UNIQUE_ID mountDevId = (PMOUNTDEV_UNIQUE_ID)irp->AssociatedIrp.SystemBuffer;
            
            WCHAR unique_id_buffer[MAXIMUM_FILENAME_LENGTH];
            USHORT unique_id_length;

            swprintf(unique_id_buffer, DIRECT_DISK_PREFIX L"%u", devId_);
            
            UNICODE_STRING uniqueId;
            RtlInitUnicodeString(&uniqueId, unique_id_buffer);
            unique_id_length = uniqueId.Length;
            
            mountDevId->UniqueIdLength = uniqueId.Length;
            int outLength = sizeof(USHORT) + uniqueId.Length;
            if(io_stack->Parameters.DeviceIoControl.OutputBufferLength < outLength)
            {
                irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
                irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
                break;
            }
            
            RtlCopyMemory(mountDevId->UniqueId, uniqueId.Buffer, uniqueId.Length);
            
            irp->IoStatus.Status = STATUS_SUCCESS;
            irp->IoStatus.Information = outLength;
        }
    }
    break;
    case IOCTL_MOUNTDEV_QUERY_STABLE_GUID:
        {
            irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            irp->IoStatus.Information = 0;
            break;
        }
    case IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY:
        {
            irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            irp->IoStatus.Information = 0;
            break;
        }
    case IOCTL_STORAGE_GET_HOTPLUG_INFO:
        {
            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength < 
                sizeof(STORAGE_HOTPLUG_INFO)) 
            {
                irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PSTORAGE_HOTPLUG_INFO hotplug = 
                (PSTORAGE_HOTPLUG_INFO)irp->AssociatedIrp.SystemBuffer;

            RtlZeroMemory(hotplug, sizeof(STORAGE_HOTPLUG_INFO));

            hotplug->Size = sizeof(STORAGE_HOTPLUG_INFO);
            hotplug->MediaRemovable = 1;

            irp->IoStatus.Information = sizeof(STORAGE_HOTPLUG_INFO);
            irp->IoStatus.Status = STATUS_SUCCESS;
        }
        break;
    case IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_READWRITE:
        irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        irp->IoStatus.Information = 0;
        break;
    default:
        KdPrint((__FUNCTION__"Unknown PNP minor function= 0x%x\n", io_stack->MinorFunction));
    }
}

void IrpHandler::dispatch(PIRP irp)
{
    PIO_STACK_LOCATION io_stack = IoGetCurrentIrpStackLocation(irp);
    switch(io_stack->MajorFunction)
    {
    case IRP_MJ_CREATE:
    case IRP_MJ_CLOSE:
        irp->IoStatus.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = 0;
        break;
    case IRP_MJ_QUERY_VOLUME_INFORMATION:
        KdPrint((__FUNCTION__" IRP_MJ_QUERY_VOLUME_INFORMATION\n"));
        irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        irp->IoStatus.Information = 0;
        break;
    case IRP_MJ_DEVICE_CONTROL:
        dispatchIoctl(irp);
        break;
    default:
        KdPrint((__FUNCTION__"Unknown MJ fnc = 0x%x\n", io_stack->MajorFunction));
     }
}
void IrpHandler::getIrpParam(PIRP irp, IrpParam* irpParam)
{
   PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(irp);
    irpParam->offset = 0;
    irpParam->type = directOperationEmpty;
    irpParam->buffer = (char*)getIrpBuffer(irp);
    if(ioStack->MajorFunction == IRP_MJ_READ)
    {
        irpParam->type = directOperationRead;
        irpParam->size = ioStack->Parameters.Read.Length;
        irpParam->offset = ioStack->Parameters.Read.ByteOffset.QuadPart; 
    }else
    if(ioStack->MajorFunction == IRP_MJ_WRITE)
    {
        irpParam->type = directOperationWrite;
        irpParam->size = ioStack->Parameters.Write.Length;
        irpParam->offset = ioStack->Parameters.Write.ByteOffset.QuadPart; 
    }
    return;    
}