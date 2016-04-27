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

NTSTATUS
FsdPrepareToUnload (
    IN PFSD_IRP_CONTEXT IrpContext
    );

NTSTATUS
FsdDeviceControlNormal (
    IN PFSD_IRP_CONTEXT IrpContext
    );

#ifdef NTDDI_VERSION
IO_COMPLETION_ROUTINE FsdDeviceControlCompletion;
#endif // NTDDI_VERSION

NTSTATUS
FsdDeviceControlCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

#pragma code_seg(FSD_PAGED_CODE)

__drv_mustHoldCriticalRegion
NTSTATUS
FsdDeviceControl (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    ULONG               IoControlCode;
    NTSTATUS            Status;

    PAGED_CODE();

    ASSERT(IrpContext != NULL);

    ASSERT((IrpContext->Identifier.Type == ICX) &&
           (IrpContext->Identifier.Size == sizeof(FSD_IRP_CONTEXT)));

    Irp = IrpContext->Irp;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

    switch (IoControlCode)
    {
#if DBG
    case IOCTL_PREPARE_TO_UNLOAD:
        Status = FsdPrepareToUnload(IrpContext);
        break;
#endif

    default:
        Status = FsdDeviceControlNormal(IrpContext);
    }

    return Status;
}

#if DBG

/*
 * A file system driver that has called IoRegisterFileSystem can not be
 * unloaded untill it has called IoUnregisterFileSystem. Therefor we implement
 * an IOCTL_PREPARE_TO_UNLOAD that an application can call to make the driver
 * ready to unload.
 */

__drv_mustHoldCriticalRegion
NTSTATUS
FsdPrepareToUnload (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    BOOLEAN         GlobalDataResourceAcquired = FALSE;

    PAGED_CODE();

    __try
    {
        ASSERT(IrpContext != NULL);

        ASSERT((IrpContext->Identifier.Type == ICX) &&
               (IrpContext->Identifier.Size == sizeof(FSD_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;

        if (DeviceObject != FsdGlobalData.DeviceObject)
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
        }

        ExAcquireResourceExclusiveLite(
            &FsdGlobalData.Resource,
            TRUE
            );

        GlobalDataResourceAcquired = TRUE;

        if (FlagOn(FsdGlobalData.Flags, FSD_UNLOAD_PENDING))
        {
            KdPrint((
                DRIVER_NAME ": %-16.16s %-31s *** Already ready to unload ***\n",
                FsdGetCurrentProcessName(),
                "FsdPrepareToUnload"
                ));

            Status = STATUS_ACCESS_DENIED;

            __leave;
        }

        if (!IsListEmpty(&FsdGlobalData.VcbList))
        {
            KdPrint((
                DRIVER_NAME ": %-16.16s %-31s *** Mounted volumes exists ***\n",
                FsdGetCurrentProcessName(),
                "FsdPrepareToUnload"
                ));

            Status = STATUS_ACCESS_DENIED;

            __leave;
        }

        IoUnregisterFileSystem(FsdGlobalData.DeviceObject);

#if (VER_PRODUCTBUILD >= 2600)
        IoDeleteDevice(FsdGlobalData.DeviceObject);
#endif

#pragma prefast( suppress: 28175, "allowed to unload" )
        FsdGlobalData.DriverObject->DriverUnload = DriverUnload;

        SetFlag(FsdGlobalData.Flags, FSD_UNLOAD_PENDING);

        KdPrint((
            DRIVER_NAME ": %-16.16s %-31s Driver is ready to unload\n",
            FsdGetCurrentProcessName(),
            "FsdPrepareToUnload"
            ));

        Status = STATUS_SUCCESS;
    }
    __finally
    {
        if (GlobalDataResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &FsdGlobalData.Resource,
                ExGetCurrentResourceThread()
                );
        }

        if (!AbnormalTermination())
        {
            IrpContext->Irp->IoStatus.Status = Status;

            FsdCompleteRequest(
                IrpContext->Irp,
                (CCHAR)
                (NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT)
                );

            FsdFreeIrpContext(IrpContext);
        }
    }

    return Status;
}

#endif // DBG

__drv_mustHoldCriticalRegion
NTSTATUS
FsdDeviceControlNormal (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    PDEVICE_OBJECT  DeviceObject;
    BOOLEAN         CompleteRequest;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PFSD_VCB        Vcb;
    PFILE_OBJECT    FileObject;
    PFSD_FCB        Fcb;
    PIRP            Irp;
    PDEVICE_OBJECT  TargetDeviceObject;

    PAGED_CODE();

    __try
    {
        ASSERT(IrpContext != NULL);

        ASSERT((IrpContext->Identifier.Type == ICX) &&
               (IrpContext->Identifier.Size == sizeof(FSD_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;

        if (DeviceObject == FsdGlobalData.DeviceObject)
        {
            CompleteRequest = TRUE;
            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
        }

        Vcb = (PFSD_VCB) DeviceObject->DeviceExtension;

        ASSERT(Vcb != NULL);

        ASSERT((Vcb->Identifier.Type == VCB) &&
               (Vcb->Identifier.Size == sizeof(FSD_VCB)));

        FileObject = IrpContext->FileObject;

        Fcb = (PFSD_FCB) FileObject->FsContext;

        ASSERT(Fcb != NULL);

        if (Fcb->Identifier.Type != VCB)
        {
            CompleteRequest = TRUE;
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        Irp = IrpContext->Irp;

        TargetDeviceObject = Vcb->TargetDeviceObject;

        //
        // Pass on the IOCTL to the driver below
        //

        CompleteRequest = FALSE;

#if DBG
        IoCopyCurrentIrpStackLocationToNext(Irp);

        IoSetCompletionRoutine(
            Irp,
            FsdDeviceControlCompletion,
            DeviceObject,
            FALSE,
            TRUE,
            TRUE
            );
#else
        IoSkipCurrentIrpStackLocation(Irp);
#endif

        Status = IoCallDriver(TargetDeviceObject, Irp);
    }
    __finally
    {
        if (!AbnormalTermination())
        {
            if (CompleteRequest)
            {
                IrpContext->Irp->IoStatus.Status = Status;

                FsdCompleteRequest(
                    IrpContext->Irp,
                    (CCHAR)
                    (NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT)
                    );
            }

            FsdFreeIrpContext(IrpContext);
        }
    }

    return Status;
}

#pragma code_seg() // end FSD_PAGED_CODE

#if DBG

NTSTATUS
FsdDeviceControlCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    if (Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }

    KdPrint((
        DRIVER_NAME ": %-16.16s %-31s *** Status: %s (%#x) ***\n",
        FsdGetCurrentProcessName(),
        "DEVICE_CONTROL",
        FsdNtStatusToString(Irp->IoStatus.Status),
        Irp->IoStatus.Status
        ));

    return STATUS_SUCCESS;
}

#endif // DBG
