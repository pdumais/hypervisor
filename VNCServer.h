#pragma once
#include <rfb/rfb.h>
#include "Worker.h"
#include "DeviceVideoCard.h"
#include "DeviceKeyboard.h"

class VNCServer: public Worker
{
private:
    DeviceVideoCard* videoCard;
    DeviceKeyboard* keyboard;
    rfbScreenInfoPtr server;

public:
    VNCServer();
    ~VNCServer();

    virtual void handle_event(int fd) override;
    virtual void work() override;
    virtual std::vector<int> getFds() override;

    DeviceVideoCard* getVideoCard();
    DeviceKeyboard* getKeyboard();
    void handlekey(rfbBool down,rfbKeySym k,rfbClientPtr cl);

};

