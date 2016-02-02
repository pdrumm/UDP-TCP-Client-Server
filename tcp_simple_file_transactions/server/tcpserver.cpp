/* tcpserver.cpp
 *
 * Authors: 	Patrick Drumm,	pdrumm
 * 		Kris Thieneman,	kthienem
 * 		Nick Lombardo,	nlombard
 * Class: CSE 30264
 *
 * Function: This tcp server file performs the following steps...
 * 	1. starts and listens on a port entered from command line
 * 	2. accepts an incoming request from a single client
 * 	3. processes the request, by performing various server-client operations (delete, list, request, upload, exit)
 * 	4. closes client connection and then continues to listen for next client connection
 *
 * Example command line:
 * 	./myftpd 41013
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
#include <sys/time.h>
#include <dirent.h>
#define MAX_PENDING 5
#define MAX_LINE 500
#define ARG_COUNT 2
// global enum
enum Client_op_list { DEL, LIS, REQ, UPL, XIT, OTHER };
using namespace std;

// function prototypes
int getFileSize(char*);
void getHash(char*,char[]);
int select_client_op(char[]);
bool delete_file(int);
bool list_files(int);
bool request_files(int);
bool upload_files(int);

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

	cout << "Welcome to the second TCP Server!!!!" << endl;
	// Wait for connection, then receive and print text
	while(1){
		// wait here until new client connects
		if((new_s = accept(s,(struct sockaddr *)&sin,(socklen_t *)&len)) < 0) {
			cerr << "simplex-talk: accept" << endl;
			exit(1);
		}

		bool isXIT = false;
		while( !isXIT ){
			// wait for new client operation
			// 	options are:
			// 		- DEL
			// 		- LIS
			// 		- REQ
			// 		- UPL
			// 		- XIT
			// 		- OTHER

			// receive client operation
			if((len=recv(new_s, buf, sizeof(buf), 0))<= 0) {
				cerr << "Sender Received Error!" << endl;
				isXIT = true;
			}

			int client_op = select_client_op( buf );
			bzero(buf, sizeof(buf));
			
			switch(client_op){
				case DEL:
					isXIT = delete_file(new_s);
					break;
				case LIS:
					isXIT = list_files(new_s);
					break;
				case REQ:
					isXIT = request_files(new_s);
					break;
				case UPL:
					isXIT = upload_files(new_s);
					break;
				case XIT:
					isXIT = true;
					break;
				default://OTHER
					break;

			}

		} // end current client while loop
		// Client interaction complete (XIT received)! Close socket and wait for next client
		close (new_s);
	} // end main while loop
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

int select_client_op(char buf[]){
	if( strcmp(buf,"DEL")==0 )
		return DEL;
	else if( strcmp(buf,"LIS")==0 )
		return LIS;
	else if( strcmp(buf,"REQ")==0 )
		return REQ;
	else if( strcmp(buf,"UPL")==0 )
		return UPL;
	else if( strcmp(buf,"XIT")==0 )
		return XIT;
	else
		return OTHER;
}

bool request_files(int new_s) {
	char buf[MAX_LINE]="Enter your filename: ",
		file_name[MAX_LINE];
	int len;
	int file_name_size, file_size;
	char file_size_char[100];
	char hash_result[16];

	// send query
	if (send(new_s, buf, sizeof(buf),0)==-1){
		cerr << "simplex-talk: send to client" << endl;
		return true;
	}
	bzero(buf, sizeof(buf));

	// receive file name size
	if((len=recv(new_s, buf, sizeof(buf), 0))== -1) {
		cerr << "Sender Received Error!" << endl;
		return true;
	}
	file_name_size = atoi( buf );
	bzero(buf, sizeof(buf));
	// receive file name
	if((len=recv(new_s, buf, sizeof(buf), 0))== -1) {
		cerr << "Sender Received Error!" << endl;
		return true;
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
		return true;
	}
	// skip to close local socket if file does not exist or file sizes did not match
	if( file_size >= 0 ){
		// calculate MD5 hash
		bzero(hash_result,sizeof(hash_result));
		getHash(file_name, hash_result);
		// send calculated hash value
		if (send(new_s, hash_result, sizeof(hash_result),0)==-1){
			cerr << "simplex-talk: send hash to client" << endl;
			return true;
		}

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
				return true;
			}
		}
		fclose(in_file);
	}
	bzero(buf, sizeof(buf));
	return false;
}

bool delete_file(int new_s) {
	char buf[1000];
	char file_size_char[100];
	char file_name[200];
	int file_name_size = 0;
	int file_size = 0;
	int len = 0;
	char confirm[5] = "-1";
	bzero(file_size_char, sizeof(file_size_char));
	bzero(file_name, sizeof(file_name));
	bzero(confirm, sizeof(confirm));
	//rec length of file
	if((len=recv(new_s, file_size_char, sizeof(file_size_char), 0))== -1) {
		cerr << "Sender Received Error!" << endl;
		return true;
	}
	file_name_size = atoi(file_size_char);
	bzero(file_size_char, sizeof(file_size_char));
	//rec file name
	if((len=recv(new_s, file_name, sizeof(file_name), 0))== -1) {
		cerr << "Sender Received Error!" << endl;
		return true;
	}
	if( len-1 != file_name_size ){
		cerr << "Sender file name does not match initially sent file name size." << endl;
		file_size = -1;
		return false;
	} else { //file size and length of name match, confirm.
		// get file size if size numbers match
		if((file_size=getFileSize(file_name))<0){
			cerr << "File does not exist" << endl;
			strcpy(confirm,"-1");
			if (send(new_s, confirm, sizeof(confirm),0)==-1){
				cerr << "simplex-talk: send to client" << endl;
				return true;
			}
			return false;
		} else {
			//file exists so now confirm
			strcpy(confirm,"1");
			if (send(new_s, confirm, sizeof(confirm),0)==-1){
				cerr << "simplex-talk: send to client" << endl;
				return true;
			}
			//
			bzero(buf, sizeof(buf));
			//rec file name
			if((len=recv(new_s, buf, sizeof(buf), 0))== -1) {
				cerr << "Sender Received Error!" << endl;
				return true;
			}
			bzero(confirm, sizeof(confirm));
			if(strcmp(buf,"Yes") == 0){
				if(remove(file_name) != 0) {
					cerr << "Error deleting file" << endl;
					strcpy(confirm,"FAIL");
					if (send(new_s, confirm, sizeof(confirm),0)==-1){
						cerr << "simplex-talk: send to client" << endl;
						return true;
					}
				}
				//success = return ack
				strcpy(confirm,"ACK");
				if (send(new_s, confirm, sizeof(confirm),0)==-1){
					cerr << "simplex-talk: send to client" << endl;
					return true;
				}
				
			} else {
			 	//client did not say yes
			 	return false;
			}
			
		}
	}
	return false;
}

bool list_files(int new_s) {
	DIR *d;
	struct dirent *dir;
	char buf[1000];
	string dir_listing;

	// open directory and read in directory names
	d = opendir(".");
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			dir_listing += dir->d_name;
			dir_listing += "\n";
		}
		closedir(d);
	}
	// send directory names
	sprintf (buf, "%d", dir_listing.size());
	if (send(new_s, buf, sizeof(buf),0)==-1){
		cerr << "simplex-talk: send to client" << endl;
		return true;
	}
	bzero(buf, sizeof(buf));
	buf[MAX_LINE-1] = '\0';
	sprintf (buf, "%s", dir_listing.c_str());
	if (send(new_s, buf, sizeof(buf),0)==-1){
		cerr << "simplex-talk: send to client" << endl;
		return true;
	}
	return false;
}

bool upload_files(int s) {
	char buf[MAX_LINE]="Enter your filename: ",
		file_name[MAX_LINE] = "";
	int len, filesize, file_name_size;
	bzero(file_name, sizeof(file_name));

	// send query
	if (send(s, buf, sizeof(buf),0)==-1){
		cerr << "simplex-talk: send to client" << endl;
		return true;
	}
	bzero(buf, sizeof(buf));

	// receive file name size
	if((len=recv(s, buf, sizeof(buf), 0))== -1) {
		cerr << "Sender Received Error!" << endl;
		return true;
	}
	file_name_size = atoi( buf );
	bzero(buf, sizeof(buf));
	// receive file name
	if((len=recv(s, buf, sizeof(buf), 0))== -1) {
		cerr << "Sender Received Error!" << endl;
		return true;
	}
	// store buffer into file name char array
	bzero(file_name, sizeof(file_name));
	for( int i=0; i<file_name_size; ++i )
		file_name[i]=buf[i];
	file_name[file_name_size] = '\0';
	bzero(buf, sizeof(buf));

	// Send acknowledgment - ready to receive file!
	strcpy(buf, "ACK");
	buf[MAX_LINE-1] = '\0';
	len = strlen(buf) + 1;
	if(send(s, buf, len, 0) == 1) {
		cerr << "client send error" << endl;
		return true;
	}
	bzero((char *)&buf, sizeof(buf));//clears buf

	// Receive size of file
	if((len = recv(s, buf, sizeof(buf), 0)) == -1) {
		cerr << "client receive error" << endl;
		return true;
	}
	//Check for valid file size
	if(len != 0) {
		if(atoi(buf) < 0) {
			return false;
		} else {
			filesize = atoi(buf);
		}
	}
	bzero((char *)&buf, sizeof(buf));//clears buf*/

	// Receive file
	int bytesRecv = 0;
	struct timeval recvBegin, recvEnd;
	FILE *outFile;
	outFile = fopen(file_name, "wb");
	gettimeofday(&recvBegin, NULL);//time at beginning of transmission
	while(bytesRecv < filesize) {//loops until all bytes are received
		/*if length is 0 in infinite loop due to it not reaching he required number
 			* of bytes so break out using return value of recv()*/
		if((len = recv(s, buf, sizeof(buf), 0)) <= 0) {
			cerr << "client recieve error" << endl;
			return true;
		}
		bytesRecv += len;
		fwrite(buf, sizeof(char), len, outFile);//writes received char[] to file
		bzero((char *)&buf, sizeof(buf));//clears buf
	}
	gettimeofday(&recvEnd, NULL);//time at end of transmission

	//close file being written to
	fclose(outFile);

	//receive hash calculated by client
	if((len = recv(s, buf, sizeof(buf), 0)) == -1) {
		cerr << "client receive error" << endl;
		return true;
	}
	//receive hash calculated by client
	//convert char[] to unsigned char[]
	char hash[16];
	for(int i = 0; i < mhash_get_block_size(MHASH_MD5); i++) {
		hash[i] = buf[i];
	}
	bzero((char *)&buf, sizeof(buf));//clears buf
	//receive hash calculated by client
	//compute hash of received file
	char myHash[16];
	getHash(file_name, myHash);

	//receive hash calculated by client
	//check if hashes match
	bool hashes_match = true;
	for(int i = 0; i < mhash_get_block_size(MHASH_MD5); i++) {
		if(myHash[i] != hash[i]) {
			hashes_match = false;
		}
	}

	if(hashes_match){
		// Calculate performance statistics
		double timeTaken = (1000000*recvEnd.tv_sec+recvEnd.tv_usec) - (1000000*recvBegin.tv_sec+recvBegin.tv_usec);
		char through[100];
		sprintf (through, "%d bytes transfered in %f seconds.\nThroughput: %f Megabytes/sec\n", filesize, timeTaken/1000000., (filesize/1000000.)/(timeTaken/1000000.));
		if(send(s, through, sizeof(through), 0) == 1) {
			cerr << "client send error" << endl;
			return true;
		}
	}else{
		// clean up on bad transfer
		string bad_hash = "fail";
		if(send(s, bad_hash.c_str(), sizeof(bad_hash.c_str()), 0) == 1) {
			cerr << "client send error" << endl;
			return true;
		}
		if(remove(file_name) != 0) {
			cerr << "Error deleting file" << endl;
		}
	}
	return false;
}
