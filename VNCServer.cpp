#include "VNCServer.h"


static void dokey(rfbBool down,rfbKeySym k,rfbClientPtr cl)
{
    VNCServer* s = (VNCServer*)cl->screen->screenData;
    s->handlekey(down, k, cl);
}

VNCServer::VNCServer() 
{
    printf("VNC server init\n");
    this->server = rfbGetScreen(0,0,800,600, 8,3,4);
    this->videoCard = new DeviceVideoCard(this->server);
    this->server->port = 5912;
    this->server->screenData = this;
    this->server->kbdAddEvent=dokey;
    rfbInitServer(server);

    this->keyboard = new DeviceKeyboard(this->server);

}

VNCServer::~VNCServer() 
{
    rfbShutdownServer(this->server, true);
    delete this->videoCard;
    delete this->keyboard;
}

void VNCServer::work()
{
    this->videoCard->work();
    rfbProcessEvents(this->server, -1);
}

DeviceVideoCard* VNCServer::getVideoCard()
{
    return this->videoCard;
}

DeviceKeyboard* VNCServer::getKeyboard()
{
    return this->keyboard;
}

void VNCServer::handlekey(rfbBool down,rfbKeySym k,rfbClientPtr cl)
{
    this->keyboard->handlekey(down, k, cl);
}

void VNCServer::handle_event(int fd)
{
}

std::vector<int> VNCServer::getFds()
{
    return std::vector<int>();
}
