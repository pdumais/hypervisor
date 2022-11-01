#include "VirtualMachine.h"
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <linux/kvm.h>
#include <stdio.h>
#include <csignal>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#include <string>
#include <stdlib.h>
#include <thread>
#include "log.h"

//std::mutex threadVMsLock;
//std::map<std::thread::id,VirtualMachine*> threadVMs;

void sighandler(int r)
{
    /*
    std::thread::id tid = std::this_thread::get_id();
    threadVMsLock.lock();
    if (!threadVMs.count(tid))
    {
        threadVMsLock.unlock();
        return;
    }
    VirtualMachine* vm = threadVMs[tid];
    threadVMsLock.unlock();
    struct kvm_run *cpu = vm->getCpuInfo(tid);
    cpu->immediate_exit = 1; //TODO: do we need to make this thread safe?*/
    log("signal %i \n", r);
}

VirtualMachine::VirtualMachine(int kvm, unsigned long memsize, int cpu_count)
{
    int ret;
    this->kvm_fd = kvm;
    for (int i = 0; i < MAX_CPU; i++) this->cpu_fd[i] = 0;
    this->cpu_count = cpu_count;

    this->vm_fd = ioctl(this->kvm_fd, KVM_CREATE_VM, 0);

    // Apparently this is required. Not sure why
    ioctl(this->vm_fd, KVM_SET_TSS_ADDR, 0xfffbd000);

    /*
    Looking at the kvm source code, KVM_CAP_CHECK_EXTENSION_VM will always return 1 for these caps
    and will always return -1 when trying to enable the caps. So it might just be that they don't need
    to be ebabled
    this->enableCap(KVM_CAP_IRQCHIP,this->vm_fd);
    this->enableCap(KVM_CAP_IRQFD,this->vm_fd);
    this->enableCap(KVM_CAP_IRQ_ROUTING,this->vm_fd);
    */

    this->enableCap(KVM_CAP_X86_USER_SPACE_MSR, this->vm_fd);
    this->configureIRQChip(); 
    kvm_pit_config pit;
    pit.flags = 0;
    ioctl(this->vm_fd, KVM_CREATE_PIT2, &pit);
    this->mem = new Memory(this->vm_fd);
    this->initCPU();
    this->initRAM(memsize);
    this->bus = new DeviceBus(this->mem);
}

void VirtualMachine::stop_vm()
{
    this->stop = true;
    this->is_paused = false;
    this->pauseCondition.notify_all();
    this->signalVCPUs();
}

void VirtualMachine::enableCap(uint32_t c, int fd)
{   
    if (ioctl(fd, KVM_CAP_CHECK_EXTENSION_VM, c) == 0)
    {
        log("*** ERROR: Capability %i is unsupported\n",c);
        return;
    }

    kvm_enable_cap cap;
    memset(&cap, 0, sizeof(kvm_enable_cap));
    cap.cap = c;
    cap.flags = 0;
    if (ioctl(fd, KVM_ENABLE_CAP, &cap) != 0)
    {
        log("*** ERROR: Can't set capability %i\n", c);
    }
}

void VirtualMachine::initRAM(unsigned long memsize)
{
    // Create RAM
    this->ram_size = memsize;
    this->ram = mem->createRegion(memsize, 0);
}

VirtualMachine::~VirtualMachine()
{
    log("destructor\n");
    //TODO: destroy ram buffer
    delete this->bus;
    delete this->mem;
    for (int i = 0; i < this->cpu_count; i++) close(this->cpu_fd[i]);
    close(this->vm_fd);
}

