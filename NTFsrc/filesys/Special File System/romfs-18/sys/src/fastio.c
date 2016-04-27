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

BOOLEAN
FsdFastIoCheckIfPossible (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN ULONG                Length,
    IN BOOLEAN              Wait,
    IN ULONG                LockKey,
    IN BOOLEAN              CheckForReadOperation,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
    )
{
    BOOLEAN         Status = FALSE;
    PFSD_FCB        Fcb;
    LARGE_INTEGER   LargeLength;

    PAGED_CODE();

    LargeLength.QuadPart = Length;

    __try
    {
        __try
        {
            FsRtlEnterFileSystem();

            if (DeviceObject == FsdGlobalData.DeviceObject)
            {
                Status = FALSE;
                __leave;
            }

            Fcb = (PFSD_FCB) FileObject->FsContext;

            ASSERT(Fcb != NULL);

            if (Fcb->Identifier.Type == VCB)
            {
                Status = FALSE;
                __leave;
            }

            ASSERT((Fcb->Identifier.Type == FCB) &&
                   (Fcb->Identifier.Size == sizeof(FSD_FCB)));

            if (FlagOn(Fcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
            {
                Status = FALSE;
                __leave;
            }

            if (CheckForReadOperation)
            {
                Status = FsRtlFastCheckLockForRead(
                    &Fcb->FileLock,
                    FileOffset,
                    &LargeLength,
                    LockKey,
                    FileObject,
                    PsGetCurrentProcess()
                    );
            }
            else
            {
#ifndef FSD_RO
                Status = FsRtlFastCheckLockForWrite(
                    &Fcb->FileLockAnchor,
                    FileOffset,
                    &LargeLength,
                    LockKey,
                    FileObject,
                    PsGetCurrentProcess()
                    );
#else
                Status = FALSE;
#endif
            }

            KdPrint((
                DRIVER_NAME ": %-16.16s %-31s %s\n",
                FsdGetCurrentProcessName(),
                "FASTIO_CHECK_IF_POSSIBLE",
                Fcb->AnsiFileName.Buffer
                ));

            KdPrint((
                DRIVER_NAME ": Offset: %I64u Length: %u Key: %u %s %s\n",
                FileOffset->QuadPart,
                Length,
                LockKey,
                (CheckForReadOperation ? "CheckForReadOperation:" : "CheckForWriteOperation:"),
                (Status ? "Succeeded" : "Failed")
                ));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            Status = FALSE;
        }
    }
    __finally
    {
        FsRtlExitFileSystem();
    }

    return Status;
}

//
// When not debugging the fast I/O read entry point is set directly to
// FsRtlCopyRead.
//

#if DBG

BOOLEAN
FsdFastIoRead (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN ULONG                Length,
    IN BOOLEAN              Wait,
    IN ULONG                LockKey,
    OUT PVOID               Buffer,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
    )
{
    BOOLEAN     Status;
    PFSD_FCB    Fcb;

    PAGED_CODE();

    Fcb = (PFSD_FCB) FileObject->FsContext;

    ASSERT(Fcb != NULL);

    ASSERT((Fcb->Identifier.Type == FCB) &&
           (Fcb->Identifier.Size == sizeof(FSD_FCB)));

    KdPrint((
        DRIVER_NAME ": %-16.16s %-31s %s\n",
        FsdGetCurrentProcessName(),
        "FASTIO_READ",
        Fcb->AnsiFileName.Buffer
        ));

    KdPrint((
        DRIVER_NAME ": Offset: %I64u Length: %u Key: %u\n",
        FileOffset->QuadPart,
        Length,
        LockKey
        ));

    Status = FsRtlCopyRead (
        FileObject,
        FileOffset,
        Length,
        Wait,
        LockKey,
        Buffer,
        IoStatus,
        DeviceObject
        );

    if (Status == FALSE)
    {
        KdPrint((
            DRIVER_NAME ": %-16.16s %-31s *** Status: FALSE ***\n",
            FsdGetCurrentProcessName(),
            "FASTIO_READ"
            ));
    }
    else if (IoStatus->Status != STATUS_SUCCESS)
    {
        KdPrint((
            DRIVER_NAME ": %-16.16s %-31s *** Status: %s (%#x) ***\n",
            FsdGetCurrentProcessName(),
            "FASTIO_READ",
            FsdNtStatusToString(IoStatus->Status),
            IoStatus->Status
            ));
    }

    return Status;
}

#endif // DBG

BOOLEAN
FsdFastIoQueryBasicInfo (
    IN PFILE_OBJECT             FileObject,
    IN BOOLEAN                  Wait,
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK        IoStatus,
    IN PDEVICE_OBJECT           DeviceObject
    )
{
    BOOLEAN     Status = FALSE;
    PFSD_FCB    Fcb;
    BOOLEAN     FcbMainResourceAcquired = FALSE;

    PAGED_CODE();

    __try
    {
        __try
        {
            FsRtlEnterFileSystem();

            if (DeviceObject == FsdGlobalData.DeviceObject)
            {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                __leave;
            }

            Fcb = (PFSD_FCB) FileObject->FsContext;

            ASSERT(Fcb != NULL);

            if (Fcb->Identifier.Type == VCB)
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }

            ASSERT((Fcb->Identifier.Type == FCB) &&
                   (Fcb->Identifier.Size == sizeof(FSD_FCB)));

            KdPrint((
                DRIVER_NAME ": %-16.16s %-31s %s\n",
                FsdGetCurrentProcessName(),
                "FASTIO_QUERY_BASIC_INFO",
                Fcb->AnsiFileName.Buffer
                ));

            if (!ExAcquireResourceSharedLite(
                    &Fcb->MainResource,
                    Wait
                    ))
            {
                Status = FALSE;
                __leave;
            }

            FcbMainResourceAcquired = TRUE;

            RtlZeroMemory(Buffer, sizeof(FILE_BASIC_INFORMATION));

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

            IoStatus->Information = sizeof(FILE_BASIC_INFORMATION);

            IoStatus->Status = STATUS_SUCCESS;

            Status =  TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            IoStatus->Status = GetExceptionCode();
            Status = TRUE;
        }
    }
    __finally
    {
        if (FcbMainResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }

        FsRtlExitFileSystem();
    }

    if (Status == FALSE)
    {
        KdPrint((
            DRIVER_NAME ": %-16.16s %-31s *** Status: FALSE ***\n",
            FsdGetCurrentProcessName(),
            "FASTIO_QUERY_BASIC_INFO"
            ));
    }
    else if (IoStatus->Status != STATUS_SUCCESS)
    {
        KdPrint((
            DRIVER_NAME ": %-16.16s %-31s *** Status: %s (%#x) ***\n",
            FsdGetCurrentProcessName(),
            "FASTIO_QUERY_BASIC_INFO",
            FsdNtStatusToString(IoStatus->Status),
            IoStatus->Status
            ));
    }

    return Status;
}

BOOLEAN
FsdFastIoQueryStandardInfo (
    IN PFILE_OBJECT                 FileObject,
    IN BOOLEAN                      Wait,
    OUT PFILE_STANDARD_INFORMATION  Buffer,
    OUT PIO_STATUS_BLOCK            IoStatus,
    IN PDEVICE_OBJECT               DeviceObject
    )
{
    BOOLEAN     Status = FALSE;
    PFSD_FCB    Fcb;
    BOOLEAN     FcbMainResourceAcquired = FALSE;

    PAGED_CODE();

    __try
    {
        __try
        {
            FsRtlEnterFileSystem();

            if (DeviceObject == FsdGlobalData.DeviceObject)
            {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                __leave;
            }

            Fcb = (PFSD_FCB) FileObject->FsContext;

            ASSERT(Fcb != NULL);

            if (Fcb->Identifier.Type == VCB)
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }

            ASSERT((Fcb->Identifier.Type == FCB) &&
                   (Fcb->Identifier.Size == sizeof(FSD_FCB)));

            KdPrint((
                DRIVER_NAME ": %-16.16s %-31s %s\n",
                FsdGetCurrentProcessName(),
                "FASTIO_QUERY_STANDARD_INFO",
                Fcb->AnsiFileName.Buffer
                ));

            if (!ExAcquireResourceSharedLite(
                    &Fcb->MainResource,
                    Wait
                    ))
            {
                Status = FALSE;
                __leave;
            }

            FcbMainResourceAcquired = TRUE;

            RtlZeroMemory(Buffer, sizeof(FILE_STANDARD_INFORMATION));

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

            IoStatus->Information = sizeof(FILE_STANDARD_INFORMATION);

            IoStatus->Status = STATUS_SUCCESS;

            Status =  TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            IoStatus->Status = GetExceptionCode();
            Status = TRUE;
        }
    }
    __finally
    {
        if (FcbMainResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }

        FsRtlExitFileSystem();
    }

    if (Status == FALSE)
    {
        KdPrint((
            DRIVER_NAME ": %-16.16s %-31s *** Status: FALSE ***\n",
            FsdGetCurrentProcessName(),
            "FASTIO_QUERY_STANDARD_INFO"
            ));
    }
    else if (IoStatus->Status != STATUS_SUCCESS)
    {
        KdPrint((
            DRIVER_NAME ": %-16.16s %-31s *** Status: %s (%#x) ***\n",
            FsdGetCurrentProcessName(),
            "FASTIO_QUERY_STANDARD_INFO",
            FsdNtStatusToString(IoStatus->Status),
            IoStatus->Status
            ));
    }

    return Status;
}

