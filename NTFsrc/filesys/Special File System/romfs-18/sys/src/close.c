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

#pragma code_seg(FSD_PAGED_CODE)

__drv_mustHoldCriticalRegion
NTSTATUS
FsdClose (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_SUCCESS;
    PFSD_VCB        Vcb;
    BOOLEAN         VcbResourceAcquired = FALSE;
    PFILE_OBJECT    FileObject;
    PFSD_FCB        Fcb;
    BOOLEAN         FcbResourceAcquired = FALSE;
    PFSD_CCB        Ccb;
    BOOLEAN         FreeVcb = FALSE;

    PAGED_CODE();

    __try
    {
        ASSERT(IrpContext != NULL);

        ASSERT((IrpContext->Identifier.Type == ICX) &&
               (IrpContext->Identifier.Size == sizeof(FSD_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;

        if (DeviceObject == FsdGlobalData.DeviceObject)
        {
            Status = STATUS_SUCCESS;
            __leave;
        }

        Vcb = (PFSD_VCB) DeviceObject->DeviceExtension;

        ASSERT(Vcb != NULL);

        ASSERT((Vcb->Identifier.Type == VCB) &&
               (Vcb->Identifier.Size == sizeof(FSD_VCB)));

#pragma prefast( suppress: 28137, "by design" )
        if (!ExAcquireResourceExclusiveLite(
                 &Vcb->MainResource,
                 IrpContext->IsSynchronous
                 ))
        {
            Status = STATUS_PENDING;
            __leave;
        }

        VcbResourceAcquired = TRUE;

        FileObject = IrpContext->FileObject;

        Fcb = (PFSD_FCB) FileObject->FsContext;

        ASSERT(Fcb != NULL);

        if (Fcb->Identifier.Type == VCB)
        {
            Vcb->ReferenceCount--;

            if (!Vcb->ReferenceCount && FlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING))
            {
                FreeVcb = TRUE;
            }

            Status = STATUS_SUCCESS;

            __leave;
        }

        ASSERT((Fcb->Identifier.Type == FCB) &&
               (Fcb->Identifier.Size == sizeof(FSD_FCB)));

#ifndef FSD_RO
        if (!FlagOn(Fcb->Flags, FCB_PAGE_FILE))
#endif
        {
#pragma prefast( suppress: 28137, "by design" )
            if (!ExAcquireResourceExclusiveLite(
                     &Fcb->MainResource,
                     IrpContext->IsSynchronous
                     ))
            {
                Status = STATUS_PENDING;
                __leave;
            }

            FcbResourceAcquired = TRUE;
        }

        Ccb = (PFSD_CCB) FileObject->FsContext2;

        ASSERT(Ccb != NULL);

        ASSERT((Ccb->Identifier.Type == CCB) &&
               (Ccb->Identifier.Size == sizeof(FSD_CCB)));

        Fcb->ReferenceCount--;

        Vcb->ReferenceCount--;

        if (!Vcb->ReferenceCount && FlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING))
        {
            FreeVcb = TRUE;
        }

        KdPrint((
            DRIVER_NAME ": OpenHandleCount: %-7u ReferenceCount: %-7u %s\n",
            Fcb->OpenHandleCount,
            Fcb->ReferenceCount,
            Fcb->AnsiFileName.Buffer
            ));

        FsdFreeCcb(Ccb);

        if (!Fcb->ReferenceCount)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread()
                );

            FcbResourceAcquired = FALSE;

            FsdFreeFcb(Fcb);
        }

        Status = STATUS_SUCCESS;
    }
    __finally
    {
        if (FcbResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }

        if (VcbResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }

        if (!AbnormalTermination())
        {
            if (Status == STATUS_PENDING)
            {
                //
                // Close should never return STATUS_PENDING
                //

                Status = STATUS_SUCCESS;

                if (IrpContext->Irp != NULL)
                {
                    IrpContext->Irp->IoStatus.Status = Status;

                    FsdCompleteRequest(
                        IrpContext->Irp,
                        (CCHAR)
                        (NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT)
                        );

                    IrpContext->Irp = NULL;
                }

                FsdQueueCloseRequest(IrpContext);
            }
            else
            {
                if (IrpContext->Irp != NULL)
                {
                    IrpContext->Irp->IoStatus.Status = Status;

                    FsdCompleteRequest(
                        IrpContext->Irp,
                        (CCHAR)
                        (NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT)
                        );
                }

                FsdFreeIrpContext(IrpContext);

                if (FreeVcb)
                {
                    FsdFreeVcb(Vcb);
                }
            }
        }
    }

    return Status;
}

VOID
FsdQueueCloseRequest (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    PAGED_CODE();

    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == ICX) &&
           (IrpContext->Identifier.Size == sizeof(FSD_IRP_CONTEXT)));

    // IsSynchronous means we can block (so we don't requeue it)
    IrpContext->IsSynchronous = TRUE;

#if (VER_PRODUCTBUILD >= 2195)

    IrpContext->WorkQueueItem = IoAllocateWorkItem(IrpContext->DeviceObject);

    IoQueueWorkItem(
        IrpContext->WorkQueueItem,
        FsdDeQueueCloseRequest,
        CriticalWorkQueue,
        IrpContext
        );

#else

    ExInitializeWorkItem(
        &IrpContext->WorkQueueItem,
        FsdDeQueueCloseRequest,
        IrpContext
        );

    ExQueueWorkItem(&IrpContext->WorkQueueItem, CriticalWorkQueue);

#endif
}

VOID
FsdDeQueueCloseRequest (
#if (VER_PRODUCTBUILD >= 2195)
    IN PDEVICE_OBJECT   DeviceObject,
#endif
    IN PVOID            Context
    )
{
    PFSD_IRP_CONTEXT IrpContext;

    PAGED_CODE();

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

            FsdClose(IrpContext);
        }
        __except (FsdExceptionFilter(IrpContext, GetExceptionCode()))
        {
            FsdExceptionHandler(IrpContext);
        }
    }
    __finally
    {
        FsRtlExitFileSystem();
    }
}

#pragma code_seg() // end FSD_PAGED_CODE
