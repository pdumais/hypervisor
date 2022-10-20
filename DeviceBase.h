#pragma once
#include <string>
#include "Memory.h"

class DeviceBase 
{
protected:
    unsigned long memsize;
    std::string name;
    void* mem;
    int memSlot;
    unsigned int id;
    int irq;
    int eventFd;
    Memory *memoryManager;
    bool memDirty();
    void kickIRQ();

public:
    DeviceBase(unsigned int id, std::string name, unsigned long memsize);
    virtual ~DeviceBase();

    void register_memory(Memory* m, uintptr_t phys);
    unsigned long getMemorySize();
    unsigned int getId();
    int getEventFd();
    std::string getName();

    virtual void handlePortWrite(unsigned int data);
    virtual uint32_t handlePortRead();
};