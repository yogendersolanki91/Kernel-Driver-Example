#ifndef MNT_FILE_IMAGE_H_INCLUDED
#define MNT_FILE_IMAGE_H_INCLUDED
#include "mntImage.h"
#include "windows.h"
class FileImage : public IImage
{
    HANDLE m_hFile;
public:
    FileImage(const wchar_t * fileName);
    ~FileImage();
    void Read(char* buf, Uint64 offset, Uint32 bytesCount);
    void Write(const char* buf, Uint64 offset, Uint32 bytesCount);
    Uint64 Size();
};
#endif