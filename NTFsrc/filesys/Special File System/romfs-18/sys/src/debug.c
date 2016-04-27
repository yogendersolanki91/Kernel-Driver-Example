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

#if DBG

static PUCHAR IrpMjStrings[] = {
    "CREATE",
    "CREATE_NAMED_PIPE",
    "CLOSE",
    "READ",
    "WRITE",
    "QUERY_INFORMATION",
    "SET_INFORMATION",
    "QUERY_EA",
    "SET_EA",
    "FLUSH_BUFFERS",
    "QUERY_VOLUME_INFORMATION",
    "SET_VOLUME_INFORMATION",
    "DIRECTORY_CONTROL",
    "FILE_SYSTEM_CONTROL",
    "DEVICE_CONTROL",
    "INTERNAL_DEVICE_CONTROL",
    "SHUTDOWN",
    "LOCK_CONTROL",
    "CLEANUP",
    "CREATE_MAILSLOT",
    "QUERY_SECURITY",
    "SET_SECURITY",
    "POWER",
    "SYSTEM_CONTROL",
    "DEVICE_CHANGE",
    "QUERY_QUOTA",
    "SET_QUOTA",
    "PNP"
};

static PUCHAR FileInformationClassStrings[] = {
    "Unknown FileInformationClass 0",
    "Directory",
    "FullDirectory",
    "BothDirectory",
    "Basic",
    "Standard",
    "Internal",
    "Ea",
    "Access",
    "Name",
    "Rename",
    "Link",
    "Names",
    "Disposition",
    "Position",
    "FullEa",
    "Mode",
    "Alignment",
    "All",
    "Allocation",
    "EndOfFile",
    "AlternateName",
    "Stream",
    "Pipe",
    "PipeLocal",
    "PipeRemote",
    "MailslotQuery",
    "MailslotSet",
    "Compression",
    "ObjectId",
    "Completion",
    "MoveCluster",
    "Quota",
    "ReparsePoint",
    "NetworkOpen",
    "AttributeTag",
    "Tracking",
    "IdBothDirectory",
    "IdFullDirectory",
    "ValidDataLength",
    "ShortName"
};

static PUCHAR FsInformationClassStrings[] = {
    "Unknown FsInformationClass 0",
    "Volume",
    "Label",
    "Size",
    "Device",
    "Attribute",
    "Control",
    "FullSize",
    "ObjectId",
    "DriverPath"
};

#define SYSTEM_PROCESS_NAME "System"

ULONG ProcessNameOffset;

ULONG 
FsdGetProcessNameOffset (
    VOID
    )
{
    PEPROCESS   Process;
    ULONG       i;

    Process = PsGetCurrentProcess();

    for(i = 0; i < PAGE_SIZE; i++)
    {
        if(!strncmp(
            SYSTEM_PROCESS_NAME,
            (PCHAR) Process + i,
            strlen(SYSTEM_PROCESS_NAME)
            ))
        {
            return i;
        }
    }

    KdPrint((DRIVER_NAME ": *** FsdGetProcessNameOffset failed ***\n"));

    return 0;
}

VOID
FsdDbgPrintCall (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PIO_STACK_LOCATION      IrpSp;
    PFILE_OBJECT            FileObject;
    PUCHAR                  FileName;
    PFSD_FCB                Fcb;
    FILE_INFORMATION_CLASS  FileInformationClass;
    FS_INFORMATION_CLASS    FsInformationClass;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    FileObject = IrpSp->FileObject;

    FileName = "Unknown";

    if (DeviceObject == FsdGlobalData.DeviceObject)
    {
        FileName = "\\" DRIVER_NAME;
    }
    else if (FileObject && FileObject->FsContext)
    {
        Fcb = (PFSD_FCB) FileObject->FsContext;

        if (Fcb->Identifier.Type == VCB)
        {
            FileName = "\\Volume";
        }
        else if (Fcb->Identifier.Type == FCB && Fcb->AnsiFileName.Buffer)
        {
            FileName = Fcb->AnsiFileName.Buffer;
        }
    }

    switch (IrpSp->MajorFunction)
    {
    case IRP_MJ_CREATE:

        FileName = NULL;

        if (DeviceObject == FsdGlobalData.DeviceObject)
        {
            FileName = "\\" DRIVER_NAME;
        }
        else if (FileObject && FileObject->FileName.Length == 0)
        {
            FileName = "\\Volume";
        }

        if (FileName)
        {
            DbgPrint(
                DRIVER_NAME ": %-16.16s %-31s %s\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName
            );
        }
        else if (FileObject && FileObject->FileName.Buffer)
        {
            DbgPrint(
                DRIVER_NAME ": %-16.16s %-31s %S\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileObject->FileName.Buffer
            );
        }
        else
        {
            DbgPrint(
                DRIVER_NAME ": %-16.16s %-31s %s\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                "Unknown"
            );
        }

        break;

    case IRP_MJ_CLOSE:

        DbgPrint(
            DRIVER_NAME ": %-16.16s %-31s %s\n",
            FsdGetCurrentProcessName(),
            IrpMjStrings[IrpSp->MajorFunction],
            FileName
            );

        break;

    case IRP_MJ_READ:

        if (IrpSp->MinorFunction & IRP_MN_COMPLETE)
        {
            DbgPrint(
                DRIVER_NAME ": %-16.16s %-31s %s IRP_MN_COMPLETE\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName
                );
        }
        else
        {
            DbgPrint(
                DRIVER_NAME ": %-16.16s %-31s %s\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName
                );

            DbgPrint(
                DRIVER_NAME ": %-16.16s Offset: %I64u Length: %u Key: %u %s%s%s%s%s%s\n",
                FsdGetCurrentProcessName(),
                IrpSp->Parameters.Read.ByteOffset.QuadPart,
                IrpSp->Parameters.Read.Length,
                IrpSp->Parameters.Read.Key,
                (IrpSp->MinorFunction & IRP_MN_DPC ? "IRP_MN_DPC " : ""),
                (IrpSp->MinorFunction & IRP_MN_MDL ? "IRP_MN_MDL " : ""),
                (IrpSp->MinorFunction & IRP_MN_COMPRESSED ? "IRP_MN_COMPRESSED " : ""),
                (Irp->Flags & IRP_PAGING_IO ? "IRP_PAGING_IO " : ""),
                (Irp->Flags & IRP_NOCACHE ? "IRP_NOCACHE " : ""),
                (FileObject->Flags & FO_SYNCHRONOUS_IO ? "FO_SYNCHRONOUS_IO " : "")
                );
        }

        break;

    case IRP_MJ_QUERY_INFORMATION:

        FileInformationClass =
            IrpSp->Parameters.QueryFile.FileInformationClass;

        if (FileInformationClass <= FileMaximumInformation)
        {
            DbgPrint(
                DRIVER_NAME ": %-16.16s %-31s %s %s\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName,
                FileInformationClassStrings[FileInformationClass]
                );
        }
        else
        {
            DbgPrint(
                DRIVER_NAME
                ": %-16.16s %-31s %s Unknown FileInformationClass %u\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName,
                FileInformationClass
                );
        }

        break;

    case IRP_MJ_SET_INFORMATION:

        FileInformationClass =
            IrpSp->Parameters.SetFile.FileInformationClass;

        if (FileInformationClass <= FileMaximumInformation)
        {
            DbgPrint(
                DRIVER_NAME ": %-16.16s %-31s %s %s\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName,
                FileInformationClassStrings[FileInformationClass]
                );
        }
        else
        {
            DbgPrint(
                DRIVER_NAME
                ": %-16.16s %-31s %s Unknown FileInformationClass %u\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName,
                FileInformationClass
                );
        }

        break;

    case IRP_MJ_QUERY_VOLUME_INFORMATION:

        FsInformationClass =
            IrpSp->Parameters.QueryVolume.FsInformationClass;

        if (FsInformationClass <= FileFsMaximumInformation)
        {
            DbgPrint(
                DRIVER_NAME ": %-16.16s %-31s %s %s\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName,
                FsInformationClassStrings[FsInformationClass]
                );
        }
        else
        {
            DbgPrint(
                DRIVER_NAME
                ": %-16.16s %-31s %s Unknown FsInformationClass %u\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName,
                FsInformationClass
                );
        }

        break;

    case IRP_MJ_DIRECTORY_CONTROL:

        if (IrpSp->MinorFunction & IRP_MN_QUERY_DIRECTORY)
        {
#ifndef _GNU_NTIFS_
            FileInformationClass =
                IrpSp->Parameters.QueryDirectory.FileInformationClass;
#else
            FileInformationClass = ((PEXTENDED_IO_STACK_LOCATION)
                IrpSp)->Parameters.QueryDirectory.FileInformationClass;
#endif

            if (FileInformationClass <= FileMaximumInformation)
            {
                DbgPrint(
                    DRIVER_NAME ": %-16.16s %-31s %s %s\n",
                    FsdGetCurrentProcessName(),
                    IrpMjStrings[IrpSp->MajorFunction],
                    FileName,
                    FileInformationClassStrings[FileInformationClass]
                    );

                if (
#ifndef _GNU_NTIFS_
                    IrpSp->Parameters.QueryDirectory.FileName
#else
                    ((PEXTENDED_IO_STACK_LOCATION)
                    IrpSp)->Parameters.QueryDirectory.FileName
#endif
                    )
                {
                    DbgPrint(
                        DRIVER_NAME ": %-16.16s FileName: %.*S FileIndex: %#x %s%s%s\n",
                        FsdGetCurrentProcessName(),
#ifndef _GNU_NTIFS_
                        IrpSp->Parameters.QueryDirectory.FileName->Length / 2,
                        IrpSp->Parameters.QueryDirectory.FileName->Buffer,
                        IrpSp->Parameters.QueryDirectory.FileIndex,
#else
                        ((PEXTENDED_IO_STACK_LOCATION)
                        IrpSp)->Parameters.QueryDirectory.FileName->Length / 2,
                        ((PEXTENDED_IO_STACK_LOCATION)
                        IrpSp)->Parameters.QueryDirectory.FileName->Buffer,
                        ((PEXTENDED_IO_STACK_LOCATION)
                        IrpSp)->Parameters.QueryDirectory.FileIndex,
#endif
                        (IrpSp->Flags & SL_RESTART_SCAN ? "SL_RESTART_SCAN " : ""),
                        (IrpSp->Flags & SL_RETURN_SINGLE_ENTRY ? "SL_RETURN_SINGLE_ENTRY " : ""),
                        (IrpSp->Flags & SL_INDEX_SPECIFIED ? "SL_INDEX_SPECIFIED " : "")
                        );
                }
                else
                {
                    DbgPrint(
                        DRIVER_NAME ": %-16.16s FileName: FileIndex: %#x %s%s%s\n",
                        FsdGetCurrentProcessName(),
#ifndef _GNU_NTIFS_
                        IrpSp->Parameters.QueryDirectory.FileIndex,
#else
                        ((PEXTENDED_IO_STACK_LOCATION)
                        IrpSp)->Parameters.QueryDirectory.FileIndex,
#endif
                        (IrpSp->Flags & SL_RESTART_SCAN ? "SL_RESTART_SCAN " : ""),
                        (IrpSp->Flags & SL_RETURN_SINGLE_ENTRY ? "SL_RETURN_SINGLE_ENTRY " : ""),
                        (IrpSp->Flags & SL_INDEX_SPECIFIED ? "SL_INDEX_SPECIFIED " : "")
                        );
                }
            }
            else
            {
                DbgPrint(
                    DRIVER_NAME
                    ": %-16.16s %-31s %s Unknown FileInformationClass %u\n",
                    FsdGetCurrentProcessName(),
                    IrpMjStrings[IrpSp->MajorFunction],
                    FileName,
                    FileInformationClass
                    );
            }
        }
        else if (IrpSp->MinorFunction & IRP_MN_NOTIFY_CHANGE_DIRECTORY)
        {
            DbgPrint(
                DRIVER_NAME ": %-16.16s %-31s %s IRP_MN_NOTIFY_CHANGE_DIRECTORY\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName
                );

            DbgPrint(
                DRIVER_NAME ": CompletionFilter: %#x %s\n",
#ifndef _GNU_NTIFS_
                IrpSp->Parameters.NotifyDirectory.CompletionFilter,
#else
                ((PEXTENDED_IO_STACK_LOCATION)
                IrpSp)->Parameters.NotifyDirectory.CompletionFilter,
#endif
                (IrpSp->Flags & SL_WATCH_TREE ? "SL_WATCH_TREE " : "")
                );
        }
        else
        {
            DbgPrint(
                DRIVER_NAME ": %-16.16s %-31s %s Unknown minor function %#x\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName,
                IrpSp->MinorFunction
                );
        }

        break;

    case IRP_MJ_FILE_SYSTEM_CONTROL:

        if (IrpSp->MinorFunction == IRP_MN_USER_FS_REQUEST)
        {
            DbgPrint(
                DRIVER_NAME
                ": %-16.16s %-31s %s IRP_MN_USER_FS_REQUEST FsControlCode: %#x\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName,
#ifndef _GNU_NTIFS_
                IrpSp->Parameters.FileSystemControl.FsControlCode
#else
                ((PEXTENDED_IO_STACK_LOCATION)
                IrpSp)->Parameters.FileSystemControl.FsControlCode
#endif
                );
        }
        else if (IrpSp->MinorFunction == IRP_MN_MOUNT_VOLUME)
        {
            DbgPrint(
                DRIVER_NAME
                ": %-16.16s %-31s %s IRP_MN_MOUNT_VOLUME DeviceObject: %#x\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName,
                IrpSp->Parameters.MountVolume.DeviceObject
                );
        }
        else if (IrpSp->MinorFunction == IRP_MN_VERIFY_VOLUME)
        {
            DbgPrint(
                DRIVER_NAME
                ": %-16.16s %-31s %s IRP_MN_VERIFY_VOLUME DeviceObject: %#x\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName,
                IrpSp->Parameters.VerifyVolume.DeviceObject
                );
        }
        else if (IrpSp->MinorFunction == IRP_MN_LOAD_FILE_SYSTEM)
        {
            DbgPrint(
                DRIVER_NAME ": %-16.16s %-31s %s IRP_MN_LOAD_FILE_SYSTEM\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName
                );
        }
        else
        {
            DbgPrint(
                DRIVER_NAME ": %-16.16s %-31s %s Unknown minor function %#x\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName,
                IrpSp->MinorFunction
                );
        }

        break;

    case IRP_MJ_DEVICE_CONTROL:

        DbgPrint(
            DRIVER_NAME ": %-16.16s %-31s %s IoControlCode: %#x\n",
            FsdGetCurrentProcessName(),
            IrpMjStrings[IrpSp->MajorFunction],
            FileName,
            IrpSp->Parameters.DeviceIoControl.IoControlCode
            );

        break;

    case IRP_MJ_LOCK_CONTROL:

        if (IrpSp->MinorFunction & IRP_MN_LOCK)
        {
            DbgPrint(
                DRIVER_NAME ": %-16.16s %-31s %s IRP_MN_LOCK Offset: %I64u Length: %I64u Key: %u %s%s\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName,
#ifndef _GNU_NTIFS_
                IrpSp->Parameters.LockControl.ByteOffset.QuadPart,
                IrpSp->Parameters.LockControl.Length->QuadPart,
                IrpSp->Parameters.LockControl.Key,
#else
                ((PEXTENDED_IO_STACK_LOCATION)
                IrpSp)->Parameters.LockControl.ByteOffset.QuadPart,
                ((PEXTENDED_IO_STACK_LOCATION)
                IrpSp)->Parameters.LockControl.Length->QuadPart,
                ((PEXTENDED_IO_STACK_LOCATION)
                IrpSp)->Parameters.LockControl.Key,
#endif
                (IrpSp->Flags & SL_FAIL_IMMEDIATELY ? "SL_FAIL_IMMEDIATELY " : ""),
                (IrpSp->Flags & SL_EXCLUSIVE_LOCK ? "SL_EXCLUSIVE_LOCK " : "")
                );
        }
        else if (IrpSp->MinorFunction & IRP_MN_UNLOCK_SINGLE)
        {
            DbgPrint(
                DRIVER_NAME ": %-16.16s %-31s %s IRP_MN_UNLOCK_SINGLE Offset: %I64u Length: %I64u Key: %u\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName,
#ifndef _GNU_NTIFS_
                IrpSp->Parameters.LockControl.ByteOffset.QuadPart,
                IrpSp->Parameters.LockControl.Length->QuadPart,
                IrpSp->Parameters.LockControl.Key
#else
                ((PEXTENDED_IO_STACK_LOCATION)
                IrpSp)->Parameters.LockControl.ByteOffset.QuadPart,
                ((PEXTENDED_IO_STACK_LOCATION)
                IrpSp)->Parameters.LockControl.Length->QuadPart,
                ((PEXTENDED_IO_STACK_LOCATION)
                IrpSp)->Parameters.LockControl.Key
#endif
                );
        }
        else if (IrpSp->MinorFunction & IRP_MN_UNLOCK_ALL)
        {
            DbgPrint(
                DRIVER_NAME ": %-16.16s %-31s %s IRP_MN_UNLOCK_ALL\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName
                );
        }
        else if (IrpSp->MinorFunction & IRP_MN_UNLOCK_ALL_BY_KEY)
        {
            DbgPrint(
                DRIVER_NAME ": %-16.16s %-31s %s IRP_MN_UNLOCK_ALL_BY_KEY Key: %u\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName,
#ifndef _GNU_NTIFS_
                IrpSp->Parameters.LockControl.Key
#else
                ((PEXTENDED_IO_STACK_LOCATION)
                IrpSp)->Parameters.LockControl.Key
#endif
                );
        }
        else
        {
            DbgPrint(
                DRIVER_NAME ": %-16.16s %-31s %s Unknown minor function %#x\n",
                FsdGetCurrentProcessName(),
                IrpMjStrings[IrpSp->MajorFunction],
                FileName,
                IrpSp->MinorFunction
                );
        }

        break;

    case IRP_MJ_CLEANUP:

        DbgPrint(
            DRIVER_NAME ": %-16.16s %-31s %s\n",
            FsdGetCurrentProcessName(),
            IrpMjStrings[IrpSp->MajorFunction],
            FileName
            );

        break;

    default:

        DbgPrint(
            DRIVER_NAME ": %-16.16s %-31s %s\n",
            FsdGetCurrentProcessName(),
            IrpMjStrings[IrpSp->MajorFunction],
            FileName
            );
    }
}

