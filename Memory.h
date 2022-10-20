#pragma once
#include <cstdint>
#include <linux/kvm.h>

class Memory
{
private:
    int current_slot;
    int vm_fd;
public:
    Memory(int vm_fd);
    ~Memory();

    void* createRegion(unsigned long size, uintptr_t addr);
    int createRegion(void* buf, unsigned long long size, unsigned long long addr, int flags = 0);
    bool isMemoryDirty(int slot, uint32_t size);

};

