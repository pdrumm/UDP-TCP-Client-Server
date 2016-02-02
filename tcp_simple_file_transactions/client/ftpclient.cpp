/* Kristopher Thieneman, kthienem
 * Patrick Drumm, pdrumm
 * Nick Lombardo, nlombard
 *
 * ftpclient.cpp
 * This program is a simple FTP trasnfer system paired with ftpserver.cpp. This
 * simple FTP client allows you to request files to be downloaded to the client,
 * upload files to the server, list the files currently in the server's directory
 * and delete files from the server directory. Once you are finished transfering
 * files you can then enter a command to close the connection with the server.
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

//arguments: int - socket number,
void REQ(int);//download file from server

//arguments: int - socket number,
void UPL(int);//upload file to server

//arguments: int - socket number,
void DEL(int);//delete file from server

//arguments: int - socket number,
void LIS(int);//list server's directory

int getFileSize(const char*);//get size of file being sent by client

int main(int argc, char * argv[])
{
	struct hostent *hp;
	struct sockaddr_in sin;
	char *host;
	int s;

	if (argc == 3) {
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

	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		cerr << "simplex-talk: connect" << endl;
		close(s);
		exit(1);
	}
	cout << "Connected to ftp server" << endl;

	/*main*/
	char buf[MAX_LINE];
	int len, filesize;
	unsigned char hash[16];
	bool running = true;
	string command;

	while(running) {
		cout << "Enter command (REQ, UPL, DEL, LIS, XIT): ";
		cin >> command;
		if(command == "REQ") {
			REQ(s);
		} else if(command == "UPL") {
			UPL(s);
		} else if(command == "DEL") {
			DEL(s);
		} else if(command == "LIS") {
			LIS(s);
		} else if(command == "XIT") {
			strcpy(buf, "XIT");
			buf[MAX_LINE-1] = '\0';
			len = strlen(buf) + 1;
			if(send(s, buf, len, 0) == -1) {
				cerr << "client send error" << endl;
				exit(1);
			}
			running = false;
		} else {
			cout << "Invlaid command. Please try again." << endl;
		}
	}
	close(s);
	
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

void REQ(int s) {
	char buf[MAX_LINE];
	int len, filesize;
	string filename;

	strcpy(buf, "REQ");
	buf[MAX_LINE-1] = '\0';
	len = strlen(buf) + 1;
	if(send(s, buf, len, 0) == -1) {//send REQ operation to server
		cerr << "client send error" << endl;
		exit(1);
	}
	bzero((char *)&buf, sizeof(buf));//clears buf

	if((len	= recv(s, buf, sizeof(buf), 0)) == -1) {//server sends filename request
		cerr << "client receive error" << endl;
		exit(1);
	}

	cout << buf;//print request to user
	cin >> filename;//get filename from user
	bzero((char *)&buf, sizeof(buf));//clears buf

	sprintf(buf, "%d", strlen(filename.c_str()));
	buf[MAX_LINE-1] = '\0';
	len = strlen(buf) + 1;
	if(send(s, buf, len, 0) == -1) {//send length of filename
		cerr << "client send error" << endl;
		exit(1);
	}
	bzero((char *)&buf, sizeof(buf));//clears buf

	strcpy(buf, filename.c_str());
	buf[MAX_LINE-1] = '\0';
	len = strlen(buf) + 1;
	if(send(s, buf, len, 0) == 1) {//send name of file
		cerr << "client send error" << endl;
		exit(1);
	}
	bzero((char *)&buf, sizeof(buf));//clears buf

	if((len = recv(s, buf, sizeof(buf), 0)) == -1) {//receive size of file
		cerr << "client receive error" << endl;
		exit(1);
	}

	//Check for valid file size
	if(len != 0) {
		if(atoi(buf) < 0) {
			cerr << "File does not exist." << endl;
			return;
		} else {
			filesize = atoi(buf);
		}
	}
	bzero((char *)&buf, sizeof(buf));//clears buf*/


	//receive hash calculated by server
	if((len = recv(s, buf, sizeof(buf), 0)) == -1) {
		cerr << "client receive error" << endl;
		exit(1);
	}

	//convert char[] to unsigned char[]
	unsigned char hash[16];
	for(int i = 0; i < mhash_get_block_size(MHASH_MD5); i++) {
		hash[i] = (unsigned char)buf[i];
	}
	bzero((char *)&buf, sizeof(buf));//clears buf

	int bytesRecv = 0;
	struct timeval recvBegin, recvEnd;
	FILE *outFile;
	outFile = fopen(filename.c_str(), "wb");
	gettimeofday(&recvBegin, NULL);//time at beginning of transmission
	while(bytesRecv < filesize) {//loops until all bytes are received
		/*if length is 0 in infinite loop due to it not reaching he required number
 			* of bytes so break out using return value of recv()*/
		if((len = recv(s, buf, sizeof(buf), 0)) <= 0) {
			cerr << "client recieve error" << endl;
			exit(1);
		}
		bytesRecv += len;
		fwrite(buf, sizeof(char), len, outFile);//writes received char[] to file
		bzero((char *)&buf, sizeof(buf));//clears buf
	}
	gettimeofday(&recvEnd, NULL);//time at end of transmission
	fclose(outFile);//close file being written to

	//compute hash of received file
	unsigned char myHash[16];
	getHash(filename.c_str(), myHash);

	//check if hashes match
	for(int i = 0; i < mhash_get_block_size(MHASH_MD5); i++) {
		if(myHash[i] != hash[i]) {
			cerr << "File hashes do not match - bad transfer" << endl;
			if(remove(filename.c_str()) != 0) {
				cerr << "Error deleting file" << endl;
			}
			return;
		}
	}

	//Print performance statistics
	double timeTaken = (1000000*recvEnd.tv_sec+recvEnd.tv_usec) - (1000000*recvBegin.tv_sec+recvBegin.tv_usec);

	/*Prints details of transmission if hashes matched*/
	cout << "Hashes match" << endl;
	cout << filesize << " bytes transfered in " << timeTaken/1000000. << " seconds." << endl;
	cout << "Throughput: " << (filesize/1000000.)/(timeTaken/1000000.) << " Megabytes/sec" << endl;
}

