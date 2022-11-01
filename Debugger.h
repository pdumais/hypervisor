#pragma once
#include <curses.h>
#include "Memory.h"
#include "MemoryView.h"
#include "VirtualMachine.h"

class Debugger
{
private:
    uint64_t base_line_pointer;
    uint64_t cursor;

    WINDOW *win;
    Memory *mem;
    VirtualMachine *vm;
    int mode;
    int bit_mode;
    std::vector<MemoryView*> views;
    int current_cpu_view;

    void dumpMem();
    void dumpCpuState(int cpu);
    void modeToNewBaseAddress(uint64_t addr);

public:
    Debugger(VirtualMachine* vm, Memory *m);
    ~Debugger();

    void start();
};

