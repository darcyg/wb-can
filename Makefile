SOURCES=main.cpp can.cpp hapcan.cpp
OBJECTS=$(SOURCES:.cpp=.o)

all: $(OBJECTS)
	g++ $(OBJECTS) -L/usr/lib -lmosquitto -o wb-can

$(OBJECTS): %.o: %.c
	g++ -c $< -o $@ -std=c++0x

.PHONY: clean

clean: 
	rm -f *.o wb-can

ifup: 
	modprobe vcan
	ip link add dev can0 type vcan
	ifconfig can0 up
