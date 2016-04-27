/*
    HTTP virtual disk driver for Windows.
    Copyright (C) 2006-2015 Bo Brantén.
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <ntifs.h>
#include <ntdddisk.h>
#include <ntddcdrm.h>
#include <ntstrsafe.h>
#include <wdmsec.h>
#include <mountmgr.h>
#include <ntddvol.h>
#include <ntddscsi.h>
#include <ntddft.h>
#include "ksocket.h"
#include "httpdisk.h"

#define HTTP_DISK_POOL_TAG      'ksiD'

#define PARAMETER_KEY           L"\\Parameters"

#define NUMBEROFDEVICES_VALUE   L"NumberOfDevices"

#define DEFAULT_NUMBEROFDEVICES 4

#define TOC_DATA_TRACK          0x04

#define BUFFER_SIZE             (4096 * 4)

HANDLE dir_handle;

typedef struct _HTTP_HEADER {
    LARGE_INTEGER ContentLength;
} HTTP_HEADER, *PHTTP_HEADER;

typedef struct _DEVICE_EXTENSION {
    BOOLEAN         media_in_device;
    UNICODE_STRING  device_name;
    ULONG           device_number;
    DEVICE_TYPE     device_type;
    ULONG           address;
    USHORT          port;
    PUCHAR          host_name;
    PUCHAR          file_name;
    LARGE_INTEGER   file_size;
    INT_PTR         socket;
    LIST_ENTRY      list_head;
    KSPIN_LOCK      list_lock;
    KEVENT          request_event;
    PVOID           thread_pointer;
    BOOLEAN         terminate_thread;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#ifdef NTDDI_VERSION
DRIVER_INITIALIZE DriverEntry;
__drv_dispatchType(IRP_MJ_CREATE) __drv_dispatchType(IRP_MJ_CLOSE) DRIVER_DISPATCH HttpDiskCreateClose;
__drv_dispatchType(IRP_MJ_READ) __drv_dispatchType(IRP_MJ_WRITE) DRIVER_DISPATCH HttpDiskReadWrite;
__drv_dispatchType(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH HttpDiskDeviceControl;
KSTART_ROUTINE HttpDiskThread;
DRIVER_UNLOAD HttpDiskUnload;
#endif // NTDDI_VERSION

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
);

NTSTATUS
HttpDiskCreateDevice (
    IN PDRIVER_OBJECT   DriverObject,
    IN ULONG            Number,
    IN DEVICE_TYPE      DeviceType
);

VOID
HttpDiskUnload (
    IN PDRIVER_OBJECT   DriverObject
);

PDEVICE_OBJECT
HttpDiskDeleteDevice (
    IN PDEVICE_OBJECT   DeviceObject
);

NTSTATUS
HttpDiskCreateClose (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
);

NTSTATUS
HttpDiskReadWrite (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
);

NTSTATUS
HttpDiskDeviceControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
);

VOID
HttpDiskThread (
    IN PVOID            Context
);

NTSTATUS
HttpDiskConnect (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
);

NTSTATUS
HttpDiskDisconnect (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
);

NTSTATUS
HttpDiskGetHeader (
    IN ULONG                Address,
    IN USHORT               Port,
    IN PUCHAR               HostName,
    IN PUCHAR               FileName,
    OUT PIO_STATUS_BLOCK    IoStatus,
    OUT PHTTP_HEADER        HttpHeader
);

NTSTATUS
HttpDiskGetBlock (
    IN INT_PTR              *Socket,
    IN ULONG                Address,
    IN USHORT               Port,
    IN PUCHAR               HostName,
    IN PUCHAR               FileName,
    IN PLARGE_INTEGER       Offset,
    IN ULONG                Length,
    OUT PIO_STATUS_BLOCK    IoStatus,
    OUT PVOID               SystemBuffer
);

__int64 __cdecl _atoi64(const char *);

#pragma code_seg("INIT")

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    )
{
    UNICODE_STRING              parameter_path;
    RTL_QUERY_REGISTRY_TABLE    query_table[2];
    ULONG                       n_devices;
    NTSTATUS                    status;
    UNICODE_STRING              device_dir_name;
    OBJECT_ATTRIBUTES           object_attributes;
    ULONG                       n;
    USHORT                      n_created_devices;

    parameter_path.Length = 0;

    parameter_path.MaximumLength = RegistryPath->Length + sizeof(PARAMETER_KEY);

    parameter_path.Buffer = (PWSTR) ExAllocatePoolWithTag(PagedPool, parameter_path.MaximumLength, HTTP_DISK_POOL_TAG);

    if (parameter_path.Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyUnicodeString(&parameter_path, RegistryPath);

    RtlAppendUnicodeToString(&parameter_path, PARAMETER_KEY);

    RtlZeroMemory(&query_table[0], sizeof(query_table));

    query_table[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    query_table[0].Name = NUMBEROFDEVICES_VALUE;
    query_table[0].EntryContext = &n_devices;

    status = RtlQueryRegistryValues(
        RTL_REGISTRY_ABSOLUTE,
        parameter_path.Buffer,
        &query_table[0],
        NULL,
        NULL
        );

    ExFreePool(parameter_path.Buffer);

    if (!NT_SUCCESS(status))
    {
        DbgPrint("HttpDisk: Query registry failed, using default values.\n");
        n_devices = DEFAULT_NUMBEROFDEVICES;
    }

    RtlInitUnicodeString(&device_dir_name, DEVICE_DIR_NAME);

    InitializeObjectAttributes(
        &object_attributes,
        &device_dir_name,
        OBJ_PERMANENT,
        NULL,
        NULL
        );

    status = ZwCreateDirectoryObject(
        &dir_handle,
        DIRECTORY_ALL_ACCESS,
        &object_attributes
        );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    ZwMakeTemporaryObject(dir_handle);

    for (n = 0, n_created_devices = 0; n < n_devices; n++)
    {
        status = HttpDiskCreateDevice(DriverObject, n, FILE_DEVICE_DISK);

        if (NT_SUCCESS(status))
        {
            n_created_devices++;
        }
    }

    for (n = 0; n < n_devices; n++)
    {
        status = HttpDiskCreateDevice(DriverObject, n, FILE_DEVICE_CD_ROM);

        if (NT_SUCCESS(status))
        {
            n_created_devices++;
        }
    }

    if (n_created_devices == 0)
    {
        ZwClose(dir_handle);
        return status;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE]         = HttpDiskCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = HttpDiskCreateClose;
    DriverObject->MajorFunction[IRP_MJ_READ]           = HttpDiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE]          = HttpDiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HttpDiskDeviceControl;

    DriverObject->DriverUnload = HttpDiskUnload;

    return STATUS_SUCCESS;
}

NTSTATUS
HttpDiskCreateDevice (
    IN PDRIVER_OBJECT   DriverObject,
    IN ULONG            Number,
    IN DEVICE_TYPE      DeviceType
    )
{
    UNICODE_STRING      device_name;
    NTSTATUS            status;
    PDEVICE_OBJECT      device_object;
    PDEVICE_EXTENSION   device_extension;
    HANDLE              thread_handle;
    UNICODE_STRING      sddl;

    ASSERT(DriverObject != NULL);

    device_name.Buffer = (PWCHAR) ExAllocatePoolWithTag(PagedPool, MAXIMUM_FILENAME_LENGTH * 2, HTTP_DISK_POOL_TAG);

    if (device_name.Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    device_name.Length = 0;
    device_name.MaximumLength = MAXIMUM_FILENAME_LENGTH * 2;

    if (DeviceType == FILE_DEVICE_CD_ROM)
    {
        RtlUnicodeStringPrintf(&device_name, DEVICE_NAME_PREFIX L"Cd" L"%u", Number);
    }
    else
    {
        RtlUnicodeStringPrintf(&device_name, DEVICE_NAME_PREFIX L"Disk" L"%u", Number);
    }

    RtlInitUnicodeString(&sddl, _T("D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;BU)"));

    status = IoCreateDeviceSecure(
        DriverObject,
        sizeof(DEVICE_EXTENSION),
        &device_name,
        DeviceType,
        0,
        FALSE,
        &sddl,
        NULL,
        &device_object
        );

    if (!NT_SUCCESS(status))
    {
        ExFreePool(device_name.Buffer);
        return status;
    }

    device_object->Flags |= DO_DIRECT_IO;

    device_extension = (PDEVICE_EXTENSION) device_object->DeviceExtension;

    device_extension->media_in_device = FALSE;

    device_extension->device_name.Length = device_name.Length;
    device_extension->device_name.MaximumLength = device_name.MaximumLength;
    device_extension->device_name.Buffer = device_name.Buffer;
    device_extension->device_number = Number;
    device_extension->device_type = DeviceType;

    device_extension->host_name = NULL;

    device_extension->file_name = NULL;

    device_extension->socket = -1;

    device_object->Characteristics |= FILE_READ_ONLY_DEVICE;

    InitializeListHead(&device_extension->list_head);

    KeInitializeSpinLock(&device_extension->list_lock);

    KeInitializeEvent(
        &device_extension->request_event,
        SynchronizationEvent,
        FALSE
        );

    device_extension->terminate_thread = FALSE;

    status = PsCreateSystemThread(
        &thread_handle,
        (ACCESS_MASK) 0L,
        NULL,
        NULL,
        NULL,
        HttpDiskThread,
        device_object
        );

    if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(device_object);
        ExFreePool(device_name.Buffer);
        return status;
    }

    status = ObReferenceObjectByHandle(
        thread_handle,
        THREAD_ALL_ACCESS,
        NULL,
        KernelMode,
        &device_extension->thread_pointer,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        ZwClose(thread_handle);

        device_extension->terminate_thread = TRUE;

        KeSetEvent(
            &device_extension->request_event,
            (KPRIORITY) 0,
            FALSE
            );

        IoDeleteDevice(device_object);

        ExFreePool(device_name.Buffer);

        return status;
    }

    ZwClose(thread_handle);

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")

VOID
HttpDiskUnload (
    IN PDRIVER_OBJECT DriverObject
    )
{
    PDEVICE_OBJECT device_object;

    PAGED_CODE();

    device_object = DriverObject->DeviceObject;

    while (device_object)
    {
        device_object = HttpDiskDeleteDevice(device_object);
    }

    ZwClose(dir_handle);
}

#ifdef NTDDI_VERSION
#pragma warning(push)
#pragma warning(disable:28175)
#endif // NTDDI_VERSION

PDEVICE_OBJECT
HttpDiskDeleteDevice (
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_EXTENSION   device_extension;
    PDEVICE_OBJECT      next_device_object;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);

    device_extension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    device_extension->terminate_thread = TRUE;

    KeSetEvent(
        &device_extension->request_event,
        (KPRIORITY) 0,
        FALSE
        );

    KeWaitForSingleObject(
        device_extension->thread_pointer,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    ObDereferenceObject(device_extension->thread_pointer);

    if (device_extension->device_name.Buffer != NULL)
    {
        ExFreePool(device_extension->device_name.Buffer);
    }

    next_device_object = DeviceObject->NextDevice;

    IoDeleteDevice(DeviceObject);

    return next_device_object;
}

#ifdef NTDDI_VERSION
#pragma warning(pop)
#endif // NTDDI_VERSION

NTSTATUS
HttpDiskCreateClose (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = FILE_OPENED;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

#pragma code_seg() // end "PAGE"

NTSTATUS
HttpDiskReadWrite (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   device_extension;
    PIO_STACK_LOCATION  io_stack;

    device_extension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    if (!device_extension->media_in_device)
    {
        Irp->IoStatus.Status = STATUS_NO_MEDIA_IN_DEVICE;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_NO_MEDIA_IN_DEVICE;
    }

    io_stack = IoGetCurrentIrpStackLocation(Irp);

    if (io_stack->Parameters.Read.Length == 0)
    {
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_SUCCESS;
    }

    IoMarkIrpPending(Irp);

    ExInterlockedInsertTailList(
        &device_extension->list_head,
        &Irp->Tail.Overlay.ListEntry,
        &device_extension->list_lock
        );

    KeSetEvent(
        &device_extension->request_event,
        (KPRIORITY) 0,
        FALSE
        );

    return STATUS_PENDING;
}

NTSTATUS
HttpDiskDeviceControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   device_extension;
    PIO_STACK_LOCATION  io_stack;
    NTSTATUS            status;

    device_extension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    io_stack = IoGetCurrentIrpStackLocation(Irp);

    if (!device_extension->media_in_device &&
        io_stack->Parameters.DeviceIoControl.IoControlCode !=
        IOCTL_HTTP_DISK_CONNECT)
    {
        Irp->IoStatus.Status = STATUS_NO_MEDIA_IN_DEVICE;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_NO_MEDIA_IN_DEVICE;
    }

    switch (io_stack->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_HTTP_DISK_CONNECT:
        {
            if (device_extension->media_in_device)
            {
                DbgPrint("HttpDisk: IOCTL_HTTP_DISK_CONNECT: Media already opened.\n");

                status = STATUS_INVALID_DEVICE_REQUEST;
                Irp->IoStatus.Information = 0;
                break;
            }

            if (io_stack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(HTTP_DISK_INFORMATION))
            {
                status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Information = 0;
                break;
            }

            if (io_stack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(HTTP_DISK_INFORMATION) +
                ((PHTTP_DISK_INFORMATION)Irp->AssociatedIrp.SystemBuffer)->FileNameLength -
                sizeof(UCHAR))
            {
                status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Information = 0;
                break;
            }

            IoMarkIrpPending(Irp);

            ExInterlockedInsertTailList(
                &device_extension->list_head,
                &Irp->Tail.Overlay.ListEntry,
                &device_extension->list_lock
                );

            KeSetEvent(
                &device_extension->request_event,
                (KPRIORITY) 0,
                FALSE
                );

            status = STATUS_PENDING;

            break;
        }

    case IOCTL_HTTP_DISK_DISCONNECT:
        {
            IoMarkIrpPending(Irp);

            ExInterlockedInsertTailList(
                &device_extension->list_head,
                &Irp->Tail.Overlay.ListEntry,
                &device_extension->list_lock
                );

            KeSetEvent(
                &device_extension->request_event,
                (KPRIORITY) 0,
                FALSE
                );

            status = STATUS_PENDING;

            break;
        }

    case IOCTL_DISK_CHECK_VERIFY:
    case IOCTL_CDROM_CHECK_VERIFY:
    case IOCTL_STORAGE_CHECK_VERIFY:
    case IOCTL_STORAGE_CHECK_VERIFY2:
        {
            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            break;
        }

    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
    case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
        {
            PDISK_GEOMETRY  disk_geometry;
            ULONGLONG       length;
            ULONG           sector_size;

            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(DISK_GEOMETRY))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = 0;
                break;
            }

            disk_geometry = (PDISK_GEOMETRY) Irp->AssociatedIrp.SystemBuffer;

            length = device_extension->file_size.QuadPart;

            if (device_extension->device_type != FILE_DEVICE_CD_ROM)
            {
                sector_size = 512;
            }
            else
            {
                sector_size = 2048;
            }

            disk_geometry->Cylinders.QuadPart = length / sector_size / 32 / 2;
            disk_geometry->MediaType = FixedMedia;
            disk_geometry->TracksPerCylinder = 2;
            disk_geometry->SectorsPerTrack = 32;
            disk_geometry->BytesPerSector = sector_size;

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);

            break;
        }

    case IOCTL_DISK_GET_LENGTH_INFO:
        {
            PGET_LENGTH_INFORMATION get_length_information;

            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(GET_LENGTH_INFORMATION))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = 0;
                break;
            }

            get_length_information = (PGET_LENGTH_INFORMATION) Irp->AssociatedIrp.SystemBuffer;

            get_length_information->Length.QuadPart = device_extension->file_size.QuadPart;

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(GET_LENGTH_INFORMATION);

        break;
        }

    case IOCTL_DISK_GET_PARTITION_INFO:
        {
            PPARTITION_INFORMATION  partition_information;
            ULONGLONG               length;

            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(PARTITION_INFORMATION))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = 0;
                break;
            }

            partition_information = (PPARTITION_INFORMATION) Irp->AssociatedIrp.SystemBuffer;

            length = device_extension->file_size.QuadPart;

            partition_information->StartingOffset.QuadPart = 0;
            partition_information->PartitionLength.QuadPart = length;
            partition_information->HiddenSectors = 1;
            partition_information->PartitionNumber = 0;
            partition_information->PartitionType = 0;
            partition_information->BootIndicator = FALSE;
            partition_information->RecognizedPartition = FALSE;
            partition_information->RewritePartition = FALSE;

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);

            break;
        }

    case IOCTL_DISK_GET_PARTITION_INFO_EX:
        {
            PPARTITION_INFORMATION_EX   partition_information_ex;
            ULONGLONG                   length;

            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(PARTITION_INFORMATION_EX))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = 0;
                break;
            }

            partition_information_ex = (PPARTITION_INFORMATION_EX) Irp->AssociatedIrp.SystemBuffer;

            length = device_extension->file_size.QuadPart;

            partition_information_ex->PartitionStyle = PARTITION_STYLE_MBR;
            partition_information_ex->StartingOffset.QuadPart = 0;
            partition_information_ex->PartitionLength.QuadPart = length;
            partition_information_ex->PartitionNumber = 0;
            partition_information_ex->RewritePartition = FALSE;
            partition_information_ex->Mbr.PartitionType = 0;
            partition_information_ex->Mbr.BootIndicator = FALSE;
            partition_information_ex->Mbr.RecognizedPartition = FALSE;
            partition_information_ex->Mbr.HiddenSectors = 1;

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION_EX);

            break;
        }

    case IOCTL_DISK_IS_WRITABLE:
        {
            status = STATUS_MEDIA_WRITE_PROTECTED;
            Irp->IoStatus.Information = 0;
            break;
        }

    case IOCTL_DISK_MEDIA_REMOVAL:
    case IOCTL_STORAGE_MEDIA_REMOVAL:
        {
            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            break;
        }

    case IOCTL_CDROM_READ_TOC:
        {
            PCDROM_TOC cdrom_toc;

            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(CDROM_TOC))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = 0;
                break;
            }

            cdrom_toc = (PCDROM_TOC) Irp->AssociatedIrp.SystemBuffer;

            RtlZeroMemory(cdrom_toc, sizeof(CDROM_TOC));

            cdrom_toc->FirstTrack = 1;
            cdrom_toc->LastTrack = 1;
            cdrom_toc->TrackData[0].Control = TOC_DATA_TRACK;

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(CDROM_TOC);

            break;
        }

    case IOCTL_CDROM_GET_LAST_SESSION:
        {
            PCDROM_TOC_SESSION_DATA cdrom_toc_s_d;

            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(CDROM_TOC_SESSION_DATA))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = 0;
                break;
            }

            cdrom_toc_s_d = (PCDROM_TOC_SESSION_DATA) Irp->AssociatedIrp.SystemBuffer;

            RtlZeroMemory(cdrom_toc_s_d, sizeof(CDROM_TOC_SESSION_DATA));

            cdrom_toc_s_d->FirstCompleteSession = 1;
            cdrom_toc_s_d->LastCompleteSession = 1;
            cdrom_toc_s_d->TrackData[0].Control = TOC_DATA_TRACK;

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(CDROM_TOC_SESSION_DATA);

            break;
        }

    case IOCTL_DISK_SET_PARTITION_INFO:
        {
            status = STATUS_MEDIA_WRITE_PROTECTED;
            Irp->IoStatus.Information = 0;
            break;
        }

    case IOCTL_DISK_VERIFY:
        {
            PVERIFY_INFORMATION verify_information;

            if (io_stack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(VERIFY_INFORMATION))
            {
                status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Information = 0;
                break;
            }

            verify_information = (PVERIFY_INFORMATION) Irp->AssociatedIrp.SystemBuffer;

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = verify_information->Length;

            break;
        }

    case IOCTL_STORAGE_GET_DEVICE_NUMBER:
        {
            PSTORAGE_DEVICE_NUMBER number;

            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(STORAGE_DEVICE_NUMBER))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = 0;
                break;
            }

            number = (PSTORAGE_DEVICE_NUMBER) Irp->AssociatedIrp.SystemBuffer;

            number->DeviceType = device_extension->device_type;
            number->DeviceNumber = device_extension->device_number;
            number->PartitionNumber = -1;

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(STORAGE_DEVICE_NUMBER);

            break;
        }

    case IOCTL_STORAGE_GET_HOTPLUG_INFO:
        {
            PSTORAGE_HOTPLUG_INFO info;

            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(STORAGE_HOTPLUG_INFO))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = 0;
                break;
            }

            info = (PSTORAGE_HOTPLUG_INFO) Irp->AssociatedIrp.SystemBuffer;

            info->Size = sizeof(STORAGE_HOTPLUG_INFO); 
            info->MediaRemovable = 0; 
            info->MediaHotplug = 0;
            info->DeviceHotplug = 0;
            info->WriteCacheEnableOverride = 0;

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(STORAGE_HOTPLUG_INFO);

            break;
        }

    case IOCTL_VOLUME_GET_GPT_ATTRIBUTES:
        {
            PVOLUME_GET_GPT_ATTRIBUTES_INFORMATION attr;

            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(VOLUME_GET_GPT_ATTRIBUTES_INFORMATION))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = 0;
                break;
            }

            attr = (PVOLUME_GET_GPT_ATTRIBUTES_INFORMATION) Irp->AssociatedIrp.SystemBuffer;

            attr->GptAttributes = 0;

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(VOLUME_GET_GPT_ATTRIBUTES_INFORMATION);

            break;
        }

    case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:
        {
            PVOLUME_DISK_EXTENTS ext;

            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(VOLUME_DISK_EXTENTS))
            {
                status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Information = 0;
                break;
            }
/*
            // not needed since there is only one disk extent to return
            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(VOLUME_DISK_EXTENTS) + ((NumberOfDiskExtents - 1) * sizeof(DISK_EXTENT)))
            {
                status = STATUS_BUFFER_OVERFLOW;
                Irp->IoStatus.Information = 0;
                break;
            }
*/
            ext = (PVOLUME_DISK_EXTENTS) Irp->AssociatedIrp.SystemBuffer;

            ext->NumberOfDiskExtents = 1;
            ext->Extents[0].DiskNumber = device_extension->device_number;
            ext->Extents[0].StartingOffset.QuadPart = 0;
            ext->Extents[0].ExtentLength.QuadPart = device_extension->file_size.QuadPart;

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(VOLUME_DISK_EXTENTS) /*+ ((NumberOfDiskExtents - 1) * sizeof(DISK_EXTENT))*/;

            break;
        }