void VirtualMachine::initCPU()
{
    int ret;
    
    struct kvm_cpuid2 *cpuid;
    int nent = 100;
    unsigned long size = sizeof(*cpuid) + (nent * sizeof(*cpuid->entries));
    cpuid = (struct kvm_cpuid2*) malloc(size);
    bzero(cpuid, size);
    cpuid->nent = nent;
    ret = ioctl(this->kvm_fd, KVM_GET_SUPPORTED_CPUID, cpuid);


    for (int i = 0; i < this->cpu_count; i++)
    {
        this->cpu_fd[i] = ioctl(this->vm_fd, KVM_CREATE_VCPU, i);
        ioctl(this->cpu_fd[i], KVM_SET_CPUID2, cpuid);

        // init registers
        struct kvm_sregs sregs;
        struct kvm_regs regs;

        ioctl(this->cpu_fd[i], KVM_GET_SREGS, &sregs);
        sregs.cs.selector = 0;
        sregs.cs.base = 0;
        sregs.ss.selector = 0;
        sregs.ss.base = 0;
        sregs.ds.selector = 0;
        sregs.ds.base = 0;
        sregs.es.selector = 0;
        sregs.es.base = 0;
        sregs.fs.selector = 0;
        sregs.fs.base = 0;
        sregs.gs.selector = 0;

        regs.rflags = 0b10;
        regs.rip = 0;  // Instruction pointer will initially point to 0, so code must be placed there
        ioctl(this->cpu_fd[i], KVM_SET_REGS, &regs);
        ioctl(this->cpu_fd[i], KVM_SET_SREGS, &sregs);
    }
    free(cpuid);

    /*this->debugInfo.arch.debugreg[0] = 0x1A;
    this->debugInfo.arch.debugreg[7] = 0xF0;
    this->debugInfo.control = KVM_GUESTDBG_USE_SW_BP;
    ioctl(this->cpu_fd[0], KVM_SET_GUEST_DEBUG, &(this->debugInfo));*/

}

void VirtualMachine::loadBinary(const std::string& fname, unsigned long address)
{
    char* mem = (char*)this->ram;

    mem += address;
    int fd = open(fname.c_str(), O_RDONLY);
    int r = 1;
    while (r >0)
    {
        r = read(fd, mem, 1024);
        if (r > 0) mem += r;
    }
    close(fd);
}

void VirtualMachine::configureIRQChip()
{
    if (ioctl(this->vm_fd, KVM_CREATE_IRQCHIP, 0) == -1)
    {
        log("*** ERROR: Could not create IRQ Chip\n");
    }

    kvm_irqchip chip;
    memset(&chip,0,sizeof(kvm_irqchip));
    chip.chip_id = 2; // IOAPIC
    if (ioctl(this->vm_fd, KVM_GET_IRQCHIP, &chip) == -1)
    {
        log("*** ERROR: Could not get IRQ Chip\n");
    }

    if (chip.chip.ioapic.base_address != 0xfec00000) 
    {
        //TODO: We hardcode the guest to use that location for the ioapic. 
        //      we should instead write the adress somewhere and get the guest to look for it
        log("*** ERROR: ioapic is not at expected base address\n");
    }

    log("IOAPIC base address = %016llx, id= %i\n",chip.chip.ioapic.base_address, chip.chip.ioapic.id);

    // Now we'll setup IRQs 1-24. We tell KVM that we want them to be routed on the ioapic
    int nr = 24;
    int s = sizeof(kvm_irq_routing)+(nr*sizeof(kvm_irq_routing_entry));
    kvm_irq_routing *r = (kvm_irq_routing*)malloc(s);
    memset(r,0,s);
    r->nr = nr;
    r->flags = 0;
    for (int i = 0; i < nr; i++)
    {
        r->entries[i].type = KVM_IRQ_ROUTING_IRQCHIP;
        r->entries[i].gsi = i; // for IRQCHIP, we can't go higher than 24
        r->entries[i].flags = 0;
        r->entries[i].u.irqchip.irqchip = 2; /* 0 = PIC1, 1 = PIC2, 2 = IOAPIC */
        r->entries[i].u.irqchip.pin = i;
    }
    if (ioctl(this->vm_fd, KVM_SET_GSI_ROUTING, r) < 0 )
    {
        log("*** ERROR: Can't set GSI routing\n");
        return;
    }
    free(r);
}

int VirtualMachine::registerFdForGSI(int gsi, int efd)
{
    kvm_irqfd irqfd;
    memset(&irqfd, 0, sizeof(kvm_irqfd));
    irqfd.gsi = gsi;
    irqfd.fd = efd; 
    if (ioctl(this->vm_fd, KVM_IRQFD, &irqfd) < 0 )
    {
        log("*** ERROR: Can't register eventfd\n");
        return -1;
    }

    return efd;
}

void VirtualMachine::run()
{
    this->stop = false;
    this->is_paused = false;
    for (int i = 0; i < this->cpu_count; i++)
    {
        std::thread *th = new std::thread([=]() {
            this->runCpu(i);
        });
        this->cpu_threads.push_back(th);
    }
}

