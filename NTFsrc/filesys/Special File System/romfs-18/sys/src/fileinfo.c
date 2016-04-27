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

__drv_mustHoldCriticalRegion
NTSTATUS
FsdQueryInformation (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT            FileObject;
    PFSD_FCB                Fcb;
    PFSD_CCB                Ccb;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;
    FILE_INFORMATION_CLASS  FileInformationClass;
    ULONG                   Length;
    PVOID                   SystemBuffer;
    BOOLEAN                 FcbResourceAcquired = FALSE;

    __try
    {
        ASSERT(IrpContext != NULL);

        ASSERT((IrpContext->Identifier.Type == ICX) &&
               (IrpContext->Identifier.Size == sizeof(FSD_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;

        if (DeviceObject == FsdGlobalData.DeviceObject)
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
        }

        FileObject = IrpContext->FileObject;

        Fcb = (PFSD_FCB) FileObject->FsContext;

        ASSERT(Fcb != NULL);

        if (Fcb->Identifier.Type == VCB)
        {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        ASSERT((Fcb->Identifier.Type == FCB) &&
               (Fcb->Identifier.Size == sizeof(FSD_FCB)));

#ifndef FSD_RO
        if (!FlagOn(Fcb->Flags, FCB_PAGE_FILE))
#endif
        {
            if (!ExAcquireResourceSharedLite(
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

        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        FileInformationClass = IrpSp->Parameters.QueryFile.FileInformationClass;

        Length = IrpSp->Parameters.QueryFile.Length;

        SystemBuffer = Irp->AssociatedIrp.SystemBuffer;

        RtlZeroMemory(SystemBuffer, Length);

        switch (FileInformationClass)
        {
        case FileBasicInformation:
            {
                PFILE_BASIC_INFORMATION Buffer;

                if (Length < sizeof(FILE_BASIC_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }

                Buffer = (PFILE_BASIC_INFORMATION) SystemBuffer;

/*
                typedef struct _FILE_BASIC_INFORMATION {
                    LARGE_INTEGER   CreationTime;
                    LARGE_INTEGER   LastAccessTime;
                    LARGE_INTEGER   LastWriteTime;
                    LARGE_INTEGER   ChangeTime;
                    ULONG           FileAttributes;
                } FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;
*/

                Buffer->CreationTime.QuadPart = 0;

                Buffer->LastAccessTime.QuadPart = 0;

                Buffer->LastWriteTime.QuadPart = 0;

                Buffer->ChangeTime.QuadPart = 0;

                Buffer->FileAttributes = Fcb->FileAttributes;

                Irp->IoStatus.Information = sizeof(FILE_BASIC_INFORMATION);
                Status = STATUS_SUCCESS;
                __leave;
            }

        case FileStandardInformation:
            {
                PFILE_STANDARD_INFORMATION Buffer;

                if (Length < sizeof(FILE_STANDARD_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }

                Buffer = (PFILE_STANDARD_INFORMATION) SystemBuffer;

/*
                typedef struct _FILE_STANDARD_INFORMATION {
                    LARGE_INTEGER   AllocationSize;
                    LARGE_INTEGER   EndOfFile;
                    ULONG           NumberOfLinks;
                    BOOLEAN         DeletePending;
                    BOOLEAN         Directory;
                } FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;
*/

                Buffer->AllocationSize.QuadPart =
                    be32_to_cpu(Fcb->romfs_inode->size);

                Buffer->EndOfFile.QuadPart =
                    be32_to_cpu(Fcb->romfs_inode->size);

                Buffer->NumberOfLinks = 1;

#ifndef FSD_RO
                Buffer->DeletePending = FlagOn(Fcb->Flags, FCB_DELETE_PENDING);
#else
                Buffer->DeletePending = FALSE;
#endif

                Buffer->Directory = FlagOn(Fcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY);

                Irp->IoStatus.Information = sizeof(FILE_STANDARD_INFORMATION);
                Status = STATUS_SUCCESS;
                __leave;
            }

        case FileInternalInformation:
            {
                PFILE_INTERNAL_INFORMATION Buffer;

                if (Length < sizeof(FILE_INTERNAL_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }

                Buffer = (PFILE_INTERNAL_INFORMATION) SystemBuffer;

/*
                typedef struct _FILE_INTERNAL_INFORMATION {
                    LARGE_INTEGER IndexNumber;
                } FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;
*/

                // The "inode number"
                Buffer->IndexNumber = Fcb->IndexNumber;

                Irp->IoStatus.Information = sizeof(FILE_INTERNAL_INFORMATION);
                Status = STATUS_SUCCESS;
                __leave;
            }

        case FileEaInformation:
            {
                PFILE_EA_INFORMATION Buffer;

                if (Length < sizeof(FILE_EA_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }

                Buffer = (PFILE_EA_INFORMATION) SystemBuffer;

/*
                typedef struct _FILE_EA_INFORMATION {
                    ULONG EaSize;
                } FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;
*/

                // Romfs doesn't have any extended attributes
                Buffer->EaSize = 0;

                Irp->IoStatus.Information = sizeof(FILE_EA_INFORMATION);
                Status = STATUS_SUCCESS;
                __leave;
            }

        case FileNameInformation:
            {
                PFILE_NAME_INFORMATION Buffer;

                if (Length < sizeof(FILE_NAME_INFORMATION) +
                    Fcb->FileName.Length - sizeof(WCHAR))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }

                Buffer = (PFILE_NAME_INFORMATION) SystemBuffer;

/*
                typedef struct _FILE_NAME_INFORMATION {
                    ULONG FileNameLength;
                    WCHAR FileName[1];
                } FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;
*/

                Buffer->FileNameLength = Fcb->FileName.Length;

                RtlCopyMemory(
                    Buffer->FileName,
                    Fcb->FileName.Buffer,
                    Fcb->FileName.Length
                    );

                Irp->IoStatus.Information = sizeof(FILE_NAME_INFORMATION) +
                    Fcb->FileName.Length - sizeof(WCHAR);
                Status = STATUS_SUCCESS;
                __leave;
            }

        case FilePositionInformation:
            {
                PFILE_POSITION_INFORMATION Buffer;

                if (Length < sizeof(FILE_POSITION_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }

                Buffer = (PFILE_POSITION_INFORMATION) SystemBuffer;

/*
                typedef struct _FILE_POSITION_INFORMATION {
                    LARGE_INTEGER CurrentByteOffset;
                } FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;
*/

                Buffer->CurrentByteOffset = FileObject->CurrentByteOffset;

                Irp->IoStatus.Information = sizeof(FILE_POSITION_INFORMATION);
                Status = STATUS_SUCCESS;
                __leave;
            }

        case FileAllInformation:
            {
                PFILE_ALL_INFORMATION       FileAllInformation;
                PFILE_BASIC_INFORMATION     FileBasicInformation;
                PFILE_STANDARD_INFORMATION  FileStandardInformation;
                PFILE_INTERNAL_INFORMATION  FileInternalInformation;
                PFILE_EA_INFORMATION        FileEaInformation;
                PFILE_POSITION_INFORMATION  FilePositionInformation;
                PFILE_NAME_INFORMATION      FileNameInformation;

                if (Length < sizeof(FILE_ALL_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }

                FileAllInformation = (PFILE_ALL_INFORMATION) SystemBuffer;

/*
                typedef struct _FILE_ALL_INFORMATION {
                    FILE_BASIC_INFORMATION      BasicInformation;
                    FILE_STANDARD_INFORMATION   StandardInformation;
                    FILE_INTERNAL_INFORMATION   InternalInformation;
                    FILE_EA_INFORMATION         EaInformation;
                    FILE_ACCESS_INFORMATION     AccessInformation;
                    FILE_POSITION_INFORMATION   PositionInformation;
                    FILE_MODE_INFORMATION       ModeInformation;
                    FILE_ALIGNMENT_INFORMATION  AlignmentInformation;
                    FILE_NAME_INFORMATION       NameInformation;
                } FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;
*/

                FileBasicInformation =
                    &FileAllInformation->BasicInformation;

                FileStandardInformation =
                    &FileAllInformation->StandardInformation;

                FileInternalInformation =
                    &FileAllInformation->InternalInformation;

                FileEaInformation =
                    &FileAllInformation->EaInformation;

                FilePositionInformation =
                    &FileAllInformation->PositionInformation;

                FileNameInformation =
                    &FileAllInformation->NameInformation;

                FileBasicInformation->CreationTime.QuadPart = 0;

                FileBasicInformation->LastAccessTime.QuadPart = 0;

                FileBasicInformation->LastWriteTime.QuadPart = 0;

                FileBasicInformation->ChangeTime.QuadPart = 0;

                FileBasicInformation->FileAttributes = Fcb->FileAttributes;

                FileStandardInformation->AllocationSize.QuadPart =
                    be32_to_cpu(Fcb->romfs_inode->size);

                FileStandardInformation->EndOfFile.QuadPart =
                    be32_to_cpu(Fcb->romfs_inode->size);

                FileStandardInformation->NumberOfLinks = 1;

#ifndef FSD_RO
                FileStandardInformation->DeletePending =
                    FlagOn(Fcb->Flags, FCB_DELETE_PENDING);
#else
                FileStandardInformation->DeletePending = FALSE;
#endif

                FileStandardInformation->Directory =
                    FlagOn(Fcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY);

                // The "inode number"
                FileInternalInformation->IndexNumber = Fcb->IndexNumber;

                // Romfs doesn't have any extended attributes
                FileEaInformation->EaSize = 0;

                FilePositionInformation->CurrentByteOffset =
                    FileObject->CurrentByteOffset;

                if (Length < sizeof(FILE_ALL_INFORMATION) +
                    Fcb->FileName.Length - sizeof(WCHAR))
                {
                    Irp->IoStatus.Information = sizeof(FILE_ALL_INFORMATION);
                    Status = STATUS_BUFFER_OVERFLOW;
                    __leave;
                }

                FileNameInformation->FileNameLength = Fcb->FileName.Length;

                RtlCopyMemory(
                    FileNameInformation->FileName,
                    Fcb->FileName.Buffer,
                    Fcb->FileName.Length
                    );

                Irp->IoStatus.Information = sizeof(FILE_ALL_INFORMATION) +
                    Fcb->FileName.Length - sizeof(WCHAR);
                Status = STATUS_SUCCESS;
                __leave;
            }

        case FileNetworkOpenInformation:
            {
                PFILE_NETWORK_OPEN_INFORMATION Buffer;

                if (Length < sizeof(FILE_NETWORK_OPEN_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }

                Buffer = (PFILE_NETWORK_OPEN_INFORMATION) SystemBuffer;

/*
                typedef struct _FILE_NETWORK_OPEN_INFORMATION {
                    LARGE_INTEGER   CreationTime;
                    LARGE_INTEGER   LastAccessTime;
                    LARGE_INTEGER   LastWriteTime;
                    LARGE_INTEGER   ChangeTime;
                    LARGE_INTEGER   AllocationSize;
                    LARGE_INTEGER   EndOfFile;
                    ULONG           FileAttributes;
                } FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;
*/

                Buffer->CreationTime.QuadPart = 0;

                Buffer->LastAccessTime.QuadPart = 0;

                Buffer->LastWriteTime.QuadPart = 0;

                Buffer->ChangeTime.QuadPart = 0;

                Buffer->AllocationSize.QuadPart =
                    be32_to_cpu(Fcb->romfs_inode->size);

                Buffer->EndOfFile.QuadPart =
                    be32_to_cpu(Fcb->romfs_inode->size);

                Buffer->FileAttributes = Fcb->FileAttributes;

                Irp->IoStatus.Information =
                    sizeof(FILE_NETWORK_OPEN_INFORMATION);
                Status = STATUS_SUCCESS;
                __leave;
            }

#if (VER_PRODUCTBUILD >= 2195)

        case FileAttributeTagInformation:
            {
                PFILE_ATTRIBUTE_TAG_INFORMATION Buffer;

                if (Length < sizeof(FILE_ATTRIBUTE_TAG_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }

                Buffer = (PFILE_ATTRIBUTE_TAG_INFORMATION) SystemBuffer;

/*
                typedef struct _FILE_ATTRIBUTE_TAG_INFORMATION {
                    ULONG FileAttributes;
                    ULONG ReparseTag;
                } FILE_ATTRIBUTE_TAG_INFORMATION, *PFILE_ATTRIBUTE_TAG_INFORMATION;
*/

                Buffer->FileAttributes = Fcb->FileAttributes;

                Buffer->ReparseTag = 0;

                Irp->IoStatus.Information =
                    sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);
                Status = STATUS_SUCCESS;
                __leave;
            }

#endif // (VER_PRODUCTBUILD >= 2195)

        default:
            Status = STATUS_INVALID_INFO_CLASS;
        }
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

__drv_mustHoldCriticalRegion
NTSTATUS
FsdSetInformation (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PFSD_VCB                Vcb;
    PFILE_OBJECT            FileObject;
    PFSD_FCB                Fcb;
    PFSD_CCB                Ccb;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;
    FILE_INFORMATION_CLASS  FileInformationClass;
    ULONG                   Length;
    PVOID                   SystemBuffer;
#ifndef FSD_RO
    BOOLEAN                 VcbResourceAcquired = FALSE;
#endif
    BOOLEAN                 FcbMainResourceAcquired = FALSE;
#ifndef FSD_RO
    BOOLEAN                 FcbPagingIoResourceAcquired = FALSE;
#endif

    __try
    {
        ASSERT(IrpContext != NULL);

        ASSERT((IrpContext->Identifier.Type == ICX) &&
               (IrpContext->Identifier.Size == sizeof(FSD_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;

        if (DeviceObject == FsdGlobalData.DeviceObject)
        {
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

        if (Fcb->Identifier.Type == VCB)
        {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        ASSERT((Fcb->Identifier.Type == FCB) &&
               (Fcb->Identifier.Size == sizeof(FSD_FCB)));

        Ccb = (PFSD_CCB) FileObject->FsContext2;

        ASSERT(Ccb != NULL);

        ASSERT((Ccb->Identifier.Type == CCB) &&
               (Ccb->Identifier.Size == sizeof(FSD_CCB)));

        Irp = IrpContext->Irp;

        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        FileInformationClass = IrpSp->Parameters.SetFile.FileInformationClass;

        Length = IrpSp->Parameters.SetFile.Length;

        SystemBuffer = Irp->AssociatedIrp.SystemBuffer;

#ifndef FSD_RO

        if (FileInformationClass == FileDispositionInformation ||
            FileInformationClass == FileRenameInformation ||
            FileInformationClass == FileLinkInformation)
        {
            if (!ExAcquireResourceExclusiveLite(
                    &Vcb->MainResource,
                    IrpContext->IsSynchronous
                    ))
            {
                Status = STATUS_PENDING;
                __leave;
            }

            VcbResourceAcquired = TRUE;
        }

#endif // !FSD_RO

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

            FcbMainResourceAcquired = TRUE;
        }

#ifndef FSD_RO

        if (FileInformationClass == FileDispositionInformation ||
            FileInformationClass == FileRenameInformation ||
            FileInformationClass == FileLinkInformation ||
            FileInformationClass == FileAllocationInformation ||
            FileInformationClass == FileEndOfFileInformation)
        {
            if (!ExAcquireResourceExclusiveLite(
                    &Fcb->PagingIoResource,
                    IrpContext->IsSynchronous
                    ))
            {
                Status = STATUS_PENDING;
                __leave;
            }

            FcbPagingIoResourceAcquired = TRUE;
        }

#endif // !FSD_RO

#ifndef FSD_RO
        if (FlagOn(Vcb->Flags, VCB_READ_ONLY))
#endif
        {
            if (FileInformationClass != FilePositionInformation)
            {
                Status = STATUS_MEDIA_WRITE_PROTECTED;
                __leave;
            }
        }

        switch (FileInformationClass)
        {
        //
        // This is the only set file information request supported on read
        // only file systems
        //
        case FilePositionInformation:
            {
                PFILE_POSITION_INFORMATION Buffer;

                if (Length < sizeof(FILE_POSITION_INFORMATION))
                {
                    Status = STATUS_INVALID_PARAMETER;
                    __leave;
                }

                Buffer = (PFILE_POSITION_INFORMATION) SystemBuffer;

/*
                typedef struct _FILE_POSITION_INFORMATION {
                    LARGE_INTEGER CurrentByteOffset;
                } FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;
*/

                if (FlagOn(FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING) &&
                    (Buffer->CurrentByteOffset.LowPart &
                     DeviceObject->AlignmentRequirement)
                   )
                {
                    Status = STATUS_INVALID_PARAMETER;
                    __leave;
                }

                FileObject->CurrentByteOffset = Buffer->CurrentByteOffset;

                Status = STATUS_SUCCESS;
                __leave;
            }

        default:
            Status = STATUS_INVALID_INFO_CLASS;
        }
    }
    __finally
    {

#ifndef FSD_RO

        if (FcbPagingIoResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->PagingIoResource,
                ExGetCurrentResourceThread()
                );
        }

#endif // !FSD_RO

        if (FcbMainResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }

#ifndef FSD_RO

        if (VcbResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }

#endif // !FSD_RO

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
