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

#pragma code_seg(FSD_PAGED_CODE)

__drv_mustHoldCriticalRegion
NTSTATUS
FsdCreate (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    PDEVICE_OBJECT      DeviceObject;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;

    PAGED_CODE();

    DeviceObject = IrpContext->DeviceObject;

    Irp = IrpContext->Irp;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    if (DeviceObject == FsdGlobalData.DeviceObject)
    {
        return FsdCreateFs(IrpContext);
    }
    else if (IrpSp->FileObject->FileName.Length == 0)
    {
        return FsdCreateVolume(IrpContext);
    }
    else
    {
        return FsdCreateFile(IrpContext);
    }
}

NTSTATUS
FsdCreateFs (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    PAGED_CODE();

    IrpContext->Irp->IoStatus.Status = STATUS_SUCCESS;

    IrpContext->Irp->IoStatus.Information = FILE_OPENED;
    
    FsdCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
    
    FsdFreeIrpContext(IrpContext);
    
    return STATUS_SUCCESS;
}

__drv_mustHoldCriticalRegion
NTSTATUS
FsdCreateVolume (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    PDEVICE_OBJECT      DeviceObject;
    PFSD_VCB            Vcb;
    PFILE_OBJECT        FileObject;

    PAGED_CODE();

    DeviceObject = IrpContext->DeviceObject;

    Vcb = (PFSD_VCB) DeviceObject->DeviceExtension;

    ASSERT(Vcb != NULL);

    ASSERT((Vcb->Identifier.Type == VCB) &&
           (Vcb->Identifier.Size == sizeof(FSD_VCB)));

    FileObject = IrpContext->FileObject;

    FileObject->FsContext = Vcb;

    ExAcquireResourceExclusiveLite(
        &Vcb->MainResource,
        TRUE
        );

    Vcb->ReferenceCount++;

    ExReleaseResourceForThreadLite(
        &Vcb->MainResource,
        ExGetCurrentResourceThread()
        );

    IrpContext->Irp->IoStatus.Status = STATUS_SUCCESS;

    IrpContext->Irp->IoStatus.Information = FILE_OPENED;
    
    FsdCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
    
    FsdFreeIrpContext(IrpContext);
    
    return STATUS_SUCCESS;
}

