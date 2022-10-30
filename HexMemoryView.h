#pragma once
#include "MemoryView.h"

class HexMemoryView: public MemoryView
{
private:
    uint8_t* buf;
    void reset();
    void update();

public:
    HexMemoryView(Memory *mem);
    ~HexMemoryView();

    virtual std::vector<MemLine> getLines(int start, int count) override;
    virtual void setBaseAddress(uint64_t addr) override;
    virtual void setBitMode(int m) override;

};

