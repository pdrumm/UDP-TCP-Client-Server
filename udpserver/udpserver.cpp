/*	udpserver.cpp
 *	Author:	Patrick Drumm
 *	NetID:	pdrumm
 *	Class:	CSE 30264
 *
 *	Goal: Write a simple UDP server program that...
 *		- accepts one command line argument -> port number
 *			- port assigned to pdrumm: 41007
 *		- listens for connection on given port and receives messages
 *		- appends accurate timestamp to message
 *		- encrypts the timestamped message
 *		- sends the encrypted message back to client
 *		- continue waiting for next client
 *
 *	Error Checking:
 *		The program will not generate any console messages unless a "fatal" error occurs. After such, the code will exit with an error code
 */

#include <iostream>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <unistd.h>
#include <sys/time.h>
#include <iomanip>
#include <sstream>
using namespace std;
#define MAX_SIZE 5000

// function prototypes
void stamp_time(char *);
void encode(char*);
void swap(char*, char*);
string int_to_str(int, int);

int main(int argc, char** argv) {
	struct sockaddr_in	sin,		// the socket to which the server is binding to the port number
				client_addr;	// empty struct, filled in with client info at recvfrom()
	char buf[MAX_SIZE];
	int len, addr_len;
	int s;
	int SERVER_PORT;

	// check arg count
	if( argc != 2 ){
		cerr << "Exactly one argument (port number) necessary to run udpserver" << endl;
		cerr << "	eg. ./udpserver 41500" << endl;
		exit(1);
	}

	SERVER_PORT = atoi(argv[1]);

	// build address data structure
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);
//	cout << "Welcome to Simple UDP Server." << endl;

	// setup passive open
	if( (s=socket(PF_INET, SOCK_DGRAM, 0)) < 0){
		cerr << "simplex-talk: socket" << endl;
		exit(1);
	}

	if((bind(s,(struct sockaddr *)&sin, sizeof(sin))) < 0){
		cerr << "simplex-talk: bind" << endl;
		exit(1); 
	}

	addr_len = sizeof(client_addr);

	while(1){
		// clear buffer and continue waiting for client communication
		bzero((char*)&buf,sizeof(buf));

		if( len = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, (socklen_t*)&addr_len) == -1){
			cerr << "Receive error!" << endl;
			exit(1);
		}
//		cout << "Server Received: " << endl << buf << endl;

		// add timestamp
		stamp_time(buf);

		// encode message
		encode(buf);

		// send message back to client
		if( sendto( s, buf, strlen(buf), 0, (struct sockaddr*)&client_addr, sizeof(client_addr) ) == -1 ){
			cerr << "Server Send Error" << endl;
			exit(1);
		}
	}
	close(s);
}
// END MAIN

void stamp_time(char* buf) {
	// retrieve current time
	struct timeval currTime;
	gettimeofday( &currTime, NULL );
	// calculate in terms of hr,min,sec of day
	int hr,min,sec,usec,to_eastern=4*60*60,tempTime;
	tempTime=currTime.tv_sec-to_eastern;
	sec=tempTime%60;
	tempTime/=60;	// integer division will convert time to minutes and then drop the remaining fraction
	min=tempTime%60;
	tempTime/=60;	// integer division will convert time to hours and then drop the remaining fraction
	hr=tempTime%24;
	// construct timestamp string to be appended
	string add_time =	" Timestamp: "+
				int_to_str(hr,2)+":"+
				int_to_str(min,2)+":"+
				int_to_str(sec,2)+"."+
				int_to_str(currTime.tv_usec,6)+"\n"; 
	// add timestamp
	strcat(buf,add_time.c_str());
}

void encode(char* encryption) {
	int i = 0;
	for( int j=strlen(encryption)-1; j >= i; j-=2, i+=2) {
		swap(encryption[i], encryption[j]);
	}
}

void swap(char a, char b) {
	char temp = a;
	a = b;
	b = temp;
}

string int_to_str(int time, int width){
	ostringstream convert;
	convert << setw(width) << setfill('0') << time;
	return convert.str();
}
