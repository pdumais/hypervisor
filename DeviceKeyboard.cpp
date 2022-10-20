#include "DeviceKeyboard.h"


DeviceKeyboard::DeviceKeyboard(rfbScreenInfoPtr server) : DeviceBase(5, "Keyboard", 32)
{
    this->server = server;
    this->kin = 0;
    this->kout = 0;
}

DeviceKeyboard::~DeviceKeyboard()
{

}

uint32_t DeviceKeyboard::handlePortRead()
{
    uint32_t ret = this->buf[this->kout];
    this->kout = (this->kout+1)&0b111111;
    return ret;
}

void DeviceKeyboard::handlekey(rfbBool down,rfbKeySym k,rfbClientPtr cl)
{
    //TODO: must convert to proper scan codes. This is just passing chars based on VNC
    if (down) return;
    this->buf[this->kin] = (uint32_t)k;
    this->kin = (this->kin+1)&0b111111;

    this->kickIRQ();
}