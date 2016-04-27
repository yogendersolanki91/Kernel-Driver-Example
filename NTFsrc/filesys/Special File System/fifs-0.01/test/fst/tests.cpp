/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   file system tester tests

   This file is part of FIFS (Framework for Implementing File Systems). 

   This software is distributed with NO WARRANTY OF ANY KIND.  No
   author or distributor accepts any responsibility for the
   consequences of using it, or for whether it serves any particular
   purpose or works at all, unless he or she says so in writing.
   Refer to the included modified Alladin Free Public License (the
   "License") for full details.

   Every copy of this software must include a copy of the License, in
   a plain ASCII text file named COPYING.  The License grants you the
   right to copy, modify and redistribute this software, but only
   under certain conditions described in the License.  Among other
   things, the License requires that the copyright notice and this
   notice be preserved on all copies.

*/

#include <windows.h>
#include <stdio.h>

// #define TEST_DIR "t:/athena.mit.edu/user/d/a/dalmeida/Public/test"
#define GBUF_SIZE 128000

UCHAR gBuffer[GBUF_SIZE];

int
create(
    const char* program,
    const char* command,
    int argc,
    char *argv[]
    )
{
    if (argc < 1) {
        printf("Usage: %s %s <file>\n",
               program, command);
        return 1;
    }
    
    LPCSTR lpFileName = argv[0];
    DWORD dwDesiredAccess = 0; // GENERIC_READ, GENERIC_WRITE
    DWORD dwShareMode = 0; // FILE_SHARE_DELETE, FILE_SHARE_READ, FILE_SHARE_WRITE
    LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL;
    DWORD dwCreationDisposition = CREATE_NEW; // CREATE_NEW, CREATE_ALWAYS, OPEN_EXISTING, OPEN_ALWAYS, TRUNCATE_EXISTING
    DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
    // FILE_ATTRIBUTE_HIDDEN, FILE_ATTRIBUTE_NORMAL, FILE_ATTRIBUTE_OFFLINE, FILE_ATTRIBUTE_READONLY, FILE_ATTRIBUTE_SYSTEM, FILE_ATTRIBUTE_TEMPORARY
    // FILE_FLAG_WRITE_THROUGH, FILE_FLAG_OVERLAPPED, FILE_FLAG_NO_BUFFERING, FILE_FLAG_RANDOM_ACCESS, FILE_FLAG_SEQUENTIAL_SCAN, FILE_FLAG_DELETE_ON_CLOSE, FILE_FLAG_BACKUP_SEMANTICS, FILE_FLAG_POSIX_SEMANTICS
    HANDLE hTemplateFile = NULL;

    HANDLE hFile = CreateFile(
        lpFileName,// pointer to name of the file
        dwDesiredAccess,// access (read-write) mode
        dwShareMode,// share mode
        lpSecurityAttributes,// pointer to security attributes
        dwCreationDisposition,// how to create
        dwFlagsAndAttributes,// file attributes
        hTemplateFile // handle to file with attributes to copy
        );
    if (!hFile || (hFile == INVALID_HANDLE_VALUE)) {
        printf("Cannot create (%d)\n", GetLastError());
        return 1;
    }
    printf("Created %s\n", lpFileName);
    return 0;
}

int
df(
    const char* program,
    const char* command,
    int argc,
    char *argv[]
    )
{
    if (argc < 1) {
        printf("Usage: %s %s <drive>\n",
               program, command);
        return 1;
    }
    
    LPCSTR lpRootPathName = argv[0];
    DWORD dwSectorsPerCluster = 0;
    DWORD dwBytesPerSector = 0;
    DWORD dwNumberOfFreeClusters = 0;
    DWORD dwTotalNumberOfClusters = 0;
    
    if (!GetDiskFreeSpace(lpRootPathName,
                          &dwSectorsPerCluster,
                          &dwBytesPerSector,
                          &dwNumberOfFreeClusters,
                          &dwTotalNumberOfClusters)) {
        printf("Cannot get free space (%d)\n", GetLastError());
        return 1;
    }
    printf("Free Space:\n"
           "\tSectors/Cluster: %d\n"
           "\tBytes/Sector   : %d\n"
           "\tFree Clusters  : %d\n"
           "\tTotal Clusters : %d\n"
           "Free Bytes  = %d\n"
           "Total Bytes = %d\n",
           dwSectorsPerCluster,
           dwBytesPerSector,
           dwNumberOfFreeClusters,
           dwTotalNumberOfClusters,
           dwSectorsPerCluster * dwBytesPerSector * dwNumberOfFreeClusters,
           dwSectorsPerCluster * dwBytesPerSector * dwTotalNumberOfClusters);
    return 0;
}

