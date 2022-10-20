#pragma once
#include "DeviceBase.h"
#include <rfb/rfb.h>
#include "radon.h"

class DeviceVideoCard: public DeviceBase
{
private:
    rfbScreenInfoPtr server;
    volatile bool running;
    char* textBackBuffer;
    bool textMode;
    int width;
    int height;

    void run();
    void updateScreen();
    void setMode(bool text, int width, int height);

public:
    DeviceVideoCard(rfbScreenInfoPtr server);
    ~DeviceVideoCard();

    void work();
    virtual void handlePortWrite(unsigned int data) override;
};

