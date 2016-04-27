#ifndef CORE_NT_DEFINITIONS_H
#define CORE_NT_DEFINITIONS_H

#include <ntddk_misc.h>
#include <ntddk_module.h>
//---------------------------------------------------------------------------------------
// versions

typedef struct _OSVERSIONINFOEXW {
    ULONG dwOSVersionInfoSize;
    ULONG dwMajorVersion;
    ULONG dwMinorVersion;
    ULONG dwBuildNumber;
    ULONG dwPlatformId;
    WCHAR  szCSDVersion[ 128 ];     // Maintenance string for PSS usage
    USHORT wServicePackMajor;
    USHORT wServicePackMinor;
    USHORT wSuiteMask;
    UCHAR wProductType;
    UCHAR wReserved;
} OSVERSIONINFOEXW_CORE, 
*POSVERSIONINFOEXW_CORE, 
*LPOSVERSIONINFOEXW_CORE, 
RTL_OSVERSIONINFOEXW_CORE, 
*PRTL_OSVERSIONINFOEXW_CORE;


typedef NTSTATUS
(*RtlGetVersionProc)(
    OUT  POSVERSIONINFOEXW_CORE lpVersionInformation
    );

//---------------------------------------------------------------------------------------
// Hooking supply
//---------------------------------------------------------------------------------------
#define SYSCALL_INDEX(_Function) *(PULONG)((PUCHAR)_Function+1)

typedef NTSTATUS (NTAPI *NTPROC) ();
typedef NTPROC *PNTPROC;
typedef struct _SYSTEM_SERVICE_TABLE
{
    /*000*/ PNTPROC ServiceTable;           // array of entry points
    /*004*/ ULONG *  CounterTable;           // array of usage counters
    /*008*/ ULONG   ServiceLimit;           // number of table entries
    /*00C*/ UCHAR *  ArgumentTable;          // array of byte counts
    /*010*/ 
}SYSTEM_SERVICE_TABLE, * PSYSTEM_SERVICE_TABLE,    **PPSYSTEM_SERVICE_TABLE;

typedef struct _SERVICE_DESCRIPTOR_TABLE
{
    /*000*/ SYSTEM_SERVICE_TABLE ntoskrnl;  // ntoskrnl.exe (native api)
    /*010*/ SYSTEM_SERVICE_TABLE win32k;    // win32k.sys   (gdi/user)
    /*020*/ SYSTEM_SERVICE_TABLE Table3;    // not used
    /*030*/ SYSTEM_SERVICE_TABLE Table4;    // not used
    /*040*/ 
}SERVICE_DESCRIPTOR_TABLE, * PSERVICE_DESCRIPTOR_TABLE,    **PPSERVICE_DESCRIPTOR_TABLE;

//---------------------------------------------------------------------------------------
// TOKENS
//---------------------------------------------------------------------------------------
NTSTATUS
ZwOpenProcessToken(
                   HANDLE Token,
                   ULONG TokenRights,
                   PVOID pointHandle
                   );

NTSTATUS 
ZwOpenThreadToken(IN HANDLE ThreadHandle, 
                  IN ACCESS_MASK DesiredAccess, 
                  IN BOOLEAN OpenAsSelf, 
                  OUT PHANDLE TokenHandle);

NTSTATUS
ZwQueryInformationToken(
                        HANDLE hToken,
                        TOKEN_INFORMATION_CLASS TokenInfoClass,
                        PVOID TokenInfoBuffer,
                        ULONG TokenInfoBufferLength,
                        PULONG BytesReturned
                        );
//---------------------------------------------------------------------------------------
// QUERIES
//---------------------------------------------------------------------------------------
typedef NTSTATUS (*ZwDelayExecutionProc)(IN BOOLEAN Alertable, IN PLARGE_INTEGER Interval);
//
// Process Device Map information
//  NtQueryInformationProcess using ProcessDeviceMap
//  NtSetInformationProcess using ProcessDeviceMap
//

typedef struct _PROCESS_DEVICEMAP_INFORMATION {
    union {
        struct {
            HANDLE DirectoryHandle;
        } Set;
        struct {
            ULONG DriveMap;
            UCHAR DriveType[ 32 ];
        } Query;
    };
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

//
// Process Information Classes
//

typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation,
    ProcessQuotaLimits,
    ProcessIoCounters,
    ProcessVmCounters,
    ProcessTimes,
    ProcessBasePriority,
    ProcessRaisePriority,
    ProcessDebugPort,
    ProcessExceptionPort,
    ProcessAccessToken,
    ProcessLdtInformation,
    ProcessLdtSize,
    ProcessDefaultHardErrorMode,
    ProcessIoPortHandlers,          // Note: this is kernel mode only
    ProcessPooledUsageAndLimits,
    ProcessWorkingSetWatch,
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup,
    ProcessPriorityClass,
    ProcessWx86Information,
    ProcessHandleCount,
    ProcessAffinityMask,
    ProcessPriorityBoost,
    ProcessDeviceMap,
    ProcessSessionInformation,
    ProcessForegroundInformation,
    ProcessWow64Information,
    MaxProcessInfoClass
} PROCESSINFOCLASS;

