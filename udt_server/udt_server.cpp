#include "udt_server.h"
#include <string>
using namespace std;

CrashReportUdtServer::CrashReportUdtServer(const std::string& port, const std::string& save_dir) {
  port_ = port;
  save_dir_ = save_dir;
  // use this function to initialize the UDT library
  UDT::startup();
}

CrashReportUdtServer::~CrashReportUdtServer() {
  UDT::close( listen_sock_);
  // use this function to release the UDT library
  UDT::cleanup();

}

bool CrashReportUdtServer::Listen() {
  addrinfo hints;
  addrinfo* res;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if (0 != getaddrinfo(NULL, port_.c_str(), &hints, &res)) {
    cout << "illegal port number or port is busy.\n" << endl;
    return false;
  }
  listen_sock_ = UDT::socket(res->ai_family, res->ai_socktype,
      res->ai_protocol);
  if (UDT::ERROR == UDT::bind(listen_sock_, res->ai_addr, res->ai_addrlen)) {
    cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
    return false;
  }
  freeaddrinfo(res);
  cout << "server is ready at port: " << port_ << endl;
  if (UDT::ERROR == UDT::listen(listen_sock_, 10)) {
    cout << "listen: " << UDT::getlasterror().getErrorMessage() << endl;
    return false;
  }
  return true;
}

void CrashReportUdtServer::Run() {
  Listen();
  sockaddr_storage clientaddr;
  int addrlen = sizeof(clientaddr);
  UDTSOCKET recver;
  while (true) {
    if (UDT::INVALID_SOCK == (recver = UDT::accept(listen_sock_,
        (sockaddr*) &clientaddr, &addrlen))) {
      cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
      continue;
    }
    char clienthost[NI_MAXHOST];
    char clientservice[NI_MAXSERV];
    getnameinfo((sockaddr *) &clientaddr, addrlen, clienthost,
        sizeof(clienthost), clientservice, sizeof(clientservice),
        NI_NUMERICHOST | NI_NUMERICSERV);
    cout << "new connection: " << clienthost << ":" << clientservice << endl;

    CreateThread(NULL, 0, ReceiveCrashReport, new UDTSOCKET(recver), 0, NULL);
  }
}

DWORD WINAPI CrashReportUdtServer::ReceiveCrashReport(LPVOID usocket) {
  UDTSOCKET recver = *(UDTSOCKET*) usocket;
  delete (UDTSOCKET*) usocket;
  // TODO(yesp) : 使用该socket接收client发送的崩溃报告数据。并发送接收成功的反馈信息。
  UDT::close(recver);
  return 0;
}