void VirtualMachine::signalVCPUs()
{
    for (std::thread* th : this->cpu_threads)
    {
        pthread_kill(th->native_handle(), SIGUSR1);
    }    
}

void VirtualMachine::join()
{
    for (auto& th : this->cpu_threads)
    {
        th->join();
        delete th;
    }
    this->cpu_threads = std::vector<std::thread*>();

}

/*struct kvm_run* VirtualMachine::getCpuInfo(std::thread::id threadID)
{
    struct kvm_run* ret = 0;
    this->cpuRegisterLock.lock();
    if (this->cpuInfos.count(threadID)) 
    {
        ret =  this->cpuInfos[threadID];
    }
    this->cpuRegisterLock.unlock();
    return ret;
}

void VirtualMachine::registerCpuInfo(struct kvm_run *run)
{
    std::thread::id tid = std::this_thread::get_id();
    this->cpuRegisterLock.lock();
    threadVMsLock.lock();
    threadVMs[tid] = this;
    threadVMsLock.unlock();
    this->cpuInfos[tid] = run;
    this->cpuRegisterLock.unlock();
}*/


void VirtualMachine::pause()
{
    this->is_paused = true;
    this->signalVCPUs();
}

void VirtualMachine::resume()
{
    this->is_paused = false;
    this->pauseCondition.notify_all();
}

