/* Kristopher Thieneman, kthienem
 * Patrick Drumm, pdrumm
 * Nick Lombardo, nlombard
 *
 * tcpclient.cpp
 * This file is a simple TCP client. To test that we have correctly implemented
 * the TCP interface a few tests are run. The arguments taken in are the machine
 * of the server, the port number, and the name of the file you wish to use. After
 * connecting to the server the program sends the length of the filename and the
 * actual filename to the server. It then waits for the server to return the size
 * of the file. If the file size if not negative then the file exists. If the file
 * exists it then receives the MD5 hash of the value and the contents of the file
 * from the server. The contents of the file are written to a temporary file. The
 * client then computes the hash of the file itself. It the client computed hash
 * mathces the server computed hash then the transfer was good and appropriate
 * messages are printed.
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <mhash.h>
#include <sys/time.h>
#include <fstream>

#define MAX_LINE 1000

using namespace std;

void getHash(const char *, unsigned char[]);//computes hash of file, arguments are name of file and the variable that the compute hash is stored in

int main(int argc, char * argv[])
{
	struct hostent *hp;
	struct sockaddr_in sin;
	char *host;
	char buf[MAX_LINE];
	int s;
	int len, filesize;
	unsigned char hash[16];

	if (argc == 4) {
		host = argv[1];
	} else {
		cerr << "usage: simplex-talk host" << endl;
		exit(1);
	}

	/*translate host name into peer's IP address*/
	hp = gethostbyname(host);
	if (!hp) {
		cerr << "simplex-talk: unknown host: " << host << endl;
		exit(1);
	}

	/*build address data structure*/
	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = htons(atoi(argv[2]));

	/*active open*/
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		cerr << "simplex-talk: socket" << endl;
		exit(1);
	}
	cout << "Welcome to TCP client!" << endl;

	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		cerr << "simplex-talk: connect" << endl;
		close(s);
		exit(1);
	}

	/*main*/
	sprintf(buf, "%d", strlen(argv[3]));
	buf[MAX_LINE-1] = '\0';
	len = strlen(buf) + 1;
	if (send(s, buf, len, 0) == -1) {//sends length of filename
		cerr << "client send error" << endl;
		exit(1);
	}
	bzero((char *)&buf, sizeof(buf));//clears buf

	strcpy(buf, argv[3]);
	buf[MAX_LINE-1] = '\0';
	len = strlen(buf) + 1;
	if (send(s, buf, len, 0) == -1) {//sends name of file
		cerr << "client send error" << endl;
		exit(1);
	}
	bzero((char *)&buf, sizeof(buf));//clears buf

	if ((len = recv(s, buf, sizeof(buf), 0)) == -1) {//receives size of file
		cerr << "client receive error" << endl;
		exit(1);
	}
	
	if (len != 0) {
		//checks if the file size is negative to tell if the file exists 
		if (atoi(buf) < 0) {
			cerr << "File does not exist" << endl;
			exit(1);
		} else {
			filesize = atoi(buf);//saves file size to use later when receiving file
		}
	}
	bzero((char *)&buf, sizeof(buf));//clears buf

	/*receive hash value calculated by server*/
	if ((len = recv(s, buf, sizeof(buf), 0)) == -1) {
		cerr << "client receive error" << endl;
		exit(1);
	}
	//converts char[] received to unsigned char and stores for later use
	for (int i = 0; i < mhash_get_block_size(MHASH_MD5); i++) {
		hash[i] = (unsigned char)buf[i];
	}
	bzero((char *)&buf, sizeof(buf));//clears buf

	/*receive file from server and put into string*/
	int bytesRecv = 0;
	string filename = "temp.txt";
	struct timeval recvBegin, recvEnd;
	ofstream myfile;
	FILE *outFile;
	outFile = fopen(filename.c_str(), "wb");

	gettimeofday(&recvBegin, NULL);//time at beginning of transmission
	while (bytesRecv < filesize) {//loops until all bytes are received
		/*if length is 0, it gets stuck in infinite loop due to it not reaching the required number of bytes so break out using return value of recv()*/
		if ((len = recv(s, buf, sizeof(buf), 0)) <= 0) {
			cerr << "client receive error" << endl;
			exit(1);
		}
		bytesRecv += len;
		fwrite(buf, sizeof(char), len, outFile);//writes received char[] to file
		bzero((char *)&buf, sizeof(buf));//clears buf
	}
	gettimeofday(&recvEnd, NULL);//time at end of transmission
	close(s);
	fclose(outFile);//close file being written to

	unsigned char myHash[16];
	getHash(filename.c_str(), myHash);
	if (remove(filename.c_str()) != 0) {
		cerr << "Error deleting file" << endl;
	}

	double timeTaken = (1000000*recvEnd.tv_sec+recvEnd.tv_usec) - (1000000*recvBegin.tv_sec+recvBegin.tv_usec);
	
	/*checks if the two hashes match*/
	for (int i = 0; i < mhash_get_block_size(MHASH_MD5); i++){
		if (myHash[i] != hash[i]){
			cerr << "File hashes do not match - bad transfer" << endl;
			exit(1);
		}
	}

	/*Prints details of transmission if hashes matched*/
	cout << "Hashes match" << endl;
	cout << filesize << " bytes transfered in " << timeTaken/1000000. << " seconds." << endl;
	cout << "Throughput: " << (filesize/1000000.)/(timeTaken/1000000.) << " Megabytes/sec" << endl;
	cout << "File MD5 sum: ";
	for (int i = 0; i < mhash_get_block_size(MHASH_MD5); i++){
		printf("%.2x", myHash[i]);
	}
	cout << endl;
}

void getHash(const char *file, unsigned char hash_result[]){
	MHASH md5;
	char temp;
	FILE *infile;
	infile = fopen(file, "rb");

	md5 = mhash_init(MHASH_MD5);
	if( md5 == MHASH_FAILED ){
		cerr << "simplex-talk: mhash" << endl;
		exit(1);
	}
	
	while (fread(&temp, 1, 1, infile)) {
		mhash(md5, &temp, 1);
	}

	mhash_deinit(md5,hash_result);
	fclose(infile);//close file being read in
}