typedef struct _SYSTEM_MODULE
{
    ULONG dReserved01;
    ULONG d04;
    PVOID pAddress;
    ULONG dSize;                // bytes
    ULONG dFlags;
    USHORT  wId;                  // zero based
    USHORT  wRank;                // 0 if not assigned
    USHORT  w18;
    USHORT  wNameOffset;
    CHAR  abName [MAXIMUM_FILENAME_LENGTH];
} SYSTEM_MODULE, * PSYSTEM_MODULE, **PPSYSTEM_MODULE;

#define SYSTEM_MODULE_ \
sizeof (SYSTEM_MODULE)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct _SYSTEM_MODULE_INFORMATION
{
    ULONG         dCount;
    SYSTEM_MODULE aSM [1];
}
SYSTEM_MODULE_INFORMATION,
* PSYSTEM_MODULE_INFORMATION,
**PPSYSTEM_MODULE_INFORMATION;

#define SYSTEM_MODULE_INFORMATION_ \
sizeof (SYSTEM_MODULE_INFORMATION)

#define SYSCALL_INDEX(_Function) *(PULONG)((PUCHAR)_Function+1)

typedef unsigned __int64 QWORD;

typedef struct _SYSTEM_THREAD
{
    QWORD        qKernelTime;       // 100 nsec units
    QWORD        qUserTime;         // 100 nsec units
    QWORD        qCreateTime;       // relative to 01-01-1601
    ULONG        d18;
    PVOID        pStartAddress;
    CLIENT_ID    Cid;               // process/thread ids
    ULONG        dPriority;
    ULONG        dBasePriority;
    ULONG        dContextSwitches;
    ULONG        dThreadState;      // 2=running, 5=waiting
    KWAIT_REASON WaitReason;
    ULONG        dReserved01;
}
SYSTEM_THREAD,
* PSYSTEM_THREAD,
**PPSYSTEM_THREAD;

#define SYSTEM_THREAD_ \
sizeof (SYSTEM_THREAD)

typedef struct _SYSTEM_THREAD_INFORMATION {
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG WaitTime;
    PVOID StartAddress;
    CLIENT_ID ClientId;
    KPRIORITY Priority;
    LONG BasePriority;
    ULONG ContextSwitches;
    ULONG ThreadState;
    ULONG WaitReason;
} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;

//
// Process Virtual Memory read\write 
//

typedef
NTSTATUS 
(*ZwWriteVirtualMemoryProc)(
                            HANDLE ProcessHandle, 
                            PVOID BaseAddress, 
                            PVOID Buffer, 
                            ULONG BufferSize, 
                            PULONG NumberOfBytesWritten
                            );

typedef
NTSTATUS 
(*ZwReadVirtualMemoryProc)(
                           HANDLE ProcessHandle, 
                           PVOID BaseAddress,
                           PVOID Buffer,
                           ULONG BufferSize, 
                           PULONG NumberOfBytesRead
                           );

//
// Process Virtual Memory Counters
//  NtQueryInformationProcess using ProcessVmCounters
//

typedef struct _VM_COUNTERS {
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} VM_COUNTERS;
typedef VM_COUNTERS *PVM_COUNTERS;

typedef struct __SYSTEM_PROCESS_INFORMATION
{
    ULONG          dNext;           // relative offset
    ULONG          dThreadCount;
    ULONG          dReserved01;
    ULONG          dReserved02;
    ULONG          dReserved03;
    ULONG          dReserved04;
    ULONG          dReserved05;
    ULONG          dReserved06;
    QWORD          qCreateTime;     // relative to 01-01-1601
    QWORD          qUserTime;       // 100 nsec units
    QWORD          qKernelTime;     // 100 nsec units
    UNICODE_STRING usName;
    KPRIORITY      BasePriority;
    ULONG          dUniqueProcessId;
    ULONG          dInheritedFromUniqueProcessId;
    ULONG          dHandleCount;
    ULONG          dReserved07;
    ULONG          dReserved08;
    VM_COUNTERS    VmCounters;
    ULONG          dCommitCharge;   // bytes
    SYSTEM_THREAD  ast [1];
}
_SYSTEM_PROCESS_INFORMATION1,
* _PSYSTEM_PROCESS_INFORMATION1,
**_PPSYSTEM_PROCESS_INFORMATION1;

#define SYSTEM_PROCESS_INFORMATION_ \
sizeof (SYSTEM_PROCESS_INFORMATION)

