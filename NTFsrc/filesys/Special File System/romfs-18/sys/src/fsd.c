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
#include "rom_fs.h"
#include "border.h"

NTSTATUS
FsdBuildRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    BOOLEAN             AtIrqlPassiveLevel = FALSE;
    BOOLEAN             IsTopLevelIrp = FALSE;
    PFSD_IRP_CONTEXT    IrpContext = NULL;
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    __try
    {
        __try
        {
            FsdDbgPrintCall(DeviceObject, Irp);

            AtIrqlPassiveLevel = (KeGetCurrentIrql() == PASSIVE_LEVEL);

            if (AtIrqlPassiveLevel)
            {
                FsRtlEnterFileSystem();
            }

            if (!IoGetTopLevelIrp())
            {
                IsTopLevelIrp = TRUE;
                IoSetTopLevelIrp(Irp);
            }

            IrpContext = FsdAllocateIrpContext(DeviceObject, Irp);

            if (!IrpContext)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Status = Status;
                FsdCompleteRequest(Irp, IO_NO_INCREMENT);
            }
            else
            {
                Status = FsdDispatchRequest(IrpContext);
            }
        }
        __except (FsdExceptionFilter(IrpContext, GetExceptionCode()))
        {
            Status = FsdExceptionHandler(IrpContext);
        }
    }
    __finally
    {
        if (IsTopLevelIrp)
        {
            IoSetTopLevelIrp(NULL);
        }

        if (AtIrqlPassiveLevel)
        {
            FsRtlExitFileSystem();
        }
    }
    
    return Status;
}

NTSTATUS
FsdQueueRequest (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == ICX) &&
           (IrpContext->Identifier.Size == sizeof(FSD_IRP_CONTEXT)));

    // IsSynchronous means we can block (so we don't requeue it)
    IrpContext->IsSynchronous = TRUE;

    IoMarkIrpPending(IrpContext->Irp);

#if (VER_PRODUCTBUILD >= 2195)

    IrpContext->WorkQueueItem = IoAllocateWorkItem(IrpContext->DeviceObject);

    IoQueueWorkItem(
        IrpContext->WorkQueueItem,
        FsdDeQueueRequest,
        CriticalWorkQueue,
        IrpContext
        );

#else

    ExInitializeWorkItem(
        &IrpContext->WorkQueueItem,
        FsdDeQueueRequest,
        IrpContext
        );

    ExQueueWorkItem(&IrpContext->WorkQueueItem, CriticalWorkQueue);

#endif

    return STATUS_PENDING;
}

VOID
FsdDeQueueRequest (
#if (VER_PRODUCTBUILD >= 2195)
    IN PDEVICE_OBJECT   DeviceObject,
#endif
    IN PVOID            Context
    )
{
    PFSD_IRP_CONTEXT IrpContext;

    IrpContext = (PFSD_IRP_CONTEXT) Context;

    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == ICX) &&
           (IrpContext->Identifier.Size == sizeof(FSD_IRP_CONTEXT)));

#if (VER_PRODUCTBUILD >= 2195)
    IoFreeWorkItem(IrpContext->WorkQueueItem);
#endif

    __try
    {
        __try
        {
            FsRtlEnterFileSystem();

            if (!IrpContext->IsTopLevel)
            {
                IoSetTopLevelIrp((PIRP) FSRTL_FSP_TOP_LEVEL_IRP);
            }

            FsdDispatchRequest(IrpContext);
        }
        __except (FsdExceptionFilter(IrpContext, GetExceptionCode()))
        {
            FsdExceptionHandler(IrpContext);
        }
    }
    __finally
    {
        IoSetTopLevelIrp(NULL);

        FsRtlExitFileSystem();
    }
}

__drv_mustHoldCriticalRegion
NTSTATUS
FsdDispatchRequest (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == ICX) &&
           (IrpContext->Identifier.Size == sizeof(FSD_IRP_CONTEXT)));

    switch (IrpContext->MajorFunction)
    {
    case IRP_MJ_CREATE:
        return FsdCreate(IrpContext);

    case IRP_MJ_CLOSE:
        return FsdClose(IrpContext);

    case IRP_MJ_READ:
        return FsdRead(IrpContext);

    case IRP_MJ_QUERY_INFORMATION:
        return FsdQueryInformation(IrpContext);

    case IRP_MJ_SET_INFORMATION:
        return FsdSetInformation(IrpContext);

    case IRP_MJ_QUERY_VOLUME_INFORMATION:
        return FsdQueryVolumeInformation(IrpContext);

    case IRP_MJ_DIRECTORY_CONTROL:
        return FsdDirectoryControl(IrpContext);

    case IRP_MJ_FILE_SYSTEM_CONTROL:
        return FsdFileSystemControl(IrpContext);

    case IRP_MJ_DEVICE_CONTROL:
        return FsdDeviceControl(IrpContext);

    case IRP_MJ_LOCK_CONTROL:
        return FsdLockControl(IrpContext);

    case IRP_MJ_CLEANUP:
        return FsdCleanup(IrpContext);

    default:
        KdPrint((
            DRIVER_NAME ": FsdDispatchRequest: Unexpected major function: %#x\n",
            IrpContext->MajorFunction
            ));
        IrpContext->Irp->IoStatus.Status = STATUS_DRIVER_INTERNAL_ERROR;
        FsdCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
        FsdFreeIrpContext(IrpContext);
        return STATUS_DRIVER_INTERNAL_ERROR;
    }
}

