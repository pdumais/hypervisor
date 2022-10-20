#include "common.h"


extern void halt();
extern void hypercall(unsigned int cmd);
extern int sprintf(char* dst, unsigned int max, char* str,...);
extern void print(device_enumeration_entry* serial, const char* str);
extern void outport(uint16_t port, uint32_t val);
extern uint32_t inport(uint16_t port);
extern void printf(device_enumeration_entry *serial, char* fmt, ...);
extern void ioAPICRedirect(uint32_t irq, uint32_t vector);
extern void registerIRQ(uint32_t irq, void* handler);
extern void arp_send(device_enumeration_entry* net, uint8_t addr[4]);
extern void net_get_mac(device_enumeration_entry* net, macaddress* ret);
extern void net_process_incoming(device_enumeration_entry* serial, device_enumeration_entry* net);

device_enumeration_entry *keyboard;
device_enumeration_entry *gfx;
device_enumeration_entry *net1 = 0;
device_enumeration_entry *net2 = 0;
device_enumeration_entry *serial = 0;

void net_handler()
{
    net_process_incoming(serial, net1);
}

void keyboard_handler()
{
    uint32_t k = inport(keyboard->io_base);
    if (k>='a' && k<='z') printf(serial,"guest keypress %c\n", k);
    ((char*)gfx->membase)[0] = (char)k;
}

void enumerateDevices()
{
    char buf[1024];

    device_enumeration_entry* devbus = (device_enumeration_entry*)0xF00000000;
    int n = 0;
    while (1)
    {
        if (devbus->id == 0) break;
        if (devbus->id == 1) 
        {
            if (net1) net2=devbus; else net1=devbus;
        }
        if (devbus->id == 3) serial = devbus;
        if (devbus->id == 2) gfx = devbus;
        if (devbus->id == 5) keyboard = devbus;

        int r = sprintf((char*)&buf[n], 1024, "Device: %s. io: %i, mem: %x, IRQ: %i\n", devbus->name, devbus->io_base, devbus->membase, devbus->irq);
        n+=r-1;
        devbus++;
    }
    print(serial, buf);
}


void fillGraphicScreen(device_enumeration_entry *gfx, char* p)
{
    for (int i=0;i<(800*600*4);i++) ((char*)gfx->membase)[i] = p[i];
}

void setGraphicMode(device_enumeration_entry *gfx, int graphOrText, unsigned short width, unsigned short height)
{
    unsigned int val;
    val = graphOrText?(1<<13):0x00; // true = grpahics, false=text
    val |= (0b1111 << 28);
    val |= ((width&0xFFF)<<12);
    val |= (height&0xFFF);
    outport(gfx->io_base, val);
}

void main()
{
    enumerateDevices();

    // Enable all IRQs
    for (int i=0;i<32;i++) ioAPICRedirect(i,i+32);
    printf(serial, "Starting main loop\n");
    char* p = 0;

    registerIRQ(keyboard->irq, &keyboard_handler);
    registerIRQ(net1->irq, &net_handler);

    macaddress mac;
    net_get_mac(net1, &mac);
    printf(serial,"%x:%x:%x:%x:%x:%x\n",mac.d[5],mac.d[4],mac.d[3],mac.d[2],mac.d[1],mac.d[0]);
//    setGraphicMode(gfx, 1, 1024, 768);
    setGraphicMode(gfx, 0, 80, 25);
    uint8_t ip[4] = {192,168,122,1};
    while (1) 
    {
    //printf(serial, "Meow\n");
        //p++; if (p > 1000) p =0; fillGraphicScreen(gfx, p); // Just filling bogus data in memory
        for (unsigned long i = 0; i< 0xFFFFFFFF; i++);
    arp_send(net1, ip);
    //hypercall(243);
        //for (unsigned char a=0; a<255; a++) ((char*)gfx->membase)[a] = a;
    }
}