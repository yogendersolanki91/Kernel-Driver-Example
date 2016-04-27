/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   NetBIOS snooper main

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
#include "config.h"
#include "dumpsmb.h"

#define EXTRA_DUMP_LINES 100
#define DUMP_SIZE (0x10000/16*80+EXTRA_DUMP_LINES*80)

static HMODULE hModule = NULL;
static DWORD dwCount = 0;
static FARPROC pFunction = NULL;
static HANDLE hFile = NULL;
static BOOL bLoaded = FALSE;

static CRITICAL_SECTION CS;

static DWORD dwTlsIndex = 0xFFFFFFFF;

static
void
ErrorBox(
    LPCSTR string
    )
{
    MessageBox(NULL, string, config.csBoxTitle.string, MB_OK);
}

BOOL WINAPI 
DllMain(
    HINSTANCE hinstDLL, // handle to DLL module
    DWORD fdwReason, // reason for calling function
    LPVOID lpvReserved // reserved
    )
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        if (bLoaded) {
            dwCount++;
            return TRUE;
        }
        dwTlsIndex = TlsAlloc();
        if (dwTlsIndex == 0xFFFFFFFF)
            return FALSE;
        InitConfig();
        hModule = LoadLibrary(config.csRealPath.string);
        pFunction = GetProcAddress(hModule, "Netbios");
        if (!pFunction) {
            if (config.bShowLoad) ErrorBox("Load Failed!");
            return FALSE;
        }
        hFile = CreateFile(config.csLogFile.string,
                           GENERIC_WRITE,
                           FILE_SHARE_READ,
                           NULL,
                           CREATE_ALWAYS,
                           0,
                           NULL);
        if (!hFile || (hFile == INVALID_HANDLE_VALUE)) {
            if (config.bShowLoad) ErrorBox("Load Failed!");
            return FALSE;
        }
        InitializeCriticalSection(&CS);
        if (config.bShowLoad) ErrorBox("Load Ok!");
        dwCount++;
        bLoaded = TRUE;
        InitDump();
        break;
    case DLL_PROCESS_DETACH:
        FreeLibrary(hModule);
        CloseHandle(hFile);
        CleanupConfig();
        CleanupDump();
        free(TlsGetValue(dwTlsIndex));
        TlsFree(dwTlsIndex);
        dwCount--;
        break;
    case DLL_THREAD_ATTACH: {
        break;
    }
    case DLL_THREAD_DETACH:
    {
        free(TlsGetValue(dwTlsIndex));
        break;
    }
    }
    return TRUE;
}

static
LPVOID GetBuffer()
{
    LPVOID Buffer;

    Buffer = TlsGetValue(dwTlsIndex);
    if (!Buffer && !GetLastError()) {
        Buffer = malloc(DUMP_SIZE*sizeof(UCHAR));
        if (!TlsSetValue(dwTlsIndex, Buffer)) {
            free(Buffer);
            Buffer = NULL;
        }
    }
    return Buffer;
}

static
BOOL
LogDump(
    PUCHAR Buffer,
    DWORD dwSize
    )
{
    DWORD dwCount;
    BOOL bResult;
    if (!Buffer) return FALSE;
    EnterCriticalSection(&CS);
    bResult = WriteFile(hFile, Buffer, dwSize, &dwCount, NULL);
    LeaveCriticalSection(&CS);
    return bResult;
}

static
void
LogError(
    PUCHAR Buffer, 
    BOOL bResult
    )
{
    if (!Buffer)
        ErrorBox("Error getting buffer for Netbios log!");
    else if (!bResult)
        ErrorBox("Error writing Netbios log!");
    return;
}

UCHAR
WINAPI
Netbios(
    PNCB p
    )
{
    PUCHAR Buffer = GetBuffer();
    PUCHAR buffer;
    UCHAR result;

    if (Buffer)
        buffer = DumpNCB(Buffer, p, TRUE);
    result = pFunction(p);
    if (Buffer) {
        buffer = DumpNCB(buffer, p, FALSE);
        buffer[0] = '\n';
        buffer[1] = 0;
        buffer++;
    }
    LogError(Buffer, LogDump(Buffer, buffer-Buffer));
    return result;
}

VOID
WINAPI
DumpSmb(
    PVOID pSmb,
    DWORD dwSize,
    BOOL bRequest
    )
{
    PUCHAR Buffer = GetBuffer();
    PUCHAR buffer;

    Buffer = GetBuffer();

    if (Buffer) {
        buffer = DumpSMB(Buffer, pSmb, dwSize, bRequest);
        buffer[0] = '\n';
        buffer[1] = 0;
        buffer++;
    }
    LogError(Buffer, LogDump(Buffer, buffer-Buffer));
}
