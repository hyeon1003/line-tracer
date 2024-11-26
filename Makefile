CX = g++   
CXFLAGS = -g -Wall -I/usr/local/include/dynamixel_sdk `pkg-config opencv4 --cflags`
LDFLAGS = -ldxl_x64_cpp `pkg-config opencv4 --libs`              
TARGET = line
OBJS = main.o dxl.o vision.o

$(TARGET) : $(OBJS)
	$(CX) -o $(TARGET) $(OBJS) $(LDFLAGS)
main.o : main.cpp vision.hpp
	$(CX) $(CXFLAGS) -c main.cpp vision.hpp
dxl.o : dxl.hpp dxl.cpp
	$(CX) $(CXFLAGS) -c dxl.cpp
vision.o : vision.hpp vision.cpp
	$(CX) $(CXFLAGS) -c vision.cpp
.PHONY: clean 

clean:
	rm -rf $(TARGET) $(OBJS)
