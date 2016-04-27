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

#if DBG

PVOID
FsdAllocatePool (
    IN POOL_TYPE    PoolType,
    IN ULONG        NumberOfBytes,
    IN ULONG        Tag
    )
{
    PVOID               p;
    PFSD_ALLOC_HEADER   AllocHeader;

    NumberOfBytes += sizeof(FSD_ALLOC_HEADER);

    p = ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);

    if (p)
    {
        RtlFillMemory(p, NumberOfBytes, '0');

        AllocHeader = (PFSD_ALLOC_HEADER) p;

        AllocHeader->Identifier.Type = FSD;
        AllocHeader->Identifier.Size = NumberOfBytes;

        return (PVOID)((PUCHAR)p + sizeof(FSD_ALLOC_HEADER));
    }
    else
    {
        return NULL;
    }
}

VOID
FsdFreePool (
    IN PVOID p
    )
{
    PFSD_ALLOC_HEADER AllocHeader;

    ASSERT(p != NULL);

    p = (PVOID)((PUCHAR)p - sizeof(FSD_ALLOC_HEADER));

    AllocHeader = (PFSD_ALLOC_HEADER) p;

    ASSERT(AllocHeader->Identifier.Type == FSD);

    RtlFillMemory(p, AllocHeader->Identifier.Size, 'X');

    ExFreePool(p);
}

#endif // DBG

PFSD_IRP_CONTEXT
FsdAllocateIrpContext (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PIO_STACK_LOCATION  IrpSp;
    PFSD_IRP_CONTEXT    IrpContext;

    ASSERT(DeviceObject != NULL);
    ASSERT(Irp != NULL);

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    IrpContext = FsdAllocatePool(NonPagedPool, sizeof(FSD_IRP_CONTEXT), 'xcIR');

    if (!IrpContext)
    {
        return NULL;
    }

    IrpContext->Identifier.Type = ICX;
    IrpContext->Identifier.Size = sizeof(FSD_IRP_CONTEXT);

    IrpContext->Irp = Irp;

    IrpContext->MajorFunction = IrpSp->MajorFunction;
    IrpContext->MinorFunction = IrpSp->MinorFunction;

    IrpContext->DeviceObject = DeviceObject;

    IrpContext->FileObject = IrpSp->FileObject;

    if (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL ||
        IrpContext->MajorFunction == IRP_MJ_DEVICE_CONTROL ||
        IrpContext->MajorFunction == IRP_MJ_SHUTDOWN)
    {
        IrpContext->IsSynchronous = TRUE;
    }
    else if (IrpContext->MajorFunction == IRP_MJ_CLEANUP ||
             IrpContext->MajorFunction == IRP_MJ_CLOSE)
    {
        IrpContext->IsSynchronous = FALSE;
    }
    else
    {
        IrpContext->IsSynchronous = IoIsOperationSynchronous(Irp);
    }

    //
    // Temporary workaround for a bug in close that makes it reference a
    // fileobject when it is no longer valid.
    //
    if (IrpContext->MajorFunction == IRP_MJ_CLOSE)
    {
        IrpContext->IsSynchronous = TRUE;
    }

    IrpContext->IsTopLevel = (IoGetTopLevelIrp() == Irp);

    return IrpContext;
}

VOID
FsdFreeIrpContext (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    ASSERT(IrpContext != NULL);

    ASSERT((IrpContext->Identifier.Type == ICX) &&
           (IrpContext->Identifier.Size == sizeof(FSD_IRP_CONTEXT)));

    FsdFreePool(IrpContext);
}

#pragma code_seg(FSD_PAGED_CODE)

