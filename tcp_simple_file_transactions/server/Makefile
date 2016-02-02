CXX=g++
CC=gcc

all: myftpd

myftpd: tcpserver.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ -lmhash

clean:
	rm -f *.o myftpd a.out
