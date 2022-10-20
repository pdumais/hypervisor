#SRCS := $(wildcard *.S)
SRCS := stage1.S
OBJS := $(patsubst %.S,%.o,$(SRCS))

all: stage1

%.o: %.S
	gcc -c -m32 $< -o $@

stage1: $(OBJS)
	ld -m elf_i386 --oformat binary -N -e _start -Ttext 0x0000 -o stage1.bin $(OBJS)

clean:
	rm -f *.bin 
	rm -f *.o 