#if (NTDDI_VERSION < NTDDI_VISTA)
#define IOCTL_DISK_IS_CLUSTERED CTL_CODE(IOCTL_DISK_BASE, 0x003e, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif  // NTDDI_VERSION < NTDDI_VISTA

    case IOCTL_DISK_IS_CLUSTERED:
        {
            PBOOLEAN clus;

            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(BOOLEAN))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = 0;
                break;
            }

            clus = (PBOOLEAN) Irp->AssociatedIrp.SystemBuffer;

            *clus = FALSE;

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(BOOLEAN);

            break;
        }

    case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
        {
            PMOUNTDEV_NAME name;

            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(MOUNTDEV_NAME))
            {
                status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Information = 0;
                break;
            }

            name = (PMOUNTDEV_NAME) Irp->AssociatedIrp.SystemBuffer;
            name->NameLength = device_extension->device_name.Length * sizeof(WCHAR);

            if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
                name->NameLength + sizeof(USHORT))
            {
                status = STATUS_BUFFER_OVERFLOW;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
                break;
            }

            RtlCopyMemory(name->Name, device_extension->device_name.Buffer, name->NameLength);

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = name->NameLength + sizeof(USHORT);

            break;
        }

    case IOCTL_CDROM_READ_TOC_EX:
        {
            KdPrint(("HttpDisk: unhandled ioctl IOCTL_CDROM_READ_TOC_EX\n"));
            status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Information = 0;
            break;
        }
    case IOCTL_DISK_GET_MEDIA_TYPES:
        {
            KdPrint(("HttpDisk: unhandled ioctl IOCTL_DISK_GET_MEDIA_TYPES\n"));
            status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Information = 0;
            break;
        }
    case FT_BALANCED_READ_MODE:
        {
            KdPrint(("HttpDisk: unhandled ioctl FT_BALANCED_READ_MODE\n"));
            status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Information = 0;
            break;
        }
    case IOCTL_SCSI_GET_CAPABILITIES:
        {
            KdPrint(("HttpDisk: unhandled ioctl IOCTL_SCSI_GET_CAPABILITIES\n"));
            status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Information = 0;
            break;
        }
    case IOCTL_SCSI_PASS_THROUGH:
        {
            KdPrint(("HttpDisk: unhandled ioctl IOCTL_SCSI_PASS_THROUGH\n"));
            status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Information = 0;
            break;
        }
    case IOCTL_STORAGE_GET_MEDIA_TYPES_EX:
        {
            KdPrint(("HttpDisk: unhandled ioctl IOCTL_STORAGE_GET_MEDIA_TYPES_EX\n"));
            status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Information = 0;
            break;
        }
    case IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES:
        {
            KdPrint(("HttpDisk: unhandled ioctl IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES\n"));
            status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Information = 0;
            break;
        }
    case IOCTL_STORAGE_QUERY_PROPERTY:
        {
            KdPrint(("HttpDisk: unhandled ioctl IOCTL_STORAGE_QUERY_PROPERTY\n"));
            status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Information = 0;
            break;
        }