BOOLEAN
FsdFastIoLock (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    IN BOOLEAN              FailImmediately,
    IN BOOLEAN              ExclusiveLock,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
    )
{
    BOOLEAN     Status = FALSE;
    PFSD_FCB    Fcb;

    PAGED_CODE();

    __try
    {
        __try
        {
            FsRtlEnterFileSystem();

            if (DeviceObject == FsdGlobalData.DeviceObject)
            {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                __leave;
            }

            Fcb = (PFSD_FCB) FileObject->FsContext;

            ASSERT(Fcb != NULL);

            if (Fcb->Identifier.Type == VCB)
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }

            ASSERT((Fcb->Identifier.Type == FCB) &&
                   (Fcb->Identifier.Size == sizeof(FSD_FCB)));

            if (FlagOn(Fcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }

            KdPrint((
                DRIVER_NAME ": %-16.16s %-31s %s\n",
                (PUCHAR) Process + ProcessNameOffset,
                "FASTIO_LOCK",
                Fcb->AnsiFileName.Buffer
                ));

            KdPrint((
                DRIVER_NAME ": Offset: %I64u Length: %I64u Key: %u %s%s\n",
                FileOffset->QuadPart,
                Length->QuadPart,
                Key,
                (FailImmediately ? "FailImmediately " : ""),
                (ExclusiveLock ? "ExclusiveLock " : "")
                ));

            if (Fcb->CommonFCBHeader.IsFastIoPossible != FastIoIsQuestionable)
            {
                KdPrint((
                    DRIVER_NAME ": %-16.16s %-31s %s\n",
                    (PUCHAR) Process + ProcessNameOffset,
                    "FastIoIsQuestionable",
                    Fcb->AnsiFileName.Buffer
                    ));

                Fcb->CommonFCBHeader.IsFastIoPossible = FastIoIsQuestionable;
            }

#pragma prefast( suppress: 28159, "bug in prefast" )
            Status = FsRtlFastLock(
                &Fcb->FileLock,
                FileObject,
                FileOffset,
                Length,
                Process,
                Key,
                FailImmediately,
                ExclusiveLock,
                IoStatus,
                NULL,
                FALSE
                );
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            IoStatus->Status = GetExceptionCode();
            Status = TRUE;
        }
    }
    __finally
    {
        FsRtlExitFileSystem();
    }

    if (Status == FALSE)
    {
        KdPrint((
            DRIVER_NAME ": %-16.16s %-31s *** Status: FALSE ***\n",
            (PUCHAR) Process + ProcessNameOffset,
            "FASTIO_LOCK"
            ));
    }
    else if (IoStatus->Status != STATUS_SUCCESS)
    {
        KdPrint((
            DRIVER_NAME ": %-16.16s %-31s *** Status: %s (%#x) ***\n",
            (PUCHAR) Process + ProcessNameOffset,
            "FASTIO_LOCK",
            FsdNtStatusToString(IoStatus->Status),
            IoStatus->Status
            ));
    }

    return Status;
}

