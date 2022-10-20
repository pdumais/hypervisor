#include "DeviceVideoCard.h"

#define VC_LINE_HEIGHT 16
#define VC_CHAR_WIDTH 8

/**
 * This is a virtual video card device. It is really just a VNC server.
 * The device's memory, that gets mapped into the guest, is the framebuffer.
 * When the guest writes into the video card memory, the vnc client should see 
 * the image.
 * 
 * It's a naive implementation that is not efficient on refreshes and detecting
 * changes in modified areas. 
 * 
 * The guest can use the io port to send commands to video card to instruct it to 
 * change resolution and video mode
 * 
 * Two modes are supported: Graphic and Text. 
 * In Graphics mode, the video memory is a simple framebuffer.
 * In text mode, each bytes in memory represent an ascii character. The video card
 * will draw each characters in the VNC framebuffer. This is similar to the 0xB8000 vga buffer
 * except that there is no bytes for colours. 
 * 
 */


DeviceVideoCard::DeviceVideoCard(rfbScreenInfoPtr server): DeviceBase(2, "Video Card", 1024*768*32)
{
    this->server = server;
    this->server->frameBuffer = (char*)this->mem;
    this->textBackBuffer = 0;
    this->setMode(false, 800, 600);
}

DeviceVideoCard::~DeviceVideoCard() 
{
    if (this->textBackBuffer)
    {
        free(this->textBackBuffer);
        this->textBackBuffer = 0;
    }
}

void DeviceVideoCard::updateScreen()
{
    // This is a very naive approach where we completely update the screen everytime. 
    // Ideally, we should just update regions that changed if needed.
    rfbMarkRectAsModified(server,0,0,this->width,this->height); 

    int char_per_line = this->width/VC_CHAR_WIDTH;
    if (this->textMode)
    {
        rfbFillRect(this->server,0,0,this->width, this->height,0);
        for (int line=0; line<(this->height/VC_LINE_HEIGHT); line++)
        {
            for (int col=0; col < char_per_line; col++)
            {
                char c = ((char*)this->mem)[col+(line*char_per_line)];
                int x = col*VC_CHAR_WIDTH;
                int y =(line+1)*VC_LINE_HEIGHT;
                rfbDrawChar(this->server, &radonFont, x, y, c, 0xFFFFFF);
            }
        }
    }

}

void DeviceVideoCard::work()
{
    // We only tell the vnc server that an update is required if the memory
    // has changed. Otherwise there's no point in updating
    if (this->memDirty()) 
    {
        this->updateScreen();
    }
}

void DeviceVideoCard::setMode(bool text, int width, int height)
{
    this->textMode = text;
    this->width = width;
    this->height = height;

    if (this->textBackBuffer)
    {
        free(this->textBackBuffer);
        this->textBackBuffer = 0;
    }

    if (!text)
    {
        // graphics mode
        rfbNewFramebuffer(this->server, (char*)this->mem, this->width, this->height, 8, 3, 4);
    } 
    else
    {
        this->textBackBuffer = (char*)malloc(this->height*this->width*4);
        rfbNewFramebuffer(this->server, (char*)this->textBackBuffer, this->width, this->height, 8, 3, 4);
    }

    this->updateScreen();

}

void DeviceVideoCard::handlePortWrite(unsigned int data)
{
    int cmd = (data >>28);
    printf("VideoCard port write: %x\n", cmd);
    if (cmd == 0x0F) // set mode
    {
        int h = data & 0xFFF;
        int w = (data>>12) & 0xFFF;

        if (data & (1<<13))
        {
            this->setMode(false, w, h);
        } 
        else
        {
            // Text mode, create a new temp buffer and we'll draw text into it.
            // For text mode, width and height are givien in number of characcters
            this->setMode(true, w * VC_CHAR_WIDTH, h * VC_LINE_HEIGHT);
        }
    }
}