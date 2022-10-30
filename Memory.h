#pragma once
#include <cstdint>
#include <linux/kvm.h>
#include <vector>

class Memory
{
private:
    int current_slot;
    uint8_t* main_mem;
    int vm_fd;
    std::vector<kvm_userspace_memory_region*> slots;

public:
    Memory(int vm_fd);
    ~Memory();

    void* createRegion(unsigned long size, uintptr_t addr);
    int createRegion(void* buf, unsigned long long size, unsigned long long addr, int flags = 0);
    bool isMemoryDirty(int slot, uint32_t size);

    void* getPointer(uint64_t addr);
    uint64_t getSize(uint64_t addr);

};

