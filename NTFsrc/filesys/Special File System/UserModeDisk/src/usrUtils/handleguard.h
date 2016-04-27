#ifndef HANDLE_GUARD_H
#define HANDLE_GUARD_H
#include "windows.h"
#include <string>
#include "winsvc.h"
#include "algorithm"

namespace utils
{
    class CSCHandleGuard
    {
        SC_HANDLE h_;
        CSCHandleGuard(CSCHandleGuard&);
        CSCHandleGuard& operator=(CSCHandleGuard&);
    public:
        explicit CSCHandleGuard(SC_HANDLE h)
            :h_(h){}
            ~CSCHandleGuard(void)
            {
                if(h_)CloseServiceHandle(h_);
            }
    };
    class CHandleGuard
    {
        HANDLE h_;
        CHandleGuard(CHandleGuard&);
        CHandleGuard& operator=(CHandleGuard&);
    public:
        explicit CHandleGuard(HANDLE h=0)
            :h_(h){}
            ~CHandleGuard(void)
            {
                if(h_)CloseHandle(h_);
            }
            HANDLE get() const {return h_;}
            HANDLE release()
            {
                HANDLE tmp = h_;
                h_ = 0;
                return tmp;
            }
            void reset(HANDLE h)
            {
                if(h_)CloseHandle(h_);
                h_ = h;
            }
    };

    class CFileCloserGuard
    {
        FILE* pFile_;
        CFileCloserGuard(CFileCloserGuard&);
        CFileCloserGuard& operator=(CFileCloserGuard&);
    public:
        explicit CFileCloserGuard(FILE* pFile=0)
            :pFile_(pFile){}
            ~CFileCloserGuard(void)
            {
                if(pFile_) fclose(pFile_);
            }
            FILE* get() const {return pFile_;}
            FILE* release()
            {
                FILE* tmp = pFile_;
                pFile_ = 0;
                return tmp;
            }
            void reset(FILE* pFile)
            {
                if(pFile_)fclose(pFile_);
                pFile_ = pFile;
            }
    };

    class CFindHandleGuard
    {
        HANDLE h_;
        CFindHandleGuard(CFindHandleGuard&);
        CFindHandleGuard& operator=(CFindHandleGuard&);
    public:
        explicit CFindHandleGuard(HANDLE h)
            :h_(h){}
            ~CFindHandleGuard(void)
            {
                if(h_)::FindClose(h_);
            }
            HANDLE get(){return h_;}
            HANDLE release()
            {
                HANDLE tmp = h_;
                h_ = 0;
                return tmp;
            }
            void reset(HANDLE h)
            {
                if(h_)::FindClose(h_);
                h_ = h;
            }
    };
    class CFileDeleterGuard
    {
        std::wstring fileName_;
        CFileDeleterGuard(CFileDeleterGuard&);
        CFileDeleterGuard& operator=(CFileDeleterGuard&);
    public:
        explicit CFileDeleterGuard(std::wstring fileName)
            :fileName_(fileName){}
            ~CFileDeleterGuard(void)
            {
                if(!fileName_.empty())
                    DeleteFileW(fileName_.c_str());
            }
            std::wstring get(){return fileName_;}
            std::wstring release()
            {
                std::wstring tmp = fileName_;
                fileName_.clear();
                return tmp;
            }
    };
    class CDCGuard
    {
        HDC h_;
        CDCGuard(CDCGuard&);
        CDCGuard& operator=(CDCGuard&);
    public:
        explicit CDCGuard(HDC h)
            :h_(h){}
            ~CDCGuard(void)
            {
                if(h_)DeleteDC(h_);
            }
    };
    class CBitMapGuard
    {
        HBITMAP h_;
        CBitMapGuard(CBitMapGuard&);
        CBitMapGuard& operator=(CBitMapGuard&);
    public:
        explicit CBitMapGuard(HBITMAP h)
            :h_(h){}
            ~CBitMapGuard(void)
            {
                if(h_)DeleteObject(h_);
            }
    };

    class CLibGuard
    {
        HMODULE h_;
        CLibGuard(const CLibGuard&);
        CLibGuard& operator=(const CLibGuard&);
    public:
        CLibGuard()
            : h_(0)
        {
        }
        explicit CLibGuard(HMODULE h)
            :h_(h){}

        void reset(HMODULE h)
        {
            if(h_) FreeLibrary(h_);
            h_ = h;
        }
        ~CLibGuard()
        {
            if(h_) FreeLibrary(h_);
        }
        HMODULE release()
        {
            HMODULE hRes = h_;
            h_ = 0;
            return hRes;
        }
		HMODULE get()
		{
			return h_;
		}
    };