BOOLEAN
FsdFastIoUnlockSingle (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
    )
{
    BOOLEAN     Status = FALSE;
    PFSD_FCB    Fcb;

    PAGED_CODE();

    __try
    {
        __try
        {
            FsRtlEnterFileSystem();

            if (DeviceObject == FsdGlobalData.DeviceObject)
            {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                __leave;
            }

            Fcb = (PFSD_FCB) FileObject->FsContext;

            ASSERT(Fcb != NULL);

            if (Fcb->Identifier.Type == VCB)
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }

            ASSERT((Fcb->Identifier.Type == FCB) &&
                   (Fcb->Identifier.Size == sizeof(FSD_FCB)));

            if (FlagOn(Fcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }

            KdPrint((
                DRIVER_NAME ": %-16.16s %-31s %s\n",
                (PUCHAR) Process + ProcessNameOffset,
                "FASTIO_UNLOCK_SINGLE",
                Fcb->AnsiFileName.Buffer
                ));

            KdPrint((
                DRIVER_NAME ": Offset: %I64u Length: %I64u Key: %u\n",
                FileOffset->QuadPart,
                Length->QuadPart,
                Key
                ));

            IoStatus->Status = FsRtlFastUnlockSingle(
                &Fcb->FileLock,
                FileObject,
                FileOffset,
                Length,
                Process,
                Key,
                NULL,
                0
                );                      

            IoStatus->Information = 0;

            Status =  TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            IoStatus->Status = GetExceptionCode();
            Status = TRUE;
        }
    }
    __finally
    {
        FsRtlExitFileSystem();
    }

    if (Status == FALSE)
    {
        KdPrint((
            DRIVER_NAME ": %-16.16s %-31s *** Status: FALSE ***\n",
            (PUCHAR) Process + ProcessNameOffset,
            "FASTIO_UNLOCK_SINGLE"
            ));
    }
    else if (IoStatus->Status != STATUS_SUCCESS)
    {
        KdPrint((
            DRIVER_NAME ": %-16.16s %-31s *** Status: %s (%#x) ***\n",
            (PUCHAR) Process + ProcessNameOffset,
            "FASTIO_UNLOCK_SINGLE",
            FsdNtStatusToString(IoStatus->Status),
            IoStatus->Status
            ));
    }

    return Status;
}

