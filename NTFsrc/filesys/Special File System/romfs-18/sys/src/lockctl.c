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
FsdLockControl (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    PDEVICE_OBJECT  DeviceObject;
    BOOLEAN         CompleteRequest;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT    FileObject;
    PFSD_FCB        Fcb;
    PIRP            Irp;

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

        FileObject = IrpContext->FileObject;

        Fcb = (PFSD_FCB) FileObject->FsContext;

        ASSERT(Fcb != NULL);

        if (Fcb->Identifier.Type == VCB)
        {
            CompleteRequest = TRUE;
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        ASSERT((Fcb->Identifier.Type == FCB) &&
               (Fcb->Identifier.Size == sizeof(FSD_FCB)));

        if (FlagOn(Fcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
        {
            CompleteRequest = TRUE;
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        Irp = IrpContext->Irp;

        //
        // While the file has any byte range locks we set IsFastIoPossible to
        // FastIoIsQuestionable so that the FastIoCheckIfPossible function is
        // called to check the locks for any fast I/O read/write operation.
        //
        if (Fcb->CommonFCBHeader.IsFastIoPossible != FastIoIsQuestionable)
        {
            KdPrint((
                DRIVER_NAME ": %-16.16s %-31s %s\n",
                FsdGetCurrentProcessName(),
                "FastIoIsQuestionable",
                Fcb->AnsiFileName.Buffer
                ));

            Fcb->CommonFCBHeader.IsFastIoPossible = FastIoIsQuestionable;
        }

        //
        // FsRtlProcessFileLock acquires FileObject->FsContext->Resource while
        // modifying the file locks and calls IoCompleteRequest when it's done.
        //

        CompleteRequest = FALSE;

        Status = FsRtlProcessFileLock(
            &Fcb->FileLock,
            Irp,
            NULL
            );

        if (Status != STATUS_SUCCESS)
        {
            KdPrint((
                DRIVER_NAME ": %-16.16s %-31s *** Status: %s (%#x) ***\n",
                FsdGetCurrentProcessName(),
                "IRP_MJ_LOCK_CONTROL",
                FsdNtStatusToString(Status),
                Status
                ));
        }
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
