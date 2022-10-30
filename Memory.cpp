#include "Memory.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "log.h"


Memory::Memory(int vm_fd) 
{
    this->current_slot = 0;
    this->vm_fd = vm_fd;
}

Memory::~Memory() 
{
    //TODO: delete all kvm_userspace_memory_region
}

void* Memory::createRegion(unsigned long size, uintptr_t addr)
{
    void *buf = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (buf == MAP_FAILED)
    {
        log("mmap failed\n");
        return 0;
    }
    this->createRegion(buf, size, addr);

    return buf;
}

int Memory::createRegion(void* buf, unsigned long long size, unsigned long long addr, int flags/* = 0*/)
{
    log("Adding memory region %i, phys=%016llx, size=%016llx\n", this->current_slot, addr, size);
    kvm_userspace_memory_region *region = new kvm_userspace_memory_region();
	region->slot = this->current_slot;
    region->flags = flags;
	region->guest_phys_addr = addr;
	region->memory_size = size;
	region->userspace_addr = (unsigned long long)buf;
    this->slots.push_back(region);
    ioctl(this->vm_fd, KVM_SET_USER_MEMORY_REGION, region);
    this->current_slot++;

    return region->slot;
}

bool Memory::isMemoryDirty(int slot, uint32_t size)
{
    bool ret = false;

    int bitmap_size = size >> 3;
    kvm_dirty_log log;
    memset(&log, 0, sizeof(kvm_dirty_log));
    log.dirty_bitmap = malloc(bitmap_size);
    memset(log.dirty_bitmap, 0, bitmap_size);
    log.slot = slot;
    if (ioctl(this->vm_fd, KVM_GET_DIRTY_LOG, &log) >= 0)
    {
        for (uint32_t i = 0; i < bitmap_size>>3; i++) // divide by 8 again because we check 64bit values
        {
            uint64_t bitmap = ((uint64_t*)log.dirty_bitmap)[i];
            ret = (bitmap != 0);
            if (ret) break;
        }
    }
    else 
    {
        //log("*** Error checking dirty log %i \n", slot);
        ret = true;
    }
    free(log.dirty_bitmap);

    return ret;
}

void* Memory::getPointer(uint64_t addr)
{
    for (kvm_userspace_memory_region* r : this->slots)
    {
        uint64_t start = r->guest_phys_addr;
        uint64_t end = start + r->memory_size -1;

        if (addr >= start && addr <= end)
        {
            return (void*)(r->userspace_addr + (addr- r->guest_phys_addr));
        }
    }
}

uint64_t Memory::getSize(uint64_t addr)
{
    for (kvm_userspace_memory_region* r : this->slots)
    {
        uint64_t start = r->guest_phys_addr;
        uint64_t end = start + r->memory_size -1;

        if (addr >= start && addr <= end)
        {
            return r->memory_size;
        }
    }
}
