#include "DeviceNetCard.h"
#include <string.h>
#include <sys/types.h>                                                                                                                                                                                         
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include "log.h"


/**
 * Very simple and naive virtual netcard. 
 * The guest driver has access to a buffer of 16 MTUs. It is the driver's responsibility to track the ring buffer.
 * The driver puts a packet in a slot and signals the netcard with a io port write of the index in the buffer to signal
 * that this buffer is ready to be sent out.
 */


//TODO: Instead: each slots should be prefixed with 16 bytes. The first one indicates if buffer is ready to be transmitted
//      The netcard will clear it once transmitted to indicate that the buffer is safe to use again.
//      so the flag must be set only after data has been written, and cleared after data is sent.
//      No need for slotReady if we do that. And no need for IO port either

// TODO: implement retrieval of mac address with io port

void run_cmd(const std::string& cmd)
{
    FILE *fd = popen(cmd.c_str(), "r");
    fclose(fd);
}

DeviceNetCard::DeviceNetCard(std::string name) : DeviceBase(1, "Network Adapter", sizeof(netcard_slot)*SLOT_COUNT*2)
{
    this->cardname = name;
    this->slots = (netcard_slot*)this->mem;
    this->lastCommand = 0;
    memset(this->mem, 0 , this->memsize);

    struct ifreq ifr;
 
    std::string tunname = "/dev/"+name;
    if( (tapfd = open("/dev/net/tun", O_RDWR)) < 0 )
    {
        tapfd = open(tunname.c_str(), O_RDWR);
    }
    else
    {
        log("Creating tap interface\n");
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_flags = IFF_TAP; 
        strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ);
        int err = ioctl(tapfd,TUNSETIFF,(void*)&ifr);
        if(err < 0)
        {
            log("ioctl error\n");
            close(this->tapfd);
            this->tapfd = 0;
            return;
        }
    }

    // Set non-blocking
    fcntl(this->tapfd, F_SETFL, fcntl(this->tapfd, F_GETFL, 0) | O_NONBLOCK);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, name.c_str());
    int err = ioctl(sock,SIOCGIFHWADDR,(void*)&ifr);
    if (err == 0)
    {
        for (int i = 0; i < 6; ++i) mac[i] = ifr.ifr_addr.sa_data[i];
        log("Netcard %s has MAC %02x:%02x:%02x:%02x:%02x:%02x\n",name.c_str(), mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    }
    else
    {
        log("*** ERROR: SIOCGIFHWADDR return %i\n",err);
        for (int i = 0; i < 6; ++i) mac[i] = 0;
    }
    close(sock);

    // We use iproute2 to do this, but ideally we should be using netlink directly
    run_cmd("ip link set " + name + " master virbr0");
    run_cmd("ip link set " + name + " up");

}

DeviceNetCard::~DeviceNetCard() 
{
    if (this->tapfd) close(this->tapfd);
}

std::vector<int> DeviceNetCard::getFds() 
{
    std::vector<int> ret;
    ret.push_back(this->tapfd);

    return ret;
}


void DeviceNetCard::handlePortWrite(unsigned int data)
{
    this->lastCommand = data;
}

uint32_t DeviceNetCard::handlePortRead()
{
    union{
        uint8_t d[4];
        uint32_t raw;
    } ret;

    ret.raw = 0;
    if (this->lastCommand == 0x01)
    {
        ret.d[0] = mac[0];
        ret.d[1] = mac[1];
        ret.d[2] = mac[2];
        ret.d[3] = mac[3];
    }
    else if (this->lastCommand == 0x02)
    {
        ret.d[0] = mac[4];
        ret.d[1] = mac[5];
    }
    return ret.raw;
}

void DeviceNetCard::work()
{
    // TODO: we should instead be triggered by an eventFD and react on the reactor event instead

    for (int i=0; i<SLOT_COUNT; i++) 
    {
        if (this->slots[i].status == 1)
        {
            write(this->tapfd, this->slots[i].tapheader, this->slots[i].size+4);
            this->slots[i].status = 0;
        }
    }

}

void DeviceNetCard::handle_event(int fd)
{
    netcard_slot* mem_in = &((netcard_slot*)this->mem)[SLOT_COUNT];
    
    if (fd == this->tapfd)
    {
        // Find an free slot
        for (int i=0; i<SLOT_COUNT; i++) 
        {
            if (mem_in[i].status == 0)
            {
                char *buf = (char*)mem_in[i].tapheader;
                int r = read(this->tapfd, buf, MTU+4);
                mem_in[i].status = 1;
                mem_in[i].size = r - 4;

                this->kickIRQ();
                break;
            }

        }

    }
}
