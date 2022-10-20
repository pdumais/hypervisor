SRCS := stage2.S lapic.S
SRCS_C := $(wildcard *.c)
OBJS := $(patsubst %.S,%.o,$(SRCS))
OBJS_C := $(patsubst %.c,%.o,$(SRCS_C))
CFLAGS=-fno-zero-initialized-in-bss -Wno-pointer-to-int-cast -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow -Wimplicit-function-declaration -Werror=implicit-function-declaration -fno-pie
all: stage2

%.o: %.S
	gcc -c $< -o $@
%.o: %.c
	gcc -c $(CFLAGS) $< -o $@

stage2: $(OBJS) $(OBJS_C)
	ld -m elf_x86_64 --oformat binary -N -e _start -Ttext 0x100000 -o stage2.bin $(OBJS) $(OBJS_C)

clean:
	rm -f *.bin 
	rm -f *.o 
