/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   SMB-FIFS glue

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
#include <htab.h>
#include <autolock.hxx>
#include <stdio.h>

// class Counter {
//     char* p;
//     int& c;
// public:
//     inline void inc() { c++; }
//     inline void dec() { c--; }
//     inline void prn() { printf("%s: %d\n", p, c); }

//     Counter(const char* _p, int& _c):c(_c) { 
//         p = new char[lstrlen(_p) + 1];
//         lstrcpy(p, _p);
//     }
//     ~Counter() {
//         prn();
//         delete [] p;
//     }
// };

static int u_count;
static int s_count;
static int f_count;

//
// USERINFO
//

static CRITICAL_SECTION csUser;

struct UserInfo;
static LIST_HEAD(UserList, UserInfo) ui_list;
static HASH_TABLE(UidTable, UserInfo)   ui_table;
static HASH_TABLE(UnameTable, UserInfo) un_table;
static USHORT highest_uid = 0;
static const int u_size = 64; // more than plenty

struct UserInfo {
    USHORT uid;
    char* name;
    FsInterface::FsDispatchTable* pDisp;

    HASH_ENTRY(UserInfo) ui_hlink;
    HASH_ENTRY(UserInfo) un_hlink;
    LIST_ENTRY(UserInfo) ui_llink;

    UserInfo(USHORT _uid, const char* _name, FsInterface::FsDispatchTable* _pDisp)
        :uid(_uid),
         name(0),
         pDisp(_pDisp)
    {
        if (_name) {
            name = new char[lstrlen(_name) + 1];
            lstrcpy(name, _name);
        }
    }
    ~UserInfo() {
        if (pDisp) pDisp->Release();
        delete[] name;
    }

};

#define ui_hash(uid) uid
#define ui_cmp(u, x) (u->uid != x)
#define ui_expand(u) (u->uid)

#define un_hash(name) hash(HASHSEED, (u_char *)name, strlen(name))
#define un_cmp(u, x) strcmp(u->name, x)
#define un_expand(u) (u->name)

inline void UI_LOOKUP(USHORT uid, UserInfo*& x) {
    HASH_LOOKUP(&ui_table, ui_hlink, nargs1, (uid), ui_hash, ui_cmp, x);
}
inline void UN_LOOKUP(const char* name, UserInfo*& x) {
    HASH_LOOKUP(&un_table, un_hlink, nargs1, (name), un_hash, un_cmp, x);
}
inline void UI_INSERT_HASH(UserInfo* x) {
    HASH_INSERT(&ui_table, ui_hlink, ui_expand, x, ui_hash);
}
inline void UN_INSERT_HASH(UserInfo* x) {
    HASH_INSERT(&un_table, un_hlink, un_expand, x, un_hash);
}
inline void UI_INSERT_LIST(UserInfo* x) {
    LIST_INSERT_HEAD(&ui_list, x, ui_llink);
}
inline void UI_REMOVE_HASH(UserInfo *x) {
    HASH_REMOVE(&ui_table, x, ui_hlink);
}
inline void UN_REMOVE_HASH(UserInfo *x) {
    HASH_REMOVE(&un_table, x, un_hlink);
}
inline void UI_REMOVE_LIST(UserInfo *x) {
    LIST_REMOVE(x, ui_llink);
}
inline void U_INSERT(UserInfo* x) {
    UI_INSERT_HASH(x);
    UN_INSERT_HASH(x);
    UI_INSERT_LIST(x);
}
inline void U_REMOVE(UserInfo *x) {
    UI_REMOVE_HASH(x);
    UN_REMOVE_HASH(x);
    UI_REMOVE_LIST(x);
}
inline void U_INIT(int size) {
    HASH_INIT(&ui_table, size, UidTable);
    HASH_INIT(&un_table, size, UnameTable);
    LIST_INIT(&ui_list);
}
inline void U_CLEANUP(){
    UserInfo* x;
    while (x = ui_list.lh_first) {
        U_REMOVE(x);
        delete x;
    }
}


//
// FINDINFO
//

static CRITICAL_SECTION csFind;

struct FindInfo;
static LIST_HEAD(FindList, FindInfo) si_list;
static HASH_TABLE(FindTable, FindInfo) si_table;
static USHORT highest_sid = 0;
static const int si_size = 16;

struct FindInfo {
    USHORT sid;
    fhandle_t handle;
    UINT32 cookie;
    dirinfo_t* entry;
    filter_t* filter;
    HASH_ENTRY(FindInfo) si_hlink;
    LIST_ENTRY(FindInfo) si_llink;
    FindInfo(USHORT f, FsInterface::fhandle_t h, UINT32 _cookie, filter_t* _filter, dirinfo_t* _entry)
        :sid(f),handle(h),cookie(_cookie),filter(_filter),entry(_entry) {}
    ~FindInfo() { }
};

#define si_hash(sid) sid
#define si_cmp(f, x) (f->sid != x)
#define si_expand(f) (f->sid)

