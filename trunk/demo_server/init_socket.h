//  Copyright 2011 . All Rights Reserved.
//  Author: yeshunping@gmail.com (Shunping Ye)
#ifndef INIT_SOCKET_H_
#define INIT_SOCKET_H_
#include <stdlib.h>
#include <winsock2.h>

#pragma comment(lib, "WS2_32")	// Á´½Óµ½WS2_32.lib
class CInitSock {
public:
  CInitSock(BYTE minorVer = 2, BYTE majorVer = 2) {
    WSADATA wsaData;
    WORD sockVersion = MAKEWORD(minorVer, majorVer);
    if (WSAStartup(sockVersion, &wsaData) != 0) {
      exit(0);
    }
  }
  ~CInitSock() {
    WSACleanup();
  }
};
#endif //  INIT_SOCKET_H_
