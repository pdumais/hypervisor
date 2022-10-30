#include "AsmMemoryView.h"
#include <sstream>
#include <iomanip>

AsmMemoryView::AsmMemoryView(Memory *mem) : MemoryView(mem)
{
    this->reset();
    this->update();
}

AsmMemoryView::~AsmMemoryView() {}

std::vector<MemLine> AsmMemoryView::getLines(int start, int count)
{
    // TODO: Make sure we don't go past the memory region's end
    // TODO: we're letting the cache grow without limit. 
    //       we should instead set a max size and discard old data
    //       but this is challenging because of the way the disasm works.

    // We must first make sure that the case has what we need
    while (this->instructions.size() < (start+count))
    {
        int r = ud_disassemble(&current_ud_obj);
        if (!r) break;

        instruction inst;
        inst.size = ud_insn_len(&current_ud_obj);

        std::stringstream ss;
        ss << std::left << std::setw(22) << ud_insn_hex(&current_ud_obj);
        ss << ud_insn_asm(&current_ud_obj);
        inst.str = ss.str();
        inst.addr = ud_insn_off(&current_ud_obj) + this->base_address;
        this->instructions.push_back(inst);
    }

    std::vector<MemLine> ret;
    for (int i = 0; i < count; i++)
    {
        if ((i+start) >= this->instructions.size()) break;

        MemLine m;
        m.address = this->instructions[i+start].addr;
        m.data = this->instructions[i+start].str;
        ret.push_back(m);
    }
    return ret;
}

void AsmMemoryView::setBaseAddress(uint64_t addr)
{
    this->base_address = addr;
    this->reset();
    this->update();
}

void AsmMemoryView::reset()
{
    //TODO: get the right memory region instead
    this->instructions = std::vector<instruction>();
    ud_init(&current_ud_obj);      
    ud_set_mode(&current_ud_obj, this->bit_mode);      
    ud_set_input_buffer(&current_ud_obj, (uint8_t*)this->mem->getPointer(0), this->mem->getSize(0)); //TODO Set appropriate size
    ud_set_syntax(&current_ud_obj, UD_SYN_INTEL);
    ud_input_skip(&current_ud_obj, this->base_address);
}

void AsmMemoryView::update()
{

}

void AsmMemoryView::setBitMode(int m)
{
    this->bit_mode = m;
    this->reset();
    this->update();
}
