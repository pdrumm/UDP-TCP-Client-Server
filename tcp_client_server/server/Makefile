CXX=g++
CC=gcc

all: tcpserver

tcpserver: tcpserver.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ -lmhash

clean:
	rm -f *.o tcpserver a.out
