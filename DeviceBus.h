#pragma once
#include "DeviceBase.h"
#include "Memory.h"

#define MAX_DEVICES 1024
// Can't go higher than that for now because we're only mapping 512gb in the guest
#define DEVICE_ENUMERATION_BASE_ADDRESS  0xF00000000

typedef struct __attribute__ ((packed)){
    unsigned short io_base;
    uintptr_t membase;
    unsigned int id;
    unsigned int irq;
    char name[256];
} device_enumeration_entry;


class DeviceBus
{
private:
    DeviceBase* devices[MAX_DEVICES];
    device_enumeration_entry* dev_enum_mem;
    unsigned long enumeration_size;
    Memory *mem;
    int nextIRQ;

    uintptr_t getNextFreeAddress();
    int getNextFreeIRQ();

public:
    DeviceBus(Memory* m);
    ~DeviceBus();

    device_enumeration_entry* addDevice(DeviceBase* dev);
    DeviceBase* getDeviceByPort(unsigned short port);
};

