/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   fswin32 configuration

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
#include "conf_win32.hxx"
#include <conf_help.hxx>
#include <debug.hxx>

static const char DEFAULT_CONFIG_PATH[] = "\\Registry\\HKEY_LOCAL_MACHINE\\Software\\Etc\\fswin32";

static const config_string_params csp_Root = { 
    "Root",
    "c:\\temp\\fakefs",
    -1
};
static const config_string_params csp_Label = { 
    "Label",
    "FsWin32",
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
    ConfigDefString(h, csp_Root, 0, config.root);
    ConfigDefString(h, csp_Label, 0, config.label);

    ConfigClose(h);
}

void
CleanupConfig()
{
    ConfigUndefString(config.root);
    ConfigUndefString(config.label);
}
