#pragma once
#include <string>
#include "DeviceBase.h"
#include "DeviceBus.h"
#include "Memory.h"
#include <vector>
#include <thread>
#include <map>
#include <mutex>
#include <condition_variable>

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
    volatile bool is_paused;
    std::vector<std::thread*> cpu_threads;
    std::mutex pauseConditionMutex;
    std::condition_variable pauseCondition;
    struct kvm_guest_debug debugInfo;
    
    void runCpu(int cpu);
    void dumpRegisters(int cpu_fd);
    void handleHypercall(unsigned int cmd, int cpu);
    void enableCap(uint32_t cap, int fd);
    void configureIRQChip();
    int registerFdForGSI(int gs, int efd);
    uint64_t translateAddress(int cpu, uint64_t addr);

    //std::map<std::thread::id,struct kvm_run*> cpuInfos;
    //std::mutex cpuRegisterLock;
    //void registerCpuInfo(struct kvm_run *run);

public:
    VirtualMachine(int kvm, unsigned long memsize, int cpu_count);
    ~VirtualMachine();

    //struct kvm_run* getCpuInfo(std::thread::id threadID);
    void loadBinary(const std::string& fname, unsigned long address);
    void initCPU();
    void initRAM(unsigned long memsize);
    void addDevice(DeviceBase* dev);
    void run();
    void join();
    void pause();
    void resume();
    void stop_vm();
    void signalVCPUs();
    Memory* getMemory();
    bool isPaused();


    struct state {
        bool valid;
        uint64_t phys_rip;
        uint64_t phys_rsp;
        uint64_t phys_rdi;
        uint64_t phys_rsi;
        struct kvm_regs regs;
    };
    VirtualMachine::state getState(int cpu);
};