VOID
FsdDbgPrintComplete (
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpSp;

    ASSERT(Irp != NULL);

    if (Irp->IoStatus.Status != STATUS_SUCCESS)
    {
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        DbgPrint(
            DRIVER_NAME ": %-16.16s %-31s *** Status: %s (%#x) ***\n",
            FsdGetCurrentProcessName(),
            IrpMjStrings[IrpSp->MajorFunction],
            FsdNtStatusToString(Irp->IoStatus.Status),
            Irp->IoStatus.Status
            );
    }
}

PUCHAR
FsdNtStatusToString (
    IN NTSTATUS Status
    )
{
    switch (Status)
    {
    case 0x00000000: return "STATUS_SUCCESS";
    case 0x00000001: return "STATUS_WAIT_1";
    case 0x00000002: return "STATUS_WAIT_2";
    case 0x00000003: return "STATUS_WAIT_3";
    case 0x0000003F: return "STATUS_WAIT_63";
    case 0x00000080: return "STATUS_ABANDONED_WAIT_0";
    case 0x000000BF: return "STATUS_ABANDONED_WAIT_63";
    case 0x000000C0: return "STATUS_USER_APC";
    case 0x00000100: return "STATUS_KERNEL_APC";
    case 0x00000101: return "STATUS_ALERTED";
    case 0x00000102: return "STATUS_TIMEOUT";
    case 0x00000103: return "STATUS_PENDING";
    case 0x00000104: return "STATUS_REPARSE";
    case 0x00000105: return "STATUS_MORE_ENTRIES";
    case 0x00000106: return "STATUS_NOT_ALL_ASSIGNED";
    case 0x00000107: return "STATUS_SOME_NOT_MAPPED";
    case 0x00000108: return "STATUS_OPLOCK_BREAK_IN_PROGRESS";
    case 0x00000109: return "STATUS_VOLUME_MOUNTED";
    case 0x0000010A: return "STATUS_RXACT_COMMITTED";
    case 0x0000010B: return "STATUS_NOTIFY_CLEANUP";
    case 0x0000010C: return "STATUS_NOTIFY_ENUM_DIR";
    case 0x0000010D: return "STATUS_NO_QUOTAS_FOR_ACCOUNT";
    case 0x0000010E: return "STATUS_PRIMARY_TRANSPORT_CONNECT_FAILED";
    case 0x00000110: return "STATUS_PAGE_FAULT_TRANSITION";
    case 0x00000111: return "STATUS_PAGE_FAULT_DEMAND_ZERO";
    case 0x00000112: return "STATUS_PAGE_FAULT_COPY_ON_WRITE";
    case 0x00000113: return "STATUS_PAGE_FAULT_GUARD_PAGE";
    case 0x00000114: return "STATUS_PAGE_FAULT_PAGING_FILE";
    case 0x00000115: return "STATUS_CACHE_PAGE_LOCKED";
    case 0x00000116: return "STATUS_CRASH_DUMP";
    case 0x00000117: return "STATUS_BUFFER_ALL_ZEROS";
    case 0x00000118: return "STATUS_REPARSE_OBJECT";
    case 0x00000119: return "STATUS_RESOURCE_REQUIREMENTS_CHANGED";
    case 0x00000120: return "STATUS_TRANSLATION_COMPLETE";
    case 0x00000121: return "STATUS_DS_MEMBERSHIP_EVALUATED_LOCALLY";
    case 0x00000122: return "STATUS_NOTHING_TO_TERMINATE";
    case 0x00000123: return "STATUS_PROCESS_NOT_IN_JOB";
    case 0x00000124: return "STATUS_PROCESS_IN_JOB";
    case 0x40000000: return "STATUS_OBJECT_NAME_EXISTS";
    case 0x40000001: return "STATUS_THREAD_WAS_SUSPENDED";
    case 0x40000002: return "STATUS_WORKING_SET_LIMIT_RANGE";
    case 0x40000003: return "STATUS_IMAGE_NOT_AT_BASE";
    case 0x40000004: return "STATUS_RXACT_STATE_CREATED";
    case 0x40000005: return "STATUS_SEGMENT_NOTIFICATION";
    case 0x40000006: return "STATUS_LOCAL_USER_SESSION_KEY";
    case 0x40000007: return "STATUS_BAD_CURRENT_DIRECTORY";
    case 0x40000008: return "STATUS_SERIAL_MORE_WRITES";
    case 0x40000009: return "STATUS_REGISTRY_RECOVERED";
    case 0x4000000A: return "STATUS_FT_READ_RECOVERY_FROM_BACKUP";
    case 0x4000000B: return "STATUS_FT_WRITE_RECOVERY";
    case 0x4000000C: return "STATUS_SERIAL_COUNTER_TIMEOUT";
    case 0x4000000D: return "STATUS_NULL_LM_PASSWORD";
    case 0x4000000E: return "STATUS_IMAGE_MACHINE_TYPE_MISMATCH";
    case 0x4000000F: return "STATUS_RECEIVE_PARTIAL";
    case 0x40000010: return "STATUS_RECEIVE_EXPEDITED";
    case 0x40000011: return "STATUS_RECEIVE_PARTIAL_EXPEDITED";
    case 0x40000012: return "STATUS_EVENT_DONE";
    case 0x40000013: return "STATUS_EVENT_PENDING";
    case 0x40000014: return "STATUS_CHECKING_FILE_SYSTEM";
    case 0x40000015: return "STATUS_FATAL_APP_EXIT";
    case 0x40000016: return "STATUS_PREDEFINED_HANDLE";
    case 0x40000017: return "STATUS_WAS_UNLOCKED";
    case 0x40000018: return "STATUS_SERVICE_NOTIFICATION";
    case 0x40000019: return "STATUS_WAS_LOCKED";
    case 0x4000001A: return "STATUS_LOG_HARD_ERROR";
    case 0x4000001B: return "STATUS_ALREADY_WIN32";
    case 0x4000001C: return "STATUS_WX86_UNSIMULATE";
    case 0x4000001D: return "STATUS_WX86_CONTINUE";
    case 0x4000001E: return "STATUS_WX86_SINGLE_STEP";
    case 0x4000001F: return "STATUS_WX86_BREAKPOINT";
    case 0x40000020: return "STATUS_WX86_EXCEPTION_CONTINUE";
    case 0x40000021: return "STATUS_WX86_EXCEPTION_LASTCHANCE";
    case 0x40000022: return "STATUS_WX86_EXCEPTION_CHAIN";
    case 0x40000023: return "STATUS_IMAGE_MACHINE_TYPE_MISMATCH_EXE";
    case 0x40000024: return "STATUS_NO_YIELD_PERFORMED";
    case 0x40000025: return "STATUS_TIMER_RESUME_IGNORED";
    case 0x40000026: return "STATUS_ARBITRATION_UNHANDLED";
    case 0x40000027: return "STATUS_CARDBUS_NOT_SUPPORTED";
    case 0x40000028: return "STATUS_WX86_CREATEWX86TIB";
    case 0x40000029: return "STATUS_MP_PROCESSOR_MISMATCH";
    case 0x4000002A: return "STATUS_HIBERNATED";
    case 0x4000002B: return "STATUS_RESUME_HIBERNATION";
    case 0x80000001: return "STATUS_GUARD_PAGE_VIOLATION";
    case 0x80000002: return "STATUS_DATATYPE_MISALIGNMENT";
    case 0x80000003: return "STATUS_BREAKPOINT";
    case 0x80000004: return "STATUS_SINGLE_STEP";
    case 0x80000005: return "STATUS_BUFFER_OVERFLOW";
    case 0x80000006: return "STATUS_NO_MORE_FILES";
    case 0x80000007: return "STATUS_WAKE_SYSTEM_DEBUGGER";
    case 0x8000000A: return "STATUS_HANDLES_CLOSED";
    case 0x8000000B: return "STATUS_NO_INHERITANCE";
    case 0x8000000C: return "STATUS_GUID_SUBSTITUTION_MADE";
    case 0x8000000D: return "STATUS_PARTIAL_COPY";
    case 0x8000000E: return "STATUS_DEVICE_PAPER_EMPTY";
    case 0x8000000F: return "STATUS_DEVICE_POWERED_OFF";
    case 0x80000010: return "STATUS_DEVICE_OFF_LINE";
    case 0x80000011: return "STATUS_DEVICE_BUSY";
    case 0x80000012: return "STATUS_NO_MORE_EAS";
    case 0x80000013: return "STATUS_INVALID_EA_NAME";
    case 0x80000014: return "STATUS_EA_LIST_INCONSISTENT";
    case 0x80000015: return "STATUS_INVALID_EA_FLAG";
    case 0x80000016: return "STATUS_VERIFY_REQUIRED";
    case 0x80000017: return "STATUS_EXTRANEOUS_INFORMATION";
    case 0x80000018: return "STATUS_RXACT_COMMIT_NECESSARY";
    case 0x8000001A: return "STATUS_NO_MORE_ENTRIES";
    case 0x8000001B: return "STATUS_FILEMARK_DETECTED";
    case 0x8000001C: return "STATUS_MEDIA_CHANGED";
    case 0x8000001D: return "STATUS_BUS_RESET";
    case 0x8000001E: return "STATUS_END_OF_MEDIA";
    case 0x8000001F: return "STATUS_BEGINNING_OF_MEDIA";
    case 0x80000020: return "STATUS_MEDIA_CHECK";
    case 0x80000021: return "STATUS_SETMARK_DETECTED";
    case 0x80000022: return "STATUS_NO_DATA_DETECTED";
    case 0x80000023: return "STATUS_REDIRECTOR_HAS_OPEN_HANDLES";
    case 0x80000024: return "STATUS_SERVER_HAS_OPEN_HANDLES";
    case 0x80000025: return "STATUS_ALREADY_DISCONNECTED";
    case 0x80000026: return "STATUS_LONGJUMP";
    case 0x80000027: return "STATUS_CLEANER_CARTRIDGE_INSTALLED";
    case 0x80000028: return "STATUS_PLUGPLAY_QUERY_VETOED";
    case 0x80000029: return "STATUS_UNWIND_CONSOLIDATE";
    case 0x80130001: return "STATUS_CLUSTER_NODE_ALREADY_UP";
    case 0x80130002: return "STATUS_CLUSTER_NODE_ALREADY_DOWN";
    case 0x80130003: return "STATUS_CLUSTER_NETWORK_ALREADY_ONLINE";
    case 0x80130004: return "STATUS_CLUSTER_NETWORK_ALREADY_OFFLINE";
    case 0x80130005: return "STATUS_CLUSTER_NODE_ALREADY_MEMBER";
    case 0xC0000001: return "STATUS_UNSUCCESSFUL";
    case 0xC0000002: return "STATUS_NOT_IMPLEMENTED";
    case 0xC0000003: return "STATUS_INVALID_INFO_CLASS";
    case 0xC0000004: return "STATUS_INFO_LENGTH_MISMATCH";
    case 0xC0000005: return "STATUS_ACCESS_VIOLATION";
    case 0xC0000006: return "STATUS_IN_PAGE_ERROR";
    case 0xC0000007: return "STATUS_PAGEFILE_QUOTA";
    case 0xC0000008: return "STATUS_INVALID_HANDLE";
    case 0xC0000009: return "STATUS_BAD_INITIAL_STACK";
    case 0xC000000A: return "STATUS_BAD_INITIAL_PC";
    case 0xC000000B: return "STATUS_INVALID_CID";
    case 0xC000000C: return "STATUS_TIMER_NOT_CANCELED";
    case 0xC000000D: return "STATUS_INVALID_PARAMETER";
    case 0xC000000E: return "STATUS_NO_SUCH_DEVICE";
    case 0xC000000F: return "STATUS_NO_SUCH_FILE";
    case 0xC0000010: return "STATUS_INVALID_DEVICE_REQUEST";
    case 0xC0000011: return "STATUS_END_OF_FILE";
    case 0xC0000012: return "STATUS_WRONG_VOLUME";
    case 0xC0000013: return "STATUS_NO_MEDIA_IN_DEVICE";
    case 0xC0000014: return "STATUS_UNRECOGNIZED_MEDIA";
    case 0xC0000015: return "STATUS_NONEXISTENT_SECTOR";
    case 0xC0000016: return "STATUS_MORE_PROCESSING_REQUIRED";
    case 0xC0000017: return "STATUS_NO_MEMORY";
    case 0xC0000018: return "STATUS_CONFLICTING_ADDRESSES";
    case 0xC0000019: return "STATUS_NOT_MAPPED_VIEW";
    case 0xC000001A: return "STATUS_UNABLE_TO_FREE_VM";
    case 0xC000001B: return "STATUS_UNABLE_TO_DELETE_SECTION";
    case 0xC000001C: return "STATUS_INVALID_SYSTEM_SERVICE";
    case 0xC000001D: return "STATUS_ILLEGAL_INSTRUCTION";
    case 0xC000001E: return "STATUS_INVALID_LOCK_SEQUENCE";
    case 0xC000001F: return "STATUS_INVALID_VIEW_SIZE";
    case 0xC0000020: return "STATUS_INVALID_FILE_FOR_SECTION";
    case 0xC0000021: return "STATUS_ALREADY_COMMITTED";
    case 0xC0000022: return "STATUS_ACCESS_DENIED";
    case 0xC0000023: return "STATUS_BUFFER_TOO_SMALL";
    case 0xC0000024: return "STATUS_OBJECT_TYPE_MISMATCH";
    case 0xC0000025: return "STATUS_NONCONTINUABLE_EXCEPTION";
    case 0xC0000026: return "STATUS_INVALID_DISPOSITION";
    case 0xC0000027: return "STATUS_UNWIND";
    case 0xC0000028: return "STATUS_BAD_STACK";
    case 0xC0000029: return "STATUS_INVALID_UNWIND_TARGET";
    case 0xC000002A: return "STATUS_NOT_LOCKED";
    case 0xC000002B: return "STATUS_PARITY_ERROR";
    case 0xC000002C: return "STATUS_UNABLE_TO_DECOMMIT_VM";
    case 0xC000002D: return "STATUS_NOT_COMMITTED";
    case 0xC000002E: return "STATUS_INVALID_PORT_ATTRIBUTES";
    case 0xC000002F: return "STATUS_PORT_MESSAGE_TOO_LONG";
    case 0xC0000030: return "STATUS_INVALID_PARAMETER_MIX";
    case 0xC0000031: return "STATUS_INVALID_QUOTA_LOWER";
    case 0xC0000032: return "STATUS_DISK_CORRUPT_ERROR";
    case 0xC0000033: return "STATUS_OBJECT_NAME_INVALID";
    case 0xC0000034: return "STATUS_OBJECT_NAME_NOT_FOUND";
    case 0xC0000035: return "STATUS_OBJECT_NAME_COLLISION";
    case 0xC0000037: return "STATUS_PORT_DISCONNECTED";
    case 0xC0000038: return "STATUS_DEVICE_ALREADY_ATTACHED";
    case 0xC0000039: return "STATUS_OBJECT_PATH_INVALID";
    case 0xC000003A: return "STATUS_OBJECT_PATH_NOT_FOUND";
    case 0xC000003B: return "STATUS_OBJECT_PATH_SYNTAX_BAD";
    case 0xC000003C: return "STATUS_DATA_OVERRUN";
    case 0xC000003D: return "STATUS_DATA_LATE_ERROR";
    case 0xC000003E: return "STATUS_DATA_ERROR";
    case 0xC000003F: return "STATUS_CRC_ERROR";
    case 0xC0000040: return "STATUS_SECTION_TOO_BIG";
    case 0xC0000041: return "STATUS_PORT_CONNECTION_REFUSED";
    case 0xC0000042: return "STATUS_INVALID_PORT_HANDLE";
    case 0xC0000043: return "STATUS_SHARING_VIOLATION";
    case 0xC0000044: return "STATUS_QUOTA_EXCEEDED";
    case 0xC0000045: return "STATUS_INVALID_PAGE_PROTECTION";
    case 0xC0000046: return "STATUS_MUTANT_NOT_OWNED";
    case 0xC0000047: return "STATUS_SEMAPHORE_LIMIT_EXCEEDED";
    case 0xC0000048: return "STATUS_PORT_ALREADY_SET";
    case 0xC0000049: return "STATUS_SECTION_NOT_IMAGE";
    case 0xC000004A: return "STATUS_SUSPEND_COUNT_EXCEEDED";
    case 0xC000004B: return "STATUS_THREAD_IS_TERMINATING";
    case 0xC000004C: return "STATUS_BAD_WORKING_SET_LIMIT";
    case 0xC000004D: return "STATUS_INCOMPATIBLE_FILE_MAP";
    case 0xC000004E: return "STATUS_SECTION_PROTECTION";
    case 0xC000004F: return "STATUS_EAS_NOT_SUPPORTED";
    case 0xC0000050: return "STATUS_EA_TOO_LARGE";
    case 0xC0000051: return "STATUS_NONEXISTENT_EA_ENTRY";
    case 0xC0000052: return "STATUS_NO_EAS_ON_FILE";
    case 0xC0000053: return "STATUS_EA_CORRUPT_ERROR";
    case 0xC0000054: return "STATUS_FILE_LOCK_CONFLICT";
    case 0xC0000055: return "STATUS_LOCK_NOT_GRANTED";
    case 0xC0000056: return "STATUS_DELETE_PENDING";
    case 0xC0000057: return "STATUS_CTL_FILE_NOT_SUPPORTED";
    case 0xC0000058: return "STATUS_UNKNOWN_REVISION";
    case 0xC0000059: return "STATUS_REVISION_MISMATCH";
    case 0xC000005A: return "STATUS_INVALID_OWNER";
    case 0xC000005B: return "STATUS_INVALID_PRIMARY_GROUP";
    case 0xC000005C: return "STATUS_NO_IMPERSONATION_TOKEN";
    case 0xC000005D: return "STATUS_CANT_DISABLE_MANDATORY";
    case 0xC000005E: return "STATUS_NO_LOGON_SERVERS";
    case 0xC000005F: return "STATUS_NO_SUCH_LOGON_SESSION";
    case 0xC0000060: return "STATUS_NO_SUCH_PRIVILEGE";
    case 0xC0000061: return "STATUS_PRIVILEGE_NOT_HELD";
    case 0xC0000062: return "STATUS_INVALID_ACCOUNT_NAME";
    case 0xC0000063: return "STATUS_USER_EXISTS";
    case 0xC0000064: return "STATUS_NO_SUCH_USER";
    case 0xC0000065: return "STATUS_GROUP_EXISTS";
    case 0xC0000066: return "STATUS_NO_SUCH_GROUP";
    case 0xC0000067: return "STATUS_MEMBER_IN_GROUP";
    case 0xC0000068: return "STATUS_MEMBER_NOT_IN_GROUP";
    case 0xC0000069: return "STATUS_LAST_ADMIN";
    case 0xC000006A: return "STATUS_WRONG_PASSWORD";
    case 0xC000006B: return "STATUS_ILL_FORMED_PASSWORD";
    case 0xC000006C: return "STATUS_PASSWORD_RESTRICTION";
    case 0xC000006D: return "STATUS_LOGON_FAILURE";
    case 0xC000006E: return "STATUS_ACCOUNT_RESTRICTION";
    case 0xC000006F: return "STATUS_INVALID_LOGON_HOURS";
    case 0xC0000070: return "STATUS_INVALID_WORKSTATION";
    case 0xC0000071: return "STATUS_PASSWORD_EXPIRED";
    case 0xC0000072: return "STATUS_ACCOUNT_DISABLED";
    case 0xC0000073: return "STATUS_NONE_MAPPED";
    case 0xC0000074: return "STATUS_TOO_MANY_LUIDS_REQUESTED";
    case 0xC0000075: return "STATUS_LUIDS_EXHAUSTED";
    case 0xC0000076: return "STATUS_INVALID_SUB_AUTHORITY";
    case 0xC0000077: return "STATUS_INVALID_ACL";
    case 0xC0000078: return "STATUS_INVALID_SID";
    case 0xC0000079: return "STATUS_INVALID_SECURITY_DESCR";
    case 0xC000007A: return "STATUS_PROCEDURE_NOT_FOUND";
    case 0xC000007B: return "STATUS_INVALID_IMAGE_FORMAT";
    case 0xC000007C: return "STATUS_NO_TOKEN";
    case 0xC000007D: return "STATUS_BAD_INHERITANCE_ACL";
    case 0xC000007E: return "STATUS_RANGE_NOT_LOCKED";
    case 0xC000007F: return "STATUS_DISK_FULL";
    case 0xC0000080: return "STATUS_SERVER_DISABLED";
    case 0xC0000081: return "STATUS_SERVER_NOT_DISABLED";
    case 0xC0000082: return "STATUS_TOO_MANY_GUIDS_REQUESTED";
    case 0xC0000083: return "STATUS_GUIDS_EXHAUSTED";
    case 0xC0000084: return "STATUS_INVALID_ID_AUTHORITY";
    case 0xC0000085: return "STATUS_AGENTS_EXHAUSTED";
    case 0xC0000086: return "STATUS_INVALID_VOLUME_LABEL";
    case 0xC0000087: return "STATUS_SECTION_NOT_EXTENDED";
    case 0xC0000088: return "STATUS_NOT_MAPPED_DATA";
    case 0xC0000089: return "STATUS_RESOURCE_DATA_NOT_FOUND";
    case 0xC000008A: return "STATUS_RESOURCE_TYPE_NOT_FOUND";
    case 0xC000008B: return "STATUS_RESOURCE_NAME_NOT_FOUND";
    case 0xC000008C: return "STATUS_ARRAY_BOUNDS_EXCEEDED";
    case 0xC000008D: return "STATUS_FLOAT_DENORMAL_OPERAND";
    case 0xC000008E: return "STATUS_FLOAT_DIVIDE_BY_ZERO";
    case 0xC000008F: return "STATUS_FLOAT_INEXACT_RESULT";
    case 0xC0000090: return "STATUS_FLOAT_INVALID_OPERATION";
    case 0xC0000091: return "STATUS_FLOAT_OVERFLOW";
    case 0xC0000092: return "STATUS_FLOAT_STACK_CHECK";
    case 0xC0000093: return "STATUS_FLOAT_UNDERFLOW";
    case 0xC0000094: return "STATUS_INTEGER_DIVIDE_BY_ZERO";
    case 0xC0000095: return "STATUS_INTEGER_OVERFLOW";
    case 0xC0000096: return "STATUS_PRIVILEGED_INSTRUCTION";
    case 0xC0000097: return "STATUS_TOO_MANY_PAGING_FILES";
    case 0xC0000098: return "STATUS_FILE_INVALID";
    case 0xC0000099: return "STATUS_ALLOTTED_SPACE_EXCEEDED";
    case 0xC000009A: return "STATUS_INSUFFICIENT_RESOURCES";
    case 0xC000009B: return "STATUS_DFS_EXIT_PATH_FOUND";
    case 0xC000009C: return "STATUS_DEVICE_DATA_ERROR";
    case 0xC000009D: return "STATUS_DEVICE_NOT_CONNECTED";
    case 0xC000009E: return "STATUS_DEVICE_POWER_FAILURE";
    case 0xC000009F: return "STATUS_FREE_VM_NOT_AT_BASE";
    case 0xC00000A0: return "STATUS_MEMORY_NOT_ALLOCATED";
    case 0xC00000A1: return "STATUS_WORKING_SET_QUOTA";
    case 0xC00000A2: return "STATUS_MEDIA_WRITE_PROTECTED";
    case 0xC00000A3: return "STATUS_DEVICE_NOT_READY";
    case 0xC00000A4: return "STATUS_INVALID_GROUP_ATTRIBUTES";
    case 0xC00000A5: return "STATUS_BAD_IMPERSONATION_LEVEL";
    case 0xC00000A6: return "STATUS_CANT_OPEN_ANONYMOUS";
    case 0xC00000A7: return "STATUS_BAD_VALIDATION_CLASS";
    case 0xC00000A8: return "STATUS_BAD_TOKEN_TYPE";
    case 0xC00000A9: return "STATUS_BAD_MASTER_BOOT_RECORD";
    case 0xC00000AA: return "STATUS_INSTRUCTION_MISALIGNMENT";
    case 0xC00000AB: return "STATUS_INSTANCE_NOT_AVAILABLE";
    case 0xC00000AC: return "STATUS_PIPE_NOT_AVAILABLE";
    case 0xC00000AD: return "STATUS_INVALID_PIPE_STATE";
    case 0xC00000AE: return "STATUS_PIPE_BUSY";
    case 0xC00000AF: return "STATUS_ILLEGAL_FUNCTION";
    case 0xC00000B0: return "STATUS_PIPE_DISCONNECTED";
    case 0xC00000B1: return "STATUS_PIPE_CLOSING";
    case 0xC00000B2: return "STATUS_PIPE_CONNECTED";
    case 0xC00000B3: return "STATUS_PIPE_LISTENING";
    case 0xC00000B4: return "STATUS_INVALID_READ_MODE";
    case 0xC00000B5: return "STATUS_IO_TIMEOUT";
    case 0xC00000B6: return "STATUS_FILE_FORCED_CLOSED";
    case 0xC00000B7: return "STATUS_PROFILING_NOT_STARTED";
    case 0xC00000B8: return "STATUS_PROFILING_NOT_STOPPED";
    case 0xC00000B9: return "STATUS_COULD_NOT_INTERPRET";
    case 0xC00000BA: return "STATUS_FILE_IS_A_DIRECTORY";
    case 0xC00000BB: return "STATUS_NOT_SUPPORTED";
    case 0xC00000BC: return "STATUS_REMOTE_NOT_LISTENING";
    case 0xC00000BD: return "STATUS_DUPLICATE_NAME";
    case 0xC00000BE: return "STATUS_BAD_NETWORK_PATH";
    case 0xC00000BF: return "STATUS_NETWORK_BUSY";
    case 0xC00000C0: return "STATUS_DEVICE_DOES_NOT_EXIST";
    case 0xC00000C1: return "STATUS_TOO_MANY_COMMANDS";
    case 0xC00000C2: return "STATUS_ADAPTER_HARDWARE_ERROR";
    case 0xC00000C3: return "STATUS_INVALID_NETWORK_RESPONSE";
    case 0xC00000C4: return "STATUS_UNEXPECTED_NETWORK_ERROR";
    case 0xC00000C5: return "STATUS_BAD_REMOTE_ADAPTER";
    case 0xC00000C6: return "STATUS_PRINT_QUEUE_FULL";
    case 0xC00000C7: return "STATUS_NO_SPOOL_SPACE";
    case 0xC00000C8: return "STATUS_PRINT_CANCELLED";
    case 0xC00000C9: return "STATUS_NETWORK_NAME_DELETED";
    case 0xC00000CA: return "STATUS_NETWORK_ACCESS_DENIED";
    case 0xC00000CB: return "STATUS_BAD_DEVICE_TYPE";
    case 0xC00000CC: return "STATUS_BAD_NETWORK_NAME";
    case 0xC00000CD: return "STATUS_TOO_MANY_NAMES";
    case 0xC00000CE: return "STATUS_TOO_MANY_SESSIONS";
    case 0xC00000CF: return "STATUS_SHARING_PAUSED";
    case 0xC00000D0: return "STATUS_REQUEST_NOT_ACCEPTED";
    case 0xC00000D1: return "STATUS_REDIRECTOR_PAUSED";
    case 0xC00000D2: return "STATUS_NET_WRITE_FAULT";
    case 0xC00000D3: return "STATUS_PROFILING_AT_LIMIT";
    case 0xC00000D4: return "STATUS_NOT_SAME_DEVICE";
    case 0xC00000D5: return "STATUS_FILE_RENAMED";
    case 0xC00000D6: return "STATUS_VIRTUAL_CIRCUIT_CLOSED";
    case 0xC00000D7: return "STATUS_NO_SECURITY_ON_OBJECT";
    case 0xC00000D8: return "STATUS_CANT_WAIT";
    case 0xC00000D9: return "STATUS_PIPE_EMPTY";
    case 0xC00000DA: return "STATUS_CANT_ACCESS_DOMAIN_INFO";
    case 0xC00000DB: return "STATUS_CANT_TERMINATE_SELF";
    case 0xC00000DC: return "STATUS_INVALID_SERVER_STATE";
    case 0xC00000DD: return "STATUS_INVALID_DOMAIN_STATE";
    case 0xC00000DE: return "STATUS_INVALID_DOMAIN_ROLE";
    case 0xC00000DF: return "STATUS_NO_SUCH_DOMAIN";
    case 0xC00000E0: return "STATUS_DOMAIN_EXISTS";
    case 0xC00000E1: return "STATUS_DOMAIN_LIMIT_EXCEEDED";
    case 0xC00000E2: return "STATUS_OPLOCK_NOT_GRANTED";
    case 0xC00000E3: return "STATUS_INVALID_OPLOCK_PROTOCOL";
    case 0xC00000E4: return "STATUS_INTERNAL_DB_CORRUPTION";
    case 0xC00000E5: return "STATUS_INTERNAL_ERROR";
    case 0xC00000E6: return "STATUS_GENERIC_NOT_MAPPED";
    case 0xC00000E7: return "STATUS_BAD_DESCRIPTOR_FORMAT";
    case 0xC00000E8: return "STATUS_INVALID_USER_BUFFER";
    case 0xC00000E9: return "STATUS_UNEXPECTED_IO_ERROR";
    case 0xC00000EA: return "STATUS_UNEXPECTED_MM_CREATE_ERR";
    case 0xC00000EB: return "STATUS_UNEXPECTED_MM_MAP_ERROR";
    case 0xC00000EC: return "STATUS_UNEXPECTED_MM_EXTEND_ERR";
    case 0xC00000ED: return "STATUS_NOT_LOGON_PROCESS";
    case 0xC00000EE: return "STATUS_LOGON_SESSION_EXISTS";
    case 0xC00000EF: return "STATUS_INVALID_PARAMETER_1";
    case 0xC00000F0: return "STATUS_INVALID_PARAMETER_2";
    case 0xC00000F1: return "STATUS_INVALID_PARAMETER_3";
    case 0xC00000F2: return "STATUS_INVALID_PARAMETER_4";
    case 0xC00000F3: return "STATUS_INVALID_PARAMETER_5";
    case 0xC00000F4: return "STATUS_INVALID_PARAMETER_6";
    case 0xC00000F5: return "STATUS_INVALID_PARAMETER_7";
    case 0xC00000F6: return "STATUS_INVALID_PARAMETER_8";
    case 0xC00000F7: return "STATUS_INVALID_PARAMETER_9";
    case 0xC00000F8: return "STATUS_INVALID_PARAMETER_10";
    case 0xC00000F9: return "STATUS_INVALID_PARAMETER_11";
    case 0xC00000FA: return "STATUS_INVALID_PARAMETER_12";
    case 0xC00000FB: return "STATUS_REDIRECTOR_NOT_STARTED";
    case 0xC00000FC: return "STATUS_REDIRECTOR_STARTED";
    case 0xC00000FD: return "STATUS_STACK_OVERFLOW";
    case 0xC00000FE: return "STATUS_NO_SUCH_PACKAGE";
    case 0xC00000FF: return "STATUS_BAD_FUNCTION_TABLE";
    case 0xC0000100: return "STATUS_VARIABLE_NOT_FOUND";
    case 0xC0000101: return "STATUS_DIRECTORY_NOT_EMPTY";
    case 0xC0000102: return "STATUS_FILE_CORRUPT_ERROR";
    case 0xC0000103: return "STATUS_NOT_A_DIRECTORY";
    case 0xC0000104: return "STATUS_BAD_LOGON_SESSION_STATE";
    case 0xC0000105: return "STATUS_LOGON_SESSION_COLLISION";
    case 0xC0000106: return "STATUS_NAME_TOO_LONG";
    case 0xC0000107: return "STATUS_FILES_OPEN";
    case 0xC0000108: return "STATUS_CONNECTION_IN_USE";
    case 0xC0000109: return "STATUS_MESSAGE_NOT_FOUND";
    case 0xC000010A: return "STATUS_PROCESS_IS_TERMINATING";
    case 0xC000010B: return "STATUS_INVALID_LOGON_TYPE";
    case 0xC000010C: return "STATUS_NO_GUID_TRANSLATION";
    case 0xC000010D: return "STATUS_CANNOT_IMPERSONATE";
    case 0xC000010E: return "STATUS_IMAGE_ALREADY_LOADED";
    case 0xC000010F: return "STATUS_ABIOS_NOT_PRESENT";
    case 0xC0000110: return "STATUS_ABIOS_LID_NOT_EXIST";
    case 0xC0000111: return "STATUS_ABIOS_LID_ALREADY_OWNED";
    case 0xC0000112: return "STATUS_ABIOS_NOT_LID_OWNER";
    case 0xC0000113: return "STATUS_ABIOS_INVALID_COMMAND";
    case 0xC0000114: return "STATUS_ABIOS_INVALID_LID";
    case 0xC0000115: return "STATUS_ABIOS_SELECTOR_NOT_AVAILABLE";
    case 0xC0000116: return "STATUS_ABIOS_INVALID_SELECTOR";
    case 0xC0000117: return "STATUS_NO_LDT";
    case 0xC0000118: return "STATUS_INVALID_LDT_SIZE";
    case 0xC0000119: return "STATUS_INVALID_LDT_OFFSET";
    case 0xC000011A: return "STATUS_INVALID_LDT_DESCRIPTOR";
    case 0xC000011B: return "STATUS_INVALID_IMAGE_NE_FORMAT";
    case 0xC000011C: return "STATUS_RXACT_INVALID_STATE";
    case 0xC000011D: return "STATUS_RXACT_COMMIT_FAILURE";
    case 0xC000011E: return "STATUS_MAPPED_FILE_SIZE_ZERO";
    case 0xC000011F: return "STATUS_TOO_MANY_OPENED_FILES";
    case 0xC0000120: return "STATUS_CANCELLED";
    case 0xC0000121: return "STATUS_CANNOT_DELETE";
    case 0xC0000122: return "STATUS_INVALID_COMPUTER_NAME";
    case 0xC0000123: return "STATUS_FILE_DELETED";
    case 0xC0000124: return "STATUS_SPECIAL_ACCOUNT";
    case 0xC0000125: return "STATUS_SPECIAL_GROUP";
    case 0xC0000126: return "STATUS_SPECIAL_USER";
    case 0xC0000127: return "STATUS_MEMBERS_PRIMARY_GROUP";
    case 0xC0000128: return "STATUS_FILE_CLOSED";
    case 0xC0000129: return "STATUS_TOO_MANY_THREADS";
    case 0xC000012A: return "STATUS_THREAD_NOT_IN_PROCESS";
    case 0xC000012B: return "STATUS_TOKEN_ALREADY_IN_USE";
    case 0xC000012C: return "STATUS_PAGEFILE_QUOTA_EXCEEDED";
    case 0xC000012D: return "STATUS_COMMITMENT_LIMIT";
    case 0xC000012E: return "STATUS_INVALID_IMAGE_LE_FORMAT";
    case 0xC000012F: return "STATUS_INVALID_IMAGE_NOT_MZ";
    case 0xC0000130: return "STATUS_INVALID_IMAGE_PROTECT";
    case 0xC0000131: return "STATUS_INVALID_IMAGE_WIN_16";
    case 0xC0000132: return "STATUS_LOGON_SERVER_CONFLICT";
    case 0xC0000133: return "STATUS_TIME_DIFFERENCE_AT_DC";
    case 0xC0000134: return "STATUS_SYNCHRONIZATION_REQUIRED";
    case 0xC0000135: return "STATUS_DLL_NOT_FOUND";
    case 0xC0000136: return "STATUS_OPEN_FAILED";
    case 0xC0000137: return "STATUS_IO_PRIVILEGE_FAILED";
    case 0xC0000138: return "STATUS_ORDINAL_NOT_FOUND";
    case 0xC0000139: return "STATUS_ENTRYPOINT_NOT_FOUND";
    case 0xC000013A: return "STATUS_CONTROL_C_EXIT";
    case 0xC000013B: return "STATUS_LOCAL_DISCONNECT";
    case 0xC000013C: return "STATUS_REMOTE_DISCONNECT";
    case 0xC000013D: return "STATUS_REMOTE_RESOURCES";
    case 0xC000013E: return "STATUS_LINK_FAILED";
    case 0xC000013F: return "STATUS_LINK_TIMEOUT";
    case 0xC0000140: return "STATUS_INVALID_CONNECTION";
    case 0xC0000141: return "STATUS_INVALID_ADDRESS";
    case 0xC0000142: return "STATUS_DLL_INIT_FAILED";
    case 0xC0000143: return "STATUS_MISSING_SYSTEMFILE";
    case 0xC0000144: return "STATUS_UNHANDLED_EXCEPTION";
    case 0xC0000145: return "STATUS_APP_INIT_FAILURE";
    case 0xC0000146: return "STATUS_PAGEFILE_CREATE_FAILED";
    case 0xC0000147: return "STATUS_NO_PAGEFILE";
    case 0xC0000148: return "STATUS_INVALID_LEVEL";
    case 0xC0000149: return "STATUS_WRONG_PASSWORD_CORE";
    case 0xC000014A: return "STATUS_ILLEGAL_FLOAT_CONTEXT";
    case 0xC000014B: return "STATUS_PIPE_BROKEN";
    case 0xC000014C: return "STATUS_REGISTRY_CORRUPT";
    case 0xC000014D: return "STATUS_REGISTRY_IO_FAILED";
    case 0xC000014E: return "STATUS_NO_EVENT_PAIR";
    case 0xC000014F: return "STATUS_UNRECOGNIZED_VOLUME";
    case 0xC0000150: return "STATUS_SERIAL_NO_DEVICE_INITED";
    case 0xC0000151: return "STATUS_NO_SUCH_ALIAS";
    case 0xC0000152: return "STATUS_MEMBER_NOT_IN_ALIAS";
    case 0xC0000153: return "STATUS_MEMBER_IN_ALIAS";
    case 0xC0000154: return "STATUS_ALIAS_EXISTS";
    case 0xC0000155: return "STATUS_LOGON_NOT_GRANTED";
    case 0xC0000156: return "STATUS_TOO_MANY_SECRETS";
    case 0xC0000157: return "STATUS_SECRET_TOO_LONG";
    case 0xC0000158: return "STATUS_INTERNAL_DB_ERROR";
    case 0xC0000159: return "STATUS_FULLSCREEN_MODE";
    case 0xC000015A: return "STATUS_TOO_MANY_CONTEXT_IDS";
    case 0xC000015B: return "STATUS_LOGON_TYPE_NOT_GRANTED";
    case 0xC000015C: return "STATUS_NOT_REGISTRY_FILE";
    case 0xC000015D: return "STATUS_NT_CROSS_ENCRYPTION_REQUIRED";
    case 0xC000015E: return "STATUS_DOMAIN_CTRLR_CONFIG_ERROR";
    case 0xC000015F: return "STATUS_FT_MISSING_MEMBER";
    case 0xC0000160: return "STATUS_ILL_FORMED_SERVICE_ENTRY";
    case 0xC0000161: return "STATUS_ILLEGAL_CHARACTER";
    case 0xC0000162: return "STATUS_UNMAPPABLE_CHARACTER";
    case 0xC0000163: return "STATUS_UNDEFINED_CHARACTER";
    case 0xC0000164: return "STATUS_FLOPPY_VOLUME";
    case 0xC0000165: return "STATUS_FLOPPY_ID_MARK_NOT_FOUND";
    case 0xC0000166: return "STATUS_FLOPPY_WRONG_CYLINDER";
    case 0xC0000167: return "STATUS_FLOPPY_UNKNOWN_ERROR";
    case 0xC0000168: return "STATUS_FLOPPY_BAD_REGISTERS";
    case 0xC0000169: return "STATUS_DISK_RECALIBRATE_FAILED";
    case 0xC000016A: return "STATUS_DISK_OPERATION_FAILED";
    case 0xC000016B: return "STATUS_DISK_RESET_FAILED";
    case 0xC000016C: return "STATUS_SHARED_IRQ_BUSY";
    case 0xC000016D: return "STATUS_FT_ORPHANING";
    case 0xC000016E: return "STATUS_BIOS_FAILED_TO_CONNECT_INTERRUPT";
    case 0xC0000172: return "STATUS_PARTITION_FAILURE";
    case 0xC0000173: return "STATUS_INVALID_BLOCK_LENGTH";
    case 0xC0000174: return "STATUS_DEVICE_NOT_PARTITIONED";
    case 0xC0000175: return "STATUS_UNABLE_TO_LOCK_MEDIA";
    case 0xC0000176: return "STATUS_UNABLE_TO_UNLOAD_MEDIA";
    case 0xC0000177: return "STATUS_EOM_OVERFLOW";
    case 0xC0000178: return "STATUS_NO_MEDIA";
    case 0xC000017A: return "STATUS_NO_SUCH_MEMBER";
    case 0xC000017B: return "STATUS_INVALID_MEMBER";
    case 0xC000017C: return "STATUS_KEY_DELETED";
    case 0xC000017D: return "STATUS_NO_LOG_SPACE";
    case 0xC000017E: return "STATUS_TOO_MANY_SIDS";
    case 0xC000017F: return "STATUS_LM_CROSS_ENCRYPTION_REQUIRED";
    case 0xC0000180: return "STATUS_KEY_HAS_CHILDREN";
    case 0xC0000181: return "STATUS_CHILD_MUST_BE_VOLATILE";
    case 0xC0000182: return "STATUS_DEVICE_CONFIGURATION_ERROR";
    case 0xC0000183: return "STATUS_DRIVER_INTERNAL_ERROR";
    case 0xC0000184: return "STATUS_INVALID_DEVICE_STATE";
    case 0xC0000185: return "STATUS_IO_DEVICE_ERROR";
    case 0xC0000186: return "STATUS_DEVICE_PROTOCOL_ERROR";
    case 0xC0000187: return "STATUS_BACKUP_CONTROLLER";
    case 0xC0000188: return "STATUS_LOG_FILE_FULL";
    case 0xC0000189: return "STATUS_TOO_LATE";
    case 0xC000018A: return "STATUS_NO_TRUST_LSA_SECRET";
    case 0xC000018B: return "STATUS_NO_TRUST_SAM_ACCOUNT";
    case 0xC000018C: return "STATUS_TRUSTED_DOMAIN_FAILURE";
    case 0xC000018D: return "STATUS_TRUSTED_RELATIONSHIP_FAILURE";
    case 0xC000018E: return "STATUS_EVENTLOG_FILE_CORRUPT";
    case 0xC000018F: return "STATUS_EVENTLOG_CANT_START";
    case 0xC0000190: return "STATUS_TRUST_FAILURE";
    case 0xC0000191: return "STATUS_MUTANT_LIMIT_EXCEEDED";
    case 0xC0000192: return "STATUS_NETLOGON_NOT_STARTED";
    case 0xC0000193: return "STATUS_ACCOUNT_EXPIRED";
    case 0xC0000194: return "STATUS_POSSIBLE_DEADLOCK";
    case 0xC0000195: return "STATUS_NETWORK_CREDENTIAL_CONFLICT";
    case 0xC0000196: return "STATUS_REMOTE_SESSION_LIMIT";
    case 0xC0000197: return "STATUS_EVENTLOG_FILE_CHANGED";
    case 0xC0000198: return "STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT";
    case 0xC0000199: return "STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT";
    case 0xC000019A: return "STATUS_NOLOGON_SERVER_TRUST_ACCOUNT";
    case 0xC000019B: return "STATUS_DOMAIN_TRUST_INCONSISTENT";
    case 0xC000019C: return "STATUS_FS_DRIVER_REQUIRED";
    case 0xC0000202: return "STATUS_NO_USER_SESSION_KEY";
    case 0xC0000203: return "STATUS_USER_SESSION_DELETED";
    case 0xC0000204: return "STATUS_RESOURCE_LANG_NOT_FOUND";
    case 0xC0000205: return "STATUS_INSUFF_SERVER_RESOURCES";
    case 0xC0000206: return "STATUS_INVALID_BUFFER_SIZE";
    case 0xC0000207: return "STATUS_INVALID_ADDRESS_COMPONENT";
    case 0xC0000208: return "STATUS_INVALID_ADDRESS_WILDCARD";
    case 0xC0000209: return "STATUS_TOO_MANY_ADDRESSES";
    case 0xC000020A: return "STATUS_ADDRESS_ALREADY_EXISTS";
    case 0xC000020B: return "STATUS_ADDRESS_CLOSED";
    case 0xC000020C: return "STATUS_CONNECTION_DISCONNECTED";
    case 0xC000020D: return "STATUS_CONNECTION_RESET";
    case 0xC000020E: return "STATUS_TOO_MANY_NODES";
    case 0xC000020F: return "STATUS_TRANSACTION_ABORTED";
    case 0xC0000210: return "STATUS_TRANSACTION_TIMED_OUT";
    case 0xC0000211: return "STATUS_TRANSACTION_NO_RELEASE";
    case 0xC0000212: return "STATUS_TRANSACTION_NO_MATCH";
    case 0xC0000213: return "STATUS_TRANSACTION_RESPONDED";
    case 0xC0000214: return "STATUS_TRANSACTION_INVALID_ID";
    case 0xC0000215: return "STATUS_TRANSACTION_INVALID_TYPE";
    case 0xC0000216: return "STATUS_NOT_SERVER_SESSION";
    case 0xC0000217: return "STATUS_NOT_CLIENT_SESSION";
    case 0xC0000218: return "STATUS_CANNOT_LOAD_REGISTRY_FILE";
    case 0xC0000219: return "STATUS_DEBUG_ATTACH_FAILED";
    case 0xC000021A: return "STATUS_SYSTEM_PROCESS_TERMINATED";
    case 0xC000021B: return "STATUS_DATA_NOT_ACCEPTED";
    case 0xC000021C: return "STATUS_NO_BROWSER_SERVERS_FOUND";
    case 0xC000021D: return "STATUS_VDM_HARD_ERROR";
    case 0xC000021E: return "STATUS_DRIVER_CANCEL_TIMEOUT";
    case 0xC000021F: return "STATUS_REPLY_MESSAGE_MISMATCH";
    case 0xC0000220: return "STATUS_MAPPED_ALIGNMENT";
    case 0xC0000221: return "STATUS_IMAGE_CHECKSUM_MISMATCH";
    case 0xC0000222: return "STATUS_LOST_WRITEBEHIND_DATA";
    case 0xC0000223: return "STATUS_CLIENT_SERVER_PARAMETERS_INVALID";
    case 0xC0000224: return "STATUS_PASSWORD_MUST_CHANGE";
    case 0xC0000225: return "STATUS_NOT_FOUND";
    case 0xC0000226: return "STATUS_NOT_TINY_STREAM";
    case 0xC0000227: return "STATUS_RECOVERY_FAILURE";
    case 0xC0000228: return "STATUS_STACK_OVERFLOW_READ";
    case 0xC0000229: return "STATUS_FAIL_CHECK";
    case 0xC000022A: return "STATUS_DUPLICATE_OBJECTID";
    case 0xC000022B: return "STATUS_OBJECTID_EXISTS";
    case 0xC000022C: return "STATUS_CONVERT_TO_LARGE";
    case 0xC000022D: return "STATUS_RETRY";
    case 0xC000022E: return "STATUS_FOUND_OUT_OF_SCOPE";
    case 0xC000022F: return "STATUS_ALLOCATE_BUCKET";
    case 0xC0000230: return "STATUS_PROPSET_NOT_FOUND";
    case 0xC0000231: return "STATUS_MARSHALL_OVERFLOW";
    case 0xC0000232: return "STATUS_INVALID_VARIANT";
    case 0xC0000233: return "STATUS_DOMAIN_CONTROLLER_NOT_FOUND";
    case 0xC0000234: return "STATUS_ACCOUNT_LOCKED_OUT";
    case 0xC0000235: return "STATUS_HANDLE_NOT_CLOSABLE";
    case 0xC0000236: return "STATUS_CONNECTION_REFUSED";
    case 0xC0000237: return "STATUS_GRACEFUL_DISCONNECT";
    case 0xC0000238: return "STATUS_ADDRESS_ALREADY_ASSOCIATED";
    case 0xC0000239: return "STATUS_ADDRESS_NOT_ASSOCIATED";
    case 0xC000023A: return "STATUS_CONNECTION_INVALID";
    case 0xC000023B: return "STATUS_CONNECTION_ACTIVE";
    case 0xC000023C: return "STATUS_NETWORK_UNREACHABLE";
    case 0xC000023D: return "STATUS_HOST_UNREACHABLE";
    case 0xC000023E: return "STATUS_PROTOCOL_UNREACHABLE";
    case 0xC000023F: return "STATUS_PORT_UNREACHABLE";
    case 0xC0000240: return "STATUS_REQUEST_ABORTED";
    case 0xC0000241: return "STATUS_CONNECTION_ABORTED";
    case 0xC0000242: return "STATUS_BAD_COMPRESSION_BUFFER";
    case 0xC0000243: return "STATUS_USER_MAPPED_FILE";
    case 0xC0000244: return "STATUS_AUDIT_FAILED";
    case 0xC0000245: return "STATUS_TIMER_RESOLUTION_NOT_SET";
    case 0xC0000246: return "STATUS_CONNECTION_COUNT_LIMIT";
    case 0xC0000247: return "STATUS_LOGIN_TIME_RESTRICTION";
    case 0xC0000248: return "STATUS_LOGIN_WKSTA_RESTRICTION";
    case 0xC0000249: return "STATUS_IMAGE_MP_UP_MISMATCH";
    case 0xC0000250: return "STATUS_INSUFFICIENT_LOGON_INFO";
    case 0xC0000251: return "STATUS_BAD_DLL_ENTRYPOINT";
    case 0xC0000252: return "STATUS_BAD_SERVICE_ENTRYPOINT";
    case 0xC0000253: return "STATUS_LPC_REPLY_LOST";
    case 0xC0000254: return "STATUS_IP_ADDRESS_CONFLICT1";
    case 0xC0000255: return "STATUS_IP_ADDRESS_CONFLICT2";
    case 0xC0000256: return "STATUS_REGISTRY_QUOTA_LIMIT";
    case 0xC0000257: return "STATUS_PATH_NOT_COVERED";
    case 0xC0000258: return "STATUS_NO_CALLBACK_ACTIVE";
    case 0xC0000259: return "STATUS_LICENSE_QUOTA_EXCEEDED";
    case 0xC000025A: return "STATUS_PWD_TOO_SHORT";
    case 0xC000025B: return "STATUS_PWD_TOO_RECENT";
    case 0xC000025C: return "STATUS_PWD_HISTORY_CONFLICT";
    case 0xC000025E: return "STATUS_PLUGPLAY_NO_DEVICE";
    case 0xC000025F: return "STATUS_UNSUPPORTED_COMPRESSION";
    case 0xC0000260: return "STATUS_INVALID_HW_PROFILE";
    case 0xC0000261: return "STATUS_INVALID_PLUGPLAY_DEVICE_PATH";
    case 0xC0000262: return "STATUS_DRIVER_ORDINAL_NOT_FOUND";
    case 0xC0000263: return "STATUS_DRIVER_ENTRYPOINT_NOT_FOUND";
    case 0xC0000264: return "STATUS_RESOURCE_NOT_OWNED";
    case 0xC0000265: return "STATUS_TOO_MANY_LINKS";
    case 0xC0000266: return "STATUS_QUOTA_LIST_INCONSISTENT";
    case 0xC0000267: return "STATUS_FILE_IS_OFFLINE";
    case 0xC0000268: return "STATUS_EVALUATION_EXPIRATION";
    case 0xC0000269: return "STATUS_ILLEGAL_DLL_RELOCATION";
    case 0xC000026A: return "STATUS_LICENSE_VIOLATION";
    case 0xC000026B: return "STATUS_DLL_INIT_FAILED_LOGOFF";
    case 0xC000026C: return "STATUS_DRIVER_UNABLE_TO_LOAD";
    case 0xC000026D: return "STATUS_DFS_UNAVAILABLE";
    case 0xC000026E: return "STATUS_VOLUME_DISMOUNTED";
    case 0xC000026F: return "STATUS_WX86_INTERNAL_ERROR";
    case 0xC0000270: return "STATUS_WX86_FLOAT_STACK_CHECK";
    case 0xC0000271: return "STATUS_VALIDATE_CONTINUE";
    case 0xC0000272: return "STATUS_NO_MATCH";
    case 0xC0000273: return "STATUS_NO_MORE_MATCHES";
    case 0xC0000275: return "STATUS_NOT_A_REPARSE_POINT";
    case 0xC0000276: return "STATUS_IO_REPARSE_TAG_INVALID";
    case 0xC0000277: return "STATUS_IO_REPARSE_TAG_MISMATCH";
    case 0xC0000278: return "STATUS_IO_REPARSE_DATA_INVALID";
    case 0xC0000279: return "STATUS_IO_REPARSE_TAG_NOT_HANDLED";
    case 0xC0000280: return "STATUS_REPARSE_POINT_NOT_RESOLVED";
    case 0xC0000281: return "STATUS_DIRECTORY_IS_A_REPARSE_POINT";
    case 0xC0000282: return "STATUS_RANGE_LIST_CONFLICT";
    case 0xC0000283: return "STATUS_SOURCE_ELEMENT_EMPTY";
    case 0xC0000284: return "STATUS_DESTINATION_ELEMENT_FULL";
    case 0xC0000285: return "STATUS_ILLEGAL_ELEMENT_ADDRESS";
    case 0xC0000286: return "STATUS_MAGAZINE_NOT_PRESENT";
    case 0xC0000287: return "STATUS_REINITIALIZATION_NEEDED";
    case 0x80000288: return "STATUS_DEVICE_REQUIRES_CLEANING";
    case 0x80000289: return "STATUS_DEVICE_DOOR_OPEN";
    case 0xC000028A: return "STATUS_ENCRYPTION_FAILED";
    case 0xC000028B: return "STATUS_DECRYPTION_FAILED";
    case 0xC000028C: return "STATUS_RANGE_NOT_FOUND";
    case 0xC000028D: return "STATUS_NO_RECOVERY_POLICY";
    case 0xC000028E: return "STATUS_NO_EFS";
    case 0xC000028F: return "STATUS_WRONG_EFS";
    case 0xC0000290: return "STATUS_NO_USER_KEYS";
    case 0xC0000291: return "STATUS_FILE_NOT_ENCRYPTED";
    case 0xC0000292: return "STATUS_NOT_EXPORT_FORMAT";
    case 0xC0000293: return "STATUS_FILE_ENCRYPTED";
    case 0x40000294: return "STATUS_WAKE_SYSTEM";
    case 0xC0000295: return "STATUS_WMI_GUID_NOT_FOUND";
    case 0xC0000296: return "STATUS_WMI_INSTANCE_NOT_FOUND";
    case 0xC0000297: return "STATUS_WMI_ITEMID_NOT_FOUND";
    case 0xC0000298: return "STATUS_WMI_TRY_AGAIN";
    case 0xC0000299: return "STATUS_SHARED_POLICY";
    case 0xC000029A: return "STATUS_POLICY_OBJECT_NOT_FOUND";
    case 0xC000029B: return "STATUS_POLICY_ONLY_IN_DS";
    case 0xC000029C: return "STATUS_VOLUME_NOT_UPGRADED";
    case 0xC000029D: return "STATUS_REMOTE_STORAGE_NOT_ACTIVE";
    case 0xC000029E: return "STATUS_REMOTE_STORAGE_MEDIA_ERROR";
    case 0xC000029F: return "STATUS_NO_TRACKING_SERVICE";
    case 0xC00002A0: return "STATUS_SERVER_SID_MISMATCH";
    case 0xC00002A1: return "STATUS_DS_NO_ATTRIBUTE_OR_VALUE";
    case 0xC00002A2: return "STATUS_DS_INVALID_ATTRIBUTE_SYNTAX";
    case 0xC00002A3: return "STATUS_DS_ATTRIBUTE_TYPE_UNDEFINED";
    case 0xC00002A4: return "STATUS_DS_ATTRIBUTE_OR_VALUE_EXISTS";
    case 0xC00002A5: return "STATUS_DS_BUSY";
    case 0xC00002A6: return "STATUS_DS_UNAVAILABLE";
    case 0xC00002A7: return "STATUS_DS_NO_RIDS_ALLOCATED";
    case 0xC00002A8: return "STATUS_DS_NO_MORE_RIDS";
    case 0xC00002A9: return "STATUS_DS_INCORRECT_ROLE_OWNER";
    case 0xC00002AA: return "STATUS_DS_RIDMGR_INIT_ERROR";
    case 0xC00002AB: return "STATUS_DS_OBJ_CLASS_VIOLATION";
    case 0xC00002AC: return "STATUS_DS_CANT_ON_NON_LEAF";
    case 0xC00002AD: return "STATUS_DS_CANT_ON_RDN";
    case 0xC00002AE: return "STATUS_DS_CANT_MOD_OBJ_CLASS";
    case 0xC00002AF: return "STATUS_DS_CROSS_DOM_MOVE_FAILED";
    case 0xC00002B0: return "STATUS_DS_GC_NOT_AVAILABLE";
    case 0xC00002B1: return "STATUS_DIRECTORY_SERVICE_REQUIRED";
    case 0xC00002B2: return "STATUS_REPARSE_ATTRIBUTE_CONFLICT";
    case 0xC00002B3: return "STATUS_CANT_ENABLE_DENY_ONLY";
    case 0xC00002B4: return "STATUS_FLOAT_MULTIPLE_FAULTS";
    case 0xC00002B5: return "STATUS_FLOAT_MULTIPLE_TRAPS";
    case 0xC00002B6: return "STATUS_DEVICE_REMOVED";
    case 0xC00002B7: return "STATUS_JOURNAL_DELETE_IN_PROGRESS";
    case 0xC00002B8: return "STATUS_JOURNAL_NOT_ACTIVE";
    case 0xC00002B9: return "STATUS_NOINTERFACE";
    case 0xC00002C1: return "STATUS_DS_ADMIN_LIMIT_EXCEEDED";
    case 0xC00002C2: return "STATUS_DRIVER_FAILED_SLEEP";
    case 0xC00002C3: return "STATUS_MUTUAL_AUTHENTICATION_FAILED";
    case 0xC00002C4: return "STATUS_CORRUPT_SYSTEM_FILE";
    case 0xC00002C5: return "STATUS_DATATYPE_MISALIGNMENT_ERROR";
    case 0xC00002C6: return "STATUS_WMI_READ_ONLY";
    case 0xC00002C7: return "STATUS_WMI_SET_FAILURE";
    case 0xC00002C8: return "STATUS_COMMITMENT_MINIMUM";
    case 0xC00002C9: return "STATUS_REG_NAT_CONSUMPTION";
    case 0xC00002CA: return "STATUS_TRANSPORT_FULL";
    case 0xC00002CB: return "STATUS_DS_SAM_INIT_FAILURE";
    case 0xC00002CC: return "STATUS_ONLY_IF_CONNECTED";
    case 0xC00002CD: return "STATUS_DS_SENSITIVE_GROUP_VIOLATION";
    case 0xC00002CE: return "STATUS_PNP_RESTART_ENUMERATION";
    case 0xC00002CF: return "STATUS_JOURNAL_ENTRY_DELETED";
    case 0xC00002D0: return "STATUS_DS_CANT_MOD_PRIMARYGROUPID";
    case 0xC00002D1: return "STATUS_SYSTEM_IMAGE_BAD_SIGNATURE";
    case 0xC00002D2: return "STATUS_PNP_REBOOT_REQUIRED";
    case 0xC00002D3: return "STATUS_POWER_STATE_INVALID";
    case 0xC00002D4: return "STATUS_DS_INVALID_GROUP_TYPE";
    case 0xC00002D5: return "STATUS_DS_NO_NEST_GLOBALGROUP_IN_MIXEDDOMAIN";
    case 0xC00002D6: return "STATUS_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN";
    case 0xC00002D7: return "STATUS_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER";
    case 0xC00002D8: return "STATUS_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER";
    case 0xC00002D9: return "STATUS_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER";
    case 0xC00002DA: return "STATUS_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER";
    case 0xC00002DB: return "STATUS_DS_LOCAL_CANT_HAVE_CROSSDOMAIN_LOCAL_MEMBER";
    case 0xC00002DC: return "STATUS_DS_HAVE_PRIMARY_MEMBERS";
    case 0xC00002DD: return "STATUS_WMI_NOT_SUPPORTED";
    case 0xC00002DE: return "STATUS_INSUFFICIENT_POWER";
    case 0xC00002DF: return "STATUS_SAM_NEED_BOOTKEY_PASSWORD";
    case 0xC00002E0: return "STATUS_SAM_NEED_BOOTKEY_FLOPPY";
    case 0xC00002E1: return "STATUS_DS_CANT_START";
    case 0xC00002E2: return "STATUS_DS_INIT_FAILURE";
    case 0xC00002E3: return "STATUS_SAM_INIT_FAILURE";
    case 0xC00002E4: return "STATUS_DS_GC_REQUIRED";
    case 0xC00002E5: return "STATUS_DS_LOCAL_MEMBER_OF_LOCAL_ONLY";
    case 0xC00002E6: return "STATUS_DS_NO_FPO_IN_UNIVERSAL_GROUPS";
    case 0xC00002E7: return "STATUS_DS_MACHINE_ACCOUNT_QUOTA_EXCEEDED";
    case 0xC00002E8: return "STATUS_MULTIPLE_FAULT_VIOLATION";
    case 0xC00002E9: return "STATUS_CURRENT_DOMAIN_NOT_ALLOWED";
    case 0xC00002EA: return "STATUS_CANNOT_MAKE";
    case 0xC00002EB: return "STATUS_SYSTEM_SHUTDOWN";
    case 0xC00002EC: return "STATUS_DS_INIT_FAILURE_CONSOLE";
    case 0xC00002ED: return "STATUS_DS_SAM_INIT_FAILURE_CONSOLE";
    case 0xC00002EE: return "STATUS_UNFINISHED_CONTEXT_DELETED";
    case 0xC00002EF: return "STATUS_NO_TGT_REPLY";
    case 0xC00002F0: return "STATUS_OBJECTID_NOT_FOUND";
    case 0xC00002F1: return "STATUS_NO_IP_ADDRESSES";
    case 0xC00002F2: return "STATUS_WRONG_CREDENTIAL_HANDLE";
    case 0xC00002F3: return "STATUS_CRYPTO_SYSTEM_INVALID";
    case 0xC00002F4: return "STATUS_MAX_REFERRALS_EXCEEDED";
    case 0xC00002F5: return "STATUS_MUST_BE_KDC";
    case 0xC00002F6: return "STATUS_STRONG_CRYPTO_NOT_SUPPORTED";
    case 0xC00002F7: return "STATUS_TOO_MANY_PRINCIPALS";
    case 0xC00002F8: return "STATUS_NO_PA_DATA";
    case 0xC00002F9: return "STATUS_PKINIT_NAME_MISMATCH";
    case 0xC00002FA: return "STATUS_SMARTCARD_LOGON_REQUIRED";
    case 0xC00002FB: return "STATUS_KDC_INVALID_REQUEST";
    case 0xC00002FC: return "STATUS_KDC_UNABLE_TO_REFER";
    case 0xC00002FD: return "STATUS_KDC_UNKNOWN_ETYPE";
    case 0xC00002FE: return "STATUS_SHUTDOWN_IN_PROGRESS";
    case 0xC00002FF: return "STATUS_SERVER_SHUTDOWN_IN_PROGRESS";
    case 0xC0000300: return "STATUS_NOT_SUPPORTED_ON_SBS";
    case 0xC0000301: return "STATUS_WMI_GUID_DISCONNECTED";
    case 0xC0000302: return "STATUS_WMI_ALREADY_DISABLED";
    case 0xC0000303: return "STATUS_WMI_ALREADY_ENABLED";
    case 0xC0000304: return "STATUS_MFT_TOO_FRAGMENTED";
    case 0xC0000305: return "STATUS_COPY_PROTECTION_FAILURE";
    case 0xC0000306: return "STATUS_CSS_AUTHENTICATION_FAILURE";
    case 0xC0000307: return "STATUS_CSS_KEY_NOT_PRESENT";
    case 0xC0000308: return "STATUS_CSS_KEY_NOT_ESTABLISHED";
    case 0xC0000309: return "STATUS_CSS_SCRAMBLED_SECTOR";
    case 0xC000030A: return "STATUS_CSS_REGION_MISMATCH";
    case 0xC000030B: return "STATUS_CSS_RESETS_EXHAUSTED";
    case 0xC0000320: return "STATUS_PKINIT_FAILURE";
    case 0xC0000321: return "STATUS_SMARTCARD_SUBSYSTEM_FAILURE";
    case 0xC0000322: return "STATUS_NO_KERB_KEY";
    case 0xC0000350: return "STATUS_HOST_DOWN";
    case 0xC0000351: return "STATUS_UNSUPPORTED_PREAUTH";
    case 0xC0000352: return "STATUS_EFS_ALG_BLOB_TOO_BIG";
    case 0xC0000353: return "STATUS_PORT_NOT_SET";
    case 0xC0000354: return "STATUS_DEBUGGER_INACTIVE";
    case 0xC0000355: return "STATUS_DS_VERSION_CHECK_FAILURE";
    case 0xC0000356: return "STATUS_AUDITING_DISABLED";
    case 0xC0000357: return "STATUS_PRENT4_MACHINE_ACCOUNT";
    case 0xC0000358: return "STATUS_DS_AG_CANT_HAVE_UNIVERSAL_MEMBER";
    case 0xC0000359: return "STATUS_INVALID_IMAGE_WIN_32";
    case 0xC000035A: return "STATUS_INVALID_IMAGE_WIN_64";
    case 0xC000035B: return "STATUS_BAD_BINDINGS";
    case 0xC000035C: return "STATUS_NETWORK_SESSION_EXPIRED";
    case 0xC000035D: return "STATUS_APPHELP_BLOCK";
    case 0xC000035E: return "STATUS_ALL_SIDS_FILTERED";
    case 0xC000035F: return "STATUS_NOT_SAFE_MODE_DRIVER";
    case 0xC0000361: return "STATUS_ACCESS_DISABLED_BY_POLICY_DEFAULT";
    case 0xC0000362: return "STATUS_ACCESS_DISABLED_BY_POLICY_PATH";
    case 0xC0000363: return "STATUS_ACCESS_DISABLED_BY_POLICY_PUBLISHER";
    case 0xC0000364: return "STATUS_ACCESS_DISABLED_BY_POLICY_OTHER";
    case 0xC0000365: return "STATUS_FAILED_DRIVER_ENTRY";
    case 0xC0000366: return "STATUS_DEVICE_ENUMERATION_ERROR";
    case 0x00000367: return "STATUS_WAIT_FOR_OPLOCK";
    case 0xC0000368: return "STATUS_MOUNT_POINT_NOT_RESOLVED";
    case 0xC0000369: return "STATUS_INVALID_DEVICE_OBJECT_PARAMETER";
    case 0xC000036A: return "STATUS_MCA_OCCURED";
    case 0xC000036B: return "STATUS_DRIVER_BLOCKED_CRITICAL";
    case 0xC000036C: return "STATUS_DRIVER_BLOCKED";
    case 0xC000036D: return "STATUS_DRIVER_DATABASE_ERROR";
    case 0xC000036E: return "STATUS_SYSTEM_HIVE_TOO_LARGE";
    case 0xC000036F: return "STATUS_INVALID_IMPORT_OF_NON_DLL";
    case 0x40000370: return "STATUS_DS_SHUTTING_DOWN";
    case 0xC0000380: return "STATUS_SMARTCARD_WRONG_PIN";
    case 0xC0000381: return "STATUS_SMARTCARD_CARD_BLOCKED";
    case 0xC0000382: return "STATUS_SMARTCARD_CARD_NOT_AUTHENTICATED";
    case 0xC0000383: return "STATUS_SMARTCARD_NO_CARD";
    case 0xC0000384: return "STATUS_SMARTCARD_NO_KEY_CONTAINER";
    case 0xC0000385: return "STATUS_SMARTCARD_NO_CERTIFICATE";
    case 0xC0000386: return "STATUS_SMARTCARD_NO_KEYSET";
    case 0xC0000387: return "STATUS_SMARTCARD_IO_ERROR";
    case 0xC0000388: return "STATUS_DOWNGRADE_DETECTED";
    case 0xC0000389: return "STATUS_SMARTCARD_CERT_REVOKED";
    case 0xC000038A: return "STATUS_ISSUING_CA_UNTRUSTED";
    case 0xC000038B: return "STATUS_REVOCATION_OFFLINE_C";
    case 0xC000038C: return "STATUS_PKINIT_CLIENT_FAILURE";
    case 0xC000038D: return "STATUS_SMARTCARD_CERT_EXPIRED";
    case 0xC000038E: return "STATUS_DRIVER_FAILED_PRIOR_UNLOAD";
    case 0xC0009898: return "STATUS_WOW_ASSERTION";
    case 0xC0140001: return "STATUS_ACPI_INVALID_OPCODE";
    case 0xC0140002: return "STATUS_ACPI_STACK_OVERFLOW";
    case 0xC0140003: return "STATUS_ACPI_ASSERT_FAILED";
    case 0xC0140004: return "STATUS_ACPI_INVALID_INDEX";
    case 0xC0140005: return "STATUS_ACPI_INVALID_ARGUMENT";
    case 0xC0140006: return "STATUS_ACPI_FATAL";
    case 0xC0140007: return "STATUS_ACPI_INVALID_SUPERNAME";
    case 0xC0140008: return "STATUS_ACPI_INVALID_ARGTYPE";
    case 0xC0140009: return "STATUS_ACPI_INVALID_OBJTYPE";
    case 0xC014000A: return "STATUS_ACPI_INVALID_TARGETTYPE";
    case 0xC014000B: return "STATUS_ACPI_INCORRECT_ARGUMENT_COUNT";
    case 0xC014000C: return "STATUS_ACPI_ADDRESS_NOT_MAPPED";
    case 0xC014000D: return "STATUS_ACPI_INVALID_EVENTTYPE";
    case 0xC014000E: return "STATUS_ACPI_HANDLER_COLLISION";
    case 0xC014000F: return "STATUS_ACPI_INVALID_DATA";
    case 0xC0140010: return "STATUS_ACPI_INVALID_REGION";
    case 0xC0140011: return "STATUS_ACPI_INVALID_ACCESS_SIZE";
    case 0xC0140012: return "STATUS_ACPI_ACQUIRE_GLOBAL_LOCK";
    case 0xC0140013: return "STATUS_ACPI_ALREADY_INITIALIZED";
    case 0xC0140014: return "STATUS_ACPI_NOT_INITIALIZED";
    case 0xC0140015: return "STATUS_ACPI_INVALID_MUTEX_LEVEL";
    case 0xC0140016: return "STATUS_ACPI_MUTEX_NOT_OWNED";
    case 0xC0140017: return "STATUS_ACPI_MUTEX_NOT_OWNER";
    case 0xC0140018: return "STATUS_ACPI_RS_ACCESS";
    case 0xC0140019: return "STATUS_ACPI_INVALID_TABLE";
    case 0xC0140020: return "STATUS_ACPI_REG_HANDLER_FAILED";
    case 0xC0140021: return "STATUS_ACPI_POWER_REQUEST_FAILED";
    case 0xC00A0001: return "STATUS_CTX_WINSTATION_NAME_INVALID";
    case 0xC00A0002: return "STATUS_CTX_INVALID_PD";
    case 0xC00A0003: return "STATUS_CTX_PD_NOT_FOUND";
    case 0x400A0004: return "STATUS_CTX_CDM_CONNECT";
    case 0x400A0005: return "STATUS_CTX_CDM_DISCONNECT";
    case 0xC00A0006: return "STATUS_CTX_CLOSE_PENDING";
    case 0xC00A0007: return "STATUS_CTX_NO_OUTBUF";
    case 0xC00A0008: return "STATUS_CTX_MODEM_INF_NOT_FOUND";
    case 0xC00A0009: return "STATUS_CTX_INVALID_MODEMNAME";
    case 0xC00A000A: return "STATUS_CTX_RESPONSE_ERROR";
    case 0xC00A000B: return "STATUS_CTX_MODEM_RESPONSE_TIMEOUT";
    case 0xC00A000C: return "STATUS_CTX_MODEM_RESPONSE_NO_CARRIER";
    case 0xC00A000D: return "STATUS_CTX_MODEM_RESPONSE_NO_DIALTONE";
    case 0xC00A000E: return "STATUS_CTX_MODEM_RESPONSE_BUSY";
    case 0xC00A000F: return "STATUS_CTX_MODEM_RESPONSE_VOICE";
    case 0xC00A0010: return "STATUS_CTX_TD_ERROR";
    case 0xC00A0012: return "STATUS_CTX_LICENSE_CLIENT_INVALID";
    case 0xC00A0013: return "STATUS_CTX_LICENSE_NOT_AVAILABLE";
    case 0xC00A0014: return "STATUS_CTX_LICENSE_EXPIRED";
    case 0xC00A0015: return "STATUS_CTX_WINSTATION_NOT_FOUND";
    case 0xC00A0016: return "STATUS_CTX_WINSTATION_NAME_COLLISION";
    case 0xC00A0017: return "STATUS_CTX_WINSTATION_BUSY";
    case 0xC00A0018: return "STATUS_CTX_BAD_VIDEO_MODE";
    case 0xC00A0022: return "STATUS_CTX_GRAPHICS_INVALID";
    case 0xC00A0024: return "STATUS_CTX_NOT_CONSOLE";
    case 0xC00A0026: return "STATUS_CTX_CLIENT_QUERY_TIMEOUT";
    case 0xC00A0027: return "STATUS_CTX_CONSOLE_DISCONNECT";
    case 0xC00A0028: return "STATUS_CTX_CONSOLE_CONNECT";
    case 0xC00A002A: return "STATUS_CTX_SHADOW_DENIED";
    case 0xC00A002B: return "STATUS_CTX_WINSTATION_ACCESS_DENIED";
    case 0xC00A002E: return "STATUS_CTX_INVALID_WD";
    case 0xC00A002F: return "STATUS_CTX_WD_NOT_FOUND";
    case 0xC00A0030: return "STATUS_CTX_SHADOW_INVALID";
    case 0xC00A0031: return "STATUS_CTX_SHADOW_DISABLED";
    case 0xC00A0032: return "STATUS_RDP_PROTOCOL_ERROR";
    case 0xC00A0033: return "STATUS_CTX_CLIENT_LICENSE_NOT_SET";
    case 0xC00A0034: return "STATUS_CTX_CLIENT_LICENSE_IN_USE";
    case 0xC00A0035: return "STATUS_CTX_SHADOW_ENDED_BY_MODE_CHANGE";
    case 0xC00A0036: return "STATUS_CTX_SHADOW_NOT_RUNNING";
    case 0xC0040035: return "STATUS_PNP_BAD_MPS_TABLE";
    case 0xC0040036: return "STATUS_PNP_TRANSLATION_FAILED";
    case 0xC0040037: return "STATUS_PNP_IRQ_TRANSLATION_FAILED";
    case 0xC0150001: return "STATUS_SXS_SECTION_NOT_FOUND";
    case 0xC0150002: return "STATUS_SXS_CANT_GEN_ACTCTX";
    case 0xC0150003: return "STATUS_SXS_INVALID_ACTCTXDATA_FORMAT";
    case 0xC0150004: return "STATUS_SXS_ASSEMBLY_NOT_FOUND";
    case 0xC0150005: return "STATUS_SXS_MANIFEST_FORMAT_ERROR";
    case 0xC0150006: return "STATUS_SXS_MANIFEST_PARSE_ERROR";
    case 0xC0150007: return "STATUS_SXS_ACTIVATION_CONTEXT_DISABLED";
    case 0xC0150008: return "STATUS_SXS_KEY_NOT_FOUND";
    case 0xC0150009: return "STATUS_SXS_VERSION_CONFLICT";
    case 0xC015000A: return "STATUS_SXS_WRONG_SECTION_TYPE";
    case 0xC015000B: return "STATUS_SXS_THREAD_QUERIES_DISABLED";
    case 0xC015000C: return "STATUS_SXS_ASSEMBLY_MISSING";
    case 0x4015000D: return "STATUS_SXS_RELEASE_ACTIVATION_CONTEXT";
    case 0xC015000E: return "STATUS_SXS_PROCESS_DEFAULT_ALREADY_SET";
    case 0xC015000F: return "STATUS_SXS_EARLY_DEACTIVATION";
    case 0xC0150010: return "STATUS_SXS_INVALID_DEACTIVATION";
    case 0xC0150011: return "STATUS_SXS_MULTIPLE_DEACTIVATION";
    case 0xC0150012: return "STATUS_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT_EMPTY";
    case 0xC0150013: return "STATUS_SXS_PROCESS_TERMINATION_REQUESTED";
    case 0xC0130001: return "STATUS_CLUSTER_INVALID_NODE";
    case 0xC0130002: return "STATUS_CLUSTER_NODE_EXISTS";
    case 0xC0130003: return "STATUS_CLUSTER_JOIN_IN_PROGRESS";
    case 0xC0130004: return "STATUS_CLUSTER_NODE_NOT_FOUND";
    case 0xC0130005: return "STATUS_CLUSTER_LOCAL_NODE_NOT_FOUND";
    case 0xC0130006: return "STATUS_CLUSTER_NETWORK_EXISTS";
    case 0xC0130007: return "STATUS_CLUSTER_NETWORK_NOT_FOUND";
    case 0xC0130008: return "STATUS_CLUSTER_NETINTERFACE_EXISTS";
    case 0xC0130009: return "STATUS_CLUSTER_NETINTERFACE_NOT_FOUND";
    case 0xC013000A: return "STATUS_CLUSTER_INVALID_REQUEST";
    case 0xC013000B: return "STATUS_CLUSTER_INVALID_NETWORK_PROVIDER";
    case 0xC013000C: return "STATUS_CLUSTER_NODE_DOWN";
    case 0xC013000D: return "STATUS_CLUSTER_NODE_UNREACHABLE";
    case 0xC013000E: return "STATUS_CLUSTER_NODE_NOT_MEMBER";
    case 0xC013000F: return "STATUS_CLUSTER_JOIN_NOT_IN_PROGRESS";
    case 0xC0130010: return "STATUS_CLUSTER_INVALID_NETWORK";
    case 0xC0130011: return "STATUS_CLUSTER_NO_NET_ADAPTERS";
    case 0xC0130012: return "STATUS_CLUSTER_NODE_UP";
    case 0xC0130013: return "STATUS_CLUSTER_NODE_PAUSED";
    case 0xC0130014: return "STATUS_CLUSTER_NODE_NOT_PAUSED";
    case 0xC0130015: return "STATUS_CLUSTER_NO_SECURITY_CONTEXT";
    case 0xC0130016: return "STATUS_CLUSTER_NETWORK_NOT_INTERNAL";
    case 0xC0130017: return "STATUS_CLUSTER_POISONED";
    default:         return "STATUS_UNKNOWN";
    }
}

#endif // DBG
