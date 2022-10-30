#include "DeviceBus.h"
#include <string.h>
#include "log.h"

DeviceBus::DeviceBus(Memory* mem) 
{
    this->mem = mem;
    this->nextIRQ = 0;
    this->enumeration_size = ((sizeof(device_enumeration_entry)*MAX_DEVICES)+0xFFF)&0xFFFFF000; // round up to next page
    for (int i=0; i<MAX_DEVICES; i++) this->devices[i] = 0;

    // Device enumeration area
    this->dev_enum_mem = (device_enumeration_entry*)mem->createRegion(this->enumeration_size, DEVICE_ENUMERATION_BASE_ADDRESS);

}
DeviceBus::~DeviceBus() 
{
    for (int i =0; i < MAX_DEVICES; i++)
    {
        if (this->devices[i] != 0)
        {
            delete this->devices[i];
        }
    }

}

int DeviceBus::getNextFreeIRQ()
{
    //TODO: this should not exceed 24
    this->nextIRQ++;
    return this->nextIRQ;
}

uintptr_t DeviceBus::getNextFreeAddress()
{
    uintptr_t membase = this->enumeration_size + DEVICE_ENUMERATION_BASE_ADDRESS;
    for (int i =0; i < MAX_DEVICES; i++)
    {
        if (this->devices[i] != 0)
        {
            membase += this->devices[i]->getMemorySize();
        }
    }

    return membase;
}

device_enumeration_entry* DeviceBus::addDevice(DeviceBase* dev)
{
    for (int i =0; i < MAX_DEVICES; i++)
    {
        if (this->devices[i] == 0)
        {
            uintptr_t addr = this->getNextFreeAddress();
            this->devices[i] = dev;
            device_enumeration_entry* e = &((device_enumeration_entry*)this->dev_enum_mem)[i];
            e->membase = addr;
            e->io_base = 100+i;
            e->irq = this->getNextFreeIRQ();
            e->id = dev->getId();
            dev->register_memory(this->mem, e->membase);
            strcpy(e->name, dev->getName().c_str());
            log("Adding device %s, io=%04x, mem=%016lx\n", e->name, e->io_base, e->membase);
            return e;
        }
    }
}

DeviceBase* DeviceBus::getDeviceByPort(unsigned short port)
{
    unsigned int p = port-100;
    if (p >= MAX_DEVICES) return 0;
    return this->devices[p];
}