void UPL(int s) {
	char buf[MAX_LINE];
	int len, filesize;
	string filename;

	strcpy(buf, "UPL");
	buf[MAX_LINE-1] = '\0';
	len = strlen(buf) + 1;
	if(send(s, buf, len, 0) == -1) {//send UPL operation to server
		cerr << "client send error" << endl;
		exit(1);
	}
	bzero((char *)&buf, sizeof(buf));//clears buf

	if((len	= recv(s, buf, sizeof(buf), 0)) == -1) {//server sends filename request
		cerr << "client receive error" << endl;
		exit(1);
	}

	cout << buf;//request filename from user
	cin >> filename;//get filename from user
	bzero((char *)&buf, sizeof(buf));//clears buf

	//check that file exists
	int file_size;
	if((file_size = getFileSize(filename.c_str())) < 0) {
		cerr << "File does not exist" << endl;
	}
	
	sprintf(buf, "%d", strlen(filename.c_str()));
	buf[MAX_LINE-1] = '\0';
	len = strlen(buf) + 1;
	if(send(s, buf, len, 0) == -1) {//send length of filename
		cerr << "client send error" << endl;
		exit(1);
	}
	bzero((char *)&buf, sizeof(buf));//clears buf

	strcpy(buf, filename.c_str());
	buf[MAX_LINE-1] = '\0';
	len = strlen(buf) + 1;
	if(send(s, buf, len, 0) == 1) {//send name of file
		cerr << "client send error" << endl;
		exit(1);
	}
	bzero((char *)&buf, sizeof(buf));//clears buf

	//server acknowledges it is ready to receive
	if((len = recv(s, buf, sizeof(buf), 0)) == -1) {
		cerr << "client receive error" << endl;
		exit(1);
	}
	if(file_size != -1) {
		if(strcmp((buf), "ACK") == 0) {
			cout << "Server ready. Sending file..." << endl;
		} else {
			cout << "Server not ready to receive at this time. Please try again later." << endl;
			return;
		}
	}
	bzero((char *)&buf, sizeof(buf));//clears buf

	sprintf(buf, "%d", file_size);
	buf[MAX_LINE-1] = '\0';
	len = strlen(buf) + 1;
	if(send(s, buf, len, 0) == -1) {//send size of file to server
		cerr << "client send error" << endl;
		exit(1);
	}
	bzero((char *)&buf, sizeof(buf));//clears buf
	if(file_size == -1) return;

	//send file
	FILE* inFile = fopen(filename.c_str(), "r");
	int subStringSize = MAX_LINE;//buffer size for sending
	for(int i = 0; i < file_size; i+=subStringSize) {
		if(file_size-i < subStringSize) {//end case fo sending less than buffer of subStringSize
			subStringSize = file_size - i;
		}
		bzero((char *)&buf, sizeof(buf));//clears buf
		fread(&buf, sizeof(char), subStringSize, inFile);
		if((len = send(s, buf, subStringSize, 0)) == -1) {
			cerr << "client send error" << endl;
			exit(1);
		}	
	}
	fclose(inFile);
	bzero((char *)&buf, sizeof(buf));//clears buf

	//Compute hash
	unsigned char myHash[16];
	getHash(filename.c_str(), myHash);
	for(int i = 0; i < mhash_get_block_size(MHASH_MD5); i++) {
		buf[i] = (char)myHash[i];
	}
	sleep(1);
	buf[MAX_LINE-1] = '\0';
	len = strlen(buf) + 1;
	if(send(s, buf, len, 0) == -1) {
		cerr << "client send error" << endl;
		exit(1);
	}
	//Receive throughput results
	if((len = recv(s, buf, sizeof(buf), 0)) == -1) {
		cerr << "Client receive error" << endl;
		exit(1);
	}
	if(strcmp(buf, "fail") == 0) {//failed transfer
		cout << "Transfer unsuccessful" << endl;
	} else {//succesful transfer
		cout << buf;
	}
}