BOOLEAN
FsdFastIoUnlockAll (
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
    )
{
    BOOLEAN     Status = FALSE;
    PFSD_FCB    Fcb;

    PAGED_CODE();

    __try
    {
        __try
        {
            FsRtlEnterFileSystem();

            if (DeviceObject == FsdGlobalData.DeviceObject)
            {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                __leave;
            }

            Fcb = (PFSD_FCB) FileObject->FsContext;

            ASSERT(Fcb != NULL);

            if (Fcb->Identifier.Type == VCB)
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }

            ASSERT((Fcb->Identifier.Type == FCB) &&
                   (Fcb->Identifier.Size == sizeof(FSD_FCB)));

            if (FlagOn(Fcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }

            KdPrint((
                DRIVER_NAME ": %-16.16s %-31s %s\n",
                (PUCHAR) Process + ProcessNameOffset,
                "FASTIO_UNLOCK_ALL",
                Fcb->AnsiFileName.Buffer
                ));

            IoStatus->Status = FsRtlFastUnlockAll(
                &Fcb->FileLock,
                FileObject,
                Process,
                NULL
                );

            IoStatus->Information = 0;

            Status =  TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            IoStatus->Status = GetExceptionCode();
            Status = TRUE;
        }
    }
    __finally
    {
        FsRtlExitFileSystem();
    }

    if (Status == FALSE)
    {
        KdPrint((
            DRIVER_NAME ": %-16.16s %-31s *** Status: FALSE ***\n",
            (PUCHAR) Process + ProcessNameOffset,
            "FASTIO_UNLOCK_ALL"
            ));
    }
    else if (IoStatus->Status != STATUS_SUCCESS)
    {
        KdPrint((
            DRIVER_NAME ": %-16.16s %-31s *** Status: %s (%#x) ***\n",
            (PUCHAR) Process + ProcessNameOffset,
            "FASTIO_UNLOCK_ALL",
            FsdNtStatusToString(IoStatus->Status),
            IoStatus->Status
            ));
    }

    return Status;
}

