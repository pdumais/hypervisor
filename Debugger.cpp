#include "Debugger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include "AsmMemoryView.h"
#include "HexMemoryView.h"

#define NLINES 20
#define MODE_ASM 0
#define MODE_HEX 1



Debugger::Debugger(Memory *m) 
{
    //TODO: handle the case where Memory has not been setup yet. 
    this->current_line_pointer = 0;
    this->mem = m;
    this->mode = MODE_ASM;
    this->bit_mode = 16;
    this->views.push_back(new AsmMemoryView(m));
    this->views.push_back(new HexMemoryView(m));
}

Debugger::~Debugger()
{
}

void Debugger::dump_mem()
{
    clear();

    MemoryView* m = this->views[this->mode];
    auto list = m->getLines(this->current_line_pointer, NLINES);
    int line = 0;
    for (auto& ml : list)
    {
        std::stringstream ss;
        ss << "0x" << std::setfill('0') << std::setw(16) << std::hex << ml.address << ": ";
        ss << ml.data;

        move(line, 0);
        printw(ss.str().c_str());
        line++;
    }

    move(0,0);
    refresh();
}

void Debugger::start()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    this->win = newwin(25, 80, 0, 0);

    int ch = 0;
    while (ch != 27 && ch != 'q')
    {
        this->dump_mem();
        ch = getch();
        switch (ch)
        {
            case KEY_UP:
            {
                if (this->current_line_pointer > 0)
                {
                    this->current_line_pointer--;
                    this->dump_mem();
                }
            }
            break;
            case KEY_DOWN:
            {
                //TODO: dont go past the end of the buffer
                this->current_line_pointer++;
                this->dump_mem();
            }
            break;
            case KEY_PPAGE:
            {
                if (this->current_line_pointer >= (NLINES))
                {
                    this->current_line_pointer -= (NLINES);
                    this->dump_mem();
                }
            }
            break;
            case KEY_NPAGE:
            {
                this->current_line_pointer += (NLINES);
                this->dump_mem();
            }
            break;
            case 'b':
            {
                this->bit_mode += this->bit_mode;
                if (this->bit_mode > 64) this->bit_mode = 16;
                for (MemoryView* m : this->views) m->setBitMode(this->bit_mode);
                this->dump_mem();
            }
            break;
            case 'm':
            {
                if (this->mode == MODE_HEX) this->mode = MODE_ASM; else this->mode = MODE_HEX;
                this->dump_mem();
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
                this->current_line_pointer = 0;
                for (MemoryView* m : this->views) m->setBaseAddress(a);                
                this->dump_mem();
            }
            break; 
        }
    }

    endwin();
}

