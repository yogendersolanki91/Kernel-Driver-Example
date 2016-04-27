/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   NetBIOS tester tests

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

BOOL
NBListen(
    int nLana, 
    LPCSTR szListenFor,
    LPCSTR szMyName
    )
{
    NCB ncb;

    memset (&ncb, 0, sizeof (ncb));
    ncb.ncb_command = NCBLISTEN;
    ncb.ncb_lana_num = nLana;

    MakeNetbiosName (ncb.ncb_callname, szListenFor);
    MakeNetbiosName (ncb.ncb_name, szMyName);

    NBDump (&ncb);
    Netbios (&ncb);
    NBCheck (&ncb);
    NBDump (&ncb);
    return (NRC_GOODRET == ncb.ncb_retcode);    
}

BOOL
NBRecv(
    int nLana, 
    LPCSTR szListenFor,
    LPCSTR szMyName
    )
{
    NCB ncb;
    int lsn;
    UCHAR remote_name[NCBNAMSZ];
    UCHAR local_name[NCBNAMSZ];
    UCHAR Buffer[64000];

    MakeNetbiosName(local_name, szMyName);

    // Add name

    memset (&ncb, 0, sizeof(ncb));
    ncb.ncb_command = NCBADDNAME;
    ncb.ncb_lana_num = nLana;

    memcpy(ncb.ncb_name, local_name, NCBNAMSZ);

    Netbios (&ncb);
    NBCheck (&ncb);

    if (NRC_GOODRET != ncb.ncb_retcode) return FALSE;

    // Listen for connection (is this really needed?)

    memset (&ncb, 0, sizeof (ncb));
    ncb.ncb_command = NCBLISTEN;
    ncb.ncb_lana_num = nLana;

    MakeNetbiosName (ncb.ncb_callname, szListenFor);
    memcpy(ncb.ncb_name, local_name, NCBNAMSZ);

    NBDump (&ncb);
    Netbios (&ncb);
    NBCheck (&ncb);
    NBDump (&ncb);

    if (NRC_GOODRET != ncb.ncb_retcode) return FALSE;

    // save results...

    memcpy(remote_name, ncb.ncb_callname, NCBNAMSZ);
    lsn = ncb.ncb_lsn;

    // Receive stuff...

    memset (&ncb, 0, sizeof (ncb));
    ncb.ncb_command = NCBRECV;
    ncb.ncb_lana_num = nLana;
    ncb.ncb_lsn = lsn;
    ncb.ncb_buffer = Buffer;
    ncb.ncb_length = sizeof(Buffer);

    memcpy(ncb.ncb_callname, remote_name, NCBNAMSZ);
    memcpy(ncb.ncb_name, local_name, NCBNAMSZ);

    NBDump (&ncb);
    Netbios (&ncb);
    NBCheck (&ncb);
    NBDump (&ncb);

    return (NRC_GOODRET == ncb.ncb_retcode);    
}

/*------------------------------------------------------------------*/

int
Main_EnumLana(
    const char* program,
    const char* command,
    int argc,
    char *argv[]
    )
{
    int lana_num;

    if (argc < 1) {
        printf("Usage: %s %s <lana_num>\n", program, command);
        return 1;
    }

    lana_num = atoi(argv[0]);

    //if (!NBReset(lana_num, MAX_SESSIONS, MAX_NAMES)) return 1;
    if (!NBEnumLana(lana_num)) return 1;
    puts("Listened Ok!");
    return 0;
}

int
Main_Listen(
    const char* program,
    const char* command,
    int argc,
    char *argv[]
    )
{
    int lana_num;
    char *node_name;
    char *listen_to;

    if ((argc < 2) || (argc > 3)) {
        printf("Usage: %s %s <lana_num> <node_name> "
               "[listen_to]\n", program, command);
        return 1;
    }

    lana_num = atoi(argv[0]);
    node_name = argv[1];
    if (argc > 2) {
        listen_to = argv[2];
    } else {
        listen_to = "*";
    }

    if (!NBReset(lana_num, MAX_SESSIONS, MAX_NAMES)) return 1;
    if (!NBAddName(lana_num, node_name)) return 1;
    if (!NBListen(lana_num, listen_to, node_name)) return 1;
    puts("Listened Ok!");
    return 0;
}