#if (NTDDI_VERSION < NTDDI_VISTA)
#define IOCTL_VOLUME_QUERY_ALLOCATION_HINT CTL_CODE(IOCTL_VOLUME_BASE, 20, METHOD_OUT_DIRECT, FILE_READ_ACCESS)
#endif  // NTDDI_VERSION < NTDDI_VISTA

    case IOCTL_VOLUME_QUERY_ALLOCATION_HINT:
        {
            KdPrint(("HttpDisk: unhandled ioctl IOCTL_VOLUME_QUERY_ALLOCATION_HINT\n"));
            status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Information = 0;
            break;
        }
    default:
        {
            KdPrint((
                "HttpDisk: unknown ioctl %#x\n",
                io_stack->Parameters.DeviceIoControl.IoControlCode
                ));

            status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Information = 0;
        }
    }

    if (status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = status;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return status;
}

#pragma code_seg("PAGE")

VOID
HttpDiskThread (
    IN PVOID Context
    )
{
    PDEVICE_OBJECT      device_object;
    PDEVICE_EXTENSION   device_extension;
    PLIST_ENTRY         request;
    PIRP                irp;
    PIO_STACK_LOCATION  io_stack;

    PAGED_CODE();

    ASSERT(Context != NULL);

    device_object = (PDEVICE_OBJECT) Context;

    device_extension = (PDEVICE_EXTENSION) device_object->DeviceExtension;

    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

    for (;;)
    {
        KeWaitForSingleObject(
            &device_extension->request_event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        if (device_extension->terminate_thread)
        {
            PsTerminateSystemThread(STATUS_SUCCESS);
        }

        while (request = ExInterlockedRemoveHeadList(
            &device_extension->list_head,
            &device_extension->list_lock
            ))
        {
            irp = CONTAINING_RECORD(request, IRP, Tail.Overlay.ListEntry);

            io_stack = IoGetCurrentIrpStackLocation(irp);

            switch (io_stack->MajorFunction)
            {
            case IRP_MJ_READ:
                HttpDiskGetBlock(
                    &device_extension->socket,
                    device_extension->address,
                    device_extension->port,
                    device_extension->host_name,
                    device_extension->file_name,
                    &io_stack->Parameters.Read.ByteOffset,
                    io_stack->Parameters.Read.Length,
                    &irp->IoStatus,
                    MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority)
                    );
                if (!NT_SUCCESS(irp->IoStatus.Status))
                {
                    DbgPrint("HttpDisk: retrying get block: offset=%I64u length=%u\n", io_stack->Parameters.Read.ByteOffset, io_stack->Parameters.Read.Length);
                    HttpDiskGetBlock(
                        &device_extension->socket,
                        device_extension->address,
                        device_extension->port,
                        device_extension->host_name,
                        device_extension->file_name,
                        &io_stack->Parameters.Read.ByteOffset,
                        io_stack->Parameters.Read.Length,
                        &irp->IoStatus,
                        MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority)
                        );
                    if (!NT_SUCCESS(irp->IoStatus.Status))
                    {
                        DbgPrint("HttpDisk: get block failed\n");
                    }
                }
                break;

            case IRP_MJ_WRITE:
                irp->IoStatus.Status = STATUS_MEDIA_WRITE_PROTECTED;
                irp->IoStatus.Information = 0;
                break;

            case IRP_MJ_DEVICE_CONTROL:
                switch (io_stack->Parameters.DeviceIoControl.IoControlCode)
                {
                case IOCTL_HTTP_DISK_CONNECT:
                    irp->IoStatus.Status = HttpDiskConnect(device_object, irp);
                    break;

                case IOCTL_HTTP_DISK_DISCONNECT:
                    irp->IoStatus.Status = HttpDiskDisconnect(device_object, irp);
                    break;

                default:
                    irp->IoStatus.Status = STATUS_DRIVER_INTERNAL_ERROR;
                }
                break;

            default:
                irp->IoStatus.Status = STATUS_DRIVER_INTERNAL_ERROR;
            }

            IoCompleteRequest(
                irp,
                (CCHAR) (NT_SUCCESS(irp->IoStatus.Status) ?
                IO_DISK_INCREMENT : IO_NO_INCREMENT)
                );
        }
    }
}