 class CMallocGuard
    {
        void * h_;
        CMallocGuard(const CMallocGuard&);
        CMallocGuard& operator=(const CMallocGuard&);
    public:
        explicit CMallocGuard(void * h)
            :h_(h){}
            ~CMallocGuard()
            {
                if(h_) free(h_); 
            }
            void * release()
            {
                void * hRes = h_;
                h_ = 0;
                return hRes;
            }
    };
    class CLocalAllocGuard
    {
        void * h_;
        CLocalAllocGuard(const CLocalAllocGuard&);
        CLocalAllocGuard& operator=(const CLocalAllocGuard&);
    public:
        explicit CLocalAllocGuard(void * h)
            :h_(h){}
        ~CLocalAllocGuard()
        {
            if(h_) LocalFree(h_); 
        }
        void * release()
        {
            void * hRes = h_;
            h_ = 0;
            return hRes;
        }
    };
    class CVirtualAllocGuard
    {
    public:
        void * h_;
        CVirtualAllocGuard(const CVirtualAllocGuard&);
        CVirtualAllocGuard& operator=(const CVirtualAllocGuard&);
    public:
        explicit CVirtualAllocGuard(void * h)
            :h_(h){}
            ~CVirtualAllocGuard()
            {
                if(h_) VirtualFree(h_, 0, MEM_RELEASE);
            }
            void * release()
            {
                void * hRes = h_;
                h_ = 0;
                return hRes;
            }
    };
    class CRegKeyGuard
    {
        HKEY hKey_;
        CRegKeyGuard(const CRegKeyGuard&);
        CRegKeyGuard& operator=(const CRegKeyGuard&);
    public:
        explicit CRegKeyGuard(HKEY hKey)
            :hKey_(hKey){}
            ~CRegKeyGuard()
            {
                if(hKey_) RegCloseKey(hKey_);
            }
            void * release()
            {
                HKEY hKey = hKey_;
                hKey_ = 0;
                return hKey;
            }
    };


    // for tokens:
    //class CHandleGuardImpersonated
    //{
    //    HANDLE m_hToken;
    //    bool m_bRevertToSelf;

    //    CHandleGuardImpersonated(const CHandleGuardImpersonated&);
    //    CHandleGuardImpersonated & operator = (const CHandleGuardImpersonated&);
    //public:
    //    CHandleGuardImpersonated(HANDLE hToken=0)
    //        : m_hToken(hToken), m_bRevertToSelf(false)
    //    {
    //    }
    //    void Init(HANDLE hToken)
    //    {
    //        Close();
    //        m_hToken = hToken;
    //    }
    //    void reset(HANDLE hToken)
    //    {
    //        Init(hToken);
    //    }
    //    HANDLE get()
    //    {
    //        return m_hToken;
    //    }
    //    void Revert()
    //    {
    //        if (!m_bRevertToSelf)
    //            return;

    //        if (!m_hToken)
    //            throw std::runtime_error("Cannot revert: no token");

    //        RevertToSelf();
    //        m_bRevertToSelf = false;
    //    }
    //    bool IsImpersonated() const
    //    {
    //        return m_bRevertToSelf;
    //    }
    //    void Impersonate()
    //    {
    //        if (!m_hToken)
    //            throw std::runtime_error("Cannot impersonate: no token");

    //        if (m_bRevertToSelf)
    //            throw std::runtime_error("Cannot impersonate twice");

    //        if (!ImpersonateLoggedOnUser(m_hToken))
    //            UTILS_THROW_WIN32_EXCEPTION("Cannot impersonate user");
    //        m_bRevertToSelf = true;
    //    }
    //    void Close()
    //    {
    //        if (m_hToken)
    //        {
    //            if (m_bRevertToSelf)
    //                RevertToSelf();

    //            CloseHandle(m_hToken);
    //            m_hToken = 0;
    //        }
    //    }
    //    void swap(CHandleGuardImpersonated & other)
    //    {
    //        std::swap(m_hToken, other.m_hToken);
    //        std::swap(m_bRevertToSelf, other.m_bRevertToSelf);
    //    }
    //    ~CHandleGuardImpersonated()
    //    {
    //        Close();
    //    }
    //};

    //template<class ObjType>
    //class AutoImpersonation
    //{
    //    ObjType * m_pObj;

    //    AutoImpersonation(const AutoImpersonation&);
    //    AutoImpersonation&operator = (const AutoImpersonation&);
    //public:
    //    AutoImpersonation(ObjType & obj)
    //        : m_pObj(&obj)
    //    {
    //        m_pObj->Impersonate();
    //    }
    //    ~AutoImpersonation()
    //    {
    //        m_pObj->Revert();
    //    }
    //};



 }//namespace utils
#endif
