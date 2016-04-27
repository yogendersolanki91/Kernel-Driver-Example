/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   Configuration Helper

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
#include "conf_help.hxx"
#include <string.h>

DWORD
ConfigOpenRegistry(
    LPCSTR RegistryPath,
    REGSAM samDesired,
    HKEY& hKey
    );

bool
ConfigDefRangedNumber2(
    config_handle_t h,
    LPCSTR szValue,
    DWORD def,
    DWORD min,
    DWORD max,
    DWORD& value
    );

bool
ConfigDefString2(
    config_handle_t h,
    const char* name,    // value name
    const char* def,     // default value, if desired
    int max_size,        // -1 for unlimited (includes NULL byte)
    subst_table_t table, // 0 for no subst
    char*& target        // where to put string pointer
    );

////////////////////////////////////////////////////////////////////////////

const char*
ConfigOpen(
    const char* config_path,
    const char* default_config_path,
    config_handle_t& h,
    DWORD& error
    )
{
    h = 0;
    HKEY hKey = 0;
    error = ERROR_SUCCESS;
    if (config_path) {
        error = ConfigOpenRegistry(config_path, KEY_READ, hKey);
        if (!error) {
	    h = (config_handle_t) hKey;
	    return config_path;
	}
    }
    if (default_config_path) {
        error = ConfigOpenRegistry(default_config_path, KEY_READ, hKey);
        if (!error) {
	    h = (config_handle_t) hKey;
	    return default_config_path;
	}
    }
    return 0;
}

void
ConfigClose(
    config_handle_t h
    )
{
    HKEY hKey = (HKEY) h;
    RegCloseKey(hKey);
}

char*
ConfigAllocPath(
    const char* base_path,
    const char* path_candidate
    )
{
    if (!path_candidate) return 0;
    HKEY hKey = 0;
    DWORD error = ConfigOpenRegistry(path_candidate, KEY_READ, hKey);
    if (!error) {
        char* temp = new char[lstrlen(path_candidate)];
        lstrcpy(temp, path_candidate);
        return temp;
    }
    if (!base_path) return 0;
    const int base_len = lstrlen(base_path);
    char* temp = new char[base_len + 1 + lstrlen(path_candidate) + 1];
    char* p = temp;
    lstrcpy(p, base_path);
    p += base_len;
    *(p++) = '\\';
    lstrcpy(p, path_candidate);
    return temp;
}


void
ConfigFreePath(
    char* path
    )
{
    delete [] path;
}


DWORD
ConfigOpenRegistry(
    LPCSTR RegistryPath,
    REGSAM samDesired,
    HKEY& hKey
    )
{
    class comparer {
        static const char* is_x(const char* str, const char *x) {
            int len = lstrlen(x);
            return _strnicmp(str, x, len)?0:str+len;
        }

    public:
        static const char* is_sep(const char* str) {
            return (!str || ((*str != '\\') && (*str != '/')))?0:str+1;
        }
        static const char* is_registry(const char* str) {
            return is_x(str, "Registry");
        }
        static const char* get_hkey(const char* str, PHKEY phKey) {
            if (!phKey) return 0;

            struct hkey_xlat_t {
                const char* name;
                HKEY hkey;
            };

            static const hkey_xlat_t xlat[] = {
                {"HKEY_LOCAL_MACHINE", HKEY_LOCAL_MACHINE}, 
                {"HKEY_USERS", HKEY_USERS}, 
                {"HKEY_CURRENT_USER", HKEY_CURRENT_USER}, 
                {"HKEY_CLASSES_ROOT", HKEY_CLASSES_ROOT} };

            static const int num_xlat = sizeof(xlat) / sizeof(xlat[0]);

            const char* result;
            for (int i = 0; i < num_xlat; i++)
                if (result = is_x(str, xlat[i].name)) {
                    *phKey = xlat[i].hkey;
                    return result;
                }
            return 0;
        }
    };

    // look for one of:
    // \Registry\HKEY_CURRENT_USER
    // \Registry\HKEY_USERS
    // \Registry\HKEY_LOCAL_MACHINE
    // \Registry\HKEY_CLASSES_ROOT

    hKey = 0;

    HKEY hRootKey;

    if (!RegistryPath ||
        !(RegistryPath = comparer::is_sep(RegistryPath)) ||
        !(RegistryPath = comparer::is_registry(RegistryPath)) ||
        !(RegistryPath = comparer::is_sep(RegistryPath)) ||
        !(RegistryPath = comparer::get_hkey(RegistryPath, &hRootKey)))
        return ERROR_INVALID_PARAMETER;

    LPCSTR lpSubKey = comparer::is_sep(RegistryPath);

    return RegOpenKeyEx(hRootKey,
                        lpSubKey,
                        0,
                        samDesired,
                        &hKey);
    return 0;
}