NTSTATUS
HttpDiskConnect (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION       device_extension;
    PHTTP_DISK_INFORMATION  http_disk_information;
    HTTP_HEADER             http_header;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);
    ASSERT(Irp != NULL);

    device_extension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    http_disk_information = (PHTTP_DISK_INFORMATION) Irp->AssociatedIrp.SystemBuffer;

    device_extension->address = http_disk_information->Address;

    device_extension->port = http_disk_information->Port;

    device_extension->host_name = ExAllocatePoolWithTag(NonPagedPool, http_disk_information->HostNameLength + 1, HTTP_DISK_POOL_TAG);

    if (device_extension->host_name == NULL)
    {
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        return Irp->IoStatus.Status;
    }

    RtlCopyMemory(
        device_extension->host_name,
        http_disk_information->HostName,
        http_disk_information->HostNameLength
        );

    device_extension->host_name[http_disk_information->HostNameLength] = '\0';

    device_extension->file_name = ExAllocatePoolWithTag(NonPagedPool, http_disk_information->FileNameLength + 1, HTTP_DISK_POOL_TAG);

    if (device_extension->file_name == NULL)
    {
        if (device_extension->host_name != NULL)
        {
            ExFreePool(device_extension->host_name);
            device_extension->host_name = NULL;
        }

        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        return Irp->IoStatus.Status;
    }

    RtlCopyMemory(
        device_extension->file_name,
        http_disk_information->FileName,
        http_disk_information->FileNameLength
        );

    device_extension->file_name[http_disk_information->FileNameLength] = '\0';

    HttpDiskGetHeader(
        device_extension->address,
        device_extension->port,
        device_extension->host_name,
        device_extension->file_name,
        &Irp->IoStatus,
        &http_header
        );

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        DbgPrint("HttpDisk: retrying get header\n");
        HttpDiskGetHeader(
            device_extension->address,
            device_extension->port,
            device_extension->host_name,
            device_extension->file_name,
            &Irp->IoStatus,
            &http_header
            );
    }

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        DbgPrint("HttpDisk: get header failed\n");

        if (device_extension->host_name != NULL)
        {
            ExFreePool(device_extension->host_name);
            device_extension->host_name = NULL;
        }

        if (device_extension->file_name != NULL)
        {
            ExFreePool(device_extension->file_name);
            device_extension->file_name = NULL;
        }

        return Irp->IoStatus.Status;
    }

    device_extension->file_size.QuadPart = http_header.ContentLength.QuadPart;

    device_extension->media_in_device = TRUE;

    return Irp->IoStatus.Status;
}

