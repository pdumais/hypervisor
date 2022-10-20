#include "DeviceBase.h"
#include <sys/mman.h>
#include <sys/eventfd.h>
#include <unistd.h>


DeviceBase::DeviceBase(unsigned int id, std::string name, unsigned long memsize)
{
    this->memoryManager = 0;
    this->memSlot = -1;
    this->id = id;
    this->name = name;
    this->memsize = (memsize+0xFFF)&0xFFFFF000; // round up to next 4k
    this->mem = mmap(0, this->memsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    this->eventFd = eventfd(0, EFD_CLOEXEC);
}

DeviceBase::~DeviceBase()
{
    //TODO: free this->mem
}

void DeviceBase::handlePortWrite(unsigned int data)
{
    
}

uint32_t DeviceBase::handlePortRead()
{
    return 0;
}

unsigned long DeviceBase::getMemorySize()
{
    return this->memsize;
}

std::string DeviceBase::getName()
{
    return this->name;
}

unsigned int DeviceBase::getId()
{
    return this->id;
}

bool DeviceBase::memDirty()
{
    if (!this->memoryManager) return false;
    return this->memoryManager->isMemoryDirty(this->memSlot, this->memsize);
}

void DeviceBase::register_memory(Memory* m, uintptr_t phys)
{
    this->memoryManager = m;
    this->memSlot = m->createRegion(this->mem, this->memsize, phys, KVM_MEM_LOG_DIRTY_PAGES);
}

int DeviceBase::getEventFd()
{
    return this->eventFd;
}

void DeviceBase::kickIRQ()
{
    uint64_t tmp = 1;
    write(this->eventFd, &tmp, 8);
}