#pragma once
#include <string>
#include "DeviceBase.h"
#include "DeviceBus.h"
#include "Memory.h"
#include <vector>
#include <thread>

#define MAX_CPU 64

class VirtualMachine {
private:
    void* ram;
    unsigned long ram_size;
    int cpu_fd[MAX_CPU];
    int vm_fd;
    int kvm_fd;
    DeviceBus *bus;
    Memory *mem;
    int cpu_count;
    volatile bool stop;
    std::vector<std::thread*> cpu_threads;

    void runCpu(int cpu);
    void dumpRegisters(int cpu_fd);
    void handleHypercall(unsigned int cmd, int cpu);
    void enableCap(uint32_t cap, int fd);
    void configureIRQChip();
    int registerFdForGSI(int gs, int efd);
public:
    VirtualMachine(int kvm, unsigned long memsize, int cpu_count);
    ~VirtualMachine();

    void loadBinary(const std::string& fname, unsigned long address);
    void initCPU();
    void initRAM(unsigned long memsize);
    void addDevice(DeviceBase* dev);
    void run();
    void join();
    void stop_vm();
    Memory* getMemory();

};