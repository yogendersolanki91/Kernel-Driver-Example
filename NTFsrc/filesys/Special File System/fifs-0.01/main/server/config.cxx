/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   server configuration

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
#include "nbhelp.hxx"
#include "config.hxx"
#include <conf_help.hxx>
#include <debug.hxx>

config_type config;

static const char DEFAULT_CONFIG_PATH[] = "\\Registry\\HKEY_LOCAL_MACHINE\\Software\\Etc\\smb_server";

static const config_string_params csp_LocalName = { 
    "LocalName",
    "%m-SFS",
    sizeof(config.local_name)
};
static const config_ranged_num_params crnp_Debug = { 
    "Debug",
    1,
    0,
    1
};
static const config_ranged_num_params crnp_KillWaitTime = { 
    "KillWaitTime",
    10000,
    0,
    0xFFFFFFFF
};
static const config_ranged_num_params crnp_NumWorkers = {
    "NumWorkers",
    1, // 4,
    1,
    20
};
static const config_ranged_num_params crnp_NumSenders = {
    "NumSenders",
    1, // 2,
    1,
    10
};
static const config_ranged_num_params crnp_BufferSize = {
    "BufferSize",
    16384,
    8192,
    0xFFFF
};
static const config_string_params csp_FsDll = { 
    "FsDll",
    "fswin32.dll",
    -1
};
static const config_string_params csp_FsName = { 
    "FsName",
    "fswin32",
    -1
};
static const config_string_params csp_FsConfig = { 
    "FsConfig",
    "fswin32",
    -1
};


inline static
void 
MyGetRangedNumber2(
    config_handle_t h,
    const config_ranged_num_params& p,
    DWORD& value, 
    LPCSTR szError
    )
{
    DFN(InitConfig);
    if (ConfigDefRangedNumber(h, p, value))
        DEBUG_PRINT(("%s must be in the range [%d, %d], defaulted to %d\n",
                     szError, p.min, p.max, p.def));
}


void
InitConfig(
    const char* config_path
    )
{
    DFN(InitConfig);
    ZeroMemory(&config, sizeof(config_type));

    config_handle_t h = 0;
    HKEY hKey = 0;
    DWORD error;

    const char* base_path = ConfigOpen(config_path, DEFAULT_CONFIG_PATH, 
                                       h, error);
    if (config_path && !base_path || (base_path == DEFAULT_CONFIG_PATH)) {
        DEBUG_PRINT(("Could not open config path %s (error %d)\n",
                     config_path, error));
    }
    if (!base_path) {
        DEBUG_PRINT(("Could not open default config path %s (error %d)\n", 
                     DEFAULT_CONFIG_PATH, error));
    }

    DWORD nSize = sizeof(config.remote_name);
    if (!GetComputerName(config.remote_name, &nSize)) {
        DEBUG_PRINT(("Could not get computer name (error %d)\n", 
                     GetLastError()));
        exit(1); // fatal error
    }

    DWORD temp = 0;
    MyGetRangedNumber2(h, crnp_Debug, temp, "debug enabled");
    config.debug = temp;
    MyGetRangedNumber2(h, crnp_KillWaitTime, config.kill_wait_time,
                       "kill wait time");
    MyGetRangedNumber2(h, crnp_NumWorkers, config.num_workers,
                      "number of worker threads");
    MyGetRangedNumber2(h, crnp_NumSenders, config.num_senders,
                      "number of sender threads");
    MyGetRangedNumber2(h, crnp_BufferSize, config.buffer_size,
                      "buffer size");

    subst_table_t st = { 0 };
    st['m'] = config.remote_name;

    char* local_name = 0;
    ConfigDefString(h, csp_LocalName, st, local_name);
    ConfigDefString(h, csp_FsDll, 0, config.fs_dll);
    ConfigDefString(h, csp_FsName, 0, config.fs_name);
    char* candidate_path = 0;
    ConfigDefString(h, csp_FsConfig, 0, candidate_path);

    ConfigClose(h);

    config.fs_config = ConfigAllocPath(base_path, candidate_path);
    ConfigUndefString(candidate_path);

    lstrcpy(config.local_name, local_name);
    ConfigUndefString(local_name);

    MakeNetbiosName(config.nb_remote_name, config.remote_name);
    SetNetbiosNameType(config.nb_remote_name, 0);
    MakeNetbiosName(config.nb_local_name, config.local_name);
}

void
CleanupConfig()
{
    ConfigUndefString(config.fs_dll);
    ConfigUndefString(config.fs_name);
    ConfigFreePath(config.fs_config);
}
