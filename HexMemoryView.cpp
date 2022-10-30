#include "HexMemoryView.h"
#include <sstream>
#include <iomanip>

HexMemoryView::HexMemoryView(Memory *mem) : MemoryView(mem)
{
    this->reset();
    this->update();
}

HexMemoryView::~HexMemoryView() {}

std::vector<MemLine> HexMemoryView::getLines(int start, int count)
{
    std::vector<MemLine> ret;
    for (int i = 0; i < count; i++)
    {
        MemLine m;
        m.address = this->base_address + ((start+i)*16);
        std::stringstream ss;
        for (int i = 0; i < 16; i++)
        {
            int d = buf[m.address+i];
            ss << std::setfill('0') << std::setw(2) << std::hex << d << " ";
        }
        m.data = ss.str();
        ret.push_back(m);
    }
    return ret;
}

void HexMemoryView::setBaseAddress(uint64_t addr)
{
    this->base_address = addr;
    this->reset();
    this->update();
}

void HexMemoryView::reset()
{
    this->buf = (uint8_t*)this->mem->getPointer(0);
}

void HexMemoryView::update()
{

}

void HexMemoryView::setBitMode(int m)
{
}
