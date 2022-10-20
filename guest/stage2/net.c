#include "common.h"

extern void outport(uint16_t port, uint32_t val);
extern uint32_t inport(uint16_t port);
extern void printf(device_enumeration_entry *serial, char* fmt, ...);


//TODO: this is copied from HV. We should not duplicate this
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

void net_process_incoming(device_enumeration_entry* serial, device_enumeration_entry* net)
{
    netcard_slot* slots = (netcard_slot*)net->membase;
    for (int i = 0 ; i < SLOT_COUNT; i++)
    {
        netcard_slot* s = &slots[SLOT_COUNT+i];
        if (s->status == 1)
        {
            if (s->payload[12] == 0x08 && s->payload[13] == 0x06 && s->payload[21] == 0x02)
            {
                
                printf(serial, "ARP Reply %i.%i.%i.%i is at %x:%x:%x:%x:%x:%x", (unsigned int)s->payload[28], (unsigned int)s->payload[29], (unsigned int)s->payload[30], (unsigned int)s->payload[31], s->payload[22],s->payload[23],s->payload[24],s->payload[25],s->payload[26],s->payload[27]);
            }
            
            s->status = 0;
        }
    }

    //printf(serial, "net irq\n");

}

void net_send(device_enumeration_entry* net, char* data, uint16_t size, macaddress *src, macaddress *dst, uint16_t type)
{
    netcard_slot* slots = (netcard_slot*)net->membase;
    for (int i=0; i<SLOT_COUNT; i++) 
    {
        netcard_slot* s = &slots[i];
        if (s->status == 0)
        {
            s->tapheader[0] = 0;
            s->tapheader[1] = 0;
            s->tapheader[2] = type>>8;
            s->tapheader[3] = type&0xFF;
            // Build eth header
            for (int i=0;i<6;i++) s->payload[i] = dst->d[5-i];
            for (int i=0;i<6;i++) s->payload[i+6] = src->d[5-i];
            s->payload[12] = type>>8;
            s->payload[13] = type&0xFF;
            for (int n = 0; n < size; n++) s->payload[n+14] = data[n];
            s->size = size+14;
            s->status = 1;
            return;
        }
    }
}

void net_get_mac(device_enumeration_entry* net, macaddress* ret)
{
    union{
        uint8_t d[4];
        uint32_t raw;
    } data;

    outport(net->io_base, 0x01);
    data.raw = inport(net->io_base);
    ret->d[5] = data.d[0];
    ret->d[4] = data.d[1];
    ret->d[3] = data.d[2];
    ret->d[2] = data.d[3];
    outport(net->io_base, 0x02);
    data.raw = inport(net->io_base);
    ret->d[1] = data.d[0];
    ret->d[0] = data.d[1];
}

void arp_send(device_enumeration_entry* net, uint8_t addr[4])
{
    char buf[256];
    uint16_t size = 28;
    macaddress mac_src;
    macaddress mac_dst;

    buf[0] = 0x00;
    buf[1] = 0x01;
    buf[2] = 0x08;
    buf[3] = 0x00;
    buf[4] = 0x06;
    buf[5] = 0x04;
    buf[6] = 0x00;
    buf[7] = 0x01;
    for (int i=0; i<6; i++) buf[8+i] = mac_src.d[i];
    for (int i=0; i<4; i++) buf[14+i] = 1;
    for (int i=0; i<6; i++) buf[18+i] = 0xFF;
    for (int i=0; i<4; i++) buf[24+i] = addr[i];
    for (int i=0; i<6; i++) mac_dst.d[i]=0xFF;

    //TODO: It's not efficient to query this all the time
    net_get_mac(net, &mac_src);

    net_send(net, buf, size, &mac_src, &mac_dst, 0x0806);
}
