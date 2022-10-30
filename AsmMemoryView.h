#pragma once
#include "MemoryView.h"
#include <udis86.h>

class AsmMemoryView: public MemoryView
{
private:
    typedef struct
    {
        std::string str;
        uint64_t addr;
        uint64_t size;
    } instruction;

    ud_t current_ud_obj;
    std::vector<instruction> instructions;
    int bit_mode;

protected:
    void reset();
    void update();


public:
    AsmMemoryView(Memory *mem);
    ~AsmMemoryView();

    virtual std::vector<MemLine> getLines(int start, int count) override;
    virtual void setBaseAddress(uint64_t addr) override;
    virtual void setBitMode(int m) override;

};

