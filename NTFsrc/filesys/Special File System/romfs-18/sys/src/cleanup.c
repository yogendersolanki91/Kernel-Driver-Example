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
FsdCleanup (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_SUCCESS;
    PFSD_VCB        Vcb;
    BOOLEAN         VcbResourceAcquired = FALSE;
    PFILE_OBJECT    FileObject;
    PFSD_FCB        Fcb;
    BOOLEAN         VolumeUnlocked = FALSE;
    BOOLEAN         FcbResourceAcquired = FALSE;
    PFSD_CCB        Ccb;
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
            if (FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED))
            {
                ClearFlag(Vcb->Flags, VCB_VOLUME_LOCKED);

                FsdClearVpbFlag(Vcb->Vpb, VPB_LOCKED);

                VolumeUnlocked = TRUE;
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

        Irp = IrpContext->Irp;

        Fcb->OpenHandleCount--;

        Vcb->OpenFileHandleCount--;

        CcUninitializeCacheMap(FileObject, NULL, NULL);

        if (FlagOn(Fcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
        {
            FsRtlNotifyCleanup(
                Vcb->NotifySync,
                &Vcb->NotifyList,
                FileObject->FsContext2
                );
        }
        else
        {
            //
            // Drop any byte range locks this process may have on the file.
            //
            FsRtlFastUnlockAll(
                &Fcb->FileLock,
                FileObject,
                IoGetRequestorProcess(Irp),
                NULL
                );

            //
            // If there are no byte range locks owned by other processes on the
            // file the fast I/O read/write functions doesn't have to check for
            // locks so we set IsFastIoPossible to FastIoIsPossible again.
            //
            if (!FsRtlGetNextFileLock(&Fcb->FileLock, TRUE))
            {
                if (Fcb->CommonFCBHeader.IsFastIoPossible != FastIoIsPossible)
                {
                    KdPrint((
                        DRIVER_NAME ": %-16.16s %-31s %s\n",
                        FsdGetCurrentProcessName(),
                        "FastIoIsPossible",
                        Fcb->AnsiFileName.Buffer
                        ));

                    Fcb->CommonFCBHeader.IsFastIoPossible = FastIoIsPossible;
                }
            }
        }

        IoRemoveShareAccess(FileObject, &Fcb->ShareAccess);

        KdPrint((
            DRIVER_NAME ": OpenHandleCount: %-7u ReferenceCount: %-7u %s\n",
            Fcb->OpenHandleCount,
            Fcb->ReferenceCount,
            Fcb->AnsiFileName.Buffer
            ));

        Status = STATUS_SUCCESS;
    }
    __finally
    {
        if (IrpContext->FileObject)
        {
            SetFlag(IrpContext->FileObject->Flags, FO_CLEANUP_COMPLETE);
        }

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

        if (IrpContext->FileObject && VolumeUnlocked)
        {
#if (VER_PRODUCTBUILD >= 2195)
            FsRtlNotifyVolumeEvent(IrpContext->FileObject, FSRTL_VOLUME_UNLOCK);
#endif
        }

        if (!AbnormalTermination())
        {
            if (Status == STATUS_PENDING)
            {
                FsdQueueRequest(IrpContext);
            }
            else
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
    }

    return Status;
}

#pragma code_seg() // end FSD_PAGED_CODE