NTSTATUS
HttpDiskDisconnect (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION device_extension;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);
    ASSERT(Irp != NULL);

    device_extension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    device_extension->media_in_device = FALSE;

    if (device_extension->host_name != NULL)
    {
        ExFreePool(device_extension->host_name);
        device_extension->host_name = NULL;
    }

    if (device_extension->file_name != NULL)
    {
        ExFreePool(device_extension->file_name);
        device_extension->file_name = NULL;
    }

    if (device_extension->socket != -1)
    {
        close(device_extension->socket);
        device_extension->socket = -1;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    return STATUS_SUCCESS;
}

NTSTATUS
HttpDiskGetHeader (
    IN ULONG                Address,
    IN USHORT               Port,
    IN PUCHAR               HostName,
    IN PUCHAR               FileName,
    OUT PIO_STATUS_BLOCK    IoStatus,
    OUT PHTTP_HEADER        HttpHeader
    )
{
    INT_PTR             kSocket;
    struct sockaddr_in  toAddr;
    int                 status, nSent, nRecv;
    char                *request, *buffer, *pStr;

    PAGED_CODE();

    ASSERT(HostName != NULL);
    ASSERT(FileName != NULL);
    ASSERT(IoStatus != NULL);
    ASSERT(HttpHeader != NULL);

    request = ExAllocatePoolWithTag(PagedPool, PAGE_SIZE, HTTP_DISK_POOL_TAG);

    if (request == NULL)
    {
        IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
        return IoStatus->Status;
    }

    buffer = ExAllocatePoolWithTag(PagedPool, BUFFER_SIZE, HTTP_DISK_POOL_TAG);

    if (buffer == NULL)
    {
        ExFreePool(request);
        IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
        return IoStatus->Status;
    }

    kSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (kSocket == -1)
    {
        KdPrint(("HttpDisk: get header: socket() returned -1\n"));
        ExFreePool(request);
        ExFreePool(buffer);
        IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
        return IoStatus->Status;
    }

    toAddr.sin_family = AF_INET;
    toAddr.sin_port = Port;
    toAddr.sin_addr.s_addr = Address;

    status = connect(kSocket, (struct sockaddr*) &toAddr, sizeof(toAddr));

    if (status < 0)
    {
        KdPrint(("HttpDisk: get header: connect() error: %#x\n", status));
        ExFreePool(request);
        ExFreePool(buffer);
        close(kSocket);
        IoStatus->Status = status;
        return IoStatus->Status;
    }

    // Example request:
    //  HEAD 'FileName' HTTP/1.1
    //  Host: 'HostName'
    //  Accept: */*
    //  User-Agent: HttpDisk/1.x
    //  Connection: close
    //
    // Interesting lines in response:
    //  HTTP/1.1 200 OK
    //  Accept-Ranges: bytes
    //  Content-Length: 'total file size'

    RtlStringCbPrintfA(
        request,
        PAGE_SIZE,
        "HEAD %s HTTP/1.1\r\nHost: %s\r\nAccept: */*\r\nUser-Agent: HttpDisk/1.8\r\nConnection: close\r\n\r\n",
        FileName,
        HostName
        );

    nSent = send(kSocket, request, strlen(request), 0);

    if (nSent < 0)
    {
        KdPrint(("HttpDisk: get header: send() error: %#x\n", nSent));
        ExFreePool(request);
        ExFreePool(buffer);
        close(kSocket);
        IoStatus->Status = nSent;
        return IoStatus->Status;
    }
    if (nSent < (int) strlen(request))
    {
        KdPrint(("HttpDisk: get header: send() did not complete: %d < %d\n", nSent, strlen(request)));
    }

    nRecv = recv(kSocket, buffer, BUFFER_SIZE, 0);

    if (nRecv <= 0)
    {
        if (nRecv == 0)
        {
            KdPrint(("HttpDisk: get header: server disconnected\n"));
            IoStatus->Status = STATUS_NO_SUCH_FILE;
        }
        else
        {
            KdPrint(("HttpDisk: get header: recv() error: %#x\n", nRecv));
            IoStatus->Status = nRecv;
        }
        ExFreePool(request);
        ExFreePool(buffer);
        close(kSocket);
        return IoStatus->Status;
    }

    close(kSocket);

    buffer[BUFFER_SIZE - 1] = '\0';

    KdPrint(("HttpDisk: get header HTTP response:\n"
             "-----------------------------------\n"
             "%.*s"
             "-----------------------------------\n",
             nRecv - 2, buffer
             ));

    if (_strnicmp(buffer, "HTTP/1.1 200 OK", 15))
    {
        if (_strnicmp(buffer, "HTTP/1.1 404 Not Found", 22))
        {
            KdPrint(("HttpDisk: get header error: other error than \'file not found\'\n"));
        }
        else
        {
            KdPrint(("HttpDisk: get header: file not found\n"));
        }
        ExFreePool(request);
        ExFreePool(buffer);
        IoStatus->Status = STATUS_NO_SUCH_FILE;
        return IoStatus->Status;
    }

    pStr = strstr(buffer, "Content-Length:");

    if (pStr == NULL || pStr + 16 >= buffer + BUFFER_SIZE)
    {
        KdPrint(("HttpDisk: get header error: field \'Content-Length\' not found\n"));
        ExFreePool(request);
        ExFreePool(buffer);
        IoStatus->Status = STATUS_NO_SUCH_FILE;
        return IoStatus->Status;
    }

    HttpHeader->ContentLength.QuadPart = _atoi64(pStr + 16);

    if (HttpHeader->ContentLength.QuadPart == 0)
    {
        KdPrint(("HttpDisk: get header error: field \'Content-Length\' not interpreted correctly\n"));
        ExFreePool(request);
        ExFreePool(buffer);
        IoStatus->Status = STATUS_NO_SUCH_FILE;
        return IoStatus->Status;
    }

    ExFreePool(request);
    ExFreePool(buffer);

    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = 0;

    return STATUS_SUCCESS;
}

