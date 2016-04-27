/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   server main

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
#include "config.hxx"
#include "server.hxx"
#include <debug.hxx>

static HANDLE hEvent;

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    switch(dwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        SetEvent(hEvent);
        return TRUE;
    default:
        return FALSE;
    }
}

void Exit()
{
    CleanupServer();
    exit(1);
}

void main(int argc, char* argv[])
{
    DFN(main);
    InitConfig((argc > 1)?argv[1]:0);
    if (!InitServer()) Exit();
    if (!RunServer()) Exit();
    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    SetConsoleCtrlHandler(HandlerRoutine, TRUE);
    WaitForSingleObject(hEvent, INFINITE);
    DEBUG_PRINT(("cleaning up server\n"));
    CloseHandle(hEvent);
    CleanupServer();
    CleanupConfig();
}
