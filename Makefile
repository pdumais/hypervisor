SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))
CC := g++ -std=c++17
CFLAGS := -g 
LDFLAGS := -lvncserver -lncurses -ludis86 -L/usr/local/lib

all: vm main




main: $(OBJS)
	g++ $(OBJS) ${LDFLAGS}

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<
vm:
	make -C guest

makeclass:
	echo "#pragma once\n\nclass ${name}\n{\nprivate:\n\npublic:\n    ${name}();\n    ~${name}();\n};\n" > ${name}.h
	echo "#include \"${name}.h\"\n\n${name}::${name}() {}\n${name}::~${name}() {}\n" > ${name}.cpp
	
clean:
	rm -f *.o
	cd guest && make clean