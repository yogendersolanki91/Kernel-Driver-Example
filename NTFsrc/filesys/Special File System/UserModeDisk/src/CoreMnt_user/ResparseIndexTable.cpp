#include "ResparseIndexTable.h"
#include <algorithm>

inline Uint32 CalcDataOffset(Uint64 logicSize, Uint32 blockSize, Uint32 granulity)
{
    Uint64 ungranulParam = sizeof(IndexTableHeader) + 
                           s_integer_div(logicSize, blockSize)*sizeof(Uint32);
    return s_granulity_to(ungranulParam, granulity);
}

CResparseIndexTable::CResparseIndexTable(Uint64 logicSize, Uint32 blockSize, Uint32 granulity):
    blockSize_(blockSize), 
    dataOffset_(CalcDataOffset(logicSize, blockSize, granulity))
{
    tableData_.resize(dataOffset_);
    tableHeader_ = (IndexTableHeader*)&tableData_[0];
    tableHeader_->indexTableSize = s_integer_div(logicSize, blockSize);
    tableHeader_->usedBlockCount = 0;
    tableHeader_->currentLogOffset = blockSize;
    tableHeader_->firstLogBlock = s_unusedBlokToken;
    tableHeader_->currentLogBlock = s_unusedBlokToken;

    std::fill(&tableHeader_->table[0], &tableHeader_->table[tableHeader_->indexTableSize], s_unusedBlokToken);
}
void CResparseIndexTable::getLogBlock()
{
    std::set<Uint32> usedBlock;
    for(Uint32 i = 0; i < tableHeader_->indexTableSize; ++i)
        if(tableHeader_->table[i] != s_unusedBlokToken)
            usedBlock.insert(tableHeader_->table[i]);

    if(tableHeader_->currentLogBlock != s_unusedBlokToken)
        for(Uint32 i = 0; i < tableHeader_->currentLogBlock; ++i)
            if(usedBlock.find(i) == usedBlock.end())
                logBlocks_.insert(i);
}
Uint64 CResparseIndexTable::getMessageBlocksSize()
{
    Uint64 retOffset = 0;
    for(std::set<Uint32>::iterator i = logBlocks_.begin();i != logBlocks_.end(); ++i)
        retOffset += blockSize_;
    if(tableHeader_->currentLogBlock != s_unusedBlokToken)
        retOffset += tableHeader_->currentLogOffset;
    return retOffset;
}
Uint64 CResparseIndexTable::LogMessageOffset2FileOffset(Uint64 virtualOffset)
{
    Uint64 maxRange = getMessageBlocksSize();
    if(maxRange < virtualOffset)
        throw std::runtime_error("Offset out of range ");
    Uint32 logBlockIndex = boost::numeric_cast<Uint32>(virtualOffset/blockSize_);
    Uint32 logBlockOffset = virtualOffset%blockSize_;
    Uint32 logBlockNum = s_unusedBlokToken;
    Uint32 index = 0;
    for(std::set<Uint32>::iterator i = logBlocks_.begin();i != logBlocks_.end(); ++i, ++index)
        if(index == logBlockIndex)
            logBlockNum = *i;
    if(logBlockIndex == logBlocks_.size())
        logBlockNum = tableHeader_->currentLogBlock;

    return logBlockNum*blockSize_ + logBlockOffset;
}
Uint64 CResparseIndexTable::getNextLogMessagePosition(Uint32 messageSize)
{
    if(messageSize > blockSize_ - tableHeader_->currentLogOffset)
    {//need add new block
        tableHeader_->currentLogBlock = tableHeader_->usedBlockCount;
        tableHeader_->currentLogOffset = 0;
        ++(tableHeader_->usedBlockCount);
        if(tableHeader_->firstLogBlock == s_unusedBlokToken)
            tableHeader_->firstLogBlock = tableHeader_->currentLogBlock;
        else
            logBlocks_.insert(tableHeader_->currentLogBlock);
    }
    if(messageSize > blockSize_ - tableHeader_->currentLogOffset)
        throw std::runtime_error("Message too long.");    
    Uint64 offset = tableHeader_->currentLogBlock * blockSize_ + tableHeader_->currentLogOffset;
    tableHeader_->currentLogOffset += messageSize;
    return offset;
}
Uint32 CResparseIndexTable::at(Uint32 index)
{
    if(index >= tableHeader_->indexTableSize)
        throw std::runtime_error("Index out of range.");
    return tableHeader_->table[index];
}
void CResparseIndexTable::set(Uint32 index, Uint32 val)
{
    if(index >= tableHeader_->indexTableSize)
        throw std::runtime_error("Index out of range.");
    if(tableHeader_->table[index] != s_unusedBlokToken || val == s_unusedBlokToken)
        throw std::runtime_error("User reset for free block.");
    tableHeader_->table[index] = val;
    ++(tableHeader_->usedBlockCount);
}
void CResparseIndexTable::reset(Uint32 index)
{
    if(index >= tableHeader_->indexTableSize)
        throw std::runtime_error("Index out of range.");
    if(tableHeader_->table[index] == s_unusedBlokToken)
        throw std::runtime_error("User reset for free block.");
    tableHeader_->table[index] = s_unusedBlokToken;
    --(tableHeader_->usedBlockCount);
}