inline void SI_LOOKUP(USHORT sid, FindInfo*& s) {
    HASH_LOOKUP(&si_table, si_hlink, nargs1, (sid), si_hash, si_cmp, s);
}
inline void SI_INSERT_HASH(FindInfo* s) {
    HASH_INSERT(&si_table, si_hlink, si_expand, s, si_hash);
}
inline void SI_INSERT_LIST(FindInfo* s) {
    LIST_INSERT_HEAD(&si_list, s, si_llink);
}
inline void SI_REMOVE_HASH(FindInfo *s) {
    HASH_REMOVE(&si_table, s, si_hlink);
}
inline void SI_REMOVE_LIST(FindInfo *s) {
    LIST_REMOVE(s, si_llink);
}
inline void SI_INSERT(FindInfo* s) {
    SI_INSERT_HASH(s);
    SI_INSERT_LIST(s);
}
inline void SI_REMOVE(FindInfo *s) {
    SI_REMOVE_HASH(s);
    SI_REMOVE_LIST(s);
}
inline void SI_INIT(int size) {
    HASH_INIT(&si_table, size, FindTable);
    LIST_INIT(&si_list);
}
inline void SI_CLEANUP(){
    FindInfo* s;
    while (s = si_list.lh_first) {
        SI_REMOVE(s);
        delete s;
    }
}


//
// FILEINFO
//

static CRITICAL_SECTION csFile;

struct FileInfo;
static LIST_HEAD(FileList, FileInfo) fi_list;
static HASH_TABLE(FileTable, FileInfo) fi_table;
static USHORT highest_fid = 0;
static const int fi_size = 64;

struct FileInfo {
    USHORT fid;
    FsInterface::fhandle_t handle;
    HASH_ENTRY(FileInfo) fi_hlink;
    LIST_ENTRY(FileInfo) fi_llink;
    FileInfo(USHORT f, FsInterface::fhandle_t h):fid(f),handle(h) {}
};

#define fi_hash(fid) fid
#define fi_cmp(f, x) (f->fid != x)
#define fi_expand(f) (f->fid)

inline void FI_LOOKUP(USHORT id, FileInfo*& x) {
    HASH_LOOKUP(&fi_table, fi_hlink, nargs1, (id), fi_hash, fi_cmp, x);
}
inline void FI_INSERT_HASH(FileInfo* x) {
    HASH_INSERT(&fi_table, fi_hlink, fi_expand, x, fi_hash);
}
inline void FI_INSERT_LIST(FileInfo* x) {
    LIST_INSERT_HEAD(&fi_list, x, fi_llink);
}
inline void FI_REMOVE_HASH(FileInfo *x) {
    HASH_REMOVE(&fi_table, x, fi_hlink);
}
inline void FI_REMOVE_LIST(FileInfo *x) {
    LIST_REMOVE(x, fi_llink);
}
inline void FI_INSERT(FileInfo* x) {
    FI_INSERT_HASH(x);
    FI_INSERT_LIST(x);
}
inline void FI_REMOVE(FileInfo *x) {
    FI_REMOVE_HASH(x);
    FI_REMOVE_LIST(x);
}
inline void FI_INIT(int size) {
    HASH_INIT(&fi_table, size, FileTable);
    LIST_INIT(&fi_list);
}
inline void FI_CLEANUP(){
    FileInfo* x;
    while (x = fi_list.lh_first) {
        FI_REMOVE(x);
        delete x;
    }
}

//
// USER/UID
//

bool
is_authenticated_user(
    const char *name, 
    void* pwd,
    int pwd_size
    )
{
    if (!name || ((*name) == 0)) {
        return true;
    } else {
        return !stricmp(name, "dalmeida");
    }
}

// add_user returns 0 for the uid if we're out of users...
USHORT
add_user(
    const char *name,
    FsInterface::FsDispatchTable* pDisp
    )
{
    AutoLock lock(&csUser);
    struct UserInfo* x;
    UN_LOOKUP(name, x);
    if (x) return x->uid;
    USHORT& highest_id = highest_uid;
    // --- MODIFY ABOVE ---
    // find new id
    USHORT id;
    USHORT start = max(highest_id + 1, 1);
    bool loop = false;
    id = start;
    do {
        if (!id || (id == 0xFFFF)) { id = 1; loop = true; }
        // --- MODIFY BELOW ---
        UI_LOOKUP(id, x);
        // --- MODIFY ABOVE ---
        id++;
    } while (x && !(loop && (id >= start)));
    if (x && (id >= start)) {
        // we exhausted the table...that's bad...
        return 0;
    }
    if (id > highest_id) highest_id = id;
    // --- MODIFY BELOW ---
    x = new UserInfo(id, name, pDisp);
    U_INSERT(x);
    return id;
}

FsInterface::FsDispatchTable*
get_user(
    USHORT uid
    )
{
    AutoLock lock(&csUser);
    struct UserInfo* x;
    UI_LOOKUP(uid, x);
    if (!x) return 0;
    return x->pDisp;
}