NTSTATUS
FsdExceptionFilter (
    IN PFSD_IRP_CONTEXT     IrpContext,
    IN NTSTATUS             ExceptionCode
    )
{
    NTSTATUS Status;

    //
    // Only use a valid IrpContext
    //
    if (IrpContext)
    {
        if ((IrpContext->Identifier.Type != ICX) ||
            (IrpContext->Identifier.Size != sizeof(FSD_IRP_CONTEXT)))
        {
            IrpContext = NULL;
        }
    }

    //
    // If the exception is expected execute our handler
    //
    if (FsRtlIsNtstatusExpected(ExceptionCode))
    {
        KdPrint((
            DRIVER_NAME ": FsdExceptionFilter: Catching exception %#x\n",
            ExceptionCode
            ));

        Status = EXCEPTION_EXECUTE_HANDLER;

        if (IrpContext)
        {
            IrpContext->ExceptionCode = ExceptionCode;
        }
    }
    //
    // else continue search for an higher level exception handler
    //
    else
    {
        KdPrint((
            DRIVER_NAME ": FsdExceptionFilter: Passing on exception %#x\n",
            ExceptionCode
            ));

        Status = EXCEPTION_CONTINUE_SEARCH;

        if (IrpContext)
        {
            FsdFreeIrpContext(IrpContext);
        }
    }

    return Status;
}

NTSTATUS
FsdExceptionHandler (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    NTSTATUS Status;

    if (IrpContext)
    {
        Status = IrpContext->ExceptionCode;

        if (IrpContext->Irp)
        {
            IrpContext->Irp->IoStatus.Status = Status;

            FsdCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
        }

        FsdFreeIrpContext(IrpContext);
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
FsdLockUserBuffer (
    IN PIRP             Irp,
    IN ULONG            Length,
    IN LOCK_OPERATION   Operation
    )
{
    NTSTATUS Status;

    ASSERT(Irp != NULL);

    if (Irp->MdlAddress != NULL)
    {
        return STATUS_SUCCESS;
    }

    IoAllocateMdl(Irp->UserBuffer, Length, FALSE, FALSE, Irp);

    if (Irp->MdlAddress == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    __try
    {
        MmProbeAndLockPages(Irp->MdlAddress, Irp->RequestorMode, Operation);

        Status = STATUS_SUCCESS;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        IoFreeMdl(Irp->MdlAddress);

        Irp->MdlAddress = NULL;

        Status = STATUS_INVALID_USER_BUFFER;
    }

    return Status;
}

PVOID
FsdGetUserBuffer (
    IN PIRP Irp
    )
{
    ASSERT(Irp != NULL);

    if (Irp->MdlAddress)
    {
#if (VER_PRODUCTBUILD >= 2195)
        return MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
#else
        return MmGetSystemAddressForMdl(Irp->MdlAddress);
#endif
    }
    else
    {
        return Irp->UserBuffer;
    }
}

#pragma code_seg(FSD_PAGED_CODE)

NTSTATUS
FsdReadInodeByIndex (
    IN PDEVICE_OBJECT           DeviceObject,
    IN ULONG                    Index,
    IN OUT struct romfs_inode*  Inode
    )
{
    PUCHAR          Buffer;
    LARGE_INTEGER   Offset;
    NTSTATUS        Status;
    PDEVICE_OBJECT  DeviceToVerify;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);
    ASSERT(Inode != NULL);

    Buffer = (PUCHAR) FsdAllocatePool(NonPagedPool, 1024, '2mTR');

    if (Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Byte offset to the inode
    Index = be32_to_cpu(Index) & ROMFH_MASK;

    //KdPrint((DRIVER_NAME ": FsdReadInodeByIndex: Index: %#x\n", Index));

    // Sector align the offset
    Offset.QuadPart = Index & ~(SECTOR_SIZE - 1);

    Status = FsdReadBlockDevice(
        DeviceObject,
        &Offset,
        1024,
        Buffer
        );

    if (Status == STATUS_VERIFY_REQUIRED)
    {
        DeviceToVerify = IoGetDeviceToVerify(PsGetCurrentThread());

        IoSetDeviceToVerify(PsGetCurrentThread(), NULL);

        Status = IoVerifyVolume(DeviceToVerify, FALSE);

        if (NT_SUCCESS(Status))
        {
            Status = FsdReadBlockDevice(
                DeviceObject,
                &Offset,
                1024,
                Buffer
                );
        }
    }

    RtlCopyMemory(
        Inode,
        Buffer + (Index & (SECTOR_SIZE - 1)),
        sizeof(struct romfs_inode) + ROMFS_MAXFN
        );

    FsdFreePool(Buffer);

    return Status;
}

#pragma code_seg() // end FSD_PAGED_CODE
