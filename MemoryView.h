#pragma once
#include <string>
#include <vector>
#include "Memory.h"

typedef struct {
    uint64_t address;
    std::string data;
} MemLine;

class MemoryView
{
private:

protected:
    Memory *mem;
    uint64_t base_address;

public:
    MemoryView(Memory *mem);
    ~MemoryView();

    virtual std::vector<MemLine> getLines(int start, int count) = 0;
    virtual void setBaseAddress(uint64_t addr) = 0;
    virtual void setBitMode(int mode) = 0;
};