bool
del_user(
    USHORT uid
    )
{
    AutoLock lock(&csUser);
    struct UserInfo* x;
    UI_LOOKUP(uid, x);
    if (!x) return false;
    U_REMOVE(x);
    delete x;
    return true;
}


//
// FIND/SID
//

USHORT
add_find(
    FsInterface::fhandle_t fh,
    UINT32 cookie,
    filter_t* filter,
    dirinfo_t* entry
    )
{
    AutoLock lock(&csFind);
//    Counter cnt("s_count", s_count);
    struct FindInfo* s;
    USHORT sid;
    USHORT start = max(highest_sid + 1, 1);
    bool loop = false;
    sid = start;
    do {
        if (!sid || (sid == 0xFFFF)) { sid = 1; loop = true; }
        SI_LOOKUP(sid, s);
        sid++;
    } while (s && !(loop && (sid >= start)));
    if (s && (sid >= start)) {
        // we exhausted the table...that's bad...
        return 0;
    }

    if (sid > highest_sid) highest_sid = sid;

    s = new FindInfo(sid, fh, 0, filter, entry);
    SI_INSERT(s);
//    cnt.inc();
    return sid;
}

bool
save_find_cookie(
    USHORT sid,
    UINT32 cookie
    )
{
    AutoLock lock(&csFind);
    struct FindInfo* s;
    SI_LOOKUP(sid, s);
    if (!s) {
        return false;
    }
    s->cookie = cookie;
    return true;
}

bool
get_find(
    USHORT sid,
    FsInterface::fhandle_t* pfh,
    UINT32 *pcookie,
    filter_t** pfilter,
    dirinfo_t** pentry
    )
{
    AutoLock lock(&csFind);
//    Counter cnt("s_count", s_count);
    struct FindInfo* s;
    SI_LOOKUP(sid, s);
    if (!s) {
        if (pfh) *pfh = INVALID_FHANDLE_T;
        if (pfilter) *pfilter = 0;
        return false;
    }
    if (pfh) *pfh = s->handle;
    if (pfilter) *pfilter = s->filter;
    if (pcookie) *pcookie = s->cookie;
    if (pentry) *pentry = s->entry;
    return true;
}

bool
del_find(
    USHORT sid
    )
{
    AutoLock lock(&csFind);
//    Counter cnt("s_count", s_count);
    struct FindInfo* s;
    SI_LOOKUP(sid, s);
    if (!s) {
        return false;
    }
    SI_REMOVE(s);
    delete s;
//    cnt.dec();
    return true;
}


//
// FILE/FID
//

// add_file returns 0 for the uid if we're out of fids...
USHORT
add_file(
    fhandle_t fh
    )
{
    AutoLock lock(&csFile);
//    Counter cnt("f_count", f_count);
    struct FileInfo* x;
    USHORT& highest_id = highest_fid;
    // --- MODIFY ABOVE ---
    // find new id
    USHORT id;
    USHORT start = max(highest_id + 1, 1);
    bool loop = false;
    id = start;
    do {
        if (!id || (id == 0xFFFF)) { id = 1; loop = true; }
        // --- MODIFY BELOW ---
        FI_LOOKUP(id, x);
        // --- MODIFY ABOVE ---
        id++;
    } while (x && !(loop && (id >= start)));
    if (x && (id >= start)) {
        // we exhausted the table...that's bad...
        return 0;
    }
    if (id > highest_id) highest_id = id;
    // --- MODIFY BELOW ---
    x = new FileInfo(id, fh);
    FI_INSERT(x);
//    cnt.inc();
    return id;
}

fhandle_t
get_file(
    USHORT id
    )
{
    AutoLock lock(&csFile);
//    Counter cnt("f_count", f_count);
    struct FileInfo* x;
    FI_LOOKUP(id, x);
    if (!x) return INVALID_FHANDLE_T;
    return x->handle;
}

bool
del_file(
    USHORT id
    )
{
    AutoLock lock(&csFile);
//    Counter cnt("f_count", f_count);
    struct FileInfo* x;
    FI_LOOKUP(id, x);
    if (!x) return false;
    FI_REMOVE(x);
    delete x;
//    cnt.dec();
    return true;
}


//
// INIT/CLEANUP
//

void
smbglue_init(
    void
    )
{
    InitializeCriticalSection(&csUser);
    InitializeCriticalSection(&csFind);
    InitializeCriticalSection(&csFile);
    U_INIT(u_size);
    SI_INIT(si_size);
    FI_INIT(fi_size);
}

void
smbglue_reset(
    void
    )
{
    AutoLock u_lock(&csUser);
    AutoLock s_lock(&csFind);
    AutoLock f_lock(&csFile);
    U_CLEANUP();
    SI_CLEANUP();
    FI_CLEANUP();
}

void
smbglue_cleanup(
    void
    )
{
    U_CLEANUP();
    SI_CLEANUP();
    FI_CLEANUP();
    DeleteCriticalSection(&csUser);
    DeleteCriticalSection(&csFind);
    DeleteCriticalSection(&csFile);
}