void VirtualMachine::runCpu(int cpuIndex)
{
    std::signal(SIGUSR1, &sighandler);

    int cpu = this->cpu_fd[cpuIndex];
    this->dumpRegisters(cpu);

    int mmap_size = ioctl(this->kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    log("mmap_size = %i\n", mmap_size);
    struct kvm_run *run = (struct kvm_run *)mmap(0, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, cpu, 0);
    //this->registerCpuInfo(run);
    log("kvm_run = 0x%08x, errno=%i, cpu_fd=%i\n", run, errno, cpu);
    unsigned char seq = 0;

    while (!this->stop)
    {

        if (this->is_paused)
        {
            log("pause\n");
            auto lock = std::unique_lock<std::mutex>(this->pauseConditionMutex);
            this->pauseCondition.wait(lock, [=](){ return !this->is_paused; });
            log("pause ended\n");
            continue;
        }

        // The main loop runs the VM and blocks until a vmexit occurs. The IOCTL to KVM_RUN is basically the 'VMRESUME' instruction
        // After the call returns, we need to check for the vmexit reason and react. Then we can resume again.
        // Needless to say that each vmexit is expensive so we want to avoid it as much as possible. We don't really have
        // control over that since it's the guest that causes those exits, but it is the hypervisor that sets the condictions to exit
        // about mmio for example.
        int err = ioctl(cpu, KVM_RUN, 0);
        if (err == -1)
        {
            if (errno == EINTR)
            {
                // This happens if we've kicked the CPU with signalVCPUs().
                // The goal was to kick the CPU out of KVM_RUN so that we can evaluate conditions such as pause or stop
                log("EINTR\n");
            }
            continue;
        }

        int reason = run->exit_reason;
        switch (reason) {
            case KVM_EXIT_MMIO:
                {
                    log("*** KVM_EXIT_MMIO %016llx %08x %i\n", run->mmio.phys_addr, run->mmio.len, run->mmio.is_write);
                    this->dumpRegisters(cpu);
                }
                break;
            case KVM_EXIT_X86_WRMSR:
                {
                    log("*** KVM_EXIT_X86_WRMSR %i %i %08x %016llx\n", run->msr.error, run->msr.reason, run->msr.index, run->msr.data);
                }
                break;
            case KVM_EXIT_SHUTDOWN:
                {
                    log("*** KVM_EXIT_SHUTDOWN (Triple fault) %i %i %08x %016llx\n", run->msr.error, run->msr.reason, run->msr.index, run->msr.data);
                    this->dumpRegisters(cpu);
                    this->stop = true;
                    continue;
                }
                break;
            case KVM_EXIT_HLT:
                {
                    // This will never be executed. We've enabled IRQCHIP, so KVM will handle IRQs and also HLT
                    // since it needs to wakeup the VM on IRQs. So this gets done automatically too.
                    log("** KVM_EXIT_HLT\n");
                }
                break;
            case KVM_EXIT_IO:
                {
                    char* data = (((char *)run) + run->io.data_offset);
            	    if (run->io.direction == KVM_EXIT_IO_IN) 
                    {
                        if (run->io.port == 0xFFFE)
                        {
                            // this is a special input to get the index of the current CPU.
                            // On a real machine, this is done by reading from the LAPIC
                            ((unsigned int*)data)[0] = cpuIndex;
                        }
                        else
                        {
                            DeviceBase *dev = this->bus->getDeviceByPort(run->io.port);
                            if (dev)
                            {
                                ((unsigned int*)data)[0] = dev->handlePortRead();
                            }
                            else
                            {
                                log("io in %04x = %04x\n", run->io.port, ((unsigned int*)data)[0]);
                            }
                        }

                    } 
                    else if (run->io.direction == KVM_EXIT_IO_OUT) 
                    {
                        if (run->io.port == 0xFFFF)
                        {
                            this->handleHypercall(((unsigned int*)data)[0], cpu);
                        }
                        else 
                        {
                            DeviceBase *dev = this->bus->getDeviceByPort(run->io.port);
                            if (dev)
                            {
                                dev->handlePortWrite(((unsigned int*)data)[0]);
                            }
                            else
                            {
                                this->dumpRegisters(cpu);
                                log("io out %04x = %04x\n", run->io.port, ((unsigned int*)data)[0]);
                            }
                        }
                    }
                }
	            break;
            case KVM_EXIT_UNKNOWN:
                {
                    log("** VM EXIT Unknown reason\n");
                    this->dumpRegisters(cpu);
                }
                break;
            /*case KVM_EXIT_DEBUG:
                {
                    printf("test\n");
                    return;
                }
                break;*/
            default:
                {
                    log("** Unhandled VM EXIT %i\n", reason);
                    this->dumpRegisters(cpu);
                    this->stop = true;
                    continue;
                }
	    } 
    }
    log("CPU %i exited\n", cpu);
}

bool VirtualMachine::isPaused()
{
    return this->is_paused;
}

void VirtualMachine::handleHypercall(unsigned int cmd, int cpu)
{
    log("*** Hypercall(%i): cmd=%08x\n", cpu, cmd);
    sleep(1);
}

void VirtualMachine::dumpRegisters(int cpu_fd)
{
    struct kvm_sregs sregs;
    struct kvm_regs regs;
    ioctl(cpu_fd, KVM_GET_SREGS, &sregs);
    ioctl(cpu_fd, KVM_GET_REGS, &regs);
    log("  efer: %016llx lapic_base: %016llx\n",sregs.efer, sregs.apic_base);
    log("  cr0: %016llx cr2: %016llx cr3: %016llx cr4: %016llx cr8: %016llx\n",sregs.cr0,sregs.cr2,sregs.cr3,sregs.cr4,sregs.cr8);
    log("  rax: %016llx rbx: %016llx rcx: %016llx rdx: %016llx rdi: %016llx rsi: %016llx\n",regs.rax,regs.rbx,regs.rcx,regs.rdx,regs.rdi,regs.rsi);
    log("  rsp: %016llx rip: %016llx flags: %016llx\n",regs.rsp,regs.rip, regs.rflags);
}

void VirtualMachine::addDevice(DeviceBase* dev)
{
    device_enumeration_entry* e = this->bus->addDevice(dev);
    this->registerFdForGSI(e->irq, dev->getEventFd());
}

Memory* VirtualMachine::getMemory()
{
    return this->mem;
}


uint64_t VirtualMachine::translateAddress(int cpu, uint64_t addr)
{
    kvm_translation tr;
    tr.linear_address = addr;
    ioctl(this->cpu_fd[cpu], KVM_TRANSLATE, &tr);
    return tr.physical_address;
}

VirtualMachine::state VirtualMachine::getState(int cpu)
{
    VirtualMachine::state s;

    s.valid = false;
    if (cpu >= MAX_CPU || this->cpu_fd[cpu] == 0) return s;

    ioctl(this->cpu_fd[cpu], KVM_GET_REGS, &(s.regs));
    s.phys_rdi = this->translateAddress(cpu, s.regs.rdi);
    s.phys_rsi = this->translateAddress(cpu, s.regs.rsi);
    s.phys_rip = this->translateAddress(cpu, s.regs.rip);
    s.phys_rsp = this->translateAddress(cpu, s.regs.rsp);

    s.valid = true;
    return s;
}
