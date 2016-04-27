/*
    This is a romfs file system driver for Windows NT/2000/XP.
    Copyright (C) 1999, 2000, 2001, 2002 Bo Brantén.
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

#include "ntifs.h"
#include "fsd.h"

#ifdef NTDDI_VERSION
IO_COMPLETION_ROUTINE FsdReadWriteBlockDeviceAtApcLevelCompletion;
#endif // NTDDI_VERSION

NTSTATUS
FsdReadWriteBlockDeviceAtApcLevelCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

#pragma code_seg(FSD_PAGED_CODE)

NTSTATUS 
FsdBlockDeviceIoControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN ULONG            IoctlCode,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferSize,
    IN OUT PVOID        OutputBuffer,
    IN OUT PULONG       OutputBufferSize
    )
{
    ULONG           OutputBufferSize2 = 0;
    KEVENT          Event;
    PIRP            Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS        Status;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);

    if (OutputBufferSize)
    {
        OutputBufferSize2 = *OutputBufferSize;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(
        IoctlCode,
        DeviceObject,
        InputBuffer,
        InputBufferSize,
        OutputBuffer,
        OutputBufferSize2,
        FALSE,
        &Event,
        &IoStatus
        );

    if (!Irp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        Status = IoStatus.Status;
    }

    if (OutputBufferSize)
    {
        *OutputBufferSize = (ULONG) IoStatus.Information;
    }

    return Status;
}

NTSTATUS
FsdReadBlockDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PLARGE_INTEGER   Offset,
    IN ULONG            Length,
    IN OUT PVOID        Buffer
    )
{
    KEVENT          Event;
    PIRP            Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS        Status;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);
    ASSERT(Offset != NULL);
    ASSERT(Buffer != NULL);

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(
        IRP_MJ_READ,
        DeviceObject,
        Buffer,
        Length,
        Offset,
        &Event,
        &IoStatus
        );

    if (!Irp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        Status = IoStatus.Status;
    }

    return Status;
}

#ifndef FSD_RO

NTSTATUS
FsdWriteBlockDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PLARGE_INTEGER   Offset,
    IN ULONG            Length,
    IN PVOID            Buffer
    )
{
    KEVENT          Event;
    PIRP            Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS        Status;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);
    ASSERT(Offset != NULL);
    ASSERT(Buffer != NULL);

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(
        IRP_MJ_WRITE,
        DeviceObject,
        Buffer,
        Length,
        Offset,
        &Event,
        &IoStatus
        );

    if (!Irp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        Status = IoStatus.Status;
    }

    return Status;
}

#endif // !FSD_RO

NTSTATUS
FsdReadBlockDeviceOverrideVerify (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PLARGE_INTEGER   Offset,
    IN ULONG            Length,
    IN OUT PVOID        Buffer
    )
{
    KEVENT              Event;
    PIRP                Irp;
    IO_STATUS_BLOCK     IoStatus;
    NTSTATUS            Status;
    PIO_STACK_LOCATION  IoStackLocation;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);
    ASSERT(Offset != NULL);
    ASSERT(Buffer != NULL);

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(
        IRP_MJ_READ,
        DeviceObject,
        Buffer,
        Length,
        Offset,
        &Event,
        &IoStatus
        );

    if (!Irp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoStackLocation = IoGetNextIrpStackLocation(Irp);

    SetFlag(IoStackLocation->Flags, SL_OVERRIDE_VERIFY_VOLUME);

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        Status = IoStatus.Status;
    }

    return Status;
}

#pragma code_seg() // end FSD_PAGED_CODE

NTSTATUS
FsdReadBlockDeviceAtApcLevel (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PLARGE_INTEGER   Offset,
    IN ULONG            Length,
    IN OUT PVOID        Buffer
    )
{
    PIRP                Irp;
    IO_STATUS_BLOCK     IoStatus;
    NTSTATUS            Status;
    KEVENT              Event;

    ASSERT(DeviceObject != NULL);
    ASSERT(Offset != NULL);
    ASSERT(Buffer != NULL);

    Irp = IoBuildAsynchronousFsdRequest(
        IRP_MJ_READ,
        DeviceObject,
        Buffer,
        Length,
        Offset,
        &IoStatus
        );

    if (!Irp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp->UserEvent = &Event;

    IoSetCompletionRoutine(
        Irp,
        FsdReadWriteBlockDeviceAtApcLevelCompletion,
        NULL,
        TRUE,
        TRUE,
        TRUE
        );

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        Status = IoStatus.Status;
    }

    return Status;
}

#ifndef FSD_RO

NTSTATUS
FsdWriteBlockDeviceAtApcLevel (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PLARGE_INTEGER   Offset,
    IN ULONG            Length,
    IN OUT PVOID        Buffer
    )
{
    PIRP                Irp;
    IO_STATUS_BLOCK     IoStatus;
    NTSTATUS            Status;
    KEVENT              Event;

    ASSERT(DeviceObject != NULL);
    ASSERT(Offset != NULL);
    ASSERT(Buffer != NULL);

    Irp = IoBuildAsynchronousFsdRequest(
        IRP_MJ_WRITE,
        DeviceObject,
        Buffer,
        Length,
        Offset,
        &IoStatus
        );

    if (!Irp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp->UserEvent = &Event;

    IoSetCompletionRoutine(
        Irp,
        FsdReadWriteBlockDeviceAtApcLevelCompletion,
        NULL,
        TRUE,
        TRUE,
        TRUE
        );

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        Status = IoStatus.Status;
    }

    return Status;
}

#endif // !FSD_RO

NTSTATUS
FsdReadWriteBlockDeviceAtApcLevelCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PMDL Mdl;

    ASSERT(Irp != NULL);

    *Irp->UserIosb = Irp->IoStatus;

	if (Irp->PendingReturned)
	{
        KeSetEvent(Irp->UserEvent, IO_NO_INCREMENT, FALSE);
	}

    if (Irp->AssociatedIrp.SystemBuffer && FlagOn(Irp->Flags, IRP_DEALLOCATE_BUFFER))
    {
        ExFreePool(Irp->AssociatedIrp.SystemBuffer);
    }
    else
    {
        while ((Mdl = Irp->MdlAddress))
        {
            Irp->MdlAddress = Mdl->Next;

            IoFreeMdl(Mdl);
        }
    }

    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}
