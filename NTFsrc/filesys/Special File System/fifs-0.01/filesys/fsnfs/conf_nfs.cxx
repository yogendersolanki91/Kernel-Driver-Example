/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   fsnfs configuration

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
#include "conf_nfs.hxx"
#include <conf_help.hxx>
#include <debug.hxx>

static const char USER_INFO[] = "UserInfo";

static const char DEFAULT_CONFIG_PATH[] = "\\Registry\\HKEY_LOCAL_MACHINE\\Software\\Etc\\fsnfs";

static const config_string_params csp_machine = { 
    "machine",
    "tma-1",
    -1
};
static const config_string_params csp_dirpath = { 
    "dirpath",
    "/disk/tm1",
    -1
};
static const config_string_params csp_protocol = { 
    "protocol",
    "udp",
    -1
};
static const config_string_params csp_Label = { 
    "Label",
    "fsnfs",
    -1
};
static const config_ranged_num_params crnp_MaxHandles = {
    "MaxHandles",
    1024,
    1024,
    0xFFFF
};

config_type config;

void
InitConfig(
    const char* config_path
    )
{
    DFN(InitConfig_win32);
    ZeroMemory(&config, sizeof(config_type));

    config_handle_t h = 0;
    DWORD error;

    const char* base_path = ConfigOpen(config_path, DEFAULT_CONFIG_PATH, 
                                       h, error);
    if (config_path && !base_path || (base_path == DEFAULT_CONFIG_PATH))
        DEBUG_PRINT(("Could not open config path %s (error %d)\n",
                     config_path, error));
    if (!base_path)
        DEBUG_PRINT(("Could not open default config path %s (error %d)\n", 
                     DEFAULT_CONFIG_PATH, error));

    ConfigDefRangedNumber(h, crnp_MaxHandles, config.max_handles);
    ConfigDefString(h, csp_machine, 0, config.machine);
    ConfigDefString(h, csp_dirpath, 0, config.dirpath);
    ConfigDefString(h, csp_protocol, 0, config.protocol);
    ConfigDefString(h, csp_Label, 0, config.label);

    config.localmachine[0] = 0;
    gethostname(config.localmachine, MAX_MACHINE_NAME);
    config.localmachine[MAX_MACHINE_NAME] = 0;
    // we error-check later, when we actually try to use the file system

    config.user_info_path = ConfigAllocPath(base_path, USER_INFO);
    // we error-check later, when we actually try to use the file system

    ConfigClose(h);
}

void
CleanupConfig()
{
    ConfigUndefString(config.machine);
    ConfigUndefString(config.dirpath);
    ConfigUndefString(config.protocol);
    ConfigUndefString(config.label);
    ConfigFreePath(config.user_info_path);
}

DWORD
GetUserInfo(
    const char* principal, int& uid, int& gid, int& len, int gids[], int maxlen
    )
{
    DFN(GetUserInfo);
    if (!config.localmachine[0]) {
        DEBUG_PRINT(("could not get local machine name\n"));
	return ERROR_INVALID_PARAMETER;
    }
    if (!config.user_info_path) {
        DEBUG_PRINT(("could not get base user info path\n"));
        return ERROR_INVALID_PARAMETER;
    }
    config_handle_t h = 0;
    DWORD error = ERROR_SUCCESS;
    ConfigOpen(config.user_info_path, 0, h, error);
    if (error) {
        DEBUG_PRINT(("could not get user info for %s\n", principal));
        return error;
    }
    DWORD size;
    size = sizeof(uid);
    error = ConfigGetSizedData(h, "uid", &uid, &size);
    if (error) {
	DEBUG_PRINT(("could not get uid for %s\n", principal));
	goto cleanup;
    }
    size = sizeof(gid);
    error = ConfigGetSizedData(h, "gid", &gid, &size);
    if (error) {
        DEBUG_PRINT(("could not get gid for %s\n", principal));
	goto cleanup;
    }
    void* tmp;
    error = ConfigGetData(h, "gids", &tmp, 0);
    if (error) {
        DEBUG_PRINT(("could not get gids for %s\n", principal));
	goto cleanup;
    }
    {
	char* buffer = (char*)tmp;
	len = 0;
	char* tok = strtok(buffer, ",");
	while (tok && (len < maxlen)) {
	    gids[len++] = atoi(tok);
	    tok = strtok(0, ",");
	}
    }
    ConfigFreeData(tmp);
    if (!len) {
        DEBUG_PRINT(("could not parse any gids for %s\n", principal));
	error = ERROR_INVALID_PARAMETER;
	goto cleanup;
    }
 cleanup:
    ConfigClose(h);
    return error;
}
