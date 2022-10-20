//#include <errno.h>
#include "VirtualMachine.h"
#include "DeviceNetCard.h"
#include "DeviceVideoCard.h"
#include "DeviceSerialPort.h"
#include "VNCServer.h"
#include <fcntl.h>
#include <unistd.h>

#include <linux/kvm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>
#include "Reactor.h"

/*
TODO
printf should output to video console

support multiple CPUs

vfio? KVM_CREATE_DEVICE with KVM_DEV_TYPE_VFIO ?
    ->https://www.kernel.org/doc/html/v5.17/virt/kvm/devices/vfio.html


--------------------------
Running
    run as root
    ip link show tap1 should show the device is up and part of a bridge

--------------------------
Hypercalls
A "vmcall" in the guest is trapped by KVM. KVM checks the rax and handles the call. 
For example, if rax==KVM_HC_SEND_IPI, then this is interpreted as sending a IPI to another CPU.
The full list is here: https://www.infradead.org/~mchehab/kernel_docs/virt/kvm/hypercalls.html
We cannot define custom hypercalls and trap them in our HV because KVM swallows them all.
So to define the same behaviour, we can isntead reserve a port and trap VM_EXIT_IO. So for
example, we can define a "out  %eax, $0xFFFF" as the hypercall and we trigger on port 0xFFFF 
in the VM_EXIT_IO handler. We could also rely on VM_EXIT_MMIO with a reserved area in memory
And example of implementing a hypercall with VM_EXIT_MMIO:
    - Create a memory region with KVM_MEM_READONLY
    - When guest will write in that region, a VM_EXIT_MMIO will be triggered
    - Hypervisor detects that the physical address that was used by the guest is within that special region: This is a hypercall
    - Hypervisor inspects what the guest wanted to write, interprets that as the "hypercall command"

*/

int main(int argc, char** argv)
{
    int kvm = open("/dev/kvm", O_RDWR | O_CLOEXEC);//open("/dev/kvm", O_RDWR);

    printf("Creating VM\n");
    VirtualMachine *vm = new VirtualMachine(kvm, 1024*1024*128);

    printf("Loading stage1\n");
    vm->loadBinary("guest/stage1/stage1.bin", 0);
    printf("Loading stage2\n");
    vm->loadBinary("guest/stage2/stage2.bin", 0x100000);
    printf("Loading devices\n");
    VNCServer* vnc = new VNCServer();
    DeviceNetCard *d1 = new DeviceNetCard("tap1");
    DeviceNetCard *d2 = new DeviceNetCard("tap2");
    DeviceSerialPort *d4 = new DeviceSerialPort();
    vm->addDevice(d1);
    vm->addDevice(d2);
    vm->addDevice(vnc->getVideoCard());
    vm->addDevice(vnc->getKeyboard());
    vm->addDevice(d4);

    Reactor reactor;
    reactor.addWorker(d1);
    reactor.addWorker(d2);
    reactor.addWorker(vnc);
    reactor.start();

    printf("Running\n");
    vm->run();

    reactor.stop();

    close(kvm);
}
