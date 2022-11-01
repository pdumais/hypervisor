#include "Debugger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include "AsmMemoryView.h"
#include "HexMemoryView.h"

#define NLINES 20
#define MODE_ASM 0
#define MODE_HEX 1
/*
USAGE:

'g': enter a hex address to set the base address to that address.
     you can no longer go before that location. 
'm': toggle between hex view and disassembler view
'b': toggle between 16/32/64 bit mode (for disassembler view only)

*/


Debugger::Debugger(VirtualMachine* vm, Memory *m) 
{
    //TODO: handle the case where Memory has not been setup yet. 
    this->base_line_pointer = 0;
    this->vm = vm;
    this->cursor = 0;
    this->mem = m;
    this->mode = MODE_ASM;
    this->current_cpu_view = 0;
    this->bit_mode = 16;
    this->views.push_back(new AsmMemoryView(m));
    this->views.push_back(new HexMemoryView(m));
}

Debugger::~Debugger()
{
}

void Debugger::refreshView()
{
    clear();
    this->dumpMem();
    this->dumpCpuState(this->current_cpu_view);
    move(0,0);
    refresh();
}

void Debugger::dumpMem()
{
    MemoryView* m = this->views[this->mode];
    auto list = m->getLines(this->base_line_pointer, NLINES);
    int line = 0;
    for (auto& ml : list)
    {
        std::stringstream ss;
        ss << "0x" << std::setfill('0') << std::setw(16) << std::hex << ml.address << ": ";
        ss << ml.data;

        move(line, 0);
        if (line == this->cursor) attron(A_STANDOUT);
        printw(ss.str().c_str());
        attroff(A_STANDOUT);
        line++;
    }
}

void Debugger::dumpCpuState(int cpu)
{
    if (!this->vm->isPaused()) return;
    int col = 70;

    VirtualMachine::state s = this->vm->getState(cpu);
    move(0,col);
    printw("CPU %i state:",cpu);
    if (!s.valid) return;

    move(1,col+4);
    printw("rip = %016llx (%016lx)",s.regs.rip, s.phys_rip);
    move(2,col+4);
    printw("rax = %016llx",s.regs.rax);
    move(3,col+4);
    printw("rbx = %016llx",s.regs.rbx);
    move(4,col+4);
    printw("rcx = %016llx",s.regs.rcx);
    move(5,col+4);
    printw("rdx = %016llx",s.regs.rdx);
    move(6,col+4);
    printw("rdi = %016llx (%016lx)",s.regs.rdi, s.phys_rdi);
    move(7,col+4);
    printw("rsi = %016llx (%016lx)",s.regs.rsi, s.phys_rsi);
    move(8,col+4);
    printw("rflags = %016llx",s.regs.rflags);
    move(9,col+4);
    printw("rsp = %016llx (%016lx)",s.regs.rsp, s.phys_rsp);

}

void Debugger::modeToNewBaseAddress(uint64_t addr)
{
    this->base_line_pointer = 0;
    this->cursor = 0;
    for (MemoryView* m : this->views) m->setBaseAddress(addr);
}


void Debugger::start()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();

    this->win = newwin(25, 80, 0, 0);

    int ch = 0;
    this->refreshView();
    while (ch != 27 && ch != 'q')
    {
        ch = getch();
        switch (ch)
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                this->current_cpu_view = ch-48;
                this->refreshView();
            }
            break;
            case KEY_UP:
            {
                if (cursor == 0)
                {
                    if (this->base_line_pointer > 0)
                    {
                        this->base_line_pointer--;
                        this->refreshView();
                    }
                }
                else
                {
                    cursor--;
                    this->refreshView();
                }
            }
            break;
            case KEY_DOWN:
            {
                if (cursor == NLINES-1)
                {
                    this->base_line_pointer++;
                    this->refreshView();
                }
                else
                {
                    cursor++;
                    this->refreshView();
                }
            }
            break;
            case KEY_PPAGE:
            {
                if (this->base_line_pointer >= (NLINES))
                {
                    this->base_line_pointer -= (NLINES);
                    this->refreshView();
                }
            }
            break;
            case KEY_NPAGE:
            {
                this->base_line_pointer += (NLINES);
                this->refreshView();
            }
            break;
            case 'b':
            {
                this->bit_mode += this->bit_mode;
                if (this->bit_mode > 64) this->bit_mode = 16;
                for (MemoryView* m : this->views) m->setBitMode(this->bit_mode);
                this->refreshView();
            }
            break;
            case 'm':
            {
                if (this->mode == MODE_HEX) this->mode = MODE_ASM; else this->mode = MODE_HEX;
                this->refreshView();
            }
            break;
            case 'p':
            {
                this->vm->pause();
                VirtualMachine::state s = this->vm->getState(this->current_cpu_view);
                if (s.valid)
                {
                    this->modeToNewBaseAddress(s.phys_rip);
                    this->refreshView();
                }
            }   
            break;
            case 'c':
            {
                this->vm->resume();
                this->refreshView();
            }   
            break;         
            case 'g':
            {
                char str[256];
                echo();
                move(NLINES+1, 0);
                printw("g ");
                getstr(str);
                noecho();
                uint64_t a;
                std::string s(str);
                a = std::stoi(s, 0, 16);
                this->modeToNewBaseAddress(a);
                this->refreshView();
            }
            break; 
        }
    }

    endwin();
}

