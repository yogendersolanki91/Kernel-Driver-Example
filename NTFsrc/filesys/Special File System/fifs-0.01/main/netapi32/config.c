/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   NetBIOS snooper configuration

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

#define STRING_KEY_MAIN "Software\\etc\\FakeNetapi32"

LPCSTR STRING_VALUE_SHOWLOAD = "ShowLoad";
const BOOL DEFAULT_SHOWLOAD = FALSE;

LPCSTR STRING_VALUE_SHOWTIME = "ShowTime";
const BOOL DEFAULT_SHOWTIME = TRUE;

LPCSTR STRING_VALUE_SHOWHEADER = "ShowHeader";
const BOOL DEFAULT_SHOWHEADER = TRUE;

LPCSTR STRING_VALUE_SHOWBUFFER = "ShowBuffer";
const BOOL DEFAULT_SHOWBUFFER = TRUE;

LPCSTR STRING_VALUE_SHOWBUFFERIFFSMB = "ShowBufferIffSmb";
const BOOL DEFAULT_SHOWBUFFERIFFSMB = TRUE;

LPCSTR STRING_VALUE_SHOWSMB = "ShowSmb";
const BOOL DEFAULT_SHOWSMB = TRUE;

LPCSTR STRING_VALUE_REALPATH = "RealPath";
LPCSTR DEFAULT_REALPATH = "c:\\winnt\\system32\\netapi32.dll";

LPCSTR STRING_VALUE_BOXTITLE = "BoxTitle";
LPCSTR DEFAULT_BOXTITLE = "FAKE NETAPI32.DLL";

LPCSTR STRING_VALUE_LOGFILE = "LogFile";
LPCSTR DEFAULT_LOGFILE = "c:\\temp\\netbios.%a.log";


config_type config;

static
DWORD
RegAllocQueryValue(
    HKEY hKey,
    LPCSTR szValue,
    LPBYTE *ppData
    )
{
    DWORD Error, dwSize;
    LPBYTE buffer;

    if (!ppData)
        return ERROR_INVALID_PARAMETER;

    Error = RegQueryValueEx(hKey, szValue, 0, NULL, NULL, &dwSize);
    if (!Error) {
        buffer = LocalAlloc(LMEM_FIXED, dwSize);
        Error = RegQueryValueEx(hKey, szValue, 0, NULL, buffer, &dwSize);
        if (Error)
            LocalFree(buffer);
    }
    if (!Error)
        *ppData = buffer;
    return Error;
}

static
VOID
RegFreeQueryValue(
    LPBYTE pData
    )
{
    LocalFree(pData);
}

static
BOOL
GetBoolFromRegistry(
    HANDLE hKey,
    LPCSTR szValue,
    BOOL def
    )
{
    DWORD dwSize = sizeof(DWORD);
    DWORD result = def;
    RegQueryValueEx(hKey, szValue, 0, NULL, (LPBYTE) &result, &dwSize);
    return (BOOL) result;
}

static
config_string
GetStringFromRegistry(
    HANDLE hKey,
    LPCSTR szValue,
    LPCSTR def
    )
{
    config_string cs;

    cs.allocated = FALSE;
    cs.string = NULL;

    RegAllocQueryValue(hKey, szValue, (LPBYTE*)&cs.string);
    if (!cs.string)
        cs.string = def;
    else
        cs.allocated = TRUE;
    return cs;
}


#define RIDICULOUS_SIZE 1024

static char machine_name[RIDICULOUS_SIZE];
static char app_name[RIDICULOUS_SIZE];

static char* name_table[256] = {0};

static 
void
init_names()
{
    DWORD size = sizeof(machine_name);

    if (!GetComputerName(machine_name, &size)) {
        lstrcpy(machine_name, "untitled_machine");
    }

    if (!GetModuleFileName(NULL, app_name, sizeof(app_name))) {
        lstrcpy(app_name, "untitled_app");
    } else {
        char* s = app_name;
        char* e = app_name + lstrlen(app_name);
        while ((e > s) && (*e != '/') && (*e != '\\'))
            e--;
        if (e > s) {
            e++;
            while (*(s++) = *(e++));
        }
    }
    name_table['m'] = machine_name;
    name_table['a'] = app_name;
}


static
int
substitute_vars(
    const char* string_template,
    char* buffer,
    int size,
    char* table[]
    )
{
    const char* s = string_template;
    char* t = buffer;
    char* name;
    int len;

    if (!s || !size) {
        len = 1;
        while (*s) {
            if (*s != '%') {
                s++;
                len++;
            } else {
                if (name = table[*(++s)]) {
                    len += lstrlen(name); // we could cache this...
                } else {
                    s++;
                    len++;
                }
            }
        }
        return len;
    } else {
        while (*s && ((len = (t - buffer + 1)) < size)) {
            if (*s != '%') {
                *(t++) = *(s++);
            } else {
                if (name = table[*(++s)]) {
                    s++;
                    lstrcpyn(t, name, size - len);
                    t += lstrlen(t); // move by amt. actually copied
                }
            }
        }
        *t = 0;
//        buffer[size - 1] = 0;
        return t - buffer + 1;
    }
}

void
substitute_names(
    config_string* cs
    )
{
    char* s;
    int len;

    len = substitute_vars(cs->string, 0, 0, name_table);
    if (len < 1) return;

    s = LocalAlloc(LMEM_FIXED, (DWORD)len * sizeof(char));
    if (!s) return;

    substitute_vars(cs->string, s, len, name_table);

    if (cs->allocated)
        RegFreeQueryValue((LPBYTE)cs->string);
    cs->string = s;
}

void
InitConfig()
{
    HKEY hKey = NULL;

    init_names();

    RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 STRING_KEY_MAIN,
                 0,
                 KEY_READ,
                 &hKey);

#define GETBOOL(x) GetBoolFromRegistry(hKey, STRING_VALUE_##x, DEFAULT_##x)

    config.bShowLoad = GETBOOL(SHOWLOAD);
    config.bShowTime = GETBOOL(SHOWTIME);
    config.bShowHeader = GETBOOL(SHOWHEADER);
    config.bShowBuffer = GETBOOL(SHOWBUFFER);
    config.bShowBufferIffSmb = GETBOOL(SHOWBUFFERIFFSMB);
    config.bShowSmb = GETBOOL(SHOWSMB);

#define GETSTRING(x) GetStringFromRegistry(hKey, STRING_VALUE_##x, DEFAULT_##x)

    config.csRealPath = GETSTRING(REALPATH);
    config.csBoxTitle = GETSTRING(BOXTITLE);
    config.csLogFile = GETSTRING(LOGFILE);

    substitute_names(&config.csRealPath);
    substitute_names(&config.csBoxTitle);
    substitute_names(&config.csLogFile);

    RegCloseKey(hKey);
}

void
CleanupConfig()
{
#define CLEANSTRING(x) {if (x.allocated)RegFreeQueryValue((LPBYTE)x.string);}

    CLEANSTRING(config.csRealPath);
    CLEANSTRING(config.csBoxTitle);
    CLEANSTRING(config.csLogFile);
}
