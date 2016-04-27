#ifndef MNT_IMAGE_H_INCLUDED
#define MNT_IMAGE_H_INCLUDED
#include "mntCmn.h"
struct IImage
{
    IImage():devId_(0), mountPoint_(0) {}
    virtual ~IImage(){}
    virtual void Read(char* buf, Uint64 offset, Uint32 bytesCount) = 0;
    virtual void Write(const char* buf, Uint64 offset, Uint32 bytesCount) = 0;
    virtual Uint64 Size() = 0;

    int GetId() {return devId_;}
    void SetId(int devId) {devId_ = devId;}
    wchar_t GetMountPoint() {return mountPoint_;}
    void SetMountPoint(int mountPoint) {mountPoint_ = mountPoint;}
private:
    int devId_;
    wchar_t mountPoint_;
};
#endif