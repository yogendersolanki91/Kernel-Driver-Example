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
FsdDirectoryControl (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == ICX) &&
           (IrpContext->Identifier.Size == sizeof(FSD_IRP_CONTEXT)));

    switch (IrpContext->MinorFunction)
    {
    case IRP_MN_QUERY_DIRECTORY:
        Status = FsdQueryDirectory(IrpContext);
        break;

    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
        Status = FsdNotifyChangeDirectory(IrpContext);
        break;

    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        IrpContext->Irp->IoStatus.Status = Status;
        FsdCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
        FsdFreeIrpContext(IrpContext);
    }

    return Status;
}

__drv_mustHoldCriticalRegion
NTSTATUS
FsdQueryDirectory (
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
    PUNICODE_STRING         FileName;
    UNICODE_STRING          UpcaseFileName;
    ULONG                   FileIndex;
    BOOLEAN                 RestartScan;
    BOOLEAN                 ReturnSingleEntry;
    BOOLEAN                 IndexSpecified;
    PUCHAR                  UserBuffer;
    BOOLEAN                 FirstQuery;
    struct romfs_inode*     Inode = NULL;
    BOOLEAN                 FcbResourceAcquired = FALSE;
    ULONG                   QueryBlockLength;
    ULONG                   UsedLength = 0;
    USHORT                  InodeFileNameLength;
    UNICODE_STRING          InodeFileName;
    PULONG                  NextEntryOffset = NULL;

    PAGED_CODE();

    UpcaseFileName.Buffer = NULL;
    InodeFileName.Buffer = NULL;

    __try
    {
        ASSERT(IrpContext);

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

        ASSERT(Fcb);

        if (Fcb->Identifier.Type == VCB)
        {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        ASSERT((Fcb->Identifier.Type == FCB) &&
               (Fcb->Identifier.Size == sizeof(FSD_FCB)));

        if (!FlagOn(Fcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
        {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        Ccb = (PFSD_CCB) FileObject->FsContext2;

        ASSERT(Ccb);

        ASSERT((Ccb->Identifier.Type == CCB) &&
               (Ccb->Identifier.Size == sizeof(FSD_CCB)));

        Irp = IrpContext->Irp;

        IrpSp = IoGetCurrentIrpStackLocation(Irp);

#ifndef _GNU_NTIFS_

        FileInformationClass =
            IrpSp->Parameters.QueryDirectory.FileInformationClass;

        Length = IrpSp->Parameters.QueryDirectory.Length;

        FileName = IrpSp->Parameters.QueryDirectory.FileName;

        FileIndex = IrpSp->Parameters.QueryDirectory.FileIndex;

#else // _GNU_NTIFS_

        FileInformationClass = ((PEXTENDED_IO_STACK_LOCATION)
            IrpSp)->Parameters.QueryDirectory.FileInformationClass;

        Length = ((PEXTENDED_IO_STACK_LOCATION)
            IrpSp)->Parameters.QueryDirectory.Length;

        FileName = ((PEXTENDED_IO_STACK_LOCATION)
            IrpSp)->Parameters.QueryDirectory.FileName;

        FileIndex = ((PEXTENDED_IO_STACK_LOCATION)
            IrpSp)->Parameters.QueryDirectory.FileIndex;

#endif // _GNU_NTIFS_

        RestartScan = FlagOn(IrpSp->Flags, SL_RESTART_SCAN);
        ReturnSingleEntry = FlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY);
        IndexSpecified = FlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED);

        if (Irp->RequestorMode != KernelMode &&
            !Irp->MdlAddress &&
            Irp->UserBuffer)
        {
            __try
            {
                ProbeForWrite(Irp->UserBuffer, Length, 1);
                Status = STATUS_SUCCESS;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                Status = STATUS_INVALID_USER_BUFFER;
            }

            if (Status != STATUS_SUCCESS)
            {
                __leave;
            }
        }

        UserBuffer = FsdGetUserBuffer(Irp);

        if (UserBuffer == NULL)
        {
            Status = STATUS_INVALID_USER_BUFFER;
            __leave;
        }

        if (!IrpContext->IsSynchronous)
        {
            Status = STATUS_PENDING;
            __leave;
        }

        if (!ExAcquireResourceSharedLite(
                 &Fcb->MainResource,
                 IrpContext->IsSynchronous
                 ))
        {
            Status = STATUS_PENDING;
            __leave;
        }

        FcbResourceAcquired = TRUE;

        if (FileName != NULL)
        {
            UpcaseFileName.Length = UpcaseFileName.MaximumLength =
                FileName->Length;

            UpcaseFileName.Buffer =
                FsdAllocatePool(NonPagedPool, FileName->Length, '1iDR');

            if (UpcaseFileName.Buffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }

            RtlUpcaseUnicodeString(
                &UpcaseFileName,
                FileName,
                FALSE
                );

            FileName = &UpcaseFileName;

            if (Ccb->DirectorySearchPattern.Buffer != NULL)
            {
                FirstQuery = FALSE;
            }
            else
            {
                FirstQuery = TRUE;

                Ccb->DirectorySearchPattern.Length =
                Ccb->DirectorySearchPattern.MaximumLength =
                    FileName->Length;

                Ccb->DirectorySearchPattern.Buffer =
                    FsdAllocatePool(NonPagedPool, FileName->Length, '2iDR');

                if (Ccb->DirectorySearchPattern.Buffer == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    __leave;
                }

                RtlCopyMemory(
                    Ccb->DirectorySearchPattern.Buffer,
                    FileName->Buffer,
                    FileName->Length
                    );
            }
        }
        else if (Ccb->DirectorySearchPattern.Buffer != NULL)
        {
            FirstQuery = FALSE;
            FileName = &Ccb->DirectorySearchPattern;
        }
        else
        {
            FirstQuery = TRUE;

            Ccb->DirectorySearchPattern.Length = sizeof(WCHAR);
            Ccb->DirectorySearchPattern.MaximumLength = sizeof(WCHAR);

            Ccb->DirectorySearchPattern.Buffer =
                FsdAllocatePool(NonPagedPool, sizeof(WCHAR), '3iDR');

            if (Ccb->DirectorySearchPattern.Buffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }

            RtlCopyMemory(Ccb->DirectorySearchPattern.Buffer, L"*", sizeof(WCHAR));

            FileName = &Ccb->DirectorySearchPattern;
        }

        if (!IndexSpecified)
        {
            if (RestartScan || FirstQuery)
            {
                FileIndex = Fcb->romfs_inode->spec;
            }
            else
            {
                FileIndex = Ccb->CurrentByteOffset;
            }
        }

        if (!(be32_to_cpu(FileIndex) & ROMFH_MASK))
        {
            if (FirstQuery)
            {
                Status = STATUS_NO_SUCH_FILE;
            }
            else
            {
                Status = STATUS_NO_MORE_FILES;
            }
            __leave;
        }

        Inode = (struct romfs_inode*) FsdAllocatePool(
            NonPagedPool,
            sizeof(struct romfs_inode) + ROMFS_MAXFN,
            '4iDR'
            );

        if (Inode == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        RtlZeroMemory(UserBuffer, Length);

        switch (FileInformationClass)
        {
        case FileDirectoryInformation:
            if (Length < sizeof(FILE_DIRECTORY_INFORMATION))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                __leave;
            }
            QueryBlockLength = sizeof(FILE_DIRECTORY_INFORMATION);
            break;

        case FileFullDirectoryInformation:
            if (Length < sizeof(FILE_FULL_DIR_INFORMATION))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                __leave;
            }
            QueryBlockLength = sizeof(FILE_FULL_DIR_INFORMATION);
            break;

        case FileBothDirectoryInformation:
            if (Length < sizeof(FILE_BOTH_DIR_INFORMATION))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                __leave;
            }
            QueryBlockLength = sizeof(FILE_BOTH_DIR_INFORMATION);
            break;

        case FileNamesInformation:
            if (Length < sizeof(FILE_NAMES_INFORMATION))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                __leave;
            }
            QueryBlockLength = sizeof(FILE_NAMES_INFORMATION);
            break;

#if (VER_PRODUCTBUILD >= 2600)

        case FileIdFullDirectoryInformation:
            if (Length < sizeof(FILE_ID_FULL_DIR_INFORMATION))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                __leave;
            }
            QueryBlockLength = sizeof(FILE_ID_FULL_DIR_INFORMATION);
            break;

        case FileIdBothDirectoryInformation:
            if (Length < sizeof(FILE_ID_BOTH_DIR_INFORMATION))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                __leave;
            }
            QueryBlockLength = sizeof(FILE_ID_BOTH_DIR_INFORMATION);
            break;

