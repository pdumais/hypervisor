#pragma once
#include "DeviceBase.h"

class DeviceSerialPort: public DeviceBase
{
private:

public:
    DeviceSerialPort();
    ~DeviceSerialPort();
    
    virtual void handlePortWrite(unsigned int data) override;
};