void DEL(int s) {
	char buf[MAX_LINE];
	int len, dirSize;
	string filename;

	strcpy(buf, "DEL");
	buf[MAX_LINE-1] = '\0';
	len = strlen(buf) + 1;
	if(send(s, buf, len, 0) == -1) {//send DEL operation to server
		cerr << "client send error" << endl;
		exit(1);
	}
	bzero((char *)&buf, sizeof(buf));//clears buf

	cout << "What file do you want to delete: ";//print request to user
	cin >> filename;//get filename from user
	bzero((char *)&buf, sizeof(buf));//clears buf

	sprintf(buf, "%d", strlen(filename.c_str()));
	buf[MAX_LINE-1] = '\0';
	len = strlen(buf) + 1;
	if(send(s, buf, len, 0) == -1) {//send length of filename
		cerr << "client send error" << endl;
		exit(1);
	}
	bzero((char *)&buf, sizeof(buf));//clears buf

	strcpy(buf, filename.c_str());
	buf[MAX_LINE-1] = '\0';
	len = strlen(buf) + 1;
	if(send(s, buf, len, 0) == 1) {//send name of file
		cerr << "client send error" << endl;
		exit(1);
	}
	bzero((char *)&buf, sizeof(buf));//clears buf

	if(recv(s, buf, sizeof(buf), 0) == -1) {
		cerr << "client receive error" << endl;
		exit(1);
	}
	if(atoi(buf) == -1) {
		cout << "The file does not exist on server" << endl;
	} else if(atoi(buf) == 1) {
		bzero((char *)&buf, sizeof(buf));//clears buf
		bool confirmed = false;
		while(!confirmed) {
			cout << "Are you sure you want to delete file " << filename << "(Yes/No)? ";
			string confirm;
			cin >> confirm;
			if(confirm == "Yes") {
				confirmed = true;
				strcpy(buf, confirm.c_str());
				if(send(s, buf, sizeof(buf), 0) == -1) {
					cerr << "client send error" << endl;
					exit(1);
				}
			} else if(confirm == "No") {
				confirmed = true;
				strcpy(buf, confirm.c_str());
				if(send(s, buf, sizeof(buf), 0) == -1) {
					cerr << "client send error" << endl;
					exit(1);
				}
				cout << "Delete abandoned by user" << endl;
				return;
			} else {
				cout << "Invalid input. Please try again." << endl;
			}
		}
		bzero((char *)&buf, sizeof(buf));//clears buf
		if((len = recv(s, buf, sizeof(buf), 0)) == -1) {
			cerr << "client receive error" << endl;
			exit(1);
		}
		if(strcmp(buf, "ACK") == 0) {
			cout << "File deleted" << endl;
		} else {
			cout << "Error deleting file" << endl;
		}
	}
}

void LIS(int s) {
	char buf[MAX_LINE];
	int len, dirSize;
	string filename;

	strcpy(buf, "LIS");
	buf[MAX_LINE-1] = '\0';
	len = strlen(buf) + 1;
	if(send(s, buf, len, 0) == -1) {//send LIS operation to server
		cerr << "client send error" << endl;
		exit(1);
	}
	bzero((char *)&buf, sizeof(buf));//clears buf

	if((len = recv(s, buf, sizeof(buf), 0)) == -1) {//receive size of directory listing
		cerr << "client receive error" << endl;
		exit(1);
	}

	dirSize = atoi(buf);

	string listing = "";
	int bytesRecv = 0;
	while(bytesRecv < dirSize) {//loops until all bytes are received
		/*if length is 0 in infinite loop due to it not reaching he required number
 			* of bytes so break out using return value of recv()*/
		if((len = recv(s, buf, sizeof(buf), 0)) <= 0) {
			cerr << "client recieve error" << endl;
			exit(1);
		}
		bytesRecv += len;
		listing += buf;
		bzero((char *)&buf, sizeof(buf));//clears buf
	}

	cout << listing << endl;;

}

int getFileSize(const char* filename) {
	int file_size;
	ifstream in_file(filename, ios::binary | ios::ate);

	if(!in_file) {
		file_size = -1;
	} else {
		file_size = in_file.tellg();
	}
	in_file.close();

	return file_size;
}
