typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
typedef union{
    uint64_t raw;
    uint8_t d[8];
} macaddress;

//TODO: make this common with the HV code?
typedef struct __attribute__ ((packed)){
    unsigned short io_base;
    void* membase;
    unsigned int id;
    unsigned int irq;
    char name[256];
} device_enumeration_entry;
