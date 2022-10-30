#include "MemoryView.h"

MemoryView::MemoryView(Memory *mem) 
{
    this->mem = mem;
    this->base_address = 0;
}

MemoryView::~MemoryView() {}