int
df2(
    const char* program,
    const char* command,
    int argc,
    char *argv[]
    )
{
    if (argc < 1) {
        printf("Usage: %s %s <dir>\n",
               program, command);
        return 1;
    }
    
    LPCSTR lpDirectoryName = argv[0];
    ULARGE_INTEGER FreeBytesAvailableToCaller;
    ULARGE_INTEGER TotalNumberOfBytes;
    ULARGE_INTEGER TotalNumberOfFreeBytes;
    FreeBytesAvailableToCaller.QuadPart = 0;
    TotalNumberOfBytes.QuadPart = 0;
    TotalNumberOfFreeBytes.QuadPart = 0;

    if (!GetDiskFreeSpaceEx(lpDirectoryName,
                            &FreeBytesAvailableToCaller,
                            &TotalNumberOfBytes,
                            &TotalNumberOfFreeBytes)) {
        printf("Cannot get extended free space (%d)\n", GetLastError());
        return 1;
    }
    printf("Free Space:\n"
           "\tQuota Bytes Available: %I64d\n" 
           "\tFree  Disk Bytes     : %I64d\n"
           "\tTotal Disk Bytes     : %I64d\n",
           FreeBytesAvailableToCaller,
           TotalNumberOfFreeBytes,
           TotalNumberOfBytes);
    return 0;
}

int
write(
    const char* program,
    const char* command,
    int argc,
    char *argv[]
    )
{
    if (argc < 1) {
        printf("Usage: %s %s <file>\n",
               program, command);
        return 1;
    }
    
    LPCSTR lpFileName = argv[0];
    HANDLE hFile = CreateFile(lpFileName,
                              GENERIC_WRITE,
                              0,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (!hFile || (hFile == INVALID_HANDLE_VALUE)) {
        printf("Cannot create (%d)\n", GetLastError());
        return 1;
    }
    FillMemory(gBuffer, sizeof(gBuffer), 'A');
    DWORD dwCount = 0;
    if (!WriteFile(hFile, gBuffer, sizeof(gBuffer), &dwCount, NULL)) {
        printf("Cannot write (%d)\n", GetLastError());
        return 1;
    }
    printf("Bytes written: %d\n", dwCount);
    return 0;
}

int
contend(
    const char* program,
    const char* command,
    int argc,
    char *argv[]
    )
{
    if (argc < 1) {
        printf("Usage: %s %s <file>\n",
               program, command);
        return 1;
    }
    
    LPCSTR lpFileName = argv[0];
    HANDLE hFile = CreateFile(lpFileName,
                              GENERIC_WRITE | GENERIC_READ,
                              0,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (!hFile || (hFile == INVALID_HANDLE_VALUE)) {
        printf("Cannot create 1 (%d)\n", GetLastError());
        return 1;
    }
    HANDLE hFile2 = CreateFile(lpFileName,
                              GENERIC_WRITE | GENERIC_READ,
                              0,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (!hFile2 || (hFile2 == INVALID_HANDLE_VALUE)) {
        printf("Cannot create 2 (%d)\n", GetLastError());
        CloseHandle(hFile);
        return 1;
    }
    CloseHandle(hFile);
    CloseHandle(hFile2);
    printf("Success\n");
    return 0;
}

int
contend2(
    const char* program,
    const char* command,
    int argc,
    char *argv[]
    )
{
    if (argc < 1) {
        printf("Usage: %s %s <file>\n",
               program, command);
        return 1;
    }
    
    LPCSTR lpFileName = argv[0];
    HANDLE hFile = CreateFile(lpFileName,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (!hFile || (hFile == INVALID_HANDLE_VALUE)) {
        printf("Cannot create 1 (%d)\n", GetLastError());
        return 1;
    }
    HANDLE hFile2 = CreateFile(lpFileName,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (!hFile2 || (hFile2 == INVALID_HANDLE_VALUE)) {
        printf("Cannot create 2 (%d)\n", GetLastError());
        CloseHandle(hFile);
        return 1;
    }
    CloseHandle(hFile);
    CloseHandle(hFile2);
    printf("Success\n");
    return 0;
}

int
contend3(
    const char* program,
    const char* command,
    int argc,
    char *argv[]
    )
{
    if (argc < 1) {
        printf("Usage: %s %s <file>\n",
               program, command);
        return 1;
    }
    
    LPCSTR lpFileName = argv[0];
    HANDLE hFile = CreateFile(lpFileName,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (!hFile || (hFile == INVALID_HANDLE_VALUE)) {
        printf("Cannot create 1 (%d)\n", GetLastError());
        return 1;
    }
    HANDLE hFile2 = CreateFile(lpFileName,
                              0,
                              FILE_SHARE_READ,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (!hFile2 || (hFile2 == INVALID_HANDLE_VALUE)) {
        printf("Cannot create 2 (%d)\n", GetLastError());
        CloseHandle(hFile);
        return 1;
    }
    CloseHandle(hFile);
    CloseHandle(hFile2);
    printf("Success\n");
    return 0;
}
