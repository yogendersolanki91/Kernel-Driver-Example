#include "mntSparseImage.h"
#include <assert.h>

SparseImage::SparseImage(std::auto_ptr<IImage> impl, Uint32 headerOffset, 
                               Uint64 logicSize, Uint32 blockSize, Uint32 granulity, bool fCreate):
    impl_(impl),
    headerOffset_(headerOffset),
    logicSize_(logicSize), blockSize_(blockSize),
    indexTable_(logicSize, blockSize, granulity)
{
    assert(headerOffset%granulity == 0);
    assert(blockSize%granulity == 0);
    if(fCreate)
        WriteTable();
    else
        ReadTable();
}
void SparseImage::WriteTable()
{
    impl_->Write(indexTable_.getBuf(), headerOffset_, indexTable_.getSize());
}
void SparseImage::ReadTable()
{
    impl_->Read(indexTable_.getBuf(), headerOffset_, indexTable_.getSize());
    indexTable_.getLogBlock();
}
void SparseImage::ReadFromBlock(char* pBuffer, Uint32 blockIndex, Uint32 offset, Uint32 size)
{
    Uint32 blockToken = indexTable_.at(blockIndex);
    assert(blockToken != s_unusedBlokToken);
    Uint64 read_offset = headerOffset_ + indexTable_.getSize() + blockToken * blockSize_ + offset;
    impl_->Read(pBuffer, read_offset, size);
}
void SparseImage::WriteToBlock(const char* pBuffer, Uint32 blockIndex, Uint32 offset, Uint32 size)
{
    Uint32 blockToken = indexTable_.at(blockIndex);
    assert(blockToken != s_unusedBlokToken);
    Uint64 write_offset = headerOffset_ + indexTable_.getSize() + blockToken * blockSize_ + offset;
    impl_->Write(pBuffer, write_offset, size);
}
void SparseImage::AddNewBlock(Uint32 index)
{
    Uint32 usedCount = indexTable_.getUsedCount();
    indexTable_.set(index, usedCount);
    try
    {
        WriteTable();
    }
    catch(...)
    {
        indexTable_.reset(index);
        throw;
    }
}
void SparseImage::Read(char*  pBuffer,
                          Uint64 offset,
                          Uint32 length)
{
    Uint32 firstBlockIndex = boost::numeric_cast<Uint32>(offset / blockSize_);
    Uint32 lastBlockIndex = boost::numeric_cast<Uint32>((offset + length) / blockSize_);
    Uint32 firstBlockOffset = offset % blockSize_;
    Uint32 lastBlockOffset = (offset + length) % blockSize_;

    Uint32 bytesWriten = 0;
    for(Uint32 currentBlockIndex = firstBlockIndex;
        currentBlockIndex <= lastBlockIndex && bytesWriten < length;
        ++currentBlockIndex)
    {
        if(currentBlockIndex == lastBlockIndex && lastBlockOffset == 0)
            break;
        Uint32 currentBlockToken = indexTable_.at(currentBlockIndex);
        if(currentBlockToken != s_unusedBlokToken)
        {
            Uint32 begin = 0;
            Uint32 end = blockSize_;
            if(currentBlockIndex == firstBlockIndex)
                begin = firstBlockOffset;
            if(currentBlockIndex == lastBlockIndex)
                end = lastBlockOffset;
            assert(end >= begin);
            ReadFromBlock(pBuffer, currentBlockIndex, begin, end - begin);

            pBuffer += (end - begin);
            bytesWriten += (end - begin);
        }
    }
}

void SparseImage::Write(const char*   pBuffer,
                           Uint64 offset,
                           Uint32 length)
{
    Uint32 firstBlockIndex = boost::numeric_cast<Uint32>(offset / blockSize_);
    Uint32 lastBlockIndex = boost::numeric_cast<Uint32>((offset + length) / blockSize_);
    Uint32 firstBlockOffset = offset % blockSize_;
    Uint32 lastBlockOffset = (offset + length) % blockSize_;

    Uint32 bytesWriten = 0;
    for(Uint32 currentBlockIndex = firstBlockIndex;
        currentBlockIndex <= lastBlockIndex && bytesWriten < length;
        ++currentBlockIndex)
    {
        Uint32 currentBlockToken = indexTable_.at(currentBlockIndex);
        if(currentBlockToken == s_unusedBlokToken)
            AddNewBlock(currentBlockIndex);

        Uint32 begin = 0;
        Uint32 end = blockSize_;
        if(currentBlockIndex == firstBlockIndex)
            begin = firstBlockOffset;
        if(currentBlockIndex == lastBlockIndex)
            end = lastBlockOffset;
        assert(end >= begin);
        WriteToBlock(pBuffer, currentBlockIndex, begin, end - begin);

        pBuffer += (end - begin);
        bytesWriten += (end - begin);
    }
}
