#include "DeviceSerialPort.h"
#include "log.h"


DeviceSerialPort::DeviceSerialPort(): DeviceBase(3, "Serial Port", 1)
{

}
DeviceSerialPort::~DeviceSerialPort() 
{

}

void DeviceSerialPort::handlePortWrite(unsigned int data)
{
    log("%c",(char)data);
}
