#ifndef MNT_SPARSE_IMAGE_H_INCLUDED
#define MNT_SPARSE_IMAGE_H_INCLUDED
#include <vector>
#include <set>
#include "mntImage.h"
#include "ResparseIndexTable.h"
class SparseImage: public IImage
{
    std::auto_ptr<IImage> impl_;

    const Uint64 logicSize_;
    const Uint32 blockSize_;
    const Uint32 headerOffset_;

    CResparseIndexTable indexTable_;

    void WriteTable();
    void ReadTable();
    void AddNewBlock(Uint32 index);
    void ReadFromBlock(char* pBuffer, Uint32 blockIndex, Uint32 offset, Uint32 size);
    void WriteToBlock(const char* pBuffer, Uint32 blockIndex, Uint32 offset, Uint32 size);

    SparseImage();
    SparseImage(const SparseImage&);
    SparseImage operator=(const SparseImage&);
public:
    SparseImage::SparseImage(std::auto_ptr<IImage> impl, Uint32 headerOffset, 
        Uint64 logicSize, Uint32 blockSize, Uint32 granulity, bool fCreate);
    void Read(char* out_buf, Uint64 offset, Uint32 bytesToRead);
    void Write(const char*   pBuffer,Uint64 offset,Uint32 length);

    Uint64 Size()
    {
        return logicSize_;
    }
};

#endif