DWORD
ConfigGetSizedData(
    config_handle_t h,
    const char* name,
    void* pData,
    DWORD* pSize
    )
{
    HKEY hKey = (HKEY)h;
    return RegQueryValueEx(hKey, name, 0, NULL, (LPBYTE)pData, pSize);
}

DWORD
ConfigGetData(
    config_handle_t h,
    const char* name,
    void** ppData,
    DWORD* pSize
    )
{
    HKEY hKey = (HKEY)h;
    DWORD error, dwSize;
    LPBYTE buffer;
    if (ppData) *ppData = 0;
    error = RegQueryValueEx(hKey, name, 0, NULL, NULL, &dwSize);
    if (error) return error;

    if (pSize) *pSize = dwSize;
    if (!ppData) return error; // we are done

    buffer = (LPBYTE)LocalAlloc(LMEM_FIXED, dwSize);
    error = RegQueryValueEx(hKey, name, 0, NULL, buffer, &dwSize);
    if (error)
	LocalFree(buffer);
    else
        *ppData = (void*)buffer;
    return error;
}

void
ConfigFreeData(
    void* pData
    )
{
    LocalFree(pData);
}

bool
ConfigDefRangedNumber(
    config_handle_t h,
    const config_ranged_num_params& p,
    DWORD& value
    )
{
    return ConfigDefRangedNumber2(h, p.name, p.def, p.min, p.max, value);
}


bool
ConfigDefRangedNumber2(
    config_handle_t h,
    LPCSTR szValue,
    DWORD def,
    DWORD min,
    DWORD max,
    DWORD& value
    )
{
    HKEY hKey = (HKEY)h;
    DWORD Size = sizeof(DWORD);
    value = def;
    RegQueryValueEx(hKey, szValue, 0, NULL, (LPBYTE)&value, &Size);
    if ((value < min) || (value > max)) {
        value = def;
        return true; // had to use default
    }
    return false;
}


bool
ConfigDefString(
    config_handle_t h,
    const config_string_params& p,
    subst_table_t table, // 0 for no subst
    char*& target        // where to put string pointer
    )
{
    return ConfigDefString2(h, p.name, p.def, p.max_size, table, target);
}


bool
ConfigDefString2(
    config_handle_t h,
    const char* name,    // value name
    const char* def,     // default value, if desired
    int max_size,        // -1 for unlimited (includes NULL byte)
    subst_table_t table, // 0 for no subst
    char*& target        // where to put string pointer
    )
{
    target = 0;
    if (!max_size) return false;

    bool limited_size = (max_size >= 0);
    int size;
    bool using_default;

    char* temp = 0;
    DWORD error;
    {
        void* tmp = 0;
        error = ConfigGetData(h, name, &tmp, 0);
        temp = (char*)tmp;
    }
    if (using_default = error) temp = (char*)def;
    size = lstrlen(temp) + 1;
    if (limited_size) size = min(size, max_size);
    target = new char[size];
    lstrcpyn(target, temp, size);
    if (!error) ConfigFreeData(temp);
    if (table) {
        temp = target;
        size = substitute_vars(temp, 0, 0, table);
        if (limited_size) size = min(size, max_size);
        target = new char[size];
        substitute_vars(temp, target, size, table);
        delete [] temp;
    }
    return using_default;
}


void
ConfigUndefString(
    char* str
    )
{
    delete [] str;
}


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

    if (!s) return -1;

    if (!size) {
        len = 0;
        while (*s) {
            if (*s != '%') {
                s++;
                len++;
            } else {
                s++; // see post-% char
                if (*s) {
                    if (name = table[*s]) {
                        len += lstrlen(name); // we could cache this...
                    } else {
                        len += 2; // for % and post-% char
                    }
                    s++; // go to post-"%char" char
                }
            }
        }
        return len + 1; // for terminating NULL
    } else {
        while (*s && ((len = (t - buffer + 1)) < size)) {
            if (*s != '%') {
                *(t++) = *(s++);
            } else {
                s++; // see post-% char
                if (*s) {
                    if (name = table[*s]) {
                        lstrcpyn(t, name, size - len);
                        t += lstrlen(t); // move by amt. actually copied
                    } else {
                        lstrcpyn(t, s - 1, 2);
                        t += 2;  // for % and post-% char
                    }
                    s++; // go to post-"%char" char
                }
            }
        }
        len = t - buffer + 1;
        buffer[len - 1] = 0;
        return len;
    }
}
