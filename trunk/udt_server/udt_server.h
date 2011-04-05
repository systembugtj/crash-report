#ifndef UDT_SERVER_H_
#define UDT_SERVER_H_

#include <iostream>
#include <string>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include "udt/udt.h"

class CrashReportUdtServer {
public:
  CrashReportUdtServer(const std::string& port, const std::string& save_dir);
  ~CrashReportUdtServer();
  bool Listen();
  void Run();
  static DWORD WINAPI ReceiveCrashReport(LPVOID usocket);
private:
  UDTSOCKET listen_sock_;
  std::string save_dir_;
  std::string port_;
};

#endif  // UDT_SERVER_H_