NTSTATUS
HttpDiskGetBlock (
    IN INT_PTR              *Socket,
    IN ULONG                Address,
    IN USHORT               Port,
    IN PUCHAR               HostName,
    IN PUCHAR               FileName,
    IN PLARGE_INTEGER       Offset,
    IN ULONG                Length,
    OUT PIO_STATUS_BLOCK    IoStatus,
    OUT PVOID               SystemBuffer
    )
{
    struct sockaddr_in  toAddr;
    int                 status, nSent, nRecv;
    unsigned int        dataLen;
    char                *request, *buffer, *pDataPart;

    PAGED_CODE();

    ASSERT(Socket != NULL);
    ASSERT(HostName != NULL);
    ASSERT(FileName != NULL);
    ASSERT(Offset != NULL);
    ASSERT(IoStatus != NULL);
    ASSERT(SystemBuffer != NULL);

    IoStatus->Information = 0;

    request = ExAllocatePoolWithTag(PagedPool, PAGE_SIZE, HTTP_DISK_POOL_TAG);

    if (request == NULL)
    {
        IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
        return IoStatus->Status;
    }

    buffer = ExAllocatePoolWithTag(PagedPool, BUFFER_SIZE + 1, HTTP_DISK_POOL_TAG);

    if (buffer == NULL)
    {
        ExFreePool(request);
        IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
        return IoStatus->Status;
    }

    // Example request:
    //  GET 'FileName' HTTP/1.1
    //  Host: 'HostName'
    //  Range: bytes='Offset'-'Offset + Length - 1'
    //  Accept: */*
    //  User-Agent: HttpDisk/1.x
    //
    // Interesting lines in response:
    //  HTTP/1.1 206 Partial content
    //  Content-Length: 'requested size'
    //  Content-Range: bytes 'start'-'end'/'total file size'
    //  Data follows after '\r\n\r\n'

    RtlStringCbPrintfA(
        request,
        PAGE_SIZE,
        "GET %s HTTP/1.1\r\nHost: %s\r\nRange: bytes=%I64u-%I64u\r\nAccept: */*\r\nUser-Agent: HttpDisk/1.8\r\n\r\n",
        FileName,
        HostName,
        Offset->QuadPart,
        Offset->QuadPart + Length - 1
        );

    if (*Socket == -1)
    {
        *Socket = socket(AF_INET, SOCK_STREAM, 0);

        if (*Socket == -1)
        {
            KdPrint(("HttpDisk: get block: socket() returned -1\n"));
            ExFreePool(request);
            ExFreePool(buffer);
            IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
            return IoStatus->Status;
        }

        toAddr.sin_family = AF_INET;
        toAddr.sin_port = Port;
        toAddr.sin_addr.s_addr = Address;

        status = connect(*Socket, (struct sockaddr*) &toAddr, sizeof(toAddr));

        if (status < 0)
        {
            KdPrint(("HttpDisk: get block: connect() error: %#x\n", status));
            ExFreePool(request);
            ExFreePool(buffer);
            close(*Socket);
            *Socket = -1;
            IoStatus->Status = status;
            return IoStatus->Status;
        }
    }

    nSent = send(*Socket, request, strlen(request), 0);

    if (nSent < 0)
    {
        KdPrint(("HttpDisk: get block: send() error: %#x, retrying send()\n", nSent));

        close(*Socket);

        *Socket = socket(AF_INET, SOCK_STREAM, 0);

        if (*Socket == -1)
        {
            KdPrint(("HttpDisk: get block: socket() returned -1\n"));
            ExFreePool(request);
            ExFreePool(buffer);
            IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
            return IoStatus->Status;
        }

        toAddr.sin_family = AF_INET;
        toAddr.sin_port = Port;
        toAddr.sin_addr.s_addr = Address;

        status = connect(*Socket, (struct sockaddr*) &toAddr, sizeof(toAddr));

        if (status < 0)
        {
            KdPrint(("HttpDisk: get block: connect() error: %#x\n", status));
            ExFreePool(request);
            ExFreePool(buffer);
            close(*Socket);
            *Socket = -1;
            IoStatus->Status = status;
            return IoStatus->Status;
        }

        nSent = send(*Socket, request, strlen(request), 0);

        if (nSent < 0)
        {
            KdPrint(("HttpDisk: get block: send() error: %#x, returning\n", nSent));
            ExFreePool(request);
            ExFreePool(buffer);
            close(*Socket);
            *Socket = -1;
            IoStatus->Status = nSent;
            return IoStatus->Status;
        }
    }
    if (nSent < (int) strlen(request))
    {
        KdPrint(("HttpDisk: get block: send() did not complete: %d < %d\n", nSent, strlen(request)));
    }

    nRecv = recv(*Socket, buffer, BUFFER_SIZE, 0);

    if (nRecv <= 0)
    {
        if (nRecv == 0)
        {
            KdPrint(("HttpDisk: get block: server disconnected, retrying both send() and recv()\n"));
        }
        else
        {
            KdPrint(("HttpDisk: get block: recv() error: %#x, retrying both send() and recv()\n", nRecv));
        }

        close(*Socket);

        *Socket = socket(AF_INET, SOCK_STREAM, 0);

        if (*Socket == -1)
        {
            KdPrint(("HttpDisk: get block: socket() returned -1\n"));
            ExFreePool(request);
            ExFreePool(buffer);
            IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
            return IoStatus->Status;
        }

        toAddr.sin_family = AF_INET;
        toAddr.sin_port = Port;
        toAddr.sin_addr.s_addr = Address;

        status = connect(*Socket, (struct sockaddr*) &toAddr, sizeof(toAddr));

        if (status < 0)
        {
            KdPrint(("HttpDisk: get block: connect() error: %#x\n", status));
            ExFreePool(request);
            ExFreePool(buffer);
            close(*Socket);
            *Socket = -1;
            IoStatus->Status = status;
            return IoStatus->Status;
        }

        nSent = send(*Socket, request, strlen(request), 0);

        if (nSent < 0)
        {
            KdPrint(("HttpDisk: get block: send() error: %#x\n", nSent));
            ExFreePool(request);
            ExFreePool(buffer);
            close(*Socket);
            *Socket = -1;
            IoStatus->Status = nSent;
            return IoStatus->Status;
        }
        if (nSent < (int) strlen(request))
        {
            KdPrint(("HttpDisk: get block: send() did not complete: %d < %d\n", nSent, strlen(request)));
        }

        nRecv = recv(*Socket, buffer, BUFFER_SIZE, 0);

        if (nRecv <= 0)
        {
            if (nRecv == 0)
            {
                KdPrint(("HttpDisk: get block: server disconnected, returning\n"));
            }
            else
            {
                KdPrint(("HttpDisk: get block: recv() error: %#x, returning\n", nRecv));
            }
            ExFreePool(request);
            ExFreePool(buffer);
            close(*Socket);
            *Socket = -1;
            IoStatus->Status = nRecv;
            return IoStatus->Status;
        }
    }

    buffer[BUFFER_SIZE] = '\0';

    if (_strnicmp(buffer, "HTTP/1.1 206 Partial Content", 28))
    {
        KdPrint(("HttpDisk: get block error: field \'206 Partial Content\' not found\n"));
        ExFreePool(request);
        ExFreePool(buffer);
        close(*Socket);
        *Socket = -1;
        IoStatus->Status = STATUS_UNSUCCESSFUL;
        return IoStatus->Status;
    }

    pDataPart = strstr(buffer, "\r\n\r\n") + 4;

    if (pDataPart == NULL || pDataPart < buffer || pDataPart > buffer + BUFFER_SIZE)
    {
        KdPrint(("HttpDisk: get block error: invalid HTTP response\n"));
        ExFreePool(request);
        ExFreePool(buffer);
        close(*Socket);
        *Socket = -1;
        IoStatus->Status = STATUS_UNSUCCESSFUL;
        return IoStatus->Status;
    }

    dataLen = nRecv - (unsigned int) (pDataPart - buffer);

    if (dataLen > Length || pDataPart + dataLen > buffer + BUFFER_SIZE)
    {
        KdPrint(("HttpDisk: get block error: invalid data length in HTTP response\n"));
        ExFreePool(request);
        ExFreePool(buffer);
        close(*Socket);
        *Socket = -1;
        IoStatus->Status = STATUS_UNSUCCESSFUL;
        return IoStatus->Status;
    }

    if (dataLen > 0)
    {
        RtlCopyMemory(
            SystemBuffer,
            pDataPart,
            dataLen
            );
    }

    while (dataLen < Length)
    {
        nRecv = recv(*Socket, buffer, ((Length - dataLen) > BUFFER_SIZE) ? BUFFER_SIZE : (Length - dataLen), 0);
        if (nRecv <= 0)
        {
            if (nRecv == 0)
            {
                KdPrint(("HttpDisk: get block: server disconnected in receive loop\n"));
            }
            else
            {
                KdPrint(("HttpDisk: get block: recv() error in receive loop: %#x\n", nRecv));
            }
            close(*Socket);
            *Socket = -1;
            break;
        }
        if (dataLen + nRecv > Length || nRecv > BUFFER_SIZE)
        {
            KdPrint(("HttpDisk: get block: invalid data length in receive loop: %u,%u,%u\n", dataLen, nRecv, Length));
            close(*Socket);
            *Socket = -1;
            break;
        }
        RtlCopyMemory(
            (PVOID)((PUCHAR) SystemBuffer + dataLen),
            buffer,
            nRecv
        );
        dataLen += nRecv;
    }

    if (dataLen != Length)
    {
        DbgPrint("HttpDisk: get block: received=%u expected=%u\n", dataLen, Length);
    }

    ExFreePool(request);
    ExFreePool(buffer);
    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = dataLen;
    return IoStatus->Status;
}

#pragma code_seg() // end "PAGE"
