/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   SMB server

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

#include "smball.hxx"
#include "config.hxx"
#include "nbhelp.hxx"
#include "server.hxx"
#include "queue.hxx"
#include <debug.hxx>
#include <process.h> // for _beginthreadex
//#include <assert.h>

struct GlobalContext {
    DWORD connection;     // monotonically increasing (read/write)
    UCHAR lsn;            // the current lsn (read/write)
    CRITICAL_SECTION cs;
    BOOL running; // read-only...changes state once
    UCHAR nLana;  // read-only after first write
};

struct ReceiverContext {
    HANDLE hUnblockEvent;
};

static GlobalContext gc = { 0 };
static ReceiverContext grc;

static Queue WorkQ;
static Queue SendQ;
static Queue BufferQ;
static Queue PacketQ;

static HANDLE* hThreads;
static DWORD nThreads;
static HANDLE hEvent;

#define THREADAPI unsigned int WINAPI

static HINSTANCE hFs = 0;
FileSystem* pFs = 0;

THREADAPI
ReceiverThread(
    LPVOID lpParameter
    )
{
    DFN(ReceiverThread);

    ReceiverContext* rc = (ReceiverContext*)lpParameter;
    Packet* p = NULL;
    HANDLE hEvents[2];
    UCHAR code;
    NCB ncb;
    DWORD waitobj;
    bool reconnect;

    DEBUG_PRINT(("started\n"));

    hEvents[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
    hEvents[1] = rc->hUnblockEvent;
    BOOL first = TRUE;

    while (gc.running) {
        DEBUG_PRINT(("running...\n"));

        ResetEvent(hEvents[0]);
        code = NBListenExN(gc.nLana, 
                           config.debug?(PUCHAR)"*":config.nb_remote_name,
                           config.nb_local_name, 
                           0, NULL, hEvents[0], &ncb);
        if (!NB_SUCCESS(code)) {
            DEBUG_PRINT(("listening error 0x%02x\n", code));
            continue;  // need to abort somehow?...
        }

        DEBUG_PRINT(("listening...\n"));
        waitobj = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
        if (waitobj == WAIT_OBJECT_0) {
            if (NB_SUCCESS(ncb.ncb_retcode)) {
                DEBUG_PRINT(("got a call on lsn %d!\n", ncb.ncb_lsn));
                EnterCriticalSection(&gc.cs);
                gc.connection++;
                gc.lsn = ncb.ncb_lsn;
                LeaveCriticalSection(&gc.cs);

                // we got a call, so we now receive...
                reconnect = 0;
                while(gc.running && !reconnect) {
                    if (!p) {
                        p = (Packet*) PacketQ.Get();
                        p->buffer = BufferQ.Get();
                    }
                    EnterCriticalSection(&gc.cs);
                    p->connection = gc.connection;
                    p->lsn = gc.lsn;
                    LeaveCriticalSection(&gc.cs);
                    p->len = (USHORT)config.buffer_size;
                    ResetEvent(hEvents[0]);
                    DEBUG_PRINT(("receiving...\n"));
                    code = NBRecvEx(gc.nLana, p->lsn, p->buffer, &p->len, 
                                    hEvents[0], &ncb);
                    if (NB_BADSESSION(code)) {
                        DEBUG_PRINT(("session %d died...restarting...\n", 
                                     p->lsn));
                        reconnect = 1;
                    } else if (!NB_SUCCESS(code)) {
                        DEBUG_PRINT(("Netbios error 0x%02x\n", code));
                    } else {
                        DEBUG_PRINT(("waiting...\n"));
                        waitobj = WaitForMultipleObjects(2, hEvents, FALSE, 
                                                         INFINITE);
                        DEBUG_PRINT(("received!\n"));
                        if (waitobj == WAIT_OBJECT_0) {
                            if (NB_BADSESSION(ncb.ncb_retcode)) {
                                DEBUG_PRINT(("session %d died...restartin'...\n", 
                                             p->lsn));
                                reconnect = 1;
                            } else if (NB_SUCCESS(ncb.ncb_retcode)) {
                                p->len = ncb.ncb_length;
                                WorkQ.Add(p);
                                p = 0;
                            } else {
                                DEBUG_PRINT(("NBRecv error %d\n",
                                             ncb.ncb_retcode));
                            }
                        } else {
                            NBCancel(&ncb);
                        }
                    }
                }

            } else {
                DEBUG_PRINT(("asynch listening error 0x%02x\n", 
                             ncb.ncb_retcode));
            }
        } else {
            NBCancel(&ncb);
        }
    }
    DEBUG_PRINT(("exiting\n"));
    return 0;
}


THREADAPI
WorkerThread(
    LPVOID lpParameter
    )
{
    DFN(WorkerThread);
    bool printwait = true;
    DPB dpb;
    DEBUG_PRINT(("started\n"));
    while (gc.running) {
        if (printwait) DEBUG_PRINT(("waiting...\n"));
        dpb.p = (Packet*) WorkQ.Get();
        if (printwait = dpb.p) {
            DEBUG_PRINT(("working...\n"));
            if (IsSmb(dpb.p->buffer, dpb.p->len)) {

                dpb.in.smb = (PNT_SMB_HEADER)dpb.p->buffer;
                dpb.in.size = dpb.p->len;
                dpb.in.offset = sizeof(NT_SMB_HEADER);
                dpb.in.command = dpb.in.smb->Command;

                dpb.out.smb = (PNT_SMB_HEADER)BufferQ.Get();
                dpb.out.size = config.buffer_size;
                dpb.out.valid = 0;

                DumpSmb(dpb.p->buffer, dpb.p->len, TRUE);
                DEBUG_PRINT(("dispatching...\n"));

                BOOL disp = SmbDispatch(&dpb);
                BufferQ.Add(dpb.in.smb); // == dpb.p->buffer

		// If we handled it ok...
                if (disp) {
                    dpb.p->buffer = (LPVOID)dpb.out.smb;
                    dpb.p->len = (USHORT)dpb.out.valid;
                    // DumpSmb(dpb.p->buffer, dpb.p->len, FALSE);
                    SendQ.Add(dpb.p);
                    dpb.p = 0;
                } else {
                    DEBUG_PRINT(("dispatch failed!\n"));
                    // did not understand...hangup on virtual circuit...
                    EnterCriticalSection(&gc.cs);
                    if (dpb.p->connection == gc.connection) {
                        DEBUG_PRINT(("hangup! -- disp failed on lsn %d\n", 
                                     dpb.p->lsn));
                        NBHangup(gc.nLana, dpb.p->lsn);
                    }
                    LeaveCriticalSection(&gc.cs);
                    BufferQ.Add(dpb.out.smb);
                    PacketQ.Add(dpb.p);
                    dpb.p = 0;
                }
            } else {
                BufferQ.Add(dpb.p->buffer);
                PacketQ.Add(dpb.p);
                dpb.p = 0;
            }
        }
        DEBUG_PRINT(("worked!\n"));
    }
    DEBUG_PRINT(("exiting\n"));
    return 0;
}

THREADAPI
SenderThread(
    LPVOID lpParameter
    )
{
    DFN(SenderThread);
    bool printwait = true;
    bool sending;
    Packet* p;
    UCHAR code;

    DEBUG_PRINT(("started\n"));
    while (gc.running) {
        if (printwait) DEBUG_PRINT(("waiting...\n"));
        p = (Packet*) SendQ.Get();
        if (printwait = p) {
            DEBUG_PRINT(("sending...\n"));
            EnterCriticalSection(&gc.cs);
            if (sending = (p->connection == gc.connection))
                code = NBSend(gc.nLana, p->lsn, p->buffer, &p->len);
            LeaveCriticalSection(&gc.cs);
            if (sending) {
                if (!NB_SUCCESS(code)) {
                    DEBUG_PRINT(("NBSend error %d\n", code));
                }
            } else {
                DEBUG_PRINT(("stale connection %d\n", p->connection));
            }
            BufferQ.Add(p->buffer);
            PacketQ.Add(p);
            p = 0;
        }
    }
    DEBUG_PRINT(("exiting\n"));
    return 0;
}

int
InitServer()
{
    DFN(InitServer);
    hFs = LoadLibrary(config.fs_dll);
    if (!hFs) {
        DWORD error = GetLastError();
        DEBUG_PRINT(("could not get filesystem (0x%02x)\n", error));
        return FALSE;
    }
    FS_CREATE_PROC pFsCreate = 
        (FS_CREATE_PROC)GetProcAddress(hFs, FS_CREATE_PROC_NAME);
    if (!pFsCreate) {
        DWORD error = GetLastError();
        DEBUG_PRINT(("could not get filesystem create function (0x%02x)\n", 
                     error));
        return FALSE;
    }
    DWORD error = pFsCreate(config.fs_name, config.fs_config, 
                            FS_VERSION_0, &pFs);
    if (error) {
        DEBUG_PRINT(("could not instantiate filesystem interface (0x%08X)\n",
                     error));
        return FALSE;
    }

    gc.connection = 0;
    gc.lsn = 0;
    InitializeCriticalSection(&gc.cs);
    gc.running = FALSE;

    UCHAR code = NBFindFirstLanaWithNameN(config.nb_remote_name, &gc.nLana);
    if(!NB_SUCCESS(code)) {
        DEBUG_PRINT(("NBReset/Find error (0x%02x)\n", code));
        return FALSE;
    }

    if (!NB_SUCCESS(NBAddNameN(gc.nLana, config.nb_local_name, NULL))) {
        DEBUG_PRINT(("NBAddName error\n"));
        return FALSE;
    }

    //
    // Start up threads in suspended mode
    //

    const int nFixedThreads = 1;
    // start up 1 listener/receiver, a few workers, a few senders....
//    gc.running = TRUE;
    grc.hUnblockEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    nThreads = nFixedThreads + config.num_workers + config.num_senders;
    hThreads = new HANDLE[nThreads];
    hThreads[0] = (HANDLE)
        _beginthreadex(NULL, 0, &ReceiverThread, &grc, CREATE_SUSPENDED, NULL);
    DWORD i = nFixedThreads;
    for ( ; i < (config.num_workers + nFixedThreads); i++)
        hThreads[i] = (HANDLE)
            _beginthreadex(NULL, 0, &WorkerThread, NULL, CREATE_SUSPENDED, NULL);
    for ( ; i < nThreads; i++)
        hThreads[i] = (HANDLE)
            _beginthreadex(NULL, 0, &SenderThread, NULL, CREATE_SUSPENDED, NULL);

    // Create 2 buffers per worker, 1 per send, 1 per receiver...plus some 
    DWORD nBuffers = 1 + 2*config.num_workers + config.num_senders + 3;
    for (i = 0; i < nBuffers; i++) {
        void* buffer = xmalloc(config.buffer_size);
        if (!buffer) {
            DEBUG_PRINT(("out of memory when creating buffers!\n"));
            return FALSE;
        }
        BufferQ.Add(buffer);
    }

    DWORD nPackets = 1 + config.num_workers + config.num_senders + 3;
    for (i = 0; i < nPackets; i++) {
        void* p = new Packet;
        if (!p) {
            DEBUG_PRINT(("out of memory when creating packets!\n"));
            return FALSE;
        }
        PacketQ.Add(p);
    }

    smbutil_init();
    smbglue_init();
    return TRUE;
}

int
RunServer()
{
    gc.running = TRUE;
    for (DWORD i = 0; i < nThreads; i++)
        ResumeThread(hThreads[i]);
    return TRUE;
}

void CleanupServer()
{
    DFN(CleanupServer);
    if (gc.running) {
        gc.running = FALSE;
        DEBUG_PRINT(("waiting for threads to die off...\n"));
        SetEvent(grc.hUnblockEvent);
        PacketQ.Unblock();
        BufferQ.Unblock();
        WorkQ.Unblock();
        SendQ.Unblock();
        // wait for them to die of natural causes before we kill them...
        WaitForMultipleObjects(nThreads, hThreads, TRUE, config.kill_wait_time);
        for (DWORD i = 0; i < nThreads; i++) {
            TerminateThread(hThreads[i], 0); // in case they did not stop...
            CloseHandle(hThreads[i]);
        }
        CloseHandle(hEvent);
        delete [] hThreads;

        // empty Qs...
        PacketQ.Unblock();
        BufferQ.Unblock();
        WorkQ.Unblock();
        SendQ.Unblock();
        Packet* p;
        while (p = (Packet*) PacketQ.Get(0))
            delete p;
        while (p = (Packet*) WorkQ.Get(0))
            delete p;
        while (p = (Packet*) SendQ.Get(0))
            delete p;
        void* b;
        while (b = BufferQ.Get(0))
            xfree(b);
    }
    smbglue_cleanup();
    smbutil_cleanup();
    if (hFs) FreeLibrary(hFs); // must do this last!
}
