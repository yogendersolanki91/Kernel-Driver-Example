#ifndef RESPARSE_INDEX_TABLE_H_INCLUDED
#define RESPARSE_INDEX_TABLE_H_INCLUDED
#include <vector>
#include <set>
#include "mntCmn.h"
#include "boost/cast.hpp"
const static Uint32 s_unusedBlokToken = 0xFFFFFFFF;

inline static Uint32 s_integer_div(Uint64 size, Uint32 block)
{
    Uint32 result = boost::numeric_cast<Uint32>(size/block);
    if (size%block)
        ++result;
    return result;
}
inline static Uint32 s_granulity_to(Uint64 size, Uint32 block)
{
    Uint64 result = (size/block)*block;
    if (size%block)
        result += block;
    return boost::numeric_cast<Uint32>(result);
}

#pragma pack(push, 1) 
#pragma warning(push)
#pragma warning(disable : 4200)
    struct IndexTableHeader
    {
        Uint32 indexTableSize;
        Uint32 usedBlockCount;
        Uint32 firstLogBlock;
        Uint32 currentLogBlock;
        Uint32 currentLogOffset;
        Uint32 table[0];
    };
#pragma warning(pop)
#pragma pack(pop) 

class CResparseIndexTable
{
    const Uint32 dataOffset_;
    const Uint32 blockSize_;
    std::vector<char> tableData_;
    CResparseIndexTable();
    CResparseIndexTable(const CResparseIndexTable&);
    const CResparseIndexTable& operator=(const CResparseIndexTable&);
    IndexTableHeader* tableHeader_;

    std::set<Uint32> logBlocks_;

public:
    CResparseIndexTable(Uint64 logicSize, Uint32 blockSize, Uint32 granulity);
    void getLogBlock();
    char* getBuf(){return &tableData_[0];}
    Uint32 getSize(){return dataOffset_;}
    Uint32 getUsedCount(){return tableHeader_->usedBlockCount;}
    Uint64 getMessageBlocksSize();
    Uint64 LogMessageOffset2FileOffset(Uint64 virtualOffset);
    Uint64 getNextLogMessagePosition(Uint32 messageSize);
    Uint32 at(Uint32 index);
    void set(Uint32 index, Uint32 val);
    void reset(Uint32 index);
};
#endif