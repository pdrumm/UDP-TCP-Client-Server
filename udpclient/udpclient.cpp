/*	udpclient.cpp
 *	Author:	Patrick Drumm
 *	NetID:	pdrumm
 *	Class: CSE 30264
 *
 *	Goal: Write a simple UDP client program that...
 *		- takes in 3 command line inputs
 *			1. host name of server
 *			2. port number on server
 *			3. the text to send (could be a string or file name)
 *		- sends the string/file to the server
 *		- receives an inverted copy of the file (with a time-stamp) back from the server
 *		- computes the RTT (round-trip-time) of the messages
 *
 * 	Example command line argument:
 * 		./udpclient student03.cse.nd.edu 41500 "Random string"
 * 	Output contains:
 * 		- the decrypted message
 * 		- the round trip time
 *
 * 	Run client on:
 * 		student00.cse.nd.edu
 * 		student01.cse.nd.edu
 *	Test on following servers:
 *		student02.cse.nd.edu:41500
 *		student03.cse.nd.edu:41500
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sys/time.h>
#include <unistd.h>
using namespace std;

//function prototypes
int isFile(const char *);
string writeBuffer(const char *);
string decode(string);
void swap(char*, char*);

int main(int argc, char**argv) {
	const int ARG_COUNT = 4;	// # of arguments that is expected
	const char* HOST_NAME;		// = argv[1]
	int PORT_NUM;			// = argv[2]
	const char* BUFFER_INPUT;	// = argv[3] - file/sting input being sent
	int sockfd, n, timestamp_size=27;
	struct sockaddr_in servaddr;
	struct hostent *hp;
	char recvline[5000];
	string recvdecoded;
	struct timeval startTime, endTime;

	//if there are not the correct number of arguments, tell user and quit
	if (argc != ARG_COUNT) {
		cerr << "Error. This UDP client program takes exactly three arguments as input." << endl;
		cerr << "eg.\n  ./udpclient student03.cse.nd.edu 41500 randomstring"<< endl;
		exit(1);
	}

	HOST_NAME 	= argv[1];
	PORT_NUM 	= atoi( argv[2] );
	BUFFER_INPUT 	= argv[3];
	// writeBuffer returns BUFFER_INPUT(or it's file contents) as a string 
	string buffer_str = writeBuffer( BUFFER_INPUT );

	// translate host name to IP address
	hp = gethostbyname(HOST_NAME);
	if (!hp) {
		cerr << "Error. Unknown host." << endl;
		exit(1);
	}

	//open socket
	if( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ){
		cerr << "simplex-talk: socket" << endl;;
		exit(1);
	}
	bzero( &servaddr, sizeof(servaddr) );
	servaddr.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *)&servaddr.sin_addr, hp->h_length); //copy hostent with ip address into the servaddr
	servaddr.sin_port = htons( PORT_NUM );

	//send message
	gettimeofday( &startTime, NULL );
	if(sendto( sockfd, buffer_str.c_str(), buffer_str.size(), 0, (struct sockaddr *)&servaddr, sizeof(servaddr) ) < 0  ) {
		cerr << "Client send error." << endl;
		exit(1);
	}

	//receive and decode message
	bzero( &recvline, sizeof(recvline) );	// clean up char array before storing the server's return message
	if( (n=recvfrom( sockfd, recvline, sizeof(recvline), 0, NULL, NULL )) < 0){
		cerr << "Client receive error." << endl;
		exit(1);
	} 
	gettimeofday( &endTime, NULL );
	recvdecoded = decode(string(recvline));

	//check for errors in message server returned to client
	if( buffer_str.c_str() != recvdecoded.substr(0, (recvdecoded.size()-1) - timestamp_size) ) {
		cerr << "Error. Message received from server is not the same as the message sent by the client." << endl;
		exit(1);
	} else if( n == sizeof(recvline) ){
		cerr << "Warning: Client received return message from server of equal or greater size than size of the receiving buffer(" << sizeof(recvline) << ")." << endl << endl;
	}

	//close socket
	if(close(sockfd) != 0){
		cerr << "sockfd close failed." << endl;
	}

	//print results
	cout << "DECRYPTED MESSAGE:\n" << recvdecoded << endl;
	cout << "RTT: " << (1000000*endTime.tv_sec+endTime.tv_usec) - (1000000*startTime.tv_sec+startTime.tv_usec) << " usec" << endl << endl;
}
// END MAIN - begin function definitions

string writeBuffer(const char* buffer_input) {
	string out_str;
	//File or string?
	if(isFile(buffer_input)){
		//read file into buffer
		string temp;
		ifstream inFile;		// object for reading from a file
		inFile.open(buffer_input, ios::in);
		while (getline(inFile,temp) ){
			out_str += temp + '\n';
		}
		if(!out_str.empty()) out_str.erase(out_str.end() - 1);	// chops final \n
		inFile.close();		
	}else{
		//read string into buffer
		out_str = string(buffer_input);
	}
	return out_str;
}

int isFile(const char* fileName) {
	ifstream inFile;		// object for reading from a file
	inFile.open(fileName, ios::in);
	if(! inFile) {
		inFile.close();
		return 0;
	}
	inFile.close();
	return 1;
}

string decode(string encryption) {
	int i = 0;
	for( int j=encryption.size()-1; j >= i; j-=2, i+=2) {
		swap(encryption[i], encryption[j]);
	}
	return encryption;
}

void swap(char a, char b) {
	char temp = a;
	a = b;
	b = temp;
}
