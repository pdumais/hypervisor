#pragma once
#include <curses.h>
#include "Memory.h"
#include "MemoryView.h"

class Debugger
{
private:
    uint64_t current_line_pointer;
    WINDOW *win;
    Memory *mem;
    int mode;
    int bit_mode;
    std::vector<MemoryView*> views;

    void dump_mem();


public:
    Debugger(Memory *m);
    ~Debugger();

    void start();
};

