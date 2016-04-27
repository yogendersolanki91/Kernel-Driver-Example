#ifndef CORE_MNT_INTERFACE_H_INCLUDED
#define CORE_MNT_INTERFACE_H_INCLUDED

#pragma pack(1)

#define ROOT_DIR_NAME        L"\\Device\\CoreMntDevDir"
#define DIRECT_DISK_PREFIX  ROOT_DIR_NAME L"\\disk"

#define SECTOR_SIZE 512
#define TOC_DATA_TRACK          0x04

#define MOUNTDEVCONTROLTYPE                 ((ULONG)'M')
#define IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_READWRITE CTL_CODE(MOUNTDEVCONTROLTYPE, 1, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define CORE_MNT_DISPATCHER        0x8001

#define CORE_MNT_MOUNT_IOCTL \
    CTL_CODE(CORE_MNT_DISPATCHER, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define CORE_MNT_EXCHANGE_IOCTL \
    CTL_CODE(CORE_MNT_DISPATCHER, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define CORE_MNT_UNMOUNT_IOCTL \
    CTL_CODE(CORE_MNT_DISPATCHER, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef enum _DiskOperationType
{
    directOperationSuccess = 0,
    directOperationEmpty = 0,
    directOperationRead,
    directOperationWrite,
    directOperationFail,
    directOperationMax = directOperationFail
}DiskOperationType;

struct CORE_MNT_MOUNT_REQUEST
{
    ULONG64 totalLength;
    WCHAR   mountPojnt;
};

struct CORE_MNT_MOUNT_RESPONSE
{
    ULONG32 deviceId;
};

struct CORE_MNT_EXCHANGE_REQUEST
{
    ULONG32 deviceId;
    ULONG32 lastType; 
    ULONG32 lastStatus; 
    ULONG32 lastSize; 
    char *  data;
    ULONG32 dataSize;
};

struct CORE_MNT_EXCHANGE_RESPONSE
{
    ULONG32 type;
    ULONG32 size; 
    ULONG64 offset; 
};

struct CORE_MNT_UNMOUNT_REQUEST
{
    ULONG32 deviceId;
};
#pragma pack()

#endif