typedef struct _SYSTEM_PROCESS_INFORMATION {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER SpareLi1;
    LARGE_INTEGER SpareLi2;
    LARGE_INTEGER SpareLi3;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG SpareUl3;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    ULONG PeakWorkingSetSize;
    ULONG WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    SYSTEM_THREAD_INFORMATION ThreadInfo[1];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;


typedef PVOID           POBJECT;
#define MAXPATHLEN     1024
#define MAXNTPATH 260

#define DRIVE_UNKNOWN     0
#define DRIVE_NO_ROOT_DIR 1
#define DRIVE_REMOVABLE   2
#define DRIVE_FIXED       3
#define DRIVE_REMOTE      4
#define DRIVE_CDROM       5
#define DRIVE_RAMDISK     6


typedef struct _OBJECT_DIRECTORY_INFORMATION {
    UNICODE_STRING ObjectName;
    UNICODE_STRING ObjectType;
    WCHAR          StringData[1];
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

typedef USHORT   SECURITY_DESCRIPTOR_CONTROL, *PSECURITY_DESCRIPTOR_CONTROL;


typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER Reserved1[2];
    ULONG Reserved2;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;
#define MAX_PROCESSOR   32

typedef struct _SYSTEM_BASIC_INFORMATION {
    CHAR Reserved1[24];
    PVOID Reserved2[4];
    CHAR NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION;

typedef struct _PROCESS_BASIC_INFORMATION {
    NTSTATUS  ExitStatus;
    PPEB  PebBaseAddress;
    KAFFINITY  AffinityMask;
    KPRIORITY  BasePriority;
    ULONG  UniqueProcessId;
    ULONG  InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;


typedef enum _SECTION_INFORMATION_CLASS
{
    SectionBasicInformation,
    SectionImageInformation
}SECTION_INFORMATION_CLASS;

typedef struct _SECTION_BASIC_INFORMATION
{
    PVOID BaseAddress;
    LARGE_INTEGER MaximumSize;
    ULONG AllocationAttributes;
} SECTION_BASIC_INFORMATION, *PSECTION_BASIC_INFORMATION;

typedef struct _SECTION_IMAGE_INFORMATION 
{
    ULONG     EntryPoint;
    ULONG     StackZeroBits;
    ULONG_PTR StackReserve;
    ULONG_PTR StackCommit;
    ULONG     Subsystem;
    USHORT    MinorSubsystemVersion;
    USHORT    MajorSubsystemVersion;
    ULONG     Unknown2;
    ULONG     Characteristics;
    USHORT    ImageNumber;
    BOOLEAN   Executable;
    UCHAR     Unknown3;
    ULONG     Unknown4[3];
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

#define SIZE_OF_TOKEN_INFORMATION                   \
sizeof( TOKEN_USER )                            \
+ sizeof( SID )                                 \
+ sizeof( ULONG ) * SID_MAX_SUB_AUTHORITIES



#define SEC_COMMIT        0x8000000

typedef enum _OBJECT_INFORMATION_CLASS
{
    ObjectBasicInformation,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectAllTypesInformation,
    ObjectHandleFlagInformation
} OBJECT_INFORMATION_CLASS;

typedef struct _OBJECT_BASIC_INFORMATION 
{
    ULONG                   Attributes;
    ACCESS_MASK             GrantedAccess;
    ULONG                   HandleCount;
    ULONG                   PointerCount;
    ULONG                   PagedPoolCharge;
    ULONG                   NonPagedPoolCharge;
    LARGE_INTEGER   CreationTime;
    ULONG     NameInfoSize;
    ULONG     TypeInfoSize;
    ULONG     SecurityDescriptorSize;
    CHAR                    Unknown2[8];
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;

//---------------------------------------------------------------------------------------
// COMMON PROTOTYPES
//---------------------------------------------------------------------------------------


NTKERNELAPI
NTSTATUS
ObOpenObjectByName(
                   IN POBJECT_ATTRIBUTES ObjectAttributes,
                   IN POBJECT_TYPE ObjectType,
                   IN KPROCESSOR_MODE AccessMode,
                   IN OUT PACCESS_STATE PassedAccessState OPTIONAL,
                   IN ACCESS_MASK DesiredAccess OPTIONAL,
                   IN OUT PVOID ParseContext OPTIONAL,
                   OUT PHANDLE Handle
                   );

NTSYSAPI 
NTSTATUS
NTAPI
ZwCreateSection (
                 OUT PHANDLE SectionHandle,
                 IN ACCESS_MASK DesiredAccess,
                 IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
                 IN PLARGE_INTEGER MaximumSize OPTIONAL,
                 IN ULONG SectionPageProtection,
                 IN ULONG AllocationAttributes,
                 IN HANDLE FileHandle OPTIONAL
                 );

NTSYSAPI
NTSTATUS
NTAPI
ZwUnmapViewOfSection(
                     IN HANDLE ProcessHandle,
                     IN PVOID BaseAddress
                     );

NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateVirtualMemory(
                        IN HANDLE ProcessHandle,
                        IN OUT PVOID *BaseAddress,
                        IN ULONG ZeroBits,
                        IN OUT PSIZE_T RegionSize,
                        IN ULONG AllocationType,
                        IN ULONG Protect
                        );

NTSYSAPI
NTSTATUS
NTAPI
ZwReadVirtualMemory (
                            IN HANDLE ProcessHandle,
                            IN PVOID BaseAddress,
                            OUT PVOID Buffer,
                            IN ULONG BufferSize,
                            OUT PULONG NumberOfBytesRead 
                            );

NTSYSAPI
NTSTATUS
NTAPI
ZwFreeVirtualMemory(
                    IN HANDLE ProcessHandle,
                    IN OUT PVOID *BaseAddress,
                    IN OUT PSIZE_T RegionSize,
                    IN ULONG FreeType
                    );
NTSYSAPI 
NTSTATUS
NTAPI
ZwQueryDirectoryFile(
                     IN HANDLE FileHandle,
                     IN HANDLE Event OPTIONAL,
                     IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
                     IN PVOID ApcContext OPTIONAL,
                     OUT PIO_STATUS_BLOCK IoStatusBlock,
                     OUT PVOID FileInformation,
                     IN ULONG Length,
                     IN FILE_INFORMATION_CLASS FileInformationClass,
                     IN BOOLEAN ReturnSingleEntry,
                     IN PUNICODE_STRING FileName OPTIONAL,
                     IN BOOLEAN RestartScan
                     );

NTSYSAPI 
NTSTATUS
NTAPI
ZwQueryInformationProcess(
                          IN HANDLE ProcessHandle,
                          IN PROCESSINFOCLASS ProcessInformationClass,
                          OUT PVOID ProcessInformation,
                          IN ULONG ProcessInformationLength,
                          OUT PULONG ReturnLength OPTIONAL
                          );
NTSTATUS
ObReferenceObjectByName(
                        IN PUNICODE_STRING ObjectName,
                        IN ULONG Attributes,
                        IN PACCESS_STATE PassedAccessState OPTIONAL,
                        IN ACCESS_MASK DesiredAccess OPTIONAL,
                        IN POBJECT_TYPE ObjectType,
                        IN KPROCESSOR_MODE AccessMode,
                        IN OUT PVOID ParseContext OPTIONAL,
                        OUT PVOID *Object
                        );
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenDirectoryObject(
                      OUT PHANDLE DirectoryHandle,
                      IN ACCESS_MASK DesiredAccess,
                      IN POBJECT_ATTRIBUTES ObjectAttributes
                      );
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryObject (
                        IN HANDLE       DirectoryHandle,
                        OUT PVOID       Buffer,
                        IN ULONG        Length,
                        IN BOOLEAN      ReturnSingleEntry,
                        IN BOOLEAN      RestartScan,
                        IN OUT PULONG   Context,
                        OUT PULONG      ReturnLength OPTIONAL
                        );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSymbolicLinkObject(
                         OUT PHANDLE LinkHandle,
                         IN ACCESS_MASK DesiredAccess,
                         IN POBJECT_ATTRIBUTES ObjectAttributes
                         );

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySymbolicLinkObject(
                          IN HANDLE LinkHandle,
                          IN OUT PUNICODE_STRING LinkTarget,
                          OUT PULONG ReturnedLength OPTIONAL
                          );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryVolumeInformationFile(
                             IN HANDLE FileHandle,
                             OUT PIO_STATUS_BLOCK IoStatusBlock,
                             OUT PVOID FsInformation,
                             IN ULONG Length,
                             IN FS_INFORMATION_CLASS FsInformationClass
                             );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenFile(
           OUT PHANDLE FileHandle,
           IN ACCESS_MASK DesiredAccess,
           IN POBJECT_ATTRIBUTES ObjectAttributes,
           OUT PIO_STATUS_BLOCK IoStatusBlock,
           IN ULONG ShareAccess,
           IN ULONG OpenOptions
           );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtClose(
        IN HANDLE Handle
        );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenProcess (
                    OUT PHANDLE ProcessHandle,
                    IN ACCESS_MASK DesiredAccess,
                    IN POBJECT_ATTRIBUTES ObjectAttributes,
                    IN PCLIENT_ID ClientId OPTIONAL
                    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQuerySection(
                    IN HANDLE SectionHandle,
                    IN SECTION_INFORMATION_CLASS SectionInformationClass,
                    OUT PVOID SectionInformation,
                    IN ULONG SectionInformationLength,
                    OUT PULONG ReturnLength OPTIONAL
                    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwFsControlFile(
                     IN HANDLE FileHandle,
                     IN HANDLE Event OPTIONAL,
                     IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
                     IN PVOID ApcContext OPTIONAL,
                     OUT PIO_STATUS_BLOCK IoStatusBlock,
                     IN ULONG FsControlCode,
                     IN PVOID InputBuffer OPTIONAL,
                     IN ULONG InputBufferLength,
                     OUT PVOID OutputBuffer OPTIONAL,
                     IN ULONG OutputBufferLength
                     );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryObject (
                    IN HANDLE Handle,
                    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
                    OUT PVOID ObjectInformation,
                    IN ULONG ObjectInformationLength,
                    OUT PULONG ReturnLength OPTIONAL
                    );

NTKERNELAPI 
NTSTATUS 
ObQueryNameString(PVOID Object, 
                  POBJECT_NAME_INFORMATION ObjectNameInfo, 
                  ULONG Length, 
                  PULONG ReturnLength);


NTSTATUS 
ZwOpenEvent(
            OUT PHANDLE  EventHandle,
            IN ACCESS_MASK  DesiredAccess,
            IN POBJECT_ATTRIBUTES  ObjectAttributes
            );

NTSTATUS
ZwNotifyChangeKey(
  IN HANDLE  KeyHandle,
  IN HANDLE  Event,
  IN PIO_APC_ROUTINE  ApcRoutine,
  IN PVOID  ApcContext,
  OUT PIO_STATUS_BLOCK  IoStatusBlock,
  IN ULONG  CompletionFilter,
  IN BOOLEAN  WatchTree,
  OUT PVOID  Buffer,
  IN ULONG  BufferSize,
  IN BOOLEAN  Asynchronous
  );

//---------------------------------------------------------------------------------------
// PROCESSES AND HANDLES
//---------------------------------------------------------------------------------------

#define REG_NOTIFY_CHANGE_NAME          0x00000001L
#define REG_NOTIFY_CHANGE_ATTRIBUTES    0x00000002L
#define REG_NOTIFY_CHANGE_LAST_SET      0x00000004L
#define REG_NOTIFY_CHANGE_SECURITY      0x00000008L

typedef struct _EX_PUSH_LOCK 
{
    union
    {
        struct
        {
            ULONG Waiting   :0x01;
            ULONG Exclusive :0x01;
            ULONG Shared    :0x1E;
        };

        ULONG Value;
        PVOID Ptr;
    };
} EX_PUSH_LOCK, *PEX_PUSH_LOCK;
typedef struct _HANDLE_TRACE_DB_ENTRY 
{
    CLIENT_ID ClientId;
    HANDLE    Handle;
    ULONG     Type;
    PVOID     StackTrace[16];
} HANDLE_TRACE_DB_ENTRY, *PHANDLE_TRACE_DB_ENTRY;

typedef PVOID PHANDLE_TABLE_ENTRY_INFO;

typedef struct _HANDLE_TRACE_DEBUG_INFO 
{
    ULONG                 CurrentStackIndex;
    HANDLE_TRACE_DB_ENTRY TraceDb[4096];
} HANDLE_TRACE_DEBUG_INFO, *PHANDLE_TRACE_DEBUG_INFO;

typedef struct _HANDLE_TABLE_ENTRY 
{
    union 
    {
        PVOID                    Object;
        ULONG                    ObAttributes;
        PHANDLE_TABLE_ENTRY_INFO InfoTable;
        ULONG                    Value;
    };

    union 
    {
        union 
        {
            ACCESS_MASK GrantedAccess;

            struct 
            {
                USHORT GrantedAccessIndex;
                USHORT CreatorBackTraceIndex;
            };
        };

        LONG NextFreeTableEntry;
    };

} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;

typedef struct _EXHANDLE 
{
    union 
    {
        struct 
        {
            ULONG TagBits : 02;
            ULONG Index   : 30;
        };

        HANDLE GenericHandleOverlay;
    };

} EXHANDLE, *PEXHANDLE;

typedef PVOID PHANDLE_TABLE;

typedef struct _XP_HANDLE_TABLE 
{
    ULONG                    TableCode;
    PEPROCESS                QuotaProcess;
    PVOID                    UniqueProcessId;
    EX_PUSH_LOCK             HandleTableLock[4];
    LIST_ENTRY               HandleTableList;
    EX_PUSH_LOCK             HandleContentionEvent;
    PHANDLE_TRACE_DEBUG_INFO DebugInfo;
    LONG                     ExtraInfoPages;
    ULONG                    FirstFree;
    ULONG                    LastFree;
    ULONG                    NextHandleNeedingPool;
    LONG                     HandleCount;
    LONG                     Flags;
    UCHAR                    StrictFIFO;
} XP_HANDLE_TABLE, *PXP_HANDLE_TABLE;


typedef struct _WIN2K_HANDLE_TABLE 
{
    ULONG                 Flags;
    LONG                  HandleCount;
    PHANDLE_TABLE_ENTRY **Table;
    PEPROCESS             QuotaProcess;
    HANDLE                UniqueProcessId;
    LONG                  FirstFreeTableEntry;
    LONG                  NextIndexNeedingPool;
    ERESOURCE             HandleTableLock;
    LIST_ENTRY            HandleTableList;
    KEVENT                HandleContentionEvent;
} WIN2K_HANDLE_TABLE , *PWIN2K_HANDLE_TABLE ;
// handle table constants
#define WIN2K_TABLE_ENTRY_LOCK_BIT    0x80000000
#define TABLE_LEVEL_MASK              3
#define XP_TABLE_ENTRY_LOCK_BIT       1

NTSYSAPI
NTSTATUS
NTAPI
PsLookupProcessByProcessId(
                           IN HANDLE ProcessId,
                           OUT PEPROCESS *Process
                           );

NTSYSAPI
NTSTATUS
NTAPI
IoGetRequestorSessionId(
                        IN PIRP Irp,
                        OUT PULONG pSessionId
                        );


typedef struct _SERVICE_STATUS {
    ULONG   dwServiceType;
    ULONG   dwCurrentState;
    ULONG   dwControlsAccepted;
    ULONG   dwWin32ExitCode;
    ULONG   dwServiceSpecificExitCode;
    ULONG   dwCheckPoint;
    ULONG   dwWaitHint;
} SERVICE_STATUS, *LPSERVICE_STATUS;

typedef struct _IMAGE_RECORD {
    struct _IMAGE_RECORD    *Prev;              // linked list
    struct _IMAGE_RECORD    *Next;              // linked list
    LPWSTR                  ImageName;          // fully qualified .exe name
    ULONG                   Pid;                // Process ID
    ULONG                   ServiceCount;       // Num services running in process
    HANDLE                  PipeHandle;         // Handle to Service
    HANDLE                  ProcessHandle;      // Handle for process
    HANDLE                  ObjectWaitHandle;   // Handle for waiting on the process
    HANDLE                  TokenHandle;        // Logon token handle
    HANDLE                  ProfileHandle;      // User profile handle
}IMAGE_RECORD, *PIMAGE_RECORD, *LPIMAGE_RECORD;

typedef struct _LOAD_ORDER_GROUP {
    struct _LOAD_ORDER_GROUP    *Next;
    struct _LOAD_ORDER_GROUP    *Prev;
    LPWSTR                      GroupName;
    ULONG                       RefCount;

} LOAD_ORDER_GROUP, *PLOAD_ORDER_GROUP, *LPLOAD_ORDER_GROUP;

typedef struct _UNRESOLVED_DEPEND {
    struct _UNRESOLVED_DEPEND *Next;
    struct _UNRESOLVED_DEPEND *Prev;
    LPWSTR                    Name;     // Service or group name
    ULONG                     RefCount;
} UNRESOLVED_DEPEND, *PUNRESOLVED_DEPEND, *LPUNRESOLVED_DEPEND;

typedef enum _DEPEND_TYPE {
    TypeDependOnService = 128,
    TypeDependOnGroup,
    TypeDependOnUnresolved  // only for service
} DEPEND_TYPE, *PDEPEND_TYPE, *LPDEPEND_TYPE;

typedef struct _DEPEND_RECORD {
    struct _DEPEND_RECORD   *Next;
    DEPEND_TYPE             DependType;
    union {
        struct _SERVICE_RECORD *    DependService;
        struct _LOAD_ORDER_GROUP *  DependGroup;
        struct _UNRESOLVED_DEPEND * DependUnresolved;
        PVOID                      Depend; // used when type doesn't matter
    };
} DEPEND_RECORD, *PDEPEND_RECORD, *LPDEPEND_RECORD;

typedef struct _SERVICE_RECORD {
    struct _SERVICE_RECORD  *Prev;          // linked list
    struct _SERVICE_RECORD  *Next;          // linked list
    LPWSTR                  ServiceName;    // points to service name
    LPWSTR                  DisplayName;    // points to display name
    ULONG                   ResumeNum;      // Ordered number for this rec
    ULONG                   ServerAnnounce; // Server announcement bit flags
    ULONG                   Signature;      // Identifies this as a service record.
    ULONG                   UseCount;       // How many open handles to service
    ULONG                   StatusFlag;     // status(delete,update...)
    union {
        LPIMAGE_RECORD      ImageRecord;    // Points to image record
        LPWSTR              ObjectName;     // Points to driver object name
    };
    SERVICE_STATUS          ServiceStatus;  // see winsvc.h
    ULONG                   StartType;      // AUTO, DEMAND, etc.
    ULONG                   ErrorControl;   // NORMAL, SEVERE, etc.
    ULONG                   Tag;            // ULONG Id for the service,0=none.
    LPDEPEND_RECORD         StartDepend;
    LPDEPEND_RECORD         StopDepend;
    LPWSTR                  Dependencies;
    PSECURITY_DESCRIPTOR    ServiceSd;
    ULONG                   StartError;
    ULONG                   StartState;
    LPLOAD_ORDER_GROUP      MemberOfGroup;
    LPLOAD_ORDER_GROUP      RegistryGroup;
} SERVICE_RECORD, *PSERVICE_RECORD, *LPSERVICE_RECORD;

//----------------------------------------------------------------------------------------
// OTHER
//----------------------------------------------------------------------------------------

typedef struct _PEB_LDR_DATA {
    ULONG Length;
    BOOLEAN Initialized;
    HANDLE SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _RTL_DRIVE_LETTER_CURDIR
{
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    UNICODE_STRING DosPath;
}RTL_DRIVE_LETTER_CURDIR;

typedef struct CURDIR
{
    UNICODE_STRING DosPath;
    PVOID Handle;
}CURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS 
{
    ULONG MaximumLength;
    ULONG Length;
    ULONG Flags;
    ULONG DebugFlags;
    PVOID ConsoleHandle;
    ULONG ConsoleFlags;
    HANDLE StandardInput;
    PVOID StandardOutput;
    PVOID StandardError;
    struct CURDIR CurrentDirectory;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
    PVOID Environment;
    ULONG StartingX;      
    ULONG StartingY;      
    ULONG CountX;         
    ULONG CountY;         
    ULONG CountCharsX;
    ULONG CountCharsY;    
    ULONG FillAttribute;  
    ULONG WindowFlags;    
    ULONG ShowWindowFlags;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING DesktopInfo;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeData;
    RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[32];
} RTL_USER_PROCESS_PARAMETERS, * PRTL_USER_PROCESS_PARAMETERS;

typedef struct _PEB_FREE_BLOCK {
    struct _PEB_FREE_BLOCK *Next;
    ULONG Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;
    UCHAR    Padding[0x10];
    PVOID    DllBase;
    UCHAR    Padding2[0x08];
    UNICODE_STRING    FullDllName;
    UNICODE_STRING    BaseDllName;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _NTPEB
{
    CHAR InheritedAddressSpace;
    CHAR ReadImageFileExecOptions;
    CHAR BeingDebugged;
    CHAR SpareBool;
    void* Mutant;
    void* ImageBaseAddress;
    struct _PEB_LDR_DATA* Ldr;
    struct _RTL_USER_PROCESS_PARAMETERS* ProcessParameters;
    void* SubSystemData;
    void* ProcessHeap;
    void* FastPebLock;
    void* FastPebLockRoutine;
    void* FastPebUnlockRoutine;
    ULONG EnvironmentUpdateCount;
    void* KernelCallbackTable;
    ULONG SystemReserved[2];
    struct _PEB_FREE_BLOCK* FreeList;
    ULONG TlsExpansionCounter;
    void* TlsBitmap;
    ULONG TlsBitmapBits[2];
    void* ReadOnlySharedMemoryBase;
    void* ReadOnlySharedMemoryHeap;
    void** ReadOnlyStaticServerData;
    void* AnsiCodePageData;
    void* OemCodePageData;
    void* UnicodeCaseTableData;
    ULONG NumberOfProcessors;
    ULONG NtGlobalFlag;
    LARGE_INTEGER CriticalSectionTimeout;
    ULONG HeapSegmentReserve;
    ULONG HeapSegmentCommit;
    ULONG HeapDeCommitTotalFreeThreshold;
    ULONG HeapDeCommitFreeBlockThreshold;
    ULONG NumberOfHeaps;
    ULONG MaximumNumberOfHeaps;
    void** ProcessHeaps;
    void* GdiSharedHandleTable;
    void* ProcessStarterHelper;
    ULONG GdiDCAttributeList;
    void* LoaderLock;
    ULONG OSMajorVersion;
    ULONG OSMinorVersion;
    USHORT OSBuildNumber;
    USHORT OSCSDVersion;
    ULONG OSPlatformId;
    ULONG ImageSubsystem;
    ULONG ImageSubsystemMajorVersion;
    ULONG ImageSubsystemMinorVersion;
    ULONG ImageProcessAffinityMask;
    ULONG GdiHandleBuffer[34];
    void* PostProcessInitRoutine;
    void* TlsExpansionBitmap;
    ULONG TlsExpansionBitmapBits[32];
    ULONG SessionId;
    ULARGE_INTEGER AppCompatFlags;
    void* pShimData;
    void* AppCompatInfo;
    UNICODE_STRING CSDVersion;
    void* ActivationContextData;
    void* ProcessAssemblyStorageMap;
    void* SystemDefaultActivationContextData;
    void* SystemAssemblyStorageMap;
    ULONG MinimumStackCommit;
} NTPEB, *PNTPEB;
PNTPEB GetPEB();

typedef VOID (*OB_DELETE_METHOD)(
                                 IN PVOID Object
                                 );
typedef PVOID OB_DUMP_METHOD;
typedef PVOID OB_OPEN_METHOD;
typedef PVOID OB_CLOSE_METHOD;
typedef PVOID OB_PARSE_METHOD;
typedef PVOID OB_SECURITY_METHOD;
typedef PVOID OB_QUERYNAME_METHOD;
typedef PVOID OB_OKAYTOCLOSE_METHOD;
typedef struct _OBJECT_TYPE_INITIALIZER 
{
    USHORT Length;
    // ligen: commented - if you want to use smth from there go to coreOffsets
    //BOOLEAN UseDefaultObject;
    //BOOLEAN Reserved;
    //ULONG InvalidAttributes;
    //GENERIC_MAPPING GenericMapping;
    //ULONG ValidAccessMask;
    //BOOLEAN SecurityRequired;
    //BOOLEAN MaintainHandleCount;
    //BOOLEAN MaintainTypeList;
    //POOL_TYPE PoolType;
    //ULONG DefaultPagedPoolCharge;
    //ULONG DefaultNonPagedPoolCharge;
    //OB_DUMP_METHOD DumpProcedure;
    //OB_OPEN_METHOD OpenProcedure;  
    //OB_CLOSE_METHOD CloseProcedure;
    //OB_DELETE_METHOD DeleteProcedure;
    //OB_PARSE_METHOD ParseProcedure;
    //OB_SECURITY_METHOD SecurityProcedure;
    //OB_QUERYNAME_METHOD QueryNameProcedure;
    //OB_OKAYTOCLOSE_METHOD OkayToCloseProcedure;
} OBJECT_TYPE_INITIALIZER, *POBJECT_TYPE_INITIALIZER;
typedef struct _OBJECT_TYPE 
{
    int dummy;
} OBJECT_TYPE, *POBJECT_TYPE;
typedef struct _OBJECT_HEADER  
{ 
    int dummy; 
} OBJECT_HEADER, *POBJECT_HEADER;

#define OBJECT_TO_OBJECT_HEADER(o) CONTAINING_RECORD((o), OBJECT_HEADER, Body)


// oops
typedef void * SECTION_OBJECT , **PSECTION_OBJECT;

#define IOCTL_VOLUME_BASE   ((ULONG) 'V')

#define IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS    CTL_CODE(IOCTL_VOLUME_BASE, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _DISK_EXTENT {
  ULONG DiskNumber;
  LARGE_INTEGER StartingOffset;
  LARGE_INTEGER ExtentLength;
} DISK_EXTENT, 
 *PDISK_EXTENT;

typedef struct _VOLUME_DISK_EXTENTS {
  ULONG NumberOfDiskExtents;
  DISK_EXTENT Extents[ANYSIZE_ARRAY];
} VOLUME_DISK_EXTENTS, 
 *PVOLUME_DISK_EXTENTS;

typedef PDEVICE_OBJECT
  (*IoGetDeviceAttachmentBaseRefProc)(
    IN PDEVICE_OBJECT  DeviceObject
    ); 

//security
typedef struct _CORE_TOKEN_LINKED_TOKEN {
  HANDLE LinkedToken;
} CORE_TOKEN_LINKED_TOKEN, *PCORE_TOKEN_LINKED_TOKEN;

typedef enum _CORE_TOKEN_INFORMATION_CLASS {
    CORE_TokenUser = 1,
    CORE_TokenGroups,
    CORE_TokenPrivileges,
    CORE_TokenOwner,
    CORE_TokenPrimaryGroup,
    CORE_TokenDefaultDacl,
    CORE_TokenSource,
    CORE_TokenType,
    CORE_TokenImpersonationLevel,
    CORE_TokenStatistics,
    CORE_TokenRestrictedSids,
    CORE_TokenSessionId,
    CORE_TokenGroupsAndPrivileges,
    CORE_TokenSessionReference,
    CORE_TokenSandBoxInert,
    CORE_TokenAuditPolicy,
    CORE_TokenOrigin,
    CORE_TokenElevationType,
    CORE_TokenLinkedToken,
    CORE_TokenElevation,
    CORE_TokenHasRestrictions,
    CORE_TokenAccessInformation,
    CORE_TokenVirtualizationAllowed,
    CORE_TokenVirtualizationEnabled,
    CORE_TokenIntegrityLevel,
    CORE_TokenUIAccess,
    CORE_TokenMandatoryPolicy,
    CORE_TokenLogonSid,
    CORE_MaxTokenInfoClass  // MaxTokenInfoClass should always be the last enum
} CORE_TOKEN_INFORMATION_CLASS, *PCORE_TOKEN_INFORMATION_CLASS;

typedef enum _CORE_TOKEN_ELEVATION_TYPE {
    TokenElevationTypeDefault = 1,
    TokenElevationTypeFull,
    TokenElevationTypeLimited,
} CORE_TOKEN_ELEVATION_TYPE, *PCORE_TOKEN_ELEVATION_TYPE;

typedef struct _CORE_TOKEN_ELEVATION {
    unsigned long TokenIsElevated;
} CORE_TOKEN_ELEVATION, *PCORE_TOKEN_ELEVATION;

#define Core_ThreadImpersonationToken 5

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationThread (
    IN HANDLE ThreadHandle,
    IN int ThreadInformationClass,
    PVOID ThreadInformation,
    IN ULONG ThreadInformationLength
    );



NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateToken(
    IN HANDLE ExistingTokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN EffectiveOnly,
    IN TOKEN_TYPE TokenType,
    OUT PHANDLE NewTokenHandle
    );

#endif