PFSD_FCB
FsdAllocateFcb (
    IN PFSD_VCB             Vcb,
    IN PUNICODE_STRING      FileName,
    IN ULONG                IndexNumber,
    IN struct romfs_inode*  romfs_inode
    )
{
    PFSD_FCB Fcb;

    PAGED_CODE();

    Fcb = FsdAllocatePool(NonPagedPool, sizeof(FSD_FCB), 'bcFR');

    if (!Fcb)
    {
        return NULL;
    }

    Fcb->Identifier.Type = FCB;
    Fcb->Identifier.Size = sizeof(FSD_FCB);

    RtlZeroMemory(&Fcb->ShareAccess, sizeof(SHARE_ACCESS));

    FsRtlInitializeFileLock(
        &Fcb->FileLock,
        NULL,
        NULL
        );

    Fcb->OpenHandleCount = 0;
    Fcb->ReferenceCount = 0;

    Fcb->FileName.Length = 0;

    Fcb->FileName.MaximumLength = FileName->Length;

    Fcb->FileName.Buffer = (PWSTR) FsdAllocatePool(
        NonPagedPool,
        Fcb->FileName.MaximumLength,
        '1cFR'
        );

    if (!Fcb->FileName.Buffer)
    {
        FsdFreePool(Fcb);
        return NULL;
    }

    RtlCopyUnicodeString(
        &Fcb->FileName,
        FileName
        );

#if DBG

    Fcb->AnsiFileName.Length = Fcb->FileName.Length / sizeof(WCHAR);

    Fcb->AnsiFileName.MaximumLength = Fcb->AnsiFileName.Length + 1;

    Fcb->AnsiFileName.Buffer = (PUCHAR) FsdAllocatePool(
        NonPagedPool,
        Fcb->AnsiFileName.MaximumLength,
        '2cFR'
        );

    if (!Fcb->AnsiFileName.Buffer)
    {
        FsdFreePool(Fcb->FileName.Buffer);
        FsdFreePool(Fcb);
        return NULL;
    }

    FsdWcharToChar(
        Fcb->AnsiFileName.Buffer,
        Fcb->FileName.Buffer,
        Fcb->FileName.Length / sizeof(WCHAR)
        );

    Fcb->AnsiFileName.Buffer[Fcb->FileName.Length / sizeof(WCHAR)] = 0;

#endif // DBG

    Fcb->FileAttributes = FILE_ATTRIBUTE_NORMAL;

    if (FlagOn(be32_to_cpu(romfs_inode->next), ROMFH_DIR))
    {
        SetFlag(Fcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY);
    }

#ifndef FSD_RO
    if (FlagOn(Vcb->Flags, VCB_READ_ONLY))
#endif
    {
        SetFlag(Fcb->FileAttributes, FILE_ATTRIBUTE_READONLY);
    }

    Fcb->IndexNumber.QuadPart = be32_to_cpu(IndexNumber) & ROMFH_MASK;

    Fcb->Flags = 0;

    Fcb->romfs_inode = romfs_inode;

    RtlZeroMemory(&Fcb->CommonFCBHeader, sizeof(FSRTL_COMMON_FCB_HEADER));

    Fcb->CommonFCBHeader.NodeTypeCode = (USHORT) FCB;
    Fcb->CommonFCBHeader.NodeByteSize = sizeof(FSD_FCB);
    Fcb->CommonFCBHeader.IsFastIoPossible = FastIoIsNotPossible;
    Fcb->CommonFCBHeader.Resource = &(Fcb->MainResource);
    Fcb->CommonFCBHeader.PagingIoResource = &(Fcb->PagingIoResource);
    Fcb->CommonFCBHeader.AllocationSize.QuadPart = be32_to_cpu(romfs_inode->size);
    Fcb->CommonFCBHeader.FileSize.QuadPart = be32_to_cpu(romfs_inode->size);
    Fcb->CommonFCBHeader.ValidDataLength.QuadPart = be32_to_cpu(romfs_inode->size);

    Fcb->SectionObject.DataSectionObject = NULL;
    Fcb->SectionObject.SharedCacheMap = NULL;
    Fcb->SectionObject.ImageSectionObject = NULL;

    ExInitializeResourceLite(&(Fcb->MainResource));
    ExInitializeResourceLite(&(Fcb->PagingIoResource));

    InsertTailList(&Vcb->FcbList, &Fcb->Next);

    return Fcb;
}

VOID
FsdFreeFcb (
    IN PFSD_FCB Fcb
    )
{
    PAGED_CODE();

    ASSERT(Fcb != NULL);

    ASSERT((Fcb->Identifier.Type == FCB) &&
           (Fcb->Identifier.Size == sizeof(FSD_FCB)));

    FsRtlUninitializeFileLock(&Fcb->FileLock);

    ExDeleteResourceLite(&Fcb->MainResource);

    ExDeleteResourceLite(&Fcb->PagingIoResource);

    RemoveEntryList(&Fcb->Next);

    FsdFreePool(Fcb->FileName.Buffer);

#if DBG
    FsdFreePool(Fcb->AnsiFileName.Buffer);
#endif

    FsdFreePool(Fcb->romfs_inode);

    FsdFreePool(Fcb);
}

PFSD_CCB
FsdAllocateCcb (
    VOID
    )
{
    PFSD_CCB Ccb;

    PAGED_CODE();

    Ccb = FsdAllocatePool(NonPagedPool, sizeof(FSD_CCB), 'bcCR');

    if (!Ccb)
    {
        return NULL;
    }

    Ccb->Identifier.Type = CCB;
    Ccb->Identifier.Size = sizeof(FSD_CCB);

    Ccb->CurrentByteOffset = 0;

    Ccb->DirectorySearchPattern.Length = 0;
    Ccb->DirectorySearchPattern.MaximumLength = 0;
    Ccb->DirectorySearchPattern.Buffer = 0;

    return Ccb;
}

VOID
FsdFreeCcb (
    IN PFSD_CCB Ccb
    )
{
    PAGED_CODE();

    ASSERT(Ccb != NULL);

    ASSERT((Ccb->Identifier.Type == CCB) &&
           (Ccb->Identifier.Size == sizeof(FSD_CCB)));

    if (Ccb->DirectorySearchPattern.Buffer != NULL)
    {
        FsdFreePool(Ccb->DirectorySearchPattern.Buffer);
    }

    FsdFreePool(Ccb);
}

__drv_mustHoldCriticalRegion
VOID
FsdFreeVcb (
    IN PFSD_VCB Vcb
    )
{
    PAGED_CODE();

    ASSERT(Vcb != NULL);

    ASSERT((Vcb->Identifier.Type == VCB) &&
           (Vcb->Identifier.Size == sizeof(FSD_VCB)));

    FsdClearVpbFlag(Vcb->Vpb, VPB_MOUNTED);

    FsRtlNotifyUninitializeSync(&Vcb->NotifySync);

    ExAcquireResourceExclusiveLite(
        &FsdGlobalData.Resource,
        TRUE
        );

    RemoveEntryList(&Vcb->Next);

    ExReleaseResourceForThreadLite(
        &FsdGlobalData.Resource,
        ExGetCurrentResourceThread()
        );

    ExDeleteResourceLite(&Vcb->MainResource);

    ExDeleteResourceLite(&Vcb->PagingIoResource);

    FsdFreePool(Vcb->romfs_super_block);

    FsdFreePool(Vcb->root_inode);

    IoDeleteDevice(Vcb->DeviceObject);

    KdPrint((DRIVER_NAME ": Vcb deallocated\n"));
}

#pragma code_seg() // end FSD_PAGED_CODE
