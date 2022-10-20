#pragma once
#include "DeviceBase.h"
#include "Worker.h"

#define MTU 1500
#define SLOT_COUNT 16

typedef struct __attribute__ ((packed)){
    uint8_t status;
    uint8_t pad1;
    uint16_t size;
    uint8_t tapheader[4];
    uint8_t payload[MTU];
    uint8_t pad2[MTU % 4];
} netcard_slot;

class DeviceNetCard: public DeviceBase, public Worker
{
private:
    uint32_t lastCommand;
    netcard_slot *slots;
    std::string cardname;
    int tapfd;
    uint8_t mac[6];

public:
    DeviceNetCard(std::string name);
    ~DeviceNetCard();

    virtual void handle_event(int fd) override;
    virtual void work() override;
    virtual void handlePortWrite(unsigned int data) override;
    virtual uint32_t handlePortRead() override;
    virtual std::vector<int> getFds() override;

};

