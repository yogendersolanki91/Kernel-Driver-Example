/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   NetBIOS tester pass-through test

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
#include <nb30.h>
#include <stdio.h>
#include "ncbutil.h"
#include "nbhelp.h"
#include <process.h>
#include <assert.h>
#include "buffers.h"

CRITICAL_SECTION CS;

VOID
NBDump2(
    PNCB pncb
    )
{
    EnterCriticalSection(&CS);
    NBDump(pncb);
    LeaveCriticalSection(&CS);
}

VOID
puts2(
    LPCSTR szLine
    )
{
    EnterCriticalSection(&CS);
    puts(szLine);
    LeaveCriticalSection(&CS);
}

#define puts puts2
#define NBDump NBDump2

#define DEFAULT_BUFFER_BLOCK_SIZE 64000

typedef struct _CONNECTION_INFO {
    int nLana;
    int from_lsn, to_lsn;
    NETBIOS_NAME local_name, from_name, to_name;
} CONNECTION_INFO, *PCONNECTION_INFO;

void
ConnectionThread(
    PCONNECTION_INFO pContext
    )
{
    NCB ncb;
    int length = 0;
    UCHAR default_buffer[DEFAULT_BUFFER_BLOCK_SIZE];
    BUFFER_LIST Buffers;

    // Initialize...

    Buffers_Initialize(&Buffers, default_buffer, sizeof(default_buffer));

    // Receive stuff...NEED SOME EXIT CONDITION WHEN THE CONNECTION DIES...

    while (1) {
        InitializeNCB(&ncb,
                      NCBRECV,
                      pContext->nLana,
                      pContext->from_lsn,
                      pContext->from_name,
                      pContext->local_name);
        ncb.ncb_buffer = Buffers_GetNodeBuffer(&Buffers);
        ncb.ncb_length = Buffers_GetNodeSize(&Buffers);

        NBDump (&ncb);
        Netbios (&ncb);
        NBCheck (&ncb);
        NBDump (&ncb);

        if (ncb.ncb_retcode == NRC_INCOMP) {
            length += ncb.ncb_length;
            if (!Buffers_AddNode(&Buffers, sizeof(default_buffer)))
                goto alloc_buffer_error;
        } else {
            // send buffers...
            PUCHAR send_buffer = default_buffer;
            BOOL free_buffer = FALSE;
            if (Buffers_IsMultiNode(&Buffers)) {
                if(!Buffers_Merge(&Buffers, length, &send_buffer))
                    goto alloc_buffer_error;
                Buffers_Cleanup(&Buffers);
            }
            // send here...
            InitializeNCB(&ncb,
                          NCBSEND,
                          pContext->nLana,
                          pContext->to_lsn,
                          pContext->to_name,
                          pContext->local_name);
            // cleanup...
            if (send_buffer != default_buffer) {
                free(send_buffer);
            }
            length = 0;
        }
    }
    assert(1);  // should never get here...
    goto cleanup;

alloc_buffer_error:
    puts("Critital malloc error while allocating extra buffers!");
    goto cleanup;

cleanup:
    Buffers_Cleanup(&Buffers);
    free(pContext);
    puts("Exiting forward thread...");
    return;
}

void
StartConnectionThread(
    PCONNECTION_INFO pContext
    )
{
    PCONNECTION_INFO pContext2 = NULL;
    NCB ncb;

    // Call target...

    InitializeNCB(&ncb,
                  NCBCALL,
                  pContext->nLana,
                  0,
                  pContext->to_name,
                  pContext->local_name);

    NBDump (&ncb);
    Netbios (&ncb);
    NBCheck (&ncb);
    NBDump (&ncb);

    if (ncb.ncb_retcode != NRC_GOODRET) {
        puts("Could not call forwarding target!");
        goto cleanup;
    }

    pContext->to_lsn = ncb.ncb_lsn;

    // Spawn another thread to handle reverse direction...

    puts("Spawning reverse thread...");
    pContext2 = malloc(sizeof(CONNECTION_INFO));
    if (!pContext2) {
        puts("Could not allocate thread context!");
        goto cleanup;
    }
    memset(pContext2, 0, sizeof(CONNECTION_INFO));

    pContext2->from_lsn = pContext->to_lsn;
    pContext2->to_lsn = pContext->from_lsn;

    COPY_NETBIOS_NAME(pContext2->local_name,
                      pContext->local_name);
    COPY_NETBIOS_NAME(pContext2->from_name,
                      pContext->to_name);
    COPY_NETBIOS_NAME(pContext2->to_name,
                      pContext->from_name);

    if (_beginthread(ConnectionThread, 0, pContext2) < 0) {
        puts("Could not create reverse thread!");
        goto cleanup;
    }
    pContext2 = NULL;

    ConnectionThread(pContext);
    pContext = NULL;

cleanup:
    free(pContext2);
    free(pContext);
    puts("Exiting connection thread...");
    return;
}

