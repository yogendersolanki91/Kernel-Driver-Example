/*
    Program to unload a file system driver.
    Copyright (C) 1999-2015 Bo Brantén.
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

    This program sends a private IOCTL to a file system driver to let it
    prepare for unloading and then calls the service control manager to
    actually unload it.
*/

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>

#ifndef _PREFAST_
#pragma warning(disable:4068)
#endif // _PREFAST_

#pragma prefast( disable: 28719, "this warning only applies to drivers not applications" )

#define IOCTL_PREPARE_TO_UNLOAD \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 2048, METHOD_NEITHER, FILE_WRITE_ACCESS)

void PrintLastError(char* Prefix)
{
    LPVOID lpMsgBuf;

    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        0,
        (LPTSTR) &lpMsgBuf,
        0,
        NULL
        );

    fprintf(stderr, "%s: %s", Prefix, (LPTSTR) lpMsgBuf);

    LocalFree(lpMsgBuf);
}

int __cdecl main(int argc, char* argv[])
{
    char            FileName[MAX_PATH + 1] = "\\\\.\\";
    HANDLE          Device;
    DWORD           BytesReturned;
    SC_HANDLE       SCManager;
    SC_HANDLE       Service;
    SERVICE_STATUS  ServiceStatus;

    if (argc < 2)
    {
        fprintf(stderr, "syntax: unload <driver>\n");
        return -1;
    }

    strncat(FileName, argv[1], MAX_PATH - strlen(FileName));

    Device = CreateFile(
        FileName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (Device == INVALID_HANDLE_VALUE)
    {
        PrintLastError(FileName);
        return -1;
    }

    if (!DeviceIoControl(
        Device,
        IOCTL_PREPARE_TO_UNLOAD,
        NULL,
        0,
        NULL,
        0,
        &BytesReturned,
        NULL
        ))
    {
        PrintLastError(FileName);
        return -1;
    }

    CloseHandle(Device);

    if ((SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)) == NULL)
    {
        PrintLastError("Service control manager");
        return -1;
    }

    if ((Service = OpenService(SCManager, argv[1], SERVICE_ALL_ACCESS)) == NULL)
    {
        PrintLastError(argv[1]);
        return -1;
    }

    if (!ControlService(Service, SERVICE_CONTROL_STOP, &ServiceStatus))
    {
        PrintLastError(argv[1]);
        return -1;
    }

    if (ServiceStatus.dwCurrentState != SERVICE_STOPPED)
    {
        SetLastError(ERROR_SERVICE_REQUEST_TIMEOUT);
        PrintLastError(argv[1]);
        return -1;
    }

    return 0;
}
