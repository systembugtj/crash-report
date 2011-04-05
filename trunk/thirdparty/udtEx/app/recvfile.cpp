#ifndef WIN32
#include <arpa/inet.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include "udt.h"

using namespace std;

int main( )
{

	// use this function to initialize the UDT library
	UDT::startup();

	UDTSOCKET fhandle = UDT::socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(short(atoi("9000")));
#ifndef WIN32
	if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0)
#else
	if (INADDR_NONE == (serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1")))
#endif
	{
		cout << "incorrect network address.\n";
		return 0;
	}
	memset(&(serv_addr.sin_zero), '\0', 8);

	if (UDT::ERROR == UDT::connect(fhandle, (sockaddr*)&serv_addr, sizeof(serv_addr)))
	{
		cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
		return 0;
	}

	// send name information of the requested file
	char* filename="C:\\Documents\ and\ Settings\\yesp2\\桌面\\test.txt";
	int len = strlen(filename);

	if (UDT::ERROR == UDT::send(fhandle, (char*)&len, sizeof(int), 0))
	{
		cout << "send: " << UDT::getlasterror().getErrorMessage() << endl;
		return 0;
	}

	if (UDT::ERROR == UDT::send(fhandle, filename, len, 0))
	{
		cout << "send: " << UDT::getlasterror().getErrorMessage() << endl;
		return 0;
	}

	// get size information
	int64_t size;

	if (UDT::ERROR == UDT::recv(fhandle, (char*)&size, sizeof(int64_t), 0))
	{
		cout << "send: " << UDT::getlasterror().getErrorMessage() << endl;
		return 0;
	}

	cout<<"file size = "<<size<<endl;

	// receive the file
	char* savefile="C:\\Documents\ and\ Settings\\yesp2\\桌面\\test_save.txt";
	//fstream ofs(savefile, ios::out | ios::binary | ios::trunc);
	FILE* fp=fopen(savefile ,"w+");
	if (fp==NULL)
	{
		cout<<"fail to open file "<<endl;
		return -1;
	}

	int64_t recvsize; 

	int64_t leftsize=size;
	const int64_t block_size=1024*1024*10;//10M/per time
	int64_t offset=0;
	while (leftsize>0)
	{
		int64_t thisblock=(leftsize> block_size)? block_size:leftsize;

		if (UDT::ERROR == (recvsize = UDT::myrecvfile(fhandle, fp, offset, thisblock)))
		{
			cout << "recvfile: " << UDT::getlasterror().getErrorMessage() << endl;
			return 0;
		}
		leftsize-=recvsize;
		offset += recvsize;
		cout<<".";//代表进度
	}

	UDT::close(fhandle);

	fclose(fp);//edited by yesp
	//ofs.close();

	// use this function to release the UDT library
	UDT::cleanup();

	return 1;
}
