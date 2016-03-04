# this is based on ubuntu 14.04 LST

CC := /usr/bin/g++ -std=c++11

CFLAGS := -m64 -Wall -O3 -fpermissive -ggdb -O0

INCLUDE := -Iinclude \
	$(pkg-config --cflags opencv)

LIBS := -L/usr/lib/x86_64-linux-gnu -lopencv_core -lopencv_highgui
	

OBJECTS := objects/FrameExtractor.o objects/main.o
	
all: objects/FrameExtractor.o objects/main.o
	$(CC) $(CFLAGS) -o frame-extractor $(OBJECTS) $(OPENCV_LIBS) $(LIBS)

# KANAKO

objects/main.o: src/Main.cpp
	$(CC) $(CFLAGS) -o objects/main.o -c -fpic src/Main.cpp $(INCLUDE)
	
objects/FrameExtractor.o: src/FrameExtractor.cpp include/FrameExtractor.h
	$(CC) $(CFLAGS) -o objects/FrameExtractor.o -c -fpic src/FrameExtractor.cpp $(INCLUDE)  
	
clean:
	rm objects/*.o