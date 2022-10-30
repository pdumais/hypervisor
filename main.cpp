#include "Debugger.h"
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
#include "log.h"


int main(int argc, char** argv)
{
    int kvm = open("/dev/kvm", O_RDWR | O_CLOEXEC);//open("/dev/kvm", O_RDWR);

    log("Creating VM\n");
    VirtualMachine *vm = new VirtualMachine(kvm, 1024*1024*128, 4);

    log("Loading stage1\n");
    vm->loadBinary("guest/stage1/stage1.bin", 0);
    log("Loading stage2\n");
    vm->loadBinary("guest/stage2/stage2.bin", 0x100000);
    log("Loading devices\n");
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

    log("Running\n");
    vm->run();

    Debugger dbg(vm->getMemory());
    dbg.start();

    vm->stop_vm();
    log("Terminating\n");
    vm->join();
    reactor.stop();

    close(kvm);
}
