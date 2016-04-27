/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   fswin32 file system dispatch table object header

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

#ifndef __FSDT_WIN32_HXX__
#define __FSDT_WIN32_HXX__

#include "fs_win32.hxx"

///////////////////////////////////////////////////////////////////////////////
//
// Defines filesystem dispatch table
//
///////////////////////////////////////////////////////////////////////////////

class FsDT_Win32: public FsDispatchTable {
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
    FsDT_Win32(const char* _principal, FileSystem* _pFs); // caller must AddRef
    ~FsDT_Win32();

    // helpers:
    DWORD init();
    DWORD create2(
        const char* fullname,
        size_t fullname_size, // including NULL byte...
        UINT32 flags, 
        fattr_t* fattr, 
        fhandle_t* phandle
        );
    DWORD close2(fhandle_t handle);
    inline fhandle_t get_new_handle();
    inline bool invalid_handle(fhandle_t handle);
    inline bool invalid_dir_handle(fhandle_t handle);
    inline bool invalid_file_handle(fhandle_t handle);

    // global:
    static ULONG global_count;
    inline static TIME64 get_time(FILETIME ft);
    inline static UINT32 get_attributes(DWORD a);
    inline static FILETIME unget_time(TIME64 t);
    inline static DWORD unget_attributes(UINT32 attr);
    inline static DWORD unget_disp(UINT32 flags);
    inline static DWORD unget_access(UINT32 flags);
    inline static DWORD unget_share(UINT32 flags);

    inline static char* construct_pathname(const char* basename, const char* name);
    inline static void destroy_pathname(char* pathname);

    // types:
    struct cache_chunk {
        UINT32 size;
        WIN32_FIND_DATA* data;
        cache_chunk* next;
        cache_chunk(size_t n):size(n),data(new WIN32_FIND_DATA[n]),next(0) {}
        ~cache_chunk() { delete [] data; }
    };
    class dir_cache {
        cache_chunk* head;
        cache_chunk* last;
        HANDLE h;
        UINT32 capacity;
        UINT32 used;
        DWORD _error;
        bool _valid;
        bool grow(size_t n = 100) {
            if (_error) return false;
            cache_chunk* c = new cache_chunk(n);
            if (!c) { _error = ERROR_NOT_ENOUGH_MEMORY; return false; }
            last->next = c;
            last = c;
            capacity += n;
            return fill(last->data);            
        }
        bool fill(WIN32_FIND_DATA* end) {
            if (_error) return false;
            UINT32 prev = used;
            BOOL bok = TRUE;
            while ((used < capacity) && (bok = FindNextFile(h, end))) {
                used++; end++;
            }
            if (!bok) _error = GetLastError();
            return (used > prev);
        }
    public:
        dir_cache(const char* dirpath, HANDLE& _h, size_t n = 100):
            _error(0),capacity(max(n,1)),used(0),
            head(0),last(0),_valid(false)
        {
            head = new cache_chunk(capacity);
            if (!head) { 
                _error = ERROR_NOT_ENOUGH_MEMORY;
                capacity = 0;
                return;
            }
            last = head;
            const int len = lstrlen(dirpath);
            const char star[] = "\\*";
            char* filter_path = new char[len + sizeof(star)];
            RtlCopyMemory(filter_path, dirpath, len);
            RtlCopyMemory(filter_path + len, star, sizeof(star));
            h = FindFirstFile(filter_path, head->data);
            delete [] filter_path;
            if (h == INVALID_HANDLE_VALUE) {
                _error = GetLastError();
            } else {
                _valid = true;
                used = 1;
                fill(head->data + 1);
            }
        }
        ~dir_cache() {
            while(head) {
                cache_chunk* c = head->next;
                delete head;
                head = c;
            }
            FindClose(h);
        }
        WIN32_FIND_DATA* get(size_t n) {
            if ((n > used) || 
                ((n == used) && !grow())) return 0;
            for (cache_chunk* c = head; (n >= c->size) && c->next; c = c->next)
                n -= c->size;
            return &c->data[n];
        }
        DWORD error() { return _error; }
        bool valid() { return _valid; }
    };

    struct handle_info_t {
        HANDLE h;
        dir_cache* dc;
        char* pathname;

        handle_info_t():
            h(INVALID_HANDLE_VALUE),
            dc(0),
            pathname(0) {}
    };

    // data:
    ULONG ref_count;
    FileSystem* pFs;
    char* principal;
    handle_info_t* handles;
    int handles_used;
    CRITICAL_SECTION csObject;

    // friends:
    friend class Fs_Win32;
};

#endif /* __FSDT_WIN32_HXX__ */