int
Main_ListNames(
    const char* program,
    const char* command,
    int argc,
    char *argv[]
    )
{
    int lana_num;
    char *node_name;
    BOOL bAddName;

    if (argc < 2) {
        printf("Usage: %s %s <lana_num> <node_name> [add]\n", 
               program, command);
        return 1;
    }
    if (argc > 2) {
        bAddName = TRUE;
    } else {
        bAddName = FALSE;
    }

    lana_num = atoi(argv[0]);
    node_name = argv[1];

    if (!NBReset (lana_num, 20, 30)) return 1;
    if (bAddName && !NBAddName (lana_num, node_name)) return 1;
    if (!NBListNames (lana_num, node_name)) return 1;
    puts("Succeeded.");
    return 0;
}

int
Main_Recv(
    const char* program,
    const char* command,
    int argc,
    char *argv[]
    )
{
    int lana_num;
    char *node_name;
    char *listen_to;

    if ((argc < 2) || (argc > 3)) {
        printf("Usage: %s %s <lana_num> <node_name> "
               "[listen_to]\n", program, command);
        return 1;
    }

    lana_num = atoi(argv[0]);
    node_name = argv[1];
    if (argc > 2) {
        listen_to = argv[2];
    } else {
        listen_to = "*";
    }

    if (!NBReset(lana_num, MAX_SESSIONS, MAX_NAMES)) return 1;
//    if (!NBAddName(lana_num, node_name)) return 1;
    if (!NBRecv(lana_num, listen_to, node_name)) return 1;
    puts("Received Ok!");
    return 0;
}

int
Main_Call(
    const char* program,
    const char* command,
    int argc,
    char *argv[]
    )
{
    int lana_num;
    char *caller;
    char *callee;
    BOOL bAddName;

    if ((argc < 3) || (argc > 4)) {
        printf("Usage: %s %s <lana_num> <caller> <callee> [add]\n",
               program, command);
        return 1;
    }

    lana_num = atoi(argv[0]);
    caller = argv[1];
    callee = argv[2];
    bAddName = (argc > 3);

    if (!NBReset(lana_num, MAX_SESSIONS, MAX_NAMES)) return 1;
    if (bAddName && !NBAddName(lana_num, caller)) return 1;
    {
        NCB ncb;
        NETBIOS_NAME caller_name, callee_name;

        MakeNetbiosName(caller_name, caller);
        MakeNetbiosName(callee_name, callee);

        InitializeNCB(&ncb,
                      NCBCALL,
                      lana_num,
                      0,
                      callee_name,
                      caller_name);
        NBDump(&ncb);
        Netbios(&ncb);
        NBCheck(&ncb);
        NBDump(&ncb);

        if (ncb.ncb_retcode) return 1;
    }
    printf("Called Ok!\n");
    return 0;
}

int
Main_Add(
    const char* program,
    const char* command,
    int argc,
    char *argv[]
    )
{
    int lana_num;
    int seconds;
    char *name;

    if (argc != 3) {
        printf("Usage: %s %s <lana_num> <name> <seconds>\n",
               program, command);
        return 1;
    }

    lana_num = atoi(argv[0]);
    name = argv[1];
    seconds = atoi(argv[2]);

    if (!NBReset(lana_num, MAX_SESSIONS, MAX_NAMES)) return 1;
    if (!NBAddName(lana_num, name)) return 1;
    printf("Sleeping...\n");
    Sleep(seconds * 1000);
    printf("Added Ok!\n");
    return 0;
}