BOOLEAN
FsdFastIoUnlockAllByKey (
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
    )
{
    BOOLEAN     Status = FALSE;
    PFSD_FCB    Fcb;

    PAGED_CODE();

    __try
    {
        __try
        {
            FsRtlEnterFileSystem();

            if (DeviceObject == FsdGlobalData.DeviceObject)
            {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                __leave;
            }

            Fcb = (PFSD_FCB) FileObject->FsContext;

            ASSERT(Fcb != NULL);

            if (Fcb->Identifier.Type == VCB)
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }

            ASSERT((Fcb->Identifier.Type == FCB) &&
                   (Fcb->Identifier.Size == sizeof(FSD_FCB)));

            if (FlagOn(Fcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }

            KdPrint((
                DRIVER_NAME ": %-16.16s %-31s %s\n",
                (PUCHAR) Process + ProcessNameOffset,
                "FASTIO_UNLOCK_ALL_BY_KEY",
                Fcb->AnsiFileName.Buffer
                ));

            KdPrint((
                DRIVER_NAME ": Key: %u\n",
                Key
                ));

            IoStatus->Status = FsRtlFastUnlockAllByKey(
                &Fcb->FileLock,
                FileObject,
                Process,
                Key,
                NULL
                );  

            IoStatus->Information = 0;

            Status =  TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            IoStatus->Status = GetExceptionCode();
            Status = TRUE;
        }
    }
    __finally
    {
        FsRtlExitFileSystem();
    }

    if (Status == FALSE)
    {
        KdPrint((
            DRIVER_NAME ": %-16.16s %-31s *** Status: FALSE ***\n",
            (PUCHAR) Process + ProcessNameOffset,
            "FASTIO_UNLOCK_ALL_BY_KEY"
            ));
    }
    else if (IoStatus->Status != STATUS_SUCCESS)
    {
        KdPrint((
            DRIVER_NAME ": %-16.16s %-31s *** Status: %s (%#x) ***\n",
            (PUCHAR) Process + ProcessNameOffset,
            "FASTIO_UNLOCK_ALL_BY_KEY",
            FsdNtStatusToString(IoStatus->Status),
            IoStatus->Status
            ));
    }

    return Status;
}

BOOLEAN
FsdFastIoQueryNetworkOpenInfo (
    IN PFILE_OBJECT                     FileObject,
    IN BOOLEAN                          Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION  Buffer,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PDEVICE_OBJECT                   DeviceObject
    )
{
    BOOLEAN     Status = FALSE;
    PFSD_FCB    Fcb;
    BOOLEAN     FcbMainResourceAcquired = FALSE;

    PAGED_CODE();

    __try
    {
        __try
        {
            FsRtlEnterFileSystem();

            if (DeviceObject == FsdGlobalData.DeviceObject)
            {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                __leave;
            }

            Fcb = (PFSD_FCB) FileObject->FsContext;

            ASSERT(Fcb != NULL);

            if (Fcb->Identifier.Type == VCB)
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }

            ASSERT((Fcb->Identifier.Type == FCB) &&
                   (Fcb->Identifier.Size == sizeof(FSD_FCB)));

            KdPrint((
                DRIVER_NAME ": %-16.16s %-31s %s\n",
                FsdGetCurrentProcessName(),
                "FASTIO_QUERY_NETWORK_OPEN_INFO",
                Fcb->AnsiFileName.Buffer
                ));

            if (!ExAcquireResourceSharedLite(
                    &Fcb->MainResource,
                    Wait
                    ))
            {
                Status = FALSE;
                __leave;
            }

            FcbMainResourceAcquired = TRUE;

            RtlZeroMemory(Buffer, sizeof(FILE_NETWORK_OPEN_INFORMATION));

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

            IoStatus->Information = sizeof(FILE_NETWORK_OPEN_INFORMATION);

            IoStatus->Status = STATUS_SUCCESS;

            Status =  TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            IoStatus->Status = GetExceptionCode();
            Status = TRUE;
        }
    }
    __finally
    {
        if (FcbMainResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }

        FsRtlExitFileSystem();
    }

    if (Status == FALSE)
    {
        KdPrint((
            DRIVER_NAME ": %-16.16s %-31s *** Status: FALSE ***\n",
            FsdGetCurrentProcessName(),
            "FASTIO_QUERY_NETWORK_OPEN_INFO"
            ));
    }
    else if (IoStatus->Status != STATUS_SUCCESS)
    {
        KdPrint((
            DRIVER_NAME ": %-16.16s %-31s *** Status: %s (%#x) ***\n",
            FsdGetCurrentProcessName(),
            "FASTIO_QUERY_NETWORK_OPEN_INFO",
            FsdNtStatusToString(IoStatus->Status),
            IoStatus->Status
            ));
    }

    return Status;
}

#pragma code_seg() // end FSD_PAGED_CODE