__drv_mustHoldCriticalRegion
NTSTATUS
FsdCreateFile (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    PDEVICE_OBJECT      DeviceObject;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;
    PFSD_VCB            Vcb = NULL;
    PFSD_FCB            Fcb;
    PFSD_CCB            Ccb;
    ULONG               found_index = 0;
    struct romfs_inode* Inode = NULL;
    BOOLEAN             VcbResourceAcquired = FALSE;

    PAGED_CODE();
    
    DeviceObject = IrpContext->DeviceObject;
    
    Vcb = (PFSD_VCB) DeviceObject->DeviceExtension;
    
    Irp = IrpContext->Irp;
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    __try
    {
        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            TRUE
            );
        
        VcbResourceAcquired = TRUE;
        
        Fcb = FsdLookupFcbByFileName(
            Vcb,
            &IrpSp->FileObject->FileName
            );
        
        if (!Fcb)
        {
            Inode = FsdAllocatePool(
                NonPagedPool,
                sizeof(struct romfs_inode) + ROMFS_MAXFN,
                '3cFR'
                );
            
            if (Inode == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }
            
            Status = FsdLookupFileName(
                Vcb,
                &IrpSp->FileObject->FileName,
                &found_index,
                Inode
                );
            
            if (!NT_SUCCESS(Status))
            {
                KdPrint((
                    DRIVER_NAME ": STATUS_OBJECT_NAME_NOT_FOUND: %.*S\n",
                    IrpSp->FileObject->FileName.Length / 2,
                    IrpSp->FileObject->FileName.Buffer
                    ));

                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                __leave;
            }

            Fcb = FsdAllocateFcb(
                Vcb,
                &IrpSp->FileObject->FileName,
                found_index,
                Inode
                );
            
            if (Fcb == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }
            
            KdPrint((
                DRIVER_NAME ": Allocated a new FCB for %s\n",
                Fcb->AnsiFileName.Buffer
                ));
        }

        if (Fcb->OpenHandleCount >= 1)
        {
            Status = IoCheckShareAccess(
                IrpSp->Parameters.Create.SecurityContext->DesiredAccess,
                IrpSp->Parameters.Create.ShareAccess,
                IrpSp->FileObject,
                &Fcb->ShareAccess,
                FALSE
                );

            if (!NT_SUCCESS(Status))
            {
                __leave;
            }
        }

        Ccb = FsdAllocateCcb();

        if (Ccb == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        Fcb->OpenHandleCount++;
        Vcb->OpenFileHandleCount++;
        Fcb->ReferenceCount++;
        Vcb->ReferenceCount++;

        if (!FlagOn(be32_to_cpu(Fcb->romfs_inode->next), ROMFH_DIR))
        {
            Fcb->CommonFCBHeader.IsFastIoPossible = FastIoIsPossible;
        }
            
        IrpSp->FileObject->FsContext = (void*) Fcb;
        IrpSp->FileObject->FsContext2 = (void*) Ccb;
        IrpSp->FileObject->PrivateCacheMap = NULL;
        IrpSp->FileObject->SectionObjectPointer = &(Fcb->SectionObject);
        IrpSp->FileObject->Vpb = Vcb->Vpb;

        if (Fcb->OpenHandleCount == 1)
        {
            IoSetShareAccess(
                IrpSp->Parameters.Create.SecurityContext->DesiredAccess,
                IrpSp->Parameters.Create.ShareAccess,
                IrpSp->FileObject,
                &Fcb->ShareAccess
                );
        }
        else
        {
            IoUpdateShareAccess(IrpSp->FileObject, &Fcb->ShareAccess);
        }

        Irp->IoStatus.Information = FILE_OPENED;
        Status = STATUS_SUCCESS;
            
        KdPrint((
            DRIVER_NAME ": %s OpenHandleCount: %u ReferenceCount: %u\n",
            Fcb->AnsiFileName.Buffer,
            Fcb->OpenHandleCount,
            Fcb->ReferenceCount
            ));
    }
    __finally
    {
        if (VcbResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }

        if (!NT_SUCCESS(Status) && Inode)
        {
            FsdFreePool(Inode);
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
            
            if (Vcb &&
                FlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING) &&
                !Vcb->ReferenceCount
                )
            {
                FsdFreeVcb(Vcb);
            }
        }
    }
    
    return Status;
}

