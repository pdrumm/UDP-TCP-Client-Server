/* tcpserver.cpp
 *
 * Authors: 	Patrick Drumm,	pdrumm
 * 		Kris Thieneman,	kthienem
 * 		Nick Lombardo,	nlombard
 * Class: CSE 30264
 *
 * Function: This tcp server file performs the following steps...
 * 	1. starts and listens on a port entered from command line
 * 	2. accepts an incoming request from a client
 * 	3. accepts a file name and the size of that name from client
 * 	4. replies back to client with size of requested file
 * 	5. calculates the MD5 hash of the requested file
 * 	6. sends the client the calculated MD5 hash
 * 	7. sends the client the contents of the requested file
 * 	8. closes client connection and then continues to listen for next client connection
 *
 * Example command line:
 * 	./tcpserver 41013
 * Output:
 * 	This tcp server will only print output to the screen/stderr if an error of some type occurs.
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fstream>
#include <mhash.h>
#define MAX_PENDING 5
#define MAX_LINE 2000
#define ARG_COUNT 2
using namespace std;

// function prototypes
int getFileSize(char*);
void getHash(char*,char[]);

int main(int argc, char** argv) {
	struct sockaddr_in sin;
	char buf[MAX_LINE], file_name[MAX_LINE];
	int len;
	int s, new_s;
	int SERVER_PORT;
	int file_name_size, file_size;
	char file_size_char[100];
	char hash_result[16];

	// check for correct number of args
	if (argc != ARG_COUNT) {
		cerr << "simplex-talk: command line call" << endl;
		exit(1);
	}
	SERVER_PORT = atoi( argv[1] );

	// build address data structure
	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);
	bzero(buf, sizeof(buf));
	
	// setup passive open
	if((s=socket(PF_INET,SOCK_STREAM,0))<0){
		cerr << "simplex-talk: socket" << endl;
		exit(1);
	}
	// allow for reuse
	int opt = 1; /* 0 to disable options */
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR,(char *)&opt, sizeof(int));
	if((bind(s,(struct sockaddr *)&sin, sizeof(sin)))<0) {
		cerr << "simplex-talk: bind" << endl;
		exit(1);
	}
	if((listen(s,MAX_PENDING))<0) {
		cerr << "simplex-talk: listen" << endl;
		exit(1);
	}

	cout << "Welcome to the first TCP Server!!!!" << endl;
	// Wait for connection, then receive and print text
	while(1){
		if((new_s = accept(s,(struct sockaddr *)&sin,(socklen_t *)&len)) < 0) {
			cerr << "simplex-talk: accept" << endl;
			exit(1);
		}
		// receive file name size
		if((len=recv(new_s, buf, sizeof(buf), 0))== -1) {
			cerr << "Sender Received Error!" << endl;
			exit(1);
		}
		file_name_size = atoi( buf );
		bzero(buf, sizeof(buf));
		// receive file name
		if((len=recv(new_s, buf, sizeof(buf), 0))== -1) {
			cerr << "Sender Received Error!" << endl;
			exit(1);
		}
		// store buffer into file name char array
		bzero(file_name, sizeof(file_name));
		for( int i=0; i<file_name_size; ++i )
			file_name[i]=buf[i];
		file_name[file_name_size] = '\0';
		bzero(buf, sizeof(buf));

		// confirm that file name size matches the file name
		if( len-1 != file_name_size ){
			cerr << "Sender file name does not match initially sent file name size." << endl;
			file_size = -1;
		} else {
			// get file size if size numbers match
			if((file_size=getFileSize(file_name))<0){
				cerr << "File does not exist" << endl;
			}
		}
		// store interger as char array
		bzero(file_size_char, sizeof(file_size_char));
		sprintf(file_size_char, "%d", file_size);
		// send file size
		if (send(new_s, file_size_char, sizeof(file_size_char),0)==-1){
			cerr << "simplex-talk: send to client" << endl;
			exit(1);
		}
		// skip to close local socket if file does not exist or file sizes did not match
		if( file_size >= 0 ){
			// calculate MD5 hash
			bzero(hash_result,sizeof(hash_result));
			getHash(file_name, hash_result);
			// send calculated hash value
			if (send(new_s, hash_result, sizeof(hash_result),0)==-1){
				cerr << "simplex-talk: send hash to client" << endl;
				exit(1);
			}

//sleep(1);
			// open file
			FILE* in_file = fopen(file_name,"r");
			// send full file
			int sub_string_size = MAX_LINE; //buffer size for sending
			for( int i=0; i<file_size; i+=sub_string_size){
				if(file_size-i < sub_string_size){ //end case of sending less than buffer of sub_string_size
					sub_string_size = (file_size - i);
				}
				bzero(buf, sizeof(buf));
				fread(&buf, sizeof(char), sub_string_size, in_file);
				// send sub_string_size chars to the client
				if ((send(new_s, buf, sub_string_size, 0))==-1){
					cerr << "simplex-talk: send file to client" << endl;
					exit(1);
				}
			}
			fclose(in_file);
		}
		bzero(buf, sizeof(buf));
		// file transfer complete! Close socket and wait for next client
		close (new_s);
	}
	close(s);
}

int getFileSize(char* file_name) {
	int file_size;
	ifstream in_file( file_name, ios::binary | ios::ate);
	if(! in_file){
		file_size = -1;
	} else {
		file_size = in_file.tellg();
	}
	in_file.close();

	return file_size;
}

void getHash(char* file_name, char hash_result[]){
	MHASH md5;
	string temp;
	char buffer;

	FILE* in_file = fopen(file_name,"r");

	md5 = mhash_init(MHASH_MD5);
	if( md5 == MHASH_FAILED ){
		cerr << "simplex-talk: mhash" << endl;
		exit(1);
	}

	while( fread(&buffer, sizeof(char), 1, in_file) == 1 ){
		mhash(md5, &buffer, 1);
	}

	mhash_deinit(md5,hash_result);
	fclose(in_file);
}
