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
    // TODO: Make sure we don't go past the memory region's end
    std::vector<MemLine> ret;
    for (int i = 0; i < count; i++)
    {
        MemLine m;
        uint64_t offset = ((start+i)*16);
        m.address = this->base_address + offset;
        std::stringstream ss;
        for (int i = 0; i < 16; i++)
        {
            int d = buf[offset+i];
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
    this->buf = (uint8_t*)this->mem->getPointer(this->base_address);
}

void HexMemoryView::update()
{

}

void HexMemoryView::setBitMode(int m)
{
}