#endif // (VER_PRODUCTBUILD >= 2600)

        default:
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        while (UsedLength < Length
           && (be32_to_cpu(FileIndex) & ROMFH_MASK))
        {
            Status = FsdReadInodeByIndex(
                Vcb->TargetDeviceObject,
                FileIndex,
                Inode
                );

            if (!NT_SUCCESS(Status))
            {
                if (!UsedLength)
                {
                    if (FirstQuery)
                    {
                        Status = STATUS_NO_SUCH_FILE;
                    }
                    else
                    {
                        Status = STATUS_NO_MORE_FILES;
                    }
                }
                else
                {
                    Status = STATUS_SUCCESS;
                }
                __leave;
            }

            InodeFileNameLength = (USHORT) strnlen(Inode->name, ROMFS_MAXFN);

            if (Length - UsedLength < QueryBlockLength +
                InodeFileNameLength * 2 - sizeof(WCHAR))
            {
                if (!UsedLength)
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else
                {
                    Status = STATUS_SUCCESS;
                }
                __leave;
            }

            InodeFileName.Length =
            InodeFileName.MaximumLength =
                InodeFileNameLength * 2;

            InodeFileName.Buffer = FsdAllocatePool(
                NonPagedPool,
                InodeFileNameLength * 2,
                '5iDR'
                );

            FsdCharToWchar(
                InodeFileName.Buffer,
                Inode->name,
                InodeFileNameLength
                );

            if (FsRtlDoesNameContainWildCards(FileName) ?
                    FsRtlIsNameInExpression(
                        FileName,
                        &InodeFileName,
                        TRUE,
                        NULL
                        ) :
                    !RtlCompareUnicodeString(
                        FileName,
                        &InodeFileName,
                        TRUE
                        )
                    )
            {
                switch (FileInformationClass)
                {
                case FileDirectoryInformation:
                    {
                        PFILE_DIRECTORY_INFORMATION Buffer;

                        Buffer = (PFILE_DIRECTORY_INFORMATION)
                            (UserBuffer + UsedLength);

/*
                        typedef struct _FILE_DIRECTORY_INFORMATION {
                            ULONG           NextEntryOffset;
                            ULONG           FileIndex;
                            LARGE_INTEGER   CreationTime;
                            LARGE_INTEGER   LastAccessTime;
                            LARGE_INTEGER   LastWriteTime;
                            LARGE_INTEGER   ChangeTime;
                            LARGE_INTEGER   EndOfFile;
                            LARGE_INTEGER   AllocationSize;
                            ULONG           FileAttributes;
                            ULONG           FileNameLength;
                            WCHAR           FileName[1];
                        } FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;
*/

                        Buffer->FileIndex = FileIndex;

                        Buffer->CreationTime.QuadPart = 0;

                        Buffer->LastAccessTime.QuadPart = 0;

                        Buffer->LastWriteTime.QuadPart = 0;

                        Buffer->ChangeTime.QuadPart = 0;

                        Buffer->EndOfFile.QuadPart =
                            be32_to_cpu(Inode->size);

                        Buffer->AllocationSize.QuadPart =
                            be32_to_cpu(Inode->size);

                        Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;

                        if (FlagOn(be32_to_cpu(Inode->next), ROMFH_DIR))
                        {
                            SetFlag(
                                Buffer->FileAttributes,
                                FILE_ATTRIBUTE_DIRECTORY
                                );
                        }

#ifndef FSD_RO
                        if (FlagOn(Vcb->Flags, VCB_READ_ONLY))
#endif
                        {
                            SetFlag(
                                Buffer->FileAttributes,
                                FILE_ATTRIBUTE_READONLY
                                );
                        }

                        Buffer->FileNameLength = InodeFileNameLength * 2;

                        RtlCopyMemory(
                            Buffer->FileName,
                            InodeFileName.Buffer,
                            InodeFileNameLength * 2
                            );

                        UsedLength += QueryBlockLength +
                            InodeFileNameLength * 2 - sizeof(WCHAR);

                        if (!ReturnSingleEntry)
                        {
                            Buffer->NextEntryOffset = QueryBlockLength +
                                InodeFileNameLength * 2 - sizeof(WCHAR) +
                                UsedLength % 8;
                        }
                        else
                        {
                            Buffer->NextEntryOffset = 0;
                        }

                        UsedLength += UsedLength % 8;

                        NextEntryOffset = &Buffer->NextEntryOffset;
                    }
                    break;

                case FileFullDirectoryInformation:
                    {
                        PFILE_FULL_DIR_INFORMATION Buffer;

                        Buffer = (PFILE_FULL_DIR_INFORMATION)
                            (UserBuffer + UsedLength);

/*
                        typedef struct _FILE_FULL_DIR_INFORMATION {
                            ULONG           NextEntryOffset;
                            ULONG           FileIndex;
                            LARGE_INTEGER   CreationTime;
                            LARGE_INTEGER   LastAccessTime;
                            LARGE_INTEGER   LastWriteTime;
                            LARGE_INTEGER   ChangeTime;
                            LARGE_INTEGER   EndOfFile;
                            LARGE_INTEGER   AllocationSize;
                            ULONG           FileAttributes;
                            ULONG           FileNameLength;
                            ULONG           EaSize;
                            WCHAR           FileName[1];
                        } FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;
*/

                        Buffer->FileIndex = FileIndex;

                        Buffer->CreationTime.QuadPart = 0;

                        Buffer->LastAccessTime.QuadPart = 0;

                        Buffer->LastWriteTime.QuadPart = 0;

                        Buffer->ChangeTime.QuadPart = 0;

                        Buffer->EndOfFile.QuadPart =
                            be32_to_cpu(Inode->size);

                        Buffer->AllocationSize.QuadPart =
                            be32_to_cpu(Inode->size);

                        Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;

                        if (FlagOn(be32_to_cpu(Inode->next), ROMFH_DIR))
                        {
                            SetFlag(
                                Buffer->FileAttributes,
                                FILE_ATTRIBUTE_DIRECTORY
                                );
                        }

#ifndef FSD_RO
                        if (FlagOn(Vcb->Flags, VCB_READ_ONLY))
#endif
                        {
                            SetFlag(
                                Buffer->FileAttributes,
                                FILE_ATTRIBUTE_READONLY
                                );
                        }

                        Buffer->FileNameLength = InodeFileNameLength * 2;

                        Buffer->EaSize = 0;

                        RtlCopyMemory(
                            Buffer->FileName,
                            InodeFileName.Buffer,
                            InodeFileNameLength * 2
                            );

                        UsedLength += QueryBlockLength +
                            InodeFileNameLength * 2 - sizeof(WCHAR);

                        if (!ReturnSingleEntry)
                        {
                            Buffer->NextEntryOffset = QueryBlockLength +
                                InodeFileNameLength * 2 - sizeof(WCHAR) +
                                UsedLength % 8;
                        }
                        else
                        {
                            Buffer->NextEntryOffset = 0;
                        }

                        UsedLength += UsedLength % 8;

                        NextEntryOffset = &Buffer->NextEntryOffset;
                    }
                    break;

                case FileBothDirectoryInformation:
                    {
                        PFILE_BOTH_DIR_INFORMATION Buffer;

                        Buffer = (PFILE_BOTH_DIR_INFORMATION)
                            (UserBuffer + UsedLength);

/*
                        typedef struct _FILE_BOTH_DIR_INFORMATION {
                            ULONG           NextEntryOffset;
                            ULONG           FileIndex;
                            LARGE_INTEGER   CreationTime;
                            LARGE_INTEGER   LastAccessTime;
                            LARGE_INTEGER   LastWriteTime;
                            LARGE_INTEGER   ChangeTime;
                            LARGE_INTEGER   EndOfFile;
                            LARGE_INTEGER   AllocationSize;
                            ULONG           FileAttributes;
                            ULONG           FileNameLength;
                            ULONG           EaSize;
                            CCHAR           ShortNameLength;
                            WCHAR           ShortName[12];
                            WCHAR           FileName[1];
                        } FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;
*/

                        Buffer->FileIndex = FileIndex;

                        Buffer->CreationTime.QuadPart = 0;

                        Buffer->LastAccessTime.QuadPart = 0;

                        Buffer->LastWriteTime.QuadPart = 0;

                        Buffer->ChangeTime.QuadPart = 0;

                        Buffer->EndOfFile.QuadPart =
                            be32_to_cpu(Inode->size);

                        Buffer->AllocationSize.QuadPart =
                            be32_to_cpu(Inode->size);

                        Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;

                        if (FlagOn(be32_to_cpu(Inode->next), ROMFH_DIR))
                        {
                            SetFlag(
                                Buffer->FileAttributes,
                                FILE_ATTRIBUTE_DIRECTORY
                                );
                        }

#ifndef FSD_RO
                        if (FlagOn(Vcb->Flags, VCB_READ_ONLY))
#endif
                        {
                            SetFlag(
                                Buffer->FileAttributes,
                                FILE_ATTRIBUTE_READONLY
                                );
                        }

                        Buffer->FileNameLength = InodeFileNameLength * 2;

                        Buffer->EaSize = 0;

                        // TODO: Present a short alias

                        // Here we would like to use RtlGenerate8dot3Name but
                        // I don't know how to use the argument
                        // PGENERATE_NAME_CONTEXT

                        // Buffer->ShortNameLength
                        // Buffer->ShortName

                        RtlCopyMemory(
                            Buffer->FileName,
                            InodeFileName.Buffer,
                            InodeFileNameLength * 2
                            );

                        UsedLength += QueryBlockLength +
                            InodeFileNameLength * 2 - sizeof(WCHAR);

                        if (!ReturnSingleEntry)
                        {
                            Buffer->NextEntryOffset = QueryBlockLength +
                                InodeFileNameLength * 2 - sizeof(WCHAR) +
                                UsedLength % 8;
                        }
                        else
                        {
                            Buffer->NextEntryOffset = 0;
                        }

                        UsedLength += UsedLength % 8;

                        NextEntryOffset = &Buffer->NextEntryOffset;
                    }
                    break;

                case FileNamesInformation:
                    {
                        PFILE_NAMES_INFORMATION Buffer;

                        Buffer = (PFILE_NAMES_INFORMATION)
                            (UserBuffer + UsedLength);

/*
                        typedef struct _FILE_NAMES_INFORMATION {
                            ULONG NextEntryOffset;
                            ULONG FileIndex;
                            ULONG FileNameLength;
                            WCHAR FileName[1];
                        } FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;
*/

                        Buffer->FileIndex = FileIndex;

                        Buffer->FileNameLength = InodeFileNameLength * 2;

                        RtlCopyMemory(
                            Buffer->FileName,
                            InodeFileName.Buffer,
                            InodeFileNameLength * 2
                            );

                        UsedLength += QueryBlockLength +
                            InodeFileNameLength * 2 - sizeof(WCHAR);

                        if (!ReturnSingleEntry)
                        {
                            Buffer->NextEntryOffset = QueryBlockLength +
                                InodeFileNameLength * 2 - sizeof(WCHAR) +
                                UsedLength % 8;
                        }
                        else
                        {
                            Buffer->NextEntryOffset = 0;
                        }

                        UsedLength += UsedLength % 8;

                        NextEntryOffset = &Buffer->NextEntryOffset;
                    }
                    break;

#if (VER_PRODUCTBUILD >= 2600)

                case FileIdFullDirectoryInformation:
                    {
                        PFILE_ID_FULL_DIR_INFORMATION Buffer;

                        Buffer = (PFILE_ID_FULL_DIR_INFORMATION)
                            (UserBuffer + UsedLength);

/*
                        typedef struct _FILE_ID_FULL_DIR_INFORMATION {
                            ULONG           NextEntryOffset;
                            ULONG           FileIndex;
                            LARGE_INTEGER   CreationTime;
                            LARGE_INTEGER   LastAccessTime;
                            LARGE_INTEGER   LastWriteTime;
                            LARGE_INTEGER   ChangeTime;
                            LARGE_INTEGER   EndOfFile;
                            LARGE_INTEGER   AllocationSize;
                            ULONG           FileAttributes;
                            ULONG           FileNameLength;
                            ULONG           EaSize;
                            LARGE_INTEGER   FileId;
                            WCHAR           FileName[1];
                        } FILE_ID_FULL_DIR_INFORMATION, *PFILE_ID_FULL_DIR_INFORMATION;
*/

                        Buffer->FileIndex = FileIndex;

                        Buffer->CreationTime.QuadPart = 0;

                        Buffer->LastAccessTime.QuadPart = 0;

                        Buffer->LastWriteTime.QuadPart = 0;

                        Buffer->ChangeTime.QuadPart = 0;

                        Buffer->EndOfFile.QuadPart =
                            be32_to_cpu(Inode->size);

                        Buffer->AllocationSize.QuadPart =
                            be32_to_cpu(Inode->size);

                        Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;

                        if (FlagOn(be32_to_cpu(Inode->next), ROMFH_DIR))
                        {
                            SetFlag(
                                Buffer->FileAttributes,
                                FILE_ATTRIBUTE_DIRECTORY
                                );
                        }

#ifndef FSD_RO
                        if (FlagOn(Vcb->Flags, VCB_READ_ONLY))
#endif
                        {
                            SetFlag(
                                Buffer->FileAttributes,
                                FILE_ATTRIBUTE_READONLY
                                );
                        }

                        Buffer->FileNameLength = InodeFileNameLength * 2;

                        Buffer->EaSize = 0;

                        Buffer->FileId = Fcb->IndexNumber;

                        RtlCopyMemory(
                            Buffer->FileName,
                            InodeFileName.Buffer,
                            InodeFileNameLength * 2
                            );

                        UsedLength += QueryBlockLength +
                            InodeFileNameLength * 2 - sizeof(WCHAR);

                        if (!ReturnSingleEntry)
                        {
                            Buffer->NextEntryOffset = QueryBlockLength +
                                InodeFileNameLength * 2 - sizeof(WCHAR) +
                                UsedLength % 8;
                        }
                        else
                        {
                            Buffer->NextEntryOffset = 0;
                        }

                        UsedLength += UsedLength % 8;

                        NextEntryOffset = &Buffer->NextEntryOffset;
                    }
                    break;

                case FileIdBothDirectoryInformation:
                    {
                        PFILE_ID_BOTH_DIR_INFORMATION Buffer;

                        Buffer = (PFILE_ID_BOTH_DIR_INFORMATION)
                            (UserBuffer + UsedLength);

/*
                        typedef struct _FILE_ID_BOTH_DIR_INFORMATION {
                            ULONG           NextEntryOffset;
                            ULONG           FileIndex;
                            LARGE_INTEGER   CreationTime;
                            LARGE_INTEGER   LastAccessTime;
                            LARGE_INTEGER   LastWriteTime;
                            LARGE_INTEGER   ChangeTime;
                            LARGE_INTEGER   EndOfFile;
                            LARGE_INTEGER   AllocationSize;
                            ULONG           FileAttributes;
                            ULONG           FileNameLength;
                            ULONG           EaSize;
                            CCHAR           ShortNameLength;
                            WCHAR           ShortName[12];
                            LARGE_INTEGER   FileId;
                            WCHAR           FileName[1];
                        } FILE_ID_BOTH_DIR_INFORMATION, *PFILE_ID_BOTH_DIR_INFORMATION;
*/

                        Buffer->FileIndex = FileIndex;

                        Buffer->CreationTime.QuadPart = 0;

                        Buffer->LastAccessTime.QuadPart = 0;

                        Buffer->LastWriteTime.QuadPart = 0;

                        Buffer->ChangeTime.QuadPart = 0;

                        Buffer->EndOfFile.QuadPart =
                            be32_to_cpu(Inode->size);

                        Buffer->AllocationSize.QuadPart =
                            be32_to_cpu(Inode->size);

                        Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;

                        if (FlagOn(be32_to_cpu(Inode->next), ROMFH_DIR))
                        {
                            SetFlag(
                                Buffer->FileAttributes,
                                FILE_ATTRIBUTE_DIRECTORY
                                );
                        }

#ifndef FSD_RO
                        if (FlagOn(Vcb->Flags, VCB_READ_ONLY))
#endif
                        {
                            SetFlag(
                                Buffer->FileAttributes,
                                FILE_ATTRIBUTE_READONLY
                                );
                        }

                        Buffer->FileNameLength = InodeFileNameLength * 2;

                        Buffer->EaSize = 0;

                        // TODO: Present a short alias

                        // Here we would like to use RtlGenerate8dot3Name but
                        // I don't know how to use the argument
                        // PGENERATE_NAME_CONTEXT

                        // Buffer->ShortNameLength
                        // Buffer->ShortName

                        Buffer->FileId = Fcb->IndexNumber;

                        RtlCopyMemory(
                            Buffer->FileName,
                            InodeFileName.Buffer,
                            InodeFileNameLength * 2
                            );

                        UsedLength += QueryBlockLength +
                            InodeFileNameLength * 2 - sizeof(WCHAR);

                        if (!ReturnSingleEntry)
                        {
                            Buffer->NextEntryOffset = QueryBlockLength +
                                InodeFileNameLength * 2 - sizeof(WCHAR) +
                                UsedLength % 8;
                        }
                        else
                        {
                            Buffer->NextEntryOffset = 0;
                        }

                        UsedLength += UsedLength % 8;

                        NextEntryOffset = &Buffer->NextEntryOffset;
                    }
                    break;

#endif // (VER_PRODUCTBUILD >= 2600)

                }
            }

            if (InodeFileName.Buffer != NULL)
            {
                FsdFreePool(InodeFileName.Buffer);
                InodeFileName.Buffer = NULL;
            }

            Ccb->CurrentByteOffset =
            FileIndex =
                Inode->next;

            if (UsedLength && ReturnSingleEntry)
            {
                Status = STATUS_SUCCESS;
                __leave;
            }
        }

        if (!UsedLength)
        {
            if (FirstQuery)
            {
                Status = STATUS_NO_SUCH_FILE;
            }
            else
            {
                Status = STATUS_NO_MORE_FILES;
            }
        }
        else
        {
            Status = STATUS_SUCCESS;
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

        if (UpcaseFileName.Buffer != NULL)
        {
            FsdFreePool(UpcaseFileName.Buffer);
        }

        if (Inode != NULL)
        {
            FsdFreePool(Inode);
        }

        if (InodeFileName.Buffer != NULL)
        {
            FsdFreePool(InodeFileName.Buffer);
        }

        if (NextEntryOffset != NULL)
        {
            *NextEntryOffset = 0;
        }

        if (!AbnormalTermination())
        {
            if (Status == STATUS_PENDING)
            {
                Status = FsdLockUserBuffer(
                    IrpContext->Irp,
                    Length,
                    IoWriteAccess
                    );

                if (NT_SUCCESS(Status))
                {
                    Status = FsdQueueRequest(IrpContext);
                }
                else
                {
                    IrpContext->Irp->IoStatus.Status = Status;
                    FsdCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
                    FsdFreeIrpContext(IrpContext);
                }
            }
            else
            {
                IrpContext->Irp->IoStatus.Information = UsedLength;
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
FsdNotifyChangeDirectory (
    IN PFSD_IRP_CONTEXT IrpContext
    )
{
    PDEVICE_OBJECT      DeviceObject;
    BOOLEAN             CompleteRequest;
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;
    PFSD_VCB            Vcb;
    PFILE_OBJECT        FileObject;
    PFSD_FCB            Fcb;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    ULONG               CompletionFilter;
    BOOLEAN             WatchTree;

    PAGED_CODE();

    __try
    {
        ASSERT(IrpContext);

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

        ASSERT(Fcb);

        if (Fcb->Identifier.Type == VCB)
        {
            CompleteRequest = TRUE;
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        ASSERT((Fcb->Identifier.Type == FCB) &&
               (Fcb->Identifier.Size == sizeof(FSD_FCB)));

        if (!FlagOn(Fcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
        {
            CompleteRequest = TRUE;
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        Irp = IrpContext->Irp;

        IrpSp = IoGetCurrentIrpStackLocation(Irp);

#ifndef _GNU_NTIFS_

        CompletionFilter =
            IrpSp->Parameters.NotifyDirectory.CompletionFilter;

#else

        CompletionFilter = ((PEXTENDED_IO_STACK_LOCATION)
            IrpSp)->Parameters.NotifyDirectory.CompletionFilter;

#endif

        WatchTree = FlagOn(IrpSp->Flags, SL_WATCH_TREE);

        CompleteRequest = FALSE;

        Status = STATUS_PENDING;

        FsRtlNotifyChangeDirectory(
            Vcb->NotifySync,
            FileObject->FsContext2,
            (PSTRING)&Fcb->FileName,
            &Vcb->NotifyList,
            WatchTree,
            CompletionFilter,
            Irp
            );

/*
    Currently the driver is read-only but here is an example on how to use the
    FsRtl-functions to report a change:

    UNICODE_STRING  TestString;
    USHORT          FileNamePartLength;

    RtlInitUnicodeString(&TestString, L"\\ntifs.h");

    FileNamePartLength = 7 * sizeof(WCHAR);

    FsRtlNotifyReportChange(
        Vcb->NotifySync,                // PNOTIFY_SYNC NotifySync
        &Vcb->NotifyList,               // PLIST_ENTRY  NotifyList
        (PSTRING)&TestString,           // PSTRING      FullTargetName
        &FileNamePartLength,            // PUSHORT      FileNamePartLength
        FILE_NOTIFY_CHANGE_FILE_NAME    // ULONG        FilterMatch
        );

    or

    UNICODE_STRING TestString;

    RtlInitUnicodeString(&TestString, L"\\ntifs.h");

    FsRtlNotifyFullReportChange(
        Vcb->NotifySync,                // PNOTIFY_SYNC NotifySync
        &Vcb->NotifyList,               // PLIST_ENTRY  NotifyList
        (PSTRING)&TestString,           // PSTRING      FullTargetName
        2,                              // USHORT       TargetNameOffset
        NULL,                           // PSTRING      StreamName OPTIONAL
        NULL,                           // PSTRING      NormalizedParentName OPTIONAL
        FILE_NOTIFY_CHANGE_FILE_NAME,   // ULONG        FilterMatch
        FILE_ACTION_ADDED,              // ULONG        Action
        NULL                            // PVOID        TargetContext
        );
*/
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
