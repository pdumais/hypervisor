#pragma once
#include <string>
#include "DeviceBase.h"
#include "DeviceBus.h"
#include "Memory.h"

class VirtualMachine {
private:
    void* ram;
    unsigned long ram_size;
    int cpu_fd;
    int vm_fd;
    int kvm_fd;
    DeviceBus *bus;
    Memory *mem;
    volatile bool stop;

    void dumpRegisters(int cpu_fd);
    void handleHypercall(unsigned int cmd);
    void enableCap(uint32_t cap, int fd);
    void configureIRQChip();
    int registerFdForGSI(int gs, int efd);
public:
    VirtualMachine(int kvm, unsigned long memsize);
    ~VirtualMachine();

    void loadBinary(const std::string& fname, unsigned long address);
    void initCPU();
    void initRAM(unsigned long memsize);
    void addDevice(DeviceBase* dev);
    void run();

};