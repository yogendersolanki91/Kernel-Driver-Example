/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   fsmunge file system dispatch table object header

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

#ifndef __FSDT_MUNGE_HXX__
#define __FSDT_MUNGE_HXX__

#include "fs_munge.hxx"

#define HACKNAME(x) tolower(x)

///////////////////////////////////////////////////////////////////////////////
//
// Defines filesystem dispatch table
//
///////////////////////////////////////////////////////////////////////////////

class FsDT_Munge: public FsDispatchTable {
public:
    // FsDispatchTable:
    ULONG AddRef();
    ULONG Release();
    const char* get_principal();
    fhandle_t get_root();
    DWORD create(fhandle_t dir, const char* name, UINT32 flags, fattr_t* attr, fhandle_t* handle);
    DWORD set_attr(fhandle_t handle, fattr_t* attr);
    DWORD lookup(fhandle_t dir, const char* name, fattr_t* attr);
    DWORD get_attr(fhandle_t handle, fattr_t* attr);
    DWORD close(fhandle_t handle);
    DWORD write(fhandle_t handle, UINT64 offset, UINT64* count, void* buffer);
    DWORD read(fhandle_t handle,  UINT64 offset, UINT64* count, void* buffer);
    DWORD readlink(fhandle_t handle, int* size, char* path_buffer);
    DWORD symlink(fhandle_t dir, const char* name, const char* path);
    DWORD read_dir(fhandle_t dir, UINT32 cookie, dirinfo_t* buffer, UINT32 size, UINT32 *entries_found);
    DWORD statfs(fhandle_t handle, fs_attr_t* attr);
    DWORD remove(fhandle_t dir, const char* name);
    DWORD rename(fhandle_t fromdir, const char* fromname, fhandle_t todir, const char* toname);
    DWORD mkdir(fhandle_t dir, const char* name, fattr_t* attr);
    DWORD rmdir(fhandle_t dir, const char* name);
    DWORD link(fhandle_t dir, const char* name, fhandle_t handle);
    DWORD ioctl(fhandle_t handle, UINT32 code, void* in_buffer, 
                UINT32* out_size, void* out_buffer);
    DWORD flush(fhandle_t handle);

private:
    FsDT_Munge(FsDispatchTable* _pDT, FileSystem* _pFs); // caller must AddRef _pFs
    ~FsDT_Munge();

    // types:
    class PathName {
        inline static bool is_sep(char c) {
            return (c == '\\') || (c == '/');
        }
        inline static void count_seps(const char* str, int& count, int& size) {
            const char* start = str;
            count = 0;
            size = 0;
            while (*str) {
                if (is_sep(*str)) count++;
                str++;
            }
            size = str - start + 1;
        }
        inline static char* make_parts(const char* str, char* buf, 
                                       char* parts[])
        {
            int i = 0;
            char* p = buf;
            while (*str) {
                *buf = HACKNAME(*str);
                if (is_sep(*buf)) {
                    *buf = 0;
                    parts[i++] = p;
                    p = buf+1;
                }
                buf++;
                str++;
            }
            *buf = *str; // copy null byte
            return p;
        }
        char* m_copy;
        char* m_final;
        char** m_parts;
        int m_count;
    public:
        PathName(const char* name):m_count(0),m_copy(0),m_parts(0) {
            if (!name) return;
            if (is_sep(name[0])) name++;
            int size;
            count_seps(name, m_count, size);
            m_copy = new char[size];
            m_parts = new char*[m_count];
            m_final = make_parts(name, m_copy, m_parts);
        }
        ~PathName() {
            delete [] m_parts;
            delete [] m_copy;
        }
        inline int count() { return m_count; }
        inline const char* part(int i) { 
            return ((i < 0)||(i >= m_count))?0:m_parts[i];
        }
        inline const char* final() { return m_final; }
    };

    class PathHandle {
        DWORD m_error;
        fhandle_t m_dir;
        fhandle_t m_dhp;
        FsDispatchTable* m_pDT;
        PathName m_pn;
    public:
        PathHandle(fhandle_t _dir, const char* _path, FsDispatchTable* _pDT)
            :m_dir(_dir),m_dhp(_dir),m_pDT(_pDT),m_pn(_path),
             m_error(ERROR_SUCCESS)
        {
            fhandle_t dh;
            for (int i = 0; i < m_pn.count(); i++) {
                m_error = m_pDT->create(m_dhp, m_pn.part(i),
                                        DISP_DIRECTORY, 0, &dh);
                if (m_error) return;
                if (m_dhp != m_dir) m_pDT->close(m_dhp);
                m_dhp = dh;
            }
        }
        ~PathHandle() { if (m_dhp != m_dir) m_pDT->close(m_dhp); }
        inline DWORD error() { return m_error; }
        inline fhandle_t dir() { return m_dhp; }
        inline const char* name() { return m_pn.final(); }
    };

    // global:
    static ULONG global_count;

    // data:
    ULONG ref_count;
    FileSystem* pFs;
    FsDispatchTable* pDT;
    CRITICAL_SECTION csObject;

    // friends:
    friend class Fs_Munge;
};

#endif /* __FSDT_MUNGE_HXX__ */