NTSTATUS
FsdLookupFileName (
    IN PFSD_VCB                 Vcb,
    IN PUNICODE_STRING          FullFileName,
    IN OUT PULONG               FoundIndex,
    IN OUT struct romfs_inode*  FilledInInode
    )
{
    UNICODE_STRING  FileName;
    ULONG           CurrentDirIndex;
    USHORT          PartFileNameLength, FullFileNameLength;
    NTSTATUS        Status;

    PAGED_CODE();

    if (FullFileName->Length == 0)
    {
        *FoundIndex = 0;
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if (FullFileName->Length == sizeof(WCHAR) && FullFileName->Buffer[0] == L'\\')
    {
        RtlCopyMemory(FilledInInode, Vcb->root_inode, sizeof(struct romfs_inode) + ROMFS_MAXFN);
        *FoundIndex = Vcb->root_inode_number;
        return STATUS_SUCCESS;
    }

    FileName = *FullFileName;
    CurrentDirIndex = Vcb->root_inode_number;

    KdPrint(("Romfs: Lookup: %.*S\n", FileName.Length / sizeof(WCHAR), FileName.Buffer));

    do {
        FileName.Buffer++;
        FileName.Length -= sizeof(WCHAR);
        FullFileNameLength = FileName.Length;
        for(PartFileNameLength = 0; PartFileNameLength * sizeof(WCHAR) < FileName.Length && FileName.Buffer[PartFileNameLength] != L'\\'; PartFileNameLength++) {}
        FileName.Length = PartFileNameLength * sizeof(WCHAR);
        KdPrint(("Romfs:   part: %.*S\n", FileName.Length / sizeof(WCHAR), FileName.Buffer));
        Status = FsdLookupFileNameInDir(
            Vcb,
            CurrentDirIndex,
            &FileName,
            FoundIndex,
            FilledInInode
        );
        FileName.Length = FullFileNameLength;
        while(FileName.Length > 0 && FileName.Buffer[0] != L'\\') { FileName.Length -= sizeof(WCHAR); FileName.Buffer++; }
        CurrentDirIndex = FilledInInode->spec;
    } while(Status == STATUS_SUCCESS && FileName.Length > 0 && !(FileName.Length == sizeof(WCHAR) && FileName.Buffer[0] == L'\\') );

    return Status;
}

NTSTATUS
FsdLookupFileNameInDir (
    IN PFSD_VCB                 Vcb,
    IN ULONG                    DirIndex,
    IN PUNICODE_STRING          FileName,
    IN OUT PULONG               Index,
    IN OUT struct romfs_inode*  Inode
    )
{
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    ULONG           FileIndex;
    USHORT          InodeFileNameLength;
    UNICODE_STRING  InodeFileName;
    UNICODE_STRING  slash_string;

    PAGED_CODE();

    InodeFileName.Buffer = NULL;

    __try
    {
        FileIndex = DirIndex;

        if (!(be32_to_cpu(FileIndex) & ROMFH_MASK))
        {
            Status = STATUS_NO_SUCH_FILE;
            __leave;
        }

        while ((be32_to_cpu(FileIndex) & ROMFH_MASK))
        {
            Status = FsdReadInodeByIndex(
                Vcb->TargetDeviceObject,
                FileIndex,
                Inode
                );

            if (!NT_SUCCESS(Status))
            {
                Status = STATUS_NO_SUCH_FILE;
                __leave;
            }

            InodeFileNameLength = (USHORT) strnlen(Inode->name, ROMFS_MAXFN);

            InodeFileName.Length =
            InodeFileName.MaximumLength =
                InodeFileNameLength * 2;

            InodeFileName.Buffer = FsdAllocatePool(
                NonPagedPool,
                InodeFileName.MaximumLength,
                '1rCR'
                );

            FsdCharToWchar(
                InodeFileName.Buffer,
                Inode->name,
                InodeFileNameLength
                );

            if (!RtlCompareUnicodeString(
                FileName,
                &InodeFileName,
                TRUE
                ))
            {
                *Index = FileIndex;
                Status = STATUS_SUCCESS;
                __leave;
            }

            if (InodeFileName.Buffer != NULL)
            {
                FsdFreePool(InodeFileName.Buffer);
                InodeFileName.Buffer = NULL;
            }

            FileIndex = Inode->next;
        }

        Status = STATUS_NO_SUCH_FILE;
    }
    __finally
    {
        if (InodeFileName.Buffer != NULL)
        {
            FsdFreePool(InodeFileName.Buffer);
        }
    }

    return Status;
}

PFSD_FCB
FsdLookupFcbByFileName (
    IN PFSD_VCB         Vcb,
    IN PUNICODE_STRING  FullFileName
    )
{
    PLIST_ENTRY ListEntry;
    PFSD_FCB    Fcb;

    PAGED_CODE();

    ListEntry = Vcb->FcbList.Flink;

    while (ListEntry != &Vcb->FcbList)
    {
        Fcb = CONTAINING_RECORD(ListEntry, FSD_FCB, Next);

        if (!RtlCompareUnicodeString(
            &Fcb->FileName,
            FullFileName,
            TRUE
            ))
        {
            KdPrint((
                DRIVER_NAME ": Found an allocated FCB for %s\n",
                Fcb->AnsiFileName.Buffer
                ));

            return Fcb;
        }

        ListEntry = ListEntry->Flink;
    }

    return NULL;
}

#pragma code_seg() // end FSD_PAGED_CODE
