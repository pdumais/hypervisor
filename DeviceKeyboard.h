#pragma once
#include "DeviceBase.h"
#include <rfb/rfb.h>

class DeviceKeyboard: public DeviceBase
{
private:
    rfbScreenInfoPtr server;
    int kin;
    int kout;
    uint32_t buf[64];

public:
    DeviceKeyboard(rfbScreenInfoPtr server);
    ~DeviceKeyboard();

    virtual uint32_t handlePortRead() override;
    void handlekey(rfbBool down,rfbKeySym k,rfbClientPtr cl);
};