DWORD
NBPass(
    int nLana, 
    LPCSTR szMyName,
    LPCSTR szListenTo,
    LPCSTR szPassTo
    )
{
    DWORD RC = ERROR_SUCCESS;
    NCB ncb;
    NETBIOS_NAME local_name, listen_to, pass_to;
    int local_num;
    PCONNECTION_INFO pContext = NULL;

    MakeNetbiosName(local_name, szMyName);
    MakeNetbiosName(listen_to, szListenTo);
    MakeNetbiosName(pass_to, szPassTo);

    // Add name

    InitializeNCB(&ncb, 
                  NCBADDNAME, 
                  nLana, 
                  0,
                  NULL, local_name);

    Netbios (&ncb);
    NBCheck (&ncb);

    if (NRC_GOODRET != ncb.ncb_retcode) return ncb.ncb_retcode;

    local_num = ncb.ncb_num;

    // Listen for connection (is this really needed?)

    while (1) {
        //
        // NOTE: Ideally, we should just fix up fields that can change...
        //

        InitializeNCB(&ncb,
                      NCBLISTEN,
                      nLana,
                      0,
                      listen_to,
                      local_name);
//        ncb.ncb_num = local_num;

        NBDump (&ncb);
        Netbios (&ncb);
        NBCheck (&ncb);
        NBDump (&ncb);

        if (ncb.ncb_retcode == NRC_GOODRET) {
            // spawn thread...
            puts("Spawning connection thread...");
            pContext = malloc(sizeof(CONNECTION_INFO));
            if (!pContext) {
                puts("Could not allocate thread context!");
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            memset(pContext, 0, sizeof(CONNECTION_INFO));
            pContext->nLana = nLana;
            pContext->from_lsn = ncb.ncb_lsn;
            COPY_NETBIOS_NAME(pContext->local_name,
                              local_name);
            COPY_NETBIOS_NAME(pContext->from_name,
                              ncb.ncb_callname);
            COPY_NETBIOS_NAME(pContext->to_name,
                              pass_to);
            if (_beginthread(StartConnectionThread, 0, pContext) < 0) {
                puts("Could not create reverse thread!");
                free(pContext);
                return errno;
            }
            pContext = NULL;
        }
    }
    assert(1); // should never get here...
}


int
Main_PassThrough(
    const char* program,
    const char* command,
    int argc,
    char *argv[]
    )
{
    DWORD RC;
    int lana_num;
    char *node_name;
    char *listen_to;
    char *pass_to;

    if ((argc < 3) || (argc > 4)) {
        printf("Usage: %s %s <lana_num> <node_name> <pass_to> "
               "[listen_to]\n", program, command);
        return 1;
    }

    lana_num = atoi(argv[0]);
    node_name = argv[1];
    pass_to = argv[2];
    if (argc > 3) {
        listen_to = argv[3];
    } else {
        listen_to = "*";
    }

    InitializeCriticalSection(&CS);

    if (!NBReset(lana_num, MAX_SESSIONS, MAX_NAMES)) return 1;
    RC = NBPass(lana_num, node_name, listen_to, pass_to);
    if (!RC)
        puts("Received Ok!");
    return RC;
}

int
Main_Test(
    const char* program,
    const char* command,
    int argc,
    char *argv[]
    )
{
    DWORD RC;
    int lana_num;
    char *node_name;
    char *listen_to;
    char *pass_to;

    if ((argc < 3) || (argc > 4)) {
        printf("Usage: %s %s <lana_num> <node_name> <pass_to> "
               "[listen_to]\n", program, command);
        return 1;
    }

    lana_num = atoi(argv[0]);
    node_name = argv[1];
    pass_to = argv[2];
    if (argc > 3) {
        listen_to = argv[3];
    } else {
        listen_to = "*";
    }

    InitializeCriticalSection(&CS);

    if (!NBReset(lana_num, MAX_SESSIONS, MAX_NAMES)) return 1;
    if (!NBAddName(lana_num, node_name)) return 1;
    {
        NCB ncb;
        int from_lsn, to_lsn;
        int nLana = lana_num;
        NETBIOS_NAME orig_name, local_name, target_name;

        MakeNetbiosName(orig_name, listen_to);
        MakeNetbiosName(local_name, node_name);
        MakeNetbiosName(target_name, pass_to);

        InitializeNCB(&ncb,
                      NCBLISTEN,
                      nLana,
                      0,
                      orig_name,
                      local_name);
        NBDump(&ncb);
        Netbios(&ncb);
        NBCheck(&ncb);
        NBDump(&ncb);
        from_lsn = ncb.ncb_lsn;
        if (ncb.ncb_retcode)
            return ncb.ncb_retcode;
        
        InitializeNCB(&ncb,
                      NCBCALL,
                      nLana,
                      0,
                      target_name,
                      orig_name);
        NBDump(&ncb);
        Netbios(&ncb);
        NBCheck(&ncb);
        NBDump(&ncb);
        to_lsn = ncb.ncb_lsn;
        if (ncb.ncb_retcode)
            return ncb.ncb_retcode;
        RC = 0;
    }
    return RC;
}
